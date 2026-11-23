// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PostgresServer.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <stdexcept>

namespace {

bool runProgram(const QString& program,
                const QStringList& arguments,
                int timeoutMs,
                QString* output = nullptr) {
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(10000)) {
        if (output) {
            *output = process.errorString();
        }
        return false;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        if (output) {
            *output = "El proceso excedió el tiempo de espera.";
        }
        return false;
    }

    if (output) {
        *output = QString::fromLocal8Bit(process.readAllStandardError());
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void writeFile(const QString& path, const QByteArray& contents) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || file.write(contents) != contents.size()
        || !file.flush()) {
        throw std::runtime_error(
            QString("No se pudo escribir la configuración PostgreSQL en %1.")
                .arg(path).toStdString()
        );
    }
}

void validateListener(const QString& addresses) {
    static const QRegularExpression allowed(R"(^[A-Za-z0-9:.,*_]+$)");
    if (addresses.isEmpty() || !allowed.match(addresses).hasMatch()) {
        throw std::runtime_error("DB_LISTEN_ADDRESSES contiene un valor inválido.");
    }
}

QString makePgHba(const QString& allowedCidrs) {
    QString contents =
        "local all all scram-sha-256\n"
        "host all ph_bootstrap 127.0.0.1/32 scram-sha-256\n"
        "host all ph_bootstrap ::1/128 scram-sha-256\n"
        "host all all 127.0.0.1/32 scram-sha-256\n"
        "host all all ::1/128 scram-sha-256\n";

    const QStringList cidrs = allowedCidrs.split(',', Qt::SkipEmptyParts);
    static const QRegularExpression validCidr(R"(^[0-9A-Fa-f:.]+/[0-9]{1,3}$)");
    for (const QString& cidrValue : cidrs) {
        const QString cidr = cidrValue.trimmed();
        if (!validCidr.match(cidr).hasMatch()) {
            throw std::runtime_error("DB_ALLOWED_CIDRS contiene una red CIDR inválida.");
        }
        contents += "host all ph_bootstrap " + cidr + " reject\n";
        contents += "host all all " + cidr + " scram-sha-256\n";
    }

    return contents;
}

void configureCluster(const QString& dataDirectory,
                      const QString& port,
                      const QString& listenAddresses,
                      const QString& allowedCidrs) {
    const QByteArray configuration =
        QString(
            "port = %1\n"
            "listen_addresses = '%2'\n"
            "unix_socket_directories = ''\n"
            "password_encryption = 'scram-sha-256'\n"
        ).arg(port, listenAddresses).toUtf8();
    writeFile(QDir(dataDirectory).filePath("postgresql.auto.conf"), configuration);
    writeFile(QDir(dataDirectory).filePath("pg_hba.conf"), makePgHba(allowedCidrs).toUtf8());
}

QString generateBootstrapPassword() {
    QString password;
    for (int i = 0; i < 8; ++i) {
        password += QString::number(QRandomGenerator::system()->generate(), 16).rightJustified(8, '0');
    }
    return password;
}

QString initializeCluster(const QString& initdb, const QString& dataDirectory) {
    const QString password = generateBootstrapPassword();
    QTemporaryFile passwordFile;
    if (!passwordFile.open()
        || passwordFile.write(password.toUtf8()) < 0
        || !passwordFile.flush()) {
        throw std::runtime_error("No se pudo preparar la contraseña inicial de PostgreSQL.");
    }

    QString output;
    const QStringList arguments{
        "-D", dataDirectory,
        "-U", "ph_bootstrap",
        "--pwfile=" + passwordFile.fileName(),
        "--auth-local=scram-sha-256",
        "--auth-host=scram-sha-256",
        "--encoding=UTF8"
    };
    if (!runProgram(initdb, arguments, 180000, &output)) {
        throw std::runtime_error(
            QString("No se pudo inicializar PostgreSQL: %1")
                .arg(output.trimmed()).toStdString()
        );
    }

    QFile storedPassword(QDir(dataDirectory).filePath("postgres-bootstrap-password"));
    if (!storedPassword.open(QIODevice::WriteOnly | QIODevice::NewOnly)
        || storedPassword.write(password.toUtf8()) != password.size()
        || !storedPassword.flush()
        || !storedPassword.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        throw std::runtime_error("No se pudo guardar la credencial local del servidor PostgreSQL.");
    }
    return password;
}

}  // namespace

