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


#include "BillingEngine.hpp"
#include "DbManager.hpp"
#include "AccountingEngine.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <stdexcept>
#include <cmath>
#include <format>

int BillingEngine::generarFacturacionMensual(const std::string& periodo,
                                             int64_t presupuestoMensualBase,
                                             const std::string& fechaEmision,
                                             const std::string& fechaVencimiento,
                                             int usuarioId) {
    if (periodo.size() != 7 || periodo[4] != '-') {
        throw std::runtime_error("El periodo debe tener formato YYYY-MM (ej. 2026-11).");
    }
    if (presupuestoMensualBase <= 0) {
        throw std::runtime_error("El presupuesto mensual base debe ser mayor a cero.");
    }

    auto& db = DatabaseManager::instance().getDb();

    // 1. Verificar que no exista ya facturación generada para este periodo
    SQLite::Statement checkPeriodo(db, "SELECT COUNT(*) FROM cuentas_cobro WHERE periodo = ? AND estado != 'ANULADA'");
    checkPeriodo.bind(1, periodo);
    checkPeriodo.executeStep();
    if (checkPeriodo.getColumn(0).getInt() > 0) {
        throw std::runtime_error("Ya existen cuentas de cobro generadas para el periodo " + periodo + ".");
    }

    // 2. Obtener tasa de mora de la configuración de la copropiedad
    double tasaMoraMensual = 0.018;
    int diaDescuento = 10;
    double pctDescuento = 0.0;

    SQLite::Statement cfgStmt(db, R"(
        SELECT tasa_mora_mensual, dia_limite_pronto_pago, porcentaje_descuento_pronto_pago
        FROM copropiedad_config WHERE id = 1
    )");
    if (cfgStmt.executeStep()) {
        tasaMoraMensual = cfgStmt.getColumn(0).getDouble();
        diaDescuento = cfgStmt.getColumn(1).getInt();
        pctDescuento = cfgStmt.getColumn(2).getDouble();
    }

    // 3. Cargar IDs y cuentas PUC de los conceptos de cobro base
    struct ConceptoInfo {
        int id;
        std::string nombre;
        std::string cuentaIngreso;
        std::string cuentaCartera;
        bool aplicaIva;
        double pctIva;
    };

    auto cargarConcepto = [&](const std::string& codigo) -> ConceptoInfo {
        SQLite::Statement q(db, R"(
            SELECT id, nombre, cuenta_ingreso_codigo, cuenta_cartera_codigo, aplica_iva, porcentaje_iva
            FROM conceptos_cobro WHERE codigo = ?
        )");
        q.bind(1, codigo);
        if (!q.executeStep()) {
            throw std::runtime_error("Falta configurar el concepto de cobro: " + codigo);
        }
        return {
            q.getColumn(0).getInt(),
            q.getColumn(1).getString(),
            q.getColumn(2).getString(),
            q.getColumn(3).getString(),
            q.getColumn(4).getInt() == 1,
            q.getColumn(5).getDouble()
        };
    };

    ConceptoInfo cCuotaOrd = cargarConcepto("CUOTA_ORD");
    ConceptoInfo cMora     = cargarConcepto("MORA");
    ConceptoInfo cParqAlq  = cargarConcepto("PARQ_ALQ");

    // 4. Obtener todas las unidades ACTIVAS
    SQLite::Statement unidadesStmt(db, R"(
        SELECT id, nomenclatura, coeficiente, propietario_principal_id, inquilino_actual_id, enviar_cobro_a
        FROM unidades_privadas
        WHERE estado = 'ACTIVA'
        ORDER BY id ASC
    )");

    std::string periodoCompacto = periodo.substr(0, 4) + periodo.substr(5, 2); // Ej: "202611"
    std::string fechaLimiteDesc = std::format("{}-{:02d}", periodo, diaDescuento);

    int cuentasGeneradas = 0;

    // Transacción global para todo el lote de facturación
    SQLite::Transaction transaction(db);

    while (unidadesStmt.executeStep()) {
        int unidadId = unidadesStmt.getColumn(0).getInt();
        std::string nomenclatura = unidadesStmt.getColumn(1).getString();
        double coeficiente = unidadesStmt.getColumn(2).getDouble();
        int propietarioId = unidadesStmt.getColumn(3).getInt();
        int terceroCobroId = propietarioId;

        std::string enviarA = unidadesStmt.getColumn(5).getString();
        if (enviarA == "INQUILINO" && !unidadesStmt.getColumn(4).isNull()) {
            terceroCobroId = unidadesStmt.getColumn(4).getInt();
        }

        // A. Consultar saldo anterior pendiente de la unidad
        SQLite::Statement saldoStmt(db, R"(
            SELECT COALESCE(SUM(saldo_pendiente), 0)
            FROM cuentas_cobro
            WHERE unidad_id = ? AND estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'VENCIDA')
        )");
        saldoStmt.bind(1, unidadId);
        saldoStmt.executeStep();
        int64_t saldoAnterior = saldoStmt.getColumn(0).getInt64();

        // B. Consultar capital vencido que genera interés de mora
        SQLite::Statement baseMoraStmt(db, R"(
            SELECT COALESCE(SUM(d.saldo_concepto), 0)
            FROM cuentas_cobro_detalle d
            INNER JOIN cuentas_cobro c ON d.cuenta_cobro_id = c.id
            INNER JOIN conceptos_cobro con ON d.concepto_id = con.id
            WHERE c.unidad_id = ?
              AND c.estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'VENCIDA')
              AND con.genera_interes_mora = 1
        )");
        baseMoraStmt.bind(1, unidadId);
        baseMoraStmt.executeStep();
        int64_t capitalVencido = baseMoraStmt.getColumn(0).getInt64();

        // C. Calcular valores del periodo
        int64_t valorCuotaOrd = std::llround(presupuestoMensualBase * coeficiente);
        int64_t valorMora = (capitalVencido > 0) ? std::llround(capitalVencido * tasaMoraMensual) : 0;

        // D. Verificar parqueaderos o bodegas comunes en alquiler asignados a esta unidad
        SQLite::Statement alqStmt(db, R"(
            SELECT COALESCE(SUM(canon_mensual), 0)
            FROM parqueaderos
            WHERE unidad_asignada_id = ? AND tipo_tenencia = 'COMUN_ALQUILER'
        )");
        alqStmt.bind(1, unidadId);
        alqStmt.executeStep();
        int64_t valorAlquilerParq = alqStmt.getColumn(0).getInt64();
        int64_t ivaAlquilerParq = cParqAlq.aplicaIva
            ? std::llround(valorAlquilerParq * (cParqAlq.pctIva / 100.0))
            : 0;

        int64_t valorPeriodo = valorCuotaOrd + valorMora + valorAlquilerParq + ivaAlquilerParq;
        int64_t totalAPagar = saldoAnterior + valorPeriodo;
        int64_t descuentoProntoPago = (pctDescuento > 0.0)
            ? std::llround(valorCuotaOrd * (pctDescuento / 100.0))
            : 0;

        cuentasGeneradas++;
        std::string numDoc = std::format("CC-{}-{:04d}", periodoCompacto, cuentasGeneradas);

        // E. Construir comprobante contable automático (Partida Doble)
        ComprobanteRequest compReq;
        compReq.tipoComprobante = "FACTURACION";
        compReq.fecha = fechaEmision;
        compReq.descripcion = std::format("Causación Cuenta Cobro {} - {} ({})", numDoc, nomenclatura, periodo);
        compReq.usuarioId = usuarioId;

        // Débito Cartera 130505 / Crédito Ingreso 417005 (Cuota Ordinaria)
        if (valorCuotaOrd > 0) {
            compReq.movimientos.push_back({cCuotaOrd.cuentaCartera, terceroCobroId, unidadId, "Cuota Ord. " + periodo, valorCuotaOrd, 0});
            compReq.movimientos.push_back({cCuotaOrd.cuentaIngreso, terceroCobroId, unidadId, "Ingreso Cuota Ord. " + periodo, 0, valorCuotaOrd});
        }
        // Débito Cartera 130515 / Crédito Ingreso 417015 (Intereses Mora)
        if (valorMora > 0) {
            compReq.movimientos.push_back({cMora.cuentaCartera, terceroCobroId, unidadId, "Intereses Mora " + periodo, valorMora, 0});
            compReq.movimientos.push_back({cMora.cuentaIngreso, terceroCobroId, unidadId, "Ingreso Mora " + periodo, 0, valorMora});
        }
        // Débito Cartera 130525 / Crédito Ingreso 417025 (Alquiler Parqueadero)
        if (valorAlquilerParq > 0) {
            int64_t totalAlq = valorAlquilerParq + ivaAlquilerParq;
            compReq.movimientos.push_back({cParqAlq.cuentaCartera, terceroCobroId, unidadId, "Alquiler Parqueadero " + periodo, totalAlq, 0});
            compReq.movimientos.push_back({cParqAlq.cuentaIngreso, terceroCobroId, unidadId, "Ingreso Alquiler Parq. " + periodo, 0, totalAlq});
        }

        int comprobanteId = AccountingEngine::registrarComprobante(compReq);

        // F. Marcar cuentas anteriores como VENCIDAS si tenían saldo pendiente
        SQLite::Statement vencStmt(db, R"(
            UPDATE cuentas_cobro
            SET estado = 'VENCIDA'
            WHERE unidad_id = ? AND estado IN ('PENDIENTE', 'PAGO_PARCIAL')
        )");
        vencStmt.bind(1, unidadId);
        vencStmt.exec();

        // G. Insertar encabezado de Cuenta de Cobro
        SQLite::Statement insertCC(db, R"(
            INSERT INTO cuentas_cobro (
                numero_documento, unidad_id, tercero_id, periodo,
                fecha_emision, fecha_vencimiento, fecha_limite_descuento,
                saldo_anterior, valor_periodo, valor_descuento_pronto_pago,
                total_a_pagar, saldo_pendiente, estado, comprobante_contable_id
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'PENDIENTE', ?)
        )");

        insertCC.bind(1, numDoc);
        insertCC.bind(2, unidadId);
        insertCC.bind(3, terceroCobroId);
        insertCC.bind(4, periodo);
        insertCC.bind(5, fechaEmision);
        insertCC.bind(6, fechaVencimiento);
        insertCC.bind(7, fechaLimiteDesc);
        insertCC.bind(8,  static_cast<int64_t>(saldoAnterior));
        insertCC.bind(9,  static_cast<int64_t>(valorPeriodo));
        insertCC.bind(10, static_cast<int64_t>(descuentoProntoPago));
        insertCC.bind(11, static_cast<int64_t>(totalAPagar));
        insertCC.bind(12, static_cast<int64_t>(valorPeriodo)); // Saldo propio de esta factura
        insertCC.bind(13, comprobanteId);
        insertCC.exec();

        int cuentaCobroId = static_cast<int>(db.getLastInsertRowid());

        // H. Insertar líneas de detalle
        SQLite::Statement insertDet(db, R"(
            INSERT INTO cuentas_cobro_detalle (
            cuenta_cobro_id, concepto_id, descripcion, valor, iva, saldo_concepto
            ) VALUES (?, ?, ?, ?, ?, ?)
        )");

        auto agregarDetalle = [&](int conceptoId, const std::string& desc, int64_t val, int64_t iva) {
            if (val <= 0) return;

            insertDet.bind(1, cuentaCobroId);
            insertDet.bind(2, conceptoId);
            insertDet.bind(3, desc);
            insertDet.bind(4, static_cast<int64_t>(val));
            insertDet.bind(5, static_cast<int64_t>(iva));
            insertDet.bind(6, static_cast<int64_t>(val + iva));
            insertDet.exec();
            insertDet.reset();
        };

        agregarDetalle(cCuotaOrd.id, "Cuota Ordinaria de Administración (" + periodo + ")", valorCuotaOrd, 0);
        agregarDetalle(cMora.id, "Intereses de Mora sobre Capital Vencido", valorMora, 0);
        agregarDetalle(cParqAlq.id, "Alquiler Parqueadero Común (" + periodo + ")", valorAlquilerParq, ivaAlquilerParq);
    }

    transaction.commit();
    return cuentasGeneradas;
}

