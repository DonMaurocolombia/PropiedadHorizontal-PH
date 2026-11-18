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
#include <QCheckBox>
#include <QPushButton>
#include <vector>
#include "models/Tercero.hpp"

class TercerosWidget : public QWidget {
    Q_OBJECT

public:
    explicit TercerosWidget(int usuarioActualId, QWidget* parent = nullptr);

signals:
    // Señal emitida cuando se crea/edita un tercero para refrescar combos en UnidadesWidget
    void tercerosActualizados();

public slots:
    void recargarTabla();

private slots:
    void onFiltroCambiado();
    void onDocumentoOTipoCambiado();
    void onFilaSeleccionada();
    void onNuevoClicked();
    void onGuardarClicked();
    void onEliminarClicked();

private:
    void setupUi();
    void limpiarFormulario();

    int m_usuarioId;
    int m_terceroSeleccionadoId = 0;
    std::vector<Tercero> m_tercerosCache;

    // Filtros y tabla
    QLineEdit* m_txtBuscar = nullptr;
    QComboBox* m_cmbFiltroRol = nullptr;
    QTableWidget* m_tablaTerceros = nullptr;

    // Formulario
    QComboBox* m_cmbTipoDoc = nullptr;
    QLineEdit* m_txtNumDoc = nullptr;
    QLineEdit* m_txtDv = nullptr;
    QLineEdit* m_txtNombre = nullptr;
    QLineEdit* m_txtCelular = nullptr;
    QLineEdit* m_txtTelefono = nullptr;
    QLineEdit* m_txtEmail = nullptr;
    QLineEdit* m_txtDireccion = nullptr;

    QCheckBox* m_chkPropietario = nullptr;
    QCheckBox* m_chkInquilino = nullptr;
    QCheckBox* m_chkProveedor = nullptr;
    QCheckBox* m_chkActivo = nullptr;

    QPushButton* m_btnNuevo = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnEliminar = nullptr;
};
