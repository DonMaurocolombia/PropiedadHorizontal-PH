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
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

class RecaudoWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecaudoWidget(int usuarioActualId, QWidget* parent = nullptr);

signals:
    void pagoRegistrado();

public slots:
    void recargarDatos();

private slots:
    void onUnidadSeleccionada(int index);
    void onRegistrarPagoClicked();
    void onGenerarPazYSalvoClicked();

private:
    void setupUi();
    void cargarComboUnidades();
    void cargarHistorialRecibos();

    int m_usuarioId;

    // Selección de unidad y estado de cartera
    QComboBox* m_cmbUnidad = nullptr;
    QLabel* m_lblSaldoUnidad = nullptr;
    QTableWidget* m_tablaDeudaDetalle = nullptr;

    // Formulario de pago
    QDateEdit* m_datePago = nullptr;
    QComboBox* m_cmbFormaPago = nullptr;
    QComboBox* m_cmbCuentaBanco = nullptr;
    QLineEdit* m_txtReferencia = nullptr;
    QDoubleSpinBox* m_spinValorPagado = nullptr;
    QLineEdit* m_txtObservaciones = nullptr;

    QPushButton* m_btnRegistrarPago = nullptr;
    QPushButton* m_btnPazYSalvo = nullptr;

    // Tabla inferior: Historial de Recibos de Caja
    QTableWidget* m_tablaRecibos = nullptr;
};
