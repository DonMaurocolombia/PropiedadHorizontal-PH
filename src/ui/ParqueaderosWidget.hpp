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
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <vector>
#include "models/ParqueaderoBodega.hpp"

class ParqueaderosWidget : public QWidget {
    Q_OBJECT

public:
    explicit ParqueaderosWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTodo();

private:
    void setupUi();
    QWidget* crearTabParqueaderos();
    QWidget* crearTabBodegas();
    QWidget* crearTabVehiculos();
    void cargarCombosUnidadesYParqueaderos();

    int m_usuarioId;

    // --- Sub-tab 1: Parqueaderos ---
    int m_parqEditandoId = 0;
    std::vector<Parqueadero> m_parqCache;
    QLineEdit* m_txtBuscarParq = nullptr;
    QTableWidget* m_tablaParq = nullptr;
    QLineEdit* m_txtNumParq = nullptr;
    QLineEdit* m_txtNivelParq = nullptr;
    QComboBox* m_cmbTenenciaParq = nullptr;
    QLineEdit* m_txtMatParq = nullptr;
    QDoubleSpinBox* m_spinCoefParq = nullptr;
    QComboBox* m_cmbUnidadParq = nullptr;
    QDoubleSpinBox* m_spinCanonParq = nullptr;
    QComboBox* m_cmbEstadoParq = nullptr;

    // --- Sub-tab 2: Bodegas ---
    int m_bodEditandoId = 0;
    std::vector<Bodega> m_bodCache;
    QLineEdit* m_txtBuscarBod = nullptr;
    QTableWidget* m_tablaBod = nullptr;
    QLineEdit* m_txtNumBod = nullptr;
    QLineEdit* m_txtUbicacionBod = nullptr;
    QComboBox* m_cmbTenenciaBod = nullptr;
    QLineEdit* m_txtMatBod = nullptr;
    QDoubleSpinBox* m_spinCoefBod = nullptr;
    QComboBox* m_cmbUnidadBod = nullptr;
    QDoubleSpinBox* m_spinCanonBod = nullptr;

    // --- Sub-tab 3: Vehículos ---
    int m_vehEditandoId = 0;
    std::vector<Vehiculo> m_vehCache;
    QLineEdit* m_txtBuscarVeh = nullptr;
    QTableWidget* m_tablaVeh = nullptr;
    QComboBox* m_cmbUnidadVeh = nullptr;
    QComboBox* m_cmbParqVeh = nullptr;
    QLineEdit* m_txtPlacaVeh = nullptr;
    QComboBox* m_cmbTipoVeh = nullptr;
    QLineEdit* m_txtMarcaVeh = nullptr;
    QLineEdit* m_txtColorVeh = nullptr;
};
