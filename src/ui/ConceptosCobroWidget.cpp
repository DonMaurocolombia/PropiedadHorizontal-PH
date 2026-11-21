// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ConceptosCobroWidget.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <stdexcept>

ConceptosCobroWidget::ConceptosCobroWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<h3>Conceptos de cobro por servicios y cuentas PUC</h3>", this));
    layout->addWidget(new QLabel(
        "Los cambios de cuentas quedan bloqueados cuando el concepto ya tiene facturas o cargos asociados. "
        "Los conceptos de servicios no liquidan IVA en esta etapa.",
        this
    ));

    auto* body = new QHBoxLayout();
    m_tabla = new QTableWidget(this);
    m_tabla->setColumnCount(4);
    m_tabla->setHorizontalHeaderLabels({
        "Código", "Concepto", "Cuenta de ingresos", "Cuenta de cartera"
    });
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabla->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tabla, &QTableWidget::itemSelectionChanged,
            this, &ConceptosCobroWidget::seleccionarFila);

    auto* group = new QGroupBox("Configuración del concepto", this);
    group->setFixedWidth(420);
    auto* form = new QFormLayout(group);
    m_txtCodigo = new QLineEdit(this);
    m_txtCodigo->setPlaceholderText("Ej: ASADOS");
    m_txtNombre = new QLineEdit(this);
    m_cmbCuentaIngreso = new QComboBox(this);
    m_cmbCuentaCartera = new QComboBox(this);
    cargarCuentas();
    form->addRow("Código *:", m_txtCodigo);
    form->addRow("Nombre del cobro *:", m_txtNombre);
    form->addRow("Cuenta de ingresos *:", m_cmbCuentaIngreso);
    form->addRow("Cuenta de cartera *:", m_cmbCuentaCartera);

    auto* buttons = new QHBoxLayout();
    auto* btnNuevo = new QPushButton("Nuevo", this);
    m_btnGuardar = new QPushButton("Guardar", this);
    m_btnEliminar = new QPushButton("Eliminar", this);
    m_btnEliminar->setEnabled(false);
    buttons->addWidget(btnNuevo);
    buttons->addWidget(m_btnGuardar);
    buttons->addWidget(m_btnEliminar);
    form->addRow(buttons);
    connect(btnNuevo, &QPushButton::clicked, this, &ConceptosCobroWidget::nuevoConcepto);
    connect(m_btnGuardar, &QPushButton::clicked, this, &ConceptosCobroWidget::guardarConcepto);
    connect(m_btnEliminar, &QPushButton::clicked, this, &ConceptosCobroWidget::eliminarConcepto);

    body->addWidget(m_tabla, 1);
    body->addWidget(group);
    layout->addLayout(body, 1);
    recargarTabla();
}

void ConceptosCobroWidget::cargarCuentas() {
    m_cmbCuentaIngreso->clear();
    for (const auto& cuenta : ConceptosCobroRepository::listarCuentasIngreso()) {
        m_cmbCuentaIngreso->addItem(
            QString("%1 - %2").arg(QString::fromStdString(cuenta.codigo),
                                   QString::fromStdString(cuenta.nombre)),
            QString::fromStdString(cuenta.codigo)
        );
    }

    m_cmbCuentaCartera->clear();
    for (const auto& cuenta : ConceptosCobroRepository::listarCuentasCartera()) {
        m_cmbCuentaCartera->addItem(
            QString("%1 - %2").arg(QString::fromStdString(cuenta.codigo),
                                   QString::fromStdString(cuenta.nombre)),
            QString::fromStdString(cuenta.codigo)
        );
    }
}

