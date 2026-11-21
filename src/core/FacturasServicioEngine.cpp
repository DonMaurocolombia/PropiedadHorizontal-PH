// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FacturasServicioEngine.hpp"

#include "AccountingEngine.hpp"
#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <algorithm>
#include <chrono>
#include <format>
#include <limits>
#include <stdexcept>

namespace {
bool fechaIsoValida(const std::string& fecha) {
    if (fecha.size() != 10 || fecha[4] != '-' || fecha[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < fecha.size(); ++i) {
        if (i != 4 && i != 7 && (fecha[i] < '0' || fecha[i] > '9')) {
            return false;
        }
    }

    const int anio = std::stoi(fecha.substr(0, 4));
    const unsigned mes = static_cast<unsigned>(std::stoi(fecha.substr(5, 2)));
    const unsigned dia = static_cast<unsigned>(std::stoi(fecha.substr(8, 2)));
    return std::chrono::year_month_day{
        std::chrono::year{anio}, std::chrono::month{mes}, std::chrono::day{dia}
    }.ok();
}

std::string categoriaServicio(const std::string& codigoConcepto) {
    if (codigoConcepto == "PARQ_VISI" || codigoConcepto == "PARQ_COMU") {
        return "PARQUEADERO_VEHICULO";
    }
    if (codigoConcepto == "SALON_SOC") {
        return "ALQUILER_SALON_COMUNAL";
    }
    if (codigoConcepto == "ASADOS" || codigoConcepto == "ZONA_ASADOS") {
        return "USO_ZONA_ASADOS";
    }
    return "OTRO";
}

void registrarAuditoria(int usuarioId, const std::string& accion, int facturaId, const std::string& detalle) {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (
            usuario_id, modulo, accion, tabla_afectada, registro_id, detalle
        ) VALUES (?, 'FACTURACION_SERVICIOS', ?, 'facturas_servicio', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, accion);
    audit.bind(3, facturaId);
    audit.bind(4, detalle);
    audit.exec();
}
}

int FacturasServicioEngine::crearBorrador(
    int terceroId,
    int conceptoId,
    int unidadVisitadaId,
    const std::string& placaVehiculo,
    const std::string& fechaEmision,
    const std::string& fechaVencimiento,
    const std::vector<FacturaServicioLineaRequest>& lineas,
    int usuarioId) {
    if (terceroId <= 0 || conceptoId <= 0 || unidadVisitadaId <= 0 || usuarioId <= 0) {
        throw std::invalid_argument("Debe seleccionar el responsable, el concepto de cobro, la unidad visitada y un usuario válido.");
    }
    if (!fechaIsoValida(fechaEmision) || !fechaIsoValida(fechaVencimiento)
        || fechaVencimiento < fechaEmision) {
        throw std::invalid_argument("Las fechas deben ser válidas y el vencimiento no puede ser anterior a la emisión.");
    }
    if (lineas.empty()) {
        throw std::invalid_argument("El borrador debe tener al menos una línea de servicio.");
    }

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement cliente(
        db, "SELECT activo, es_propietario, es_inquilino, es_residente, es_visitante "
            "FROM terceros WHERE id = ?"
    );
    cliente.bind(1, terceroId);
    if (!cliente.executeStep()
        || cliente.getColumn(0).getInt() != 1
        || (cliente.getColumn(1).getInt() != 1
            && cliente.getColumn(2).getInt() != 1
            && cliente.getColumn(3).getInt() != 1
            && cliente.getColumn(4).getInt() != 1)) {
        throw std::invalid_argument("El responsable debe ser un tercero activo registrado como propietario, residente o visitante.");
    }

    SQLite::Statement concepto(db, R"(
        SELECT codigo
        FROM conceptos_cobro
        WHERE id = ? AND tipo_concepto = 'SERVICIO'
    )");
    concepto.bind(1, conceptoId);
    if (!concepto.executeStep()) {
        throw std::invalid_argument("El concepto de servicio seleccionado no existe.");
    }
    const std::string tipoServicio = categoriaServicio(concepto.getColumn(0).getString());
    if (tipoServicio == "PARQUEADERO_VEHICULO"
        && placaVehiculo.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("Debe indicar la placa del vehículo para cobrar el servicio de parqueadero.");
    }

    SQLite::Statement unidad(db, "SELECT estado FROM unidades_privadas WHERE id = ?");
    unidad.bind(1, unidadVisitadaId);
    if (!unidad.executeStep() || unidad.getColumn(0).getString() != "ACTIVA") {
        throw std::invalid_argument("La unidad privada visitada no existe o no está activa.");
    }

    if (placaVehiculo.size() > 20) {
        throw std::invalid_argument("La placa del vehículo no puede superar 20 caracteres.");
    }

    int64_t subtotal = 0;
    constexpr int64_t ivaTotal = 0;
    for (const auto& linea : lineas) {
        if (linea.descripcion.find_first_not_of(" \t\r\n") == std::string::npos
            || linea.valorBaseCop <= 0 || linea.valorBaseCop > 1000000000000LL) {
            throw std::invalid_argument("Cada línea requiere descripción y un valor positivo.");
        }
        if (linea.valorBaseCop > std::numeric_limits<int64_t>::max() - subtotal) {
            throw std::overflow_error("El subtotal excede el valor máximo permitido.");
        }
        subtotal += linea.valorBaseCop;
    }
    const int64_t total = subtotal + ivaTotal;

    SQLite::Transaction transaction(db);
    SQLite::Statement insertar(db, R"(
        INSERT INTO facturas_servicio (
            tercero_id, fecha_emision, fecha_vencimiento,
            subtotal_cop, iva_total_cop, total_cop, usuario_id
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    insertar.bind(1, terceroId);
    insertar.bind(2, fechaEmision);
    insertar.bind(3, fechaVencimiento);
    insertar.bind(4, subtotal);
    insertar.bind(5, ivaTotal);
    insertar.bind(6, total);
    insertar.bind(7, usuarioId);
    insertar.exec();
    const int id = static_cast<int>(db.getLastInsertRowid());

    SQLite::Statement contexto(db, R"(
        INSERT INTO facturas_servicio_contexto (
            factura_id, unidad_visitada_id, tipo_servicio, placa_vehiculo
        ) VALUES (?, ?, ?, ?)
    )");
    contexto.bind(1, id);
    contexto.bind(2, unidadVisitadaId);
    contexto.bind(3, tipoServicio);
    contexto.bind(4, placaVehiculo);
    contexto.exec();

    SQLite::Statement cobro(db, R"(
        INSERT INTO facturas_servicio_cobro (factura_id, concepto_id, estado)
        VALUES (?, ?, 'BORRADOR')
    )");
    cobro.bind(1, id);
    cobro.bind(2, conceptoId);
    cobro.exec();

    SQLite::Statement insertarLinea(db, R"(
        INSERT INTO facturas_servicio_detalle (
            factura_id, descripcion, valor_base_cop,
            iva_tasa_puntos_base, iva_cop
        ) VALUES (?, ?, ?, ?, ?)
    )");
    for (const auto& linea : lineas) {
        insertarLinea.bind(1, id);
        insertarLinea.bind(2, linea.descripcion);
        insertarLinea.bind(3, linea.valorBaseCop);
        insertarLinea.bind(4, 0);
        insertarLinea.bind(5, 0);
        insertarLinea.exec();
        insertarLinea.reset();
    }

    registrarAuditoria(usuarioId, "CREAR_BORRADOR", id,
                       "Servicio: " + tipoServicio
                       + "; concepto ID: " + std::to_string(conceptoId)
                       + "; unidad visitada ID: " + std::to_string(unidadVisitadaId)
                       + "; total COP sin IVA: " + std::to_string(subtotal));
    transaction.commit();
    return id;
}

void FacturasServicioEngine::confirmarPago(int facturaId,
                                           const std::string& fechaPago,
                                           const std::string& formaPago,
                                           const std::string& cuentaBancoCodigo,
                                           const std::string& referencia,
                                           int usuarioId) {
    static const std::vector<std::string> formasPago = {
        "TRANSFERENCIA", "CONSIGNACION", "PSE", "EFECTIVO", "DATAFONO"
    };
    if (facturaId <= 0 || usuarioId <= 0 || !fechaIsoValida(fechaPago)
        || std::find(formasPago.begin(), formasPago.end(), formaPago) == formasPago.end()
        || cuentaBancoCodigo.empty()) {
        throw std::invalid_argument("Los datos del pago están incompletos o son inválidos.");
    }

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);
    int terceroId = 0;
    int64_t valor = 0;
    int unidadId = 0;
    std::string cuentaIngreso;
    std::string nombreConcepto;
    {
        SQLite::Statement factura(db, R"(
            SELECT f.tercero_id, f.total_cop, ctx.unidad_visitada_id,
                   cob.estado, con.cuenta_ingreso_codigo, con.nombre
            FROM facturas_servicio f
            INNER JOIN facturas_servicio_contexto ctx
                ON ctx.factura_id = f.id
            INNER JOIN facturas_servicio_cobro cob
                ON cob.factura_id = f.id
            INNER JOIN conceptos_cobro con ON con.id = cob.concepto_id
            WHERE f.id = ?
        )");
        factura.bind(1, facturaId);
        if (!factura.executeStep()) {
            throw std::invalid_argument("No se encontró el borrador o no tiene un concepto configurado.");
        }
        if (factura.getColumn(3).getString() != "BORRADOR") {
            throw std::runtime_error("Solo se puede confirmar el pago de un borrador pendiente.");
        }
        terceroId = factura.getColumn(0).getInt();
        valor = factura.getColumn(1).getInt64();
        unidadId = factura.getColumn(2).getInt();
        cuentaIngreso = factura.getColumn(4).getString();
        nombreConcepto = factura.getColumn(5).getString();
    }

    SQLite::Statement cuentaPago(db, R"(
        SELECT clase, naturaleza, permite_movimiento
        FROM puc_cuentas WHERE codigo = ?
    )");
    cuentaPago.bind(1, cuentaBancoCodigo);
    if (!cuentaPago.executeStep()
        || cuentaPago.getColumn(0).getString() != "ACTIVO"
        || cuentaPago.getColumn(1).getString() != "DEBITO"
        || cuentaPago.getColumn(2).getInt() != 1
        || cuentaBancoCodigo.rfind("11", 0) != 0) {
        throw std::invalid_argument("Seleccione una cuenta de caja o bancos válida.");
    }

    const int siguiente = db.execAndGet(
        "SELECT COALESCE(MAX(id), 0) + 1 FROM facturas_servicio_pagos"
    ).getInt();
    const std::string recibo = std::format("RS-{:06d}", siguiente);

    ComprobanteRequest comprobante;
    comprobante.tipoComprobante = "INGRESO";
    comprobante.fecha = fechaPago;
    comprobante.descripcion = std::format("Pago de servicio {} - borrador {}", nombreConcepto, facturaId);
    comprobante.usuarioId = usuarioId;
    comprobante.movimientos.push_back({
        cuentaBancoCodigo, terceroId, unidadId, "Pago recibido " + recibo, valor, 0
    });
    comprobante.movimientos.push_back({
        cuentaIngreso, terceroId, unidadId, "Ingreso por servicio " + recibo, 0, valor
    });
    const int comprobanteId = AccountingEngine::registrarComprobanteEnTransaccion(comprobante);
    if (comprobanteId <= 0) {
        throw std::runtime_error("No se pudo contabilizar el pago de la factura de servicio.");
    }

    SQLite::Statement pago(db, R"(
        INSERT INTO facturas_servicio_pagos (
            factura_id, numero_recibo, fecha_pago, forma_pago,
            cuenta_banco_codigo, referencia_bancaria, valor_pagado,
            comprobante_contable_id, usuario_id
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    pago.bind(1, facturaId);
    pago.bind(2, recibo);
    pago.bind(3, fechaPago);
    pago.bind(4, formaPago);
    pago.bind(5, cuentaBancoCodigo);
    pago.bind(6, referencia);
    pago.bind(7, valor);
    pago.bind(8, comprobanteId);
    pago.bind(9, usuarioId);
    pago.exec();

    SQLite::Statement actualizar(db, R"(
        UPDATE facturas_servicio_cobro
        SET estado = 'PAGADA', comprobante_contable_id = ?
        WHERE factura_id = ? AND estado = 'BORRADOR'
    )");
    actualizar.bind(1, comprobanteId);
    actualizar.bind(2, facturaId);
    actualizar.exec();
    if (db.getChanges() != 1) {
        throw std::runtime_error("El estado del borrador cambió durante el registro del pago.");
    }

    registrarAuditoria(usuarioId, "CONFIRMAR_PAGO", facturaId,
                       "Recibo: " + recibo + "; total COP: " + std::to_string(valor));
    transaction.commit();
}

void FacturasServicioEngine::asignarProximaCuenta(int facturaId, int usuarioId) {
    if (facturaId <= 0 || usuarioId <= 0) {
        throw std::invalid_argument("Borrador o usuario inválido.");
    }
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);
    SQLite::Statement asignar(db, R"(
        UPDATE facturas_servicio_cobro
        SET estado = 'POR_COBRAR_UNIDAD'
        WHERE factura_id = ? AND estado = 'BORRADOR'
          AND concepto_id IS NOT NULL
          AND EXISTS (
              SELECT 1
              FROM facturas_servicio_contexto ctx
              INNER JOIN unidades_privadas u ON u.id = ctx.unidad_visitada_id
              WHERE ctx.factura_id = ? AND u.estado = 'ACTIVA'
          )
    )");
    asignar.bind(1, facturaId);
    asignar.bind(2, facturaId);
    asignar.exec();
    if (db.getChanges() != 1) {
        throw std::runtime_error("El borrador no está pendiente, no tiene concepto contable o carece de unidad activa.");
    }
    registrarAuditoria(usuarioId, "ASIGNAR_UNIDAD", facturaId,
                       "Cobro enviado a la próxima cuenta del inmueble visitado");
    transaction.commit();
}

std::vector<FacturaServicioBorrador> FacturasServicioEngine::listarBorradores() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement consulta(db, R"(
        SELECT f.id, f.tercero_id, t.nombre_razon_social, t.tipo_documento,
               t.numero_documento, c.tipo_servicio, c.unidad_visitada_id,
               COALESCE(u.nomenclatura, ''), COALESCE(c.placa_vehiculo, ''),
               f.fecha_emision, f.fecha_vencimiento,
               f.subtotal_cop, f.iva_total_cop, f.total_cop,
               COALESCE(cob.concepto_id, 0), COALESCE(con.nombre, ''),
               COALESCE(cob.estado, 'BORRADOR')
        FROM facturas_servicio f
        INNER JOIN terceros t ON t.id = f.tercero_id
        LEFT JOIN facturas_servicio_contexto c
            ON c.factura_id = f.id
        LEFT JOIN unidades_privadas u ON u.id = c.unidad_visitada_id
        LEFT JOIN facturas_servicio_cobro cob ON cob.factura_id = f.id
        LEFT JOIN conceptos_cobro con ON con.id = cob.concepto_id
        ORDER BY f.id DESC
    )");

    std::vector<FacturaServicioBorrador> borradores;
    while (consulta.executeStep()) {
        FacturaServicioBorrador borrador;
        borrador.id = consulta.getColumn(0).getInt();
        borrador.terceroId = consulta.getColumn(1).getInt();
        borrador.terceroNombre = consulta.getColumn(2).getString();
        borrador.tipoDocumento = consulta.getColumn(3).getString();
        borrador.numeroDocumento = consulta.getColumn(4).getString();
        if (!consulta.getColumn(5).isNull()) {
            borrador.tipoServicio = consulta.getColumn(5).getString();
            borrador.unidadVisitadaId = consulta.getColumn(6).getInt();
            borrador.unidadVisitada = consulta.getColumn(7).getString();
            borrador.placaVehiculo = consulta.getColumn(8).getString();
        }
        borrador.fechaEmision = consulta.getColumn(9).getString();
        borrador.fechaVencimiento = consulta.getColumn(10).getString();
        borrador.subtotalCop = consulta.getColumn(11).getInt64();
        borrador.ivaTotalCop = consulta.getColumn(12).getInt64();
        borrador.totalCop = consulta.getColumn(13).getInt64();
        borrador.conceptoId = consulta.getColumn(14).getInt();
        borrador.conceptoNombre = consulta.getColumn(15).getString();
        borrador.estado = consulta.getColumn(16).getString();
        borradores.push_back(std::move(borrador));
    }
    return borradores;
}
