// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/PermisosPaginasRepository.hpp"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;

class PermisosPaginasWidget : public QWidget {
    Q_OBJECT

public:
    explicit PermisosPaginasWidget(int usuarioId, QWidget* parent = nullptr);

signals:
    void configuracionActualizada();

private slots:
    void cargarPermisos();
    void onPermisoCambiado(QTableWidgetItem* item);
    void onSubirClicked();
    void onBajarClicked();
    void onGuardarOrdenClicked();
    void actualizarBotonesOrden();

private:
    void setupUi();
    void moverFila(int fila, int destino);
    std::vector<std::string> obtenerOrdenVisible() const;
    bool guardarOrden();

    int m_usuarioId;
    bool m_cargando = false;
    bool m_ordenPendiente = false;
    int m_rolIdCargado = 0;
    std::vector<std::string> m_ordenOriginal;
    QComboBox* m_cmbRol = nullptr;
    QTableWidget* m_tablaPermisos = nullptr;
    QLabel* m_lblEstado = nullptr;
    QPushButton* m_btnSubir = nullptr;
    QPushButton* m_btnBajar = nullptr;
    QPushButton* m_btnGuardarOrden = nullptr;
};
