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

struct CuentaCobroResumen {
    int id = 0;
    std::string numeroDocumento;
    int unidadId = 0;
    std::string unidadNomenclatura;
    std::string propietarioNombre;
    std::string periodo;
    std::string fechaEmision;
    std::string fechaVencimiento;
    int64_t saldoAnterior = 0;
    int64_t valorPeriodo = 0;
    int64_t totalAPagar = 0;
    int64_t saldoPendiente = 0;
    std::string estado;
};

struct ConceptoDetallePDF {
    std::string codigo;
    std::string descripcion;
    int64_t valor = 0;
    int64_t iva = 0;
};

struct CuentaCobroCompleta {
    CuentaCobroResumen encabezado;
    std::string torreBloque;
    double coeficientePct = 0.0;
    std::string terceroDocumento;
    std::string terceroEmail;
    std::string fechaLimiteDescuento;
    int64_t valorDescuentoProntoPago = 0;
    std::vector<ConceptoDetallePDF> detalles;
};

class BillingEngine {
public:
    // Genera masivamente las cuentas de cobro del periodo (ej. "2026-11") para todas las unidades ACTIVAS.
    // Retorna la cantidad de cuentas generadas.
    static int generarFacturacionMensual(const std::string& periodo,
                                         int64_t presupuestoMensualBase,
                                         const std::string& fechaEmision,
                                         const std::string& fechaVencimiento,
                                         int usuarioId);

    // Lista cuentas de cobro filtradas por periodo (o todas si periodo está vacío)
    static std::vector<CuentaCobroResumen> listarCuentasCobro(const std::string& periodo = "");

    // Obtiene toda la información y detalles de una cuenta para exportar a PDF
    static CuentaCobroCompleta obtenerCuentaCompleta(int cuentaCobroId);
};
