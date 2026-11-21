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

#include <QWidget>
#include <QTabWidget>
#include <QDateEdit>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include "core/ReportsEngine.hpp"

class ReportesWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReportesWidget(QWidget* parent = nullptr);

public slots:
    void actualizarTodosLosReportes();

private slots:
    void onConsultarCarteraClicked();
    void onExportarCarteraPdfClicked();

    void onConsultarEstadoResultadosClicked();
    void onExportarEstadoResultadosPdfClicked();

    void onConsultarBalanceGeneralClicked();
    void onExportarBalanceGeneralPdfClicked();

private:
    void setupUi();
    QWidget* crearTabCartera();
    QWidget* crearTabEstadoResultados();
    QWidget* crearTabBalanceGeneral();

    // Tab 1: Cartera por Edades
    QDateEdit* m_dateCorteCartera = nullptr;
    QTableWidget* m_tablaCartera = nullptr;
    QLabel* m_lblTotalCartera = nullptr;
    std::vector<CarteraEdadItem> m_carteraCache;

    // Tab 2: Estado de Resultados
    QDateEdit* m_dateInicioER = nullptr;
    QDateEdit* m_dateFinER = nullptr;
    QTableWidget* m_tablaER = nullptr;
    QLabel* m_lblResultadoER = nullptr;
    EstadoResultadosResumen m_erCache;

    // Tab 3: Balance General
    QDateEdit* m_dateCorteBG = nullptr;
    QTableWidget* m_tablaBG = nullptr;
    QLabel* m_lblEcuacionBG = nullptr;
    BalanceGeneralResumen m_bgCache;
};
