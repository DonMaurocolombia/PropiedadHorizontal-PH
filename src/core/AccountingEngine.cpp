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


#include "AccountingEngine.hpp"
#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <stdexcept>
#include <format>

int AccountingEngine::registrarComprobante(const ComprobanteRequest& req) {
    // 1. Validaciones previas en memoria antes de tocar la BD
    if (req.movimientos.size() < 2) {
        throw std::runtime_error("Un comprobante contable debe tener al menos 2 movimientos.");
    }

    validarPartidaDoble(req.movimientos);

    for (const auto& mov : req.movimientos) {
        validarCuentaPUC(mov.cuentaCodigo, mov.terceroId);
    }

    auto& db = DatabaseManager::instance().getDb();

    // 2. Transacción atómica: o se guarda todo el asiento o no se guarda nada
    SQLite::Transaction transaction(db);

    int consecutivo = obtenerSiguienteConsecutivo(req.tipoComprobante);

    // Insertar encabezado del comprobante
    SQLite::Statement insertComp(db, R"(
        INSERT INTO comprobantes_contables (
            tipo_comprobante, consecutivo, fecha, descripcion, estado, usuario_id
        ) VALUES (?, ?, ?, ?, 'CONTABILIZADO', ?)
    )");

    insertComp.bind(1, req.tipoComprobante);
    insertComp.bind(2, consecutivo);
    insertComp.bind(3, req.fecha);
    insertComp.bind(4, req.descripcion);
    insertComp.bind(5, req.usuarioId);
    insertComp.exec();

    int comprobanteId = static_cast<int>(db.getLastInsertRowid());

    // Insertar cada línea de movimiento
    SQLite::Statement insertMov(db, R"(
        INSERT INTO movimientos_contables (
            comprobante_id, cuenta_codigo, tercero_id, unidad_id, detalle, debito, credito
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    for (const auto& mov : req.movimientos) {
        insertMov.bind(1, comprobanteId);
        insertMov.bind(2, mov.cuentaCodigo);
        insertMov.bind(3, mov.terceroId);

        if (mov.unidadId.has_value()) {
            insertMov.bind(4, mov.unidadId.value());
        } else {
            insertMov.bind(4); // NULL
        }

        insertMov.bind(5, mov.detalle);
        insertMov.bind(6, static_cast<int64_t>(mov.debito));
        insertMov.bind(7, static_cast<int64_t>(mov.credito));

        insertMov.exec();
        insertMov.reset();
    }

    // Registrar en auditoría
    SQLite::Statement insertAudit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'CONTABILIDAD', 'CREAR', 'comprobantes_contables', ?, ?)
    )");
    insertAudit.bind(1, req.usuarioId);
    insertAudit.bind(2, comprobanteId);
    insertAudit.bind(3, std::format("Comprobante {} #{} por: {}", req.tipoComprobante, consecutivo, req.descripcion));
    insertAudit.exec();

    transaction.commit();
    return comprobanteId;
}

void AccountingEngine::validarPartidaDoble(const std::vector<LineaMovimiento>& movimientos) {
    int64_t sumaDebitos = 0;
    int64_t sumaCreditos = 0;

    for (const auto& mov : movimientos) {
        if (mov.debito < 0 || mov.credito < 0) {
            throw std::runtime_error("Los valores contables no pueden ser negativos.");
        }
        if (mov.debito > 0 && mov.credito > 0) {
            throw std::runtime_error("Una línea contable no puede tener Débito y Crédito al mismo tiempo.");
        }
        if (mov.debito == 0 && mov.credito == 0) {
            throw std::runtime_error("Una línea contable no puede tener valor cero en Débito y Crédito.");
        }

        sumaDebitos += mov.debito;
        sumaCreditos += mov.credito;
    }

    if (sumaDebitos != sumaCreditos) {
        throw std::runtime_error(std::format(
            "Asiento descuadrado: Débitos (${}) != Créditos (${}). Diferencia: ${}",
            sumaDebitos, sumaCreditos, std::abs(sumaDebitos - sumaCreditos)
        ));
    }
}

void AccountingEngine::validarCuentaPUC(const std::string& cuentaCodigo, int terceroId) {
    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement query(db, "SELECT permite_movimiento, exige_tercero FROM puc_cuentas WHERE codigo = ?");
    query.bind(1, cuentaCodigo);

    if (!query.executeStep()) {
        throw std::runtime_error(std::format("La cuenta PUC '{}' no existe.", cuentaCodigo));
    }

    int permiteMovimiento = query.getColumn(0).getInt();
    int exigeTercero = query.getColumn(1).getInt();

    if (permiteMovimiento == 0) {
        throw std::runtime_error(std::format(
            "La cuenta PUC '{}' es una cuenta mayor/grupo y no admite movimientos directos.", cuentaCodigo
        ));
    }

    if (exigeTercero == 1 && terceroId <= 0) {
        throw std::runtime_error(std::format(
            "La cuenta PUC '{}' exige un Tercero válido.", cuentaCodigo
        ));
    }
}

int AccountingEngine::obtenerSiguienteConsecutivo(const std::string& tipoComprobante) {
    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement query(db, R"(
        SELECT COALESCE(MAX(consecutivo), 0) + 1
        FROM comprobantes_contables
        WHERE tipo_comprobante = ?
    )");
    query.bind(1, tipoComprobante);
    query.executeStep();

    return query.getColumn(0).getInt();
}

