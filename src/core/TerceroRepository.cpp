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


#include "TerceroRepository.hpp"
#include "DbManager.hpp"
#include "core/PostgresDatabase.hpp"
#include <stdexcept>
#include <format>
#include <array>
#include <cctype>

std::vector<Tercero> TerceroRepository::listar(const std::string& textoBusqueda,
                                               const std::string& filtroRol) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<Tercero> lista;

    std::string sql = R"(
        SELECT
            id, tipo_documento, numero_documento, COALESCE(dv, ''),
            nombre_razon_social, COALESCE(telefono, ''), COALESCE(celular, ''),
            COALESCE(email, ''), COALESCE(direccion_correspondencia, ''),
            es_propietario, es_inquilino, es_residente, es_proveedor, es_visitante, activo
        FROM terceros
        WHERE 1=1
    )";

    if (filtroRol == "PROPIETARIO") {
        sql += " AND es_propietario = 1 ";
    } else if (filtroRol == "INQUILINO") {
        sql += " AND es_inquilino = 1 ";
    } else if (filtroRol == "RESIDENTE") {
        sql += " AND es_residente = 1 ";
    } else if (filtroRol == "PROVEEDOR") {
        sql += " AND es_proveedor = 1 ";
    } else if (filtroRol == "VISITANTE") {
        sql += " AND es_visitante = 1 ";
    }

    if (!textoBusqueda.empty()) {
        sql += " AND (numero_documento LIKE ? OR nombre_razon_social LIKE ? OR email LIKE ?) ";
    }

    sql += " ORDER BY nombre_razon_social ASC;";

    Db::Statement query(db, sql);
    if (!textoBusqueda.empty()) {
        std::string pattern = "%" + textoBusqueda + "%";
        query.bind(1, pattern);
        query.bind(2, pattern);
        query.bind(3, pattern);
    }

    while (query.executeStep()) {
        Tercero t;
        t.id = query.getColumn(0).getInt();
        t.tipoDocumento = query.getColumn(1).getString();
        t.numeroDocumento = query.getColumn(2).getString();
        t.dv = query.getColumn(3).getString();
        t.nombreRazonSocial = query.getColumn(4).getString();
        t.telefono = query.getColumn(5).getString();
        t.celular = query.getColumn(6).getString();
        t.email = query.getColumn(7).getString();
        t.direccionCorrespondencia = query.getColumn(8).getString();
        t.esPropietario = query.getColumn(9).getInt() == 1;
        t.esInquilino = query.getColumn(10).getInt() == 1;
        t.esResidente = query.getColumn(11).getInt() == 1;
        t.esProveedor = query.getColumn(12).getInt() == 1;
        t.esVisitante = query.getColumn(13).getInt() == 1;
        t.activo = query.getColumn(14).getInt() == 1;
        lista.push_back(t);
    }

    return lista;
}

int TerceroRepository::crear(const Tercero& t, int usuarioId) {
    if (t.numeroDocumento.empty()) {
        throw std::runtime_error("El número de documento es obligatorio.");
    }
    if (t.nombreRazonSocial.empty()) {
        throw std::runtime_error("El nombre o razón social es obligatorio.");
    }
    if (!t.esPropietario && !t.esInquilino && !t.esResidente && !t.esProveedor && !t.esVisitante) {
        throw std::runtime_error("Debe marcar al menos un rol (Propietario, Inquilino, Residente, Proveedor o Visitante).");
    }

    auto& db = DatabaseManager::instance().getDb();

    // Verificar documento duplicado
    Db::Statement checkDup(db, "SELECT COUNT(*) FROM terceros WHERE numero_documento = ?");
    checkDup.bind(1, t.numeroDocumento);
    checkDup.executeStep();
    if (checkDup.getColumn(0).getInt() > 0) {
        throw std::runtime_error("Ya existe un tercero registrado con ese número de documento.");
    }

    Db::Transaction transaction(db);

    Db::Statement stmt(db, R"(
        INSERT INTO terceros (
            tipo_documento, numero_documento, dv, nombre_razon_social,
            telefono, celular, email, direccion_correspondencia,
            es_propietario, es_inquilino, es_residente, es_proveedor, es_visitante, activo
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    stmt.bind(1, t.tipoDocumento);
    stmt.bind(2, t.numeroDocumento);
    stmt.bind(3, t.tipoDocumento == "NIT" ? t.dv : "");
    stmt.bind(4, t.nombreRazonSocial);
    stmt.bind(5, t.telefono);
    stmt.bind(6, t.celular);
    stmt.bind(7, t.email);
    stmt.bind(8, t.direccionCorrespondencia);
    stmt.bind(9, t.esPropietario ? 1 : 0);
    stmt.bind(10, t.esInquilino ? 1 : 0);
    stmt.bind(11, t.esResidente ? 1 : 0);
    stmt.bind(12, t.esProveedor ? 1 : 0);
    stmt.bind(13, t.esVisitante ? 1 : 0);
    stmt.bind(14, t.activo ? 1 : 0);
    stmt.exec();

    int newId = static_cast<int>(db.getLastInsertRowid());

    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'TERCEROS', 'CREAR', 'terceros', ?, ?)
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, newId);
    auditStmt.bind(3, std::format("Creación tercero {} - {}", t.numeroDocumento, t.nombreRazonSocial));
    auditStmt.exec();

    transaction.commit();
    return newId;
}

