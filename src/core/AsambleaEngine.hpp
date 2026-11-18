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

#include "models/Asamblea.hpp"
#include <vector>
#include <string>

class AsambleaEngine {
public:
    static int crearAsamblea(const Asamblea& a, int usuarioId);
    static std::vector<Asamblea> listarAsambleas();
    static void cambiarEstadoAsamblea(int asambleaId, const std::string& nuevoEstado, int usuarioId);

    // Registra la asistencia de una unidad privada (propietario o apoderado) y recalcula el quórum
    static void registrarAsistencia(int asambleaId,
                                    int unidadId,
                                    const std::string& asistenteNombre,
                                    bool esApoderado,
                                    int usuarioId);

    static void eliminarAsistencia(int asistenciaId, int usuarioId);
    static std::vector<AsistenciaItem> listarAsistencia(int asambleaId);

    // Exporta la planilla oficial de asistencia y certificación de quórum a PDF
    static void exportarPlanillaQuorumPDF(int asambleaId, const std::string& rutaPdf);
};
