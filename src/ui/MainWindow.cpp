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


#include "MainWindow.hpp"
#include "core/DbManager.hpp"
#include "ui/TercerosWidget.hpp"
#include "ui/UnidadesWidget.hpp"
#include "ui/ParqueaderosWidget.hpp"
#include "ui/FacturacionWidget.hpp"
#include "ui/RecaudoWidget.hpp"
#include "ui/EgresosWidget.hpp"
#include "ui/PresupuestoWidget.hpp"
#include "ui/ReportesWidget.hpp"
#include "ui/AsambleasWidget.hpp"
#include "ui/UsuariosAuditoriaWidget.hpp"
#include "ui/ConfiguracionWidget.hpp"
#include "ui/VerticalTabStyle.hpp"
#include <QStatusBar>
#include <QToolBar>
#include <QPushButton>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QCloseEvent>
#include <QTimer>


MainWindow::MainWindow(const UsuarioSesion& sesion,
                       const std::string& rutaDb,
                       QWidget* parent)
    : QMainWindow(parent), m_sesion(sesion), m_rutaDb(rutaDb) {
    setupUi();
    aplicarPermisosPorRol();
}

void MainWindow::setupUi() {
    int orientacion = 1;  // 0: superior, 1: izquierda, 2: inferior, 3: derecha
    // Cargar nombre de la copropiedad en el título de la ventana
    QString nombreCopropiedad = "Administración Propiedad Horizontal";
    auto& db = DatabaseManager::instance().getDb();
    // SQLite::Statement q(db, "SELECT razon_social FROM copropiedad_config WHERE id = 1");
    SQLite::Statement q(db, "SELECT razon_social, orientacion_menu FROM copropiedad_config WHERE id = 1");
    if (q.executeStep()) {
        nombreCopropiedad = QString::fromStdString(q.getColumn(0).getString());
        orientacion = q.getColumn(1).getInt();
    }

    setWindowTitle(nombreCopropiedad + " - Sistema de Gestión PH");
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry();  // excluye taskbar
    int widthNow = available.width();
    int width = (widthNow < 1750)? width = widthNow  * 0.9 : 1750;
    int heightNow = available.height();
    int height = (heightNow < 700)? heightNow * 0.9 : 700; 
    resize(width, height);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);

    // North: Arriba (default) | West: Izquierda | South: Abajo | East: Derecha
    QTabWidget::TabPosition position = QTabWidget::North;   // superior (por omisión o default)
    if (orientacion == 1) {                                 // izquierda
        position = QTabWidget::West;
    } else if (orientacion == 2) {                          // inferior
        position = QTabWidget::South;
    } else if (orientacion == 3) {                          // derecha
        position = QTabWidget::East;
    }

    m_tabs->setTabPosition(position);
    if (position == QTabWidget::West || position == QTabWidget::East) {
        // Aplicar el estilo personalizado al tabBar
        auto* vts = new VerticalTabStyle(m_tabs->tabBar()->style());
        m_tabs->tabBar()->setStyle(vts);
        m_tabs->tabBar()->setMinimumWidth(200);
        m_tabs->tabBar()->setStyleSheet(
            "QTabBar::tab { text-align: left; }"
            "QTabBar::tab:selected  { background-color: #3498db; }"
            "QTabBar::tab:!selected { background-color: #85ee8e; }"
        );
    } else {
        m_tabs->setStyleSheet(
            "QTabBar { color: black; }"
            "QTabBar::tab { text-align: center; }"
            "QTabBar::tab:selected { background-color: #3498db; }"
            "QTabBar::tab:!selected { background-color: #85ee8e; }"
        );
    }

    // Instanciar todos los módulos del sistema
    auto* tabTerceros      = new TercerosWidget(m_sesion.id, this);
    auto* tabUnidades      = new UnidadesWidget(m_sesion.id, this);
    auto* tabParqueaderos  = new ParqueaderosWidget(m_sesion.id, this);
    auto* tabFacturacion   = new FacturacionWidget(m_sesion.id, this);
    auto* tabRecaudo       = new RecaudoWidget(m_sesion.id, this);
    auto* tabEgresos       = new EgresosWidget(m_sesion.id, this);
    auto* tabPresupuesto   = new PresupuestoWidget(m_sesion.id, this);
    auto* tabReportes      = new ReportesWidget(this);
    auto* tabAsambleas     = new AsambleasWidget(m_sesion.id, this);
    auto* tabSeguridad     = new UsuariosAuditoriaWidget(m_sesion, this);
    auto* tabConfig        = new ConfiguracionWidget(m_sesion, m_rutaDb, this);

    // --- SINCRONIZACIÓN EN TIEMPO REAL ENTRE MÓDULOS ---
    // 1. Al crear/editar terceros, actualizar combos en Unidades y Egresos
    connect(tabTerceros, &TercerosWidget::tercerosActualizados, tabUnidades, &UnidadesWidget::recargarDatos);
    connect(tabTerceros, &TercerosWidget::tercerosActualizados, tabEgresos, &EgresosWidget::recargarDatos);

    // 2. Al generar facturación mensual, actualizar Recaudo y Reportes
    connect(tabFacturacion, &FacturacionWidget::facturacionGenerada, tabRecaudo, &RecaudoWidget::recargarDatos);
    connect(tabFacturacion, &FacturacionWidget::facturacionGenerada, tabReportes, &ReportesWidget::actualizarTodosLosReportes);

    // 3. Al registrar un pago (Recibo de Caja), actualizar Facturación y Reportes
    connect(tabRecaudo, &RecaudoWidget::pagoRegistrado, tabFacturacion, &FacturacionWidget::recargarTabla);
    connect(tabRecaudo, &RecaudoWidget::pagoRegistrado, tabReportes, &ReportesWidget::actualizarTodosLosReportes);

    // 4. Al registrar un egreso o anulación, actualizar Presupuesto y Reportes Financieros
    connect(tabEgresos, &EgresosWidget::egresoRegistrado, tabPresupuesto, &PresupuestoWidget::recargarTodo);
    connect(tabEgresos, &EgresosWidget::egresoRegistrado, tabReportes, &ReportesWidget::actualizarTodosLosReportes);

    // 5. Al actualizar el nombre de la copropiedad, refrescar título de ventana
    connect(tabConfig, &ConfiguracionWidget::configuracionActualizada, this, [this](const QString& nuevoNombre) {
        setWindowTitle(nuevoNombre + " - Sistema de Gestión PH");
    });

    // Registrar pestañas en orden lógico de operación
    m_tabs->addTab(tabUnidades,     "1. Unidades Privadas");        // indice: 0 
    m_tabs->addTab(tabTerceros,     "2. Terceros");                 // indice: 1
    m_tabs->addTab(tabParqueaderos, "3. Parqueaderos y Bodegas");   // indice: 2
    m_tabs->addTab(tabFacturacion,  "4. Facturación");              // indice: 3
    m_tabs->addTab(tabRecaudo,      "5. Recaudo y Paz y Salvos");   // indice: 4
    m_tabs->addTab(tabEgresos,      "6. Egresos y Proveedores");    // indice: 5
    m_tabs->addTab(tabPresupuesto,  "7. Presupuesto");              // indice: 6
    m_tabs->addTab(tabReportes,     "8. Reportes Financieros");     // indice: 7
    m_tabs->addTab(tabAsambleas,    "9. Asambleas y Quórum");       // indice: 8
    m_tabs->addTab(tabSeguridad,    "10. Usuarios y Auditoría");    // indice: 9
    m_tabs->addTab(tabConfig,       "11. Configuración");           // indice: 10

    setCentralWidget(m_tabs);

    // --- Barra de herramientas con botón de salida ---
    auto* toolbar = addToolBar("Acciones");
    toolbar->setMovable(false);
    auto* btnSalir = new QPushButton("🚪 Salir");
    btnSalir->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; font-weight: bold; "
        "padding: 6px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #a93226; }"
    );
    toolbar->addWidget(btnSalir);

    // --- Salida controlada con confirmación + auditoría ---
    connect(btnSalir, &QPushButton::clicked, this, [this]() {
        this->close();  // ← dispara closeEvent() con toda la lógica
    });

    QTimer::singleShot(0, this, [this]() {
        QMessageBox::information(
            this, "Bienvenida",
            QString(R"(Bienvenido %1 al Sistema de Gestión PH.<br>)").arg(QString::fromStdString(m_sesion.nombreCompleto))
        );
    });

    // Barra de estado inferior con información de sesión
    statusBar()->showMessage(
        QString("Usuario activo: %1 (%2)  |  Perfil: %3  |  Base de Datos: SQLite Local")
            .arg(QString::fromStdString(m_sesion.nombreCompleto))
            .arg(QString::fromStdString(m_sesion.username))
            .arg(QString::fromStdString(m_sesion.rolNombre))
    );