void TerceroRepository::actualizar(const Tercero& t, int usuarioId) {
    if (t.id <= 0) throw std::runtime_error("ID de tercero inválido.");
    if (t.numeroDocumento.empty() || t.nombreRazonSocial.empty()) {
        throw std::runtime_error("Documento y Nombre/Razón Social son obligatorios.");
    }
    if (!t.esPropietario && !t.esInquilino && !t.esResidente && !t.esProveedor && !t.esVisitante) {
        throw std::runtime_error("Debe marcar al menos un rol para el tercero.");
    }

    auto& db = DatabaseManager::instance().getDb();

    // Validar que el documento no pertenezca a otro registro
    Db::Statement checkDup(db, "SELECT COUNT(*) FROM terceros WHERE numero_documento = ? AND id != ?");
    checkDup.bind(1, t.numeroDocumento);
    checkDup.bind(2, t.id);
    checkDup.executeStep();
    if (checkDup.getColumn(0).getInt() > 0) {
        throw std::runtime_error("Otro tercero ya tiene registrado ese número de documento.");
    }

    Db::Transaction transaction(db);

    Db::Statement stmt(db, R"(
        UPDATE terceros SET
            tipo_documento = ?, numero_documento = ?, dv = ?, nombre_razon_social = ?,
            telefono = ?, celular = ?, email = ?, direccion_correspondencia = ?,
            es_propietario = ?, es_inquilino = ?, es_residente = ?,
            es_proveedor = ?, es_visitante = ?, activo = ?
        WHERE id = ?
    )");

    stmt.bind(1, t.tipoDocumento);
    stmt.bind(2, t.numeroDocumento);
    stmt.bind(3, t.tipoDocumento == "NIT" ? t.dv : "");
    stmt.bind(4, t.nombreRazonSocial);
    stmt.bind(5, t.telefono);
    stmt.bind(6, t.celular);
    stmt.bind(7, t.email);
    stmt.bind(8, t.direccionCorrespondencia);
    stmt.bind(9, t.esPropietario ? 1 : 0);
    stmt.bind(10, t.esInquilino ? 1 : 0);
    stmt.bind(11, t.esResidente ? 1 : 0);
    stmt.bind(12, t.esProveedor ? 1 : 0);
    stmt.bind(13, t.esVisitante ? 1 : 0);
    stmt.bind(14, t.activo ? 1 : 0);
    stmt.bind(15, t.id);
    stmt.exec();

    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'TERCEROS', 'EDITAR', 'terceros', ?, ?)
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, t.id);
    auditStmt.bind(3, std::format("Actualización tercero {} - {}", t.numeroDocumento, t.nombreRazonSocial));
    auditStmt.exec();

    transaction.commit();
}

void TerceroRepository::eliminar(int id, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();

    // Verificar si está vinculado a unidades o movimientos contables
    Db::Statement checkUnidades(db, R"(
        SELECT COUNT(*) FROM unidades_privadas
        WHERE propietario_principal_id = ? OR inquilino_actual_id = ?
    )");
    checkUnidades.bind(1, id);
    checkUnidades.bind(2, id);
    checkUnidades.executeStep();
    if (checkUnidades.getColumn(0).getInt() > 0) {
        throw std::runtime_error(
            "No se puede eliminar porque está asociado a una o más unidades privadas. "
            "Desmárquelo como 'Activo' si ya no tiene relación con la copropiedad."
        );
    }

    Db::Statement checkMovs(db, "SELECT COUNT(*) FROM movimientos_contables WHERE tercero_id = ?");
    checkMovs.bind(1, id);
    checkMovs.executeStep();
    if (checkMovs.getColumn(0).getInt() > 0) {
        throw std::runtime_error("No se puede eliminar porque tiene movimientos contables registrados.");
    }

    Db::Transaction transaction(db);

    Db::Statement delStmt(db, "DELETE FROM terceros WHERE id = ?");
    delStmt.bind(1, id);
    delStmt.exec();

    Db::Statement auditStmt(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'TERCEROS', 'ELIMINAR', 'terceros', ?, 'Eliminación de tercero sin movimientos')
    )");
    auditStmt.bind(1, usuarioId);
    auditStmt.bind(2, id);
    auditStmt.exec();

    transaction.commit();
}

std::string TerceroRepository::calcularDigitoVerificacionDIAN(const std::string& nit) {
    // Factores primos oficiales de la DIAN (de derecha a izquierda)
    constexpr std::array<int, 15> factores = {
        3, 7, 13, 17, 19, 23, 29, 37, 41, 43, 47, 53, 59, 67, 71
    };

    std::string soloDigitos;
    for (char c : nit) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            soloDigitos.push_back(c);
        }
    }

    if (soloDigitos.empty() || soloDigitos.size() > 15) {
        return "";
    }

    int suma = 0;
    int n = static_cast<int>(soloDigitos.size());
    for (int i = 0; i < n; ++i) {
        int digito = soloDigitos[n - 1 - i] - '0';
        suma += digito * factores[i];
    }

    int residuo = suma % 11;
    int dv = (residuo > 1) ? (11 - residuo) : residuo;
    return std::to_string(dv);
}
