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


#include "PresupuestoWidget.hpp"
#include "core/DbManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QLocale>
#include <QDate>
#include <QColor>
#include <cmath>

static QString formatoCop(int64_t val) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(val));
}

PresupuestoWidget::PresupuestoWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTodo();
}

void PresupuestoWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- BARRA SUPERIOR: SELECTOR VIGENCIA + ALERTA FONDO DE IMPREVISTOS ---
    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("<h3>Presupuesto Anual y Ejecución Presupuestal (Ley 675)</h3>", this));
    topBar->addStretch();

    m_lblAlertaFondoImprevistos = new QLabel(this);
    m_lblAlertaFondoImprevistos->setStyleSheet("padding: 6px 12px; border-radius: 4px; font-weight: bold;");
    topBar->addWidget(m_lblAlertaFondoImprevistos);
    mainLayout->addLayout(topBar);

    // --- BARRA DE FILTRO DE EJECUCIÓN ---
    auto* filterBar = new QHBoxLayout();
    m_cmbPresupuestoAnio = new QComboBox(this);
    m_cmbPresupuestoAnio->setMinimumWidth(260);

    m_cmbMesCorte = new QComboBox(this);
    QStringList meses = {
        "1 - Enero", "2 - Febrero", "3 - Marzo", "4 - Abril", "5 - Mayo", "6 - Junio",
        "7 - Julio", "8 - Agosto", "9 - Septiembre", "10 - Octubre", "11 - Noviembre", "12 - Diciembre"
    };
    for (int i = 0; i < meses.size(); ++i) {
        m_cmbMesCorte->addItem(meses[i], i + 1);
    }
    m_cmbMesCorte->setCurrentIndex(QDate::currentDate().month() - 1);

    auto* btnPdf = new QPushButton("Exportar Ejecución a PDF", this);
    btnPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 6px 12px;");

    connect(m_cmbPresupuestoAnio, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresupuestoWidget::onSeleccionPresupuestoCambiada);
    connect(m_cmbMesCorte, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresupuestoWidget::onSeleccionPresupuestoCambiada);
    connect(btnPdf, &QPushButton::clicked, this, &PresupuestoWidget::onExportarPdfClicked);

    filterBar->addWidget(new QLabel("Vigencia Presupuestal:", this));
    filterBar->addWidget(m_cmbPresupuestoAnio);
    filterBar->addWidget(new QLabel("Corte Acumulado a:", this));
    filterBar->addWidget(m_cmbMesCorte);
    filterBar->addStretch();
    filterBar->addWidget(btnPdf);
    mainLayout->addLayout(filterBar);

    // --- CONTENIDO DIVIDIDO: FORMULARIOS (IZQUIERDA) + TABLA EJECUCIÓN (DERECHA) ---
    auto* contentLayout = new QHBoxLayout();

    auto* leftPanel = new QVBoxLayout();
    leftPanel->setContentsMargins(0, 0, 0, 0);

    // Grupo A: Crear nueva vigencia anual
    auto* newYearGroup = new QGroupBox("1. Apertura de Presupuesto Anual", this);
    newYearGroup->setFixedWidth(380);
    auto* yearForm = new QFormLayout(newYearGroup);

    m_spinNuevoAnio = new QSpinBox(this);
    m_spinNuevoAnio->setRange(2020, 2050);
    m_spinNuevoAnio->setValue(QDate::currentDate().year());

    m_dateAprobacion = new QDateEdit(QDate::currentDate(), this);
    m_dateAprobacion->setCalendarPopup(true);
    m_dateAprobacion->setDisplayFormat("yyyy-MM-dd");

    m_txtActaAsamblea = new QLineEdit(this);
    m_txtActaAsamblea->setPlaceholderText("Ej: Acta Asamblea Ordinaria #012");

    m_spinPctImprevistos = new QDoubleSpinBox(this);
    m_spinPctImprevistos->setRange(1.0, 20.0);
    m_spinPctImprevistos->setDecimals(2);
    m_spinPctImprevistos->setValue(1.0);
    m_spinPctImprevistos->setSuffix(" %");

    auto* btnCrearAnio = new QPushButton("Crear Vigencia Anual", this);
    btnCrearAnio->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold;");
    connect(btnCrearAnio, &QPushButton::clicked, this, &PresupuestoWidget::onCrearNuevoAnioClicked);

    yearForm->addRow("Año Fiscal:", m_spinNuevoAnio);
    yearForm->addRow("Fecha Aprobación:", m_dateAprobacion);
    yearForm->addRow("N° Acta Asamblea:", m_txtActaAsamblea);
    yearForm->addRow("% Fondo Imprevistos:", m_spinPctImprevistos);
    yearForm->addRow(btnCrearAnio);

    // Grupo B: Agregar o editar rubro presupuestal
    auto* rubroGroup = new QGroupBox("2. Asignar Rubro al Presupuesto Vigente", this);
    rubroGroup->setFixedWidth(380);
    auto* rubroForm = new QFormLayout(rubroGroup);

    m_cmbTipoRubro = new QComboBox(this);
    m_cmbTipoRubro->addItems({"GASTO", "FONDO_IMPREVISTOS", "INGRESO"});

    m_cmbCuentaPuc = new QComboBox(this);
    m_txtNombreRubro = new QLineEdit(this);
    m_txtNombreRubro->setPlaceholderText("Nombre descriptivo del rubro");

    m_spinValorAnual = new QDoubleSpinBox(this);
    m_spinValorAnual->setRange(1000.0, 10000000000.0);
    m_spinValorAnual->setDecimals(0);
    m_spinValorAnual->setPrefix("$ ");
    m_spinValorAnual->setGroupSeparatorShown(true);

    auto* rubroBtns = new QHBoxLayout();
    auto* btnGuardarRubro = new QPushButton("Guardar Rubro", this);
    auto* btnEliminarRubro = new QPushButton("Quitar Rubro", this);
    btnGuardarRubro->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(btnGuardarRubro, &QPushButton::clicked, this, &PresupuestoWidget::onAgregarRubroClicked);
    connect(btnEliminarRubro, &QPushButton::clicked, this, &PresupuestoWidget::onEliminarRubroClicked);
    connect(m_cmbCuentaPuc, &QComboBox::currentTextChanged, this, [this](const QString& txt) {
        int dashIdx = txt.indexOf(" - ");
        if (dashIdx > 0) m_txtNombreRubro->setText(txt.mid(dashIdx + 3));
    });

    rubroBtns->addWidget(btnGuardarRubro);
    rubroBtns->addWidget(btnEliminarRubro);

    rubroForm->addRow("Tipo Rubro:", m_cmbTipoRubro);
    rubroForm->addRow("Cuenta PUC:", m_cmbCuentaPuc);
    rubroForm->addRow("Nombre Rubro:", m_txtNombreRubro);
    rubroForm->addRow("Valor Anual Aprobado:", m_spinValorAnual);
    rubroForm->addRow(rubroBtns);

    leftPanel->addWidget(newYearGroup);
    leftPanel->addWidget(rubroGroup);
    leftPanel->addStretch();

    // Panel Derecho: Tabla de Ejecución Presupuestal
    auto* rightPanel = new QVBoxLayout();
    m_tablaEjecucion = new QTableWidget(this);
    m_tablaEjecucion->setColumnCount(7);
    m_tablaEjecucion->setHorizontalHeaderLabels({
        "Cuenta", "Rubro", "Tipo", "Aprobado Anual", "Ejecutado Real", "Saldo Disponible", "% Ejecución"
    });
    m_tablaEjecucion->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaEjecucion->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaEjecucion->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_lblResumenGlobal = new QLabel("Presupuesto Gastos: $ 0 | Ejecutado: $ 0 (0.0%)", this);
    m_lblResumenGlobal->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 8px; background: #E3F2FD; color: #0D47A1;");

    rightPanel->addWidget(m_tablaEjecucion);
    rightPanel->addWidget(m_lblResumenGlobal);

    contentLayout->addLayout(leftPanel);
    contentLayout->addLayout(rightPanel, 1);
    mainLayout->addLayout(contentLayout);
}

