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


#include "ConfiguracionWidget.hpp"
#include "core/DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <cmath>
#include <format>
#include <QHeaderView>
// #include <QDate>


ConfiguracionWidget::ConfiguracionWidget(const UsuarioSesion& sesion,
                                         const std::string& rutaDbActual,
                                         QWidget* parent)
    : QWidget(parent), m_sesion(sesion), m_rutaDb(rutaDbActual) {
    setupUi();
    cargarDatos();
    cargarHistorialTasas();
}

void ConfiguracionWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel(
        "<h3>Parámetros Generales e Historial de Tasas de Mora (Ley 675 / Superfinanciera)</h3>", 
        this
    ));

    auto* columns = new QHBoxLayout();

    // --- COLUMNA 1: DATOS LEGALES, POLÍTICAS Y RESPALDO ---
    auto* leftCol = new QVBoxLayout();

    auto* legalGroup = new QGroupBox("1. Identificación Legal y Órganos de Administración", this);
    auto* legalForm = new QFormLayout(legalGroup);

    m_txtNit = new QLineEdit(this);
    m_txtNit->setPlaceholderText("Ej: 900.123.456-7");

    m_txtRazonSocial = new QLineEdit(this);
    m_txtRazonSocial->setPlaceholderText("Ej: CONJUNTO RESIDENCIAL LOS RESIDENTES P.H.");

    m_txtDireccion      = new QLineEdit(this);
    m_txtTelefono       = new QLineEdit(this);
    m_txtEmail          = new QLineEdit(this);
    m_txtRepLegal       = new QLineEdit(this);
    m_txtContadorNombre = new QLineEdit(this);
    m_txtContadorTp     = new QLineEdit(this);
    m_txtRevisorNombre  = new QLineEdit(this);
    m_txtRevisorTp      = new QLineEdit(this);

    m_txtContadorTp->setPlaceholderText("Tarjeta Profesional");
    m_txtRevisorTp->setPlaceholderText("Tarjeta Profesional (si aplica)");

    legalForm->addRow("NIT Copropiedad *:", m_txtNit);
    legalForm->addRow("Razón Social *:", m_txtRazonSocial);
    legalForm->addRow("Dirección *:", m_txtDireccion);
    legalForm->addRow("Teléfono:", m_txtTelefono);
    legalForm->addRow("Correo Institucional:", m_txtEmail);
    legalForm->addRow("Administrador / Rep. Legal *:", m_txtRepLegal);
    legalForm->addRow("Contador(a) Público:", m_txtContadorNombre);
    legalForm->addRow("T.P. Contador(a):", m_txtContadorTp);
    legalForm->addRow("Revisor(a) Fiscal:", m_txtRevisorNombre);
    legalForm->addRow("T.P. Revisor(a) Fiscal:", m_txtRevisorTp);

    auto* finGroup = new QGroupBox("2. Políticas Generales, Mora y Descuentos", this);
    auto* finForm = new QFormLayout(finGroup);

    m_spinTasaMoraPct = new QDoubleSpinBox(this);
    /*
    m_spinTasaMoraPct->setRange(0.0, 5.0);
    m_spinTasaMoraPct->setDecimals(4);
    // m_spinTasaMoraPct->setSingleStep(0.05);
    m_spinTasaMoraPct->setValue(1.8000);
    m_spinTasaMoraPct->setSuffix(" % mensual");
     */

    m_spinDiaProntoPago = new QSpinBox(this);
    m_spinDiaProntoPago->setRange(1, 28);
    m_spinDiaProntoPago->setValue(10);
    m_spinDiaProntoPago->setPrefix("Día ");

    m_spinDescuentoProntoPagoPct = new QDoubleSpinBox(this);
    m_spinDescuentoProntoPagoPct->setRange(0.0, 30.0);
    m_spinDescuentoProntoPagoPct->setDecimals(2);
    m_spinDescuentoProntoPagoPct->setValue(10.0);
    m_spinDescuentoProntoPagoPct->setSuffix(" %");

    m_spinOrientacionMenu = new QComboBox(this);
    m_spinOrientacionMenu->addItem("Arriba", 0);
    m_spinOrientacionMenu->addItem("Izquierda", 1);
    m_spinOrientacionMenu->addItem("Abajo", 2);
    m_spinOrientacionMenu->addItem("Derecha", 3);
    // m_spinOrientacionMenu->setCurrentIndex();
    m_spinOrientacionMenu->setToolTip("Orientación del menú de pestañas");

    m_btnBackup = new QPushButton("Exportar Copia de Seguridad (.db)", this);
    m_btnBackup->setStyleSheet("background-color: #455A64; color: white; font-weight: bold; padding: 6px;");
    connect(m_btnBackup, &QPushButton::clicked, this, &ConfiguracionWidget::onCrearBackupClicked);

    // finForm->addRow("Tasa Interés Mora (Art. 30):", m_spinTasaMoraPct);
    finForm->addRow("Límite Pronto Pago:", m_spinDiaProntoPago);
    finForm->addRow("Descuento Pronto Pago:", m_spinDescuentoProntoPagoPct);
    finForm->addRow("Respaldo Base de Datos:", m_btnBackup);
    finForm->addRow("Orientación del Menú:", m_spinOrientacionMenu);

    m_btnGuardar = new QPushButton("Guardar Datos Generales de la Copropiedad", this);
    m_btnGuardar->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 8px;");
    bool puedeEditar = (m_sesion.rolNombre == "ADMIN" || m_sesion.rolNombre == "CONTADOR");
    m_btnGuardar->setEnabled(puedeEditar);
    connect(m_btnGuardar, &QPushButton::clicked, this, &ConfiguracionWidget::onGuardarClicked);

    leftCol->addWidget(legalGroup);
    leftCol->addWidget(finGroup);
    leftCol->addWidget(m_btnGuardar);

    // --- COLUMNA 2: PARÁMETROS FINANCIEROS Y RESPALDO ---
    auto* rightGroup    = new QGroupBox("3. Historial Mensual de Tasas de Mora (Certificación Superfinanciera)", this);
    auto* rightLayout   = new QVBoxLayout(rightGroup);
    auto* tasaForm      = new QFormLayout();

    m_txtPeriodoTasa = new QLineEdit(this);
    m_txtPeriodoTasa->setText(QDate::currentDate().toString("yyyy-MM"));
    m_txtPeriodoTasa->setPlaceholderText("YYYY-MM (Ej: 2026-11)");

    m_spinTasaEA = new QDoubleSpinBox(this);
    m_spinTasaEA->setRange(0.0, 60.0);
    m_spinTasaEA->setDecimals(2);
    m_spinTasaEA->setSingleStep(0.10);
    m_spinTasaEA->setValue(12.34);
    m_spinTasaEA->setSuffix(" % T.E.Anual.");

    m_spinTasaMensualCalc = new QDoubleSpinBox(this);
    m_spinTasaMensualCalc->setRange(0.0, 10.0);
    m_spinTasaMensualCalc->setDecimals(3);
    m_spinTasaMensualCalc->setSuffix(" % T.E.Mensual");

    m_txtResolucionSfc = new QLineEdit(this);
    m_txtResolucionSfc->setPlaceholderText("Ej: Resolución SFC 1890 / Acta Asamblea");

    connect(
        m_spinTasaEA, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &ConfiguracionWidget::onTasaEACambiada
    );

    m_btnGuardarTasaMes = new QPushButton("Registrar Tasa del Mes en Historial", this);
    m_btnGuardarTasaMes->setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 7px;");
    m_btnGuardarTasaMes->setEnabled(puedeEditar);
    connect(m_btnGuardarTasaMes, &QPushButton::clicked,
            this, &ConfiguracionWidget::onRegistrarTasaMensualClicked);

    tasaForm->addRow("Periodo (YYYY-MM) *:", m_txtPeriodoTasa);
    tasaForm->addRow("Tasa Usura Superfinanciera (% E.A.):", m_spinTasaEA);
    tasaForm->addRow("Tasa Mensual a Aplicar:", m_spinTasaMensualCalc);
    tasaForm->addRow("Resolución / Soporte:", m_txtResolucionSfc);
    tasaForm->addRow(m_btnGuardarTasaMes);

    // Calcular conversión inicial
    onTasaEACambiada(m_spinTasaEA->value());

    m_tablaTasasHistorial = new QTableWidget(this);
    m_tablaTasasHistorial->setColumnCount(5);
    m_tablaTasasHistorial->setHorizontalHeaderLabels({
        "Periodo", "Tasa E.A.", "Tasa Mensual", "Resolución / Soporte", "Fecha Registro"
    });

    m_tablaTasasHistorial->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablaTasasHistorial->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    rightLayout->addLayout(tasaForm);
    rightLayout->addWidget(new QLabel("<b>Histórico de Tasas Aplicadas por Mes:</b>", this));
    rightLayout->addWidget(m_tablaTasasHistorial);

    columns->addLayout(leftCol, 1);
    columns->addWidget(rightGroup, 1);

    // --- Validaciones y formateo en tiempo real ---
    connect(m_txtNit, &QLineEdit::textChanged, this, &ConfiguracionWidget::onNitTextChanged);
    connect(m_txtTelefono, &QLineEdit::textChanged, this, &ConfiguracionWidget::onTelefonoTextChanged);
    connect(m_txtEmail, &QLineEdit::textChanged, this, &ConfiguracionWidget::onEmailTextChanged);   

    mainLayout->addLayout(columns);
}

