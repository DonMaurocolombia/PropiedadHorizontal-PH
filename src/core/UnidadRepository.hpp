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


#pragma once

#include "models/UnidadPrivada.hpp"
#include <vector>

class UnidadRepository {
public:
    static std::vector<UnidadPrivada> listarTodas(const std::string& filtro = "");
    static int crear(const UnidadPrivada& unidad, int usuarioId);
    static void actualizar(const UnidadPrivada& unidad, int usuarioId);
    static void eliminar(int id, int usuarioId);

    // Retorna la suma total de coeficientes de unidades ACTIVAS (ej. 1.0 = 100%)
    static double obtenerSumaCoeficientesActivos();

    // Listas desplegables para asignar propietario o inquilino
    static std::vector<TerceroResumen> listarPropietarios();
    static std::vector<TerceroResumen> listarInquilinos();
};
