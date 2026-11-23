// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/PucCuentasRepository.hpp"

#include <QWidget>

#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class PucCuentasWidget : public QWidget {
    Q_OBJECT

public:
    explicit PucCuentasWidget(int usuarioId, QWidget* parent = nullptr);

private slots:
    void recargarTabla();
    void onFilaSeleccionada();
    void onNuevoClicked();
    void onGuardarClicked();
    void onEliminarClicked();
    void onCodigoCambiado(const QString& codigo);

private:
    void setupUi();
    void limpiarFormulario();
    PucCuenta leerFormulario() const;

    int m_usuarioId;
    std::string m_codigoOriginal;
    std::vector<PucCuenta> m_cuentas;

    QLineEdit* m_txtBuscar = nullptr;
    QTableWidget* m_tablaCuentas = nullptr;
    QLineEdit* m_txtCodigo = nullptr;
    QLineEdit* m_txtNombre = nullptr;
    QLabel* m_lblClase = nullptr;
    QLabel* m_lblNivel = nullptr;
    QLabel* m_lblPadre = nullptr;
    QComboBox* m_cmbNaturaleza = nullptr;
    QCheckBox* m_chkPermiteMovimiento = nullptr;
    QCheckBox* m_chkExigeTercero = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnEliminar = nullptr;
};