int AccountingEngine::anularComprobante(int comprobanteId, int usuarioId, const std::string& motivo) {
    auto& db = DatabaseManager::instance().getDb();

    // Verificar estado actual
    SQLite::Statement checkStmt(db, "SELECT estado, tipo_comprobante, consecutivo FROM comprobantes_contables WHERE id = ?");
    checkStmt.bind(1, comprobanteId);

    if (!checkStmt.executeStep()) {
        throw std::runtime_error("El comprobante a anular no existe.");
    }
    if (checkStmt.getColumn(0).getString() == "ANULADO") {
        throw std::runtime_error("El comprobante ya se encuentra anulado.");
    }

    std::string tipoOrig = checkStmt.getColumn(1).getString();
    int consOrig = checkStmt.getColumn(2).getInt();

    // Leer movimientos originales e invertir débitos y créditos
    SQLite::Statement movStmt(db, R"(
        SELECT cuenta_codigo, tercero_id, unidad_id, detalle, debito, credito
        FROM movimientos_contables
        WHERE comprobante_id = ?
    )");
    movStmt.bind(1, comprobanteId);

    ComprobanteRequest notaAnulacion;
    notaAnulacion.tipoComprobante = "NOTA_CONTABLE";
    notaAnulacion.fecha = "date('now', 'localtime')"; // En producción pasar fecha actual formateada YYYY-MM-DD
    notaAnulacion.descripcion = std::format("Anulación de {} #{}: {}", tipoOrig, consOrig, motivo);
    notaAnulacion.usuarioId = usuarioId;

    while (movStmt.executeStep()) {
        LineaMovimiento inv;
        inv.cuentaCodigo = movStmt.getColumn(0).getString();
        inv.terceroId = movStmt.getColumn(1).getInt();
        if (!movStmt.getColumn(2).isNull()) {
            inv.unidadId = movStmt.getColumn(2).getInt();
        }
        inv.detalle = "Reversión: " + movStmt.getColumn(3).getString();
        // Invertir naturaleza
        inv.debito = movStmt.getColumn(5).getInt64();
        inv.credito = movStmt.getColumn(4).getInt64();
        notaAnulacion.movimientos.push_back(inv);
    }

    // Marcar original como ANULADO y registrar la nota contable inversa
    SQLite::Statement updateOrig(db, "UPDATE comprobantes_contables SET estado = 'ANULADO' WHERE id = ?");
    updateOrig.bind(1, comprobanteId);
    updateOrig.exec();

    return registrarComprobante(notaAnulacion);
}

std::vector<SaldoCuenta> AccountingEngine::obtenerBalancePrueba(const std::string& fechaInicio,
                                                                const std::string& fechaFin) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<SaldoCuenta> resultado;

    SQLite::Statement query(db, R"(
        SELECT
            p.codigo,
            p.nombre,
            p.clase,
            p.naturaleza,
            COALESCE(SUM(m.debito), 0) AS total_debito,
            COALESCE(SUM(m.credito), 0) AS total_credito
        FROM puc_cuentas p
        LEFT JOIN movimientos_contables m ON p.codigo = m.cuenta_codigo
        LEFT JOIN comprobantes_contables c ON m.comprobante_id = c.id
            AND c.estado = 'CONTABILIZADO'
            AND c.fecha BETWEEN ? AND ?
        WHERE p.permite_movimiento = 1
        GROUP BY p.codigo, p.nombre, p.clase, p.naturaleza
        HAVING total_debito > 0 OR total_credito > 0
        ORDER BY p.codigo ASC
    )");

    query.bind(1, fechaInicio);
    query.bind(2, fechaFin);

    while (query.executeStep()) {
        SaldoCuenta sc;
        sc.codigo = query.getColumn(0).getString();
        sc.nombre = query.getColumn(1).getString();
        sc.clase = query.getColumn(2).getString();
        sc.naturaleza = query.getColumn(3).getString();
        sc.totalDebito = query.getColumn(4).getInt64();
        sc.totalCredito = query.getColumn(5).getInt64();

        if (sc.naturaleza == "DEBITO") {
            sc.saldoFinal = sc.totalDebito - sc.totalCredito;
        } else {
            sc.saldoFinal = sc.totalCredito - sc.totalDebito;
        }

        resultado.push_back(sc);
    }

    return resultado;
}

int64_t AccountingEngine::obtenerSaldoTerceroCuenta(int terceroId, const std::string& cuentaCodigo) {
    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement query(db, R"(
        SELECT
            p.naturaleza,
            COALESCE(SUM(m.debito), 0),
            COALESCE(SUM(m.credito), 0)
        FROM puc_cuentas p
        LEFT JOIN movimientos_contables m ON p.codigo = m.cuenta_codigo AND m.tercero_id = ?
        LEFT JOIN comprobantes_contables c ON m.comprobante_id = c.id AND c.estado = 'CONTABILIZADO'
        WHERE p.codigo = ?
        GROUP BY p.naturaleza
    )");

    query.bind(1, terceroId);
    query.bind(2, cuentaCodigo);

    if (!query.executeStep()) return 0;

    std::string naturaleza = query.getColumn(0).getString();
    int64_t debito = query.getColumn(1).getInt64();
    int64_t credito = query.getColumn(2).getInt64();
    
    return (naturaleza == "DEBITO") ? (debito - credito) : (credito - debito);
}
