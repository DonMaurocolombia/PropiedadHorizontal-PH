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
#include <vector>
#include <cstdint>

struct CarteraEdadItem {
    int unidadId = 0;
    std::string torreBloque;
    std::string nomenclatura;
    std::string propietarioNombre;
    int64_t de0a30Dias = 0;
    int64_t de31a60Dias = 0;
    int64_t de61a90Dias = 0;
    int64_t masDe90Dias = 0;
    int64_t totalDeuda = 0;
};

struct LineaEstadoFinanciero {
    std::string codigo;
    std::string nombre;
    std::string clase; // 'ACTIVO', 'PASIVO', 'PATRIMONIO', 'INGRESOS', 'GASTOS'
    int64_t saldo = 0;
};

struct EstadoResultadosResumen {
    std::vector<LineaEstadoFinanciero> ingresos;
    std::vector<LineaEstadoFinanciero> gastos;
    int64_t totalIngresos = 0;
    int64_t totalGastos = 0;
    int64_t excedenteODeficit = 0; // Positivo = Excedente, Negativo = Déficit
};

struct BalanceGeneralResumen {
    std::vector<LineaEstadoFinanciero> activos;
    std::vector<LineaEstadoFinanciero> pasivos;
    std::vector<LineaEstadoFinanciero> patrimonio;
    int64_t totalActivos = 0;
    int64_t totalPasivos = 0;
    int64_t totalPatrimonio = 0;
    int64_t resultadoEjercicio = 0;
};

class ReportsEngine {
public:
    // Calcula la cartera vencida por edades a una fecha de corte (ej. '2026-11-13')
    static std::vector<CarteraEdadItem> generarCarteraPorEdades(const std::string& fechaCorte);

    // Genera el Estado de Resultados entre dos fechas
    static EstadoResultadosResumen generarEstadoResultados(const std::string& fechaInicio,
                                                           const std::string& fechaFin);

    // Genera el Balance General acumulado hasta la fecha de corte
    static BalanceGeneralResumen generarBalanceGeneral(const std::string& fechaCorte);

    // Exporta cualquiera de los reportes a PDF con encabezado oficial de la copropiedad
    static void exportarCarteraPDF(const std::vector<CarteraEdadItem>& datos,
                                   const std::string& fechaCorte,
                                   const std::string& rutaPdf);

    static void exportarEstadoResultadosPDF(const EstadoResultadosResumen& er,
                                            const std::string& fechaInicio,
                                            const std::string& fechaFin,
                                            const std::string& rutaPdf);

    static void exportarBalanceGeneralPDF(const BalanceGeneralResumen& bg,
                                          const std::string& fechaCorte,
                                          const std::string& rutaPdf);
};
