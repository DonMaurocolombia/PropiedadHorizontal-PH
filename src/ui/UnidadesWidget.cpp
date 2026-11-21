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


#include "UnidadesWidget.hpp"
#include "core/UnidadRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <cmath>

UnidadesWidget::UnidadesWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarDatos();
}

void UnidadesWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- BARRA SUPERIOR: INDICADOR DE COEFICIENTES (LEY 675) ---
    auto* topBar = new QHBoxLayout();
    auto* titleLabel = new QLabel("<h3>Gestión de Unidades Privadas (Inmuebles)</h3>", this);
    m_lblTotalCoeficiente = new QLabel(this);
    m_lblTotalCoeficiente->setStyleSheet("padding: 6px 12px; border-radius: 4px; font-weight: bold;");

    topBar->addWidget(titleLabel);
    topBar->addStretch();
    topBar->addWidget(m_lblTotalCoeficiente);
    mainLayout->addLayout(topBar);

    // --- CONTENIDO DIVIDIDO: TABLA (IZQUIERDA) + FORMULARIO (DERECHA) ---
    auto* contentLayout = new QHBoxLayout();

    // Panel Izquierdo: Buscador + Tabla
    auto* leftLayout = new QVBoxLayout();
    m_txtBuscar = new QLineEdit(this);
    m_txtBuscar->setPlaceholderText("Buscar por torre, apartamento o propietario...");
    connect(m_txtBuscar, &QLineEdit::textChanged, this, &UnidadesWidget::onBuscarTextoCambiado);

    m_tablaUnidades = new QTableWidget(this);
    m_tablaUnidades->setColumnCount(7);
    m_tablaUnidades->setHorizontalHeaderLabels({
        "Torre/Bloque", "Unidad", "Tipo", "Área (m²)", "Coeficiente (%)", "Propietario", "Estado"
    });

    m_tablaUnidades->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaUnidades->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaUnidades->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaUnidades->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tablaUnidades, &QTableWidget::itemSelectionChanged, this, &UnidadesWidget::onFilaSeleccionada);

    leftLayout->addWidget(m_txtBuscar);
    leftLayout->addWidget(m_tablaUnidades);

    // Panel Derecho: Formulario de Registro / Edición
    auto* formGroup = new QGroupBox("Datos de la Unidad Privada", this);
    formGroup->setFixedWidth(380);
    auto* formLayout = new QFormLayout(formGroup);

    m_txtTorre = new QLineEdit(this);
    m_txtTorre->setPlaceholderText("Ej: Torre 1 / Interior A");

    m_txtNomenclatura = new QLineEdit(this);
    m_txtNomenclatura->setPlaceholderText("Ej: Apto 302");

    m_cmbTipo = new QComboBox(this);
    m_cmbTipo->addItems({"APARTAMENTO", "CASA", "LOCAL", "OFICINA"});

    m_txtMatricula = new QLineEdit(this);
    m_txtMatricula->setPlaceholderText("Ej: 50C-1234567");

    m_spinArea = new QDoubleSpinBox(this);
    m_spinArea->setRange(1.0, 10000.0);
    m_spinArea->setDecimals(2);
    m_spinArea->setSuffix(" m²");

    m_spinCoeficientePct = new QDoubleSpinBox(this);
    m_spinCoeficientePct->setRange(0.0001, 100.0);
    m_spinCoeficientePct->setDecimals(4);
    m_spinCoeficientePct->setSingleStep(0.01);
    m_spinCoeficientePct->setSuffix(" %");

    m_cmbPropietario = new QComboBox(this);
    m_cmbInquilino = new QComboBox(this);

    m_cmbEnviarCobro = new QComboBox(this);
    m_cmbEnviarCobro->addItems({"PROPIETARIO", "INQUILINO", "AMBOS"});

    m_cmbEstado = new QComboBox(this);
    m_cmbEstado->addItems({"ACTIVA", "EN_CONSTRUCCION", "INACTIVA"});

    formLayout->addRow("Torre / Bloque:", m_txtTorre);
    formLayout->addRow("Nomenclatura *:", m_txtNomenclatura);
    formLayout->addRow("Tipo Inmueble:", m_cmbTipo);
    formLayout->addRow("Matrícula Inmob.:", m_txtMatricula);
    formLayout->addRow("Área Privada:", m_spinArea);
    formLayout->addRow("Coeficiente *:", m_spinCoeficientePct);
    formLayout->addRow("Propietario *:", m_cmbPropietario);
    formLayout->addRow("Inquilino Actual:", m_cmbInquilino);
    formLayout->addRow("Destinatario Cobro:", m_cmbEnviarCobro);
    formLayout->addRow("Estado:", m_cmbEstado);

    // Botones de acción
    auto* btnLayout = new QHBoxLayout();
    m_btnNuevo = new QPushButton("Nuevo", this);
    m_btnGuardar = new QPushButton("Guardar", this);
    m_btnEliminar = new QPushButton("Eliminar", this);
    m_btnEliminar->setEnabled(false);

    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(m_btnNuevo, &QPushButton::clicked, this, &UnidadesWidget::onNuevoClicked);
    connect(m_btnGuardar, &QPushButton::clicked, this, &UnidadesWidget::onGuardarClicked);
    connect(m_btnEliminar, &QPushButton::clicked, this, &UnidadesWidget::onEliminarClicked);

    btnLayout->addWidget(m_btnNuevo);
    btnLayout->addWidget(m_btnGuardar);
    btnLayout->addWidget(m_btnEliminar);
    formLayout->addRow(btnLayout);

    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addWidget(formGroup);
    mainLayout->addLayout(contentLayout);
}

