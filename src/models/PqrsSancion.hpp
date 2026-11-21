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

#include <string>
#include <optional>
#include <cstdint>

struct PqrsSancion {
    int id = 0;
    std::string radicado;
    std::optional<int> unidadId;
    std::string unidadNomenclatura; // Para visualización en tabla
    int terceroId = 0;
    std::string terceroNombre;      // Para visualización en tabla
    std::string tipo = "PETICION";  // 'PETICION', 'QUEJA', 'RECLAMO', 'SUGERENCIA', 'PROCESO_SANCIONATORIO'
    std::string asunto;
    std::string descripcion;
    std::string fechaRadicacion;
    std::string fechaDescargos;
    int64_t valorMulta = 0;
    std::string estado = "ABIERTA"; // 'ABIERTA', 'EN_DESCARGOS', 'SANCIONADA', 'CERRADA'
};
