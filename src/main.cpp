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


#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QStyleFactory>
#include "core/DbManager.hpp"
#include "core/AuthManager.hpp"
#include "ui/LoginDialog.hpp"
#include "ui/MainWindow.hpp"


int main(int argc, char* argv[]) {
    // hace refencia al archivo PropiedadHorizontalPH.desktop
    QGuiApplication::setDesktopFileName("PropiedadHorizontalPH");

    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));  // ← agrega esta línea
    app.setApplicationName("PropiedadHorizontalPH");
    app.setOrganizationName("GestionPHColombia");

    // Guardar la base de datos en la carpeta de datos locales del usuario
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    std::string dbPath = (dataDir + "/copropiedad.db").toStdString();
    qDebug() << "dataDir:" << dataDir;

    if (!DatabaseManager::instance().initialize(dbPath)) {
        QMessageBox::critical(
            nullptr,
            "Error Crítico",
            "No se pudo inicializar la base de datos SQLite en [" + dataDir + "]."
        );
        return 1;
    }

    // Crea el usuario admin por defecto si la tabla usuarios está vacía
    AuthManager::asegurarUsuarioAdminInicial();

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 0; // El usuario cerró el login sin autenticarse
    }

    MainWindow window(login.getSesionAutenticada(), dbPath);
    window.show();

    return app.exec();
}
