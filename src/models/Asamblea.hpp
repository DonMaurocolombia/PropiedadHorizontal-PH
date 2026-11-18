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
#include <vector>

struct Asamblea {
    int id = 0;
    std::string tipo = "ORDINARIA"; // 'ORDINARIA', 'EXTRAORDINARIA'
    std::string fecha;
    std::string horaInicio;
    double quorumRequerido = 0.500001; // > 50% en fracción
    double quorumAlcanzado = 0.0;      // Suma de coeficientes registrados
    std::string estado = "PROGRAMADA"; // 'PROGRAMADA', 'EN_CURSO', 'CERRADA'
};

struct AsistenciaItem {
    int id = 0;
    int asambleaId = 0;
    int unidadId = 0;
    std::string torreBloque;
    std::string unidadNomenclatura;
    std::string propietarioNombre;
    std::string asistenteNombre;
    bool esApoderado = false;
    double coeficienteRepresentado = 0.0;
    std::string horaRegistro;
};