void ConfiguracionWidget::onTasaEACambiada(double tasaEaPct) {
    // Conversión financiera exacta de Efectiva Anual (EA) a Tasa Mensual Vencida (TMV)
    double eaDecimal = tasaEaPct / 100.0;

    /*
     * La Tasa Efectiva Anual (TEA) se incrementa en 1.5 para hayar el máximo de usura
     * y es la base para calcular la Tasa Efectiva Mensual.
     * (1 + TEA * 1.5)^(1/12) - 1
     */
    double mensualDecimal = std::pow(1.0 + eaDecimal * 1.5, 1.0 / 12.0) - 1.0;
    m_spinTasaMensualCalc->setValue(mensualDecimal * 100.0);
}

void ConfiguracionWidget::onRegistrarTasaMensualClicked() {
    std::string periodo = m_txtPeriodoTasa->text().trimmed().toStdString();
    if (periodo.size() != 7 || periodo != "-") {
        QMessageBox::warning(this, "Validación", "El periodo debe tener formato YYYY-MM (ej: 2026-11).");
        return;
    }

    double tasaEaDecimal = m_spinTasaEA->value() / 100.0;
    double tasaMensualDecimal = m_spinTasaMensualCalc->value() / 100.0;
    std::string resolucion = m_txtResolucionSfc->text().trimmed().toStdString();

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);

    // Insertar o actualizar la tasa del periodo en el historial
    SQLite::Statement stmt(db, R"(
        INSERT INTO tasas_mora_historial (
            periodo, tasa_efectiva_anual, tasa_mensual_aplicada, resolucion_sfc, registrado_por_id
        ) VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(periodo) DO UPDATE SET
            tasa_efectiva_anual = excluded.tasa_efectiva_anual,
            tasa_mensual_aplicada = excluded.tasa_mensual_aplicada,
            resolucion_sfc = excluded.resolucion_sfc,
            registrado_por_id = excluded.registrado_por_id,
            fecha_registro = datetime('now', 'localtime')
    )");

    stmt.bind(1, periodo);
    stmt.bind(2, tasaEaDecimal);
    stmt.bind(3, tasaMensualDecimal);
    stmt.bind(4, resolucion);
    stmt.bind(5, m_sesion.id);
    stmt.exec();

    // Actualizar también la tasa vigente por defecto si ya existe configuración general
    SQLite::Statement updCfg(db, "UPDATE copropiedad_config SET tasa_mora_mensual = ? WHERE id = 1");
    updCfg.bind(1, tasaMensualDecimal);
    updCfg.exec();

    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
        VALUES (?, 'CONFIGURACION', 'EDITAR', 'tasas_mora_historial', 0, ?)
    )");
    audit.bind(1, m_sesion.id);
    audit.bind(2, std::format(
        "Tasa mora {} registrada: {:.3f}% mensual ({:.2f}% EA)",
        periodo, tasaMensualDecimal * 100.0, 
        tasaEaDecimal * 100.0
    ));
    audit.exec();

    transaction.commit();

    // m_spinTasaMoraPct->setValue(tasaMensualDecimal * 100.0);
    m_txtResolucionSfc->clear();
    cargarHistorialTasas();

    QMessageBox::information(
        this, "Tasa Registrada",
        QString("Se registró la tasa de mora del %1% mensual para el periodo %2.")
            .arg(m_spinTasaMensualCalc->value(), 0, 'f', 3)
            .arg(QString::fromStdString(periodo))
    );
}

