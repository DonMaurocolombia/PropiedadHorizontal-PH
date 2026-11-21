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


#include "LoginDialog.hpp"
#include "core/AuthManager.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Iniciar Sesión - Propiedad Horizontal PH");
    setFixedSize(380, 240);
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel(
        "<h3 style='color:#1976D2; margin-bottom:2px;'>Control de Acceso</h3>"
        "<p style='color:#555; font-size:9pt;'>Administración Propiedad Horizontal (Ley 675)</p>",
        this
    );
    header->setAlignment(Qt::AlignCenter);
    layout->addWidget(header);

    auto* form = new QFormLayout();
    m_txtUsername = new QLineEdit(this);
    m_txtUsername->setPlaceholderText("Usuario (ej: admin)");

    m_txtPassword = new QLineEdit(this);
    m_txtPassword->setEchoMode(QLineEdit::Password);
    m_txtPassword->setPlaceholderText("Contraseña");

    form->addRow("Usuario:", m_txtUsername);
    form->addRow("Contraseña:", m_txtPassword);
    layout->addLayout(form);

    m_btnIngresar = new QPushButton("Ingresar al Sistema", this);
    m_btnIngresar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 8px;");
    m_btnIngresar->setDefault(true);
    connect(m_btnIngresar, &QPushButton::clicked, this, &LoginDialog::onIngresarClicked);

    layout->addWidget(m_btnIngresar);
}

void LoginDialog::onIngresarClicked() {
    std::string user = m_txtUsername->text().trimmed().toStdString();
    std::string pass = m_txtPassword->text().toStdString();

    if (user.empty() || pass.empty()) {
        QMessageBox::warning(this, "Campos Vacíos", "Ingrese su usuario y contraseña.");
        return;
    }

    try {
        auto sesionOpt = AuthManager::iniciarSesion(user, pass);
        if (!sesionOpt.has_value()) {
            QMessageBox::warning(this, "Acceso Denegado", "Usuario o contraseña incorrectos.");
            m_txtPassword->clear();
            m_txtPassword->setFocus();
            return;
        }

        m_sesion = sesionOpt.value();
        accept();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error de Autenticación", e.what());
    }
}
