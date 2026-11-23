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


#include "UnidadRepository.hpp"
#include "DbManager.hpp"
#include "core/PostgresDatabase.hpp"
#include <stdexcept>
#include <format>

std::vector<UnidadPrivada> UnidadRepository::listarTodas(const std::string& filtro) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<UnidadPrivada> lista;

    std::string sql = R"(
        SELECT
            u.id, u.torre_bloque, u.nomenclatura, u.tipo,
            COALESCE(u.matricula_inmobiliaria, ''), u.area_privada_m2, u.coeficiente,
            u.propietario_principal_id, p.nombre_razon_social,
            u.inquilino_actual_id, COALESCE(i.nombre_razon_social, ''),
            u.enviar_cobro_a, u.estado
        FROM unidades_privadas u
        INNER JOIN terceros p ON u.propietario_principal_id = p.id
        LEFT JOIN terceros i ON u.inquilino_actual_id = i.id
    )";

    if (!filtro.empty()) {
        sql += " WHERE u.nomenclatura LIKE ? OR u.torre_bloque LIKE ? OR p.nombre_razon_social LIKE ? ";
    }
    sql += " ORDER BY u.torre_bloque ASC, u.nomenclatura ASC;";

    Db::Statement query(db, sql);
    if (!filtro.empty()) {
        std::string likePattern = "%" + filtro + "%";
        query.bind(1, likePattern);
        query.bind(2, likePattern);
        query.bind(3, likePattern);
    }

    while (query.executeStep()) {
        UnidadPrivada u;
        u.id = query.getColumn(0).getInt();
        u.torreBloque = query.getColumn(1).getString();
        u.nomenclatura = query.getColumn(2).getString();
        u.tipo = query.getColumn(3).getString();
        u.matriculaInmobiliaria = query.getColumn(4).getString();
        u.areaPrivadaM2 = query.getColumn(5).getDouble();
        u.coeficiente = query.getColumn(6).getDouble();
        u.propietarioPrincipalId = query.getColumn(7).getInt();
        u.propietarioNombre = query.getColumn(8).getString();

        if (!query.getColumn(9).isNull()) {
            u.inquilinoActualId = query.getColumn(9).getInt();
            u.inquilinoNombre = query.getColumn(10).getString();
        }

        u.enviarCobroA = query.getColumn(11).getString();
        u.estado = query.getColumn(12).getString();
        lista.push_back(u);
    }

    return lista;
}

int UnidadRepository::crear(const UnidadPrivada& u, int usuarioId) {
    if (u.nomenclatura.empty()) {
        throw std::runtime_error("La nomenclatura del inmueble es obligatoria.");
    }
    if (u.propietarioPrincipalId <= 0) {
        throw std::runtime_error("Debe seleccionar un propietario principal.");
    }
    if (u.coeficiente <= 0.0 || u.coeficiente > 1.0) {
        throw std::runtime_error("El coeficiente debe ser mayor a 0% y menor o igual a 100%.");
    }

    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);

    Db::Statement stmt(db, R"(
        INSERT INTO unidades_privadas (
            torre_bloque, nomenclatura, tipo, matricula_inmobiliaria,
            area_privada_m2, coeficiente, propietario_principal_id,
            inquilino_actual_id, enviar_cobro_a, estado
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    stmt.bind(1, u.torreBloque);
    stmt.bind(2, u.nomenclatura);
    stmt.bind(3, u.tipo);
    if (u.matriculaInmobiliaria.empty()) stmt.bind(4);
    else stmt.bind(4, u.matriculaInmobiliaria);

    stmt.bind(5, u.areaPrivadaM2);
    stmt.bind(6, u.coeficiente);
    stmt.bind(7, u.propietarioPrincipalId);

    if (u.inquilinoActualId.has_value() && u.inquilinoActualId.value() > 0) {
        stmt.bind(8, u.inquilinoActualId.value());
    } else {
        stmt.bind(8);
    }

    stmt.bind(9, u.enviarCobroA);
    stmt.bind(10, u.estado);
    stmt.exec();

    int newId = static_cast<int>(db.getLastInsertRowid());

    // Registrar historial inicial de propiedad
    Db::Statement histStmt(db, R"(
        INSERT INTO unidad_propietarios_historial (unidad_id, tercero_id, porcentaje_propiedad, fecha_inicio)
        VALUES (?, ?, 100.0, CURRENT_DATE::text)
    )");
    histStmt.bind(1, newId);
    histStmt.bind(2, u.propietarioPrincipalId);
    histStmt.exec();

    // Auditoría
    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'UNIDADES', 'CREAR', 'unidades_privadas', ?, ?)
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, newId);
    auditStmt.bind(3, std::format("Creación unidad {} (Coef: {:.6f})", u.nomenclatura, u.coeficiente));
    auditStmt.exec();

    transaction.commit();
    return newId;
}

