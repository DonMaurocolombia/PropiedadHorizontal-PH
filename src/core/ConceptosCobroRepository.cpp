// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ConceptosCobroRepository.hpp"

#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
void validarCodigo(const std::string& codigo) {
    if (codigo.size() < 2 || codigo.size() > 30
        || !std::all_of(codigo.begin(), codigo.end(), [](unsigned char ch) {
               return std::isupper(ch) || std::isdigit(ch) || ch == '_';
           })) {
        throw std::invalid_argument("El código debe tener entre 2 y 30 caracteres en mayúsculas, números o guion bajo.");
    }
}

void validarCuenta(const std::string& codigo,
                   const std::string& claseEsperada,
                   const std::string& naturalezaEsperada) {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT clase, naturaleza, permite_movimiento
        FROM puc_cuentas
        WHERE codigo = ?
    )");
    query.bind(1, codigo);
    if (!query.executeStep()
        || query.getColumn(0).getString() != claseEsperada
        || query.getColumn(1).getString() != naturalezaEsperada
        || query.getColumn(2).getInt() != 1) {
        throw std::invalid_argument("La cuenta PUC seleccionada no existe, no permite movimientos o tiene una clasificación incorrecta.");
    }
}

int cantidadAsociaciones(int conceptoId) {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT
            (SELECT COUNT(*) FROM cuentas_cobro_detalle WHERE concepto_id = ?)
            + (SELECT COUNT(*) FROM facturas_servicio_cobro WHERE concepto_id = ?)
    )");
    query.bind(1, conceptoId);
    query.bind(2, conceptoId);
    query.executeStep();
    return query.getColumn(0).getInt();
}

void registrarAuditoria(int usuarioId,
                        const std::string& accion,
                        int conceptoId,
                        const std::string& detalle) {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement audit(db, R"(
        INSERT INTO auditoria_logs (
            usuario_id, modulo, accion, tabla_afectada, registro_id, detalle
        ) VALUES (?, 'CONCEPTOS_COBRO', ?, 'conceptos_cobro', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, accion);
    audit.bind(3, conceptoId);
    audit.bind(4, detalle);
    audit.exec();
}
}

std::vector<ConceptoCobroServicio> ConceptosCobroRepository::listarServicios() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT c.id, c.codigo, c.nombre,
               c.cuenta_ingreso_codigo, i.nombre,
               c.cuenta_cartera_codigo, a.nombre
        FROM conceptos_cobro c
        INNER JOIN puc_cuentas i ON i.codigo = c.cuenta_ingreso_codigo
        INNER JOIN puc_cuentas a ON a.codigo = c.cuenta_cartera_codigo
        WHERE c.tipo_concepto = 'SERVICIO'
        ORDER BY c.codigo
    )");

    std::vector<ConceptoCobroServicio> conceptos;
    while (query.executeStep()) {
        conceptos.push_back({
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getString(),
            query.getColumn(3).getString(),
            query.getColumn(4).getString(),
            query.getColumn(5).getString(),
            query.getColumn(6).getString()
        });
    }
    return conceptos;
}

std::vector<CuentaPucOpcion> ConceptosCobroRepository::listarCuentasIngreso() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT codigo, nombre
        FROM puc_cuentas
        WHERE clase = 'INGRESOS' AND naturaleza = 'CREDITO' AND permite_movimiento = 1
        ORDER BY codigo
    )");
    std::vector<CuentaPucOpcion> cuentas;
    while (query.executeStep()) {
        cuentas.push_back({query.getColumn(0).getString(), query.getColumn(1).getString()});
    }
    return cuentas;
}

std::vector<CuentaPucOpcion> ConceptosCobroRepository::listarCuentasCartera() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT codigo, nombre
        FROM puc_cuentas
        WHERE clase = 'ACTIVO' AND naturaleza = 'DEBITO' AND permite_movimiento = 1
        ORDER BY codigo
    )");
    std::vector<CuentaPucOpcion> cuentas;
    while (query.executeStep()) {
        cuentas.push_back({query.getColumn(0).getString(), query.getColumn(1).getString()});
    }
    return cuentas;
}

std::vector<CuentaPucOpcion> ConceptosCobroRepository::listarCuentasPago() {
    auto& db = DatabaseManager::instance().getDb();
    SQLite::Statement query(db, R"(
        SELECT codigo, nombre
        FROM puc_cuentas
        WHERE clase = 'ACTIVO' AND naturaleza = 'DEBITO'
          AND permite_movimiento = 1 AND codigo LIKE '11%'
        ORDER BY codigo
    )");
    std::vector<CuentaPucOpcion> cuentas;
    while (query.executeStep()) {
        cuentas.push_back({query.getColumn(0).getString(), query.getColumn(1).getString()});
    }
    return cuentas;
}

