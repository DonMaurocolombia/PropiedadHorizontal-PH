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


#include "ParqueaderoRepository.hpp"
#include "DbManager.hpp"
#include "core/PostgresDatabase.hpp"
#include <stdexcept>
#include <format>
#include <algorithm>
#include <cctype>

static std::string normalizarPlaca(std::string placa) {
    placa.erase(std::remove_if(placa.begin(), placa.end(), ::isspace), placa.end());
    std::transform(placa.begin(), placa.end(), placa.begin(), ::toupper);
    return placa;
}

// ============================================================================
// PARQUEADEROS
// ============================================================================

std::vector<Parqueadero> ParqueaderoRepository::listarParqueaderos(const std::string& filtro) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<Parqueadero> lista;

    std::string sql = R"(
        SELECT
            p.id, p.numero, COALESCE(p.nivel_sotano, ''), p.tipo_tenencia,
            COALESCE(p.matricula_inmobiliaria, ''), p.coeficiente,
            p.unidad_asignada_id, COALESCE(u.nomenclatura, 'Sin Asignar'),
            p.canon_mensual, p.estado
        FROM parqueaderos p
        LEFT JOIN unidades_privadas u ON p.unidad_asignada_id = u.id
    )";

    if (!filtro.empty()) {
        sql += " WHERE p.numero LIKE ? OR u.nomenclatura LIKE ? OR p.nivel_sotano LIKE ? ";
    }
    sql += " ORDER BY p.numero ASC;";

    Db::Statement query(db, sql);
    if (!filtro.empty()) {
        std::string pattern = "%" + filtro + "%";
        query.bind(1, pattern);
        query.bind(2, pattern);
        query.bind(3, pattern);
    }

    while (query.executeStep()) {
        Parqueadero p;
        p.id = query.getColumn(0).getInt();
        p.numero = query.getColumn(1).getString();
        p.nivelSotano = query.getColumn(2).getString();
        p.tipoTenencia = query.getColumn(3).getString();
        p.matriculaInmobiliaria = query.getColumn(4).getString();
        p.coeficiente = query.getColumn(5).getDouble();

        if (!query.getColumn(6).isNull()) {
            p.unidadAsignadaId = query.getColumn(6).getInt();
        }
        p.unidadNomenclatura = query.getColumn(7).getString();
        p.canonMensual = query.getColumn(8).getInt64();
        p.estado = query.getColumn(9).getString();
        lista.push_back(p);
    }

    return lista;
}

