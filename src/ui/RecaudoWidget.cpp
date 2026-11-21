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


#include "RecaudoWidget.hpp"
#include "core/CollectionEngine.hpp"
#include "core/UnidadRepository.hpp"
#include "core/PdfGenerator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QLocale>
#include <QDate>

RecaudoWidget::RecaudoWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarDatos();
}

void RecaudoWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();
    auto* titleLabel = new QLabel("<h3>Recaudo de Cartera, Recibos de Caja y Paz y Salvos</h3>", this);
    m_lblSaldoUnidad = new QLabel("Seleccione una unidad", this);
    m_lblSaldoUnidad->setStyleSheet("padding: 6px 12px; border-radius: 4px; font-weight: bold;");

    topBar->addWidget(titleLabel);
    topBar->addStretch();
    topBar->addWidget(m_lblSaldoUnidad);
    mainLayout->addLayout(topBar);

    // --- SECCIÓN SUPERIOR DIVIDIDA: CARTERA PENDIENTE + FORMULARIO DE PAGO ---
    auto* upperLayout = new QHBoxLayout();

    // Panel Izquierdo: Desglose de deuda por prelación legal
    auto* debtGroup = new QGroupBox("Cartera Pendiente (Orden de Imputación Legal)", this);
    auto* debtLayout = new QVBoxLayout(debtGroup);

    m_cmbUnidad = new QComboBox(this);
    connect(m_cmbUnidad, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecaudoWidget::onUnidadSeleccionada);

    m_tablaDeudaDetalle = new QTableWidget(this);
    m_tablaDeudaDetalle->setColumnCount(4);
    m_tablaDeudaDetalle->setHorizontalHeaderLabels({
        "Cuenta Cobro", "Periodo", "Concepto", "Saldo Pendiente"
    });
    m_tablaDeudaDetalle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaDeudaDetalle->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    debtLayout->addWidget(m_cmbUnidad);
    debtLayout->addWidget(m_tablaDeudaDetalle);

    // Panel Derecho: Registro de Recibo de Caja
    auto* payGroup = new QGroupBox("Registrar Recibo de Caja", this);
    payGroup->setFixedWidth(400);
    auto* formLayout = new QFormLayout(payGroup);

    m_datePago = new QDateEdit(QDate::currentDate(), this);
    m_datePago->setCalendarPopup(true);
    m_datePago->setDisplayFormat("yyyy-MM-dd");

    m_cmbFormaPago = new QComboBox(this);
    m_cmbFormaPago->addItems({"TRANSFERENCIA", "CONSIGNACION", "PSE", "EFECTIVO", "DATAFONO"});

    m_cmbCuentaBanco = new QComboBox(this);
    m_cmbCuentaBanco->addItem("111005 - Cuenta Corriente Principal", "111005");
    m_cmbCuentaBanco->addItem("112005 - Cuenta Ahorros Operaciones", "112005");
    m_cmbCuentaBanco->addItem("110505 - Caja General", "110505");

    m_txtReferencia = new QLineEdit(this);
    m_txtReferencia->setPlaceholderText("N° Comprobante / Aprobación bancaria");

    m_spinValorPagado = new QDoubleSpinBox(this);
    m_spinValorPagado->setRange(1000.0, 500000000.0);
    m_spinValorPagado->setDecimals(0);
    m_spinValorPagado->setPrefix("$ ");
    m_spinValorPagado->setGroupSeparatorShown(true);

    m_txtObservaciones = new QLineEdit(this);
    m_txtObservaciones->setPlaceholderText("Notas adicionales del pago");

    m_btnRegistrarPago = new QPushButton("Registrar Pago y Contabilizar", this);
    m_btnRegistrarPago->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 7px;");
    connect(m_btnRegistrarPago, &QPushButton::clicked, this, &RecaudoWidget::onRegistrarPagoClicked);

    m_btnPazYSalvo = new QPushButton("Emitir Certificado Paz y Salvo (PDF)", this);
    m_btnPazYSalvo->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 7px;");
    m_btnPazYSalvo->setEnabled(false);
    connect(m_btnPazYSalvo, &QPushButton::clicked, this, &RecaudoWidget::onGenerarPazYSalvoClicked);

    formLayout->addRow("Fecha Pago:", m_datePago);
    formLayout->addRow("Forma Pago:", m_cmbFormaPago);
    formLayout->addRow("Cuenta Destino:", m_cmbCuentaBanco);
    formLayout->addRow("Referencia:", m_txtReferencia);
    formLayout->addRow("Valor Recibido *:", m_spinValorPagado);
    formLayout->addRow("Observaciones:", m_txtObservaciones);
    formLayout->addRow(m_btnRegistrarPago);
    formLayout->addRow(m_btnPazYSalvo);

    upperLayout->addWidget(debtGroup, 1);
    upperLayout->addWidget(payGroup);
    mainLayout->addLayout(upperLayout, 1);

    // --- SECCIÓN INFERIOR: HISTORIAL DE RECIBOS DE CAJA ---
    auto* histLabel = new QLabel("<h4>Historial de Recibos de Caja Emitidos</h4>", this);
    mainLayout->addWidget(histLabel);

    m_tablaRecibos = new QTableWidget(this);
    m_tablaRecibos->setColumnCount(7);
    m_tablaRecibos->setHorizontalHeaderLabels({
        "N° Recibo", "Fecha", "Unidad", "Propietario", "Forma Pago", "Referencia", "Valor Pagado"
    });
    m_tablaRecibos->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaRecibos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_tablaRecibos, 1);
}

void RecaudoWidget::recargarDatos() {
    cargarComboUnidades();
    cargarHistorialRecibos();
}

