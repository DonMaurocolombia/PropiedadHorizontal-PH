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


#include "FacturacionWidget.hpp"
#include "core/PdfGenerator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QLocale>
#include <QDate>

FacturacionWidget::FacturacionWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTabla();
}

void FacturacionWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("<h3>Cuentas de Cobro (Ley 675)</h3>", this);
    mainLayout->addWidget(titleLabel);

    // --- PANEL SUPERIOR: LIQUIDACIÓN MASIVA ---
    auto* genGroup = new QGroupBox("Generar Lote Mensual de Cuentas de Cobro", this);
    auto* genLayout = new QHBoxLayout(genGroup);

    m_txtPeriodo = new QLineEdit(this);
    m_txtPeriodo->setText(QDate::currentDate().toString("yyyy-MM"));
    m_txtPeriodo->setFixedWidth(90);

    m_spinPresupuestoMensual = new QDoubleSpinBox(this);
    m_spinPresupuestoMensual->setRange(100000.0, 5000000000.0);
    m_spinPresupuestoMensual->setDecimals(0);
    m_spinPresupuestoMensual->setPrefix("$ ");
    m_spinPresupuestoMensual->setGroupSeparatorShown(true);
    m_spinPresupuestoMensual->setValue(25000000.0);

    m_dateEmision = new QDateEdit(QDate::currentDate(), this);
    m_dateEmision->setCalendarPopup(true);
    m_dateEmision->setDisplayFormat("yyyy-MM-dd");

    m_dateVencimiento = new QDateEdit(QDate::currentDate().addDays(15), this);
    m_dateVencimiento->setCalendarPopup(true);
    m_dateVencimiento->setDisplayFormat("yyyy-MM-dd");

    m_btnGenerarLote = new QPushButton("Liquidar y Contabilizar Mes", this);
    m_btnGenerarLote->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 6px 14px;");
    connect(m_btnGenerarLote, &QPushButton::clicked, this, &FacturacionWidget::onGenerarLoteClicked);

    genLayout->addWidget(new QLabel("Periodo (YYYY-MM):", this));
    genLayout->addWidget(m_txtPeriodo);
    genLayout->addWidget(new QLabel("Presupuesto Mensual Base:", this));
    genLayout->addWidget(m_spinPresupuestoMensual);
    genLayout->addWidget(new QLabel("Emisión:", this));
    genLayout->addWidget(m_dateEmision);
    genLayout->addWidget(new QLabel("Vencimiento:", this));
    genLayout->addWidget(m_dateVencimiento);
    genLayout->addStretch();
    genLayout->addWidget(m_btnGenerarLote);

    mainLayout->addWidget(genGroup);

    // --- TABLA DE CUENTAS GENERADAS ---
    m_tablaCuentas = new QTableWidget(this);
    m_tablaCuentas->setColumnCount(9);
    m_tablaCuentas->setHorizontalHeaderLabels({
        "Documento", "Periodo", "Unidad", "Propietario / Responsable",
        "Saldo Anterior", "Cargos Mes", "Total a Pagar", "Vencimiento", "Estado"
    });
    m_tablaCuentas->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaCuentas->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaCuentas->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaCuentas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    mainLayout->addWidget(m_tablaCuentas);

    // --- BARRA INFERIOR: EXPORTACIÓN PDF ---
    auto* bottomBar = new QHBoxLayout();
    m_btnPdfIndividual = new QPushButton("Exportar Cuenta Seleccionada a PDF", this);
    m_btnPdfMasivo = new QPushButton("Exportar Todo el Periodo a PDFs (Carpeta)", this);

    m_btnPdfIndividual->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 6px 12px;");

    connect(m_btnPdfIndividual, &QPushButton::clicked, this, &FacturacionWidget::onExportarPdfIndividualClicked);
    connect(m_btnPdfMasivo, &QPushButton::clicked, this, &FacturacionWidget::onExportarPdfMasivoClicked);

    bottomBar->addStretch();
    bottomBar->addWidget(m_btnPdfIndividual);
    bottomBar->addWidget(m_btnPdfMasivo);
    mainLayout->addLayout(bottomBar);
}

