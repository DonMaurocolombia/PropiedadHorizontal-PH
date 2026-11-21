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


#include "PqrsRepository.hpp"
#include "DbManager.hpp"
#include "AccountingEngine.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <stdexcept>
#include <format>

std::vector<PqrsSancion> PqrsRepository::listar(const std::string& filtroEstado,
                                                const std::string& textoBusqueda) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<PqrsSancion> lista;

    std::string sql = R"(
        SELECT
            p.id, p.radicado, p.unidad_id, COALESCE(u.nomenclatura, 'General / Zonas Comunes'),
            p.tercero_id, t.nombre_razon_social, p.tipo, p.asunto, p.descripcion,
            p.fecha_radicacion, COALESCE(p.fecha_descargos, ''), COALESCE(p.valor_multa, 0), p.estado
        FROM pqrs_sanciones p
        INNER JOIN terceros t ON p.tercero_id = t.id
        LEFT JOIN unidades_privadas u ON p.unidad_id = u.id
        WHERE 1=1
    )";

    if (filtroEstado != "TODOS") {
        sql += " AND p.estado = ? ";
    }
    if (!textoBusqueda.empty()) {
        sql += " AND (p.radicado LIKE ? OR p.asunto LIKE ? OR t.nombre_razon_social LIKE ? OR u.nomenclatura LIKE ?) ";
    }
    sql += " ORDER BY p.id DESC;";

    SQLite::Statement query(db, sql);
    int idx = 1;
    if (filtroEstado != "TODOS") {
        query.bind(idx++, filtroEstado);
    }
    if (!textoBusqueda.empty()) {
        std::string pattern = "%" + textoBusqueda + "%";
        query.bind(idx++, pattern);
        query.bind(idx++, pattern);
        query.bind(idx++, pattern);
        query.bind(idx++, pattern);
    }

    while (query.executeStep()) {
        PqrsSancion item;
        item.id = query.getColumn(0).getInt();
        item.radicado = query.getColumn(1).getString();
        if (!query.getColumn(2).isNull()) {
            item.unidadId = query.getColumn(2).getInt();
        }
        item.unidadNomenclatura = query.getColumn(3).getString();
        item.terceroId = query.getColumn(4).getInt();
        item.terceroNombre = query.getColumn(5).getString();
        item.tipo = query.getColumn(6).getString();
        item.asunto = query.getColumn(7).getString();
        item.descripcion = query.getColumn(8).getString();
        item.fechaRadicacion = query.getColumn(9).getString();
        item.fechaDescargos = query.getColumn(10).getString();
        item.valorMulta = query.getColumn(11).getInt64();
        item.estado = query.getColumn(12).getString();
        lista.push_back(item);
    }

    return lista;
}

std::string PqrsRepository::radicar(const PqrsSancion& item, int usuarioId) {
    if (item.terceroId <= 0) {
        throw std::runtime_error("Debe seleccionar el residente o tercero involucrado.");
    }
    if (item.asunto.empty() || item.descripcion.empty()) {
        throw std::runtime_error("El asunto y la descripción detallada son obligatorios.");
    }
    if (item.tipo == "PROCESO_SANCIONATORIO" && !item.unidadId.has_value()) {
        throw std::runtime_error("Un proceso sancionatorio debe estar asociado a una unidad privada.");
    }

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);

    std::string anio = item.fechaRadicacion.substr(0, 4);
    int siguienteCons = db.execAndGet("SELECT COALESCE(MAX(id), 0) + 1 FROM pqrs_sanciones;").getInt();
    std::string radicado = std::format("PQRS-{}-{:04d}", anio, siguienteCons);

    std::string estadoInicial = (item.tipo == "PROCESO_SANCIONATORIO" && !item.fechaDescargos.empty())
        ? "EN_DESCARGOS"
        : "ABIERTA";

    SQLite::Statement ins(db, R"(
        INSERT INTO pqrs_sanciones (
            radicado, unidad_id, tercero_id, tipo, asunto, descripcion,
            fecha_radicacion, fecha_descargos, valor_multa, estado
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, ?)
    )");

    ins.bind(1, radicado);
    if (item.unidadId.has_value() && item.unidadId.value() > 0) {
        ins.bind(2, item.unidadId.value());
    } else {
        ins.bind(2);
    }
    ins.bind(3, item.terceroId);
    ins.bind(4, item.tipo);
    ins.bind(5, item.asunto);
    ins.bind(6, item.descripcion);
    ins.bind(7, item.fechaRadicacion);
    if (item.fechaDescargos.empty()) ins.bind(8); else ins.bind(8, item.fechaDescargos);
    ins.bind(9, estadoInicial);
    ins.exec();

    int newId = static_cast<int>(db.getLastInsertRowid());

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PQRS', 'CREAR', 'pqrs_sanciones', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, newId);
    audit.bind(3, std::format("Radicado {} ({}): {}", radicado, item.tipo, item.asunto));
    audit.exec();

    transaction.commit();
    return radicado;
}

