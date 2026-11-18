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


#include "UsuariosAuditoriaWidget.hpp"
#include "core/AuthManager.hpp"
#include "core/DbManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

UsuariosAuditoriaWidget::UsuariosAuditoriaWidget(const UsuarioSesion& sesionActual, QWidget* parent)
    : QWidget(parent), m_sesionActual(sesionActual) {
    setupUi();
    recargarTodo();
}

void UsuariosAuditoriaWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("<h3>Perfiles de Usuario y Pista de Auditoría del Sistema</h3>", this);
    mainLayout->addWidget(titleLabel);

    // --- SECCIÓN SUPERIOR: USUARIOS Y ROLES ---
    auto* topLayout = new QHBoxLayout();

    m_tablaUsuarios = new QTableWidget(this);
    m_tablaUsuarios->setColumnCount(6);
    m_tablaUsuarios->setHorizontalHeaderLabels({
        "Usuario", "Nombre Completo", "Perfil / Rol", "Email", "Estado", "Último Acceso"
    });
    m_tablaUsuarios->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaUsuarios->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaUsuarios->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaUsuarios->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tablaUsuarios, &QTableWidget::itemSelectionChanged,
            this, &UsuariosAuditoriaWidget::onUsuarioSeleccionado);

    auto* formGroup = new QGroupBox("Configuración de Usuario", this);
    formGroup->setFixedWidth(380);
    auto* formLayout = new QFormLayout(formGroup);

    m_txtUsername = new QLineEdit(this);
    m_txtNombre = new QLineEdit(this);
    m_txtEmail = new QLineEdit(this);

    m_cmbRol = new QComboBox(this);
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement rolQuery(db, "SELECT id, nombre FROM roles ORDER BY id ASC;");
    while (rolQuery.executeStep()) {
        m_cmbRol->addItem(
            QString::fromStdString(rolQuery.getColumn(1).getString()),
            rolQuery.getColumn(0).getInt()
        );
    }

    m_txtPassword = new QLineEdit(this);
    m_txtPassword->setEchoMode(QLineEdit::Password);
    m_txtPassword->setPlaceholderText("Mínimo 6 caracteres");

    m_chkActivo = new QCheckBox("Usuario Activo", this);
    m_chkActivo->setChecked(true);

    formLayout->addRow("Usuario (Login) *:", m_txtUsername);
    formLayout->addRow("Nombre Completo *:", m_txtNombre);
    formLayout->addRow("Correo:", m_txtEmail);
    formLayout->addRow("Rol / Perfil *:", m_cmbRol);
    formLayout->addRow("Contraseña:", m_txtPassword);
    formLayout->addRow("", m_chkActivo);

    auto* btnLayout = new QHBoxLayout();
    m_btnNuevo = new QPushButton("Nuevo", this);
    m_btnGuardar = new QPushButton("Guardar Usuario", this);
    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(m_btnNuevo, &QPushButton::clicked, this, &UsuariosAuditoriaWidget::onNuevoUsuarioClicked);
    connect(m_btnGuardar, &QPushButton::clicked, this, &UsuariosAuditoriaWidget::onGuardarUsuarioClicked);

    // Solo el rol ADMIN puede crear o modificar usuarios
    bool esAdmin = (m_sesionActual.rolNombre == "ADMIN");
    formGroup->setEnabled(esAdmin);

    btnLayout->addWidget(m_btnNuevo);
    btnLayout->addWidget(m_btnGuardar);
    formLayout->addRow(btnLayout);

    topLayout->addWidget(m_tablaUsuarios, 1);
    topLayout->addWidget(formGroup);
    mainLayout->addLayout(topLayout, 1);

    // --- SECCIÓN INFERIOR: LOG DE AUDITORÍA ---
    auto* auditBar = new QHBoxLayout();
    auditBar->addWidget(new QLabel("<h4>Registro de Auditoría (Trazabilidad Revisoría Fiscal)</h4>", this));
    auditBar->addStretch();

    m_cmbFiltroModulo = new QComboBox(this);
    m_cmbFiltroModulo->addItems({
        "TODOS", "SEGURIDAD", "UNIDADES", "TERCEROS", "FACTURACION", "RECAUDO", "CONTABILIDAD"
    });
    connect(m_cmbFiltroModulo, &QComboBox::currentTextChanged,
            this, &UsuariosAuditoriaWidget::onFiltroAuditoriaCambiado);

    m_btnRefrescarAudit = new QPushButton("Actualizar Logs", this);
    connect(m_btnRefrescarAudit, &QPushButton::clicked,
            this, &UsuariosAuditoriaWidget::onFiltroAuditoriaCambiado);

    auditBar->addWidget(new QLabel("Filtrar Módulo:", this));
    auditBar->addWidget(m_cmbFiltroModulo);
    auditBar->addWidget(m_btnRefrescarAudit);
    mainLayout->addLayout(auditBar);

    m_tablaAuditoria = new QTableWidget(this);
    m_tablaAuditoria->setColumnCount(6);
    m_tablaAuditoria->setHorizontalHeaderLabels({
        "Fecha y Hora", "Usuario", "Módulo", "Acción", "Tabla / ID", "Detalle de la Operación"
    });
    m_tablaAuditoria->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaAuditoria->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_tablaAuditoria, 1);
}

