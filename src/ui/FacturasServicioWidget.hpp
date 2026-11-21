// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/FacturasServicioEngine.hpp"
#include "core/ConceptosCobroRepository.hpp"
#include "models/UnidadPrivada.hpp"
#include <QWidget>
#include <vector>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QPushButton;
class QTableWidget;

class FacturasServicioWidget : public QWidget {
    Q_OBJECT

public:
    explicit FacturasServicioWidget(int usuarioActualId, QWidget* parent = nullptr);

public slots:
    void recargarTerceros();
    void recargarUnidades();
    void recargarConceptos();

private slots:
    void agregarLinea();
    void quitarLinea();
    void guardarBorrador();
    void actualizarCampoPlaca();
    void confirmarPagoSeleccionado();
    void asignarProximaCuentaSeleccionada();
    void actualizarBotonesAccion();

private:
    void setupUi();
    void recargarBorradores();

    int m_usuarioId;
    QComboBox* m_cmbCliente = nullptr;
    QComboBox* m_cmbServicio = nullptr;
    QComboBox* m_cmbUnidadVisitada = nullptr;
    QLineEdit* m_txtPlacaVehiculo = nullptr;
    QDateEdit* m_fechaEmision = nullptr;
    QDateEdit* m_fechaVencimiento = nullptr;
    QTableWidget* m_tablaLineas = nullptr;
    QTableWidget* m_tablaBorradores = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    std::vector<FacturaServicioBorrador> m_borradoresCache;
    std::vector<ConceptoCobroServicio> m_conceptosCache;
    std::vector<UnidadPrivada> m_unidadesCache;
    QPushButton* m_btnConfirmarPago = nullptr;
    QPushButton* m_btnAsignarCuenta = nullptr;

signals:
    void facturacionActualizada();
};
