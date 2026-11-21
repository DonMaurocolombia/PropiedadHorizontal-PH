// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

struct ConceptoCobroServicio {
    int id = 0;
    std::string codigo;
    std::string nombre;
    std::string cuentaIngresoCodigo;
    std::string cuentaIngresoNombre;
    std::string cuentaCarteraCodigo;
    std::string cuentaCarteraNombre;
};

struct CuentaPucOpcion {
    std::string codigo;
    std::string nombre;
};

class ConceptosCobroRepository {
public:
    static std::vector<ConceptoCobroServicio> listarServicios();
    static std::vector<CuentaPucOpcion> listarCuentasIngreso();
    static std::vector<CuentaPucOpcion> listarCuentasCartera();
    static std::vector<CuentaPucOpcion> listarCuentasPago();

    static int crearServicio(const std::string& codigo,
                             const std::string& nombre,
                             const std::string& cuentaIngresoCodigo,
                             const std::string& cuentaCarteraCodigo,
                             int usuarioId);
    static void actualizarServicio(int id,
                                   const std::string& nombre,
                                   const std::string& cuentaIngresoCodigo,
                                   const std::string& cuentaCarteraCodigo,
                                   int usuarioId);
    static void eliminarServicio(int id, int usuarioId);
};
