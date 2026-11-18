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


#include "BudgetEngine.hpp"
#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QPrinter>
#include <QTextDocument>
#include <QPageSize>
#include <QLocale>
#include <QString>
#include <stdexcept>
#include <format>
#include <cmath>

static QString fmtCop(int64_t val) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(val));
}

int BudgetEngine::crearPresupuestoAnual(const PresupuestoEncabezado& pres, int usuarioId) {
    if (pres.anio < 2000 || pres.anio > 2100) {
        throw std::runtime_error("El año presupuestal no es válido.");
    }
    if (pres.porcentajeFondoImprevistos < 1.0) {
        throw std::runtime_error(
            "De acuerdo con el Art. 35 de la Ley 675 de 2001, el Fondo de Imprevistos "
            "no puede ser inferior al 1.0% del presupuesto anual."
        );
    }

    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement check(db, "SELECT COUNT(*) FROM presupuestos WHERE anio = ?");
    check.bind(1, pres.anio);
    check.executeStep();
    if (check.getColumn(0).getInt() > 0) {
        throw std::runtime_error(std::format("Ya existe un presupuesto creado para el año {}.", pres.anio));
    }

    SQLite::Transaction transaction(db);

    SQLite::Statement ins(db, R"(
        INSERT INTO presupuestos (
            anio, fecha_aprobacion_asamblea, acta_asamblea_numero,
            valor_total_anual, porcentaje_fondo_imprevistos, estado
        ) VALUES (?, ?, ?, 0, ?, 'VIGENTE')
    )");
    ins.bind(1, pres.anio);
    ins.bind(2, pres.fechaAprobacionAsamblea);
    ins.bind(3, pres.actaAsambleaNumero);
    ins.bind(4, pres.porcentajeFondoImprevistos);
    ins.exec();

    int newId = static_cast<int>(db.getLastInsertRowid());

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PRESUPUESTO', 'CREAR', 'presupuestos', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, newId);
    audit.bind(3, std::format("Creación presupuesto anual {} (Acta: {})", pres.anio, pres.actaAsambleaNumero));
    audit.exec();

    transaction.commit();
    return newId;
}

std::vector<PresupuestoEncabezado> BudgetEngine::listarPresupuestos() {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<PresupuestoEncabezado> lista;

    SQLite::Statement q(db, R"(
        SELECT id, anio, fecha_aprobacion_asamblea, COALESCE(acta_asamblea_numero, ''),
               valor_total_anual, porcentaje_fondo_imprevistos, estado
        FROM presupuestos
        ORDER BY anio DESC
    )");

    while (q.executeStep()) {
        lista.push_back({
            q.getColumn(0).getInt(),
            q.getColumn(1).getInt(),
            q.getColumn(2).getString(),
            q.getColumn(3).getString(),
            q.getColumn(4).getInt64(),
            q.getColumn(5).getDouble(),
            q.getColumn(6).getString()
        });
    }
    return lista;
}

void BudgetEngine::guardarRubroPresupuesto(int presupuestoId,
                                           const std::string& cuentaCodigo,
                                           const std::string& nombreRubro,
                                           const std::string& tipo,
                                           int64_t valorAnualAprobado,
                                           int usuarioId) {
    if (presupuestoId <= 0) throw std::runtime_error("Seleccione primero un año presupuestal.");
    if (cuentaCodigo.empty()) throw std::runtime_error("Seleccione una cuenta PUC asociada.");
    if (valorAnualAprobado <= 0) throw std::runtime_error("El valor anual aprobado debe ser mayor a $0.");

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);

    int64_t valorMensual = std::llround(static_cast<double>(valorAnualAprobado) / 12.0);

    // Verificar si ya existe esa cuenta en este presupuesto para actualizarla o insertarla
    SQLite::Statement check(db, "SELECT id FROM presupuesto_rubros WHERE presupuesto_id = ? AND cuenta_codigo = ?");
    check.bind(1, presupuestoId);
    check.bind(2, cuentaCodigo);

    if (check.executeStep()) {
        int rubroId = check.getColumn(0).getInt();
        SQLite::Statement upd(db, R"(
            UPDATE presupuesto_rubros
            SET nombre_rubro = ?, tipo = ?, valor_anual_aprobado = ?, valor_mensual_proyectado = ?
            WHERE id = ?
        )");
        upd.bind(1, nombreRubro);
        upd.bind(2, tipo);
        upd.bind(3, static_cast<int64_t>(valorAnualAprobado));
        upd.bind(4, static_cast<int64_t>(valorMensual));
        upd.bind(5, rubroId);
        upd.exec();
    } else {
        SQLite::Statement ins(db, R"(
            INSERT INTO presupuesto_rubros (
                presupuesto_id, cuenta_codigo, nombre_rubro, tipo,
                valor_anual_aprobado, valor_mensual_proyectado
            ) VALUES (?, ?, ?, ?, ?, ?)
        )");
        ins.bind(1, presupuestoId);
        ins.bind(2, cuentaCodigo);
        ins.bind(3, nombreRubro);
        ins.bind(4, tipo);
        ins.bind(5, static_cast<int64_t>(valorAnualAprobado));
        ins.bind(6, static_cast<int64_t>(valorMensual));
        ins.exec();
    }

    // Recalcular el valor_total_anual de gastos + fondo de imprevistos del encabezado
    SQLite::Statement updTotal(db, R"(
        UPDATE presupuestos
        SET valor_total_anual = COALESCE((
            SELECT SUM(valor_anual_aprobado)
            FROM presupuesto_rubros
            WHERE presupuesto_id = ? AND tipo IN ('GASTO', 'FONDO_IMPREVISTOS')
        ), 0)
        WHERE id = ?
    )");
    updTotal.bind(1, presupuestoId);
    updTotal.bind(2, presupuestoId);
    updTotal.exec();

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PRESUPUESTO', 'EDITAR', 'presupuesto_rubros', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, presupuestoId);
    audit.bind(3, std::format("Rubro {} ({}) aprobado por ${}", cuentaCodigo, nombreRubro, valorAnualAprobado));
    audit.exec();

    transaction.commit();
}

