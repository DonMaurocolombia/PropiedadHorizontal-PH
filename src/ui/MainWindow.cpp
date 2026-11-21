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


#include "MainWindow.hpp"
#include "core/DbManager.hpp"
#include "ui/TercerosWidget.hpp"
#include "ui/UnidadesWidget.hpp"
#include "ui/ParqueaderosWidget.hpp"
#include "ui/FacturacionWidget.hpp"
#include "ui/FacturasServicioWidget.hpp"
#include "ui/ConceptosCobroWidget.hpp"
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
    m_tabTerceros         = new TercerosWidget(m_sesion.id, this);
    m_tabUnidades         = new UnidadesWidget(m_sesion.id, this);
    m_tabParqueaderos     = new ParqueaderosWidget(m_sesion.id, this);
    m_tabFacturacion      = new FacturacionWidget(m_sesion.id, this);
    m_tabFacturasServicio = new FacturasServicioWidget(m_sesion.id, this);
    m_tabConceptosCobro   = new ConceptosCobroWidget(m_sesion.id, this);
    m_tabRecaudo          = new RecaudoWidget(m_sesion.id, this);
    m_tabEgresos          = new EgresosWidget(m_sesion.id, this);
    m_tabPresupuesto      = new PresupuestoWidget(m_sesion.id, this);
    m_tabReportes         = new ReportesWidget(this);
    m_tabAsambleas        = new AsambleasWidget(m_sesion.id, this);
    m_tabSeguridad        = new UsuariosAuditoriaWidget(m_sesion, this);
    m_tabConfig           = new ConfiguracionWidget(m_sesion, m_rutaDb, this);

    // --- SINCRONIZACIÓN EN TIEMPO REAL ENTRE MÓDULOS ---
    // 1. Al crear/editar terceros, actualizar combos en Unidades y Egresos
    connect(m_tabTerceros, &TercerosWidget::tercerosActualizados, 
            m_tabUnidades, &UnidadesWidget::recargarDatos);
    connect(m_tabTerceros, &TercerosWidget::tercerosActualizados, 
            m_tabEgresos,  &EgresosWidget::recargarDatos);
    connect(m_tabTerceros, &TercerosWidget::tercerosActualizados,
            m_tabFacturasServicio, &FacturasServicioWidget::recargarTerceros);
    connect(m_tabUnidades,         &UnidadesWidget::unidadesActualizadas,
            m_tabFacturasServicio, &FacturasServicioWidget::recargarUnidades);
    connect(m_tabConceptosCobro,   &ConceptosCobroWidget::conceptosActualizados,
            m_tabFacturasServicio, &FacturasServicioWidget::recargarConceptos);
    connect(m_tabFacturasServicio, &FacturasServicioWidget::facturacionActualizada,
            m_tabFacturacion,      &FacturacionWidget::recargarTabla);
    connect(m_tabFacturasServicio, &FacturasServicioWidget::facturacionActualizada,
            m_tabRecaudo,          &RecaudoWidget::recargarDatos);
    connect(m_tabFacturasServicio, &FacturasServicioWidget::facturacionActualizada,
            m_tabReportes,         &ReportesWidget::actualizarTodosLosReportes);

    // 2. Al generar facturación mensual, actualizar Recaudo y Reportes
    connect(m_tabFacturacion, &FacturacionWidget::facturacionGenerada, 
            m_tabRecaudo,     &RecaudoWidget::recargarDatos);
    connect(m_tabFacturacion, &FacturacionWidget::facturacionGenerada, 
            m_tabReportes,    &ReportesWidget::actualizarTodosLosReportes);

    // 3. Al registrar un pago (Recibo de Caja), actualizar Facturación y Reportes
    connect(m_tabRecaudo,     &RecaudoWidget::pagoRegistrado, 
            m_tabFacturacion, &FacturacionWidget::recargarTabla);
    connect(m_tabRecaudo,     &RecaudoWidget::pagoRegistrado, 
            m_tabReportes,    &ReportesWidget::actualizarTodosLosReportes);

    // 4. Al registrar un egreso o anulación, actualizar Presupuesto y Reportes Financieros
    connect(m_tabEgresos,     &EgresosWidget::egresoRegistrado, 
            m_tabPresupuesto, &PresupuestoWidget::recargarTodo);
    connect(m_tabEgresos,     &EgresosWidget::egresoRegistrado, 
            m_tabReportes,    &ReportesWidget::actualizarTodosLosReportes);

    // 5. Al actualizar el nombre de la copropiedad, refrescar título de ventana
    connect(m_tabConfig, &ConfiguracionWidget::configuracionActualizada, 
            this,        [this](const QString& nuevoNombre) {
        setWindowTitle(nuevoNombre + " - Sistema de Gestión PH");
    });

    // Registrar pestañas en orden lógico de operación
    m_tabs->addTab(m_tabFacturasServicio, "1. Facturas Servicios");            // indice: 0
    m_tabs->addTab(m_tabUnidades,         "2. Unidades Privadas");             // indice: 1 
    m_tabs->addTab(m_tabTerceros,         "3. Terceros");                      // indice: 2
    m_tabs->addTab(m_tabParqueaderos,     "4. Parqueaderos y Bodegas");        // indice: 3
    m_tabs->addTab(m_tabFacturacion,      "5. Cuentas de Cobro");              // indice: 4
    m_tabs->addTab(m_tabRecaudo,          "6. Recaudo y Paz y Salvos");        // indice: 5
    m_tabs->addTab(m_tabEgresos,          "7. Egresos y Proveedores");         // indice: 6
    m_tabs->addTab(m_tabPresupuesto,      "8. Presupuesto");                   // indice: 7
    m_tabs->addTab(m_tabReportes,         "9. Reportes Financieros");          // indice: 8
    m_tabs->addTab(m_tabAsambleas,        "10. Asambleas y Quórum");           // indice: 9
    m_tabs->addTab(m_tabSeguridad,        "11. Usuarios y Auditoría");         // indice: 10
    m_tabs->addTab(m_tabConceptosCobro,   "12. Conceptos cobro de servicios"); // indice: 11
    m_tabs->addTab(m_tabConfig,           "13. Configuración");                // indice: 12

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
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabFacturacion),    false); // Facturación 
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabRecaudo),        false); // Recaudo
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabEgresos),        false); // Egresos
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabConfig),         false); // Configuración
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabConceptosCobro), false); // Facturas de Servicios 
        m_tabs->setCurrentIndex(m_tabs->indexOf(m_tabReportes));            // Abre directamente en Reportes Financieros 
        // Aquí podrías deshabilitar botones de edición en cada widget
        // (ej: unidadesTab->setSoloLectura(true))
    } else if (m_sesion.rolNombre == "ASISTENTE") {
        // El Asistente opera residentes, unidades, parqueaderos, recaudo y asambleas
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabEgresos),        false); // Egresos
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabPresupuesto),    false); // Presupuesto 
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabSeguridad),      false); // Usuarios y Auditoría
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabConfig),         false); // Configuración
        m_tabs->setTabEnabled(m_tabs->indexOf(m_tabConceptosCobro), false); // Facturas de Servicios
    }
    /***************
     * Agrega más roles según necesidad, con la misma lógica de habilitar o deshabilitar
     * pestañas y botones según permisos definidos.
     * 
     * A futuro, se podría implementar un sistema de permisos leyendo desde la base de datos,
     * lo que permitiría asignar permisos más dinámicos y detallados de cada rol o usuario y
     * no quemado en el código fuente.
     *********************************/
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