void PresupuestoWidget::recargarTodo() {
    cargarComboCuentasPUC();
    cargarComboPresupuestos();
}

void PresupuestoWidget::cargarComboCuentasPUC() {
    m_cmbCuentaPuc->clear();
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement q(db, R"(
        SELECT codigo, nombre FROM puc_cuentas
        WHERE clase IN ('GASTOS', 'INGRESOS', 'PATRIMONIO') AND permite_movimiento = 1
        ORDER BY codigo ASC
    )");
    while (q.executeStep()) {
        QString cod = QString::fromStdString(q.getColumn(0).getString());
        QString nom = QString::fromStdString(q.getColumn(1).getString());
        m_cmbCuentaPuc->addItem(cod + " - " + nom, cod);
    }
}

void PresupuestoWidget::cargarComboPresupuestos() {
    int actualId = m_cmbPresupuestoAnio->currentData().toInt();
    m_cmbPresupuestoAnio->blockSignals(true);
    m_cmbPresupuestoAnio->clear();

    for (const auto& p : BudgetEngine::listarPresupuestos()) {
        QString label = QString("Año %1 (%2) - Total: %3")
            .arg(p.anio)
            .arg(QString::fromStdString(p.actaAsambleaNumero))
            .arg(formatoCop(p.valorTotalAnual));
        m_cmbPresupuestoAnio->addItem(label, p.id);
    }

    int idx = m_cmbPresupuestoAnio->findData(actualId);
    if (idx >= 0) m_cmbPresupuestoAnio->setCurrentIndex(idx);
    m_cmbPresupuestoAnio->blockSignals(false);

    refrescarTablaEjecucion();
}

