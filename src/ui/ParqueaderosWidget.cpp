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


#include "ParqueaderosWidget.hpp"
#include "core/ParqueaderoRepository.hpp"
#include "core/UnidadRepository.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QLocale>

ParqueaderosWidget::ParqueaderosWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTodo();
}

void ParqueaderosWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("<h3>Administración de Parqueaderos, Bodegas y Censo Vehicular</h3>", this));

    auto* tabs = new QTabWidget(this);
    tabs->addTab(crearTabParqueaderos(), "1. Parqueaderos");
    tabs->addTab(crearTabBodegas(), "2. Bodegas / Depósitos");
    tabs->addTab(crearTabVehiculos(), "3. Vehículos y Placas");

    mainLayout->addWidget(tabs);
}

QWidget* ParqueaderosWidget::crearTabParqueaderos() {
    auto* w = new QWidget(this);
    auto* layout = new QHBoxLayout(w);

    auto* left = new QVBoxLayout();
    m_txtBuscarParq = new QLineEdit(this);
    m_txtBuscarParq->setPlaceholderText("Buscar por número de parqueadero, sótano o unidad...");

    m_tablaParq = new QTableWidget(this);
    m_tablaParq->setColumnCount(6);
    m_tablaParq->setHorizontalHeaderLabels({
        "Número", "Nivel/Sótano", "Régimen / Tenencia", "Unidad Asignada", "Canon Mensual", "Estado"
    });
    m_tablaParq->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaParq->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaParq->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    left->addWidget(m_txtBuscarParq);
    left->addWidget(m_tablaParq);

    auto* formGroup = new QGroupBox("Ficha de Parqueadero", this);
    formGroup->setFixedWidth(380);
    auto* form = new QFormLayout(formGroup);

    m_txtNumParq = new QLineEdit(this);
    m_txtNumParq->setPlaceholderText("Ej: P-101, S1-15");

    m_txtNivelParq = new QLineEdit(this);
    m_txtNivelParq->setPlaceholderText("Ej: Sótano 1, Plataforma");

    m_cmbTenenciaParq = new QComboBox(this);
    m_cmbTenenciaParq->addItems({"USO_EXCLUSIVO", "PRIVADO", "COMUN_ALQUILER", "VISITANTES"});

    m_txtMatParq = new QLineEdit(this);
    m_txtMatParq->setPlaceholderText("Solo si es PRIVADO");

    m_spinCoefParq = new QDoubleSpinBox(this);
    m_spinCoefParq->setRange(0.0, 100.0);
    m_spinCoefParq->setDecimals(4);
    m_spinCoefParq->setSuffix(" %");

    m_cmbUnidadParq = new QComboBox(this);

    m_spinCanonParq = new QDoubleSpinBox(this);
    m_spinCanonParq->setRange(0.0, 50000000.0);
    m_spinCanonParq->setDecimals(0);
    m_spinCanonParq->setPrefix("$ ");
    m_spinCanonParq->setGroupSeparatorShown(true);

    m_cmbEstadoParq = new QComboBox(this);
    m_cmbEstadoParq->addItems({"DISPONIBLE", "OCUPADO", "MANTENIMIENTO"});

    // Habilitar campos según el tipo de tenencia
    auto actualizarCamposParq = [this](const QString& tenencia) {
        bool esPrivado = (tenencia == "PRIVADO");
        bool esAlquiler = (tenencia == "COMUN_ALQUILER");
        bool esVisitantes = (tenencia == "VISITANTES");

        m_txtMatParq->setEnabled(esPrivado);
        m_spinCoefParq->setEnabled(esPrivado);
        m_spinCanonParq->setEnabled(esAlquiler);
        m_cmbUnidadParq->setEnabled(!esVisitantes);
        if (esVisitantes) m_cmbUnidadParq->setCurrentIndex(0);
    };
    connect(m_cmbTenenciaParq, &QComboBox::currentTextChanged, this, actualizarCamposParq);
    actualizarCamposParq(m_cmbTenenciaParq->currentText());

    form->addRow("Número Cupo *:", m_txtNumParq);
    form->addRow("Nivel / Sótano:", m_txtNivelParq);
    form->addRow("Régimen Tenencia:", m_cmbTenenciaParq);
    form->addRow("Matrícula Inmob.:", m_txtMatParq);
    form->addRow("Coeficiente Propio:", m_spinCoefParq);
    form->addRow("Unidad Asignada:", m_cmbUnidadParq);
    form->addRow("Canon Alquiler:", m_spinCanonParq);
    form->addRow("Estado:", m_cmbEstadoParq);

    auto* btns = new QHBoxLayout();
    auto* btnNuevo = new QPushButton("Nuevo", this);
    auto* btnGuardar = new QPushButton("Guardar", this);
    auto* btnEliminar = new QPushButton("Eliminar", this);
    btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");

    btns->addWidget(btnNuevo);
    btns->addWidget(btnGuardar);
    btns->addWidget(btnEliminar);
    form->addRow(btns);

    // Conexiones CRUD Parqueaderos
    auto refrescarTablaParq = [this]() {
        QLocale col(QLocale::Spanish, QLocale::Colombia);
        m_parqCache = ParqueaderoRepository::listarParqueaderos(m_txtBuscarParq->text().toStdString());
        m_tablaParq->setRowCount(static_cast<int>(m_parqCache.size()));
        for (int r = 0; r < static_cast<int>(m_parqCache.size()); ++r) {
            const auto& p = m_parqCache[r];
            m_tablaParq->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(p.numero)));
            m_tablaParq->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(p.nivelSotano)));
            m_tablaParq->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(p.tipoTenencia)));
            m_tablaParq->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(p.unidadNomenclatura)));
            m_tablaParq->setItem(r, 4, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(p.canonMensual))));
            m_tablaParq->setItem(r, 5, new QTableWidgetItem(QString::fromStdString(p.estado)));
        }
    };

    connect(m_txtBuscarParq, &QLineEdit::textChanged, this, refrescarTablaParq);

    connect(m_tablaParq, &QTableWidget::itemSelectionChanged, this, [this]() {
        auto sel = m_tablaParq->selectedItems();
        if (sel.isEmpty()) return;
        const auto& p = m_parqCache[sel.first()->row()];
        m_parqEditandoId = p.id;
        m_txtNumParq->setText(QString::fromStdString(p.numero));
        m_txtNivelParq->setText(QString::fromStdString(p.nivelSotano));
        m_cmbTenenciaParq->setCurrentText(QString::fromStdString(p.tipoTenencia));
        m_txtMatParq->setText(QString::fromStdString(p.matriculaInmobiliaria));
        m_spinCoefParq->setValue(p.coeficiente * 100.0);
        int idxU = p.unidadAsignadaId.has_value() ? m_cmbUnidadParq->findData(p.unidadAsignadaId.value()) : 0;
        m_cmbUnidadParq->setCurrentIndex(idxU >= 0 ? idxU : 0);
        m_spinCanonParq->setValue(static_cast<double>(p.canonMensual));
        m_cmbEstadoParq->setCurrentText(QString::fromStdString(p.estado));
    });

    connect(btnNuevo, &QPushButton::clicked, this, [this]() {
        m_parqEditandoId = 0;
        m_tablaParq->clearSelection();
        m_txtNumParq->clear();
        m_txtNivelParq->clear();
        m_cmbTenenciaParq->setCurrentIndex(0);
        m_txtMatParq->clear();
        m_spinCoefParq->setValue(0.0);
        m_cmbUnidadParq->setCurrentIndex(0);
        m_spinCanonParq->setValue(0.0);
        m_cmbEstadoParq->setCurrentIndex(0);
    });

    connect(btnGuardar, &QPushButton::clicked, this, [this, refrescarTablaParq, btnNuevo]() {
        try {
            Parqueadero p;
            p.id = m_parqEditandoId;
            p.numero = m_txtNumParq->text().trimmed().toStdString();
            p.nivelSotano = m_txtNivelParq->text().trimmed().toStdString();
            p.tipoTenencia = m_cmbTenenciaParq->currentText().toStdString();
            p.matriculaInmobiliaria = m_txtMatParq->text().trimmed().toStdString();
            p.coeficiente = m_spinCoefParq->value() / 100.0;
            int uId = m_cmbUnidadParq->currentData().toInt();
            if (uId > 0) p.unidadAsignadaId = uId;
            p.canonMensual = static_cast<int64_t>(m_spinCanonParq->value());
            p.estado = m_cmbEstadoParq->currentText().toStdString();

            ParqueaderoRepository::guardarParqueadero(p, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Parqueadero guardado correctamente.");
            btnNuevo->click();
            refrescarTablaParq();
            cargarCombosUnidadesYParqueaderos();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Validación", e.what());
        }
    });

    connect(btnEliminar, &QPushButton::clicked, this, [this, refrescarTablaParq, btnNuevo]() {
        if (m_parqEditandoId == 0) return;
        try {
            ParqueaderoRepository::eliminarParqueadero(m_parqEditandoId, m_usuarioId);
            btnNuevo->click();
            refrescarTablaParq();
            cargarCombosUnidadesYParqueaderos();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    });

    layout->addLayout(left, 1);
    layout->addWidget(formGroup);
    return w;
}

QWidget* ParqueaderosWidget::crearTabBodegas() {
    auto* w = new QWidget(this);
    auto* layout = new QHBoxLayout(w);

    auto* left = new QVBoxLayout();
    m_txtBuscarBod = new QLineEdit(this);
    m_txtBuscarBod->setPlaceholderText("Buscar por número de bodega o unidad...");

    m_tablaBod = new QTableWidget(this);
    m_tablaBod->setColumnCount(5);
    m_tablaBod->setHorizontalHeaderLabels({
        "Número", "Ubicación", "Régimen / Tenencia", "Unidad Asignada", "Canon Mensual"
    });
    m_tablaBod->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaBod->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaBod->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    left->addWidget(m_txtBuscarBod);
    left->addWidget(m_tablaBod);

    auto* formGroup = new QGroupBox("Ficha de Bodega / Depósito", this);
    formGroup->setFixedWidth(380);
    auto* form = new QFormLayout(formGroup);

    m_txtNumBod = new QLineEdit(this);
    m_txtNumBod->setPlaceholderText("Ej: B-01, DEP-12");

    m_txtUbicacionBod = new QLineEdit(this);
    m_txtUbicacionBod->setPlaceholderText("Ej: Sótano 1 Torre A");

    m_cmbTenenciaBod = new QComboBox(this);
    m_cmbTenenciaBod->addItems({"USO_EXCLUSIVO", "PRIVADA", "COMUN_ALQUILER"});
    m_txtMatBod = new QLineEdit(this);
    m_spinCoefBod = new QDoubleSpinBox(this);
    m_spinCoefBod->setRange(0.0, 100.0);
    m_spinCoefBod->setDecimals(4);
    m_spinCoefBod->setSuffix(" %");
    m_cmbUnidadBod = new QComboBox(this);
    m_spinCanonBod = new QDoubleSpinBox(this);
    m_spinCanonBod->setRange(0.0, 50000000.0);
    m_spinCanonBod->setDecimals(0);
    m_spinCanonBod->setPrefix("$ ");
    m_spinCanonBod->setGroupSeparatorShown(true);

    form->addRow("Número Bodega *:", m_txtNumBod);
    form->addRow("Ubicación:", m_txtUbicacionBod);
    form->addRow("Régimen Tenencia:", m_cmbTenenciaBod);
    form->addRow("Matrícula Inmob.:", m_txtMatBod);
    form->addRow("Coeficiente Propio:", m_spinCoefBod);
    form->addRow("Unidad Asignada:", m_cmbUnidadBod);
    form->addRow("Canon Alquiler:", m_spinCanonBod);

    auto* btns = new QHBoxLayout();
    auto* btnNuevo = new QPushButton("Nuevo", this);
    auto* btnGuardar = new QPushButton("Guardar", this);
    auto* btnEliminar = new QPushButton("Eliminar", this);
    
    btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");
    btns->addWidget(btnNuevo);
    btns->addWidget(btnGuardar);
    btns->addWidget(btnEliminar);
    form->addRow(btns);
    auto refrescarTablaBod = [this]() {
        QLocale col(QLocale::Spanish, QLocale::Colombia);
        m_bodCache = ParqueaderoRepository::listarBodegas(m_txtBuscarBod->text().toStdString());
        m_tablaBod->setRowCount(static_cast<int>(m_bodCache.size()));
        for (int r = 0; r < static_cast<int>(m_bodCache.size()); ++r) {
            const auto& b = m_bodCache[r];
            m_tablaBod->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(b.numero)));
            m_tablaBod->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(b.ubicacion)));
            m_tablaBod->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(b.tipoTenencia)));
            m_tablaBod->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(b.unidadNomenclatura)));
            m_tablaBod->setItem(r, 4, new QTableWidgetItem("$ " + col.toString(static_cast<qlonglong>(b.canonMensual))));
        }
    };

    connect(m_txtBuscarBod, &QLineEdit::textChanged, this, refrescarTablaBod);
    connect(m_tablaBod, &QTableWidget::itemSelectionChanged, this, [this]() {
        auto sel = m_tablaBod->selectedItems();
        if (sel.isEmpty()) return;

        const auto& b = m_bodCache[sel.first()->row()];
        m_bodEditandoId = b.id;
        m_txtNumBod->setText(QString::fromStdString(b.numero));
        m_txtUbicacionBod->setText(QString::fromStdString(b.ubicacion));
        m_cmbTenenciaBod->setCurrentText(QString::fromStdString(b.tipoTenencia));
        m_txtMatBod->setText(QString::fromStdString(b.matriculaInmobiliaria));
        m_spinCoefBod->setValue(b.coeficiente * 100.0);
        int idxU = b.unidadAsignadaId.has_value() ? m_cmbUnidadBod->findData(b.unidadAsignadaId.value()) : 0;
        m_cmbUnidadBod->setCurrentIndex(idxU >= 0 ? idxU : 0);
        m_spinCanonBod->setValue(static_cast<int64_t>(b.canonMensual));
    });

    connect(btnNuevo, &QPushButton::clicked, this, [this]() {
        m_bodEditandoId = 0;
        m_tablaBod->clearSelection();
        m_txtNumBod->clear();
        m_txtUbicacionBod->clear();
        m_cmbTenenciaBod->setCurrentIndex(0);
        m_txtMatBod->clear();
        m_spinCoefBod->setValue(0.0);
        m_cmbUnidadBod->setCurrentIndex(0);
        m_spinCanonBod->setValue(0.0);
    });

    connect(btnGuardar, &QPushButton::clicked, this, [this, refrescarTablaBod, btnNuevo]() {
        try {
            Bodega b;
            b.id = m_bodEditandoId;
            b.numero = m_txtNumBod->text().trimmed().toStdString();
            b.ubicacion = m_txtUbicacionBod->text().trimmed().toStdString();
            b.tipoTenencia = m_cmbTenenciaBod->currentText().toStdString();
            b.matriculaInmobiliaria = m_txtMatBod->text().trimmed().toStdString();
            b.coeficiente = m_spinCoefBod->value() / 100.0;
            int uId = m_cmbUnidadBod->currentData().toInt();
            if (uId > 0) b.unidadAsignadaId = uId;

            b.canonMensual = static_cast<int64_t>(m_spinCanonBod->value());
            ParqueaderoRepository::guardarBodega(b, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Bodega guardada correctamente.");
            btnNuevo->click();
            refrescarTablaBod();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Validación", e.what());
        }
    });

    connect(btnEliminar, &QPushButton::clicked, this, [this, refrescarTablaBod, btnNuevo]() {
        if (m_bodEditandoId == 0) return;

        ParqueaderoRepository::eliminarBodega(m_bodEditandoId, m_usuarioId);
        btnNuevo->click();
        refrescarTablaBod();
    });

    layout->addLayout(left, 1);
    layout->addWidget(formGroup);
    return w;
}

