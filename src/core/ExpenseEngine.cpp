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


#include "ExpenseEngine.hpp"
#include "DbManager.hpp"
#include "AccountingEngine.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QPrinter>
#include <QTextDocument>
#include <QPageSize>
#include <QLocale>
#include <QString>
#include <stdexcept>
#include <format>

static QString fmtPesos(int64_t val) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(val));
}

int ExpenseEngine::registrarEgreso(const ComprobanteEgresoRequest& req) {
    if (req.proveedorId <= 0) {
        throw std::runtime_error("Debe seleccionar un proveedor o beneficiario.");
    }

    if (req.conceptoDescripcion.empty()) {
        throw std::runtime_error("La descripción o concepto del pago es obligatoria.");
    }

    if (req.valorBruto <= 0) {
        throw std::runtime_error("El valor bruto del gasto debe ser mayor a $0.");
    }

    if (req.valorRetefuente < 0 || req.valorRetefuente >= req.valorBruto) {
        throw std::runtime_error("El valor de retención en la fuente no puede ser negativo ni mayor o igual al valor bruto.");
    }

    if (req.valorRetefuente > 0 && req.cuentaRetefuenteCodigo.empty()) {
        throw std::runtime_error("Seleccione la cuenta PUC de retención en la fuente.");
    }

    int64_t valorNetoBanco = req.valorBruto - req.valorRetefuente;

    std::string descCompleta = req.conceptoDescripcion;
    if (!req.numeroFacturaSoporte.empty()) {
        descCompleta += " (Soporte/Fact: " + req.numeroFacturaSoporte + ")";
    }

    ComprobanteRequest comp;
    comp.tipoComprobante = "EGRESO";
    comp.fecha = req.fecha;
    comp.descripcion = descCompleta;
    comp.usuarioId = req.usuarioId;

    // 1. Débito a la cuenta de Gasto (Clase 5)
    comp.movimientos.push_back({
        req.cuentaGastoCodigo,
        req.proveedorId,
        std::nullopt,
        descCompleta,
        req.valorBruto,
        0
    });

    // 2. Crédito a Retención en la Fuente por Pagar (Pasivo 2365xx) si aplica
    if (req.valorRetefuente > 0) {
        comp.movimientos.push_back({
            req.cuentaRetefuenteCodigo,
            req.proveedorId,
            std::nullopt,
            "Retefuente practicada - " + descCompleta,
            0,
            req.valorRetefuente
        });
    }

    // 3. Crédito al Banco o Caja por el valor neto girado (Activo 11xx)
    comp.movimientos.push_back({
        req.cuentaBancoCodigo,
        req.proveedorId,
        std::nullopt,
        "Giro neto proveedor - " + descCompleta,
        0,
        valorNetoBanco
    });

    return AccountingEngine::registrarComprobante(comp);
}

std::vector<EgresoResumen> ExpenseEngine::listarEgresos() {
    std::vector<EgresoResumen> lista;

    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Statement query(db, R"(
            SELECT
                c.id,
                c.consecutivo,
                c.fecha,
                t.nombre_razon_social,
                t.numero_documento,
                c.descripcion,
                COALESCE((
                    SELECT p.codigo || ' - ' || p.nombre
                    FROM movimientos_contables m
                    INNER JOIN puc_cuentas p ON m.cuenta_codigo = p.codigo
                    WHERE m.comprobante_id = c.id AND m.debito > 0
                    LIMIT 1
                ), 'Gasto General') AS cuenta_gasto,
                COALESCE((
                    SELECT SUM(debito) FROM movimientos_contables WHERE comprobante_id = c.id
                ), 0) AS valor_bruto,
                COALESCE((
                    SELECT SUM(credito) FROM movimientos_contables
                    WHERE comprobante_id = c.id AND cuenta_codigo LIKE '2365%'
                ), 0) AS valor_retencion,
                COALESCE((
                    SELECT SUM(credito) FROM movimientos_contables
                    WHERE comprobante_id = c.id AND cuenta_codigo LIKE '11%'
                ), 0) AS valor_neto,
                c.estado
            FROM comprobantes_contables c
            INNER JOIN movimientos_contables m_ref ON m_ref.comprobante_id = c.id AND m_ref.debito > 0
            INNER JOIN terceros t ON m_ref.tercero_id = t.id
            WHERE c.tipo_comprobante = 'EGRESO'
            GROUP BY c.id
            ORDER BY c.id DESC
        )");

        while (query.executeStep()) {
            lista.push_back({
                query.getColumn(0).getInt(),
                query.getColumn(1).getInt(),
                query.getColumn(2).getString(),
                query.getColumn(3).getString(),
                query.getColumn(4).getString(),
                query.getColumn(5).getString(),
                query.getColumn(6).getString(),
                query.getColumn(7).getInt64(),
                query.getColumn(8).getInt64(),
                query.getColumn(9).getInt64(),
                query.getColumn(10).getString()
            });
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al listar egresos: " << e.what();
    }

    return lista;
}

