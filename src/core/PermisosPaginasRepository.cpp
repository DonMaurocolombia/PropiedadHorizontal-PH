// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PermisosPaginasRepository.hpp"

#include "DbManager.hpp"

#include <QDebug>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>

namespace {

struct RolUsuario {
    int id = 0;
    std::string nombre;
};

RolUsuario obtenerRolUsuario(Db::Database& db, int usuarioId) {
    Db::Statement query(db, R"(
        SELECT r.id, r.nombre
        FROM usuarios u
        INNER JOIN roles r ON r.id = u.rol_id
        WHERE u.id = ? AND u.activo = 1
    )");
    query.bind(1, usuarioId);
    if (!query.executeStep()) {
        throw std::runtime_error("El usuario no existe o está inactivo.");
    }
    return {query.getColumn(0).getInt(), query.getColumn(1).getString()};
}

bool rolTieneAcceso(Db::Database& db, int rolId, const std::string& paginaCodigo) {
    Db::Statement query(db, R"(
        SELECT COALESCE(
            (SELECT habilitado
             FROM roles_permisos_pagina
             WHERE rol_id = ? AND pagina_codigo = ?),
            0
        )
    )");
    query.bind(1, rolId);
    query.bind(2, paginaCodigo);
    query.executeStep();
    return query.getColumn(0).getInt() != 0;
}

void validarPuedeGestionar(Db::Database& db, int usuarioId) {
    const RolUsuario rol = obtenerRolUsuario(db, usuarioId);
    if (rol.nombre != "ADMIN"
        && !rolTieneAcceso(db, rol.id, "GESTION_PERMISOS")) {
        throw std::runtime_error(
            "El perfil activo no tiene permiso para administrar los accesos a páginas."
        );
    }
}

void registrarAuditoria(Db::Database& db,
                        int usuarioId,
                        int rolId,
                        const std::string& accion,
                        const std::string& detalle) {
    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (
            usuario_id, modulo, accion, tabla_afectada, registro_id, detalle
        ) VALUES (?, 'SEGURIDAD', ?, 'roles_permisos_pagina', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, accion);
    audit.bind(3, rolId);
    audit.bind(4, detalle);
    audit.exec();
}

}  // namespace

bool PermisosPaginasRepository::tieneAcceso(int rolId, const std::string& paginaCodigo) {
    auto& db = DatabaseManager::instance().getDb();
    Db::Statement roleQuery(db, "SELECT nombre FROM roles WHERE id = ?");
    roleQuery.bind(1, rolId);
    if (!roleQuery.executeStep()) {
        return false;
    }
    if (roleQuery.getColumn(0).getString() == "ADMIN") {
        return true;
    }
    return rolTieneAcceso(db, rolId, paginaCodigo);
}

bool PermisosPaginasRepository::puedeGestionarPermisos(int usuarioId) {
    auto& db = DatabaseManager::instance().getDb();
    const RolUsuario rol = obtenerRolUsuario(db, usuarioId);
    return rol.nombre == "ADMIN" || rolTieneAcceso(db, rol.id, "GESTION_PERMISOS");
}

std::vector<RolConfiguracionPermisos>
PermisosPaginasRepository::obtenerRolesConfigurables() {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<RolConfiguracionPermisos> roles;
    Db::Statement query(db, R"(
        SELECT id, nombre
        FROM roles
        WHERE nombre NOT IN ('ADMIN', 'SISTEMA')
        ORDER BY nombre
    )");
    while (query.executeStep()) {
        roles.push_back({query.getColumn(0).getInt(), query.getColumn(1).getString()});
    }
    return roles;
}

std::vector<PermisoPagina> PermisosPaginasRepository::obtenerCatalogoPaginas() {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<PermisoPagina> paginas;
    Db::Statement query(db, R"(
        SELECT codigo, nombre, orden
        FROM paginas_aplicacion
        ORDER BY orden, codigo
    )");
    while (query.executeStep()) {
        paginas.push_back({
            query.getColumn(0).getString(),
            query.getColumn(1).getString(),
            query.getColumn(2).getInt(),
            false
        });
    }
    return paginas;
}

std::vector<PermisoPagina>
PermisosPaginasRepository::obtenerPermisosPorRol(int rolId) {
    auto& db = DatabaseManager::instance().getDb();
    Db::Statement roleQuery(db,
        "SELECT nombre FROM roles WHERE id = ? AND nombre <> 'ADMIN'");
    roleQuery.bind(1, rolId);
    if (!roleQuery.executeStep()) {
        throw std::runtime_error("El perfil seleccionado no existe o no es configurable.");
    }

    std::vector<PermisoPagina> permisos;
    Db::Statement query(db, R"(
        SELECT p.codigo, p.nombre, p.orden, COALESCE(rp.habilitado, 0)
        FROM paginas_aplicacion p
        LEFT JOIN roles_permisos_pagina rp
            ON rp.pagina_codigo = p.codigo AND rp.rol_id = ?
        ORDER BY p.orden
    )");
    query.bind(1, rolId);
    while (query.executeStep()) {
        permisos.push_back({
            query.getColumn(0).getString(),
            query.getColumn(1).getString(),
            query.getColumn(2).getInt(),
            query.getColumn(3).getInt() != 0
        });
    }
    return permisos;
}