void ConceptosCobroWidget::recargarTabla() {
    m_conceptos = ConceptosCobroRepository::listarServicios();
    m_tabla->setRowCount(static_cast<int>(m_conceptos.size()));
    for (int fila = 0; fila < static_cast<int>(m_conceptos.size()); ++fila) {
        const auto& concepto = m_conceptos[static_cast<std::size_t>(fila)];
        m_tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(concepto.codigo)));
        m_tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(concepto.nombre)));
        m_tabla->setItem(fila, 2, new QTableWidgetItem(QString("%1 - %2")
            .arg(QString::fromStdString(concepto.cuentaIngresoCodigo),
                 QString::fromStdString(concepto.cuentaIngresoNombre))));
        m_tabla->setItem(fila, 3, new QTableWidgetItem(QString("%1 - %2")
            .arg(QString::fromStdString(concepto.cuentaCarteraCodigo),
                 QString::fromStdString(concepto.cuentaCarteraNombre))));
    }
}

void ConceptosCobroWidget::seleccionarFila() {
    const auto selected = m_tabla->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const auto& concepto = m_conceptos[static_cast<std::size_t>(selected.first()->row())];
    m_conceptoSeleccionadoId = concepto.id;
    m_txtCodigo->setText(QString::fromStdString(concepto.codigo));
    m_txtCodigo->setReadOnly(true);
    m_txtNombre->setText(QString::fromStdString(concepto.nombre));
    m_cmbCuentaIngreso->setCurrentIndex(m_cmbCuentaIngreso->findData(
        QString::fromStdString(concepto.cuentaIngresoCodigo)));
    m_cmbCuentaCartera->setCurrentIndex(m_cmbCuentaCartera->findData(
        QString::fromStdString(concepto.cuentaCarteraCodigo)));
    m_btnEliminar->setEnabled(true);
}

void ConceptosCobroWidget::nuevoConcepto() {
    limpiarFormulario();
}

void ConceptosCobroWidget::limpiarFormulario() {
    m_conceptoSeleccionadoId = 0;
    m_tabla->clearSelection();
    m_txtCodigo->setReadOnly(false);
    m_txtCodigo->clear();
    m_txtNombre->clear();
    if (m_cmbCuentaIngreso->count() > 0) m_cmbCuentaIngreso->setCurrentIndex(0);
    if (m_cmbCuentaCartera->count() > 0) m_cmbCuentaCartera->setCurrentIndex(0);
    m_btnEliminar->setEnabled(false);
    m_txtCodigo->setFocus();
}

void ConceptosCobroWidget::guardarConcepto() {
    try {
        const std::string codigo = m_txtCodigo->text().trimmed().toUpper().toStdString();
        const std::string nombre = m_txtNombre->text().trimmed().toStdString();
        const std::string cuentaIngreso = m_cmbCuentaIngreso->currentData().toString().toStdString();
        const std::string cuentaCartera = m_cmbCuentaCartera->currentData().toString().toStdString();
        if (m_conceptoSeleccionadoId == 0) {
            ConceptosCobroRepository::crearServicio(
                codigo, nombre, cuentaIngreso, cuentaCartera, m_usuarioId);
        } else {
            ConceptosCobroRepository::actualizarServicio(
                m_conceptoSeleccionadoId, nombre, cuentaIngreso, cuentaCartera, m_usuarioId);
        }
        limpiarFormulario();
        recargarTabla();
        emit conceptosActualizados();
        QMessageBox::information(this, "Conceptos de cobro", "El concepto se guardó correctamente.");
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "No se pudo guardar", e.what());
    }
}

void ConceptosCobroWidget::eliminarConcepto() {
    if (m_conceptoSeleccionadoId <= 0) {
        return;
    }
    const auto respuesta = QMessageBox::question(
        this,
        "Eliminar concepto",
        "¿Desea eliminar este concepto? Solo se permite si no tiene facturas ni cargos asociados.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (respuesta != QMessageBox::Yes) {
        return;
    }
    try {
        ConceptosCobroRepository::eliminarServicio(m_conceptoSeleccionadoId, m_usuarioId);
        limpiarFormulario();
        recargarTabla();
        emit conceptosActualizados();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "No se pudo eliminar", e.what());
    }
}