void FacturacionWidget::recargarTabla() {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    m_cuentasCache = BillingEngine::listarCuentasCobro();
    m_tablaCuentas->setRowCount(static_cast<int>(m_cuentasCache.size()));

    for (int row = 0; row < static_cast<int>(m_cuentasCache.size()); ++row) {
        const auto& c = m_cuentasCache[row];
        m_tablaCuentas->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.numeroDocumento)));
        m_tablaCuentas->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c.periodo)));
        m_tablaCuentas->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c.unidadNomenclatura)));
        m_tablaCuentas->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(c.propietarioNombre)));
        m_tablaCuentas->setItem(row, 4, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(c.saldoAnterior))));
        m_tablaCuentas->setItem(row, 5, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(c.valorPeriodo))));
        m_tablaCuentas->setItem(row, 6, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(c.totalAPagar))));
        m_tablaCuentas->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(c.fechaVencimiento)));
        m_tablaCuentas->setItem(row, 8, new QTableWidgetItem(QString::fromStdString(c.estado)));
    }
}

void FacturacionWidget::onGenerarLoteClicked() {
    std::string periodo = m_txtPeriodo->text().trimmed().toStdString();
    int64_t presupuesto = static_cast<int64_t>(m_spinPresupuestoMensual->value());
    std::string fEmision = m_dateEmision->date().toString("yyyy-MM-dd").toStdString();
    std::string fVenc = m_dateVencimiento->date().toString("yyyy-MM-dd").toStdString();

    auto confirm = QMessageBox::question(
        this, "Confirmar Facturación Masiva",
        QString("¿Desea generar las cuentas de cobro del periodo %1 y causar sus asientos contables?")
            .arg(QString::fromStdString(periodo)),
        QMessageBox::Yes | QMessageBox::No
    );

    if (confirm != QMessageBox::Yes) return;

    try {
        int generadas = BillingEngine::generarFacturacionMensual(
            periodo, presupuesto, fEmision, fVenc, m_usuarioId
        );
        QMessageBox::information(
            this, "Proceso Exitoso",
            QString("Se generaron y contabilizaron %1 cuentas de cobro correctamente.").arg(generadas)
        );

        recargarTabla();
        
        emit facturacionGenerada();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error en Facturación", e.what());
    }
}

void FacturacionWidget::onExportarPdfIndividualClicked() {
    auto selected = m_tablaCuentas->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Selección", "Seleccione primero una cuenta de cobro de la tabla.");
        return;
    }

    int row = selected.first()->row();
    const auto& resumen = m_cuentasCache[row];

    QString nombreSugerido = QString("%1_%2.pdf")
        .arg(QString::fromStdString(resumen.numeroDocumento))
        .arg(QString::fromStdString(resumen.unidadNomenclatura).replace(" ", "_"));

    QString ruta = QFileDialog::getSaveFileName(
        this, "Guardar Cuenta de Cobro PDF", nombreSugerido, "Archivos PDF (*.pdf)"
    );
    if (ruta.isEmpty()) return;

    try {
        CuentaCobroCompleta completa = BillingEngine::obtenerCuentaCompleta(resumen.id);
        PdfGenerator::exportarCuentaCobroPDF(completa, ruta.toStdString());
        QMessageBox::information(this, "PDF Generado", "Cuenta de cobro exportada exitosamente.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error PDF", e.what());
    }
}

void FacturacionWidget::onExportarPdfMasivoClicked() {
    std::string periodo = m_txtPeriodo->text().trimmed().toStdString();
    auto cuentasPeriodo = BillingEngine::listarCuentasCobro(periodo);

    if (cuentasPeriodo.empty()) {
        QMessageBox::warning(this, "Sin Datos", "No hay cuentas de cobro generadas para el periodo " + QString::fromStdString(periodo));
        return;
    }

    QString carpeta = QFileDialog::getExistingDirectory(this, "Seleccionar Carpeta de Destino para los PDFs");
    if (carpeta.isEmpty()) return;

    try {
        for (const auto& c : cuentasPeriodo) {
            CuentaCobroCompleta completa = BillingEngine::obtenerCuentaCompleta(c.id);
            QString rutaArchivo = QString("%1/%2_%3.pdf")
                .arg(carpeta)
                .arg(QString::fromStdString(c.numeroDocumento))
                .arg(QString::fromStdString(c.unidadNomenclatura).replace(" ", "_"));
            PdfGenerator::exportarCuentaCobroPDF(completa, rutaArchivo.toStdString());
        }
        QMessageBox::information(
            this, "Exportación Masiva Completa",
            QString("Se generaron %1 archivos PDF en la carpeta seleccionada.").arg(cuentasPeriodo.size())
        );
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error Exportación Masiva", e.what());
    }
}
