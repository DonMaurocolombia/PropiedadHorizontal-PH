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
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <vector>
#include "core/ExpenseEngine.hpp"

class EgresosWidget : public QWidget {
    Q_OBJECT

public:
    explicit EgresosWidget(int usuarioActualId, QWidget* parent = nullptr);

signals:
    void egresoRegistrado();

public slots:
    void recargarDatos();

private slots:
    void recalcularNeto();
    void onRegistrarEgresoClicked();
    void onExportarPdfClicked();
    void onAnularEgresoClicked();

private:
    void setupUi();
    void cargarCombos();
    void cargarTablaEgresos();

    int m_usuarioId;
    std::vector<EgresoResumen> m_egresosCache;

    // Formulario
    QDateEdit* m_dateEgreso = nullptr;
    QComboBox* m_cmbProveedor = nullptr;
    QLineEdit* m_txtNumFactura = nullptr;
    QComboBox* m_cmbCuentaGasto = nullptr;
    QLineEdit* m_txtConcepto = nullptr;
    QDoubleSpinBox* m_spinValorBruto = nullptr;
    QComboBox* m_cmbCuentaRetefuente = nullptr;
    QDoubleSpinBox* m_spinPctRetefuente = nullptr;
    QDoubleSpinBox* m_spinValorRetefuente = nullptr;
    QComboBox* m_cmbCuentaBanco = nullptr;
    QLabel* m_lblValorNetoGirar = nullptr;

    // Tabla y botones
    QTableWidget* m_tablaEgresos = nullptr;
    QPushButton* m_btnExportarPdf = nullptr;
    QPushButton* m_btnAnular = nullptr;
};
