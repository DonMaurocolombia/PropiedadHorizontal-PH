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

#include <QMainWindow>
#include <QTabWidget>
#include <string>
#include "models/UsuarioSesion.hpp"
// #include "core/AuthManager.hpp"   // para UsuarioSesion

// class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // explicit MainWindow(const UsuarioSesion& sesion, QWidget* parent = nullptr);
    explicit MainWindow(const UsuarioSesion& sesion,
                        const std::string& rutaDb,
                        QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUi();
    // void aplicarPermisos();
    void aplicarPermisosPorRol();

    UsuarioSesion m_sesion;
    std::string m_rutaDb;
    QTabWidget* m_tabs = nullptr;

protected:
    void closeEvent(QCloseEvent* event) override;
};
