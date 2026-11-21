// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FacturasServicioWidget.hpp"

#include "core/TerceroRepository.hpp"
#include "core/UnidadRepository.hpp"
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <stdexcept>

FacturasServicioWidget::FacturasServicioWidget(int usuarioActualId, QWidget* parent)
    : QWidget(parent), m_usuarioId(usuarioActualId) {
    setupUi();
    recargarTerceros();
    recargarUnidades();
    recargarConceptos();
    recargarBorradores();
}

void FacturasServicioWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<h3>Borradores locales de cobros por servicios</h3>", this));
    layout->addWidget(new QLabel(
        "Preparación local únicamente: no asigna numeración autorizada, no genera XML UBL firmado "
        "ni transmite o valida documentos ante la DIAN. En esta etapa no se cobra ni se muestra IVA.",
        this
    ));

    auto* datosGroup = new QGroupBox("Datos del borrador", this);
    auto* datosForm = new QFormLayout(datosGroup);
    m_cmbCliente = new QComboBox(this);
    m_cmbServicio = new QComboBox(this);
    m_cmbUnidadVisitada = new QComboBox(this);
    m_txtPlacaVehiculo = new QLineEdit(this);
    m_txtPlacaVehiculo->setPlaceholderText("Obligatoria para parqueadero de vehículo");
    connect(m_cmbServicio, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FacturasServicioWidget::actualizarCampoPlaca);
    m_fechaEmision = new QDateEdit(QDate::currentDate(), this);
    m_fechaEmision->setCalendarPopup(true);
    m_fechaEmision->setDisplayFormat("yyyy-MM-dd");
    m_fechaVencimiento = new QDateEdit(QDate::currentDate().addDays(30), this);
    m_fechaVencimiento->setCalendarPopup(true);
    m_fechaVencimiento->setDisplayFormat("yyyy-MM-dd");
    datosForm->addRow("Responsable de pago:", m_cmbCliente);
    datosForm->addRow("Servicio:", m_cmbServicio);
    datosForm->addRow("Unidad privada visitada:", m_cmbUnidadVisitada);
    datosForm->addRow("Placa del vehículo:", m_txtPlacaVehiculo);
    datosForm->addRow("Fecha de emisión prevista:", m_fechaEmision);
    datosForm->addRow("Fecha de vencimiento:", m_fechaVencimiento);
    actualizarCampoPlaca();
    layout->addWidget(datosGroup);

    auto* lineasGroup = new QGroupBox("Servicios", this);
    auto* lineasLayout = new QVBoxLayout(lineasGroup);
    m_tablaLineas = new QTableWidget(this);
    m_tablaLineas->setColumnCount(2);
    m_tablaLineas->setHorizontalHeaderLabels({"Descripción del servicio", "Valor (COP), sin IVA"});
    m_tablaLineas->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tablaLineas->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tablaLineas->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaLineas->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* lineasBotones = new QHBoxLayout();
    auto* btnAgregarLinea = new QPushButton("Agregar servicio", this);
    auto* btnQuitarLinea = new QPushButton("Quitar servicio seleccionado", this);
    connect(btnAgregarLinea, &QPushButton::clicked, this, &FacturasServicioWidget::agregarLinea);
    connect(btnQuitarLinea, &QPushButton::clicked, this, &FacturasServicioWidget::quitarLinea);
    lineasBotones->addWidget(btnAgregarLinea);
    lineasBotones->addWidget(btnQuitarLinea);
    lineasBotones->addStretch();
    lineasLayout->addWidget(m_tablaLineas);
    lineasLayout->addLayout(lineasBotones);
    layout->addWidget(lineasGroup, 1);
    agregarLinea();

    m_btnGuardar = new QPushButton("Guardar borrador local", this);
    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 7px;");
    connect(m_btnGuardar, &QPushButton::clicked, this, &FacturasServicioWidget::guardarBorrador);
    layout->addWidget(m_btnGuardar, 0, Qt::AlignRight);

    m_tablaBorradores = new QTableWidget(this);
    m_tablaBorradores->setColumnCount(9);
    m_tablaBorradores->setHorizontalHeaderLabels({
        "Referencia local", "Servicio", "Unidad visitada", "Responsable de pago",
        "Documento", "Emisión", "Vencimiento", "Total sin IVA", "Estado"
    });
    m_tablaBorradores->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaBorradores->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablaBorradores->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablaBorradores->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tablaBorradores, &QTableWidget::itemSelectionChanged,
            this, &FacturasServicioWidget::actualizarBotonesAccion);
    layout->addWidget(new QLabel("<b>Borradores guardados (no emitidos)</b>", this));
    auto* acciones = new QHBoxLayout();
    m_btnConfirmarPago = new QPushButton("Confirmar pago", this);
    m_btnAsignarCuenta = new QPushButton("Cobrar en próxima cuenta de la unidad", this);
    m_btnConfirmarPago->setEnabled(false);
    m_btnAsignarCuenta->setEnabled(false);
    connect(m_btnConfirmarPago, &QPushButton::clicked,
            this, &FacturasServicioWidget::confirmarPagoSeleccionado);
    connect(m_btnAsignarCuenta, &QPushButton::clicked,
            this, &FacturasServicioWidget::asignarProximaCuentaSeleccionada);
    acciones->addWidget(m_btnConfirmarPago);
    acciones->addWidget(m_btnAsignarCuenta);
    acciones->addStretch();
    layout->addLayout(acciones);
    layout->addWidget(m_tablaBorradores, 1);
}

