// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PucCuentasWidget.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString claseParaCodigo(const QString& codigo) {
    if (codigo.isEmpty()) {
        return {};
    }
    switch (codigo.front().unicode()) {
    case '1': return "ACTIVO";
    case '2': return "PASIVO";
    case '3': return "PATRIMONIO";
    case '4': return "INGRESOS";
    case '5': return "GASTOS";
    case '8':
    case '9': return "ORDEN";
    default: return "No soportada";
    }
}

QString naturalezaParaCodigo(const QString& codigo) {
    if (codigo.isEmpty()) {
        return {};
    }
    const QChar clase = codigo.front();
    return clase == '2' || clase == '3' || clase == '4' || clase == '9'
        ? "CREDITO"
        : "DEBITO";
}

int nivelParaLongitud(int longitud) {
    switch (longitud) {
    case 1: return 1;
    case 2: return 2;
    case 4: return 3;
    case 6: return 4;
    case 9: return 5;
    default: return 0;
    }
}

QString padreParaCodigo(const QString& codigo) {
    const int nivel = nivelParaLongitud(codigo.size());
    static constexpr int longitudes[] = {1, 2, 4, 6, 9};
    if (nivel <= 1) {
        return "—";
    }
    return codigo.left(longitudes[nivel - 2]);
}

}  // namespace

PucCuentasWidget::PucCuentasWidget(int usuarioId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioId) {
    setupUi();
    recargarTabla();
}

void PucCuentasWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(
        "<h3>Plan Único de Cuentas</h3>"
        "Estructura de códigos: clase (1), grupo (2), cuenta (4), "
        "subcuenta (6) y auxiliar (9 dígitos).", this));

    auto* buscar = new QHBoxLayout();
    m_txtBuscar = new QLineEdit(this);
    m_txtBuscar->setPlaceholderText("Filtrar por código o nombre...");
    connect(m_txtBuscar, &QLineEdit::textChanged, this, &PucCuentasWidget::recargarTabla);
    buscar->addWidget(m_txtBuscar);
    layout->addLayout(buscar);

    m_tablaCuentas = new QTableWidget(this);
    m_tablaCuentas->setColumnCount(8);
    m_tablaCuentas->setHorizontalHeaderLabels({
        "Código", "Nombre", "Clase", "Naturaleza", "Nivel", "Cuenta padre",
        "Movimiento", "Exige tercero"
    });
    m_tablaCuentas->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaCuentas->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaCuentas->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaCuentas->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tablaCuentas->horizontalHeader()->setStretchLastSection(true);
    connect(m_tablaCuentas, &QTableWidget::itemSelectionChanged,
            this, &PucCuentasWidget::onFilaSeleccionada);
    layout->addWidget(m_tablaCuentas, 1);

    auto* form = new QGroupBox("Datos de la cuenta", this);
    auto* formLayout = new QFormLayout(form);

    m_txtCodigo = new QLineEdit(this);
    m_txtCodigo->setMaxLength(9);
    m_txtCodigo->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[1-9][0-9]{0,8}"), m_txtCodigo));
    connect(m_txtCodigo, &QLineEdit::textChanged,
            this, &PucCuentasWidget::onCodigoCambiado);

    m_txtNombre = new QLineEdit(this);
    m_lblClase = new QLabel(this);
    m_lblNivel = new QLabel(this);
    m_lblPadre = new QLabel(this);
    m_cmbNaturaleza = new QComboBox(this);
    m_cmbNaturaleza->addItem("Débito", "DEBITO");
    m_cmbNaturaleza->addItem("Crédito", "CREDITO");
    m_chkPermiteMovimiento = new QCheckBox("Permite registrar movimientos", this);
    m_chkExigeTercero = new QCheckBox("Exige tercero", this);
    m_chkExigeTercero->setChecked(true);

    formLayout->addRow("Código *:", m_txtCodigo);
    formLayout->addRow("Nombre *:", m_txtNombre);
    formLayout->addRow("Clase (según código):", m_lblClase);
    formLayout->addRow("Naturaleza *:", m_cmbNaturaleza);
    formLayout->addRow("Nivel (según código):", m_lblNivel);
    formLayout->addRow("Cuenta padre (según código):", m_lblPadre);
    formLayout->addRow(m_chkPermiteMovimiento);
    formLayout->addRow(m_chkExigeTercero);

    auto* actions = new QHBoxLayout();
    auto* btnNuevo = new QPushButton("Nuevo", this);
    m_btnGuardar = new QPushButton("Crear cuenta", this);
    m_btnEliminar = new QPushButton("Eliminar", this);
    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold;");
    m_btnEliminar->setEnabled(false);
    connect(btnNuevo, &QPushButton::clicked, this, &PucCuentasWidget::onNuevoClicked);
    connect(m_btnGuardar, &QPushButton::clicked, this, &PucCuentasWidget::onGuardarClicked);
    connect(m_btnEliminar, &QPushButton::clicked, this, &PucCuentasWidget::onEliminarClicked);
    actions->addWidget(btnNuevo);
    actions->addWidget(m_btnGuardar);
    actions->addWidget(m_btnEliminar);
    formLayout->addRow(actions);
    layout->addWidget(form);
}

