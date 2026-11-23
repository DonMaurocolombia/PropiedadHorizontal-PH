// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PermisosPaginasWidget.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

PermisosPaginasWidget::PermisosPaginasWidget(int usuarioId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioId) {
    setupUi();
}

void PermisosPaginasWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(
        "<h3>Permisos de acceso a páginas</h3>"
        "Los cambios se guardan al activar o desactivar cada página. "
        "El perfil Administrador siempre conserva acceso total.", this));

    m_cmbRol = new QComboBox(this);
    layout->addWidget(new QLabel("Perfil que se configurará:", this));
    layout->addWidget(m_cmbRol);

    m_tablaPermisos = new QTableWidget(this);
    m_tablaPermisos->setColumnCount(3);
    m_tablaPermisos->setHorizontalHeaderLabels({"N.º", "Permitir acceso", "Página"});
    m_tablaPermisos->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaPermisos->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tablaPermisos->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tablaPermisos->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    connect(m_tablaPermisos, &QTableWidget::itemChanged,
            this, &PermisosPaginasWidget::onPermisoCambiado);
    connect(m_tablaPermisos, &QTableWidget::itemSelectionChanged,
            this, &PermisosPaginasWidget::actualizarBotonesOrden);
    layout->addWidget(m_tablaPermisos, 1);

    auto* controlesOrden = new QHBoxLayout();
    m_btnSubir = new QPushButton("Subir", this);
    m_btnBajar = new QPushButton("Bajar", this);
    m_btnGuardarOrden = new QPushButton("Guardar orden", this);
    m_btnSubir->setEnabled(false);
    m_btnBajar->setEnabled(false);
    m_btnGuardarOrden->setEnabled(false);
    connect(m_btnSubir, &QPushButton::clicked,
            this, &PermisosPaginasWidget::onSubirClicked);
    connect(m_btnBajar, &QPushButton::clicked,
            this, &PermisosPaginasWidget::onBajarClicked);
    connect(m_btnGuardarOrden, &QPushButton::clicked,
            this, &PermisosPaginasWidget::onGuardarOrdenClicked);
    controlesOrden->addWidget(m_btnSubir);
    controlesOrden->addWidget(m_btnBajar);
    controlesOrden->addWidget(m_btnGuardarOrden);
    controlesOrden->addStretch();
    layout->addLayout(controlesOrden);

    m_lblEstado = new QLabel(
        "ADMIN no aparece en la lista porque sus permisos son permanentes.", this);
    layout->addWidget(m_lblEstado);

    connect(m_cmbRol, &QComboBox::currentIndexChanged,
            this, &PermisosPaginasWidget::cargarPermisos);

    try {
        const auto roles = PermisosPaginasRepository::obtenerRolesConfigurables();
        for (const auto& rol : roles) {
            m_cmbRol->addItem(QString::fromStdString(rol.nombre), rol.id);
        }
        if (roles.empty()) {
            m_cmbRol->setEnabled(false);
            m_lblEstado->setText("No hay perfiles configurables.");
        } else {
            cargarPermisos();
        }
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Error al cargar perfiles", error.what());
    }
}

void PermisosPaginasWidget::cargarPermisos() {
    const int rolId = m_cmbRol->currentData().toInt();
    if (rolId <= 0) {
        return;
    }

    if (m_ordenPendiente && m_rolIdCargado != 0 && rolId != m_rolIdCargado) {
        const auto respuesta = QMessageBox::question(
            this, "Orden sin guardar",
            "¿Desea guardar el nuevo orden antes de cambiar de perfil?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
            QMessageBox::Yes);
        if (respuesta == QMessageBox::Cancel
            || (respuesta == QMessageBox::Yes && !guardarOrden())) {
            const QSignalBlocker blocker(m_cmbRol);
            m_cmbRol->setCurrentIndex(m_cmbRol->findData(m_rolIdCargado));
            return;
        }
    }

    m_cargando = true;
    try {
        const auto permisos = PermisosPaginasRepository::obtenerPermisosPorRol(rolId);
        m_tablaPermisos->setRowCount(static_cast<int>(permisos.size()));
        for (int row = 0; row < static_cast<int>(permisos.size()); ++row) {
            const auto& permiso = permisos[static_cast<std::size_t>(row)];
            auto* ordenItem = new QTableWidgetItem(QString::number(permiso.orden));
            ordenItem->setTextAlignment(Qt::AlignCenter);
            m_tablaPermisos->setItem(row, 0, ordenItem);

            auto* accesoItem = new QTableWidgetItem();
            accesoItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            accesoItem->setTextAlignment(Qt::AlignCenter);
            accesoItem->setCheckState(permiso.habilitado ? Qt::Checked : Qt::Unchecked);
            m_tablaPermisos->setItem(row, 1, accesoItem);

            auto* paginaItem = new QTableWidgetItem(QString::fromStdString(permiso.nombre));
            paginaItem->setData(Qt::UserRole, QString::fromStdString(permiso.codigo));
            m_tablaPermisos->setItem(row, 2, paginaItem);
        }
        m_lblEstado->setText(
            "Los cambios se guardan automáticamente. Desactivar «Permisos por perfil» "
            "retira a ese perfil la capacidad de administrar accesos. "
            "El orden se aplica a todas las páginas."
        );
    } catch (const std::exception& error) {
        m_tablaPermisos->setRowCount(0);
        m_ordenOriginal.clear();
        m_ordenPendiente = false;
        QMessageBox::critical(this, "Error al cargar permisos", error.what());
    }
    if (m_tablaPermisos->rowCount() > 0) {
        m_ordenOriginal = obtenerOrdenVisible();
        m_ordenPendiente = false;
    }
    m_rolIdCargado = rolId;
    m_cargando = false;
    actualizarBotonesOrden();
}