/*
    statusBar()->showMessage(
        QString("Bienvenido %1 al Sistema de Gestión PH.")
            .arg(QString::fromStdString(m_sesion.nombreCompleto)),
        10000  // 5 segundos
    );
 */
}

void MainWindow::aplicarPermisosPorRol() {
    // Control de acceso basado en roles (RBAC)
    // Ajusta los índices según el orden en que agregaste las pestañas
    if (m_sesion.rolNombre == "REVISOR_FISCAL") {
        // El Revisor Fiscal audita libros, presupuesto, reportes y logs, sin operar facturación diaria
        m_tabs->setTabEnabled(3, false); // Facturación
        m_tabs->setTabEnabled(4, false); // Recaudo
        m_tabs->setTabEnabled(5, false); // Egresos
        m_tabs->setCurrentIndex(7);      // Abre directamente en Reportes Financieros
        // Aquí podrías deshabilitar botones de edición en cada widget
        // (ej: unidadesTab->setSoloLectura(true))
    } else if (m_sesion.rolNombre == "ASISTENTE") {
        // El Asistente opera residentes, unidades, parqueaderos, recaudo y asambleas
        m_tabs->setTabEnabled(5, false);  // Egresos
        m_tabs->setTabEnabled(6, false);  // Presupuesto
        m_tabs->setTabEnabled(9, false);  // Usuarios y Auditoría
        m_tabs->setTabEnabled(10, false); // Configuración
    }
    // Agrega más roles según necesites
}

void MainWindow::closeEvent(QCloseEvent* event) {
    auto resp = QMessageBox::question(
        this, "Salir",
        "¿Desea salir de la aplicación?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (resp != QMessageBox::Yes) {
        event->ignore();  // ← cancela el cierre
        return;
    }

    // Registrar en auditoría
    try {
        auto& db = DatabaseManager::instance().getDb();
        std::string ts = QDateTime::currentDateTime()
            .toString("yyyy-MM-dd HH:mm:ss").toStdString();
        SQLite::Statement log(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'PRINCIPAL', 'SALIR', 'auditoria_logs', ?, ?)
        )");
        log.bind(1, m_sesion.id);
        log.bind(2, "SALIDA_APLICACION");
        log.bind(3, "[" + ts + "] - Cierre controlado de sesión");
        log.exec();
    } catch (const std::exception& e) {
        qWarning() << "Error al registrar salida en auditoría:" << e.what();
    }

    event->accept();  // ← permite el cierre
}
