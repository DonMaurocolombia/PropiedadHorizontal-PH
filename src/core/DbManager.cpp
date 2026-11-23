// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DbManager.hpp"

#include "PostgresServer.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDebug>

#include <pqxx/pqxx>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

QMap<QString, QString> readEnvironmentFile(const QString& applicationDirectory) {
    const QStringList candidates{
        QDir(applicationDirectory).filePath("datos.env"),
        QDir::current().filePath("datos.env"),
        QDir(applicationDirectory).filePath("../datos.env")
    };

    QString environmentPath;
    for (const QString& candidate : candidates) {
        const QString absolute = QDir::cleanPath(QFileInfo(candidate).absoluteFilePath());
        if (QFile::exists(absolute)) {
            environmentPath = absolute;
            break;
        }
    }

    if (environmentPath.isEmpty()) {
        throw std::runtime_error(
            "No se encontró datos.env junto a la aplicación ni en el directorio de trabajo."
        );
    }

    QFile file(environmentPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("No se pudo leer datos.env.");
    }

    QMap<QString, QString> values;
    QTextStream stream(&file);
    static const QRegularExpression assignment(
        R"(^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$)"
    );
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const QRegularExpressionMatch match = assignment.match(line);
        if (!match.hasMatch()) {
            continue;
        }

        QString value = match.captured(2).trimmed();
        if (value.size() >= 2
            && ((value.startsWith('"') && value.endsWith('"'))
                || (value.startsWith('\'') && value.endsWith('\'')))) {
            value = value.mid(1, value.size() - 2);
        } else {
            const qsizetype commentPosition = value.indexOf('#');
            if (commentPosition >= 0) {
                value = value.left(commentPosition).trimmed();
            }
        }
        values.insert(match.captured(1), value);
    }

    return values;
}

QString connectionOption(const QString& key, const QString& value) {
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('\'', "\\'");
    return key + "='" + escaped + "'";
}

QString connectionString(const QString& host,
                         const QString& port,
                         const QString& databaseName,
                         const QString& username,
                         const QString& password,
                         const QString& sslMode) {
    return QString("%1 %2 %3 %4 %5 %6")
        .arg(connectionOption("host", host),
             connectionOption("port", port),
             connectionOption("dbname", databaseName),
             connectionOption("user", username),
             connectionOption("password", password),
             connectionOption("sslmode", sslMode));
}

std::string quoteIdentifier(const std::string& identifier) {
    std::string quoted = "\"";
    for (const char character : identifier) {
        if (character == '"') {
            quoted += "\"\"";
        } else {
            quoted += character;
        }
    }
    quoted += '"';
    return quoted;
}

void ensureDatabaseAndApplicationRole(const QString& host,
                                      const QString& port,
                                      const QString& databaseName,
                                      const QString& username,
                                      const QString& password,
                                      const QString& bootstrapPassword,
                                      const QString& sslMode) {
    pqxx::connection connection(
        connectionString(
            host, port, "postgres", "ph_bootstrap", bootstrapPassword, sslMode
        ).toStdString()
    );
    pqxx::nontransaction transaction(connection);
    const std::string name = databaseName.toStdString();
    const std::string user = username.toStdString();

    const pqxx::result existingRole = transaction.exec(
        "SELECT 1 FROM pg_roles WHERE rolname = " + connection.quote(user)
    );
    if (existingRole.empty()) {
        transaction.exec(
            "CREATE ROLE " + quoteIdentifier(user)
            + " WITH LOGIN PASSWORD " + connection.quote(password.toStdString())
            + " NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION NOBYPASSRLS"
        );
    } else {
        transaction.exec(
            "ALTER ROLE " + quoteIdentifier(user)
            + " WITH LOGIN PASSWORD " + connection.quote(password.toStdString())
            + " NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION NOBYPASSRLS"
        );
    }

    const pqxx::result result = transaction.exec(
        "SELECT 1 FROM pg_database WHERE datname = " + connection.quote(name)
    );
    if (result.empty()) {
        transaction.exec(
            "CREATE DATABASE " + quoteIdentifier(name)
            + " OWNER " + quoteIdentifier(user)
        );
    } else {
        transaction.exec(
            "ALTER DATABASE " + quoteIdentifier(name)
            + " OWNER TO " + quoteIdentifier(user)
        );
    }
}