void FacturasServicioWidget::recargarTerceros() {
    const QSignalBlocker blocker(m_cmbCliente);
    m_cmbCliente->clear();
    m_cmbCliente->addItem("-- Seleccione responsable de pago --", 0);

    for (const auto& tercero : TerceroRepository::listar()) {
        if (!tercero.activo
            || (!tercero.esPropietario && !tercero.esInquilino
                && !tercero.esResidente && !tercero.esVisitante)) {
            continue;
        }
        const QString documento = QString::fromStdString(
            tercero.tipoDocumento + " " + tercero.numeroDocumento
            + (tercero.dv.empty() ? "" : "-" + tercero.dv)
        );
        m_cmbCliente->addItem(
            QString("%1 — %2").arg(documento, QString::fromStdString(tercero.nombreRazonSocial)),
            tercero.id
        );
    }
    m_btnGuardar->setEnabled(m_cmbCliente->count() > 1
                             && m_cmbUnidadVisitada->count() > 1
                             && m_cmbServicio->currentData().toInt() > 0);
}

void FacturasServicioWidget::recargarUnidades() {
    const QSignalBlocker blocker(m_cmbUnidadVisitada);
    m_cmbUnidadVisitada->clear();
    m_cmbUnidadVisitada->addItem("-- Seleccione unidad visitada --", 0);
    m_unidadesCache = UnidadRepository::listarTodas();
    for (const auto& unidad : m_unidadesCache) {
        if (unidad.estado != "ACTIVA") {
            continue;
        }
        const QString label = QString("%1 - %2")
            .arg(QString::fromStdString(unidad.torreBloque),
                 QString::fromStdString(unidad.nomenclatura));
        m_cmbUnidadVisitada->addItem(label, unidad.id);
    }
    m_btnGuardar->setEnabled(m_cmbCliente->count() > 1
                             && m_cmbUnidadVisitada->count() > 1
                             && m_cmbServicio->currentData().toInt() > 0);
}

void FacturasServicioWidget::recargarConceptos() {
    const int conceptoActual = m_cmbServicio->currentData().toInt();
    const QSignalBlocker blocker(m_cmbServicio);
    m_cmbServicio->clear();
    m_cmbServicio->addItem("-- Seleccione concepto de cobro --", 0);
    m_conceptosCache = ConceptosCobroRepository::listarServicios();
    for (const auto& concepto : m_conceptosCache) {
        m_cmbServicio->addItem(
            QString("%1 (%2)").arg(QString::fromStdString(concepto.nombre),
                                   QString::fromStdString(concepto.codigo)),
            concepto.id
        );
        m_cmbServicio->setItemData(m_cmbServicio->count() - 1,
                                   QString::fromStdString(concepto.codigo),
                                   Qt::UserRole + 1);
    }
    const int indexActual = m_cmbServicio->findData(conceptoActual);
    if (indexActual >= 0) {
        m_cmbServicio->setCurrentIndex(indexActual);
    }
    actualizarCampoPlaca();
    m_btnGuardar->setEnabled(m_cmbCliente->count() > 1
                             && m_cmbUnidadVisitada->count() > 1
                             && m_cmbServicio->currentData().toInt() > 0);
}

void FacturasServicioWidget::agregarLinea() {
    const int fila = m_tablaLineas->rowCount();
    m_tablaLineas->insertRow(fila);
    m_tablaLineas->setItem(fila, 0, new QTableWidgetItem());

    auto* base = new QDoubleSpinBox(m_tablaLineas);
    base->setRange(0, 1000000000000.0);
    base->setDecimals(0);
    base->setPrefix("$ ");
    base->setGroupSeparatorShown(true);
    m_tablaLineas->setCellWidget(fila, 1, base);
}