int ConceptosCobroRepository::crearServicio(const std::string& codigo,
                                            const std::string& nombre,
                                            const std::string& cuentaIngresoCodigo,
                                            const std::string& cuentaCarteraCodigo,
                                            int usuarioId) {
    validarCodigo(codigo);
    if (nombre.empty() || usuarioId <= 0) {
        throw std::invalid_argument("El nombre del cobro y el usuario son obligatorios.");
    }
    validarCuenta(cuentaIngresoCodigo, "INGRESOS", "CREDITO");
    validarCuenta(cuentaCarteraCodigo, "ACTIVO", "DEBITO");

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);
    SQLite::Statement insert(db, R"(
        INSERT INTO conceptos_cobro (
            codigo, nombre, cuenta_ingreso_codigo, cuenta_cartera_codigo,
            genera_interes_mora, aplica_iva, porcentaje_iva, tipo_concepto
        ) VALUES (?, ?, ?, ?, 0, 0, 0, 'SERVICIO')
    )");
    insert.bind(1, codigo);
    insert.bind(2, nombre);
    insert.bind(3, cuentaIngresoCodigo);
    insert.bind(4, cuentaCarteraCodigo);
    insert.exec();
    const int id = static_cast<int>(db.getLastInsertRowid());
    registrarAuditoria(usuarioId, "CREAR", id, "Código: " + codigo);
    transaction.commit();
    return id;
}

void ConceptosCobroRepository::actualizarServicio(int id,
                                                  const std::string& nombre,
                                                  const std::string& cuentaIngresoCodigo,
                                                  const std::string& cuentaCarteraCodigo,
                                                  int usuarioId) {
    if (id <= 0 || nombre.empty() || usuarioId <= 0) {
        throw std::invalid_argument("El concepto, su nombre y el usuario son obligatorios.");
    }
    validarCuenta(cuentaIngresoCodigo, "INGRESOS", "CREDITO");
    validarCuenta(cuentaCarteraCodigo, "ACTIVO", "DEBITO");

    auto& db = DatabaseManager::instance().getDb();
    std::string cuentaIngresoActual;
    std::string cuentaCarteraActual;
    {
        SQLite::Statement actual(db, R"(
            SELECT cuenta_ingreso_codigo, cuenta_cartera_codigo
            FROM conceptos_cobro
            WHERE id = ? AND tipo_concepto = 'SERVICIO'
        )");
        actual.bind(1, id);
        if (!actual.executeStep()) {
            throw std::invalid_argument("El concepto de servicio seleccionado no existe.");
        }
        cuentaIngresoActual = actual.getColumn(0).getString();
        cuentaCarteraActual = actual.getColumn(1).getString();
    }
    const bool cambioCuentas = cuentaIngresoActual != cuentaIngresoCodigo
        || cuentaCarteraActual != cuentaCarteraCodigo;
    if (cambioCuentas && cantidadAsociaciones(id) > 0) {
        throw std::runtime_error("No se pueden cambiar las cuentas: el concepto ya tiene facturas o cargos asociados.");
    }

    SQLite::Transaction transaction(db);
    SQLite::Statement update(db, R"(
        UPDATE conceptos_cobro
        SET nombre = ?, cuenta_ingreso_codigo = ?, cuenta_cartera_codigo = ?
        WHERE id = ? AND tipo_concepto = 'SERVICIO'
    )");
    update.bind(1, nombre);
    update.bind(2, cuentaIngresoCodigo);
    update.bind(3, cuentaCarteraCodigo);
    update.bind(4, id);
    update.exec();
    registrarAuditoria(usuarioId, "EDITAR", id, "Actualización de nombre o cuentas PUC");
    transaction.commit();
}

void ConceptosCobroRepository::eliminarServicio(int id, int usuarioId) {
    if (id <= 0 || usuarioId <= 0) {
        throw std::invalid_argument("Concepto o usuario inválido.");
    }
    if (cantidadAsociaciones(id) > 0) {
        throw std::runtime_error("No se puede eliminar el concepto porque tiene facturas o cuentas de cobro asociadas.");
    }

    auto& db = DatabaseManager::instance().getDb();
    SQLite::Transaction transaction(db);
    SQLite::Statement remove(db, R"(
        DELETE FROM conceptos_cobro
        WHERE id = ? AND tipo_concepto = 'SERVICIO'
    )");
    remove.bind(1, id);
    remove.exec();
    if (db.getChanges() != 1) {
        throw std::invalid_argument("El concepto de servicio seleccionado no existe.");
    }
    registrarAuditoria(usuarioId, "ELIMINAR", id, "Eliminación sin registros asociados");
    transaction.commit();
}
