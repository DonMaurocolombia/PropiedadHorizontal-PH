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

struct ComprobanteEgresoRequest {
    std::string fecha;               // 'YYYY-MM-DD'
    int proveedorId = 0;
    std::string numeroFacturaSoporte;
    std::string conceptoDescripcion;
    std::string cuentaGastoCodigo;   // Ej: '513505' (Vigilancia), '511010' (Honorarios)
    int64_t valorBruto = 0;          // Valor total antes de retenciones
    std::string cuentaRetefuenteCodigo; // Opcional: '236515', '236525', '236540' o "" si no aplica
    int64_t valorRetefuente = 0;
    std::string cuentaBancoCodigo;   // '111005' (Bancos), '112005' (Ahorros), '110510' (Caja Menor)
    int usuarioId = 0;
};

struct EgresoResumen {
    int comprobanteId = 0;
    int consecutivo = 0;
    std::string fecha;
    std::string proveedorNombre;
    std::string proveedorDocumento;
    std::string descripcion;
    std::string cuentaGastoNombre;
    int64_t valorBruto = 0;
    int64_t valorRetencion = 0;
    int64_t valorNetoPagado = 0;
    std::string estado;
};

class ExpenseEngine {
public:
    // Registra el egreso en partida doble y retorna el ID del comprobante contable
    static int registrarEgreso(const ComprobanteEgresoRequest& req);

    // Lista el historial de comprobantes de egreso contabilizados o anulados
    static std::vector<EgresoResumen> listarEgresos();

    // Exporta un comprobante de egreso individual a PDF
    static void exportarComprobanteEgresoPDF(int comprobanteId, const std::string& rutaPdf);
};