void FacturasServicioWidget::quitarLinea() {
    const int fila = m_tablaLineas->currentRow();
    if (fila >= 0) {
        m_tablaLineas->removeRow(fila);
    }
}

void FacturasServicioWidget::guardarBorrador() {
    if (m_cmbCliente->currentData().toInt() <= 0) {
        QMessageBox::warning(this, "Responsable requerido", "Seleccione un tercero activo registrado como propietario, inquilino, residente o visitante.");
        return;
    }
    if (m_cmbUnidadVisitada->currentData().toInt() <= 0) {
        QMessageBox::warning(this, "Unidad requerida", "Seleccione la unidad privada visitada, aunque el responsable de pago sea otra persona.");
        return;
    }
    if (m_cmbServicio->currentData().toInt() <= 0) {
        QMessageBox::warning(this, "Concepto requerido", "Seleccione un concepto de cobro de servicios configurado.");
        return;
    }
    if (m_fechaVencimiento->date() < m_fechaEmision->date()) {
        QMessageBox::warning(this, "Fechas inválidas", "El vencimiento no puede ser anterior a la fecha prevista de emisión.");
        return;
    }

    std::vector<FacturaServicioLineaRequest> lineas;
    for (int fila = 0; fila < m_tablaLineas->rowCount(); ++fila) {
        const auto* descripcionItem = m_tablaLineas->item(fila, 0);
        auto* base = qobject_cast<QDoubleSpinBox*>(m_tablaLineas->cellWidget(fila, 1));
        if (!descripcionItem || !base) {
            QMessageBox::critical(this, "Error de formulario", "No se pudo leer una línea del borrador.");
            return;
        }

        const std::string descripcion = descripcionItem->text().trimmed().toStdString();
        if (descripcion.empty() || base->value() <= 0) {
            QMessageBox::warning(this, "Línea incompleta", "Cada servicio requiere descripción y una base mayor que cero.");
            return;
        }
        lineas.push_back({
            descripcion,
            static_cast<int64_t>(base->value())
        });
    }

    try {
        const int id = FacturasServicioEngine::crearBorrador(
            m_cmbCliente->currentData().toInt(),
            m_cmbServicio->currentData().toInt(),
            m_cmbUnidadVisitada->currentData().toInt(),
            m_txtPlacaVehiculo->text().trimmed().toUpper().toStdString(),
            m_fechaEmision->date().toString("yyyy-MM-dd").toStdString(),
            m_fechaVencimiento->date().toString("yyyy-MM-dd").toStdString(),
            lineas,
            m_usuarioId
        );
        recargarBorradores();
        QMessageBox::information(
            this,
            "Borrador guardado",
            QString("Se guardó el borrador local BORRADOR-%1. No es una factura electrónica emitida ni validada por la DIAN.")
                .arg(id, 6, 10, QChar('0'))
        );
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "No se pudo guardar el borrador", e.what());
    }
}

void FacturasServicioWidget::actualizarCampoPlaca() {
    const QString codigo = m_cmbServicio->currentData(Qt::UserRole + 1).toString();
    const bool requierePlaca = codigo == "PARQ_VISI" || codigo == "PARQ_COMU";
    m_txtPlacaVehiculo->setEnabled(requierePlaca);
    if (!requierePlaca) {
        m_txtPlacaVehiculo->clear();
    }
}

void FacturasServicioWidget::recargarBorradores() {
    const QLocale moneda(QLocale::Spanish, QLocale::Colombia);
    m_borradoresCache = FacturasServicioEngine::listarBorradores();
    m_tablaBorradores->setRowCount(static_cast<int>(m_borradoresCache.size()));

    for (int fila = 0; fila < static_cast<int>(m_borradoresCache.size()); ++fila) {
        const auto& borrador = m_borradoresCache[static_cast<std::size_t>(fila)];
        const QString referencia = QString("BORRADOR-%1").arg(borrador.id, 6, 10, QChar('0'));
        const QString concepto = borrador.conceptoNombre.empty()
            ? QString("Concepto anterior no configurado")
            : QString::fromStdString(borrador.conceptoNombre);
        QString unidad = QString::fromStdString(borrador.unidadVisitada);
        if (unidad.isEmpty()) {
            unidad = "Sin unidad (registro anterior)";
        }
        if (!borrador.placaVehiculo.empty()) {
            unidad += " / Placa " + QString::fromStdString(borrador.placaVehiculo);
        }
        const QString documento = QString::fromStdString(
            borrador.tipoDocumento + " " + borrador.numeroDocumento
        );
        m_tablaBorradores->setItem(fila, 0, new QTableWidgetItem(referencia));
        m_tablaBorradores->setItem(fila, 1, new QTableWidgetItem(concepto));
        m_tablaBorradores->setItem(fila, 2, new QTableWidgetItem(unidad));
        m_tablaBorradores->setItem(fila, 3, new QTableWidgetItem(QString::fromStdString(borrador.terceroNombre)));
        m_tablaBorradores->setItem(fila, 4, new QTableWidgetItem(documento));
        m_tablaBorradores->setItem(fila, 5, new QTableWidgetItem(QString::fromStdString(borrador.fechaEmision)));
        m_tablaBorradores->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(borrador.fechaVencimiento)));
        m_tablaBorradores->setItem(fila, 7, new QTableWidgetItem(
            "$ " + moneda.toString(static_cast<qlonglong>(borrador.totalCop))));
        m_tablaBorradores->setItem(fila, 8, new QTableWidgetItem(
            QString::fromStdString(borrador.estado + " — SIN TRANSMITIR")));
    }
    actualizarBotonesAccion();
}

