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


#include "AuthManager.hpp"
#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <stdexcept>
#include <format>

std::string AuthManager::generarPasswordHash(const std::string& password) {
    // Generar salt aleatorio de 16 bytes
    QByteArray salt;
    for (int i = 0; i < 4; ++i) {
        quint32 val = QRandomGenerator::global()->generate();
        salt.append(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    QString saltHex = salt.toHex();
    QByteArray data = (saltHex + QString::fromStdString(password)).toUtf8();
    QString hashHex = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();

    // Formato almacenado: "saltHex$hashHex"
    return (saltHex + "$" + hashHex).toStdString();
}

bool AuthManager::verificarPassword(const std::string& password, const std::string& hashAlmacenado) {
    QString almacenado = QString::fromStdString(hashAlmacenado);
    QStringList partes = almacenado.split('$');
    if (partes.size() != 2) return false;

    QString saltHex = partes[0];
    QString hashEsperado = partes[1];

    QByteArray data = (saltHex + QString::fromStdString(password)).toUtf8();
    QString hashCalculado = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();

    return hashCalculado == hashEsperado;
}

void AuthManager::asegurarUsuarioAdminInicial() {
    try {
        auto& db = DatabaseManager::instance().getDb();
        int count = db.execAndGet("SELECT COUNT(*) FROM usuarios WHERE id > 0;").getInt();
        if (count > 0) return;

        contrasenaSystemInicial();

        // Buscar ID del rol ADMIN
        SQLite::Statement rolStmt(db, "SELECT id FROM roles WHERE nombre = 'ADMIN';");
        int rolAdminId = 1;
        if (rolStmt.executeStep()) {
            rolAdminId = rolStmt.getColumn(0).getInt();
        }

        //std::string hashInicial = generarPasswordHash("Admin123*");
        SQLite::Statement ins(db, R"(
            INSERT INTO usuarios (rol_id, username, password_hash, nombre_completo, email, activo)
            VALUES (?, 'admin', ?, 'Administrador Principal', 'admin@copropiedad.local', 1)
        )");
        ins.bind(1, rolAdminId);
        // ins.bind(2, hashInicial);
        ins.bind(2, generarPasswordHash("Admin123*"));
        ins.exec();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error asegurando usuario Admin inicial: " << e.what();
    }
}

void AuthManager::contrasenaSystemInicial() {
    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement ins(db, R"(
            UPDATE usuarios SET password_hash = ? WHERE id = ?
        )");
        ins.bind(1, generarPasswordHash("S1ST3M&*!123*"));
        ins.bind(2, 0);
        ins.exec();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al cambiar la contrasena System inicial: " << e.what();
    }
}

std::optional<UsuarioSesion> AuthManager::iniciarSesion(
    const std::string& username, const std::string& password
) {
    UsuarioSesion sesion;

    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement query(db, R"(
            SELECT u.id, u.username, u.password_hash, u.nombre_completo, COALESCE(u.email, ''),
                u.rol_id, r.nombre, u.activo
            FROM usuarios u
            INNER JOIN roles r ON u.rol_id = r.id
            WHERE u.id > 0 AND u.username = ?
        )");
        query.bind(1, username);

        if (!query.executeStep()) {
            return std::nullopt;
        }

        bool activo = query.getColumn(7).getInt() == 1;
        if (!activo) {
            throw std::runtime_error("Este usuario se encuentra inactivo. Contacte al administrador.");
        }

        std::string hashAlmacenado = query.getColumn(2).getString();
        if (!verificarPassword(password, hashAlmacenado)) {
            return std::nullopt;
        }

        sesion.id = query.getColumn(0).getInt();
        sesion.username = query.getColumn(1).getString();
        sesion.nombreCompleto = query.getColumn(3).getString();
        sesion.email = query.getColumn(4).getString();
        sesion.rolId = query.getColumn(5).getInt();
        sesion.rolNombre = query.getColumn(6).getString();
        sesion.activo = true;

        // Actualizar fecha de último acceso y registrar ingreso en auditoría
        SQLite::Statement upd(db, "UPDATE usuarios SET ultimo_acceso = datetime('now', 'localtime') WHERE id = ?");
        upd.bind(1, sesion.id);
        upd.exec();

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'SEGURIDAD', 'LOGIN', 'usuarios', ?, 'Inicio de sesión exitoso')
        )");
        audit.bind(1, sesion.id);
        audit.bind(2, sesion.id);
        audit.exec();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al iniciar sesion: " << e.what();
    }

    return sesion;
}