void ConfiguracionWidget::cargarHistorialTasas() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement q(db, R"(
        SELECT periodo, tasa_efectiva_anual, tasa_mensual_aplicada,
        COALESCE(resolucion_sfc, ''), fecha_registro
        FROM tasas_mora_historial
        ORDER BY periodo DESC
    )");
    m_tablaTasasHistorial->setRowCount(0);
    int row = 0;
    while (q.executeStep()) {
        m_tablaTasasHistorial->insertRow(row);
        m_tablaTasasHistorial->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(q.getColumn(0).getString())));
        m_tablaTasasHistorial->setItem(row, 1, new QTableWidgetItem(QString::number(q.getColumn(1).getDouble() * 100.0, 'f', 2) + "%"));
        m_tablaTasasHistorial->setItem(row, 2, new QTableWidgetItem(QString::number(q.getColumn(2).getDouble() * 100.0, 'f', 3) + "%"));
        m_tablaTasasHistorial->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(q.getColumn(3).getString())));
        m_tablaTasasHistorial->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(q.getColumn(4).getString())));
        row++;
    }
}

void ConfiguracionWidget::cargarDatos() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement q(db, R"(
        SELECT nit, razon_social, direccion, COALESCE(telefono, ''), COALESCE(email, ''),
        representante_legal, COALESCE(contador_nombre, ''), COALESCE(contador_tp, ''),
        COALESCE(revisor_fiscal_nombre, ''), COALESCE(revisor_fiscal_tp, ''),
        tasa_mora_mensual, dia_limite_pronto_pago, porcentaje_descuento_pronto_pago, orientacion_menu
        FROM copropiedad_config WHERE id = 1
    )");
    if (q.executeStep()) {
        m_txtNit->setText(QString::fromStdString(q.getColumn(0).getString()));
        m_txtRazonSocial->setText(QString::fromStdString(q.getColumn(1).getString()));
        m_txtDireccion->setText(QString::fromStdString(q.getColumn(2).getString()));
        m_txtTelefono->setText(QString::fromStdString(q.getColumn(3).getString()));
        m_txtEmail->setText(QString::fromStdString(q.getColumn(4).getString()));
        m_txtRepLegal->setText(QString::fromStdString(q.getColumn(5).getString()));
        m_txtContadorNombre->setText(QString::fromStdString(q.getColumn(6).getString()));
        m_txtContadorTp->setText(QString::fromStdString(q.getColumn(7).getString()));
        m_txtRevisorNombre->setText(QString::fromStdString(q.getColumn(8).getString()));
        m_txtRevisorTp->setText(QString::fromStdString(q.getColumn(9).getString()));
        m_spinTasaMoraPct->setValue(q.getColumn(10).getDouble() * 100.0);
        m_spinDiaProntoPago->setValue(q.getColumn(11).getInt());
        m_spinDescuentoProntoPagoPct->setValue(q.getColumn(12).getDouble());
        m_spinOrientacionMenu->setCurrentIndex(q.getColumn(13).getInt());
    }
}

