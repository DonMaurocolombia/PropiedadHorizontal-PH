// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/* MiPrograma - Descripción breve
   Copyright (C) 2026 Oscar Mauricio Bonil P.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "DbManager.hpp"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <stdexcept>
#include <SQLiteCpp/SQLiteCpp.h>
#include <sqlite3.h>  // SQLiteCpp ya lo incluye, pero por claridad
#include <QLoggingCategory>


DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const std::string& dbPath) {
    try {
        // Abre la BD o la crea si no existe
        m_db = std::make_unique<SQLite::Database>(
            dbPath,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );

        // Activar llaves foráneas y modo WAL (mayor velocidad y seguridad ante cortes de luz)
        m_db->exec("PRAGMA foreign_keys = ON;");
        m_db->exec("PRAGMA journal_mode = WAL;");

        // Verificar si la base de datos está vacía (sin tablas)
        int tableCount = m_db->execAndGet(
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='copropiedad_config';"
        ).getInt();

        bool resp = true;
        if (tableCount == 0) {
            qInfo() << "Base de datos nueva detectada. Ejecutando schema.sql...";
            if ( runSchemaFromResources(":/db/schema.sql") ) {
                qInfo() << "Cargando registros puc_colombia.sql...";
                resp = runSchemaFromResources(":/db/puc_colombia.sql") ? true: false;
            } else {
                resp = false;
            }
        }

        return resp;
    } catch (const std::exception& e) {
        qCritical() << "Error inicializando SQLite:" << e.what();
        return false;
    }
}

SQLite::Database& DatabaseManager::getDb() {
    if (!m_db) {
        throw std::runtime_error("La base de datos no ha sido inicializada.");
    }
    return *m_db;
}

bool DatabaseManager::runSchemaFromResources(const std::string& dbName) {
    QFile schemaFile(QString::fromStdString(dbName));
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("No se pudo abrir " + dbName + " desde los recursos.");
    }

    QTextStream in(&schemaFile);
    std::string sqlScript = in.readAll().toStdString();

    try {
        SQLite::Transaction transaction(*m_db);
        // if (!ejecutarSchema(m_db.get(), sqlScript, dbName)) {
        if (!ejecutarSchema(sqlScript, dbName)) {
            return false;  // rollback automático al salir del scope
        }
        transaction.commit();
    } catch (const std::exception& e) {
        qCritical() << "Error ejecutando " << dbName << ": " << e.what();
        return false;
    }   
    /*
    try {
        // Ejecutar todo el esquema dentro de una transacción atómica
        SQLite::Transaction transaction(*m_db);
        m_db->exec(sqlScript);
        transaction.commit();
    } catch (const std::exception& e) {
        qCritical() << "Error ejecutando " << dbName << ": " << e.what();
        return false;
    }
    */

    qInfo() << "Esquema de base de datos [" + dbName + "] creado exitosamente.";
    return true;
}

bool DatabaseManager::ejecutarSchema(const std::string& sqlScript, const std::string& dbName) {
    sqlite3* rawDb = m_db->getHandle();  // SQLiteCpp expone el handle crudo
    const char* remaining = sqlScript.c_str();
    int stmtIndex = 0;

    while (true) {
        // Saltar whitespace / comentarios al inicio
        while (*remaining && (isspace(static_cast<unsigned char>(*remaining))
                              || *remaining == '-' || *remaining == '/')) {
            // Si es comentario de línea (--), saltar hasta \n
            if (remaining[0] == '-' && remaining[1] == '-') {
                while (*remaining && *remaining != '\n') remaining++;
            }
            else if (remaining[0] == '/' && remaining[1] == '*') { // Si es comentario bloque (/* ... */), saltar hasta */
                remaining += 2;
                while (*remaining && !(remaining[0] == '*' && remaining[1] == '/')) remaining++;
                remaining += 2;
            }
            else remaining++;
        }

        if (!*remaining) break;  // fin del script

        // Preparar la siguiente sentencia
        sqlite3_stmt* stmt = nullptr;
        const char* tail = nullptr;
        int rc = sqlite3_prepare_v2(rawDb, remaining, -1, &stmt, &tail);

        // Extraer el texto de la sentencia que acabamos de preparar
        std::string stmtText(remaining, tail ? tail - remaining : strlen(remaining));

        if (rc != SQLITE_OK) {
            // Error de sintaxis: tail apunta a donde el parser se detuvo
            std::string errorFragment(remaining, tail ? tail - remaining : strlen(remaining));
            qCritical() << "Error sintáctico en sentencia #" << (stmtIndex + 1)
                        << " de" << dbName << ":\n"
                        << "  SQL: " << errorFragment << "\n"
                        << "  Error: " << sqlite3_errmsg(rawDb);
            return false;
        }

        if (!stmt) {
            // Solo whitespace/comentarios — avanzar a tail
            if (!tail || tail == remaining) break;  // evitar loop infinito
            remaining = tail;
            continue;
        }

        // Ejecutar
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            qCritical() << "Error ejecutando sentencia #" << (stmtIndex + 1)
                        << " de" << dbName << ":\n"
                        << "  SQL: " << stmtText << "\n"
                        << "  Error: " << sqlite3_errmsg(rawDb);
            return false;
        }

        stmtIndex++;
        remaining = tail;
    }

    return true;
}
