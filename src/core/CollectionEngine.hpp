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

struct DetalleDeudaUnidad {
    int cuentaCobroId = 0;
    std::string numeroCuentaCobro;
    std::string periodo;
    int detalleId = 0;
    std::string conceptoCodigo;
    std::string descripcion;
    int64_t saldoConcepto = 0;
};

struct ReciboCajaRequest {
    int unidadId = 0;
    std::string fechaPago;          // 'YYYY-MM-DD'
    std::string formaPago;          // 'TRANSFERENCIA', 'CONSIGNACION', 'PSE', 'EFECTIVO', 'DATAFONO'
    std::string cuentaBancoCodigo;  // Ej: '111005' (Bancos) o '110505' (Caja)
    std::string referenciaBancaria;
    int64_t valorPagado = 0;
    std::string observaciones;
    int usuarioId = 0;
};

struct ReciboCajaResumen {
    int id = 0;
    std::string numeroRecibo;
    std::string fechaPago;
    std::string unidadNomenclatura;
    std::string terceroNombre;
    std::string formaPago;
    std::string referencia;
    int64_t valorPagado = 0;
};

class CollectionEngine {
public:
    // Consulta toda la cartera pendiente de una unidad ordenada por prelación legal:
    // 1° Intereses de mora (MORA), 2° Capital más antiguo
    static std::vector<DetalleDeudaUnidad> consultarDeudaPorPrelacion(int unidadId);

    // Retorna el saldo total adeudado por una unidad (0 = A Paz y Salvo)
    static int64_t obtenerSaldoTotalUnidad(int unidadId);

    // Registra un Recibo de Caja, cruza los saldos pendientes y genera el asiento contable de INGRESO.
    // Si el pago supera la deuda actual, el excedente queda como Anticipo (Cuenta 280505).
    static std::string registrarPago(const ReciboCajaRequest& req);

    // Lista los recibos de caja registrados
    static std::vector<ReciboCajaResumen> listarRecibos();
};
