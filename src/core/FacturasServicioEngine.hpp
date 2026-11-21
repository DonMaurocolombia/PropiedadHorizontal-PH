// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct FacturaServicioLineaRequest {
    std::string descripcion;
    int64_t valorBaseCop = 0;
};

struct FacturaServicioBorrador {
    int id = 0;
    int terceroId = 0;
    int conceptoId = 0;
    std::string terceroNombre;
    std::string tipoDocumento;
    std::string numeroDocumento;
    std::string conceptoNombre;
    std::string tipoServicio;
    int unidadVisitadaId = 0;
    std::string unidadVisitada;
    std::string placaVehiculo;
    std::string fechaEmision;
    std::string fechaVencimiento;
    int64_t subtotalCop = 0;
    int64_t ivaTotalCop = 0;
    int64_t totalCop = 0;
    std::string estado;
};

class FacturasServicioEngine {
public:
    static int crearBorrador(int terceroId,
                             int conceptoId,
                             int unidadVisitadaId,
                             const std::string& placaVehiculo,
                             const std::string& fechaEmision,
                             const std::string& fechaVencimiento,
                             const std::vector<FacturaServicioLineaRequest>& lineas,
                             int usuarioId);

    static void confirmarPago(int facturaId,
                              const std::string& fechaPago,
                              const std::string& formaPago,
                              const std::string& cuentaBancoCodigo,
                              const std::string& referencia,
                              int usuarioId);
    static void asignarProximaCuenta(int facturaId, int usuarioId);

    static std::vector<FacturaServicioBorrador> listarBorradores();
};