void UsuariosAuditoriaWidget::recargarTodo() {
    cargarTablaUsuarios();
    cargarTablaAuditoria();
}

void UsuariosAuditoriaWidget::cargarTablaUsuarios() {
    m_usuariosCache = AuthManager::listarUsuarios();
    m_tablaUsuarios->setRowCount(static_cast<int>(m_usuariosCache.size()));

    for (int row = 0; row < static_cast<int>(m_usuariosCache.size()); ++row) {
        const auto& u = m_usuariosCache[row];
        m_tablaUsuarios->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(u.username)));
        m_tablaUsuarios->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(u.nombreCompleto)));
        m_tablaUsuarios->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(u.rolNombre)));
        m_tablaUsuarios->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(u.email)));
        m_tablaUsuarios->setItem(row, 4, new QTableWidgetItem(u.activo ? "Activo" : "Inactivo"));
        m_tablaUsuarios->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(u.ultimoAcceso)));
    }
}

void UsuariosAuditoriaWidget::cargarTablaAuditoria() {
    std::string modulo = m_cmbFiltroModulo->currentText().toStdString();
    auto logs = AuthManager::listarAuditoria(modulo, 200);
    m_tablaAuditoria->setRowCount(static_cast<int>(logs.size()));

    for (int row = 0; row < static_cast<int>(logs.size()); ++row) {
        const auto& log = logs[row];
        QString refTabla = QString::fromStdString(log.tablaAfectada) + " #" + QString::number(log.registroId);

        m_tablaAuditoria->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(log.fechaHora)));
        m_tablaAuditoria->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(log.usuarioNombre)));
        m_tablaAuditoria->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(log.modulo)));
        m_tablaAuditoria->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(log.accion)));
        m_tablaAuditoria->setItem(row, 4, new QTableWidgetItem(refTabla));
        m_tablaAuditoria->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(log.detalle)));
    }
}

void UsuariosAuditoriaWidget::onFiltroAuditoriaCambiado() {
    cargarTablaAuditoria();
}

void UsuariosAuditoriaWidget::onUsuarioSeleccionado() {
    auto selected = m_tablaUsuarios->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    const auto& u = m_usuariosCache[row];

    m_usuarioEditandoId = u.id;
    m_txtUsername->setText(QString::fromStdString(u.username));
    m_txtUsername->setReadOnly(true);
    m_txtNombre->setText(QString::fromStdString(u.nombreCompleto));
    m_txtEmail->setText(QString::fromStdString(u.email));

    int idxRol = m_cmbRol->findData(u.rolId);
    if (idxRol >= 0) m_cmbRol->setCurrentIndex(idxRol);

    m_txtPassword->clear();
    m_txtPassword->setPlaceholderText("Dejar vacío para conservar actual");
    m_chkActivo->setChecked(u.activo);
}

void UsuariosAuditoriaWidget::onNuevoUsuarioClicked() {
    limpiarFormulario();
}

void UsuariosAuditoriaWidget::limpiarFormulario() {
    m_usuarioEditandoId = 0;
    m_tablaUsuarios->clearSelection();
    m_txtUsername->clear();
    m_txtUsername->setReadOnly(false);
    m_txtNombre->clear();
    m_txtEmail->clear();
    m_cmbRol->setCurrentIndex(0);
    m_txtPassword->clear();
    m_txtPassword->setPlaceholderText("Mínimo 6 caracteres");
    m_chkActivo->setChecked(true);
    m_txtUsername->setFocus();
}

void UsuariosAuditoriaWidget::onGuardarUsuarioClicked() {
    try {
        UsuarioSesion u;
        u.id = m_usuarioEditandoId;
        u.username = m_txtUsername->text().trimmed().toStdString();
        u.nombreCompleto = m_txtNombre->text().trimmed().toStdString();
        u.email = m_txtEmail->text().trimmed().toStdString();
        u.rolId = m_cmbRol->currentData().toInt();
        u.activo = m_chkActivo->isChecked();

        std::string pass = m_txtPassword->text().toStdString();

        if (m_usuarioEditandoId == 0) {
            AuthManager::crearUsuario(u, pass, m_sesionActual.id);
            QMessageBox::information(this, "Éxito", "Usuario creado correctamente.");
        } else {
            AuthManager::actualizarUsuario(u, pass, m_sesionActual.id);
            QMessageBox::information(this, "Éxito", "Perfil de usuario actualizado.");
        }

        limpiarFormulario();
        recargarTodo();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}