void BudgetEngine::eliminarRubro(int rubroId, int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();

    SQLite::Statement getPres(db, "SELECT presupuesto_id FROM presupuesto_rubros WHERE id = ?");
    getPres.bind(1, rubroId);
    if (!getPres.executeStep()) return;
    int presupuestoId = getPres.getColumn(0).getInt();

    SQLite::Transaction transaction(db);

    SQLite::Statement del(db, "DELETE FROM presupuesto_rubros WHERE id = ?");
    del.bind(1, rubroId);
    del.exec();

    SQLite::Statement updTotal(db, R"(
        UPDATE presupuestos
        SET valor_total_anual = COALESCE((
            SELECT SUM(valor_anual_aprobado)
            FROM presupuesto_rubros
            WHERE presupuesto_id = ? AND tipo IN ('GASTO', 'FONDO_IMPREVISTOS')
        ), 0)
        WHERE id = ?
    )");
    updTotal.bind(1, presupuestoId);
    updTotal.bind(2, presupuestoId);
    updTotal.exec();

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'PRESUPUESTO', 'ELIMINAR', 'presupuesto_rubros', ?, 'Eliminación rubro presupuestal')
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, rubroId);
    audit.exec();

    transaction.commit();
}

ResumenEjecucionPresupuestal BudgetEngine::calcularEjecucion(int presupuestoId, int mesCorte) {
    if (mesCorte < 1 || mesCorte > 12) mesCorte = 12;

    auto& db = DatabaseManager::instance().getDb();
    ResumenEjecucionPresupuestal res;
    res.mesCorte = mesCorte;

    SQLite::Statement headStmt(db, R"(
        SELECT id, anio, fecha_aprobacion_asamblea, COALESCE(acta_asamblea_numero, ''),
               valor_total_anual, porcentaje_fondo_imprevistos, estado
        FROM presupuestos WHERE id = ?
    )");
    headStmt.bind(1, presupuestoId);
    if (!headStmt.executeStep()) {
        throw std::runtime_error("Presupuesto anual no encontrado.");
    }

    res.encabezado = {
        headStmt.getColumn(0).getInt(),
        headStmt.getColumn(1).getInt(),
        headStmt.getColumn(2).getString(),
        headStmt.getColumn(3).getString(),
        headStmt.getColumn(4).getInt64(),
        headStmt.getColumn(5).getDouble(),
        headStmt.getColumn(6).getString()
    };

    std::string fechaInicioAnio = std::format("{}-01-01", res.encabezado.anio);
    std::string fechaCorte = std::format("{}-{:02d}-31", res.encabezado.anio, mesCorte);

    SQLite::Statement rubrosStmt(db, R"(
        SELECT r.id, r.cuenta_codigo, r.nombre_rubro, r.tipo,
               r.valor_anual_aprobado, r.valor_mensual_proyectado, p.naturaleza
        FROM presupuesto_rubros r
        INNER JOIN puc_cuentas p ON r.cuenta_codigo = p.codigo
        WHERE r.presupuesto_id = ?
        ORDER BY r.tipo DESC, r.cuenta_codigo ASC
    )");
    rubrosStmt.bind(1, presupuestoId);

    // Consulta preparada para obtener el movimiento real contabilizado de cada cuenta PUC en el año
    SQLite::Statement movRealStmt(db, R"(
        SELECT COALESCE(SUM(m.debito), 0), COALESCE(SUM(m.credito), 0)
        FROM movimientos_contables m
        INNER JOIN comprobantes_contables c ON m.comprobante_id = c.id
        WHERE m.cuenta_codigo = ?
          AND c.estado = 'CONTABILIZADO'
          AND c.fecha BETWEEN ? AND ?
    )");

    while (rubrosStmt.executeStep()) {
        RubroEjecucionItem item;
        item.rubroId = rubrosStmt.getColumn(0).getInt();
        item.cuentaCodigo = rubrosStmt.getColumn(1).getString();
        item.nombreRubro = rubrosStmt.getColumn(2).getString();
        item.tipo = rubrosStmt.getColumn(3).getString();
        item.valorAnualAprobado = rubrosStmt.getColumn(4).getInt64();
        item.valorMensualProyectado = rubrosStmt.getColumn(5).getInt64();

        std::string naturaleza = rubrosStmt.getColumn(6).getString();
        item.proyectadoAcumuladoCorte = item.valorMensualProyectado * mesCorte;
        movRealStmt.bind(1, item.cuentaCodigo);
        movRealStmt.bind(2, fechaInicioAnio);
        movRealStmt.bind(3, fechaCorte);
        movRealStmt.executeStep();
        int64_t deb = movRealStmt.getColumn(0).getInt64();
        int64_t cred = movRealStmt.getColumn(1).getInt64();
        movRealStmt.reset();
        item.ejecutadoRealAcumulado = (naturaleza == "DEBITO") ? (deb - cred) : (cred - deb);
        item.saldoPorEjecutar = item.valorAnualAprobado - item.ejecutadoRealAcumulado;
        item.porcentajeEjecucionAnual = (item.valorAnualAprobado > 0)
            ? static_cast<double>(item.ejecutadoRealAcumulado / item.valorAnualAprobado * 100.0)
            : 0.0;

        if (item.tipo == "INGRESO") {
            res.totalIngresosAprobados += item.valorAnualAprobado;
            res.totalIngresosEjecutados += item.ejecutadoRealAcumulado;
        } else {
            res.totalGastosAprobados += item.valorAnualAprobado;
            res.totalGastosEjecutados += item.ejecutadoRealAcumulado;
            if (item.tipo == "FONDO_IMPREVISTOS") {
                res.valorFondoImprevistosAprobado += item.valorAnualAprobado;
            }
        }

        res.rubros.push_back(item);
    }

    res.porcentajeGlobalGastos = (res.totalGastosAprobados > 0)
        ? static_cast<double>(res.totalGastosEjecutados / res.totalGastosAprobados * 100.0)
        : 0.0;

    return res;
}

