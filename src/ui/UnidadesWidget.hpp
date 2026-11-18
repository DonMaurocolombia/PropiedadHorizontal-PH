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
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include "models/UnidadPrivada.hpp"

class UnidadesWidget : public QWidget {
    Q_OBJECT

public:
    explicit UnidadesWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarDatos();

private slots:
    void onBuscarTextoCambiado(const QString& texto);
    void onFilaSeleccionada();
    void onNuevoClicked();
    void onGuardarClicked();
    void onEliminarClicked();

private:
    void setupUi();
    void cargarCombosTerceros();
    void actualizarIndicadorCoeficientes();
    void limpiarFormulario();

    int m_usuarioId;
    int m_unidadSeleccionadaId = 0;
    std::vector<UnidadPrivada> m_unidadesCache;

    // Widgets de cabecera y tabla
    QLabel* m_lblTotalCoeficiente = nullptr;
    QLineEdit* m_txtBuscar = nullptr;
    QTableWidget* m_tablaUnidades = nullptr;

    // Widgets de formulario
    QLineEdit* m_txtTorre = nullptr;
    QLineEdit* m_txtNomenclatura = nullptr;
    QComboBox* m_cmbTipo = nullptr;
    QLineEdit* m_txtMatricula = nullptr;
    QDoubleSpinBox* m_spinArea = nullptr;
    QDoubleSpinBox* m_spinCoeficientePct = nullptr; // Se captura en % (ej. 1.2500%)
    QComboBox* m_cmbPropietario = nullptr;
    QComboBox* m_cmbInquilino = nullptr;
    QComboBox* m_cmbEnviarCobro = nullptr;
    QComboBox* m_cmbEstado = nullptr;

    QPushButton* m_btnNuevo = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnEliminar = nullptr;
};