int ParqueaderoRepository::guardarParqueadero(const Parqueadero& p, int usuarioId) {
    if (p.numero.empty()) {
        throw std::runtime_error("El número o código del parqueadero es obligatorio.");
    }
    if (p.tipoTenencia == "COMUN_ALQUILER" && p.canonMensual <= 0) {
        throw std::runtime_error("Un parqueadero común en alquiler debe tener un canon mensual mayor a $0.");
    }
    if (p.tipoTenencia == "VISITANTES" && p.unidadAsignadaId.has_value()) {
        throw std::runtime_error("Un parqueadero de visitantes no puede asignarse a una unidad privada.");
    }

    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);

    int64_t canonEfectivo = (p.tipoTenencia == "COMUN_ALQUILER") ? p.canonMensual : 0;
    double coefEfectivo = (p.tipoTenencia == "PRIVADO") ? p.coeficiente : 0.0;
    std::string matEfectiva = (p.tipoTenencia == "PRIVADO") ? p.matriculaInmobiliaria : "";
    std::string estadoEfectivo = p.unidadAsignadaId.has_value() ? "OCUPADO" : p.estado;

    int registroId = p.id;

    if (p.id == 0) {
        Db::Statement ins(db, R"(
            INSERT INTO parqueaderos (
                numero, nivel_sotano, tipo_tenencia, matricula_inmobiliaria,
                coeficiente, unidad_asignada_id, canon_mensual, estado
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        )");
        ins.bind(1, p.numero);
        ins.bind(2, p.nivelSotano);
        ins.bind(3, p.tipoTenencia);
        if (matEfectiva.empty()) ins.bind(4); else ins.bind(4, matEfectiva);
        ins.bind(5, coefEfectivo);
        if (p.unidadAsignadaId.has_value()) ins.bind(6, p.unidadAsignadaId.value()); else ins.bind(6);
        ins.bind(7, static_cast<int64_t>(canonEfectivo));
        ins.bind(8, estadoEfectivo);
        ins.exec();
        registroId = static_cast<int>(db.getLastInsertRowid());
    } else {
        Db::Statement upd(db, R"(
            UPDATE parqueaderos SET
                numero = ?, nivel_sotano = ?, tipo_tenencia = ?, matricula_inmobiliaria = ?,
                coeficiente = ?, unidad_asignada_id = ?, canon_mensual = ?, estado = ?
            WHERE id = ?
        )");
        upd.bind(1, p.numero);
        upd.bind(2, p.nivelSotano);
        upd.bind(3, p.tipoTenencia);
        if (matEfectiva.empty()) upd.bind(4); else upd.bind(4, matEfectiva);
        upd.bind(5, coefEfectivo);
        if (p.unidadAsignadaId.has_value()) upd.bind(6, p.unidadAsignadaId.value()); else upd.bind(6);
        upd.bind(7, static_cast<int64_t>(canonEfectivo));
        upd.bind(8, estadoEfectivo);
        upd.bind(9, p.id);
        upd.exec();
    }

    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PARQUEADEROS', ?, 'parqueaderos', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, p.id == 0 ? "CREAR" : "EDITAR");
    audit.bind(3, registroId);
    audit.bind(4, std::format("Parqueadero {} ({})", p.numero, p.tipoTenencia));
    audit.exec();

    transaction.commit();
    return registroId;
}

void ParqueaderoRepository::eliminarParqueadero(int id, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();
    Db::Statement checkVeh(db, "SELECT COUNT(*) FROM vehiculos WHERE parqueadero_id = ?");
    checkVeh.bind(1, id);
    checkVeh.executeStep();
    if (checkVeh.getColumn(0).getInt() > 0) {
        throw std::runtime_error("No se puede eliminar el parqueadero porque tiene vehículos asociados.");
    }

    Db::Transaction transaction(db);
    Db::Statement del(db, "DELETE FROM parqueaderos WHERE id = ?");
    del.bind(1, id);
    del.exec();

    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PARQUEADEROS', 'ELIMINAR', 'parqueaderos', ?, 'Eliminación de parqueadero')
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, id);
    audit.exec();

    transaction.commit();
}

// ============================================================================
// BODEGAS
// ============================================================================

std::vector<Bodega> ParqueaderoRepository::listarBodegas(const std::string& filtro) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<Bodega> lista;

    std::string sql = R"(
        SELECT
            b.id, b.numero, COALESCE(b.ubicacion, ''), b.tipo_tenencia,
            COALESCE(b.matricula_inmobiliaria, ''), b.coeficiente,
            b.unidad_asignada_id, COALESCE(u.nomenclatura, 'Sin Asignar'),
            b.canon_mensual
        FROM bodegas b
        LEFT JOIN unidades_privadas u ON b.unidad_asignada_id = u.id
    )";

    if (!filtro.empty()) {
        sql += " WHERE b.numero LIKE ? OR u.nomenclatura LIKE ? ";
    }
    sql += " ORDER BY b.numero ASC;";

    Db::Statement query(db, sql);
    if (!filtro.empty()) {
        std::string pattern = "%" + filtro + "%";
        query.bind(1, pattern);
        query.bind(2, pattern);
    }

    while (query.executeStep()) {
        Bodega b;
        b.id = query.getColumn(0).getInt();
        b.numero = query.getColumn(1).getString();
        b.ubicacion = query.getColumn(2).getString();
        b.tipoTenencia = query.getColumn(3).getString();
        b.matriculaInmobiliaria = query.getColumn(4).getString();
        b.coeficiente = query.getColumn(5).getDouble();
        if (!query.getColumn(6).isNull()) {
            b.unidadAsignadaId = query.getColumn(6).getInt();
        }
        b.unidadNomenclatura = query.getColumn(7).getString();
        b.canonMensual = query.getColumn(8).getInt64();
        lista.push_back(b);
    }

    return lista;
}

