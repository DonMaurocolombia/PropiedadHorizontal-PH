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


#include "ReportesWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>
#include <QDate>

static QString formatoMoneda(int64_t val) {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    return "$ " + col.toString(static_cast<qlonglong>(val));
}

ReportesWidget::ReportesWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    actualizarTodosLosReportes();
}

void ReportesWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* title = new QLabel("<h3>Reportes Financieros y Análisis de Cartera (NIIF Grupo 3)</h3>", this);
    mainLayout->addWidget(title);

    auto* subTabs = new QTabWidget(this);
    subTabs->addTab(crearTabCartera(), "1. Cartera por Edades (Mora)");
    subTabs->addTab(crearTabEstadoResultados(), "2. Estado de Resultados");
    subTabs->addTab(crearTabBalanceGeneral(), "3. Balance General");

    mainLayout->addWidget(subTabs);
}

QWidget* ReportesWidget::crearTabCartera() {
    auto* w = new QWidget(this);
    auto* layout = new QVBoxLayout(w);

    auto* bar = new QHBoxLayout();
    m_dateCorteCartera = new QDateEdit(QDate::currentDate(), this);
    m_dateCorteCartera->setCalendarPopup(true);
    m_dateCorteCartera->setDisplayFormat("yyyy-MM-dd");

    auto* btnConsultar = new QPushButton("Generar Informe", this);
    auto* btnPdf = new QPushButton("Exportar PDF", this);
    btnPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(btnConsultar, &QPushButton::clicked, this, &ReportesWidget::onConsultarCarteraClicked);
    connect(btnPdf, &QPushButton::clicked, this, &ReportesWidget::onExportarCarteraPdfClicked);

    bar->addWidget(new QLabel("Fecha de Corte:", this));
    bar->addWidget(m_dateCorteCartera);
    bar->addWidget(btnConsultar);
    bar->addStretch();
    bar->addWidget(btnPdf);
    layout->addLayout(bar);

    m_tablaCartera = new QTableWidget(this);
    m_tablaCartera->setColumnCount(7);
    m_tablaCartera->setHorizontalHeaderLabels({
        "Unidad", "Propietario", "0 - 30 Días", "31 - 60 Días", "61 - 90 Días", "> 90 Días", "Total Deuda"
    });
    m_tablaCartera->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaCartera->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_tablaCartera);

    m_lblTotalCartera = new QLabel("Total Cartera Vencida: $ 0", this);
    m_lblTotalCartera->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 6px; background: #FFEBEE; color: #C62828;");
    layout->addWidget(m_lblTotalCartera);

    return w;
}

QWidget* ReportesWidget::crearTabEstadoResultados() {
    auto* w = new QWidget(this);
    auto* layout = new QVBoxLayout(w);

    auto* bar = new QHBoxLayout();
    QDate hoy = QDate::currentDate();
    m_dateInicioER = new QDateEdit(QDate(hoy.year(), 1, 1), this);
    m_dateInicioER->setCalendarPopup(true);
    m_dateInicioER->setDisplayFormat("yyyy-MM-dd");

    m_dateFinER = new QDateEdit(hoy, this);
    m_dateFinER->setCalendarPopup(true);
    m_dateFinER->setDisplayFormat("yyyy-MM-dd");

    auto* btnConsultar = new QPushButton("Calcular Estado de Resultados", this);
    auto* btnPdf = new QPushButton("Exportar PDF", this);
    btnPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(btnConsultar, &QPushButton::clicked, this, &ReportesWidget::onConsultarEstadoResultadosClicked);
    connect(btnPdf, &QPushButton::clicked, this, &ReportesWidget::onExportarEstadoResultadosPdfClicked);

    bar->addWidget(new QLabel("Desde:", this));
    bar->addWidget(m_dateInicioER);
    bar->addWidget(new QLabel("Hasta:", this));
    bar->addWidget(m_dateFinER);
    bar->addWidget(btnConsultar);
    bar->addStretch();
    bar->addWidget(btnPdf);
    layout->addLayout(bar);

    m_tablaER = new QTableWidget(this);
    m_tablaER->setColumnCount(4);
    m_tablaER->setHorizontalHeaderLabels({"Grupo", "Código PUC", "Cuenta Contable", "Saldo Periodo"});
    m_tablaER->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaER->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_tablaER);

    m_lblResultadoER = new QLabel("Excedente / (Déficit) del Periodo: $ 0", this);
    m_lblResultadoER->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 6px; background: #E8F5E9; color: #2E7D32;");
    layout->addWidget(m_lblResultadoER);

    return w;
}

