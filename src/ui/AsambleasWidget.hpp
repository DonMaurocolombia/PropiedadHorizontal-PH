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
#include <QDateEdit>
#include <QTimeEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QPushButton>
#include <vector>
#include "models/Asamblea.hpp"
#include "models/UnidadPrivada.hpp"

class AsambleasWidget : public QWidget {
    Q_OBJECT

public:
    explicit AsambleasWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTodo();

private slots:
    void onCrearAsambleaClicked();
    void onAsambleaSeleccionada();
    void onUnidadAsistenteCambiada(int index);
    void onRegistrarAsistenciaClicked();
    void onQuitarAsistenciaClicked();
    void onCerrarAsambleaClicked();
    void onExportarPlanillaPdfClicked();

private:
    void setupUi();
    void cargarCombos();
    void refrescarTablaAsistencia();

    int m_usuarioId;
    std::vector<Asamblea> m_asambleasCache;
    std::vector<UnidadPrivada> m_unidadesCache;
    std::vector<AsistenciaItem> m_asistenciaCache;

    // Panel superior: Indicador de Quórum en vivo
    QComboBox* m_cmbAsambleaActiva = nullptr;
    QLabel* m_lblEstadoQuorum = nullptr;
    QProgressBar* m_barQuorum = nullptr;

    // Formulario apertura asamblea
    QComboBox* m_cmbTipoAsamblea = nullptr;
    QDateEdit* m_dateAsamblea = nullptr;
    QTimeEdit* m_timeInicio = nullptr;
    QComboBox* m_cmbTipoQuorum = nullptr;

    // Formulario registro asistencia
    QComboBox* m_cmbUnidad = nullptr;
    QLineEdit* m_txtNombreAsistente = nullptr;
    QCheckBox* m_chkEsApoderado = nullptr;
    QPushButton* m_btnRegistrarAsistencia = nullptr;

    // Tabla asistencia y acciones
    QTableWidget* m_tablaAsistencia = nullptr;
    QPushButton* m_btnCerrarAsamblea = nullptr;
};
