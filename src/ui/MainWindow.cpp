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
#include "ui/PermisosPaginasWidget.hpp"
#include "ui/PucCuentasWidget.hpp"
#include "ui/VerticalTabStyle.hpp"
#include "core/PermisosPaginasRepository.hpp"
#include <QStatusBar>
#include <QToolBar>
#include <QPushButton>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QCloseEvent>
#include <QTimer>

#include <exception>
#include <unordered_map>

MainWindow::MainWindow(const UsuarioSesion& sesion, QWidget* parent)
    : QMainWindow(parent), m_sesion(sesion) {
    setupUi();
    aplicarPermisosPorPagina();
}

void MainWindow::setupUi() {
    int orientacion = 1;  // 0: superior, 1: izquierda, 2: inferior, 3: derecha
    // Cargar nombre de la copropiedad en el título de la ventana
    QString nombreCopropiedad = "Administración Propiedad Horizontal";
    auto& db = DatabaseManager::instance().getDb();
    Db::Statement q(db, "SELECT razon_social, orientacion_menu FROM copropiedad_config WHERE id = 1");
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
    m_tabConfig           = new ConfiguracionWidget(m_sesion, this);
    m_tabCuentasContables = new PucCuentasWidget(m_sesion.id, this);
    if (PermisosPaginasRepository::puedeGestionarPermisos(m_sesion.id)) {
        m_tabPermisosPaginas = new PermisosPaginasWidget(m_sesion.id, this);
    }

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

    const std::unordered_map<std::string, QWidget*> widgetsPorPagina{
        {"FACTURAS_SERVICIO", m_tabFacturasServicio},
        {"UNIDADES", m_tabUnidades},
        {"TERCEROS", m_tabTerceros},
        {"PARQUEADEROS", m_tabParqueaderos},
        {"FACTURACION", m_tabFacturacion},
        {"RECAUDO", m_tabRecaudo},
        {"EGRESOS", m_tabEgresos},
        {"PRESUPUESTO", m_tabPresupuesto},
        {"REPORTES", m_tabReportes},
        {"ASAMBLEAS", m_tabAsambleas},
        {"SEGURIDAD", m_tabSeguridad},
        {"CONCEPTOS_COBRO", m_tabConceptosCobro},
        {"CONFIGURACION", m_tabConfig},
        {"CUENTAS_CONTABLES", m_tabCuentasContables},
        {"GESTION_PERMISOS", m_tabPermisosPaginas}
    };
    for (const auto& pagina : PermisosPaginasRepository::obtenerCatalogoPaginas()) {
        const auto widget = widgetsPorPagina.find(pagina.codigo);
        if (widget == widgetsPorPagina.end()) {
            qWarning() << "La página del catálogo no tiene un widget asociado:"
                       << QString::fromStdString(pagina.codigo);
            continue;
        }
        if (!widget->second) {
            continue;
        }
        m_tabs->addTab(
            widget->second,
            QString("%1. %2").arg(pagina.orden).arg(QString::fromStdString(pagina.nombre))
        );
    }
    if (m_tabPermisosPaginas) {
        connect(m_tabPermisosPaginas, &PermisosPaginasWidget::configuracionActualizada,
                this, &MainWindow::aplicarPermisosPorPagina);
    }

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

    if (DatabaseManager::instance().isServerMode()) {
        auto* btnApagarBase = new QPushButton("⏻ Salir y apagar PostgreSQL");
        btnApagarBase->setStyleSheet(
            "QPushButton { background-color: #7f1d1d; color: white; font-weight: bold; "
            "padding: 6px 16px; border-radius: 4px; }"
            "QPushButton:hover { background-color: #641616; }"
        );
        toolbar->addWidget(btnApagarBase);

        connect(btnApagarBase, &QPushButton::clicked, this, [this]() {
            const auto response = QMessageBox::warning(
                this,
                "Apagar PostgreSQL",
                "Se apagará la base de datos y se desconectarán los clientes conectados. "
                "¿Desea continuar?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (response == QMessageBox::Yes) {
                m_shutdownDatabaseOnExit = true;
                close();
            }
        });
    }

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
        QString("Usuario activo: %1 (%2)  |  Perfil: %3  |  Base de Datos: PostgreSQL")
            .arg(QString::fromStdString(m_sesion.nombreCompleto))
            .arg(QString::fromStdString(m_sesion.username))
            .arg(QString::fromStdString(m_sesion.rolNombre))
    );
}