QWidget* ReportesWidget::crearTabBalanceGeneral() {
    auto* w = new QWidget(this);
    auto* layout = new QVBoxLayout(w);

    auto* bar = new QHBoxLayout();
    m_dateCorteBG = new QDateEdit(QDate::currentDate(), this);
    m_dateCorteBG->setCalendarPopup(true);
    m_dateCorteBG->setDisplayFormat("yyyy-MM-dd");

    auto* btnConsultar = new QPushButton("Generar Balance", this);
    auto* btnPdf = new QPushButton("Exportar PDF", this);
    btnPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    connect(btnConsultar, &QPushButton::clicked, this, &ReportesWidget::onConsultarBalanceGeneralClicked);
    connect(btnPdf, &QPushButton::clicked, this, &ReportesWidget::onExportarBalanceGeneralPdfClicked);

    bar->addWidget(new QLabel("Fecha de Corte:", this));
    bar->addWidget(m_dateCorteBG);
    bar->addWidget(btnConsultar);
    bar->addStretch();
    bar->addWidget(btnPdf);
    layout->addLayout(bar);

    m_tablaBG = new QTableWidget(this);
    m_tablaBG->setColumnCount(4);
    m_tablaBG->setHorizontalHeaderLabels({"Clase", "Código PUC", "Cuenta Contable", "Saldo Acumulado"});
    m_tablaBG->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaBG->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_tablaBG);

    m_lblEcuacionBG = new QLabel("Activo: $ 0 | Pasivo + Patrimonio: $ 0", this);
    m_lblEcuacionBG->setStyleSheet("font-weight: bold; font-size: 11pt; padding: 6px; background: #E3F2FD; color: #0D47A1;");
    layout->addWidget(m_lblEcuacionBG);

    return w;
}

void ReportesWidget::actualizarTodosLosReportes() {
    onConsultarCarteraClicked();
    onConsultarEstadoResultadosClicked();
    onConsultarBalanceGeneralClicked();
}

void ReportesWidget::onConsultarCarteraClicked() {
    std::string corte = m_dateCorteCartera->date().toString("yyyy-MM-dd").toStdString();
    m_carteraCache = ReportsEngine::generarCarteraPorEdades(corte);

    m_tablaCartera->setRowCount(static_cast<int>(m_carteraCache.size()));
    int64_t granTotal = 0;

    for (int r = 0; r < static_cast<int>(m_carteraCache.size()); ++r) {
        const auto& c = m_carteraCache[r];
        granTotal += c.totalDeuda;

        QString unidad = QString::fromStdString(c.torreBloque + " - " + c.nomenclatura);
        m_tablaCartera->setItem(r, 0, new QTableWidgetItem(unidad));
        m_tablaCartera->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(c.propietarioNombre)));
        m_tablaCartera->setItem(r, 2, new QTableWidgetItem(formatoMoneda(c.de0a30Dias)));
        m_tablaCartera->setItem(r, 3, new QTableWidgetItem(formatoMoneda(c.de31a60Dias)));
        m_tablaCartera->setItem(r, 4, new QTableWidgetItem(formatoMoneda(c.de61a90Dias)));
        m_tablaCartera->setItem(r, 5, new QTableWidgetItem(formatoMoneda(c.masDe90Dias)));
        m_tablaCartera->setItem(r, 6, new QTableWidgetItem(formatoMoneda(c.totalDeuda)));
    }

    m_lblTotalCartera->setText("Total Cartera por Cobrar a Corte: " + formatoMoneda(granTotal));
}

void ReportesWidget::onExportarCarteraPdfClicked() {
    QString ruta = QFileDialog::getSaveFileName(this, "Guardar Cartera por Edades", "Cartera_Edades_PH.pdf", "PDF (*.pdf)");
    if (ruta.isEmpty()) return;

    std::string corte = m_dateCorteCartera->date().toString("yyyy-MM-dd").toStdString();
    ReportsEngine::exportarCarteraPDF(m_carteraCache, corte, ruta.toStdString());
    QMessageBox::information(this, "PDF Exportado", "Informe de Cartera por Edades guardado.");
}