void PermisosPaginasRepository::guardarOrdenPaginas(
    int usuarioId,
    const std::vector<std::string>& codigosEnOrden) {
    auto& db = DatabaseManager::instance().getDb();
    try {
        Db::Transaction transaction(db);
        validarPuedeGestionar(db, usuarioId);

        std::vector<std::string> ordenActual;
        Db::Statement pages(db,
            "SELECT codigo FROM paginas_aplicacion ORDER BY orden, codigo FOR UPDATE");
        while (pages.executeStep()) {
            ordenActual.push_back(pages.getColumn(0).getString());
        }

        std::unordered_set<std::string> codigosUnicos(
            codigosEnOrden.begin(), codigosEnOrden.end());
        if (codigosUnicos.size() != codigosEnOrden.size()
            || codigosEnOrden.size() != ordenActual.size()
            || std::any_of(ordenActual.begin(), ordenActual.end(),
                [&codigosUnicos](const std::string& codigo) {
                    return !codigosUnicos.contains(codigo);
                })) {
            throw std::runtime_error(
                "La lista de páginas cambió. Recargue la ventana e intente de nuevo."
            );
        }

        if (codigosEnOrden == ordenActual) {
            transaction.commit();
            return;
        }

        Db::Statement bounds(db,
            "SELECT COALESCE(MAX(orden), 0), COALESCE(MIN(orden), 0), COUNT(*) "
            "FROM paginas_aplicacion");
        bounds.executeStep();
        const std::int64_t maxOrden = bounds.getColumn(0).getInt64();
        const std::int64_t minOrden = bounds.getColumn(1).getInt64();
        const std::int64_t cantidad = bounds.getColumn(2).getInt64();
        const std::int64_t desplazamiento = maxOrden - minOrden + cantidad + 1;

        Db::Statement moverTemporal(
            db, "UPDATE paginas_aplicacion SET orden = orden + ?");
        moverTemporal.bind(1, desplazamiento);
        moverTemporal.exec();

        Db::Statement actualizarOrden(
            db, "UPDATE paginas_aplicacion SET orden = ? WHERE codigo = ?");
        for (std::size_t index = 0; index < codigosEnOrden.size(); ++index) {
            actualizarOrden.bind(1, static_cast<int>(index + 1));
            actualizarOrden.bind(2, codigosEnOrden[index]);
            actualizarOrden.exec();
            actualizarOrden.reset();
        }

        std::string detalle = "Nuevo orden de páginas: ";
        for (const std::string& codigo : codigosEnOrden) {
            if (detalle.back() != ' ') {
                detalle += ", ";
            }
            detalle += codigo;
        }
        registrarAuditoria(db, usuarioId, 0, "REORDENAR_PAGINAS", detalle);
        transaction.commit();
    } catch (const std::exception& error) {
        try {
            Db::Transaction auditTransaction(db);
            registrarAuditoria(
                db, usuarioId, 0, "INTENTO_REORDENAR_PAGINAS",
                std::string("Motivo: ") + error.what()
            );
            auditTransaction.commit();
        } catch (const std::exception& auditError) {
            qCritical() << "No se pudo auditar el intento de reordenar páginas:"
                        << auditError.what();
        }
        throw;
    }
}

void PermisosPaginasRepository::cambiarPermiso(int usuarioId,
                                               int rolId,
                                               const std::string& paginaCodigo,
                                               bool habilitado) {
    auto& db = DatabaseManager::instance().getDb();
    std::string rolNombre;
    std::string paginaNombre = paginaCodigo;

    try {
        Db::Transaction transaction(db);
        validarPuedeGestionar(db, usuarioId);

        Db::Statement roleQuery(db,
            "SELECT nombre FROM roles WHERE id = ?");
        roleQuery.bind(1, rolId);
        if (!roleQuery.executeStep()) {
            throw std::runtime_error("El perfil seleccionado no existe.");
        }
        rolNombre = roleQuery.getColumn(0).getString();
        if (rolNombre == "ADMIN") {
            throw std::runtime_error(
                "Los permisos del Administrador son permanentes y no se pueden modificar."
            );
        }

        Db::Statement pageQuery(db,
            "SELECT nombre FROM paginas_aplicacion WHERE codigo = ?");
        pageQuery.bind(1, paginaCodigo);
        if (!pageQuery.executeStep()) {
            throw std::runtime_error("La página seleccionada no existe.");
        }
        paginaNombre = pageQuery.getColumn(0).getString();

        const bool valorActual = rolTieneAcceso(db, rolId, paginaCodigo);
        if (valorActual == habilitado) {
            transaction.commit();
            return;
        }

        Db::Statement upsert(db, R"(
            INSERT INTO roles_permisos_pagina (rol_id, pagina_codigo, habilitado)
            VALUES (?, ?, ?)
            ON CONFLICT (rol_id, pagina_codigo)
            DO UPDATE SET habilitado = EXCLUDED.habilitado
        )");
        upsert.bind(1, rolId);
        upsert.bind(2, paginaCodigo);
        upsert.bind(3, habilitado ? 1 : 0);
        upsert.exec();

        const std::string accion = habilitado ? "HABILITAR_PAGINA" : "REVOCAR_PAGINA";
        const std::string detalle =
            "Perfil " + rolNombre + ": "
            + (habilitado ? "se habilitó" : "se revocó")
            + " el acceso a la página '" + paginaNombre + "'.";
        registrarAuditoria(db, usuarioId, rolId, accion, detalle);
        transaction.commit();
    } catch (const std::exception& error) {
        try {
            Db::Transaction auditTransaction(db);
            registrarAuditoria(
                db, usuarioId, rolId, "INTENTO_CAMBIAR_PERMISO",
                "Intento fallido para el perfil " + rolNombre
                    + " y la página '" + paginaNombre + "'. Motivo: " + error.what()
            );
            auditTransaction.commit();
        } catch (const std::exception& auditError) {
            qCritical() << "No se pudo auditar el intento de cambio de permisos:"
                        << auditError.what();
        }
        throw;
    }
}