void FacturasServicioWidget::actualizarBotonesAccion() {
    const auto selected = m_tablaBorradores->selectedItems();
    if (selected.isEmpty()) {
        m_btnConfirmarPago->setEnabled(false);
        m_btnAsignarCuenta->setEnabled(false);
        return;
    }
    const int fila = selected.first()->row();
    const auto& borrador = m_borradoresCache[static_cast<std::size_t>(fila)];
    const bool accionable = borrador.estado == "BORRADOR" && borrador.conceptoId > 0;
    m_btnConfirmarPago->setEnabled(accionable);
    m_btnAsignarCuenta->setEnabled(accionable);
}

void FacturasServicioWidget::confirmarPagoSeleccionado() {
    const auto selected = m_tablaBorradores->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const auto& borrador = m_borradoresCache[static_cast<std::size_t>(selected.first()->row())];

    QDialog dialog(this);
    dialog.setWindowTitle("Confirmar pago del servicio");
    auto* form = new QFormLayout(&dialog);
    auto* fecha = new QDateEdit(QDate::currentDate(), &dialog);
    fecha->setCalendarPopup(true);
    fecha->setDisplayFormat("yyyy-MM-dd");
    auto* forma = new QComboBox(&dialog);
    forma->addItems({"TRANSFERENCIA", "CONSIGNACION", "PSE", "EFECTIVO", "DATAFONO"});
    auto* cuenta = new QComboBox(&dialog);
    const auto cuentas = ConceptosCobroRepository::listarCuentasPago();
    for (const auto& opcion : cuentas) {
        cuenta->addItem(QString("%1 - %2").arg(QString::fromStdString(opcion.codigo),
                                              QString::fromStdString(opcion.nombre)),
                        QString::fromStdString(opcion.codigo));
    }
    auto* referencia = new QLineEdit(&dialog);
    referencia->setPlaceholderText("Referencia o comprobante (opcional)");
    form->addRow("Fecha de pago:", fecha);
    form->addRow("Forma de pago:", forma);
    form->addRow("Cuenta de caja/bancos:", cuenta);
    form->addRow("Referencia:", referencia);
    auto* botones = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(botones, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(botones, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(botones);
    if (cuentas.empty()) {
        QMessageBox::warning(this, "Sin cuentas de pago", "No hay cuentas de caja o bancos configuradas en el PUC.");
        return;
    }
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    try {
        FacturasServicioEngine::confirmarPago(
            borrador.id,
            fecha->date().toString("yyyy-MM-dd").toStdString(),
            forma->currentText().toStdString(),
            cuenta->currentData().toString().toStdString(),
            referencia->text().trimmed().toStdString(),
            m_usuarioId
        );
        recargarBorradores();
        emit facturacionActualizada();
        QMessageBox::information(this, "Pago registrado", "El pago quedó contabilizado y aparecerá en los reportes financieros.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "No se pudo confirmar el pago", e.what());
    }
}

void FacturasServicioWidget::asignarProximaCuentaSeleccionada() {
    const auto selected = m_tablaBorradores->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const auto& borrador = m_borradoresCache[static_cast<std::size_t>(selected.first()->row())];
    const auto respuesta = QMessageBox::question(
        this,
        "Asignar a cuenta de cobro",
        "¿Desea cargar este servicio a la próxima cuenta de cobro que se genere para la unidad visitada?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (respuesta != QMessageBox::Yes) {
        return;
    }
    try {
        FacturasServicioEngine::asignarProximaCuenta(borrador.id, m_usuarioId);
        recargarBorradores();
        emit facturacionActualizada();
        QMessageBox::information(this, "Cobro asignado", "El cargo se incluirá en la próxima cuenta de cobro de la unidad visitada.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "No se pudo asignar el cobro", e.what());
    }
}