void ConfiguracionWidget::onGuardarClicked() {
    QString nit = m_txtNit->text().trimmed();
    QString razon = m_txtRazonSocial->text().trimmed();
    QString dir = m_txtDireccion->text().trimmed();
    QString rep = m_txtRepLegal->text().trimmed();
    if (nit.isEmpty() || razon.isEmpty() || dir.isEmpty() || rep.isEmpty()) {
        QMessageBox::warning(this, "Validación", "NIT, Razón Social, Dirección y Representante Legal son obligatorios.");
        return;
    }

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);
    try {
        SQLite::Statement stmt(db, R"(
            INSERT INTO copropiedad_config (
                id, nit, razon_social, direccion, telefono, email, representante_legal, contador_nombre, contador_tp,
                revisor_fiscal_nombre, revisor_fiscal_tp, tasa_mora_mensual, dia_limite_pronto_pago, 
                porcentaje_descuento_pronto_pago, orientacion_menu
            ) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(id) DO UPDATE SET
                nit = excluded.nit,
                razon_social = excluded.razon_social,
                direccion = excluded.direccion,
                telefono = excluded.telefono,
                email = excluded.email,
                representante_legal = excluded.representante_legal,
                contador_nombre = excluded.contador_nombre,
                contador_tp = excluded.contador_tp,
                revisor_fiscal_nombre = excluded.revisor_fiscal_nombre,
                revisor_fiscal_tp = excluded.revisor_fiscal_tp,
                tasa_mora_mensual = excluded.tasa_mora_mensual,
                dia_limite_pronto_pago = excluded.dia_limite_pronto_pago,
                porcentaje_descuento_pronto_pago = excluded.porcentaje_descuento_pronto_pago,
                orientacion_menu = excluded.orientacion_menu
        )");
        stmt.bind(1, nit.toStdString());
        stmt.bind(2, razon.toStdString());
        stmt.bind(3, dir.toStdString());
        stmt.bind(4, m_txtTelefono->text().trimmed().toStdString());
        stmt.bind(5, m_txtEmail->text().trimmed().toStdString());
        stmt.bind(6, rep.toStdString());
        stmt.bind(7, m_txtContadorNombre->text().trimmed().toStdString());
        stmt.bind(8, m_txtContadorTp->text().trimmed().toStdString());
        stmt.bind(9, m_txtRevisorNombre->text().trimmed().toStdString());
        stmt.bind(10, m_txtRevisorTp->text().trimmed().toStdString());
        stmt.bind(11, 0.0);
        // stmt.bind(11, m_spinTasaMoraPct->value() / 100.0);
        stmt.bind(12, m_spinDiaProntoPago->value());
        stmt.bind(13, m_spinDescuentoProntoPagoPct->value());
        stmt.bind(14, m_spinOrientacionMenu->currentData().toInt());
        stmt.exec();
    } catch (const std::exception& e) {
        transaction.rollback();
        QMessageBox::warning(
            this, 
            "Error", 
            QString("Error ejecutando query: INSERT INTO copropiedad_config - Error: %1").arg(e.what())
        );
        qCritical() << "Error ejecutando query: INSERT INTO copropiedad_config - " << e.what();
        return;
    }

    try {
        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'CONFIGURACION', 'EDITAR', 'copropiedad_config', 1, 'Actualización parámetros generales')
        )");
        audit.bind(1, m_sesion.id);
        audit.exec();
    } catch (const std::exception& e) {
        QMessageBox::warning(
            this, 
            "Error", 
            QString("Error almacenando en el log, aunque el registro de guardará - Error: %1").arg(e.what())
        );
    }

    transaction.commit();
    QMessageBox::information(this, "Guardado", "Configuración de la copropiedad actualizada correctamente.");
    emit configuracionActualizada(razon);
}

