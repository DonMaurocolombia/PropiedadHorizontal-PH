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

#include "models/PqrsSancion.hpp"
#include <vector>
#include <string>

class PqrsRepository {
public:
    static std::vector<PqrsSancion> listar(const std::string& filtroEstado = "TODOS",
                                           const std::string& textoBusqueda = "");

    // Radica una nueva PQRS o abre un proceso sancionatorio
    static std::string radicar(const PqrsSancion& item, int usuarioId);

    // Avanza el estado del caso respetando el debido proceso.
    // Si pasa a 'SANCIONADA' con valorMulta > 0, contabiliza el cargo en cartera automáticamente.
    static void actualizarTramite(int id,
                                  const std::string& nuevoEstado,
                                  const std::string& fechaDescargos,
                                  int64_t valorMulta,
                                  const std::string& notaSeguimiento,
                                  int usuarioId);
};