std::vector<std::string> splitSqlStatements(const std::string& script) {
    std::vector<std::string> statements;
    std::string statement;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inLineComment = false;
    int blockCommentDepth = 0;

    for (std::size_t i = 0; i < script.size(); ++i) {
        const char current = script[i];
        const char next = i + 1 < script.size() ? script[i + 1] : '\0';

        if (inLineComment) {
            statement += current;
            if (current == '\n') {
                inLineComment = false;
            }
            continue;
        }
        if (blockCommentDepth > 0) {
            statement += current;
            if (current == '/' && next == '*') {
                statement += script[++i];
                ++blockCommentDepth;
            } else if (current == '*' && next == '/') {
                statement += script[++i];
                --blockCommentDepth;
            }
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && current == '-' && next == '-') {
            statement += current;
            statement += script[++i];
            inLineComment = true;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && current == '/' && next == '*') {
            statement += current;
            statement += script[++i];
            blockCommentDepth = 1;
            continue;
        }

        if (current == '\'' && !inDoubleQuote) {
            statement += current;
            if (inSingleQuote && next == '\'') {
                statement += script[++i];
            } else {
                inSingleQuote = !inSingleQuote;
            }
            continue;
        }
        if (current == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            statement += current;
            continue;
        }

        if (current == ';' && !inSingleQuote && !inDoubleQuote) {
            if (!QString::fromStdString(statement).trimmed().isEmpty()) {
                statements.push_back(statement);
            }
            statement.clear();
        } else {
            statement += current;
        }
    }

    if (!QString::fromStdString(statement).trimmed().isEmpty()) {
        statements.push_back(statement);
    }
    return statements;
}

void executeSqlScript(Db::Database& database, const QString& resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(
            QString("No se pudo leer el recurso SQL %1.").arg(resourcePath).toStdString()
        );
    }

    const std::string script = QTextStream(&file).readAll().toStdString();
    for (const std::string& statement : splitSqlStatements(script)) {
        database.exec(statement);
    }
}

}  // namespace

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const QString& applicationDirectory) {
    m_serverMode = false;
    try {
        const QMap<QString, QString> environment = readEnvironmentFile(applicationDirectory);
        const QString mode = environment.value("DB_MODE", "server").trimmed().toLower();
        if (mode != "server" && mode != "client") {
            throw std::runtime_error("DB_MODE debe ser 'server' o 'client'.");
        }

        m_applicationDirectory = QDir(applicationDirectory).absolutePath();
        m_host = environment.value("DB_HOST", "127.0.0.1").trimmed();
        m_port = environment.value("DB_PORT", mode == "server" ? "5433" : "5432").trimmed();
        m_databaseName = environment.value("DB_NAME").trimmed();
        m_username = environment.value("DB_USER").trimmed();
        m_password = environment.value("DB_PASSWORD");
        m_sslMode = environment.value("DB_SSLMODE", "prefer").trimmed();
        m_binaryDirectory = environment.value("DB_BIN_DIR").trimmed();

        bool validPort = false;
        const int portNumber = m_port.toInt(&validPort);
        if (!validPort || portNumber < 1 || portNumber > 65535) {
            throw std::runtime_error("DB_PORT debe ser un puerto válido.");
        }

        static const QRegularExpression identifier(R"(^[A-Za-z_][A-Za-z0-9_]*$)");
        if (!identifier.match(m_databaseName).hasMatch()
            || !identifier.match(m_username).hasMatch()) {
            throw std::runtime_error(
                "DB_NAME y DB_USER solo pueden contener letras, números y guion bajo."
            );
        }
        if (m_password.isEmpty()) {
            throw std::runtime_error("DB_PASSWORD no puede estar vacío.");
        }

        static const QStringList sslModes{
            "disable", "allow", "prefer", "require", "verify-ca", "verify-full"
        };
        if (!sslModes.contains(m_sslMode)) {
            throw std::runtime_error("DB_SSLMODE no contiene un modo SSL válido.");
        }

        if (mode == "server") {
            if (m_databaseName == "postgres") {
                throw std::runtime_error(
                    "DB_NAME debe ser una base propia de la aplicación, no 'postgres'."
                );
            }
            if (m_username == "ph_bootstrap") {
                throw std::runtime_error("DB_USER no puede ser el usuario interno PostgreSQL.");
            }
            const QString normalizedHost = m_host.toLower();
            if (normalizedHost != "localhost"
                && normalizedHost != "127.0.0.1"
                && normalizedHost != "::1") {
                throw std::runtime_error(
                    "En modo server, DB_HOST debe apuntar a la máquina local."
                );
            }
            if (portNumber != 5433) {
                throw std::runtime_error(
                    "El servidor PostgreSQL local debe usar DB_PORT=5433."
                );
            }

            const QString dataDirectory =
                QDir(m_applicationDirectory).filePath("postgres-data");
            const QString bootstrapPassword = PostgresServer::ensureRunning(
                m_applicationDirectory,
                dataDirectory,
                m_binaryDirectory,
                m_port,
                environment.value("DB_LISTEN_ADDRESSES", "127.0.0.1,::1").trimmed(),
                environment.value("DB_ALLOWED_CIDRS", "127.0.0.1/32,::1/128").trimmed()
            );

            ensureDatabaseAndApplicationRole(
                m_host, m_port, m_databaseName, m_username, m_password,
                bootstrapPassword, m_sslMode
            );
        }

        m_db = std::make_unique<Db::Database>(
            connectionString(
                m_host, m_port, m_databaseName, m_username, m_password, m_sslMode
            ).toStdString()
        );

        const bool schemaExists =
            m_db->execAndGet("SELECT to_regclass('public.copropiedad_config') IS NOT NULL")
                .getString() == "t";
        if (!schemaExists) {
            if (mode == "client") {
                throw std::runtime_error(
                    "La base de datos no tiene el esquema de la aplicación; inicialícelo en el servidor."
                );
            }
            qInfo() << "Base PostgreSQL nueva detectada. Cargando esquema y datos iniciales.";
            runSchemaFromResources();
        }

        {
            Db::Transaction transaction(*m_db);
            m_db->exec("SELECT pg_advisory_xact_lock(735842195117331)");
            executeSqlScript(*m_db, ":/db/migrations/003_permisos_paginas.sql");
            transaction.commit();
        }

        m_lastError.clear();
        m_serverMode = mode == "server";
        return true;
    } catch (const std::exception& error) {
        m_db.reset();
        m_lastError = QString::fromUtf8(error.what());
        qCritical() << "Error inicializando PostgreSQL:" << m_lastError;
        return false;
    }
}