// std::vector<CuentaCobroResumen> listarCuentasCobro(const std::string& periodo = "") {
std::vector<CuentaCobroResumen> BillingEngine::listarCuentasCobro(const std::string& periodo) { 
    auto& db = DatabaseManager::instance().getDb();
    std::vector<CuentaCobroResumen> lista;
    std::string sql = R"(
        SELECT
        c.id, c.numero_documento, c.unidad_id, u.nomenclatura,
        t.nombre_razon_social, c.periodo, c.fecha_emision, c.fecha_vencimiento,
        c.saldo_anterior, c.valor_periodo, c.total_a_pagar, c.saldo_pendiente, c.estado
        FROM cuentas_cobro c
        INNER JOIN unidades_privadas u ON c.unidad_id = u.id
        INNER JOIN terceros t ON c.tercero_id = t.id
    )";

    if (!periodo.empty()) {
        sql += " WHERE c.periodo = ? ";
    }

    sql += " ORDER BY c.id DESC;";
    SQLite::Statement query(db, sql);
    if (!periodo.empty()) {
        query.bind(1, periodo);
    }

    while (query.executeStep()) {
        CuentaCobroResumen r;
        r.id = query.getColumn(0).getInt();
        r.numeroDocumento = query.getColumn(1).getString();
        r.unidadId = query.getColumn(2).getInt();
        r.unidadNomenclatura = query.getColumn(3).getString();
        r.propietarioNombre = query.getColumn(4).getString();
        r.periodo = query.getColumn(5).getString();
        r.fechaEmision = query.getColumn(6).getString();
        r.fechaVencimiento = query.getColumn(7).getString();
        r.saldoAnterior = query.getColumn(8).getInt64();
        r.valorPeriodo = query.getColumn(9).getInt64();
        r.totalAPagar = query.getColumn(10).getInt64();
        r.saldoPendiente = query.getColumn(11).getInt64();
        r.estado = query.getColumn(12).getString();
        lista.push_back(r);
    }
    return lista;
}