void BudgetEngine::exportarEjecucionPDF(
    const ResumenEjecucionPresupuestal& res,
    const std::string& rutaPdf
) {
    auto& db = DatabaseManager::instance().getDb();
    QString razonSocial = "CONJUNTO RESIDENCIAL PH";
    QString nit = "900.000.000-1";
    SQLite::Statement cfg(db, "SELECT razon_social, nit FROM copropiedad_config WHERE id = 1");
    if (cfg.executeStep()) {
        razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
        nit = QString::fromStdString(cfg.getColumn(1).getString());
    }

    QString filas;
    for (const auto& r : res.rubros) {
        QString colorPct = (r.porcentajeEjecucionAnual > 100.0 && r.tipo != "INGRESO")
                                ? "color: #C62828; font-weight: bold;"
                                : "color: #222;";
        filas += QString(R"(
            %1
            %2 (%3)
            %4
            %5
            %6
            %8%
        )")
            .arg(QString::fromStdString(r.cuentaCodigo))
            .arg(QString::fromStdString(r.nombreRubro))
            .arg(QString::fromStdString(r.tipo))
            .arg(fmtCop(r.valorAnualAprobado))
            .arg(fmtCop(r.ejecutadoRealAcumulado))
            .arg(fmtCop(r.saldoPorEjecutar))
            .arg(colorPct)
            .arg(QString::number(r.porcentajeEjecucionAnual, 'f', 1));
    }

    QString html = QString(R"(
        ?
        ?
        ?
        %1
        NIT: %2 | Ley 675 de 2001
        INFORME DE EJECUCIÓN PRESUPUESTAL - VIGENCIA %3
        Acta Asamblea: %4 | Corte Acumulado: Mes %5 de 12
        %6
        Cuenta
        Rubro Presupuestal
        Aprobado Anual
        Ejecutado Real
        Saldo Disponible
        % Ejec.
    )")
        .arg(razonSocial, nit)
        .arg(res.encabezado.anio)
        .arg(QString::fromStdString(res.encabezado.actaAsambleaNumero))
        .arg(res.mesCorte)
        .arg(filas)
        .arg(fmtCop(res.totalGastosAprobados))
        .arg(fmtCop(res.totalGastosEjecutados))
        .arg(fmtCop(res.totalGastosAprobados - res.totalGastosEjecutados))
        .arg(QString::number(res.porcentajeGlobalGastos, 'f', 1));

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}