void PqrsRepository::actualizarTramite(int id,
                                       const std::string& nuevoEstado,
                                       const std::string& fechaDescargos,
                                       int64_t valorMulta,
                                       const std::string& notaSeguimiento,
                                       int usuarioId) {
    if (id <= 0) throw std::runtime_error("Seleccione un caso válido.");

    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement getStmt(db, R"(
        SELECT radicado, unidad_id, tercero_id, tipo, asunto, descripcion, estado, COALESCE(fecha_descargos, '')
        FROM pqrs_sanciones WHERE id = ?
    )");
    getStmt.bind(1, id);
    if (!getStmt.executeStep()) {
        throw std::runtime_error("El radicado seleccionado no existe.");
    }

    std::string radicado = getStmt.getColumn(0).getString();
    std::optional<int> unidadId;
    if (!getStmt.getColumn(1).isNull()) unidadId = getStmt.getColumn(1).getInt();
    int terceroId = getStmt.getColumn(2).getInt();
    std::string tipo = getStmt.getColumn(3).getString();
    std::string asunto = getStmt.getColumn(4).getString();
    std::string descActual = getStmt.getColumn(5).getString();
    std::string estadoActual = getStmt.getColumn(6).getString();
    std::string descFechaExistente = getStmt.getColumn(7).getString();

    if (estadoActual == "SANCIONADA" || estadoActual == "CERRADA") {
        throw std::runtime_error("Este caso ya fue cerrado o sancionado y no admite modificaciones.");
    }

    // Validación del Debido Proceso (Art. 59-60 Ley 675 de 2001)
    if (nuevoEstado == "EN_DESCARGOS" && fechaDescargos.empty()) {
        throw std::runtime_error("Debe indicar la fecha de citación a descargos para cambiar a este estado.");
    }

    if (nuevoEstado == "SANCIONADA") {
        if (tipo != "PROCESO_SANCIONATORIO") {
            throw std::runtime_error("Solo los casos de tipo 'PROCESO_SANCIONATORIO' pueden culminar en sanción.");
        }
        if (estadoActual != "EN_DESCARGOS" && descFechaExistente.empty() && fechaDescargos.empty()) {
            throw std::runtime_error(
                "Violación al Debido Proceso (Ley 675): Antes de imponer una sanción, el residente debe haber sido "
                "citado a etapa de DESCARGOS con fecha registrada."
            );
        }
        if (valorMulta <= 0) {
            throw std::runtime_error("Para imponer una sanción pecuniaria debe especificar un valor de multa mayor a $0.");
        }
        if (!unidadId.has_value()) {
            throw std::runtime_error("El proceso sancionatorio requiere una unidad privada para cargar la multa.");
        }
    }

    SQLite::Transaction transaction(db);

    std::string nuevaDescripcion = descActual;
    if (!notaSeguimiento.empty()) {
        nuevaDescripcion += "\n--- Seguimiento (" + nuevoEstado + ") ---\n" + notaSeguimiento;
    }

    std::string fechaDescFinal = !fechaDescargos.empty() ? fechaDescargos : descFechaExistente;

    SQLite::Statement upd(db, R"(
        UPDATE pqrs_sanciones
        SET estado = ?, fecha_descargos = ?, valor_multa = ?, descripcion = ?
        WHERE id = ?
    )");
    upd.bind(1, nuevoEstado);
    if (fechaDescFinal.empty()) upd.bind(2); else upd.bind(2, fechaDescFinal);
    upd.bind(3, static_cast<int64_t>(nuevoEstado == "SANCIONADA" ? valorMulta : 0));
    upd.bind(4, nuevaDescripcion);
    upd.bind(5, id);
    upd.exec();

    // Si se impone multa, causar contablemente en Cartera (130520) vs Ingreso Multas (417020)
    if (nuevoEstado == "SANCIONADA" && valorMulta > 0) {
        ComprobanteRequest comp;
        comp.tipoComprobante = "NOTA_CONTABLE";
        comp.fecha = db.execAndGet("SELECT date('now', 'localtime');").getString();
        comp.descripcion = std::format("Imposición Multa Convivencia {} - {}", radicado, asunto);
        comp.usuarioId = usuarioId;

        comp.movimientos = {
            {"130520", terceroId, unidadId, std::format("Multa {} - {}", radicado, asunto), valorMulta, 0},
            {"417020", terceroId, unidadId, std::format("Ingreso Sanción {} - {}", radicado, asunto), 0, valorMulta}
        };

        AccountingEngine::registrarComprobante(comp);
    }

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PQRS', 'EDITAR', 'pqrs_sanciones', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, id);
    audit.bind(3, std::format("Radicado {} actualizado a estado '{}' (Multa: ${})", radicado, nuevoEstado, valorMulta));
    audit.exec();

    transaction.commit();
}
