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


#include "TercerosWidget.hpp"
#include "core/TerceroRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

TercerosWidget::TercerosWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTabla();
}

void TercerosWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("<h3>Directorio de Terceros (Propietarios, Inquilinos, Residentes, Visitantes y Proveedores)</h3>", this);
    mainLayout->addWidget(titleLabel);

    auto* contentLayout = new QHBoxLayout();

    // --- PANEL IZQUIERDO: FILTROS + TABLA ---
    auto* leftLayout = new QVBoxLayout();
    auto* filterBar = new QHBoxLayout();

    m_txtBuscar = new QLineEdit(this);
    m_txtBuscar->setPlaceholderText("Buscar por cédula, NIT, nombre o correo...");
    connect(m_txtBuscar, &QLineEdit::textChanged, this, &TercerosWidget::onFiltroCambiado);

    m_cmbFiltroRol = new QComboBox(this);
    m_cmbFiltroRol->addItem("Todos los Roles", "TODOS");
    m_cmbFiltroRol->addItem("Solo Propietarios", "PROPIETARIO");
    m_cmbFiltroRol->addItem("Solo Inquilinos", "INQUILINO");
    m_cmbFiltroRol->addItem("Solo Residentes", "RESIDENTE");
    m_cmbFiltroRol->addItem("Solo Proveedores", "PROVEEDOR");
    m_cmbFiltroRol->addItem("Solo Visitantes", "VISITANTE");
    connect(m_cmbFiltroRol, &QComboBox::currentIndexChanged, this, &TercerosWidget::onFiltroCambiado);

    filterBar->addWidget(m_txtBuscar, 1);
    filterBar->addWidget(m_cmbFiltroRol);

    m_tablaTerceros = new QTableWidget(this);
    m_tablaTerceros->setColumnCount(6);
    m_tablaTerceros->setHorizontalHeaderLabels({
        "Documento", "Nombre / Razón Social", "Roles", "Celular", "Correo Electrónico", "Estado"
    });
    m_tablaTerceros->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaTerceros->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaTerceros->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaTerceros->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tablaTerceros, &QTableWidget::itemSelectionChanged, this, &TercerosWidget::onFilaSeleccionada);

    leftLayout->addLayout(filterBar);
    leftLayout->addWidget(m_tablaTerceros);

    // --- PANEL DERECHO: FORMULARIO ---
    auto* formGroup = new QGroupBox("Ficha del Tercero", this);
    formGroup->setFixedWidth(400);
    auto* formLayout = new QFormLayout(formGroup);

    m_cmbTipoDoc = new QComboBox(this);
    m_cmbTipoDoc->addItems({"CC", "NIT", "CE", "PASAPORTE", "TI"});

    auto* docLayout = new QHBoxLayout();
    m_txtNumDoc = new QLineEdit(this);
    m_txtNumDoc->setPlaceholderText("Número sin puntos ni guiones");
    m_txtDv = new QLineEdit(this);
    m_txtDv->setFixedWidth(40);
    m_txtDv->setReadOnly(true);
    m_txtDv->setPlaceholderText("DV");
    m_txtDv->setAlignment(Qt::AlignCenter);

    docLayout->addWidget(m_txtNumDoc, 1);
    docLayout->addWidget(new QLabel("-", this));
    docLayout->addWidget(m_txtDv);

    connect(m_cmbTipoDoc, &QComboBox::currentTextChanged, this, &TercerosWidget::onDocumentoOTipoCambiado);
    connect(m_txtNumDoc, &QLineEdit::textChanged, this, &TercerosWidget::onDocumentoOTipoCambiado);

    m_txtNombre = new QLineEdit(this);
    m_txtNombre->setPlaceholderText("Nombres y Apellidos o Razón Social");

    m_txtCelular = new QLineEdit(this);
    m_txtTelefono = new QLineEdit(this);
    m_txtEmail = new QLineEdit(this);
    m_txtEmail->setPlaceholderText("Para envío de cuentas de cobro PDF");
    m_txtDireccion = new QLineEdit(this);
    m_txtDireccion->setPlaceholderText("Dirección de notificación");

    // Grupo de Roles (un tercero puede ser Propietario y Proveedor a la vez)
    auto* rolesBox = new QGroupBox("Clasificación / Roles *", this);
    auto* rolesLayout = new QGridLayout(rolesBox);
    m_chkPropietario = new QCheckBox("Propietario", this);
    m_chkInquilino = new QCheckBox("Inquilino", this);
    m_chkResidente = new QCheckBox("Residente", this);
    m_chkProveedor = new QCheckBox("Proveedor", this);
    m_chkVisitante = new QCheckBox("Visitante", this);
    rolesLayout->addWidget(m_chkPropietario, 0, 0);
    rolesLayout->addWidget(m_chkInquilino, 0, 1);
    rolesLayout->addWidget(m_chkResidente, 0, 2);
    rolesLayout->addWidget(m_chkProveedor, 1, 0);
    rolesLayout->addWidget(m_chkVisitante, 1, 1);

    m_chkActivo = new QCheckBox("Tercero Activo", this);
    m_chkActivo->setChecked(true);

    formLayout->addRow("Tipo Documento:", m_cmbTipoDoc);
    formLayout->addRow("Número / DV *:", docLayout);
    formLayout->addRow("Nombre / Razón *:", m_txtNombre);
    formLayout->addRow("Celular:", m_txtCelular);
    formLayout->addRow("Teléfono Fijo:", m_txtTelefono);
    formLayout->addRow("Correo Electrónico:", m_txtEmail);
    formLayout->addRow("Dirección:", m_txtDireccion);
    formLayout->addRow(rolesBox);
    formLayout->addRow("", m_chkActivo);

    // Botones
    auto* btnLayout = new QHBoxLayout();
    m_btnNuevo = new QPushButton("Nuevo", this);
    m_btnGuardar = new QPushButton("Guardar", this);
    m_btnEliminar = new QPushButton("Eliminar", this);
    m_btnEliminar->setEnabled(false);

    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(m_btnNuevo, &QPushButton::clicked, this, &TercerosWidget::onNuevoClicked);
    connect(m_btnGuardar, &QPushButton::clicked, this, &TercerosWidget::onGuardarClicked);
    connect(m_btnEliminar, &QPushButton::clicked, this, &TercerosWidget::onEliminarClicked);

    btnLayout->addWidget(m_btnNuevo);
    btnLayout->addWidget(m_btnGuardar);
    btnLayout->addWidget(m_btnEliminar);
    formLayout->addRow(btnLayout);

    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addWidget(formGroup);
    mainLayout->addLayout(contentLayout);
}