void ConfiguracionWidget::onCrearBackupClicked() {
    QString sugerido = QString("Backup_PH_%1.db")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QString destino = QFileDialog::getSaveFileName(
        this, "Guardar Copia de Seguridad SQLite", sugerido, "Base de Datos SQLite (*.db)"
    );

    if (destino.isEmpty()) return;

    DatabaseManager::instance().getDb().exec("PRAGMA wal_checkpoint(TRUNCATE);");
    if (QFile::exists(destino)) {
        QFile::remove(destino);
    }

    if (QFile::copy(QString::fromStdString(m_rutaDb), destino)) {
        QMessageBox::information(this, "Respaldo Exitoso", "Copia de seguridad creada correctamente.");
    } else {
        QMessageBox::critical(this, "Error", "No se pudo copiar el archivo de base de datos.");
    }
}

// ============================================================
// NIT: solo números, formateo XXX.XXX.XXX-D y cálculo del DV
// ============================================================

void ConfiguracionWidget::onNitTextChanged(const QString& text) {
    // Extraer solo dígitos
    QString digits;
    for (QChar c : text) {
        if (c.isDigit()) digits += c;
    }

    // Limitar a 9 dígitos (base del NIT para persona jurídica)
    if (digits.size() > 9) digits = digits.left(9);

    // Reformatar
    QString formatted = formatNit(digits);

    // Si hay 9 dígitos, calcular y anexar el dígito de verificación
    if (digits.size() == 9) {
        int dv = calcularDvNit(digits);
        formatted += "-" + QString::number(dv);
    }

    // Evitar bucle infinito al reescribir el texto
    if (formatted != text) {
        m_txtNit->blockSignals(true);
        m_txtNit->setText(formatted);
        m_txtNit->blockSignals(false);
    }
}