QWidget* ParqueaderosWidget::crearTabVehiculos() {
    auto* w = new QWidget(this);
    auto* layout = new QHBoxLayout(w);
    auto* left = new QVBoxLayout();
    m_txtBuscarVeh = new QLineEdit(this);
    m_txtBuscarVeh->setPlaceholderText("Buscar por placa, apartamento o marca...");
    m_tablaVeh = new QTableWidget(this);
    m_tablaVeh->setColumnCount(6);
    m_tablaVeh->setHorizontalHeaderLabels({
        "Placa", "Tipo", "Marca / Modelo", "Color", "Unidad", "Parqueadero"
    });

    m_tablaVeh->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaVeh->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaVeh->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    left->addWidget(m_txtBuscarVeh);
    left->addWidget(m_tablaVeh);
    auto* formGroup = new QGroupBox("Registro de Vehículo (Control Portería)", this);
    formGroup->setFixedWidth(380);
    auto* form = new QFormLayout(formGroup);
    m_cmbUnidadVeh = new QComboBox(this);
    m_cmbParqVeh = new QComboBox(this);
    m_txtPlacaVeh = new QLineEdit(this);
    m_txtPlacaVeh->setPlaceholderText("Ej: ABC123");
    m_cmbTipoVeh = new QComboBox(this);
    m_cmbTipoVeh->addItems({"AUTOMOVIL", "CAMIONETA", "MOTOCICLETA", "BICICLETA"});
    m_txtMarcaVeh = new QLineEdit(this);
    m_txtMarcaVeh->setPlaceholderText("Ej: Mazda CX-30");
    m_txtColorVeh = new QLineEdit(this);
    m_txtColorVeh->setPlaceholderText("Ej: Gris Machine");

    form->addRow("Unidad Privada *:", m_cmbUnidadVeh);
    form->addRow("Parqueadero:", m_cmbParqVeh);
    form->addRow("Placa / Serie *:", m_txtPlacaVeh);
    form->addRow("Tipo Vehículo:", m_cmbTipoVeh);
    form->addRow("Marca / Modelo:", m_txtMarcaVeh);
    form->addRow("Color:", m_txtColorVeh);

    auto* btns = new QHBoxLayout();
    auto* btnNuevo = new QPushButton("Nuevo", this);
    auto* btnGuardar = new QPushButton("Guardar", this);
    auto* btnEliminar = new QPushButton("Eliminar", this);
    btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");
    btns->addWidget(btnNuevo);
    btns->addWidget(btnGuardar);
    btns->addWidget(btnEliminar);
    form->addRow(btns);
    auto refrescarTablaVeh = [this]() {
        m_vehCache = ParqueaderoRepository::listarVehiculos(m_txtBuscarVeh->text().toStdString());
        m_tablaVeh->setRowCount(static_cast<int>(m_vehCache.size()));
        for (int r = 0; r < static_cast<int>(m_vehCache.size()); ++r) {
            const auto& v = m_vehCache[r];
            m_tablaVeh->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(v.placa)));
            m_tablaVeh->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(v.tipo)));
            m_tablaVeh->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(v.marcaModelo)));
            m_tablaVeh->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(v.color)));
            m_tablaVeh->setItem(r, 4, new QTableWidgetItem(QString::fromStdString(v.unidadNomenclatura)));
            m_tablaVeh->setItem(r, 5, new QTableWidgetItem(QString::fromStdString(v.parqueaderoNumero)));
        }
    };

    connect(m_txtBuscarVeh, &QLineEdit::textChanged, this, refrescarTablaVeh);
    connect(m_tablaVeh, &QTableWidget::itemSelectionChanged, this, [this]() {
        auto sel = m_tablaVeh->selectedItems();
        if (sel.isEmpty()) return;

        const auto& v = m_vehCache[sel.first()->row()];
        m_vehEditandoId = v.id;
        int idxU = m_cmbUnidadVeh->findData(v.unidadId);
        m_cmbUnidadVeh->setCurrentIndex(idxU >= 0 ? idxU : 0);
        int idxP = v.parqueaderoId.has_value() ? m_cmbParqVeh->findData(v.parqueaderoId.value()) : 0;
        m_cmbParqVeh->setCurrentIndex(idxP >= 0 ? idxP : 0);
        m_txtPlacaVeh->setText(QString::fromStdString(v.placa));
        m_cmbTipoVeh->setCurrentText(QString::fromStdString(v.tipo));
        m_txtMarcaVeh->setText(QString::fromStdString(v.marcaModelo));
        m_txtColorVeh->setText(QString::fromStdString(v.color));
    });

    connect(btnNuevo, &QPushButton::clicked, this, [this]() {
        m_vehEditandoId = 0;
        m_tablaVeh->clearSelection();
        m_cmbUnidadVeh->setCurrentIndex(0);
        m_cmbParqVeh->setCurrentIndex(0);
        m_txtPlacaVeh->clear();
        m_cmbTipoVeh->setCurrentIndex(0);
        m_txtMarcaVeh->clear();
        m_txtColorVeh->clear();
    });

    connect(btnGuardar, &QPushButton::clicked, this, [this, refrescarTablaVeh, btnNuevo]() {
        try {
            Vehiculo v;
            v.id = m_vehEditandoId;
            v.unidadId = m_cmbUnidadVeh->currentData().toInt();
            int pId = m_cmbParqVeh->currentData().toInt();
            if (pId > 0) v.parqueaderoId = pId;

            v.placa = m_txtPlacaVeh->text().toStdString();
            v.tipo = m_cmbTipoVeh->currentText().toStdString();
            v.marcaModelo = m_txtMarcaVeh->text().trimmed().toStdString();
            v.color = m_txtColorVeh->text().trimmed().toStdString();
            ParqueaderoRepository::guardarVehiculo(v, m_usuarioId);
            QMessageBox::information(this, "Éxito", "Vehículo registrado correctamente.");
            btnNuevo->click();
            refrescarTablaVeh();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Validación", e.what());
        }
    });

    connect(btnEliminar, &QPushButton::clicked, this, [this, refrescarTablaVeh, btnNuevo]() {
        if (m_vehEditandoId == 0) return;

        ParqueaderoRepository::eliminarVehiculo(m_vehEditandoId, m_usuarioId);
        btnNuevo->click();
        refrescarTablaVeh();
    });

    layout->addLayout(left, 1);
    layout->addWidget(formGroup);

    return w;
}

