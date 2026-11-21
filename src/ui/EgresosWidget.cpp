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


#include "EgresosWidget.hpp"
#include "core/DbManager.hpp"
#include "core/TerceroRepository.hpp"
#include "core/AccountingEngine.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLocale>
#include <QDate>
#include <cmath>

EgresosWidget::EgresosWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarDatos();
}

void EgresosWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("<h3>Egresos, Pago a Proveedores y Retención en la Fuente</h3>", this));

    auto* contentLayout = new QHBoxLayout();

    // --- PANEL IZQUIERDO: FORMULARIO DE REGISTRO ---
    auto* formGroup = new QGroupBox("Nuevo Comprobante de Egreso", this);
    formGroup->setFixedWidth(430);
    auto* form = new QFormLayout(formGroup);

    m_dateEgreso = new QDateEdit(QDate::currentDate(), this);
    m_dateEgreso->setCalendarPopup(true);
    m_dateEgreso->setDisplayFormat("yyyy-MM-dd");

    m_cmbProveedor = new QComboBox(this);
    m_txtNumFactura = new QLineEdit(this);
    m_txtNumFactura->setPlaceholderText("Ej: FV-9821 / Cuenta Cobro #4");

    m_cmbCuentaGasto = new QComboBox(this);
    m_txtConcepto = new QLineEdit(this);
    m_txtConcepto->setPlaceholderText("Ej: Servicio vigilancia mes de noviembre");

    m_spinValorBruto = new QDoubleSpinBox(this);
    m_spinValorBruto->setRange(100.0, 1000000000.0);
    m_spinValorBruto->setDecimals(0);
    m_spinValorBruto->setPrefix("$ ");
    m_spinValorBruto->setGroupSeparatorShown(true);

    m_cmbCuentaRetefuente = new QComboBox(this);
    m_cmbCuentaRetefuente->addItem("-- Sin Retención en la Fuente --", "");
    m_cmbCuentaRetefuente->addItem("236515 - Retefuente Honorarios (10% / 11%)", "236515");
    m_cmbCuentaRetefuente->addItem("236525 - Retefuente Servicios (4% / 6%)", "236525");
    m_cmbCuentaRetefuente->addItem("236540 - Retefuente Compras (2.5% / 3.5%)", "236540");

    auto* reteLayout = new QHBoxLayout();
    m_spinPctRetefuente = new QDoubleSpinBox(this);
    m_spinPctRetefuente->setRange(0.0, 35.0);
    m_spinPctRetefuente->setDecimals(2);
    m_spinPctRetefuente->setSuffix(" %");
    m_spinPctRetefuente->setFixedWidth(90);

    m_spinValorRetefuente = new QDoubleSpinBox(this);
    m_spinValorRetefuente->setRange(0.0, 500000000.0);
    m_spinValorRetefuente->setDecimals(0);
    m_spinValorRetefuente->setPrefix("$ ");
    m_spinValorRetefuente->setGroupSeparatorShown(true);

    reteLayout->addWidget(m_spinPctRetefuente);
    reteLayout->addWidget(m_spinValorRetefuente, 1);

    m_cmbCuentaBanco = new QComboBox(this);
    m_cmbCuentaBanco->addItem("111005 - Cuenta Corriente Principal", "111005");
    m_cmbCuentaBanco->addItem("112005 - Cuenta Ahorros Operaciones", "112005");
    m_cmbCuentaBanco->addItem("110510 - Caja Menor", "110510");

    m_lblValorNetoGirar = new QLabel("$ 0", this);
    m_lblValorNetoGirar->setStyleSheet(
        "background-color: #E8F5E9; color: #1B5E20; font-size: 12pt; font-weight: bold; padding: 6px; border-radius: 4px;"
    );

    auto* btnGuardar = new QPushButton("Registrar Egreso y Contabilizar", this);
    btnGuardar->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 8px;");

    connect(m_spinValorBruto, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EgresosWidget::recalcularNeto);
    connect(m_spinPctRetefuente, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EgresosWidget::recalcularNeto);
    connect(m_spinValorRetefuente, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        QLocale col(QLocale::Spanish, QLocale::Colombia);
        int64_t neto = static_cast<int64_t>(m_spinValorBruto->value() - m_spinValorRetefuente->value());
        m_lblValorNetoGirar->setText("$ " + col.toString(static_cast<qlonglong>(neto)));
    });
    connect(btnGuardar, &QPushButton::clicked, this, &EgresosWidget::onRegistrarEgresoClicked);

    form->addRow("Fecha Egreso:", m_dateEgreso);
    form->addRow("Proveedor *:", m_cmbProveedor);
    form->addRow("N° Factura / Soporte:", m_txtNumFactura);
    form->addRow("Rubro / Cuenta Gasto *:", m_cmbCuentaGasto);
    form->addRow("Concepto del Pago *:", m_txtConcepto);
    form->addRow("Valor Bruto *:", m_spinValorBruto);
    form->addRow("Concepto Retefuente:", m_cmbCuentaRetefuente);
    form->addRow("% y Valor Retención:", reteLayout);
    form->addRow("Cuenta Origen Giro:", m_cmbCuentaBanco);
    form->addRow("NETO A GIRAR:", m_lblValorNetoGirar);
    form->addRow(btnGuardar);

    // --- PANEL DERECHO: HISTORIAL DE EGRESOS ---
    auto* rightLayout = new QVBoxLayout();

    m_tablaEgresos = new QTableWidget(this);
    m_tablaEgresos->setColumnCount(8);
    m_tablaEgresos->setHorizontalHeaderLabels({
        "N° Egreso", "Fecha", "Proveedor", "Cuenta Gasto", "Valor Bruto", "Retefuente", "Neto Girado", "Estado"
    });
    m_tablaEgresos->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaEgresos->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaEgresos->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaEgresos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto* actionBar = new QHBoxLayout();
    m_btnExportarPdf = new QPushButton("Imprimir Comprobante Egreso (PDF)", this);
    m_btnExportarPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 6px 12px;");
    m_btnAnular = new QPushButton("Anular Comprobante (Nota Inversa)", this);

    connect(m_btnExportarPdf, &QPushButton::clicked, this, &EgresosWidget::onExportarPdfClicked);
    connect(m_btnAnular, &QPushButton::clicked, this, &EgresosWidget::onAnularEgresoClicked);

    actionBar->addStretch();
    actionBar->addWidget(m_btnExportarPdf);
    actionBar->addWidget(m_btnAnular);

    rightLayout->addWidget(m_tablaEgresos);
    rightLayout->addLayout(actionBar);

    contentLayout->addWidget(formGroup);
    contentLayout->addLayout(rightLayout, 1);
    mainLayout->addLayout(contentLayout);
}

