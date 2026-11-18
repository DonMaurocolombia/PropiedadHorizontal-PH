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


#include "PdfGenerator.hpp"
#include "DbManager.hpp"
#include "CollectionEngine.hpp"
#include <QPrinter>
#include <QTextDocument>
#include <QLocale>
#include <QString>
#include <QPageSize>
#include <QDate>
#include <stdexcept>


static QString formatearPesos(int64_t valor) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(valor));
}

void PdfGenerator::exportarCuentaCobroPDF(const CuentaCobroCompleta& cc,
                                          const std::string& rutaArchivoPdf) {
    auto& db = DatabaseManager::instance().getDb();

    QString razonSocial = "CONJUNTO RESIDENCIAL PROPIEDAD HORIZONTAL";
    QString nitCopropiedad = "900.000.000-1";
    QString direccion = "Calle Principal # 10 - 20";
    QString telefono = "300 000 0000";

    SQLite::Statement cfg(db, "SELECT razon_social, nit, direccion, COALESCE(telefono, '') FROM copropiedad_config WHERE id = 1");
    if (cfg.executeStep()) {
        razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
        nitCopropiedad = QString::fromStdString(cfg.getColumn(1).getString());
        direccion = QString::fromStdString(cfg.getColumn(2).getString());
        telefono = QString::fromStdString(cfg.getColumn(3).getString());
    }

    // Construir filas de conceptos
    QString filasHtml;
    for (const auto& det : cc.detalles) {
        filasHtml += QString(R"(
            <tr>
                <td style="padding: 8px; border-bottom: 1px solid #ddd;">%1</td>
                <td style="padding: 8px; border-bottom: 1px solid #ddd; text-align: right;">%2</td>
                <td style="padding: 8px; border-bottom: 1px solid #ddd; text-align: right;">%3</td>
                <td style="padding: 8px; border-bottom: 1px solid #ddd; text-align: right; font-weight: bold;">%4</td>
            </tr>
        )")
        .arg(QString::fromStdString(det.descripcion))
        .arg(formatearPesos(det.valor))
        .arg(formatearPesos(det.iva))
        .arg(formatearPesos(det.valor + det.iva));
    }

    QString html = QString(R"(
        <html>
        <body style="font-family: Arial, Helvetica, sans-serif; color: #333; font-size: 11pt;">
            <table width="100%" style="border-bottom: 2px solid #1976D2; padding-bottom: 10px;">
                <tr>
                    <td>
                        <h2 style="margin: 0; color: #1976D2;">%1</h2>
                        <p style="margin: 2px 0;">NIT: %2 | Régimen Propiedad Horizontal (Ley 675 de 2001)</p>
                        <p style="margin: 2px 0; font-size: 9pt; color: #666;">%3 - Tel: %4</p>
                    </td>
                    <td style="text-align: right; vertical-align: top;">
                        <div style="background-color: #E3F2FD; padding: 10px; border-radius: 5px;">
                            <strong style="color: #0D47A1; font-size: 12pt;">CUENTA DE COBRO</strong><br/>
                            <span style="font-size: 11pt; font-weight: bold;">N° %5</span><br/>
                            <span style="font-size: 9pt;">Periodo: %6</span>
                        </div>
                    </td>
                </tr>
            </table>

            <br/>

            <table width="100%" style="background-color: #F9F9F9; padding: 10px; border: 1px solid #E0E0E0;">
                <tr>
                    <td width="55%">
                        <strong>Propietario / Responsable:</strong> %7<br/>
                        <strong>Documento:</strong> %8<br/>
                        <strong>Inmueble:</strong> %9 - %10 (Coeficiente: %11%)
                    </td>
                    <td width="45%">
                        <strong>Fecha Emisión:</strong> %12<br/>
                        <strong>Fecha Límite Pago:</strong> %13<br/>
                        <strong>Estado:</strong> %14
                    </td>
                </tr>
            </table>

            <br/>

            <table width="100%" cellspacing="0" style="border-collapse: collapse; border: 1px solid #ddd;">
                <thead>
                    <tr style="background-color: #1976D2; color: white;">
                        <th style="padding: 8px; text-align: left;">Concepto</th>
                        <th style="padding: 8px; text-align: right;">Valor Base</th>
                        <th style="padding: 8px; text-align: right;">IVA</th>
                        <th style="padding: 8px; text-align: right;">Subtotal</th>
                    </tr>
                </thead>
                <tbody>
                    %15
                </tbody>
            </table>

            <br/>

            <table width="50%" align="right" cellspacing="0" style="border-collapse: collapse;">
                <tr>
                    <td style="padding: 6px;">Saldo Anterior en Mora:</td>
                    <td style="padding: 6px; text-align: right;">%16</td>
                </tr>
                <tr>
                    <td style="padding: 6px;">Cargos del Periodo:</td>
                    <td style="padding: 6px; text-align: right;">%17</td>
                </tr>
                <tr style="background-color: #E8F5E9; font-size: 12pt; font-weight: bold; color: #1B5E20;">
                    <td style="padding: 10px; border-top: 2px solid #1B5E20;">TOTAL A PAGAR:</td>
                    <td style="padding: 10px; border-top: 2px solid #1B5E20; text-align: right;">%18</td>
                </tr>
            </table>

            <div style="clear: both;"></div>
            <br/><br/>
            <p style="font-size: 8.5pt; color: #666; border-top: 1px solid #ccc; padding-top: 8px;">
                Nota: De conformidad con el Art. 48 de la Ley 675 de 2001, esta cuenta de cobro presta mérito ejecutivo.
                Por favor realice su pago indicando el número de unidad privada.
            </p>
        </body>
        </html>
    )")
    .arg(razonSocial)
    .arg(nitCopropiedad)
    .arg(direccion)
    .arg(telefono)
    .arg(QString::fromStdString(cc.encabezado.numeroDocumento))
    .arg(QString::fromStdString(cc.encabezado.periodo))
    .arg(QString::fromStdString(cc.encabezado.propietarioNombre))
    .arg(QString::fromStdString(cc.terceroDocumento))
    .arg(QString::fromStdString(cc.torreBloque))
    .arg(QString::fromStdString(cc.encabezado.unidadNomenclatura))
    .arg(QString::number(cc.coeficientePct, 'f', 4))
    .arg(QString::fromStdString(cc.encabezado.fechaEmision))
    .arg(QString::fromStdString(cc.encabezado.fechaVencimiento))
    .arg(QString::fromStdString(cc.encabezado.estado))
    .arg(filasHtml)
    .arg(formatearPesos(cc.encabezado.saldoAnterior))
    .arg(formatearPesos(cc.encabezado.valorPeriodo))
    .arg(formatearPesos(cc.encabezado.totalAPagar));

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaArchivoPdf));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}

