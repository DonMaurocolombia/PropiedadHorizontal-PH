// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/* MiPrograma - Descripción breve
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

#include <string>
#include <optional>

struct UnidadPrivada {
    int id = 0;
    std::string torreBloque;
    std::string nomenclatura;
    std::string tipo; // 'APARTAMENTO', 'CASA', 'LOCAL', 'OFICINA'
    std::string matriculaInmobiliaria;
    double areaPrivadaM2 = 0.0;
    double coeficiente = 0.0; // Almacenado como fracción: 0.012500 = 1.2500%
    int propietarioPrincipalId = 0;
    std::string propietarioNombre; // Para mostrar en tabla
    std::optional<int> inquilinoActualId;
    std::string inquilinoNombre;   // Para mostrar en tabla
    std::string enviarCobroA = "PROPIETARIO"; // 'PROPIETARIO', 'INQUILINO', 'AMBOS'
    std::string estado = "ACTIVA";            // 'ACTIVA', 'EN_CONSTRUCCION', 'INACTIVA'
};

struct TerceroResumen {
    int id;
    std::string documento;
    std::string nombre;
};
