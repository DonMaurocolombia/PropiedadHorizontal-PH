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


#include "AsambleasWidget.hpp"
#include "core/AsambleaEngine.hpp"
#include "core/UnidadRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDate>
#include <QTime>

AsambleasWidget::AsambleasWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTodo();
}

void AsambleasWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- BARRA SUPERIOR: INDICADOR DE QUÓRUM EN TIEMPO REAL ---
    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("<h3>Asambleas de Copropietarios y Control de Quórum (Ley 675)</h3>", this));
    topBar->addStretch();

    m_lblEstadoQuorum = new QLabel("Quórum: 0.0000%", this);
    m_lblEstadoQuorum->setStyleSheet("padding: 6px 12px; border-radius: 4px; font-weight: bold;");
    topBar->addWidget(m_lblEstadoQuorum);
    mainLayout->addLayout(topBar);

    // Selector de asamblea + Barra de progreso del quórum
    auto* quorumBar = new QHBoxLayout();
    m_cmbAsambleaActiva = new QComboBox(this);
    m_cmbAsambleaActiva->setMinimumWidth(300);
    connect(m_cmbAsambleaActiva, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AsambleasWidget::onAsambleaSeleccionada);

    m_barQuorum = new QProgressBar(this);
    m_barQuorum->setRange(0, 10000); // Precisión de 2 decimales (100.00%)
    m_barQuorum->setFormat("Quórum Alcanzado: %p%");

    auto* btnPdf = new QPushButton("Exportar Planilla Firmas / Quórum (PDF)", this);
    btnPdf->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 6px 12px;");
    connect(btnPdf, &QPushButton::clicked, this, &AsambleasWidget::onExportarPlanillaPdfClicked);

    quorumBar->addWidget(new QLabel("Asamblea:", this));
    quorumBar->addWidget(m_cmbAsambleaActiva);
    quorumBar->addWidget(m_barQuorum, 1);
    quorumBar->addWidget(btnPdf);
    mainLayout->addLayout(quorumBar);

    // --- CONTENIDO DIVIDIDO: FORMULARIOS (IZQUIERDA) + TABLA ASISTENTES (DERECHA) ---
    auto* contentLayout = new QHBoxLayout();
    auto* leftPanel = new QVBoxLayout();

    // Grupo 1: Convocar / Abrir Asamblea
    auto* openGroup = new QGroupBox("1. Crear / Abrir Sesión de Asamblea", this);
    openGroup->setFixedWidth(390);
    auto* formOpen = new QFormLayout(openGroup);

    m_cmbTipoAsamblea = new QComboBox(this);
    m_cmbTipoAsamblea->addItems({"ORDINARIA", "EXTRAORDINARIA"});

    m_dateAsamblea = new QDateEdit(QDate::currentDate(), this);
    m_dateAsamblea->setCalendarPopup(true);
    m_dateAsamblea->setDisplayFormat("yyyy-MM-dd");

    m_timeInicio = new QTimeEdit(QTime::currentTime(), this);
    m_timeInicio->setDisplayFormat("HH:mm");

    m_cmbTipoQuorum = new QComboBox(this);
    m_cmbTipoQuorum->addItem("Primera Convocatoria (> 50% Coeficientes)", 0.500001);
    m_cmbTipoQuorum->addItem("Mayoría Calificada Art. 46 (70% Coeficientes)", 0.700000);
    m_cmbTipoQuorum->addItem("Segunda Convocatoria (Cualquier número plural)", 0.000100);

    auto* btnCrear = new QPushButton("Abrir Asamblea", this);
    btnCrear->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 6px;");
    connect(btnCrear, &QPushButton::clicked, this, &AsambleasWidget::onCrearAsambleaClicked);

    formOpen->addRow("Tipo Asamblea:", m_cmbTipoAsamblea);
    formOpen->addRow("Fecha:", m_dateAsamblea);
    formOpen->addRow("Hora Inicio:", m_timeInicio);
    formOpen->addRow("Regla Quórum:", m_cmbTipoQuorum);
    formOpen->addRow(btnCrear);

    // Grupo 2: Registro de Ingreso a Asamblea
    auto* regGroup = new QGroupBox("2. Registro de Asistencia al Quórum", this);
    regGroup->setFixedWidth(390);
    auto* formReg = new QFormLayout(regGroup);

    m_cmbUnidad = new QComboBox(this);
    connect(m_cmbUnidad, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AsambleasWidget::onUnidadAsistenteCambiada);

    m_txtNombreAsistente = new QLineEdit(this);
    m_txtNombreAsistente->setPlaceholderText("Nombre completo quien firma asistencia");

    m_chkEsApoderado = new QCheckBox("Asiste mediante Poder Escrito (Apoderado)", this);

    m_btnRegistrarAsistencia = new QPushButton("Registrar en Quórum", this);
    m_btnRegistrarAsistencia->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 7px;");
    connect(m_btnRegistrarAsistencia, &QPushButton::clicked, this, &AsambleasWidget::onRegistrarAsistenciaClicked);

    formReg->addRow("Unidad Privada *:", m_cmbUnidad);
    formReg->addRow("Nombre Asistente *:", m_txtNombreAsistente);
    formReg->addRow("", m_chkEsApoderado);
    formReg->addRow(m_btnRegistrarAsistencia);

    leftPanel->addWidget(openGroup);
    leftPanel->addWidget(regGroup);
    leftPanel->addStretch();

    // Panel Derecho: Tabla de Asistentes Registrados
    auto* rightPanel = new QVBoxLayout();
    m_tablaAsistencia = new QTableWidget(this);
    m_tablaAsistencia->setColumnCount(6);
    m_tablaAsistencia->setHorizontalHeaderLabels({
        "Unidad", "Propietario Titular", "Asistente Registrado", "Calidad", "Coeficiente (%)", "Hora Ingreso"
    });
    m_tablaAsistencia->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaAsistencia->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaAsistencia->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto* bottomBtns = new QHBoxLayout();
    auto* btnQuitar = new QPushButton("Retirar Unidad del Quórum", this);
    m_btnCerrarAsamblea = new QPushButton("Cerrar Asamblea Definitivamente", this);

    connect(btnQuitar, &QPushButton::clicked, this, &AsambleasWidget::onQuitarAsistenciaClicked);
    connect(m_btnCerrarAsamblea, &QPushButton::clicked, this, &AsambleasWidget::onCerrarAsambleaClicked);

    bottomBtns->addWidget(btnQuitar);
    bottomBtns->addStretch();
    bottomBtns->addWidget(m_btnCerrarAsamblea);

    rightPanel->addWidget(m_tablaAsistencia);
    rightPanel->addLayout(bottomBtns);

    contentLayout->addLayout(leftPanel);
    contentLayout->addLayout(rightPanel, 1);
    mainLayout->addLayout(contentLayout);
}