void ParqueaderosWidget::cargarCombosUnidadesYParqueaderos() {
    auto unidades = UnidadRepository::listarTodas();
    auto llenarUnidades = [&](QComboBox* cmb, const QString& textoDefault) {
        int actual = cmb->currentData().toInt();
        cmb->clear();
        cmb->addItem(textoDefault, 0);
        for (const auto& u : unidades) {
            cmb->addItem(QString::fromStdString(u.torreBloque + " - " + u.nomenclatura), u.id);
        }
        
        int idx = cmb->findData(actual);
        cmb->setCurrentIndex(idx >= 0 ? idx : 0);
    };

    llenarUnidades(m_cmbUnidadParq, "-- Sin Asignar --");
    llenarUnidades(m_cmbUnidadBod, "-- Sin Asignar --");
    llenarUnidades(m_cmbUnidadVeh, "-- Seleccione Unidad * --");
    int parqActual = m_cmbParqVeh->currentData().toInt();
    m_cmbParqVeh->clear();
    m_cmbParqVeh->addItem("-- Sin Parqueadero Fijo --", 0);
    for (const auto& p : ParqueaderoRepository::listarParqueaderos()) {
        m_cmbParqVeh->addItem(QString::fromStdString(p.numero + " (" + p.nivelSotano + ")"), p.id);
    }

    int idxP = m_cmbParqVeh->findData(parqActual);
    m_cmbParqVeh->setCurrentIndex(idxP >= 0 ? idxP : 0);
}

void ParqueaderosWidget::recargarTodo() {
    cargarCombosUnidadesYParqueaderos();
    emit m_txtBuscarParq->textChanged(m_txtBuscarParq->text());
    emit m_txtBuscarBod->textChanged(m_txtBuscarBod->text());
    emit m_txtBuscarVeh->textChanged(m_txtBuscarVeh->text());
}
