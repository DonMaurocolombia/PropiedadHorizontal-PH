// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
   Administrador - PH: Administrador PH busca facilitar y organizar la 
   administración de copropiedades mediante una plataforma que centralice 
   la información, los documentos y los procesos administrativos, bajo la 
   Licencia GPL-3.0-or-later.
   
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
/*
        if (resp) {
            for (const char* nombreColumna : {"es_visitante", "es_residente"}) {
                bool existeColumna = false;
                SQLite::Statement columnas(*m_db, "PRAGMA table_info(terceros)");
                while (columnas.executeStep()) {
                    if (columnas.getColumn(1).getString() == nombreColumna) {
                        existeColumna = true;
                        break;
                    }
                }
                if (!existeColumna) {
                    m_db->exec(
                        "ALTER TABLE terceros ADD COLUMN " + std::string(nombreColumna)
                        + " INTEGER NOT NULL DEFAULT 0 CHECK (" + std::string(nombreColumna)
                        + " IN (0, 1))"
                    );
                }
            }
            resp = runSchemaFromResources(":/db/migrations/002_facturas_servicio_contexto.sql");
        }

        if (resp) {
            bool tieneTipoConcepto = false;
            SQLite::Statement columnas(*m_db, "PRAGMA table_info(conceptos_cobro)");
            while (columnas.executeStep()) {
                if (columnas.getColumn(1).getString() == "tipo_concepto") {
                    tieneTipoConcepto = true;
                    break;
                }
            }
            if (!tieneTipoConcepto) {
                m_db->exec(
                    "ALTER TABLE conceptos_cobro ADD COLUMN tipo_concepto "
                    "TEXT NOT NULL DEFAULT 'CUENTA_MENSUAL' "
                    "CHECK (tipo_concepto IN ('CUENTA_MENSUAL', 'SERVICIO'))"
                );
            }
            resp = runSchemaFromResources(":/db/migrations/003_cobros_servicios.sql");
        }

        if (resp) {
            SQLite::Statement migration(*m_db, R"(
                SELECT COUNT(*) FROM facturas_servicio_migraciones WHERE version = 3
            )");
            migration.executeStep();
            if (migration.getColumn(0).getInt() == 0) {
                SQLite::Transaction transaction(*m_db);
                m_db->exec(R"(
                    UPDATE conceptos_cobro
                    SET tipo_concepto = 'SERVICIO',
                        genera_interes_mora = 0,
                        aplica_iva = 0,
                        porcentaje_iva = 0.0
                    WHERE codigo IN ('SALON_SOC', 'PARQ_VISI', 'BODE_DEPO', 'PARQ_COMU')
                )");
                m_db->exec(R"(
                    INSERT OR IGNORE INTO facturas_servicio_cobro (
                        factura_id, concepto_id, estado
                    )
                    SELECT f.id,
                           cc.id,
                           'BORRADOR'
                    FROM facturas_servicio f
                    LEFT JOIN facturas_servicio_contexto ctx
                        ON ctx.factura_id = f.id
                    LEFT JOIN conceptos_cobro cc
                        ON cc.codigo = CASE ctx.tipo_servicio
                            WHEN 'PARQUEADERO_VEHICULO' THEN 'PARQ_VISI'
                            WHEN 'ALQUILER_SALON_COMUNAL' THEN 'SALON_SOC'
                            ELSE NULL
                        END
                )");
                m_db->exec("INSERT INTO facturas_servicio_migraciones (version) VALUES (3)");
                transaction.commit();
            }
        }

        if (resp) {
            SQLite::Statement migration(*m_db, R"(
                SELECT COUNT(*) FROM facturas_servicio_migraciones WHERE version = 2
            )");
            migration.executeStep();
            if (migration.getColumn(0).getInt() == 0) {
                SQLite::Transaction transaction(*m_db);
                m_db->exec(R"(
                    UPDATE facturas_servicio_detalle
                    SET iva_tasa_puntos_base = 0, iva_cop = 0
                )");

                m_db->exec(R"(
                    UPDATE facturas_servicio
                    SET iva_total_cop = 0, total_cop = subtotal_cop
                )");

                m_db->exec(
                    "INSERT INTO facturas_servicio_migraciones (version) VALUES (2)"
                );
                transaction.commit();
            }
        }
 */

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
        if (!ejecutarSchema(sqlScript, dbName)) {
            return false;  // rollback automático al salir del scope
        }
        transaction.commit();
    } catch (const std::exception& e) {
        qCritical() << "Error ejecutando " << dbName << ": " << e.what();
        return false;
    }   

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
