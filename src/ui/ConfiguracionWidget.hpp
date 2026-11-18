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
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include "models/UsuarioSesion.hpp"


class ConfiguracionWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConfiguracionWidget(const UsuarioSesion& sesion,
                                 const std::string& rutaDbActual,
                                 QWidget* parent = nullptr);

signals:
    void configuracionActualizada(const QString& nuevaRazonSocial);

private slots:
    void cargarDatos();
    void onGuardarClicked();
    void onCrearBackupClicked();
    void cargarHistorialTasas();
    void onTasaEACambiada(double tasaEaPct);
    void onRegistrarTasaMensualClicked();

private:
    void setupUi();

    UsuarioSesion m_sesion;
    std::string m_rutaDb;

    // Datos legales
    QLineEdit* m_txtNit = nullptr;
    QLineEdit* m_txtRazonSocial = nullptr;
    QLineEdit* m_txtDireccion = nullptr;
    QLineEdit* m_txtTelefono = nullptr;
    QLineEdit* m_txtEmail = nullptr;
    QLineEdit* m_txtRepLegal = nullptr;
    QLineEdit* m_txtContadorNombre = nullptr;
    QLineEdit* m_txtContadorTp = nullptr;
    QLineEdit* m_txtRevisorNombre = nullptr;
    QLineEdit* m_txtRevisorTp = nullptr;

    // Políticas generales
    QDoubleSpinBox* m_spinTasaMoraPct = nullptr;
    QSpinBox* m_spinDiaProntoPago = nullptr;
    QDoubleSpinBox* m_spinDescuentoProntoPagoPct = nullptr;

    // Historial mensual Superfinanciera
    QLineEdit* m_txtPeriodoTasa = nullptr;
    QDoubleSpinBox* m_spinTasaEA = nullptr;
    QDoubleSpinBox* m_spinTasaMensualCalc = nullptr;
    QLineEdit* m_txtResolucionSfc = nullptr;
    QPushButton* m_btnGuardarTasaMes = nullptr;
    QTableWidget* m_tablaTasasHistorial = nullptr;

    // Acciones generales
    QComboBox* m_spinOrientacionMenu = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnBackup = nullptr;
};