void TercerosWidget::onDocumentoOTipoCambiado() {
    if (m_cmbTipoDoc->currentText() == "NIT") {
        std::string dv = TerceroRepository::calcularDigitoVerificacionDIAN(
            m_txtNumDoc->text().toStdString()
        );
        m_txtDv->setText(QString::fromStdString(dv));
    } else {
        m_txtDv->clear();
    }
}

void TercerosWidget::recargarTabla() {
    onFiltroCambiado();
}

void TercerosWidget::onFiltroCambiado() {
    std::string texto = m_txtBuscar->text().trimmed().toStdString();
    std::string rol = m_cmbFiltroRol->currentData().toString().toStdString();

    m_tercerosCache = TerceroRepository::listar(texto, rol);
    m_tablaTerceros->setRowCount(static_cast<int>(m_tercerosCache.size()));

    for (int row = 0; row < static_cast<int>(m_tercerosCache.size()); ++row) {
        const auto& t = m_tercerosCache[row];

        QString docFormateado = QString::fromStdString(t.tipoDocumento + " " + t.numeroDocumento);
        if (t.tipoDocumento == "NIT" && !t.dv.empty()) {
            docFormateado += "-" + QString::fromStdString(t.dv);
        }

        QStringList roles;
        if (t.esPropietario) roles << "Propietario";
        if (t.esInquilino) roles << "Inquilino";
        if (t.esResidente) roles << "Residente";
        if (t.esProveedor) roles << "Proveedor";
        if (t.esVisitante) roles << "Visitante";

        m_tablaTerceros->setItem(row, 0, new QTableWidgetItem(docFormateado));
        m_tablaTerceros->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t.nombreRazonSocial)));
        m_tablaTerceros->setItem(row, 2, new QTableWidgetItem(roles.join(" | ")));
        m_tablaTerceros->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(t.celular)));
        m_tablaTerceros->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(t.email)));
        m_tablaTerceros->setItem(row, 5, new QTableWidgetItem(t.activo ? "Activo" : "Inactivo"));
    }
}