void PdfGenerator::exportarPazYSalvoPDF(int unidadId, const std::string& rutaArchivoPdf) {
    int64_t saldo = CollectionEngine::obtenerSaldoTotalUnidad(unidadId);
    if (saldo > 0) {
        throw std::runtime_error("No se puede emitir Paz y Salvo: la unidad tiene un saldo pendiente.");
    }

    auto& db = DatabaseManager::instance().getDb();

    QString razonSocial = "COPROPIEDAD PH";
    QString nit = "900.000.000-1";
    QString repLegal = "Administración";

    SQLite::Statement cfg(db, "SELECT razon_social, nit, representante_legal FROM copropiedad_config WHERE id = 1");
    if (cfg.executeStep()) {
        razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
        nit = QString::fromStdString(cfg.getColumn(1).getString());
        repLegal = QString::fromStdString(cfg.getColumn(2).getString());
    }

    SQLite::Statement unit(db, R"(
        SELECT u.torre_bloque, u.nomenclatura, COALESCE(u.matricula_inmobiliaria, 'N/A'),
               t.nombre_razon_social, t.tipo_documento, t.numero_documento
        FROM unidades_privadas u
        INNER JOIN terceros t ON u.propietario_principal_id = t.id
        WHERE u.id = ?
    )");
    unit.bind(1, unidadId);
    if (!unit.executeStep()) {
        throw std::runtime_error("Unidad privada no encontrada.");
    }

    QString html = QString(R"(
        <html>
        <body style="font-family: Arial, Helvetica, sans-serif; color: #222; padding: 30px; line-height: 1.6;">
            <div style="text-align: center; border-bottom: 2px solid #1976D2; padding-bottom: 15px;">
                <h2 style="margin: 0; color: #1976D2;">%1</h2>
                <p style="margin: 4px 0;">NIT: %2 | Régimen de Propiedad Horizontal (Ley 675 de 2001)</p>
            </div>

            <h3 style="text-align: center; margin-top: 35px; letter-spacing: 1px;">
                CERTIFICADO DE PAZ Y SALVO
            </h3>

            <p style="margin-top: 30px; text-align: justify;">
                La Administración de <strong>%1</strong> certifica que el inmueble identificado como
                <strong>%3 - %4</strong> (Matrícula Inmobiliaria: <strong>%5</strong>), propiedad de
                <strong>%6</strong> identificado(a) con <strong>%7 N° %8</strong>, se encuentra
                <strong>A PAZ Y SALVO</strong> por concepto de expensas comunes ordinarias, extraordinarias,
                intereses de mora y demás obligaciones pecuniarias con la copropiedad hasta la fecha de expedición
                del presente documento.
            </p>

            <p style="margin-top: 20px;">
                Se expide en la ciudad correspondiente a solicitud del interesado, el día <strong>%9</strong>,
                en cumplimiento del Artículo 29 de la Ley 675 de 2001.
            </p>

            <div style="margin-top: 70px;">
                <p style="margin: 0;">_________________________________________</p>
                <p style="margin: 4px 0;"><strong>%10</strong></p>
                <p style="margin: 0; font-size: 9.5pt; color: #555;">Administrador(a) y Representante Legal</p>
            </div>
        </body>
        </html>
    )")
        .arg(razonSocial)
        .arg(nit)
        .arg(QString::fromStdString(unit.getColumn(0).getString()))
        .arg(QString::fromStdString(unit.getColumn(1).getString()))
        .arg(QString::fromStdString(unit.getColumn(2).getString()))
        .arg(QString::fromStdString(unit.getColumn(3).getString()))
        .arg(QString::fromStdString(unit.getColumn(4).getString()))
        .arg(QString::fromStdString(unit.getColumn(5).getString()))
        .arg(QDate::currentDate().toString("yyyy-MM-dd"))
        .arg(repLegal);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaArchivoPdf));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}