void PermisosPaginasWidget::onPermisoCambiado(QTableWidgetItem* item) {
    if (m_cargando || !item || item->column() != 1) {
        return;
    }

    const int rolId = m_cmbRol->currentData().toInt();
    const QString codigo = m_tablaPermisos->item(item->row(), 2)
        ->data(Qt::UserRole).toString();
    if (rolId <= 0 || codigo.isEmpty()) {
        return;
    }

    try {
        PermisosPaginasRepository::cambiarPermiso(
            m_usuarioId, rolId, codigo.toStdString(), item->checkState() == Qt::Checked);
        m_lblEstado->setText("Permiso guardado.");
        emit configuracionActualizada();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "No se pudo cambiar el permiso", error.what());
        cargarPermisos();
    }
}

void PermisosPaginasWidget::onSubirClicked() {
    const int fila = m_tablaPermisos->currentRow();
    if (fila > 0) {
        moverFila(fila, fila - 1);
    }
}

void PermisosPaginasWidget::onBajarClicked() {
    const int fila = m_tablaPermisos->currentRow();
    if (fila >= 0 && fila < m_tablaPermisos->rowCount() - 1) {
        moverFila(fila, fila + 1);
    }
}

void PermisosPaginasWidget::onGuardarOrdenClicked() {
    guardarOrden();
}

void PermisosPaginasWidget::actualizarBotonesOrden() {
    const int fila = m_tablaPermisos->currentRow();
    m_btnSubir->setEnabled(fila > 0);
    m_btnBajar->setEnabled(fila >= 0 && fila < m_tablaPermisos->rowCount() - 1);
    m_btnGuardarOrden->setEnabled(m_ordenPendiente);
}

void PermisosPaginasWidget::moverFila(int fila, int destino) {
    if (fila < 0 || destino < 0
        || fila >= m_tablaPermisos->rowCount()
        || destino >= m_tablaPermisos->rowCount()
        || fila == destino) {
        return;
    }

    m_cargando = true;
    for (int columna = 0; columna < m_tablaPermisos->columnCount(); ++columna) {
        QTableWidgetItem* actual = m_tablaPermisos->takeItem(fila, columna);
        QTableWidgetItem* vecino = m_tablaPermisos->takeItem(destino, columna);
        m_tablaPermisos->setItem(fila, columna, vecino);
        m_tablaPermisos->setItem(destino, columna, actual);
    }
    for (int row = 0; row < m_tablaPermisos->rowCount(); ++row) {
        m_tablaPermisos->item(row, 0)->setText(QString::number(row + 1));
    }
    m_cargando = false;

    m_tablaPermisos->setCurrentCell(destino, 2);
    m_tablaPermisos->selectRow(destino);
    m_ordenPendiente = obtenerOrdenVisible() != m_ordenOriginal;
    m_lblEstado->setText(m_ordenPendiente
        ? "Orden pendiente de guardar."
        : "El orden coincide con el guardado.");
    actualizarBotonesOrden();
}

std::vector<std::string> PermisosPaginasWidget::obtenerOrdenVisible() const {
    std::vector<std::string> orden;
    orden.reserve(static_cast<std::size_t>(m_tablaPermisos->rowCount()));
    for (int row = 0; row < m_tablaPermisos->rowCount(); ++row) {
        orden.push_back(
            m_tablaPermisos->item(row, 2)->data(Qt::UserRole).toString().toStdString());
    }
    return orden;
}

bool PermisosPaginasWidget::guardarOrden() {
    if (!m_ordenPendiente) {
        return true;
    }

    try {
        const auto orden = obtenerOrdenVisible();
        PermisosPaginasRepository::guardarOrdenPaginas(m_usuarioId, orden);
        m_ordenOriginal = orden;
        m_ordenPendiente = false;
        m_lblEstado->setText("Orden guardado; la navegación se actualizó.");
        actualizarBotonesOrden();
        emit configuracionActualizada();
        return true;
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "No se pudo guardar el orden", error.what());
        return false;
    }
}