void AsambleasWidget::recargarTodo() {
    cargarCombos();
    onAsambleaSeleccionada();
}

void AsambleasWidget::cargarCombos() {
    int asamActual = m_cmbAsambleaActiva->currentData().toInt();
    m_cmbAsambleaActiva->blockSignals(true);
    m_cmbAsambleaActiva->clear();

    m_asambleasCache = AsambleaEngine::listarAsambleas();
    for (const auto& a : m_asambleasCache) {
        QString label = QString("%1 - %2 (%3)")
            .arg(QString::fromStdString(a.fecha))
            .arg(QString::fromStdString(a.tipo))
            .arg(QString::fromStdString(a.estado));
        m_cmbAsambleaActiva->addItem(label, a.id);
    }

    int idxA = m_cmbAsambleaActiva->findData(asamActual);
    if (idxA >= 0) m_cmbAsambleaActiva->setCurrentIndex(idxA);
    m_cmbAsambleaActiva->blockSignals(false);

    m_cmbUnidad->blockSignals(true);
    m_cmbUnidad->clear();
    m_cmbUnidad->addItem("-- Seleccione Unidad Privada --", 0);
    m_unidadesCache = UnidadRepository::listarTodas();
    for (const auto& u : m_unidadesCache) {
        if (u.estado != "ACTIVA") continue;
        QString label = QString("%1 - %2 (Coef: %3%)")
            .arg(QString::fromStdString(u.torreBloque))
            .arg(QString::fromStdString(u.nomenclatura))
            .arg(QString::number(u.coeficiente * 100.0, 'f', 4));
        m_cmbUnidad->addItem(label, u.id);
    }
    m_cmbUnidad->blockSignals(false);
}

void AsambleasWidget::onUnidadAsistenteCambiada(int) {
    int unidadId = m_cmbUnidad->currentData().toInt();
    m_txtNombreAsistente->clear();
    m_chkEsApoderado->setChecked(false);

    for (const auto& u : m_unidadesCache) {
        if (u.id == unidadId) {
            m_txtNombreAsistente->setText(QString::fromStdString(u.propietarioNombre));
            break;
        }
    }
}

void AsambleasWidget::onCrearAsambleaClicked() {
    try {
        Asamblea a;
        a.tipo = m_cmbTipoAsamblea->currentText().toStdString();
        a.fecha = m_dateAsamblea->date().toString("yyyy-MM-dd").toStdString();
        a.horaInicio = m_timeInicio->time().toString("HH:mm").toStdString();
        a.quorumRequerido = m_cmbTipoQuorum->currentData().toDouble();

        AsambleaEngine::crearAsamblea(a, m_usuarioId);
        QMessageBox::information(this, "Asamblea Abierta", "Sesión de asamblea creada e iniciada.");
        recargarTodo();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Validación", e.what());
    }
}

void AsambleasWidget::onAsambleaSeleccionada() {
    refrescarTablaAsistencia();
}