void EgresosWidget::recargarDatos() {
    cargarCombos();
    cargarTablaEgresos();
}

void EgresosWidget::cargarCombos() {
    m_cmbProveedor->clear();
    m_cmbProveedor->addItem("-- Seleccione Proveedor --", 0);
    for (const auto& p : TerceroRepository::listar("", "PROVEEDOR")) {
        m_cmbProveedor->addItem(
            QString::fromStdString(p.nombreRazonSocial + " (" + p.numeroDocumento + ")"), p.id
        );
    }

    m_cmbCuentaGasto->clear();
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement q(db, R"(
        SELECT codigo, nombre FROM puc_cuentas
        WHERE clase = 'GASTOS' AND permite_movimiento = 1
        ORDER BY codigo ASC
    )");
    while (q.executeStep()) {
        QString codigo = QString::fromStdString(q.getColumn(0).getString());
        QString nombre = QString::fromStdString(q.getColumn(1).getString());
        m_cmbCuentaGasto->addItem(codigo + " - " + nombre, codigo);
    }
}

void EgresosWidget::recalcularNeto() {
    double bruto = m_spinValorBruto->value();
    double pct = m_spinPctRetefuente->value();
    int64_t retencion = std::llround(bruto * (pct / 100.0));

    m_spinValorRetefuente->blockSignals(true);
    m_spinValorRetefuente->setValue(static_cast<double>(retencion));
    m_spinValorRetefuente->blockSignals(false);

    QLocale col(QLocale::Spanish, QLocale::Colombia);
    int64_t neto = static_cast<int64_t>(bruto) - retencion;
    m_lblValorNetoGirar->setText("$ " + col.toString(static_cast<qlonglong>(neto)));
}

