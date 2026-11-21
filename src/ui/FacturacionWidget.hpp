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
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <vector>
#include "core/BillingEngine.hpp"

class FacturacionWidget : public QWidget {
    Q_OBJECT

public:
    explicit FacturacionWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTabla();

private slots:
    void onGenerarLoteClicked();
    void onExportarPdfIndividualClicked();
    void onExportarPdfMasivoClicked();

private:
    void setupUi();

    int m_usuarioId;
    std::vector<CuentaCobroResumen> m_cuentasCache;

    // Controles de generación masiva
    QLineEdit* m_txtPeriodo = nullptr;
    QDoubleSpinBox* m_spinPresupuestoMensual = nullptr;
    QDateEdit* m_dateEmision = nullptr;
    QDateEdit* m_dateVencimiento = nullptr;
    QPushButton* m_btnGenerarLote = nullptr;

    // Tabla y exportación
    QTableWidget* m_tablaCuentas = nullptr;
    QPushButton* m_btnPdfIndividual = nullptr;
    QPushButton* m_btnPdfMasivo = nullptr;

signals:
    void facturacionGenerada();
};
