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


#include "ReportsEngine.hpp"
#include "DbManager.hpp"
#include "core/PostgresDatabase.hpp"
#include <QPrinter>
#include <QTextDocument>
#include <QPageSize>
#include <QLocale>
#include <QString>

static QString fmtCop(int64_t valor) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(valor));
}

static QString construirEncabezadoCopropiedadHTML(const QString& tituloReporte, const QString& subtitulo) {
    auto& db = DatabaseManager::instance().getDb();
    QString razonSocial = "CONJUNTO RESIDENCIAL PROPIEDAD HORIZONTAL";
    QString nit = "900.000.000-1";

    Db::Statement cfg(db, "SELECT razon_social, nit FROM copropiedad_config WHERE id = 1");
    if (cfg.executeStep()) {
        razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
        nit = QString::fromStdString(cfg.getColumn(1).getString());
    }

    return QString(R"(
        <div style="text-align: center; border-bottom: 2px solid #1976D2; padding-bottom: 10px; margin-bottom: 15px;">
            <h2 style="margin: 0; color: #1976D2;">%1</h2>
            <p style="margin: 2px 0; font-size: 9.5pt;">NIT: %2 | Régimen de Propiedad Horizontal</p>
            <h3 style="margin: 8px 0 2px 0; color: #222;">%3</h3>
            <p style="margin: 0; font-size: 9pt; color: #555;">%4</p>
        </div>
    )").arg(razonSocial, nit, tituloReporte, subtitulo);
}

static QString construirFirmasHTML() {
    auto& db = DatabaseManager::instance().getDb();
    QString repLegal = "Representante Legal";
    QString contador = "Contador Público";
    QString revisor = "";

    Db::Statement cfg(db, R"(
        SELECT representante_legal,
               COALESCE(contador_nombre || ' (TP ' || contador_tp || ')', 'Contador Público'),
               COALESCE(revisor_fiscal_nombre || ' (TP ' || revisor_fiscal_tp || ')', '')
        FROM copropiedad_config WHERE id = 1
    )");
    if (cfg.executeStep()) {
        repLegal = QString::fromStdString(cfg.getColumn(0).getString());
        contador = QString::fromStdString(cfg.getColumn(1).getString());
        revisor = QString::fromStdString(cfg.getColumn(2).getString());
    }

    QString html = QString(R"(
        <br/><br/><br/>
        <table width="100%" style="font-size: 9pt; text-align: center;">
            <tr>
                <td width="33%">___________________________<br/><strong>%1</strong><br/>Administrador(a)</td>
                <td width="33%">___________________________<br/><strong>%2</strong><br/>Contador(a)</td>
    )").arg(repLegal, contador);

    if (!revisor.isEmpty()) {
        html += QString(R"(<td width="33%">___________________________<br/><strong>%1</strong><br/>Revisor(a) Fiscal</td>)").arg(revisor);
    }

    html += "</tr></table>";
    return html;
}