void UnidadesWidget::recargarDatos() {
    cargarCombosTerceros();
    onBuscarTextoCambiado(m_txtBuscar->text());
    actualizarIndicadorCoeficientes();
}

void UnidadesWidget::cargarCombosTerceros() {
    m_cmbPropietario->clear();
    m_cmbPropietario->addItem("-- Seleccione Propietario --", 0);
    for (const auto& p : UnidadRepository::listarPropietarios()) {
        m_cmbPropietario->addItem(
            QString::fromStdString(p.nombre + " (" + p.documento + ")"), p.id
        );
    }

    m_cmbInquilino->clear();
    m_cmbInquilino->addItem("-- Sin Inquilino (Habitada por dueño) --", 0);
    for (const auto& i : UnidadRepository::listarInquilinos()) {
        m_cmbInquilino->addItem(
            QString::fromStdString(i.nombre + " (" + i.documento + ")"), i.id
        );
    }
}

void UnidadesWidget::actualizarIndicadorCoeficientes() {
    double sumaFraccion = UnidadRepository::obtenerSumaCoeficientesActivos();
    double sumaPct = sumaFraccion * 100.0;

    m_lblTotalCoeficiente->setText(
        QString("Suma Coeficientes Activos: %1% / 100.0000%").arg(sumaPct, 0, 'f', 4)
    );

    if (std::abs(sumaPct - 100.0) < 0.0001) {
        m_lblTotalCoeficiente->setStyleSheet(
            "background-color: #E8F5E9; color: #2E7D32; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    } else {
        m_lblTotalCoeficiente->setStyleSheet(
            "background-color: #FFF3E0; color: #EF6C00; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    }
}

void UnidadesWidget::onBuscarTextoCambiado(const QString& texto) {
    m_unidadesCache = UnidadRepository::listarTodas(texto.toStdString());
    m_tablaUnidades->setRowCount(static_cast<int>(m_unidadesCache.size()));

    for (int row = 0; row < static_cast<int>(m_unidadesCache.size()); ++row) {
        const auto& u = m_unidadesCache[row];
        m_tablaUnidades->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(u.torreBloque)));
        m_tablaUnidades->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(u.nomenclatura)));
        m_tablaUnidades->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(u.tipo)));
        m_tablaUnidades->setItem(row, 3, new QTableWidgetItem(QString::number(u.areaPrivadaM2, 'f', 2)));
        m_tablaUnidades->setItem(row, 4, new QTableWidgetItem(QString::number(u.coeficiente * 100.0, 'f', 4) + "%"));
        m_tablaUnidades->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(u.propietarioNombre)));
        m_tablaUnidades->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(u.estado)));
    }
}