void TercerosWidget::onFilaSeleccionada() {
    auto selected = m_tablaTerceros->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    const auto& t = m_tercerosCache[row];

    m_terceroSeleccionadoId = t.id;
    m_cmbTipoDoc->setCurrentText(QString::fromStdString(t.tipoDocumento));
    m_txtNumDoc->setText(QString::fromStdString(t.numeroDocumento));
    m_txtDv->setText(QString::fromStdString(t.dv));
    m_txtNombre->setText(QString::fromStdString(t.nombreRazonSocial));
    m_txtCelular->setText(QString::fromStdString(t.celular));
    m_txtTelefono->setText(QString::fromStdString(t.telefono));
    m_txtEmail->setText(QString::fromStdString(t.email));
    m_txtDireccion->setText(QString::fromStdString(t.direccionCorrespondencia));

    m_chkPropietario->setChecked(t.esPropietario);
    m_chkInquilino->setChecked(t.esInquilino);
    m_chkResidente->setChecked(t.esResidente);
    m_chkProveedor->setChecked(t.esProveedor);
    m_chkVisitante->setChecked(t.esVisitante);
    m_chkActivo->setChecked(t.activo);

    m_btnEliminar->setEnabled(true);
}

void TercerosWidget::onNuevoClicked() {
    limpiarFormulario();
}

void TercerosWidget::limpiarFormulario() {
    m_terceroSeleccionadoId = 0;
    m_tablaTerceros->clearSelection();
    m_cmbTipoDoc->setCurrentIndex(0);
    m_txtNumDoc->clear();
    m_txtDv->clear();
    m_txtNombre->clear();
    m_txtCelular->clear();
    m_txtTelefono->clear();
    m_txtEmail->clear();
    m_txtDireccion->clear();
    m_chkPropietario->setChecked(false);
    m_chkInquilino->setChecked(false);
    m_chkResidente->setChecked(false);
    m_chkProveedor->setChecked(false);
    m_chkVisitante->setChecked(false);
    m_chkActivo->setChecked(true);
    m_btnEliminar->setEnabled(false);
    m_txtNumDoc->setFocus();
}

void TercerosWidget::onGuardarClicked() {
    try {
        Tercero t;
        t.id = m_terceroSeleccionadoId;
        t.tipoDocumento = m_cmbTipoDoc->currentText().toStdString();
        t.numeroDocumento = m_txtNumDoc->text().trimmed().toStdString();
        t.dv = m_txtDv->text().trimmed().toStdString();
        t.nombreRazonSocial = m_txtNombre->text().trimmed().toStdString();
        t.celular = m_txtCelular->text().trimmed().toStdString();
        t.telefono = m_txtTelefono->text().trimmed().toStdString();
        t.email = m_txtEmail->text().trimmed().toStdString();
        t.direccionCorrespondencia = m_txtDireccion->text().trimmed().toStdString();
        t.esPropietario = m_chkPropietario->isChecked();
        t.esInquilino = m_chkInquilino->isChecked();
        t.esResidente = m_chkResidente->isChecked();
        t.esProveedor = m_chkProveedor->isChecked();
        t.esVisitante = m_chkVisitante->isChecked();
        t.activo = m_chkActivo->isChecked();

        if (m_terceroSeleccionadoId == 0) {
            TerceroRepository::crear(t, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Tercero registrado correctamente.");
        } else {
            TerceroRepository::actualizar(t, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Tercero actualizado correctamente.");
        }

        limpiarFormulario();
        recargarTabla();
        emit tercerosActualizados(); // Notifica a UnidadesWidget para actualizar sus listas desplegables
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void TercerosWidget::onEliminarClicked() {
    if (m_terceroSeleccionadoId == 0) return;
    auto reply = QMessageBox::question(
        this, "Confirmar Eliminación",
        "¿Está seguro de eliminar este tercero?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        try {
            TerceroRepository::eliminar(m_terceroSeleccionadoId, m_usuarioId);
            limpiarFormulario();
            recargarTabla();
            emit tercerosActualizados();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }
}
