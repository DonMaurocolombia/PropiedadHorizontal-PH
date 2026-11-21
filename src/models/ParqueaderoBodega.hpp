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

struct Parqueadero {
    int id = 0;
    std::string numero;
    std::string nivelSotano;
    std::string tipoTenencia = "USO_EXCLUSIVO"; // 'PRIVADO', 'USO_EXCLUSIVO', 'COMUN_ALQUILER', 'VISITANTES'
    std::string matriculaInmobiliaria;
    double coeficiente = 0.0;
    std::optional<int> unidadAsignadaId;
    std::string unidadNomenclatura; // Para visualización en tabla
    int64_t canonMensual = 0;
    std::string estado = "DISPONIBLE"; // 'OCUPADO', 'DISPONIBLE', 'MANTENIMIENTO'
};

struct Bodega {
    int id = 0;
    std::string numero;
    std::string ubicacion;
    std::string tipoTenencia = "USO_EXCLUSIVO"; // 'PRIVADA', 'USO_EXCLUSIVO', 'COMUN_ALQUILER'
    std::string matriculaInmobiliaria;
    double coeficiente = 0.0;
    std::optional<int> unidadAsignadaId;
    std::string unidadNomenclatura;
    int64_t canonMensual = 0;
};

struct Vehiculo {
    int id = 0;
    int unidadId = 0;
    std::string unidadNomenclatura;
    std::optional<int> parqueaderoId;
    std::string parqueaderoNumero;
    std::string placa;
    std::string tipo = "AUTOMOVIL"; // 'AUTOMOVIL', 'CAMIONETA', 'MOTOCICLETA', 'BICICLETA'
    std::string marcaModelo;
    std::string color;
};
