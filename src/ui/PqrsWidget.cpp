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


#include "PqrsWidget.hpp"
#include "core/PqrsRepository.hpp"
#include "core/UnidadRepository.hpp"
#include "core/TerceroRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QLocale>
#include <QDate>
#include <QColor>

PqrsWidget::PqrsWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTodo();
}

void PqrsWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("<h3>Gestión de PQRS, Convivencia y Procesos Sancionatorios (Art. 59-60 Ley 675)</h3>", this));

    auto* contentLayout = new QHBoxLayout();

    // --- PANEL IZQUIERDO: FILTROS + TABLA DE RADICADOS ---
    auto* leftLayout = new QVBoxLayout();
    auto* filterBar = new QHBoxLayout();

    m_txtBuscar = new QLineEdit(this);
    m_txtBuscar->setPlaceholderText("Buscar por radicado, asunto, unidad o residente...");

    m_cmbFiltroEstado = new QComboBox(this);
    m_cmbFiltroEstado->addItems({"TODOS", "ABIERTA", "EN_DESCARGOS", "SANCIONADA", "CERRADA"});

    connect(m_txtBuscar, &QLineEdit::textChanged, this, &PqrsWidget::onFiltroCambiado);
    connect(m_cmbFiltroEstado, &QComboBox::currentTextChanged, this, &PqrsWidget::onFiltroCambiado);

    filterBar->addWidget(m_txtBuscar, 1);
    filterBar->addWidget(m_cmbFiltroEstado);

    m_tablaCasos = new QTableWidget(this);
    m_tablaCasos->setColumnCount(7);
    m_tablaCasos->setHorizontalHeaderLabels({
        "Radicado", "Fecha", "Tipo", "Unidad", "Residente / Tercero", "Multa Impuesta", "Estado"
    });
    m_tablaCasos->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaCasos->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaCasos->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaCasos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tablaCasos, &QTableWidget::itemSelectionChanged, this, &PqrsWidget::onCasoSeleccionado);

    leftLayout->addLayout(filterBar);
    leftLayout->addWidget(m_tablaCasos);

    // --- PANEL DERECHO: RADICACIÓN Y DEBIDO PROCESO ---
    auto* rightPanel = new QVBoxLayout();

    // Grupo 1: Radicar nuevo caso
    auto* newGroup = new QGroupBox("1. Radicar Nueva PQRS / Apertura Proceso", this);
    newGroup->setFixedWidth(410);
    auto* formNew = new QFormLayout(newGroup);

    m_cmbTipo = new QComboBox(this);
    m_cmbTipo->addItems({"PETICION", "QUEJA", "RECLAMO", "SUGERENCIA", "PROCESO_SANCIONATORIO"});

    m_cmbUnidad = new QComboBox(this);
    m_cmbTercero = new QComboBox(this);

    m_dateRadicacion = new QDateEdit(QDate::currentDate(), this);
    m_dateRadicacion->setCalendarPopup(true);
    m_dateRadicacion->setDisplayFormat("yyyy-MM-dd");

    m_txtAsunto = new QLineEdit(this);
    m_txtAsunto->setPlaceholderText("Ej: Ruido excesivo / Solicitud mantenimiento");

    m_txtDescripcion = new QTextEdit(this);
    m_txtDescripcion->setMaximumHeight(75);
    m_txtDescripcion->setPlaceholderText("Relato de los hechos, pruebas o solicitud...");

    auto* btnRadicar = new QPushButton("Radicar Caso", this);
    btnRadicar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 6px;");
    connect(btnRadicar, &QPushButton::clicked, this, &PqrsWidget::onRadicarNuevoClicked);

    formNew->addRow("Tipo de Trámite:", m_cmbTipo);
    formNew->addRow("Unidad Privada:", m_cmbUnidad);
    formNew->addRow("Residente / Tercero *:", m_cmbTercero);
    formNew->addRow("Fecha Radicación:", m_dateRadicacion);
    formNew->addRow("Asunto *:", m_txtAsunto);
    formNew->addRow("Descripción *:", m_txtDescripcion);
    formNew->addRow(btnRadicar);

    // Grupo 2: Seguimiento, Descargos y Sanción
    auto* procGroup = new QGroupBox("2. Trámite, Descargos y Resolución", this);
    procGroup->setFixedWidth(410);
    auto* formProc = new QFormLayout(procGroup);

    m_cmbNuevoEstado = new QComboBox(this);
    m_cmbNuevoEstado->addItems({"EN_DESCARGOS", "CERRADA", "SANCIONADA"});

    m_dateDescargos = new QDateEdit(QDate::currentDate().addDays(5), this);
    m_dateDescargos->setCalendarPopup(true);
    m_dateDescargos->setDisplayFormat("yyyy-MM-dd");

    m_spinValorMulta = new QDoubleSpinBox(this);
    m_spinValorMulta->setRange(0.0, 20000000.0);
    m_spinValorMulta->setDecimals(0);
    m_spinValorMulta->setPrefix("$ ");
    m_spinValorMulta->setGroupSeparatorShown(true);
    m_spinValorMulta->setEnabled(false);

    connect(m_cmbNuevoEstado, &QComboBox::currentTextChanged, this, [this](const QString& st) {
        m_spinValorMulta->setEnabled(st == "SANCIONADA");
        if (st != "SANCIONADA") m_spinValorMulta->setValue(0);
    });

    m_txtNotaSeguimiento = new QTextEdit(this);
    m_txtNotaSeguimiento->setMaximumHeight(65);
    m_txtNotaSeguimiento->setPlaceholderText("Acta de descargos, respuesta emitida o resolución...");

    m_btnActualizarCaso = new QPushButton("Guardar Actuación / Resolución", this);
    m_btnActualizarCaso->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 6px;");
    m_btnActualizarCaso->setEnabled(false);
    connect(m_btnActualizarCaso, &QPushButton::clicked, this, &PqrsWidget::onActualizarTramiteClicked);

    formProc->addRow("Siguiente Etapa:", m_cmbNuevoEstado);
    formProc->addRow("Fecha Descargos:", m_dateDescargos);
    formProc->addRow("Valor Multa (COP):", m_spinValorMulta);
    formProc->addRow("Acta / Respuesta:", m_txtNotaSeguimiento);
    formProc->addRow(m_btnActualizarCaso);

    rightPanel->addWidget(newGroup);
    rightPanel->addWidget(procGroup);
    rightPanel->addStretch();

    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addLayout(rightPanel);
    mainLayout->addLayout(contentLayout);
}

