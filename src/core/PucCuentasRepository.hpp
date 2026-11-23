// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

struct PucCuenta {
    std::string codigo;
    std::string nombre;
    std::string clase;
    std::string naturaleza;
    int nivel = 0;
    std::string cuentaPadre;
    bool permiteMovimiento = false;
    bool exigeTercero = true;
};

class PucCuentasRepository {
public:
    static std::vector<PucCuenta> listar(const std::string& filtro = {});
    static void crear(const PucCuenta& cuenta, int usuarioId);
    static void actualizar(const std::string& codigoOriginal,
                           const PucCuenta& cuenta,
                           int usuarioId);
    static void eliminar(const std::string& codigo, int usuarioId);
};