std::vector<UsuarioSesion> AuthManager::listarUsuarios() {
    std::vector<UsuarioSesion> lista;

    try{
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement query(db, R"(
            SELECT u.id, u.username, u.nombre_completo, COALESCE(u.email, ''),
                u.rol_id, r.nombre, u.activo, COALESCE(u.ultimo_acceso, 'Nunca')
            FROM usuarios u
            INNER JOIN roles r ON u.rol_id = r.id
            WHERE u.id > 0
            ORDER BY u.id ASC
        )");

        while (query.executeStep()) {
            UsuarioSesion u;
            u.id = query.getColumn(0).getInt();
            u.username = query.getColumn(1).getString();
            u.nombreCompleto = query.getColumn(2).getString();
            u.email = query.getColumn(3).getString();
            u.rolId = query.getColumn(4).getInt();
            u.rolNombre = query.getColumn(5).getString();
            u.activo = query.getColumn(6).getInt() == 1;
            u.ultimoAcceso = query.getColumn(7).getString();
            lista.push_back(u);
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al listar usuarios: " << e.what();
    }

    return lista;
}

int AuthManager::crearUsuario(const UsuarioSesion& user, const std::string& password, int adminId) {
    if (user.username.empty() || user.nombreCompleto.empty()) {
        throw std::runtime_error("El nombre de usuario y nombre completo son obligatorios.");
    }

    if (password.size() < 6) {
        throw std::runtime_error("La contraseña debe tener al menos 6 caracteres.");
    }

    int newId;
    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Statement checkDup(db, "SELECT COUNT(*) FROM usuarios WHERE username = ?");
        checkDup.bind(1, user.username);
        checkDup.executeStep();
        if (checkDup.getColumn(0).getInt() > 0) {
            throw std::runtime_error("El nombre de usuario ya está en uso.");
        }

        SQLite::Transaction transaction(db);

        std::string hash = generarPasswordHash(password);
        SQLite::Statement ins(db, R"(
            INSERT INTO usuarios (rol_id, username, password_hash, nombre_completo, email, activo)
            VALUES (?, ?, ?, ?, ?, ?)
        )");
        ins.bind(1, user.rolId);
        ins.bind(2, user.username);
        ins.bind(3, hash);
        ins.bind(4, user.nombreCompleto);
        ins.bind(5, user.email);
        ins.bind(6, user.activo ? 1 : 0);
        ins.exec();

        newId = static_cast<int>(db.getLastInsertRowid());

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'SEGURIDAD', 'CREAR', 'usuarios', ?, ?)
        )");
        audit.bind(1, adminId);
        audit.bind(2, newId);
        audit.bind(3, std::format("Creación usuario '{}' con rol ID {}", user.username, user.rolId));
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al crear usuario: " << e.what();
    }

    return newId;
}

void AuthManager::actualizarUsuario(const UsuarioSesion& user, const std::string& nuevaPasswordOpcional, int adminId) {
    if (user.id <= 0) {
        throw std::runtime_error("ID de usuario inválido.");
    }

    if (user.nombreCompleto.empty()) {
        throw std::runtime_error("El nombre completo es obligatorio.");
    }

    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Transaction transaction(db);

        if (!nuevaPasswordOpcional.empty()) {
            if (nuevaPasswordOpcional.size() < 6) {
                throw std::runtime_error("La nueva contraseña debe tener al menos 6 caracteres.");
            }
            std::string hash = generarPasswordHash(nuevaPasswordOpcional);
            SQLite::Statement upd(db, R"(
                UPDATE usuarios
                SET rol_id = ?, nombre_completo = ?, email = ?, activo = ?, password_hash = ?
                WHERE id = ?
            )");
            upd.bind(1, user.rolId);
            upd.bind(2, user.nombreCompleto);
            upd.bind(3, user.email);
            upd.bind(4, user.activo ? 1 : 0);
            upd.bind(5, hash);
            upd.bind(6, user.id);
            upd.exec();
        } else {
            SQLite::Statement upd(db, R"(
                UPDATE usuarios
                SET rol_id = ?, nombre_completo = ?, email = ?, activo = ?
                WHERE id = ?
            )");
            upd.bind(1, user.rolId);
            upd.bind(2, user.nombreCompleto);
            upd.bind(3, user.email);
            upd.bind(4, user.activo ? 1 : 0);
            upd.bind(5, user.id);
            upd.exec();
        }

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'SEGURIDAD', 'EDITAR', 'usuarios', ?, ?)
        )");
        audit.bind(1, adminId);
        audit.bind(2, user.id);
        audit.bind(3, std::format("Actualización perfil usuario '{}'", user.username));
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al actualizar usuario: " << e.what();
    }
}

std::vector<AuditoriaLogItem> AuthManager::listarAuditoria(const std::string& filtroModulo, int limite) {
    std::vector<AuditoriaLogItem> lista;

    try {
        auto& db = DatabaseManager::instance().getDb();

        std::string sql = R"(
            SELECT a.id, a.fecha_hora, u.nombre_completo || ' (' || u.username || ')',
                a.modulo, a.accion, a.tabla_afectada, a.registro_id, COALESCE(a.detalle, '')
            FROM auditoria_logs a
            INNER JOIN usuarios u ON a.usuario_id = u.id
        )";

        if (filtroModulo != "TODOS") {
            sql += " WHERE a.modulo = ? ";
        }
        sql += " ORDER BY a.id DESC LIMIT ?;";

        SQLite::Statement query(db, sql);
        int paramIdx = 1;
        if (filtroModulo != "TODOS") {
            query.bind(paramIdx++, filtroModulo);
        }
        query.bind(paramIdx, limite);

        while (query.executeStep()) {
            lista.push_back({
                query.getColumn(0).getInt(),
                query.getColumn(1).getString(),
                query.getColumn(2).getString(),
                query.getColumn(3).getString(),
                query.getColumn(4).getString(),
                query.getColumn(5).getString(),
                query.getColumn(6).getInt(),
                query.getColumn(7).getString()
            });
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al listar auditoria: " << e.what();
    }

    return lista;
}