void EgresosWidget::onRegistrarEgresoClicked() {
    try {
        ComprobanteEgresoRequest req;
        req.fecha = m_dateEgreso->date().toString("yyyy-MM-dd").toStdString();
        req.proveedorId = m_cmbProveedor->currentData().toInt();
        req.numeroFacturaSoporte = m_txtNumFactura->text().trimmed().toStdString();
        req.cuentaGastoCodigo = m_cmbCuentaGasto->currentData().toString().toStdString();
        req.conceptoDescripcion = m_txtConcepto->text().trimmed().toStdString();
        req.valorBruto = static_cast<int64_t>(m_spinValorBruto->value());
        req.cuentaRetefuenteCodigo = m_cmbCuentaRetefuente->currentData().toString().toStdString();
        req.valorRetefuente = static_cast<int64_t>(m_spinValorRetefuente->value());
        req.cuentaBancoCodigo = m_cmbCuentaBanco->currentData().toString().toStdString();
        req.usuarioId = m_usuarioId;

        ExpenseEngine::registrarEgreso(req);

        QMessageBox::information(this, "Éxito", "Comprobante de egreso registrado y contabilizado.");
        m_txtNumFactura->clear();
        m_txtConcepto->clear();
        m_spinValorBruto->setValue(0);
        m_spinPctRetefuente->setValue(0);
        cargarTablaEgresos();
        emit egresoRegistrado();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void EgresosWidget::cargarTablaEgresos() {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    m_egresosCache = ExpenseEngine::listarEgresos();
    m_tablaEgresos->setRowCount(static_cast<int>(m_egresosCache.size()));

    for (int r = 0; r < static_cast<int>(m_egresosCache.size()); ++r) {
        const auto& e = m_egresosCache[r];
        QString numDoc = QString("CE-%1").arg(e.consecutivo, 5, 10, QChar('0'));

        m_tablaEgresos->setItem(r, 0, new QTableWidgetItem(numDoc));
        m_tablaEgresos->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(e.fecha)));
        m_tablaEgresos->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(e.proveedorNombre)));
        m_tablaEgresos->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(e.cuentaGastoNombre)));
        m_tablaEgresos->setItem(r, 4, new QTableWidgetItem("$ " + col.toString(static_cast<int64_t>(e.valorBruto))));
        m_tablaEgresos->setItem(r, 5, new QTableWidgetItem("$ " + col.toString(static_cast<int64_t>(e.valorRetencion))));
        m_tablaEgresos->setItem(r, 6, new QTableWidgetItem("$ " + col.toString(static_cast<int64_t>(e.valorNetoPagado))));
        m_tablaEgresos->setItem(r, 7, new QTableWidgetItem(QString::fromStdString(e.estado)));
    }
}

void EgresosWidget::onExportarPdfClicked() {
    auto sel = m_tablaEgresos->selectedItems();
    if (sel.isEmpty()) {
        QMessageBox::information(this, "Selección", "Seleccione un comprobante de egreso de la tabla.");
        return;
    }

    const auto& e = m_egresosCache[sel.first()->row()];
    QString nombreSugerido = QString("CE-%1.pdf").arg(e.consecutivo, 5, 10, QChar('0'));
    QString ruta = QFileDialog::getSaveFileName(this, "Guardar Comprobante de Egreso", nombreSugerido, "PDF (*.pdf)");
    if (ruta.isEmpty()) return;

    try {
        ExpenseEngine::exportarComprobanteEgresoPDF(e.comprobanteId, ruta.toStdString());
        QMessageBox::information(this, "PDF Generado", "Comprobante de egreso exportado correctamente.");
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error PDF", ex.what());
    }
}

void EgresosWidget::onAnularEgresoClicked() {
    auto sel = m_tablaEgresos->selectedItems();
    if (sel.isEmpty()) {
        QMessageBox::information(this, "Selección", "Seleccione el comprobante que desea anular.");
        return;
    }

    const auto& e = m_egresosCache[sel.first()->row()];
    bool ok = false;
    QString motivo = QInputDialog::getText(
        this, "Anular Comprobante de Egreso",
        "Ingrese el motivo de anulación (se creará una Nota Contable inversa):",
        QLineEdit::Normal, "", &ok
    );

    if (!ok || motivo.trimmed().isEmpty()) return;

    try {
        AccountingEngine::anularComprobante(e.comprobanteId, m_usuarioId, motivo.trimmed().toStdString());
        QMessageBox::information(this, "Anulado", "Comprobante anulado mediante nota contable inversa.");
        cargarTablaEgresos();
        emit egresoRegistrado();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Error", ex.what());
    }
}
