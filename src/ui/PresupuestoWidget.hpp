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
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include "core/BudgetEngine.hpp"

class PresupuestoWidget : public QWidget {
    Q_OBJECT

public:
    explicit PresupuestoWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTodo();

private slots:
    void onCrearNuevoAnioClicked();
    void onSeleccionPresupuestoCambiada();
    void onAgregarRubroClicked();
    void onEliminarRubroClicked();
    void onExportarPdfClicked();

private:
    void setupUi();
    void cargarComboPresupuestos();
    void cargarComboCuentasPUC();
    void refrescarTablaEjecucion();

    int m_usuarioId;
    ResumenEjecucionPresupuestal m_resumenActual;

    // Selección de vigencia y corte
    QComboBox* m_cmbPresupuestoAnio = nullptr;
    QComboBox* m_cmbMesCorte = nullptr;
    QLabel* m_lblAlertaFondoImprevistos = nullptr;

    // Formulario creación de vigencia anual
    QSpinBox* m_spinNuevoAnio = nullptr;
    QDateEdit* m_dateAprobacion = nullptr;
    QLineEdit* m_txtActaAsamblea = nullptr;
    QDoubleSpinBox* m_spinPctImprevistos = nullptr;

    // Formulario agregar rubro
    QComboBox* m_cmbTipoRubro = nullptr;
    QComboBox* m_cmbCuentaPuc = nullptr;
    QLineEdit* m_txtNombreRubro = nullptr;
    QDoubleSpinBox* m_spinValorAnual = nullptr;

    // Tabla comparativa y totales
    QTableWidget* m_tablaEjecucion = nullptr;
    QLabel* m_lblResumenGlobal = nullptr;
};