QString PostgresServer::findProgram(const QString& name,
                                    const QString& applicationDirectory,
                                    const QString& binaryDirectory) {
    QStringList candidates;
    if (!binaryDirectory.isEmpty()) {
        candidates << QDir(binaryDirectory).filePath(name);
#ifdef Q_OS_WIN
        candidates << QDir(binaryDirectory).filePath(name + ".exe");
#endif
    }
    candidates << QDir(applicationDirectory).filePath(name)
               << QDir(applicationDirectory).filePath("postgresql/bin/" + name);
#ifdef Q_OS_WIN
    candidates << QDir(applicationDirectory).filePath(name + ".exe")
               << QDir(applicationDirectory).filePath("postgresql/bin/" + name + ".exe");
#endif

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    QString executable = QStandardPaths::findExecutable(name);
#ifdef Q_OS_WIN
    if (executable.isEmpty()) {
        executable = QStandardPaths::findExecutable(name + ".exe");
    }
#endif
    return executable;
}

QString PostgresServer::ensureRunning(const QString& applicationDirectory,
                                      const QString& dataDirectory,
                                      const QString& binaryDirectory,
                                      const QString& port,
                                      const QString& listenAddresses,
                                      const QString& allowedCidrs) {
    validateListener(listenAddresses);

    const QString initdb = findProgram("initdb", applicationDirectory, binaryDirectory);
    const QString pgCtl = findProgram("pg_ctl", applicationDirectory, binaryDirectory);
    if (initdb.isEmpty() || pgCtl.isEmpty()) {
        throw std::runtime_error(
            "No se encontraron initdb y pg_ctl. Instale PostgreSQL o configure DB_BIN_DIR."
        );
    }

    QDir directory;
    if (!directory.mkpath(dataDirectory)) {
        throw std::runtime_error("No se pudo crear el directorio local de datos PostgreSQL.");
    }

    const QString versionFile = QDir(dataDirectory).filePath("PG_VERSION");
    QString bootstrapPassword;
    if (!QFile::exists(versionFile)) {
        const QStringList existingEntries = QDir(dataDirectory).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot
        );
        if (!existingEntries.isEmpty()) {
            throw std::runtime_error(
                "El directorio PostgreSQL existe pero no contiene un clúster válido."
            );
        }
        bootstrapPassword = initializeCluster(initdb, dataDirectory);
    } else {
        QFile passwordFile(QDir(dataDirectory).filePath("postgres-bootstrap-password"));
        if (!passwordFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error(
                "Falta la credencial local del clúster PostgreSQL. No se modificará un clúster ajeno."
            );
        }
        bootstrapPassword = QString::fromUtf8(passwordFile.readAll()).trimmed();
        if (bootstrapPassword.isEmpty()) {
            throw std::runtime_error("La credencial local del clúster PostgreSQL está vacía.");
        }
    }

    configureCluster(dataDirectory, port, listenAddresses, allowedCidrs);

    QString output;
    if (runProgram(pgCtl, {"-D", dataDirectory, "status"}, 15000)) {
        if (!runProgram(pgCtl, {"-D", dataDirectory, "reload"}, 15000, &output)) {
            throw std::runtime_error(
                QString("No se pudo recargar la configuración PostgreSQL: %1")
                    .arg(output.trimmed()).toStdString()
            );
        }
        return bootstrapPassword;
    }

    const QString logFile = QDir(dataDirectory).filePath("server.log");
    if (!runProgram(
            pgCtl,
            {"-D", dataDirectory, "-l", logFile, "-w", "-t", "60", "start"},
            75000,
            &output
        )) {
        throw std::runtime_error(
            QString("No se pudo iniciar PostgreSQL: %1").arg(output.trimmed()).toStdString()
        );
    }
    return bootstrapPassword;
}

bool PostgresServer::stop(const QString& applicationDirectory,
                          const QString& dataDirectory,
                          const QString& binaryDirectory,
                          QString* errorMessage) {
    const QString pgCtl = findProgram("pg_ctl", applicationDirectory, binaryDirectory);
    if (pgCtl.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "No se encontró pg_ctl para apagar PostgreSQL.";
        }
        return false;
    }

    QString output;
    if (!runProgram(
            pgCtl,
            {"-D", dataDirectory, "-m", "fast", "-w", "-t", "60", "stop"},
            75000,
            &output
        )) {
        if (errorMessage) {
            *errorMessage = output.trimmed().isEmpty()
                ? "pg_ctl no pudo apagar PostgreSQL."
                : output.trimmed();
        }
        return false;
    }

    return true;
}
