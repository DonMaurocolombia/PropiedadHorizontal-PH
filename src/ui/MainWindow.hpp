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


#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <string>
#include "models/UsuarioSesion.hpp"
#include "ui/TercerosWidget.hpp"
#include "ui/UnidadesWidget.hpp"
#include "ui/ParqueaderosWidget.hpp"
#include "ui/FacturacionWidget.hpp"
#include "ui/FacturasServicioWidget.hpp"
#include "ui/ConceptosCobroWidget.hpp"
#include "ui/RecaudoWidget.hpp"
#include "ui/EgresosWidget.hpp"
#include "ui/PresupuestoWidget.hpp"
#include "ui/ReportesWidget.hpp"
#include "ui/AsambleasWidget.hpp"
#include "ui/UsuariosAuditoriaWidget.hpp"
#include "ui/ConfiguracionWidget.hpp"

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

    TercerosWidget*          m_tabTerceros          = nullptr;
    UnidadesWidget*          m_tabUnidades          = nullptr;
    ParqueaderosWidget*      m_tabParqueaderos      = nullptr;
    FacturacionWidget*       m_tabFacturacion       = nullptr;
    FacturasServicioWidget*  m_tabFacturasServicio  = nullptr;
    ConceptosCobroWidget*    m_tabConceptosCobro    = nullptr;
    RecaudoWidget*           m_tabRecaudo           = nullptr;
    EgresosWidget*           m_tabEgresos           = nullptr;
    PresupuestoWidget*       m_tabPresupuesto       = nullptr;
    ReportesWidget*          m_tabReportes          = nullptr;
    AsambleasWidget*         m_tabAsambleas         = nullptr;
    UsuariosAuditoriaWidget* m_tabSeguridad         = nullptr;
    ConfiguracionWidget*     m_tabConfig            = nullptr;


protected:
    void closeEvent(QCloseEvent* event) override;
};