std::vector<CarteraEdadItem> ReportsEngine::generarCarteraPorEdades(const std::string& fechaCorte) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<CarteraEdadItem> lista;

    Db::Statement query(db, R"(
        SELECT
            u.id,
            COALESCE(u.torre_bloque, ''),
            u.nomenclatura,
            t.nombre_razon_social,
            COALESCE(SUM(CASE WHEN (CAST(? AS date) - CAST(c.fecha_vencimiento AS date)) <= 30 THEN c.saldo_pendiente ELSE 0 END), 0) AS d_0_30,
            COALESCE(SUM(CASE WHEN (CAST(? AS date) - CAST(c.fecha_vencimiento AS date)) BETWEEN 31 AND 60 THEN c.saldo_pendiente ELSE 0 END), 0) AS d_31_60,
            COALESCE(SUM(CASE WHEN (CAST(? AS date) - CAST(c.fecha_vencimiento AS date)) BETWEEN 61 AND 90 THEN c.saldo_pendiente ELSE 0 END), 0) AS d_61_90,
            COALESCE(SUM(CASE WHEN (CAST(? AS date) - CAST(c.fecha_vencimiento AS date)) > 90 THEN c.saldo_pendiente ELSE 0 END), 0) AS d_90_mas,
            COALESCE(SUM(c.saldo_pendiente), 0) AS total
        FROM unidades_privadas u
        INNER JOIN terceros t ON u.propietario_principal_id = t.id
        INNER JOIN cuentas_cobro c ON u.id = c.unidad_id
        WHERE c.estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'VENCIDA')
          AND c.saldo_pendiente > 0
          AND CAST(c.fecha_emision AS date) <= CAST(? AS date)
        GROUP BY u.id, u.torre_bloque, u.nomenclatura, t.nombre_razon_social
        HAVING COALESCE(SUM(c.saldo_pendiente), 0) > 0
        ORDER BY total DESC, u.nomenclatura ASC
    )");

    query.bind(1, fechaCorte);
    query.bind(2, fechaCorte);
    query.bind(3, fechaCorte);
    query.bind(4, fechaCorte);
    query.bind(5, fechaCorte);

    while (query.executeStep()) {
        lista.push_back({
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getString(),
            query.getColumn(3).getString(),
            query.getColumn(4).getInt64(),
            query.getColumn(5).getInt64(),
            query.getColumn(6).getInt64(),
            query.getColumn(7).getInt64(),
            query.getColumn(8).getInt64()
        });
    }

    return lista;
}

EstadoResultadosResumen ReportsEngine::generarEstadoResultados(const std::string& fechaInicio,
                                                               const std::string& fechaFin) {
    auto& db = DatabaseManager::instance().getDb();
    EstadoResultadosResumen er;

    Db::Statement query(db, R"(
        SELECT
            p.codigo,
            p.nombre,
            p.clase,
            p.naturaleza,
            COALESCE(SUM(m.debito), 0) AS deb,
            COALESCE(SUM(m.credito), 0) AS cred
        FROM puc_cuentas p
        INNER JOIN movimientos_contables m ON p.codigo = m.cuenta_codigo
        INNER JOIN comprobantes_contables c ON m.comprobante_id = c.id
        WHERE p.clase IN ('INGRESOS', 'GASTOS')
          AND c.estado = 'CONTABILIZADO'
          AND c.fecha BETWEEN ? AND ?
        GROUP BY p.codigo, p.nombre, p.clase, p.naturaleza
        HAVING COALESCE(SUM(m.debito), 0) > 0 OR COALESCE(SUM(m.credito), 0) > 0
        ORDER BY p.codigo ASC
    )");

    query.bind(1, fechaInicio);
    query.bind(2, fechaFin);

    while (query.executeStep()) {
        std::string codigo = query.getColumn(0).getString();
        std::string nombre = query.getColumn(1).getString();
        std::string clase = query.getColumn(2).getString();
        std::string naturaleza = query.getColumn(3).getString();
        int64_t deb = query.getColumn(4).getInt64();
        int64_t cred = query.getColumn(5).getInt64();

        int64_t saldo = (naturaleza == "CREDITO") ? (cred - deb) : (deb - cred);

        if (clase == "INGRESOS") {
            er.ingresos.push_back({codigo, nombre, clase, saldo});
            er.totalIngresos += saldo;
        } else {
            er.gastos.push_back({codigo, nombre, clase, saldo});
            er.totalGastos += saldo;
        }
    }

    er.excedenteODeficit = er.totalIngresos - er.totalGastos;
    return er;
}

