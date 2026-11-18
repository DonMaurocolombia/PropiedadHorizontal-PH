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

struct UsuarioSesion {
    int id = 0;
    std::string username;
    std::string nombreCompleto;
    std::string email;
    int rolId = 0;
    std::string rolNombre; // 'ADMIN', 'CONTADOR', 'REVISOR_FISCAL', 'ASISTENTE'
    bool activo = true;
    std::string ultimoAcceso;
};

struct AuditoriaLogItem {
    int id = 0;
    std::string fechaHora;
    std::string usuarioNombre;
    std::string modulo;
    std::string accion;
    std::string tablaAfectada;
    int registroId = 0;
    std::string detalle;
};