void PqrsWidget::recargarTodo() {
    cargarCombos();
    onFiltroCambiado();
}

void PqrsWidget::cargarCombos() {
    m_cmbUnidad->clear();
    m_cmbUnidad->addItem("-- General / Zonas Comunes --", 0);
    for (const auto& u : UnidadRepository::listarTodas()) {
        m_cmbUnidad->addItem(QString::fromStdString(u.torreBloque + " - " + u.nomenclatura), u.id);
    }

    m_cmbTercero->clear();
    m_cmbTercero->addItem("-- Seleccione Residente / Tercero --", 0);
    for (const auto& t : TerceroRepository::listar()) {
        m_cmbTercero->addItem(QString::fromStdString(t.nombreRazonSocial + " (" + t.numeroDocumento + ")"), t.id);
    }
}

void PqrsWidget::onFiltroCambiado() {
    QLocale col(QLocale::Spanish, QLocale::Colombia);
    std::string estado = m_cmbFiltroEstado->currentText().toStdString();
    std::string texto = m_txtBuscar->text().trimmed().toStdString();

    m_casosCache = PqrsRepository::listar(estado, texto);
    m_tablaCasos->setRowCount(static_cast<int>(m_casosCache.size()));

    for (int r = 0; r < static_cast<int>(m_casosCache.size()); ++r) {
        const auto& c = m_casosCache[r];

        auto* estadoItem = new QTableWidgetItem(QString::fromStdString(c.estado));
        if (c.estado == "SANCIONADA") {
            estadoItem->setForeground(QColor("#C62828"));
            estadoItem->setBackground(QColor("#FFEBEE"));
        } else if (c.estado == "EN_DESCARGOS") {
            estadoItem->setForeground(QColor("#EF6C00"));
            estadoItem->setBackground(QColor("#FFF3E0"));
        }

        m_tablaCasos->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(c.radicado)));
        m_tablaCasos->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(c.fechaRadicacion)));
        m_tablaCasos->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(c.tipo)));
        m_tablaCasos->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(c.unidadNomenclatura)));
        m_tablaCasos->setItem(r, 4, new QTableWidgetItem(QString::fromStdString(c.terceroNombre)));
        m_tablaCasos->setItem(r, 5, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(c.valorMulta))));
        m_tablaCasos->setItem(r, 6, estadoItem);
    }
}

void PqrsWidget::onCasoSeleccionado() {
    auto sel = m_tablaCasos->selectedItems();
    if (sel.isEmpty()) {
        m_casoSeleccionadoId = 0;
        m_btnActualizarCaso->setEnabled(false);
        return;
    }

    const auto& c = m_casosCache[sel.first()->row()];
    m_casoSeleccionadoId = c.id;
    bool editable = (c.estado == "ABIERTA" || c.estado == "EN_DESCARGOS");
    m_btnActualizarCaso->setEnabled(editable);

    if (!c.fechaDescargos.empty()) {
        m_dateDescargos->setDate(QDate::fromString(QString::fromStdString(c.fechaDescargos), "yyyy-MM-dd"));
    }
}

void PqrsWidget::onRadicarNuevoClicked() {
    try {
        PqrsSancion item;
        item.tipo = m_cmbTipo->currentText().toStdString();
        int uId = m_cmbUnidad->currentData().toInt();
        if (uId > 0) item.unidadId = uId;
        item.terceroId = m_cmbTercero->currentData().toInt();
        item.fechaRadicacion = m_dateRadicacion->date().toString("yyyy-MM-dd").toStdString();
        item.asunto = m_txtAsunto->text().trimmed().toStdString();
        item.descripcion = m_txtDescripcion->toPlainText().trimmed().toStdString();

        std::string radicado = PqrsRepository::radicar(item, m_usuarioId);
        QMessageBox::information(
            this, "Caso Radicado",
            QString("Se ha generado el radicado %1 correctamente.").arg(QString::fromStdString(radicado))
        );

        m_txtAsunto->clear();
        m_txtDescripcion->clear();
        onFiltroCambiado();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void PqrsWidget::onActualizarTramiteClicked() {
    if (m_casoSeleccionadoId == 0) return;

    try {
        std::string nuevoEstado = m_cmbNuevoEstado->currentText().toStdString();
        std::string fechaDesc = m_dateDescargos->date().toString("yyyy-MM-dd").toStdString();
        int64_t multa = static_cast<int64_t>(m_spinValorMulta->value());
        std::string nota = m_txtNotaSeguimiento->toPlainText().trimmed().toStdString();
        PqrsRepository::actualizarTramite(
            m_casoSeleccionadoId, nuevoEstado, fechaDesc, multa, nota, m_usuarioId
        );

        QMessageBox::information(this, "Actuación Guardada", "El trámite se ha actualizado correctamente.");
        m_txtNotaSeguimiento->clear();
        m_spinValorMulta->setValue(0);
        onFiltroCambiado();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación de Debido Proceso", e.what());
    }
}