void PresupuestoWidget::onSeleccionPresupuestoCambiada() {
    refrescarTablaEjecucion();
}

void PresupuestoWidget::refrescarTablaEjecucion() {
    int presId = m_cmbPresupuestoAnio->currentData().toInt();
    m_tablaEjecucion->setRowCount(0);

    if (presId <= 0) {
        m_lblAlertaFondoImprevistos->setText("Sin presupuesto seleccionado");
        return;
    }

    int mesCorte = m_cmbMesCorte->currentData().toInt();
    m_resumenActual = BudgetEngine::calcularEjecucion(presId, mesCorte);

    m_tablaEjecucion->setRowCount(static_cast<int>(m_resumenActual.rubros.size()));

    for (int r = 0; r < static_cast<int>(m_resumenActual.rubros.size()); ++r) {
        const auto& item = m_resumenActual.rubros[r];

        auto* itemId = new QTableWidgetItem(QString::fromStdString(item.cuentaCodigo));
        itemId->setData(Qt::UserRole, item.rubroId);

        auto* pctItem = new QTableWidgetItem(QString::number(item.porcentajeEjecucionAnual, 'f', 1) + "%");
        if (item.porcentajeEjecucionAnual > 100.0 && item.tipo != "INGRESO") {
            pctItem->setForeground(QColor("#C62828"));
            pctItem->setBackground(QColor("#FFEBEE"));
        }

        m_tablaEjecucion->setItem(r, 0, itemId);
        m_tablaEjecucion->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(item.nombreRubro)));
        m_tablaEjecucion->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(item.tipo)));
        m_tablaEjecucion->setItem(r, 3, new QTableWidgetItem(formatoCop(item.valorAnualAprobado)));
        m_tablaEjecucion->setItem(r, 4, new QTableWidgetItem(formatoCop(item.ejecutadoRealAcumulado)));
        m_tablaEjecucion->setItem(r, 5, new QTableWidgetItem(formatoCop(item.saldoPorEjecutar)));
        m_tablaEjecucion->setItem(r, 6, pctItem);
    }

    // Validar cumplimiento del 1% mínimo para Fondo de Imprevistos (Art. 35 Ley 675)
    int64_t minimoLegal = std::llround(m_resumenActual.totalGastosAprobados * (m_resumenActual.encabezado.porcentajeFondoImprevistos / 100.0));
    if (m_resumenActual.totalGastosAprobados > 0 && m_resumenActual.valorFondoImprevistosAprobado >= minimoLegal) {
        m_lblAlertaFondoImprevistos->setText(
            QString("Fondo Imprevistos OK: %1").arg(formatoCop(m_resumenActual.valorFondoImprevistosAprobado))
        );

        m_lblAlertaFondoImprevistos->setStyleSheet(
            "background-color: #E8F5E9; color: #2E7D32; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    } else {
        m_lblAlertaFondoImprevistos->setText(
            QString("Alerta Ley 675: Fondo Imprevistos mínimo sugerido: %1").arg(formatoCop(minimoLegal))
        );

        m_lblAlertaFondoImprevistos->setStyleSheet(
            "background-color: #FFF3E0; color: #EF6C00; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    }

    m_lblResumenGlobal->setText(
        QString("Total Gastos Aprobados: %1 | Ejecutado Real: %2 (%3%) | Cuota Mensual Sugerida: %4")
            .arg(formatoCop(m_resumenActual.totalGastosAprobados))
            .arg(formatoCop(m_resumenActual.totalGastosEjecutados))
            .arg(QString::number(m_resumenActual.porcentajeGlobalGastos, 'f', 1))
            .arg(formatoCop(std::llround(m_resumenActual.totalGastosAprobados / 12.0)))
    );
}

void PresupuestoWidget::onCrearNuevoAnioClicked() {
    try {
        PresupuestoEncabezado p;
        p.anio = m_spinNuevoAnio->value();
        p.fechaAprobacionAsamblea = m_dateAprobacion->date().toString("yyyy-MM-dd").toStdString();
        p.actaAsambleaNumero = m_txtActaAsamblea->text().trimmed().toStdString();
        p.porcentajeFondoImprevistos = m_spinPctImprevistos->value();
        BudgetEngine::crearPresupuestoAnual(p, m_usuarioId);
        QMessageBox::information(this, "Éxito", "Vigencia presupuestal creada correctamente.");
        m_txtActaAsamblea->clear();
        cargarComboPresupuestos();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void PresupuestoWidget::onAgregarRubroClicked() {
    int presId = m_cmbPresupuestoAnio->currentData().toInt();
    try {
        BudgetEngine::guardarRubroPresupuesto(
            presId,
            m_cmbCuentaPuc->currentData().toString().toStdString(),
            m_txtNombreRubro->text().trimmed().toStdString(),
            m_cmbTipoRubro->currentText().toStdString(),
            static_cast<int64_t>(m_spinValorAnual->value()),
            m_usuarioId
        );

        m_spinValorAnual->setValue(0);
        cargarComboPresupuestos();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void PresupuestoWidget::onEliminarRubroClicked() {
    auto sel = m_tablaEjecucion->selectedItems();
    if (sel.isEmpty()) return;

    int row = sel.first()->row();
    int rubroId = m_tablaEjecucion->item(row, 0)->data(Qt::UserRole).toInt();
    BudgetEngine::eliminarRubro(rubroId, m_usuarioId);
    cargarComboPresupuestos();
}

void PresupuestoWidget::onExportarPdfClicked() {
    if (m_resumenActual.encabezado.id == 0) return;

    QString ruta = QFileDialog::getSaveFileName(
        this, "Exportar Ejecución Presupuestal",
        QString("Ejecucion_Presupuestal_%1.pdf").arg(m_resumenActual.encabezado.anio),
        "PDF (*.pdf)"
    );

    if (ruta.isEmpty()) return;
    
    BudgetEngine::exportarEjecucionPDF(m_resumenActual, ruta.toStdString());
    QMessageBox::information(this, "PDF Generado", "Informe de ejecución presupuestal exportado.");
}