int ParqueaderoRepository::guardarBodega(const Bodega& b, int usuarioId) {
    if (b.numero.empty()) {
        throw std::runtime_error("El número o código de la bodega es obligatorio.");
    }

    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);

    int64_t canonEfectivo = (b.tipoTenencia == "COMUN_ALQUILER") ? b.canonMensual : 0;
    double coefEfectivo = (b.tipoTenencia == "PRIVADA") ? b.coeficiente : 0.0;
    std::string matEfectiva = (b.tipoTenencia == "PRIVADA") ? b.matriculaInmobiliaria : "";
    int registroId = b.id;

    if (b.id == 0) {
        Db::Statement ins(db, R"(
            INSERT INTO bodegas (
                numero, ubicacion, tipo_tenencia, matricula_inmobiliaria,
                coeficiente, unidad_asignada_id, canon_mensual
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        )");
        ins.bind(1, b.numero);
        ins.bind(2, b.ubicacion);
        ins.bind(3, b.tipoTenencia);
        if (matEfectiva.empty()) ins.bind(4); else ins.bind(4, matEfectiva);
        ins.bind(5, coefEfectivo);
        if (b.unidadAsignadaId.has_value()) ins.bind(6, b.unidadAsignadaId.value()); else ins.bind(6);
        ins.bind(7, static_cast<int64_t>(canonEfectivo));
        ins.exec();
        registroId = static_cast<int>(db.getLastInsertRowid());
    } else {
        Db::Statement upd(db, R"(
            UPDATE bodegas SET
                numero = ?, ubicacion = ?, tipo_tenencia = ?, matricula_inmobiliaria = ?,
                coeficiente = ?, unidad_asignada_id = ?, canon_mensual = ?
            WHERE id = ?
        )");
        upd.bind(1, b.numero);
        upd.bind(2, b.ubicacion);
        upd.bind(3, b.tipoTenencia);
        if (matEfectiva.empty()) upd.bind(4); else upd.bind(4, matEfectiva);
        upd.bind(5, coefEfectivo);
        if (b.unidadAsignadaId.has_value()) upd.bind(6, b.unidadAsignadaId.value());
        else upd.bind(6);

        upd.bind(7, static_cast<int64_t>(canonEfectivo));
        upd.bind(8, b.id);
        upd.exec();
    }

    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'BODEGAS', ?, 'bodegas', ?, ?)
    )");

    audit.bind(1, usuarioId);
    audit.bind(2, b.id == 0 ? "CREAR" : "EDITAR");
    audit.bind(3, registroId);
    audit.bind(4, std::format("Bodega {} ({})", b.numero, b.tipoTenencia));
    audit.exec();
    transaction.commit();

    return registroId;
}

void ParqueaderoRepository::eliminarBodega(int id, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);
    Db::Statement del(db, "DELETE FROM bodegas WHERE id = ?");
    del.bind(1, id);
    del.exec();
    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'BODEGAS', 'ELIMINAR', 'bodegas', ?, 'Eliminación de bodega')
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, id);
    audit.exec();
    transaction.commit();
}