void PucCuentasWidget::recargarTabla() {
    try {
        const std::string codigoSeleccionado = m_codigoOriginal;
        m_cuentas = PucCuentasRepository::listar(m_txtBuscar->text().trimmed().toStdString());
        m_tablaCuentas->setRowCount(static_cast<int>(m_cuentas.size()));
        int filaSeleccionada = -1;
        for (int row = 0; row < static_cast<int>(m_cuentas.size()); ++row) {
            const PucCuenta& cuenta = m_cuentas[static_cast<std::size_t>(row)];
            if (cuenta.codigo == codigoSeleccionado) {
                filaSeleccionada = row;
            }
            const QStringList valores{
                QString::fromStdString(cuenta.codigo),
                QString::fromStdString(cuenta.nombre),
                QString::fromStdString(cuenta.clase),
                cuenta.naturaleza == "DEBITO" ? "Débito" : "Crédito",
                QString::number(cuenta.nivel),
                cuenta.cuentaPadre.empty() ? "—" : QString::fromStdString(cuenta.cuentaPadre),
                cuenta.permiteMovimiento ? "Sí" : "No",
                cuenta.exigeTercero ? "Sí" : "No"
            };
            for (int column = 0; column < valores.size(); ++column) {
                m_tablaCuentas->setItem(row, column, new QTableWidgetItem(valores[column]));
            }
        }
        if (filaSeleccionada >= 0) {
            m_tablaCuentas->selectRow(filaSeleccionada);
        } else if (!codigoSeleccionado.empty()) {
            limpiarFormulario();
        }
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Error al cargar cuentas", error.what());
    }
}

void PucCuentasWidget::onFilaSeleccionada() {
    const auto seleccion = m_tablaCuentas->selectedItems();
    if (seleccion.isEmpty()) {
        return;
    }
    const int fila = seleccion.first()->row();
    if (fila < 0 || static_cast<std::size_t>(fila) >= m_cuentas.size()) {
        return;
    }
    const PucCuenta& cuenta = m_cuentas[static_cast<std::size_t>(fila)];
    m_codigoOriginal = cuenta.codigo;
    m_txtCodigo->setText(QString::fromStdString(cuenta.codigo));
    m_txtNombre->setText(QString::fromStdString(cuenta.nombre));
    m_cmbNaturaleza->setCurrentIndex(m_cmbNaturaleza->findData(
        QString::fromStdString(cuenta.naturaleza)));
    m_chkPermiteMovimiento->setChecked(cuenta.permiteMovimiento);
    m_chkExigeTercero->setChecked(cuenta.exigeTercero);
    m_btnGuardar->setText("Guardar cambios");
    m_btnEliminar->setEnabled(true);
}

void PucCuentasWidget::onNuevoClicked() {
    limpiarFormulario();
}

void PucCuentasWidget::limpiarFormulario() {
    m_codigoOriginal.clear();
    m_tablaCuentas->clearSelection();
    m_txtCodigo->clear();
    m_txtNombre->clear();
    m_lblClase->clear();
    m_lblNivel->clear();
    m_lblPadre->setText("—");
    m_cmbNaturaleza->setCurrentIndex(0);
    m_chkPermiteMovimiento->setChecked(false);
    m_chkExigeTercero->setChecked(true);
    m_btnGuardar->setText("Crear cuenta");
    m_btnEliminar->setEnabled(false);
    m_txtCodigo->setFocus();
}

void PucCuentasWidget::onCodigoCambiado(const QString& codigo) {
    m_lblClase->setText(claseParaCodigo(codigo));
    const int nivel = nivelParaLongitud(codigo.size());
    m_lblNivel->setText(nivel == 0 ? "—" : QString::number(nivel));
    m_lblPadre->setText(padreParaCodigo(codigo));
    const QString naturaleza = naturalezaParaCodigo(codigo);
    if (!naturaleza.isEmpty()) {
        m_cmbNaturaleza->setCurrentIndex(m_cmbNaturaleza->findData(naturaleza));
    }
}

PucCuenta PucCuentasWidget::leerFormulario() const {
    PucCuenta cuenta;
    cuenta.codigo = m_txtCodigo->text().trimmed().toStdString();
    cuenta.nombre = m_txtNombre->text().trimmed().toStdString();
    cuenta.clase = claseParaCodigo(m_txtCodigo->text()).toStdString();
    cuenta.naturaleza = m_cmbNaturaleza->currentData().toString().toStdString();
    cuenta.nivel = nivelParaLongitud(m_txtCodigo->text().size());
    const QString padre = padreParaCodigo(m_txtCodigo->text());
    cuenta.cuentaPadre = padre == "—" ? "" : padre.toStdString();
    cuenta.permiteMovimiento = m_chkPermiteMovimiento->isChecked();
    cuenta.exigeTercero = m_chkExigeTercero->isChecked();
    return cuenta;
}

void PucCuentasWidget::onGuardarClicked() {
    try {
        const PucCuenta cuenta = leerFormulario();
        if (m_codigoOriginal.empty()) {
            PucCuentasRepository::crear(cuenta, m_usuarioId);
            QMessageBox::information(this, "Cuenta creada", "La cuenta contable fue creada correctamente.");
        } else {
            PucCuentasRepository::actualizar(m_codigoOriginal, cuenta, m_usuarioId);
            QMessageBox::information(this, "Cuenta actualizada", "La cuenta contable fue actualizada.");
        }
        limpiarFormulario();
        recargarTabla();
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "No se pudo guardar la cuenta", error.what());
    }
}

void PucCuentasWidget::onEliminarClicked() {
    if (m_codigoOriginal.empty()) {
        return;
    }
    const auto confirmacion = QMessageBox::question(
        this, "Confirmar eliminación",
        "¿Desea eliminar la cuenta " + QString::fromStdString(m_codigoOriginal) + "?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (confirmacion != QMessageBox::Yes) {
        return;
    }

    try {
        PucCuentasRepository::eliminar(m_codigoOriginal, m_usuarioId);
        limpiarFormulario();
        recargarTabla();
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "No se pudo eliminar la cuenta", error.what());
    }
}
