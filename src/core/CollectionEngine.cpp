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


#include "CollectionEngine.hpp"
#include "DbManager.hpp"
#include "AccountingEngine.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QDebug>
#include <stdexcept>
#include <format>
#include <algorithm>
#include <map>

std::vector<DetalleDeudaUnidad> CollectionEngine::consultarDeudaPorPrelacion(int unidadId) {
    std::vector<DetalleDeudaUnidad> lista;

    try {
        auto& db = DatabaseManager::instance().getDb();

        // Prelación legal: primero conceptos de MORA (prioridad 1), luego capital ordenado por periodo ASC
        SQLite::Statement query(db, R"(
            SELECT
                c.id,
                c.numero_documento,
                c.periodo,
                d.id,
                con.codigo,
                d.descripcion,
                d.saldo_concepto
            FROM cuentas_cobro_detalle d
            INNER JOIN cuentas_cobro c ON d.cuenta_cobro_id = c.id
            INNER JOIN conceptos_cobro con ON d.concepto_id = con.id
            WHERE c.unidad_id = ?
            AND c.estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'VENCIDA')
            AND d.saldo_concepto > 0
            ORDER BY
                CASE WHEN con.codigo = 'MORA' THEN 1 ELSE 2 END ASC,
                c.periodo ASC,
                d.id ASC
        )");

        query.bind(1, unidadId);

        while (query.executeStep()) {
            DetalleDeudaUnidad item;
            item.cuentaCobroId = query.getColumn(0).getInt();
            item.numeroCuentaCobro = query.getColumn(1).getString();
            item.periodo = query.getColumn(2).getString();
            item.detalleId = query.getColumn(3).getInt();
            item.conceptoCodigo = query.getColumn(4).getString();
            item.descripcion = query.getColumn(5).getString();
            item.saldoConcepto = query.getColumn(6).getInt64();
            lista.push_back(item);
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al consultar deuda por prelacion: " << e.what();
    }

    return lista;
}

int64_t CollectionEngine::obtenerSaldoTotalUnidad(int unidadId) {
    int64_t resp = 0;

    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Statement query(db, R"(
            SELECT COALESCE(SUM(saldo_pendiente), 0)
            FROM cuentas_cobro
            WHERE unidad_id = ? AND estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'VENCIDA')
        )");
        query.bind(1, unidadId);
        query.executeStep();
        resp = query.getColumn(0).getInt64();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al obtener saldo total por unidad: " << e.what();
    }

    return resp;
}