QString ConfiguracionWidget::formatNit(const QString& digits) {
    if (digits.size() <= 3) return digits;
    if (digits.size() <= 6) return digits.left(3) + "." + digits.mid(3);
    // 7-9 dígitos: XXX.XXX.XXX
    QString result = digits.left(3) + "." + digits.mid(3, 3);
    if (digits.size() > 6) result += "." + digits.mid(6);
    return result;
}

int ConfiguracionWidget::calcularDvNit(const QString& digits) {
    // Pesos primos de la DIAN (derecha a izquierda)
    static const int pesos[] = {3, 7, 13, 17, 19, 23, 29, 37, 41, 43, 47, 53, 59, 67, 71};

    int suma = 0;
    int n = digits.size();

    // Recorrer de derecha a izquierda
    for (int i = 0; i < n; ++i) {
        int digito = digits[n - 1 - i].unicode() - '0';
        suma += digito * pesos[i];
    }

    int residuo = suma % 11;
    if (residuo < 2) return residuo;
    int dv = 11 - residuo;
    return (dv == 11) ? 0 : dv;
}


// ============================================================
// Teléfono: solo números, formateo según tipo (fijo/celular)
// ============================================================

void ConfiguracionWidget::onTelefonoTextChanged(const QString& text) {
    // Extraer solo dígitos
    QString digits;
    for (QChar c : text) {
        if (c.isDigit()) digits += c;
    }

    // Limitar a 10 dígitos (formato unificado Colombia desde 2021)
    if (digits.size() > 10) digits = digits.left(10);

    QString formatted = formatTelefono(digits);

    if (formatted != text) {
        m_txtTelefono->blockSignals(true);
        m_txtTelefono->setText(formatted);
        m_txtTelefono->blockSignals(false);
    }
}

QString ConfiguracionWidget::formatTelefono(const QString& digits) {
    int n = digits.size();

    if (n <= 3) return digits;

    // Celular: inicia con 3 → XXX XXX XXXX
    if (n >= 10 && digits[0] == '3') {
        return digits.left(3) + " " + digits.mid(3, 3) + " " + digits.mid(6);
    }

    // Fijo: inicia con 6 → 60X XXX XXXX
    if (n >= 10 && digits[0] == '6') {
        return digits.left(3) + " " + digits.mid(3, 3) + " " + digits.mid(6);
    }

    // En construcción: agrupar de 3 en 3
    QString result;
    for (int i = 0; i < n; ++i) {
        if (i > 0 && i % 3 == 0) result += " ";
        result += digits[i];
    }
    return result;
}


// ============================================================
// Email: validación en tiempo real
// ============================================================

void ConfiguracionWidget::onEmailTextChanged(const QString& text) {
    if (text.isEmpty()) return;

    if (!validarEmail(text)) {
        m_txtEmail->setStyleSheet("border: 2px solid #e53935;");  // rojo
        m_txtEmail->setToolTip("Formato de correo no válido");
    } else {
        m_txtEmail->setStyleSheet("");
        m_txtEmail->setToolTip("");
    }
}

bool ConfiguracionWidget::validarEmail(const QString& email) {
    // Reglas: local@dominio.tld
    // - Local: letras, dígitos, . _ % + -
    // - Dominio: letras, dígitos, . -
    // - TLD: mínimo 2 caracteres alfabéticos
    static const QRegularExpression re(
        R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)"
    );

    if (!re.match(email).hasMatch()) return false;

    // Verificaciones adicionales:
    // 1. No debe tener doble punto
    if (email.contains("..")) return false;

    // 2. Local no debe iniciar ni terminar con punto
    int atPos = email.indexOf('@');
    QString local = email.left(atPos);
    if (local.startsWith('.') || local.endsWith('.')) return false;

    // 3. Dominio no debe iniciar ni terminar con guion o punto
    QString domain = email.mid(atPos + 1);
    if (domain.startsWith('.') || domain.endsWith('.') ||
        domain.startsWith('-') || domain.endsWith('-')) return false;

    // 4. Longitud máxima total: 254 caracteres (RFC 5321)
    if (email.size() > 254) return false;

    return true;
}   