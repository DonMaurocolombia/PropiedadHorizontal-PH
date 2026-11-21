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

#include "models/ParqueaderoBodega.hpp"
#include <vector>
#include <string>

class ParqueaderoRepository {
public:
    // --- PARQUEADEROS ---
    static std::vector<Parqueadero> listarParqueaderos(const std::string& filtro = "");
    static int guardarParqueadero(const Parqueadero& p, int usuarioId);
    static void eliminarParqueadero(int id, int usuarioId);

    // --- BODEGAS ---
    static std::vector<Bodega> listarBodegas(const std::string& filtro = "");
    static int guardarBodega(const Bodega& b, int usuarioId);
    static void eliminarBodega(int id, int usuarioId);

    // --- VEHÍCULOS ---
    static std::vector<Vehiculo> listarVehiculos(const std::string& filtro = "");
    static int guardarVehiculo(const Vehiculo& v, int usuarioId);
    static void eliminarVehiculo(int id, int usuarioId);
};
