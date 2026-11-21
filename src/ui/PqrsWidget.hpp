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
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <vector>
#include "models/PqrsSancion.hpp"

class PqrsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PqrsWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTodo();

private slots:
    void onFiltroCambiado();
    void onCasoSeleccionado();
    void onRadicarNuevoClicked();
    void onActualizarTramiteClicked();

private:
    void setupUi();
    void cargarCombos();

    int m_usuarioId;
    int m_casoSeleccionadoId = 0;
    std::vector<PqrsSancion> m_casosCache;

    // Filtros y tabla central
    QLineEdit* m_txtBuscar = nullptr;
    QComboBox* m_cmbFiltroEstado = nullptr;
    QTableWidget* m_tablaCasos = nullptr;

    // Formulario 1: Radicar nueva PQRS / Proceso
    QComboBox* m_cmbTipo = nullptr;
    QComboBox* m_cmbUnidad = nullptr;
    QComboBox* m_cmbTercero = nullptr;
    QLineEdit* m_txtAsunto = nullptr;
    QTextEdit* m_txtDescripcion = nullptr;
    QDateEdit* m_dateRadicacion = nullptr;

    // Formulario 2: Gestión de Debido Proceso y Sanción
    QComboBox* m_cmbNuevoEstado = nullptr;
    QDateEdit* m_dateDescargos = nullptr;
    QDoubleSpinBox* m_spinValorMulta = nullptr;
    QTextEdit* m_txtNotaSeguimiento = nullptr;
    QPushButton* m_btnActualizarCaso = nullptr;
};
