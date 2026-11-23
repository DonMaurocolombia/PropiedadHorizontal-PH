// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

struct RolConfiguracionPermisos {
    int id = 0;
    std::string nombre;
};

struct PermisoPagina {
    std::string codigo;
    std::string nombre;
    int orden = 0;
    bool habilitado = false;
};

class PermisosPaginasRepository {
public:
    static bool tieneAcceso(int rolId, const std::string& paginaCodigo);
    static bool puedeGestionarPermisos(int usuarioId);
    static std::vector<RolConfiguracionPermisos> obtenerRolesConfigurables();
    static std::vector<PermisoPagina> obtenerCatalogoPaginas();
    static std::vector<PermisoPagina> obtenerPermisosPorRol(int rolId);
    static void guardarOrdenPaginas(int usuarioId,
                                    const std::vector<std::string>& codigosEnOrden);
    static void cambiarPermiso(int usuarioId,
                               int rolId,
                               const std::string& paginaCodigo,
                               bool habilitado);
};