void RecaudoWidget::cargarComboUnidades() {
    int idActual = m_cmbUnidad->currentData().toInt();
    m_cmbUnidad->blockSignals(true);
    m_cmbUnidad->clear();
    m_cmbUnidad->addItem("-- Seleccione Unidad Privada --", 0);

    for (const auto& u : UnidadRepository::listarTodas()) {
        QString texto = QString::fromStdString(
            u.torreBloque + " - " + u.nomenclatura + " (" + u.propietarioNombre + ")"
        );
        m_cmbUnidad->addItem(texto, u.id);
    }

    int idx = m_cmbUnidad->findData(idActual);
    m_cmbUnidad->setCurrentIndex(idx >= 0 ? idx : 0);
    m_cmbUnidad->blockSignals(false);
    onUnidadSeleccionada(m_cmbUnidad->currentIndex());
}

void RecaudoWidget::onUnidadSeleccionada(int) {
    int unidadId = m_cmbUnidad->currentData().toInt();
    m_tablaDeudaDetalle->setRowCount(0);

    if (unidadId <= 0) {
        m_lblSaldoUnidad->setText("Seleccione una unidad");
        m_lblSaldoUnidad->setStyleSheet("padding: 6px 12px;");
        m_btnPazYSalvo->setEnabled(false);
        return;
    }

    QLocale col(QLocale::Spanish, QLocale::Colombia);
    int64_t saldoTotal = CollectionEngine::obtenerSaldoTotalUnidad(unidadId);
    auto detalles = CollectionEngine::consultarDeudaPorPrelacion(unidadId);

    m_tablaDeudaDetalle->setRowCount(static_cast<int>(detalles.size()));
    for (int row = 0; row < static_cast<int>(detalles.size()); ++row) {
        const auto& d = detalles[row];
        m_tablaDeudaDetalle->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(d.numeroCuentaCobro)));
        m_tablaDeudaDetalle->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(d.periodo)));
        m_tablaDeudaDetalle->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(d.descripcion)));
        m_tablaDeudaDetalle->setItem(row, 3, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(d.saldoConcepto))));
    }

    if (saldoTotal == 0) {
        m_lblSaldoUnidad->setText("ESTADO: A PAZ Y SALVO ($ 0)");
        m_lblSaldoUnidad->setStyleSheet(
            "background-color: #E8F5E9; color: #2E7D32; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
        m_btnPazYSalvo->setEnabled(true);
        m_spinValorPagado->setValue(0);
    } else {
        m_lblSaldoUnidad->setText("DEUDA TOTAL: $ " + col.toString(static_cast<qlonglong>(saldoTotal)));
        m_lblSaldoUnidad->setStyleSheet(
            "background-color: #FFEBEE; color: #C62828; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
        m_btnPazYSalvo->setEnabled(false);
        m_spinValorPagado->setValue(static_cast<double>(saldoTotal));
    }
}

void RecaudoWidget::onRegistrarPagoClicked() {
    int unidadId = m_cmbUnidad->currentData().toInt();
    if (unidadId <= 0) {
        QMessageBox::warning(this, "Validación", "Seleccione primero una unidad privada.");
        return;
    }

    try {
        ReciboCajaRequest req;
        req.unidadId = unidadId;
        req.fechaPago = m_datePago->date().toString("yyyy-MM-dd").toStdString();
        req.formaPago = m_cmbFormaPago->currentText().toStdString();
        req.cuentaBancoCodigo = m_cmbCuentaBanco->currentData().toString().toStdString();
        req.referenciaBancaria = m_txtReferencia->text().trimmed().toStdString();
        req.valorPagado = static_cast<int64_t>(m_spinValorPagado->value());
        req.observaciones = m_txtObservaciones->text().trimmed().toStdString();
        req.usuarioId = m_usuarioId;

        std::string numRecibo = CollectionEngine::registrarPago(req);

        QMessageBox::information(
            this, "Pago Registrado",
            QString("Se generó y contabilizó el Recibo de Caja %1 correctamente.")
                .arg(QString::fromStdString(numRecibo))
        );

        m_txtReferencia->clear();
        m_txtObservaciones->clear();
        onUnidadSeleccionada(m_cmbUnidad->currentIndex());
        cargarHistorialRecibos();
        emit pagoRegistrado();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error en Recaudo", e.what());
    }
}

void RecaudoWidget::onGenerarPazYSalvoClicked() {
    int unidadId = m_cmbUnidad->currentData().toInt();
    if (unidadId <= 0) return;

    QString ruta = QFileDialog::getSaveFileName(
        this, "Guardar Certificado Paz y Salvo", "Paz_y_Salvo_PH.pdf", "Archivos PDF (*.pdf)"
    );
    if (ruta.isEmpty()) return;

    try {
        PdfGenerator::exportarPazYSalvoPDF(unidadId, ruta.toStdString());
        QMessageBox::information(this, "Paz y Salvo", "Certificado PDF generado exitosamente.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", e.what());
    }
}

void RecaudoWidget::cargarHistorialRecibos() {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    auto recibos = CollectionEngine::listarRecibos();
    m_tablaRecibos->setRowCount(static_cast<int64_t>(recibos.size()));
    for (int row = 0; row < static_cast<int64_t>(recibos.size()); ++row) {
        const auto& r = recibos[row];
        m_tablaRecibos->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(r.numeroRecibo)));
        m_tablaRecibos->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.fechaPago)));
        m_tablaRecibos->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(r.unidadNomenclatura)));
        m_tablaRecibos->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(r.terceroNombre)));
        m_tablaRecibos->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(r.formaPago)));
        m_tablaRecibos->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(r.referencia)));
        m_tablaRecibos->setItem(row, 6, new QTableWidgetItem("$ " + col.toString(static_cast<int64_t>(r.valorPagado))));
    }
}