// ============================================================================
// VEHÍCULOS
// ============================================================================
std::vector<Vehiculo> ParqueaderoRepository::listarVehiculos(const std::string& filtro) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<Vehiculo> lista;
    std::string sql = R"(
        SELECT
        v.id, v.unidad_id, u.nomenclatura,
        v.parqueadero_id, COALESCE(p.numero, 'N/A'),
        v.placa, v.tipo, COALESCE(v.marca_modelo, ''), COALESCE(v.color, '')
        FROM vehiculos v
        INNER JOIN unidades_privadas u ON v.unidad_id = u.id
        LEFT JOIN parqueaderos p ON v.parqueadero_id = p.id
    )";

    if (!filtro.empty()) {
        sql += " WHERE v.placa LIKE ? OR u.nomenclatura LIKE ? OR v.marca_modelo LIKE ? ";
    }

    sql += " ORDER BY v.placa ASC;";
    Db::Statement query(db, sql);
    if (!filtro.empty()) {
        std::string pattern = "%" + filtro + "%";
        query.bind(1, pattern);
        query.bind(2, pattern);
        query.bind(3, pattern);
    }

    while (query.executeStep()) {
        Vehiculo v;
        v.id = query.getColumn(0).getInt();
        v.unidadId = query.getColumn(1).getInt();
        v.unidadNomenclatura = query.getColumn(2).getString();
        if (!query.getColumn(3).isNull()) {
            v.parqueaderoId = query.getColumn(3).getInt();
        }

        v.parqueaderoNumero = query.getColumn(4).getString();
        v.placa = query.getColumn(5).getString();
        v.tipo = query.getColumn(6).getString();
        v.marcaModelo = query.getColumn(7).getString();
        v.color = query.getColumn(8).getString();
        lista.push_back(v);
    }

    return lista;
}

int ParqueaderoRepository::guardarVehiculo(const Vehiculo& v, int usuarioId) {
    if (v.unidadId <= 0) {
        throw std::runtime_error("Debe seleccionar la unidad privada propietaria o residente del vehículo.");
    }

    std::string placaLimpia = normalizarPlaca(v.placa);
    if (placaLimpia.empty()) {
        throw std::runtime_error("La placa o identificación del vehículo es obligatoria.");
    }

    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);
    int registroId = v.id;
    if (v.id == 0) {
        Db::Statement ins(db, R"(
            INSERT INTO vehiculos (unidad_id, parqueadero_id, placa, tipo, marca_modelo, color)
            VALUES (?, ?, ?, ?, ?, ?)
        )");

        ins.bind(1, v.unidadId);
        if (v.parqueaderoId.has_value()) ins.bind(2, v.parqueaderoId.value()); else ins.bind(2);
        ins.bind(3, placaLimpia);
        ins.bind(4, v.tipo);
        ins.bind(5, v.marcaModelo);
        ins.bind(6, v.color);
        ins.exec();
        registroId = static_cast<int64_t>(db.getLastInsertRowid());
    } else {
        Db::Statement upd(db, R"(
            UPDATE vehiculos SET
            unidad_id = ?, parqueadero_id = ?, placa = ?, tipo = ?, marca_modelo = ?, color = ?
            WHERE id = ?
        )");

        upd.bind(1, v.unidadId);
        if (v.parqueaderoId.has_value()) upd.bind(2, v.parqueaderoId.value());
        else upd.bind(2);

        upd.bind(3, placaLimpia);
        upd.bind(4, v.tipo);
        upd.bind(5, v.marcaModelo);
        upd.bind(6, v.color);
        upd.bind(7, v.id);
        upd.exec();
    }

    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'VEHICULOS', ?, 'vehiculos', ?, ?)
    )");

    audit.bind(1, usuarioId);
    audit.bind(2, v.id == 0 ? "CREAR" : "EDITAR");
    audit.bind(3, registroId);
    audit.bind(4, std::format("Vehículo placa {} ({})", placaLimpia, v.tipo));
    audit.exec();
    transaction.commit();

    return registroId;
}

void ParqueaderoRepository::eliminarVehiculo(int id, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();
    Db::Transaction transaction(db);
    Db::Statement del(db, "DELETE FROM vehiculos WHERE id = ?");
    del.bind(1, id);
    del.exec();
    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'VEHICULOS', 'ELIMINAR', 'vehiculos', ?, 'Eliminación de vehículo')
    )");
    
    audit.bind(1, usuarioId);
    audit.bind(2, id);
    audit.exec();
    transaction.commit();
}
