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


#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <vector>
#include "models/UsuarioSesion.hpp"

class UsuariosAuditoriaWidget : public QWidget {
    Q_OBJECT

public:
    explicit UsuariosAuditoriaWidget(const UsuarioSesion& sesionActual, QWidget* parent = nullptr);

public slots:
    void recargarTodo();

private slots:
    void onUsuarioSeleccionado();
    void onNuevoUsuarioClicked();
    void onGuardarUsuarioClicked();
    void onFiltroAuditoriaCambiado();

private:
    void setupUi();
    void cargarTablaUsuarios();
    void cargarTablaAuditoria();
    void limpiarFormulario();

    UsuarioSesion m_sesionActual;
    int m_usuarioEditandoId = 0;
    std::vector<UsuarioSesion> m_usuariosCache;

    // Gestión de Usuarios
    QTableWidget* m_tablaUsuarios = nullptr;
    QLineEdit* m_txtUsername = nullptr;
    QLineEdit* m_txtNombre = nullptr;
    QLineEdit* m_txtEmail = nullptr;
    QComboBox* m_cmbRol = nullptr;
    QLineEdit* m_txtPassword = nullptr;
    QCheckBox* m_chkActivo = nullptr;
    QPushButton* m_btnNuevo = nullptr;
    QPushButton* m_btnGuardar = nullptr;

    // Log de Auditoría
    QComboBox* m_cmbFiltroModulo = nullptr;
    QPushButton* m_btnRefrescarAudit = nullptr;
    QTableWidget* m_tablaAuditoria = nullptr;
};