void UnidadesWidget::onFilaSeleccionada() {
    auto selected = m_tablaUnidades->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    const auto& u = m_unidadesCache[row];

    m_unidadSeleccionadaId = u.id;
    m_txtTorre->setText(QString::fromStdString(u.torreBloque));
    m_txtNomenclatura->setText(QString::fromStdString(u.nomenclatura));
    m_cmbTipo->setCurrentText(QString::fromStdString(u.tipo));
    m_txtMatricula->setText(QString::fromStdString(u.matriculaInmobiliaria));
    m_spinArea->setValue(u.areaPrivadaM2);
    m_spinCoeficientePct->setValue(u.coeficiente * 100.0);

    int idxProp = m_cmbPropietario->findData(u.propietarioPrincipalId);
    m_cmbPropietario->setCurrentIndex(idxProp >= 0 ? idxProp : 0);

    int idxInq = u.inquilinoActualId.has_value() ? m_cmbInquilino->findData(u.inquilinoActualId.value()) : 0;
    m_cmbInquilino->setCurrentIndex(idxInq >= 0 ? idxInq : 0);

    m_cmbEnviarCobro->setCurrentText(QString::fromStdString(u.enviarCobroA));
    m_cmbEstado->setCurrentText(QString::fromStdString(u.estado));
    m_btnEliminar->setEnabled(true);
}

void UnidadesWidget::onNuevoClicked() {
    limpiarFormulario();
}

void UnidadesWidget::limpiarFormulario() {
    m_unidadSeleccionadaId = 0;
    m_tablaUnidades->clearSelection();
    m_txtTorre->clear();
    m_txtNomenclatura->clear();
    m_cmbTipo->setCurrentIndex(0);
    m_txtMatricula->clear();
    m_spinArea->setValue(60.0);
    m_spinCoeficientePct->setValue(0.0);
    m_cmbPropietario->setCurrentIndex(0);
    m_cmbInquilino->setCurrentIndex(0);
    m_cmbEnviarCobro->setCurrentIndex(0);
    m_cmbEstado->setCurrentIndex(0);
    m_btnEliminar->setEnabled(false);
    m_txtTorre->setFocus();
}

void UnidadesWidget::onGuardarClicked() {
    try {
        UnidadPrivada u;
        u.id = m_unidadSeleccionadaId;
        u.torreBloque = m_txtTorre->text().trimmed().toStdString();
        u.nomenclatura = m_txtNomenclatura->text().trimmed().toStdString();
        u.tipo = m_cmbTipo->currentText().toStdString();
        u.matriculaInmobiliaria = m_txtMatricula->text().trimmed().toStdString();
        u.areaPrivadaM2 = m_spinArea->value();
        u.coeficiente = m_spinCoeficientePct->value() / 100.0; // Convertir % a fracción
        u.propietarioPrincipalId = m_cmbPropietario->currentData().toInt();

        int inqId = m_cmbInquilino->currentData().toInt();
        if (inqId > 0) u.inquilinoActualId = inqId;

        u.enviarCobroA = m_cmbEnviarCobro->currentText().toStdString();
        u.estado = m_cmbEstado->currentText().toStdString();
        if (m_unidadSeleccionadaId == 0) {
            UnidadRepository::crear(u, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Unidad privada creada correctamente.");
        } else {
            UnidadRepository::actualizar(u, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Unidad privada actualizada correctamente.");
        }

        limpiarFormulario();
        recargarDatos();
        emit unidadesActualizadas();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void UnidadesWidget::onEliminarClicked() {
    if (m_unidadSeleccionadaId == 0) return;
    auto reply = QMessageBox::question(
        this, "Confirmar Eliminación",
        "¿Está seguro de eliminar esta unidad privada?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        try {
            UnidadRepository::eliminar(m_unidadSeleccionadaId, m_usuarioId);
            limpiarFormulario();
            recargarDatos();
            emit unidadesActualizadas();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }
}