Db::Database& DatabaseManager::getDb() {
    if (!m_db) {
        throw std::runtime_error("La conexión PostgreSQL no ha sido inicializada.");
    }
    return *m_db;
}

QString DatabaseManager::lastError() const {
    return m_lastError;
}

bool DatabaseManager::isServerMode() const {
    return m_serverMode;
}

bool DatabaseManager::stopLocalServer(QString* errorMessage) const {
    if (!m_serverMode) {
        if (errorMessage) {
            *errorMessage = "No se puede apagar PostgreSQL desde el modo cliente.";
        }
        return false;
    }

    const QString dataDirectory =
        QDir(m_applicationDirectory).filePath("postgres-data");
    return PostgresServer::stop(
        m_applicationDirectory, dataDirectory, m_binaryDirectory, errorMessage
    );
}

bool DatabaseManager::createBackup(const QString& destination, QString* errorMessage) const {
    const QString pgDump =
        PostgresServer::findProgram("pg_dump", m_applicationDirectory, m_binaryDirectory);
    if (pgDump.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "No se encontró pg_dump. Instale las herramientas cliente de PostgreSQL.";
        }
        return false;
    }

    QTemporaryFile temporaryFile(
        QFileInfo(destination).dir().filePath(".ph-backup-XXXXXX")
    );
    if (!temporaryFile.open()) {
        if (errorMessage) {
            *errorMessage = "No se pudo preparar un archivo temporal para el respaldo.";
        }
        return false;
    }
    const QString temporaryPath = temporaryFile.fileName();
    temporaryFile.close();
    temporaryFile.setAutoRemove(false);
    QFile::remove(temporaryPath);

    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("PGPASSWORD", m_password);
    process.setProcessEnvironment(environment);
    process.start(pgDump, {
        "--host", m_host,
        "--port", m_port,
        "--username", m_username,
        "--format=custom",
        "--file", temporaryPath,
        m_databaseName
    });

    if (!process.waitForStarted(10000)) {
        QFile::remove(temporaryPath);
        if (errorMessage) {
            *errorMessage = process.errorString();
        }
        return false;
    }
    if (!process.waitForFinished(180000)) {
        process.kill();
        process.waitForFinished();
        QFile::remove(temporaryPath);
        if (errorMessage) {
            *errorMessage = "pg_dump excedió el tiempo de espera.";
        }
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        QFile::remove(temporaryPath);
        if (errorMessage) {
            *errorMessage = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        }
        return false;
    }

    const QString previousPath = destination + ".previous";
    const bool hadPreviousFile = QFile::exists(destination);
    if (hadPreviousFile) {
        QFile::remove(previousPath);
        if (!QFile::rename(destination, previousPath)) {
            QFile::remove(temporaryPath);
            if (errorMessage) {
                *errorMessage = "No se pudo reemplazar el archivo de respaldo existente.";
            }
            return false;
        }
    }
    if (!QFile::rename(temporaryPath, destination)) {
        if (hadPreviousFile) {
            QFile::rename(previousPath, destination);
        }
        QFile::remove(temporaryPath);
        if (errorMessage) {
            *errorMessage = "No se pudo guardar el respaldo PostgreSQL.";
        }
        return false;
    }
    if (hadPreviousFile) {
        QFile::remove(previousPath);
    }

    return true;
}

void DatabaseManager::runSchemaFromResources() {
    Db::Transaction transaction(*m_db);
    executeSqlScript(*m_db, ":/db/schema.sql");
    executeSqlScript(*m_db, ":/db/puc_colombia.sql");
    transaction.commit();
}