void ExpenseEngine::exportarComprobanteEgresoPDF(int comprobanteId, const std::string& rutaPdf) {
    QString razonSocial = "CONJUNTO RESIDENCIAL PH";
    QString nitCopropiedad = "900.000.000-1";
    QString repLegal = "Administrador(a)";
    QString contador = "Contador(a) Público";
    QString fecha, descripcion, estado, beneficiarioNombre, beneficiarioDoc, filasAsiento;
    int64_t totalDeb = 0, totalCred = 0, netoGirado = 0, consecutivo = 0;

    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement cfg(db, R"(
            SELECT razon_social, nit, representante_legal,
                COALESCE(contador_nombre, 'Contador(a) Público')
            FROM copropiedad_config WHERE id = 1
        )");
        if (cfg.executeStep()) {
            razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
            nitCopropiedad = QString::fromStdString(cfg.getColumn(1).getString());
            repLegal = QString::fromStdString(cfg.getColumn(2).getString());
            contador = QString::fromStdString(cfg.getColumn(3).getString());
        }

        SQLite::Statement compStmt(db, R"(
            SELECT consecutivo, fecha, descripcion, estado
            FROM comprobantes_contables
            WHERE id = ? AND tipo_comprobante = 'EGRESO'
        )");
        compStmt.bind(1, comprobanteId);
        if (!compStmt.executeStep()) {
            throw std::runtime_error("Comprobante de egreso no encontrado.");
        }

        consecutivo = compStmt.getColumn(0).getInt();
        fecha = QString::fromStdString(compStmt.getColumn(1).getString());
        descripcion = QString::fromStdString(compStmt.getColumn(2).getString());
        estado = QString::fromStdString(compStmt.getColumn(3).getString());

        // Obtener líneas contables del egreso y datos del beneficiario
        SQLite::Statement movStmt(db, R"(
            SELECT m.cuenta_codigo, p.nombre, t.nombre_razon_social, t.tipo_documento, t.numero_documento,
                m.debito, m.credito
            FROM movimientos_contables m
            INNER JOIN puc_cuentas p ON m.cuenta_codigo = p.codigo
            INNER JOIN terceros t ON m.tercero_id = t.id
            WHERE m.comprobante_id = ?
            ORDER BY m.id ASC
        )");
        movStmt.bind(1, comprobanteId);

        while (movStmt.executeStep()) {
            QString cta = QString::fromStdString(movStmt.getColumn(0).getString());
            QString ctaNombre = QString::fromStdString(movStmt.getColumn(1).getString());
            if (beneficiarioNombre.isEmpty()) {
                beneficiarioNombre = QString::fromStdString(movStmt.getColumn(2).getString());
                beneficiarioDoc = QString::fromStdString(movStmt.getColumn(3).getString() + " " + movStmt.getColumn(4).getString());
            }
            int64_t deb = movStmt.getColumn(5).getInt64();
            int64_t cred = movStmt.getColumn(6).getInt64();

            totalDeb += deb;
            totalCred += cred;
            if (cta.startsWith("11") && cred > 0) {
                netoGirado += cred;
            }

            filasAsiento += QString(R"(
                <tr>
                    <td style="padding: 6px; border-bottom: 1px solid #ddd;">%1</td>
                    <td style="padding: 6px; border-bottom: 1px solid #ddd;">%2</td>
                    <td style="padding: 6px; border-bottom: 1px solid #ddd; text-align: right;">%3</td>
                    <td style="padding: 6px; border-bottom: 1px solid #ddd; text-align: right;">%4</td>
                </tr>
            )").arg(cta, ctaNombre, fmtPesos(deb), fmtPesos(cred));
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al exportar el comprobante de egreso en PDF: " << e.what();
    }

    QString html = QString(R"(
        <html>
        <body style="font-family: Arial, sans-serif; font-size: 10pt; color: #222;">
            <table width="100%" style="border-bottom: 2px solid #1976D2; padding-bottom: 10px;">
                <tr>
                    <td>
                        <h2 style="margin: 0; color: #1976D2;">%1</h2>
                        <p style="margin: 2px 0;">NIT: %2 | Régimen Propiedad Horizontal</p>
                    </td>
                    <td style="text-align: right;">
                        <div style="background: #FFEBEE; padding: 10px; border-radius: 5px;">
                            <strong style="color: #C62828; font-size: 11pt;">COMPROBANTE DE EGRESO</strong><br/>
                            <strong style="font-size: 12pt;">N° CE-%3</strong><br/>
                            <span>Fecha: %4 | Estado: %5</span>
                        </div>
                    </td>
                </tr>
            </table>
            <br/>
            <table width="100%" style="background: #F9F9F9; padding: 10px; border: 1px solid #ddd;">
                <tr>
                    <td><strong>Pagado a (Beneficiario):</strong> %6</td>
                    <td><strong>Identificación:</strong> %7</td>
                </tr>
                <tr>
                    <td colspan="2" style="padding-top: 6px;"><strong>Concepto:</strong> %8</td>
                </tr>
                <tr>
                    <td colspan="2" style="padding-top: 6px; font-size: 11pt; color: #1B5E20;">
                        <strong>Valor Neto Girado: %9</strong>
                    </td>
                </tr>
            </table>
            <br/>
            <h4>Imputación Contable (Partida Doble)</h4>
            %10
            Cuenta PUC
            Nombre de la Cuenta
            Débito
            Crédito
            _________________________
            %13
            Aprobó (Administrador)
            _________________________
            %14
            Contabilizó
            _________________________
            Firma y Sello / C.C.
            Recibí Conforme
        </body>
        </html>
    )")
        .arg(razonSocial, nitCopropiedad)
        .arg(QString("%1").arg(consecutivo, 5, 10, QChar('0')))
        .arg(fecha, estado, beneficiarioNombre, beneficiarioDoc, descripcion)
        .arg(fmtPesos(netoGirado))
        .arg(filasAsiento, fmtPesos(totalDeb), fmtPesos(totalCred), repLegal, contador);
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}