BalanceGeneralResumen ReportsEngine::generarBalanceGeneral(const std::string& fechaCorte) {
    auto& db = DatabaseManager::instance().getDb();
    BalanceGeneralResumen bg;

    Db::Statement query(db, R"(
        SELECT
            p.codigo,
            p.nombre,
            p.clase,
            p.naturaleza,
            COALESCE(SUM(m.debito), 0) AS deb,
            COALESCE(SUM(m.credito), 0) AS cred
        FROM puc_cuentas p
        INNER JOIN movimientos_contables m ON p.codigo = m.cuenta_codigo
        INNER JOIN comprobantes_contables c ON m.comprobante_id = c.id
        WHERE p.clase IN ('ACTIVO', 'PASIVO', 'PATRIMONIO')
          AND c.estado = 'CONTABILIZADO'
          AND c.fecha <= ?
        GROUP BY p.codigo, p.nombre, p.clase, p.naturaleza
        HAVING COALESCE(SUM(m.debito), 0) <> COALESCE(SUM(m.credito), 0)
        ORDER BY p.codigo ASC
    )");

    query.bind(1, fechaCorte);

    while (query.executeStep()) {
        std::string codigo = query.getColumn(0).getString();
        std::string nombre = query.getColumn(1).getString();
        std::string clase = query.getColumn(2).getString();
        std::string naturaleza = query.getColumn(3).getString();
        int64_t deb = query.getColumn(4).getInt64();
        int64_t cred = query.getColumn(5).getInt64();

        int64_t saldo = (naturaleza == "DEBITO") ? (deb - cred) : (cred - deb);

        if (clase == "ACTIVO") {
            bg.activos.push_back({codigo, nombre, clase, saldo});
            bg.totalActivos += saldo;
        } else if (clase == "PASIVO") {
            bg.pasivos.push_back({codigo, nombre, clase, saldo});
            bg.totalPasivos += saldo;
        } else if (clase == "PATRIMONIO") {
            bg.patrimonio.push_back({codigo, nombre, clase, saldo});
            bg.totalPatrimonio += saldo;
        }
    }

    // Calcular resultado acumulado (Ingresos - Gastos hasta fechaCorte) para cuadrar Patrimonio
    auto erAcumulado = generarEstadoResultados("1900-01-01", fechaCorte);
    bg.resultadoEjercicio = erAcumulado.excedenteODeficit;
    bg.totalPatrimonio += bg.resultadoEjercicio;

    return bg;
}

void ReportsEngine::exportarCarteraPDF(const std::vector<CarteraEdadItem>& datos,
                                       const std::string& fechaCorte,
                                       const std::string& rutaPdf) {
    int64_t sum30 = 0, sum60 = 0, sum90 = 0, sum90Mas = 0, sumTotal = 0;
    QString filas;

    for (const auto& item : datos) {
        sum30 += item.de0a30Dias;
        sum60 += item.de31a60Dias;
        sum90 += item.de61a90Dias;
        sum90Mas += item.masDe90Dias;
        sumTotal += item.totalDeuda;

        filas += QString(R"(
            <tr>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%1 - %2</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%3</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd; text-align: right;">%4</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd; text-align: right;">%5</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%6</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%7</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%8</td>
        )")
        .arg(QString::fromStdString(item.torreBloque))
        .arg(QString::fromStdString(item.nomenclatura))
        .arg(QString::fromStdString(item.propietarioNombre))
        .arg(fmtCop(item.de0a30Dias))
        .arg(fmtCop(item.de31a60Dias))
        .arg(fmtCop(item.de61a90Dias))
        .arg(fmtCop(item.masDe90Dias))
        .arg(fmtCop(item.totalDeuda));
    }

    QString html = "";
    html += construirEncabezadoCopropiedadHTML(
        "INFORME DE CARTERA POR EDADES",
        "Fecha de Corte: " + QString::fromStdString(fechaCorte)
    );

    html += QString(R"(
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">%1</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">Unidad</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">Propietario</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">0 - 30 Días</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">31 - 60 Días</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">61 - 90 Días</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">> 90 Días</td>
                <td style="padding: 6px; border-bottom: 1px solid #ddd;">Total Saldo</td>
    )")
    .arg(filas)
    .arg(fmtCop(sum30))
    .arg(fmtCop(sum60))
    .arg(fmtCop(sum90))
    .arg(fmtCop(sum90Mas))
    .arg(fmtCop(sumTotal));

    html += construirFirmasHTML() + "";
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}

