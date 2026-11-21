#   SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
#
#   SPDX-License-Identifier: GPL-3.0-or-later
#
#   Administrador - PH: Administrador PH busca facilitar y organizar la 
#   administración de copropiedades mediante una plataforma que centralice 
#   la información, los documentos y los procesos administrativos, bajo la 
#   Licencia GPL-3.0-or-later.
#   
#   Copyright (C) 2026 Oscar Mauricio Bonil P.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.21)

project(PropiedadHorizontalPH VERSION 1.0.0 LANGUAGES CXX)

# Estándar C++20 obligatorio
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración automática de Qt (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Buscar componentes de Qt 6 (Widgets para GUI, PrintSupport/Pdf para cuentas de cobro)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)

# Descargar automáticamente SQLiteCpp + SQLite3 embebido
include(FetchContent)
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp
    GIT_TAG        3.4.0
    GIT_SHALLOW    FALSE
)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SQLiteCpp)

# Archivos fuente del proyecto
set(PROJECT_SOURCES
    src/main.cpp
    # Motores y Repositorios (Core)
    src/core/DbManager.hpp
    src/core/DbManager.cpp
    src/core/AccountingEngine.hpp
    src/core/AccountingEngine.cpp
    src/core/UnidadRepository.hpp
    src/core/UnidadRepository.cpp
    src/core/TerceroRepository.hpp
    src/core/TerceroRepository.cpp
    src/core/ParqueaderoRepository.hpp
    src/core/ParqueaderoRepository.cpp
    src/core/BillingEngine.hpp
    src/core/BillingEngine.cpp
    src/core/ConceptosCobroRepository.hpp
    src/core/ConceptosCobroRepository.cpp
    src/core/FacturasServicioEngine.hpp
    src/core/FacturasServicioEngine.cpp
    src/core/CollectionEngine.hpp
    src/core/CollectionEngine.cpp
    src/core/ExpenseEngine.hpp
    src/core/ExpenseEngine.cpp
    src/core/BudgetEngine.hpp
    src/core/BudgetEngine.cpp
    src/core/ReportsEngine.hpp
    src/core/ReportsEngine.cpp
    src/core/AuthManager.hpp
    src/core/AuthManager.cpp
    src/core/PdfGenerator.hpp
    src/core/PdfGenerator.cpp
    src/core/PqrsRepository.hpp
    src/core/PqrsRepository.cpp
    src/core/AsambleaEngine.hpp
    src/core/AsambleaEngine.cpp
    # Modelos
    src/models/UnidadPrivada.hpp
    src/models/Tercero.hpp
    src/models/ParqueaderoBodega.hpp
    src/models/UsuarioSesion.hpp
    src/models/PqrsSancion.hpp
    src/models/Asamblea.hpp
    # Interfaz Gráfica (UI)
    src/ui/LoginDialog.hpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.hpp
    src/ui/MainWindow.cpp
    src/ui/TercerosWidget.hpp
    src/ui/TercerosWidget.cpp
    src/ui/UnidadesWidget.hpp
    src/ui/UnidadesWidget.cpp
    src/ui/ParqueaderosWidget.hpp
    src/ui/ParqueaderosWidget.cpp
    src/ui/FacturacionWidget.hpp
    src/ui/FacturacionWidget.cpp
    src/ui/FacturasServicioWidget.hpp
    src/ui/FacturasServicioWidget.cpp
    src/ui/ConceptosCobroWidget.hpp
    src/ui/ConceptosCobroWidget.cpp
    src/ui/RecaudoWidget.hpp
    src/ui/RecaudoWidget.cpp
    src/ui/EgresosWidget.hpp
    src/ui/EgresosWidget.cpp
    src/ui/PresupuestoWidget.hpp
    src/ui/PresupuestoWidget.cpp
    src/ui/ReportesWidget.hpp
    src/ui/ReportesWidget.cpp
    src/ui/UsuariosAuditoriaWidget.hpp
    src/ui/UsuariosAuditoriaWidget.cpp
    src/ui/PqrsWidget.hpp
    src/ui/PqrsWidget.cpp
    src/ui/AsambleasWidget.hpp
    src/ui/AsambleasWidget.cpp
    src/ui/ConfiguracionWidget.hpp
    src/ui/ConfiguracionWidget.cpp
    src/ui/VerticalTabStyle.hpp
    # Recursos
    resources/resources.qrc
)

# Crear ejecutable (WIN32 oculta la consola negra en Windows)
qt_add_executable(PropiedadHorizontalPH
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Incluir directorios de cabeceras
target_include_directories(PropiedadHorizontalPH PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enlazar librerías de Qt 6 y SQLiteCpp
target_link_libraries(PropiedadHorizontalPH PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    SQLiteCpp
)

# Finalizar configuración de ejecutable Qt 6
qt_finalize_executable(PropiedadHorizontalPH)
