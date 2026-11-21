// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/ConceptosCobroRepository.hpp"
#include <QWidget>
#include <vector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

class ConceptosCobroWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConceptosCobroWidget(int usuarioActualId, QWidget* parent = nullptr);

signals:
    void conceptosActualizados();

public slots:
    void recargarTabla();

private slots:
    void seleccionarFila();
    void nuevoConcepto();
    void guardarConcepto();
    void eliminarConcepto();

private:
    void limpiarFormulario();
    void cargarCuentas();

    int m_usuarioId;
    int m_conceptoSeleccionadoId = 0;
    std::vector<ConceptoCobroServicio> m_conceptos;
    QTableWidget* m_tabla = nullptr;
    QLineEdit* m_txtCodigo = nullptr;
    QLineEdit* m_txtNombre = nullptr;
    QComboBox* m_cmbCuentaIngreso = nullptr;
    QComboBox* m_cmbCuentaCartera = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnEliminar = nullptr;
};