void ReportsEngine::exportarEstadoResultadosPDF(
    const EstadoResultadosResumen& er,
    const std::string& fechaInicio,
    const std::string& fechaFin,
    const std::string& rutaPdf
) {
    auto construirBloque = [](const std::vector<LineaEstadoFinanciero>& lineas) {
        QString s;
        for (const auto& l : lineas) {
            s += QString(R"(
                        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%1</td>
                        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%2</td>
                        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%3</td>
            )")
                .arg(QString::fromStdString(l.codigo), QString::fromStdString(l.nombre), fmtCop(l.saldo));
        }

        return s;
    };

    QString html = "";
    html += construirEncabezadoCopropiedadHTML(
        "ESTADO DE RESULTADOS INTEGRAL",
        QString("Periodo del %1 al %2 (Cifras en Pesos Colombianos - COP)")
            .arg(QString::fromStdString(fechaInicio), QString::fromStdString(fechaFin))
    );
    html += QString(R"(
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%1</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%3</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">INGRESOS OPERACIONALES Y NO OPERACIONALES</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">TOTAL INGRESOS DEL PERIODO</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%2</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">GASTOS DE ADMINISTRACIÓN Y OPERACIÓN</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">TOTAL GASTOS DEL PERIODO</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%4</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">EXCEDENTE / (DÉFICIT) DEL EJERCICIO</td>
        <td style="padding: 6px; border-bottom: 1px solid #ddd;">%5</td>
    )")
        .arg(construirBloque(er.ingresos))
        .arg(fmtCop(er.totalIngresos))
        .arg(construirBloque(er.gastos))
        .arg(fmtCop(er.totalGastos))
        .arg(fmtCop(er.excedenteODeficit));

    html += construirFirmasHTML() + "";
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}

void ReportsEngine::exportarBalanceGeneralPDF(
    const BalanceGeneralResumen& bg,
    const std::string& fechaCorte,
    const std::string& rutaPdf
) {
    auto construirBloque = [](const std::vector<LineaEstadoFinanciero>& lineas) {
        QString s;
        for (const auto& l : lineas) {
            s += QString(R"(
            %1
            %2
            %3
            )").arg(QString::fromStdString(l.codigo), QString::fromStdString(l.nombre), fmtCop(l.saldo));
        }

        return s;
    };

    QString html = "";
    html += construirEncabezadoCopropiedadHTML(
        "ESTADO DE SITUACIÓN FINANCIERA (BALANCE GENERAL)",
        "Corte a: " + QString::fromStdString(fechaCorte) + " (NIIF Grupo 3 - OT 15 CTCP)"
    );

    html += QString(R"(
        %1
        %3
        %5
        ACTIVO
        TOTAL ACTIVO
        %2
        PASIVO
        TOTAL PASIVO
        %4
        PATRIMONIO
        3605
        Excedentes / (Déficit) Acumulado del Ejercicio
        %6
        TOTAL PATRIMONIO
        %7
        TOTAL PASIVO + PATRIMONIO
        %8
    )")
        .arg(construirBloque(bg.activos))
        .arg(fmtCop(bg.totalActivos))
        .arg(construirBloque(bg.pasivos))
        .arg(fmtCop(bg.totalPasivos))
        .arg(construirBloque(bg.patrimonio))
        .arg(fmtCop(bg.resultadoEjercicio))
        .arg(fmtCop(bg.totalPatrimonio))
        .arg(fmtCop(bg.totalPasivos + bg.totalPatrimonio));

    html += construirFirmasHTML() + "";
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}