void ReportesWidget::onConsultarEstadoResultadosClicked() {
    std::string ini = m_dateInicioER->date().toString("yyyy-MM-dd").toStdString();
    std::string fin = m_dateFinER->date().toString("yyyy-MM-dd").toStdString();
    m_erCache = ReportsEngine::generarEstadoResultados(ini, fin);

    int totalFilas = static_cast<int>(m_erCache.ingresos.size() + m_erCache.gastos.size());
    m_tablaER->setRowCount(totalFilas);

    int row = 0;
    for (const auto& ing : m_erCache.ingresos) {
        m_tablaER->setItem(row, 0, new QTableWidgetItem("INGRESO"));
        m_tablaER->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(ing.codigo)));
        m_tablaER->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(ing.nombre)));
        m_tablaER->setItem(row, 3, new QTableWidgetItem(formatoMoneda(ing.saldo)));
        row++;
    }
    for (const auto& gas : m_erCache.gastos) {
        m_tablaER->setItem(row, 0, new QTableWidgetItem("GASTO"));
        m_tablaER->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(gas.codigo)));
        m_tablaER->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(gas.nombre)));
        m_tablaER->setItem(row, 3, new QTableWidgetItem(formatoMoneda(gas.saldo)));
        row++;
    }

    m_lblResultadoER->setText(
        QString("Ingresos: %1  |  Gastos: %2  |  Excedente/(Déficit): %3")
            .arg(formatoMoneda(m_erCache.totalIngresos))
            .arg(formatoMoneda(m_erCache.totalGastos))
            .arg(formatoMoneda(m_erCache.excedenteODeficit))
    );
}

void ReportesWidget::onExportarEstadoResultadosPdfClicked() {
    QString ruta = QFileDialog::getSaveFileName(this, "Guardar Estado de Resultados", "Estado_Resultados_PH.pdf", "PDF (*.pdf)");
    if (ruta.isEmpty()) return;

    std::string ini = m_dateInicioER->date().toString("yyyy-MM-dd").toStdString();
    std::string fin = m_dateFinER->date().toString("yyyy-MM-dd").toStdString();

    ReportsEngine::exportarEstadoResultadosPDF(m_erCache, ini, fin, ruta.toStdString());
    QMessageBox::information(this, "PDF Exportado", "Estado de Resultados guardado exitosamente.");
}

void ReportesWidget::onConsultarBalanceGeneralClicked() {
    std::string corte = m_dateCorteBG->date().toString("yyyy-MM-dd").toStdString();
    m_bgCache = ReportsEngine::generarBalanceGeneral(corte);
    int totalFilas = static_cast<int64_t>(m_bgCache.activos.size() + m_bgCache.pasivos.size() + m_bgCache.patrimonio.size() + 1);
    m_tablaBG->setRowCount(totalFilas);
    int row = 0;
    auto insertarFilas = [&](const std::vector<LineaEstadoFinanciero>& lista) {
        for (const auto& l : lista) {
            m_tablaBG->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(l.clase)));
            m_tablaBG->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(l.codigo)));
            m_tablaBG->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(l.nombre)));
            m_tablaBG->setItem(row, 3, new QTableWidgetItem(formatoMoneda(l.saldo)));
            row++;
        }
    };

    insertarFilas(m_bgCache.activos);
    insertarFilas(m_bgCache.pasivos);
    insertarFilas(m_bgCache.patrimonio);
    m_tablaBG->setItem(row, 0, new QTableWidgetItem("PATRIMONIO"));
    m_tablaBG->setItem(row, 1, new QTableWidgetItem("3605"));
    m_tablaBG->setItem(row, 2, new QTableWidgetItem("Resultado Acumulado del Ejercicio"));
    m_tablaBG->setItem(row, 3, new QTableWidgetItem(formatoMoneda(m_bgCache.resultadoEjercicio)));
    m_lblEcuacionBG->setText(
        QString("TOTAL ACTIVO: %1 | TOTAL PASIVO + PATRIMONIO: %2")
            .arg(formatoMoneda(m_bgCache.totalActivos))
            .arg(formatoMoneda(m_bgCache.totalPasivos + m_bgCache.totalPatrimonio))
    );
}

void ReportesWidget::onExportarBalanceGeneralPdfClicked() {
    QString ruta = QFileDialog::getSaveFileName(this, "Guardar Balance General", "Balance_General_PH.pdf", "PDF (*.pdf)");
    if (ruta.isEmpty()) return;
    
    std::string corte = m_dateCorteBG->date().toString("yyyy-MM-dd").toStdString();
    ReportsEngine::exportarBalanceGeneralPDF(m_bgCache, corte, ruta.toStdString());
    QMessageBox::information(this, "PDF Exportado", "Balance General guardado exitosamente.");
}