void UnidadRepository::actualizar(const UnidadPrivada& u, int usuarioId) {
    if (u.id <= 0) throw std::runtime_error("ID de unidad inválido para actualizar.");
    if (u.nomenclatura.empty()) throw std::runtime_error("La nomenclatura es obligatoria.");

    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);

    // Verificar si cambió el propietario para actualizar el historial de tradición
    Db::Statement checkProp(db, "SELECT propietario_principal_id FROM unidades_privadas WHERE id = ?");
    checkProp.bind(1, u.id);
    if (!checkProp.executeStep()) {
        throw std::runtime_error("La unidad no existe.");
    }
    int propietarioAnterior = checkProp.getColumn(0).getInt();

    Db::Statement stmt(db, R"(
        UPDATE unidades_privadas SET
            torre_bloque = ?, nomenclatura = ?, tipo = ?, matricula_inmobiliaria = ?,
            area_privada_m2 = ?, coeficiente = ?, propietario_principal_id = ?,
            inquilino_actual_id = ?, enviar_cobro_a = ?, estado = ?
        WHERE id = ?
    )");

    stmt.bind(1, u.torreBloque);
    stmt.bind(2, u.nomenclatura);
    stmt.bind(3, u.tipo);
    if (u.matriculaInmobiliaria.empty()) stmt.bind(4);
    else stmt.bind(4, u.matriculaInmobiliaria);

    stmt.bind(5, u.areaPrivadaM2);
    stmt.bind(6, u.coeficiente);
    stmt.bind(7, u.propietarioPrincipalId);

    if (u.inquilinoActualId.has_value() && u.inquilinoActualId.value() > 0) {
        stmt.bind(8, u.inquilinoActualId.value());
    } else {
        stmt.bind(8);
    }

    stmt.bind(9, u.enviarCobroA);
    stmt.bind(10, u.estado);
    stmt.bind(11, u.id);
    stmt.exec();

    if (propietarioAnterior != u.propietarioPrincipalId) {
        Db::Statement closeHist(db, R"(
            UPDATE unidad_propietarios_historial
            SET fecha_fin = CURRENT_DATE::text
            WHERE unidad_id = ? AND fecha_fin IS NULL
        )");
        closeHist.bind(1, u.id);
        closeHist.exec();

        Db::Statement newHist(db, R"(
            INSERT INTO unidad_propietarios_historial (unidad_id, tercero_id, porcentaje_propiedad, fecha_inicio)
            VALUES (?, ?, 100.0, CURRENT_DATE::text)
        )");
        newHist.bind(1, u.id);
        newHist.bind(2, u.propietarioPrincipalId);
        newHist.exec();
    }

    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'UNIDADES', 'EDITAR', 'unidades_privadas', ?, ?)
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, u.id);
    auditStmt.bind(3, std::format("Actualización unidad {}", u.nomenclatura));
    auditStmt.exec();

    transaction.commit();
}

void UnidadRepository::eliminar(int id, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();

    // Evitar borrar unidades que ya tienen cuentas de cobro o movimientos
    Db::Statement checkMovs(db, "SELECT COUNT(*) FROM cuentas_cobro WHERE unidad_id = ?");
    checkMovs.bind(1, id);
    checkMovs.executeStep();
    if (checkMovs.getColumn(0).getInt() > 0) {
        throw std::runtime_error(
            "No se puede eliminar la unidad porque tiene cuentas de cobro asociadas. "
            "Cámbiela a estado 'INACTIVA'."
        );
    }

    Db::Transaction transaction(db);

    Db::Statement delHist(db, "DELETE FROM unidad_propietarios_historial WHERE unidad_id = ?");
    delHist.bind(1, id);
    delHist.exec();

    Db::Statement delUnit(db, "DELETE FROM unidades_privadas WHERE id = ?");
    delUnit.bind(1, id);
    delUnit.exec();

    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'UNIDADES', 'ELIMINAR', 'unidades_privadas', ?, 'Eliminación de unidad sin movimientos')
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, id);
    auditStmt.exec();

    transaction.commit();
}

double UnidadRepository::obtenerSumaCoeficientesActivos() {
    auto& db = DatabaseManager::instance().getDb();
    return db.execAndGet(
        "SELECT COALESCE(SUM(coeficiente), 0.0) FROM unidades_privadas WHERE estado = 'ACTIVA';"
    ).getDouble();
}

std::vector<TerceroResumen> UnidadRepository::listarPropietarios() {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<TerceroResumen> lista;
    Db::Statement query(db, R"(
        SELECT id, numero_documento, nombre_razon_social
        FROM terceros
        WHERE es_propietario = 1 AND activo = 1
        ORDER BY nombre_razon_social ASC
    )");
    while (query.executeStep()) {
        lista.push_back({query.getColumn(0).getInt(), query.getColumn(1).getString(), query.getColumn(2).getString()});
    }
    return lista;
}

std::vector<TerceroResumen> UnidadRepository::listarInquilinos() {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<TerceroResumen> lista;
    Db::Statement query(db, R"(
        SELECT id, numero_documento, nombre_razon_social
        FROM terceros
        WHERE es_inquilino = 1 AND activo = 1
        ORDER BY nombre_razon_social ASC
    )");
    while (query.executeStep()) {
        lista.push_back({query.getColumn(0).getInt(), query.getColumn(1).getString(), query.getColumn(2).getString()});
    }
    return lista;
}
