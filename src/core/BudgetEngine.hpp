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
#include <cstdint>

struct PresupuestoEncabezado {
    int id = 0;
    int anio = 0;
    std::string fechaAprobacionAsamblea;
    std::string actaAsambleaNumero;
    int64_t valorTotalAnual = 0;
    double porcentajeFondoImprevistos = 1.0;
    std::string estado = "VIGENTE"; // 'BORRADOR', 'VIGENTE', 'CERRADO'
};

struct RubroEjecucionItem {
    int rubroId = 0;
    std::string cuentaCodigo;
    std::string nombreRubro;
    std::string tipo; // 'INGRESO', 'GASTO', 'FONDO_IMPREVISTOS'
    int64_t valorAnualAprobado = 0;
    int64_t valorMensualProyectado = 0;
    int64_t proyectadoAcumuladoCorte = 0; // Según el mes de corte (1..12)
    int64_t ejecutadoRealAcumulado = 0;   // Leído desde movimientos_contables
    int64_t saldoPorEjecutar = 0;
    double porcentajeEjecucionAnual = 0.0;
};

struct ResumenEjecucionPresupuestal {
    PresupuestoEncabezado encabezado;
    int mesCorte = 12;
    std::vector<RubroEjecucionItem> rubros;
    int64_t totalGastosAprobados = 0;
    int64_t totalGastosEjecutados = 0;
    int64_t totalIngresosAprobados = 0;
    int64_t totalIngresosEjecutados = 0;
    int64_t valorFondoImprevistosAprobado = 0;
    double porcentajeGlobalGastos = 0.0;
};

class BudgetEngine {
public:
    // Crea o recupera el encabezado del presupuesto de un año
    static int crearPresupuestoAnual(const PresupuestoEncabezado& pres, int usuarioId);

    // Lista todos los años presupuestados
    static std::vector<PresupuestoEncabezado> listarPresupuestos();

    // Agrega o actualiza un rubro dentro de un presupuesto anual
    static void guardarRubroPresupuesto(int presupuestoId,
                                        const std::string& cuentaCodigo,
                                        const std::string& nombreRubro,
                                        const std::string& tipo,
                                        int64_t valorAnualAprobado,
                                        int usuarioId);

    static void eliminarRubro(int rubroId, int usuarioId);

    // Calcula el comparativo completo (Aprobado vs. Ejecutado real en contabilidad) hasta mesCorte (1..12)
    static ResumenEjecucionPresupuestal calcularEjecucion(int presupuestoId, int mesCorte);

    // Exporta el comparativo de ejecución presupuestal a PDF
    static void exportarEjecucionPDF(const ResumenEjecucionPresupuestal& resumen,
                                     const std::string& rutaPdf);
};