void MainWindow::aplicarPermisosPorPagina() {
    const std::unordered_map<std::string, QWidget*> widgetsPorPagina{
        {"FACTURAS_SERVICIO", m_tabFacturasServicio},
        {"UNIDADES", m_tabUnidades},
        {"TERCEROS", m_tabTerceros},
        {"PARQUEADEROS", m_tabParqueaderos},
        {"FACTURACION", m_tabFacturacion},
        {"RECAUDO", m_tabRecaudo},
        {"EGRESOS", m_tabEgresos},
        {"PRESUPUESTO", m_tabPresupuesto},
        {"REPORTES", m_tabReportes},
        {"ASAMBLEAS", m_tabAsambleas},
        {"SEGURIDAD", m_tabSeguridad},
        {"CONCEPTOS_COBRO", m_tabConceptosCobro},
        {"CONFIGURACION", m_tabConfig},
        {"CUENTAS_CONTABLES", m_tabCuentasContables},
        {"GESTION_PERMISOS", m_tabPermisosPaginas}
    };

    try {
        QWidget* paginaSeleccionada = m_tabs->currentWidget();
        int posicion = 0;
        for (const auto& pagina : PermisosPaginasRepository::obtenerCatalogoPaginas()) {
            const auto widget = widgetsPorPagina.find(pagina.codigo);
            if (widget == widgetsPorPagina.end()) {
                qWarning() << "La página del catálogo no tiene un widget asociado:"
                           << QString::fromStdString(pagina.codigo);
                continue;
            }
            if (!widget->second) {
                continue;
            }

            const int indexActual = m_tabs->indexOf(widget->second);
            if (indexActual >= 0) {
                const QString titulo = QString("%1. %2").arg(pagina.orden)
                    .arg(QString::fromStdString(pagina.nombre));
                const bool visible = m_tabs->isTabVisible(indexActual);
                if (indexActual != posicion || m_tabs->tabText(indexActual) != titulo) {
                    m_tabs->removeTab(indexActual);
                    const int nuevoIndice = m_tabs->insertTab(posicion, widget->second, titulo);
                    m_tabs->setTabVisible(nuevoIndice, visible);
                }
                m_tabs->setTabVisible(
                    posicion,
                    PermisosPaginasRepository::tieneAcceso(m_sesion.rolId, pagina.codigo));
                ++posicion;
            }
        }
        const int indiceSeleccionado = m_tabs->indexOf(paginaSeleccionada);
        if (indiceSeleccionado >= 0 && m_tabs->isTabVisible(indiceSeleccionado)) {
            m_tabs->setCurrentIndex(indiceSeleccionado);
        }
    } catch (const std::exception& error) {
        for (const auto& [codigo, widget] : widgetsPorPagina) {
            Q_UNUSED(codigo);
            const int index = m_tabs->indexOf(widget);
            if (index >= 0) {
                m_tabs->setTabVisible(index, false);
            }
        }
        if (m_tabPermisosPaginas) {
            m_tabs->setTabVisible(m_tabs->indexOf(m_tabPermisosPaginas), false);
        }
        QMessageBox::critical(
            this, "Error de permisos",
            QString("No se pudieron cargar los permisos de páginas:\n%1")
                .arg(QString::fromUtf8(error.what())));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_shutdownDatabaseOnExit) {
        const auto response = QMessageBox::question(
            this,
            "Salir",
            "¿Desea salir de la aplicación?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (response != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    // Registrar en auditoría
    try {
        auto& db = DatabaseManager::instance().getDb();
        std::string ts = QDateTime::currentDateTime()
            .toString("yyyy-MM-dd HH:mm:ss").toStdString();
        Db::Statement log(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'PRINCIPAL', 'SALIR', 'usuarios', ?, ?)
        )");
        log.bind(1, m_sesion.id);
        log.bind(2, m_sesion.id);
        const std::string action = m_shutdownDatabaseOnExit
            ? "Cierre de sesión y apagado de PostgreSQL"
            : "Cierre controlado de sesión";
        log.bind(3, "[" + ts + "] - " + action);
        log.exec();
    } catch (const std::exception& e) {
        qWarning() << "Error al registrar salida en auditoría:" << e.what();
    }

    if (m_shutdownDatabaseOnExit) {
        QString errorMessage;
        if (!DatabaseManager::instance().stopLocalServer(&errorMessage)) {
            m_shutdownDatabaseOnExit = false;
            QMessageBox::critical(
                this,
                "Error al apagar PostgreSQL",
                "No se pudo apagar la base de datos:\n" + errorMessage
            );
            event->ignore();
            return;
        }
    }

    event->accept();  // ← permite el cierre
}