void AsambleasWidget::refrescarTablaAsistencia() {
    int idx = m_cmbAsambleaActiva->currentIndex();
    m_tablaAsistencia->setRowCount(0);

    if (idx < 0 || idx >= static_cast<int>(m_asambleasCache.size())) {
        m_lblEstadoQuorum->setText("Sin asamblea seleccionada");
        m_barQuorum->setValue(0);
        return;
    }

    // Recargar datos frescos de la asamblea desde BD
    m_asambleasCache = AsambleaEngine::listarAsambleas();
    const auto& asam = m_asambleasCache[idx];

    double alcanzadoPct = asam.quorumAlcanzado * 100.0;
    double requeridoPct = asam.quorumRequerido * 100.0;
    bool cumple = (asam.quorumAlcanzado >= asam.quorumRequerido);

    m_barQuorum->setValue(static_cast<int>(alcanzadoPct * 100.0));

    if (cumple) {
        m_lblEstadoQuorum->setText(
            QString("QUÓRUM VÁLIDO: %1% (Mínimo: %2%)").arg(alcanzadoPct, 0, 'f', 4).arg(requeridoPct, 0, 'f', 2)
        );
        m_lblEstadoQuorum->setStyleSheet(
            "background-color: #E8F5E9; color: #2E7D32; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    } else {
        m_lblEstadoQuorum->setText(
            QString("QUÓRUM INSUFICIENTE: %1% / %2%").arg(alcanzadoPct, 0, 'f', 4).arg(requeridoPct, 0, 'f', 2)
        );

        m_lblEstadoQuorum->setStyleSheet(
            "background-color: #FFF3E0; color: #EF6C00; padding: 6px 12px; border-radius: 4px; font-weight: bold;"
        );
    }

    bool abierta = (asam.estado != "CERRADA");
    m_btnRegistrarAsistencia->setEnabled(abierta);
    m_btnCerrarAsamblea->setEnabled(abierta);
    m_asistenciaCache = AsambleaEngine::listarAsistencia(asam.id);
    m_tablaAsistencia->setRowCount(static_cast<int>(m_asistenciaCache.size()));
    for (int r = 0; r < static_cast<int>(m_asistenciaCache.size()); ++r) {
        const auto& item = m_asistenciaCache[r];
        m_tablaAsistencia->setItem(r, 0, new QTableWidgetItem(
            QString::fromStdString(item.torreBloque + " - " + item.unidadNomenclatura)
        ));
        
        m_tablaAsistencia->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(item.propietarioNombre)));
        m_tablaAsistencia->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(item.asistenteNombre)));
        m_tablaAsistencia->setItem(r, 3, new QTableWidgetItem(item.esApoderado ? "Apoderado" : "Propietario"));
        m_tablaAsistencia->setItem(r, 4, new QTableWidgetItem(QString::number(item.coeficienteRepresentado * 100.0, 'f', 4) + "%"));
        m_tablaAsistencia->setItem(r, 5, new QTableWidgetItem(QString::fromStdString(item.horaRegistro)));
    }
}

void AsambleasWidget::onRegistrarAsistenciaClicked() {
    int asambleaId = m_cmbAsambleaActiva->currentData().toInt();
    int unidadId = m_cmbUnidad->currentData().toInt();
    try {
        AsambleaEngine::registrarAsistencia(
            asambleaId,
            unidadId,
            m_txtNombreAsistente->text().trimmed().toStdString(),
            m_chkEsApoderado->isChecked(),
            m_usuarioId
        );

        m_cmbUnidad->setCurrentIndex(0);
        m_txtNombreAsistente->clear();
        m_chkEsApoderado->setChecked(false);
        refrescarTablaAsistencia();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Registro de Quórum", e.what());
    }
}

void AsambleasWidget::onQuitarAsistenciaClicked() {
    auto sel = m_tablaAsistencia->selectedItems();
    if (sel.isEmpty()) return;

    int asistenciaId = m_asistenciaCache[sel.first()->row()].id;
    AsambleaEngine::eliminarAsistencia(asistenciaId, m_usuarioId);
    refrescarTablaAsistencia();
}

void AsambleasWidget::onCerrarAsambleaClicked() {
    int asambleaId = m_cmbAsambleaActiva->currentData().toInt();
    if (asambleaId <= 0) return;

    auto reply = QMessageBox::question(
        this, "Cerrar Asamblea",
        "¿Está seguro de cerrar definitivamente esta asamblea? No se podrán agregar más asistentes.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        AsambleaEngine::cambiarEstadoAsamblea(asambleaId, "CERRADA", m_usuarioId);
        recargarTodo();
    }
}

void AsambleasWidget::onExportarPlanillaPdfClicked() {
    int asambleaId = m_cmbAsambleaActiva->currentData().toInt();
    if (asambleaId <= 0) return;

    QString ruta = QFileDialog::getSaveFileName(
        this, "Guardar Planilla de Quórum", "Planilla_Quorum_Asamblea.pdf", "PDF (*.pdf)"
    );

    if (ruta.isEmpty()) return;

    try {
        AsambleaEngine::exportarPlanillaQuorumPDF(asambleaId, ruta.toStdString());
        QMessageBox::information(this, "PDF Generado", "Planilla oficial de quórum y firmas guardada.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error PDF", e.what());
    }
}
