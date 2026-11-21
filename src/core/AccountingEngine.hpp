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
#include <optional>
#include <cstdint>

struct LineaMovimiento {
    std::string cuentaCodigo;
    int terceroId;
    std::optional<int> unidadId; // Opcional (para estado de cuenta por inmueble)
    std::string detalle;
    int64_t debito = 0;          // En pesos COP exactos
    int64_t credito = 0;         // En pesos COP exactos
};

struct ComprobanteRequest {
    std::string tipoComprobante; // 'INGRESO', 'EGRESO', 'NOTA_CONTABLE', 'FACTURACION', 'CIERRE'
    std::string fecha;           // 'YYYY-MM-DD'
    std::string descripcion;
    int usuarioId;
    std::vector<LineaMovimiento> movimientos;
};

struct SaldoCuenta {
    std::string codigo;
    std::string nombre;
    std::string clase;
    std::string naturaleza;
    int64_t totalDebito = 0;
    int64_t totalCredito = 0;
    int64_t saldoFinal = 0;
};

class AccountingEngine {
public:
    // Registra un comprobante contable validando partida doble y reglas del PUC.
    // Retorna el ID del comprobante creado o lanza std::runtime_error si hay inconsistencia.
    static int registrarComprobante(const ComprobanteRequest& req);

    // Registra dentro de una transacción abierta por el llamador.
    static int registrarComprobanteEnTransaccion(const ComprobanteRequest& req);

    // Anula un comprobante generando una nota contable inversa (nunca se borran registros contables)
    static int anularComprobante(int comprobanteId, int usuarioId, const std::string& motivo);

    // Consulta el balance de prueba entre dos fechas
    static std::vector<SaldoCuenta> obtenerBalancePrueba(const std::string& fechaInicio,
                                                         const std::string& fechaFin);

    // Consulta el saldo actual de un tercero en una cuenta específica (ej. Cartera 130505)
    static int64_t obtenerSaldoTerceroCuenta(int terceroId, const std::string& cuentaCodigo);

private:
    static int insertarComprobante(const ComprobanteRequest& req);
    static void validarPartidaDoble(const std::vector<LineaMovimiento>& movimientos);
    static void validarCuentaPUC(const std::string& cuentaCodigo, int terceroId);
    static int obtenerSiguienteConsecutivo(const std::string& tipoComprobante);
};