std::string CollectionEngine::registrarPago(const ReciboCajaRequest& req) {
    if (req.unidadId <= 0) {
        throw std::runtime_error("Debe seleccionar una unidad privada.");
    }

    if (req.valorPagado <= 0) {
        throw std::runtime_error("El valor pagado debe ser mayor a cero.");
    }

    if (req.cuentaBancoCodigo.empty()) {
        throw std::runtime_error("Debe seleccionar la cuenta de banco o caja destino.");
    }

    std::string numeroRecibo = "";
    try {
        auto& db = DatabaseManager::instance().getDb();

        // Obtener propietario/tercero de la unidad
        SQLite::Statement unitStmt(db, R"(
            SELECT nomenclatura, propietario_principal_id, inquilino_actual_id, enviar_cobro_a
            FROM unidades_privadas WHERE id = ?
        )");
        unitStmt.bind(1, req.unidadId);
        if (!unitStmt.executeStep()) {
            throw std::runtime_error("La unidad privada seleccionada no existe.");
        }

        std::string nomenclatura = unitStmt.getColumn(0).getString();
        int terceroId = unitStmt.getColumn(1).getInt();
        if (unitStmt.getColumn(3).getString() == "INQUILINO" && !unitStmt.getColumn(2).isNull()) {
            terceroId = unitStmt.getColumn(2).getInt();
        }

        // Obtener conceptos pendientes ordenados por prelación legal
        auto deudasPendientes = consultarDeudaPorPrelacion(req.unidadId);

        SQLite::Transaction transaction(db);

        // Generar consecutivo de Recibo de Caja
        int siguienteCons = db.execAndGet("SELECT COALESCE(MAX(id), 0) + 1 FROM recibos_caja;").getInt();
        numeroRecibo = std::format("RC-{:06d}", siguienteCons);

        // Preparar asiento contable de INGRESO
        ComprobanteRequest compReq;
        compReq.tipoComprobante = "INGRESO";
        compReq.fecha = req.fechaPago;
        compReq.descripcion = std::format("Recibo de Caja {} - {} (Ref: {})",
                                        numeroRecibo, nomenclatura, req.referenciaBancaria);
        compReq.usuarioId = req.usuarioId;

        // 1. Débito al Banco o Caja por el total recibido
        compReq.movimientos.push_back({
            req.cuentaBancoCodigo,
            terceroId,
            req.unidadId,
            std::format("Recaudo {} - {}", numeroRecibo, req.formaPago),
            req.valorPagado,
            0
        });

        // Agrupar créditos por cuenta PUC de cartera (130505, 130515, etc.)
        std::map<std::string, int64_t> creditosPorCuenta;

        struct AplicacionDetalle {
            int detalleId;
            int cuentaCobroId;
            int64_t valorAplicado;
        };
        std::vector<AplicacionDetalle> aplicaciones;

        int64_t remanente = req.valorPagado;

        // 2. Aplicar pago según prelación legal
        for (const auto& item : deudasPendientes) {
            if (remanente <= 0) break;

            int64_t abono = std::min(remanente, item.saldoConcepto);
            remanente -= abono;

            // Buscar cuenta PUC de cartera asociada al concepto
            SQLite::Statement ctaStmt(db, "SELECT cuenta_cartera_codigo FROM conceptos_cobro WHERE codigo = ?");
            ctaStmt.bind(1, item.conceptoCodigo);
            ctaStmt.executeStep();
            std::string cuentaCartera = ctaStmt.getColumn(0).getString();

            creditosPorCuenta[cuentaCartera] += abono;
            aplicaciones.push_back({item.detalleId, item.cuentaCobroId, abono});
        }

        // Agregar créditos que disminuyen la cartera (1305xx)
        for (const auto& [cuentaPuc, valorCredito] : creditosPorCuenta) {
            compReq.movimientos.push_back({
                cuentaPuc,
                terceroId,
                req.unidadId,
                std::format("Abono Cartera {} - {}", numeroRecibo, nomenclatura),
                0,
                valorCredito
            });
        }

        // 3. Si sobra dinero después de pagar toda la deuda, registrarlo como Anticipo (Pasivo 280505)
        if (remanente > 0) {
            compReq.movimientos.push_back({
                "280505",
                terceroId,
                req.unidadId,
                std::format("Anticipo Cuotas Administración {} - {}", numeroRecibo, nomenclatura),
                0,
                remanente
            });
        }

        int comprobanteId = AccountingEngine::registrarComprobanteEnTransaccion(compReq);

        // 4. Insertar encabezado de Recibo de Caja
        SQLite::Statement insertRC(db, R"(
            INSERT INTO recibos_caja (
                numero_recibo, unidad_id, tercero_id, fecha_pago, forma_pago,
                cuenta_banco_codigo, referencia_bancaria, valor_total_pagado,
                observaciones, comprobante_contable_id, usuario_id
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");

        insertRC.bind(1, numeroRecibo);
        insertRC.bind(2, req.unidadId);
        insertRC.bind(3, terceroId);
        insertRC.bind(4, req.fechaPago);
        insertRC.bind(5, req.formaPago);
        insertRC.bind(6, req.cuentaBancoCodigo);
        insertRC.bind(7, req.referenciaBancaria);
        insertRC.bind(8, static_cast<int64_t>(req.valorPagado));
        insertRC.bind(9, req.observaciones);
        insertRC.bind(10, comprobanteId);
        insertRC.bind(11, req.usuarioId);
        insertRC.exec();

        int reciboId = static_cast<int>(db.getLastInsertRowid());

        // 5. Actualizar saldos en cuentas_cobro_detalle y cuentas_cobro
        SQLite::Statement insApl(db, R"(
            INSERT INTO recibos_caja_aplicacion (recibo_caja_id, cuenta_cobro_detalle_id, valor_aplicado)
            VALUES (?, ?, ?)
        )");
        SQLite::Statement updDet(db, R"(
            UPDATE cuentas_cobro_detalle
            SET saldo_concepto = saldo_concepto - ?
            WHERE id = ?
        )");
        SQLite::Statement updCC(db, R"(
            UPDATE cuentas_cobro
            SET saldo_pendiente = saldo_pendiente - ?,
                estado = CASE
                    WHEN (saldo_pendiente - ?) <= 0 THEN 'PAGADA'
                    ELSE 'PAGO_PARCIAL'
                END
            WHERE id = ?
        )");

        for (const auto& apl : aplicaciones) {
            insApl.bind(1, reciboId);
            insApl.bind(2, apl.detalleId);
            insApl.bind(3, static_cast<int64_t>(apl.valorAplicado));
            insApl.exec();
            insApl.reset();

            updDet.bind(1, static_cast<int64_t>(apl.valorAplicado));
            updDet.bind(2, apl.detalleId);
            updDet.exec();
            updDet.reset();

            updCC.bind(1, static_cast<int64_t>(apl.valorAplicado));
            updCC.bind(2, static_cast<int64_t>(apl.valorAplicado));
            updCC.bind(3, apl.cuentaCobroId);
            updCC.exec();
            updCC.reset();
        }

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al registrar pago: " << e.what();
    }

    return numeroRecibo;
}

std::vector<ReciboCajaResumen> CollectionEngine::listarRecibos() {
    std::vector<ReciboCajaResumen> lista;

    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Statement query(db, R"(
            SELECT
                r.id, r.numero_recibo, r.fecha_pago, u.nomenclatura,
                t.nombre_razon_social, r.forma_pago, COALESCE(r.referencia_bancaria, ''),
                r.valor_total_pagado
            FROM recibos_caja r
            INNER JOIN unidades_privadas u ON r.unidad_id = u.id
            INNER JOIN terceros t ON r.tercero_id = t.id
            ORDER BY r.id DESC
        )");

        while (query.executeStep()) {
            lista.push_back({
                query.getColumn(0).getInt(),
                query.getColumn(1).getString(),
                query.getColumn(2).getString(),
                query.getColumn(3).getString(),
                query.getColumn(4).getString(),
                query.getColumn(5).getString(),
                query.getColumn(6).getString(),
                query.getColumn(7).getInt64()
            });
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al listar recibos: " << e.what();
    }

    return lista;
}