CuentaCobroCompleta BillingEngine::obtenerCuentaCompleta(int cuentaCobroId) {
    auto& db = DatabaseManager::instance().getDb();
    CuentaCobroCompleta cc;
    SQLite::Statement query(db, R"(
        SELECT
        c.id, c.numero_documento, c.unidad_id, u.nomenclatura, u.torre_bloque, u.coeficiente,
        t.nombre_razon_social, t.numero_documento, COALESCE(t.email, ''),
        c.periodo, c.fecha_emision, c.fecha_vencimiento, COALESCE(c.fecha_limite_descuento, ''),
        c.saldo_anterior, c.valor_periodo, c.valor_descuento_pronto_pago,
        c.total_a_pagar, c.saldo_pendiente, c.estado
        FROM cuentas_cobro c
        INNER JOIN unidades_privadas u ON c.unidad_id = u.id
        INNER JOIN terceros t ON c.tercero_id = t.id
        WHERE c.id = ?
    )");

    query.bind(1, cuentaCobroId);
    if (!query.executeStep()) {
        throw std::runtime_error("No se encontró la cuenta de cobro solicitada.");
    
    }

    cc.encabezado.id = query.getColumn(0).getInt();
    cc.encabezado.numeroDocumento = query.getColumn(1).getString();
    cc.encabezado.unidadId = query.getColumn(2).getInt();
    cc.encabezado.unidadNomenclatura = query.getColumn(3).getString();
    cc.torreBloque = query.getColumn(4).getString();
    cc.coeficientePct = query.getColumn(5).getDouble() * 100.0;
    cc.encabezado.propietarioNombre = query.getColumn(6).getString();
    cc.terceroDocumento = query.getColumn(7).getString();
    cc.terceroEmail = query.getColumn(8).getString();
    cc.encabezado.periodo = query.getColumn(9).getString();
    cc.encabezado.fechaEmision = query.getColumn(10).getString();
    cc.encabezado.fechaVencimiento = query.getColumn(11).getString();
    cc.fechaLimiteDescuento = query.getColumn(12).getString();
    cc.encabezado.saldoAnterior = query.getColumn(13).getInt64();
    cc.encabezado.valorPeriodo = query.getColumn(14).getInt64();
    cc.valorDescuentoProntoPago = query.getColumn(15).getInt64();
    cc.encabezado.totalAPagar = query.getColumn(16).getInt64();
    cc.encabezado.saldoPendiente = query.getColumn(17).getInt64();
    cc.encabezado.estado = query.getColumn(18).getString();
    SQLite::Statement detStmt(db, R"(
        SELECT con.codigo, d.descripcion, d.valor, d.iva
        FROM cuentas_cobro_detalle d
        INNER JOIN conceptos_cobro con ON d.concepto_id = con.id
        WHERE d.cuenta_cobro_id = ?
    )");

    detStmt.bind(1, cuentaCobroId);
    while (detStmt.executeStep()) {
        cc.detalles.push_back({
            detStmt.getColumn(0).getString(),
            detStmt.getColumn(1).getString(),
            detStmt.getColumn(2).getInt64(),
            detStmt.getColumn(3).getInt64()
        });
    }
    
    return cc;
}
