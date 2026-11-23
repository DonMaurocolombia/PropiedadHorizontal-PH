// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PucCuentasRepository.hpp"

#include "DbManager.hpp"

#include <QDebug>
#include <QDateTime>

#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>

namespace {

constexpr std::array<int, 5> nivelesCodigo{1, 2, 4, 6, 9};

struct ClaseCuenta {
    const char* nombre;
    const char* naturaleza;
};

ClaseCuenta claseParaCodigo(const std::string& codigo) {
    if (codigo.empty()) {
        throw std::runtime_error("El código de la cuenta es obligatorio.");
    }

    switch (codigo.front()) {
    case '1': return {"ACTIVO", "DEBITO"};
    case '2': return {"PASIVO", "CREDITO"};
    case '3': return {"PATRIMONIO", "CREDITO"};
    case '4': return {"INGRESOS", "CREDITO"};
    case '5': return {"GASTOS", "DEBITO"};
    case '8': return {"ORDEN", "DEBITO"};
    case '9': return {"ORDEN", "CREDITO"};
    default:
        throw std::runtime_error(
            "El primer dígito debe corresponder a una clase PUC soportada (1–5, 8 o 9)."
        );
    }
}

int nivelParaCodigo(const std::string& codigo) {
    const auto nivel = std::find(nivelesCodigo.begin(), nivelesCodigo.end(),
                                 static_cast<int>(codigo.size()));
    if (nivel == nivelesCodigo.end()) {
        throw std::runtime_error(
            "La cuenta debe tener 1, 2, 4, 6 o 9 dígitos según su nivel."
        );
    }
    return static_cast<int>(std::distance(nivelesCodigo.begin(), nivel)) + 1;
}

void validarCuenta(const PucCuenta& cuenta) {
    if (cuenta.codigo.empty()
        || !std::all_of(cuenta.codigo.begin(), cuenta.codigo.end(),
                        [](unsigned char digit) { return digit >= '0' && digit <= '9'; })) {
        throw std::runtime_error("El código de la cuenta solo puede contener dígitos.");
    }
    if (cuenta.nombre.empty()) {
        throw std::runtime_error("El nombre de la cuenta es obligatorio.");
    }
    const ClaseCuenta clase = claseParaCodigo(cuenta.codigo);
    if (cuenta.clase != clase.nombre) {
        throw std::runtime_error("La clase contable no corresponde al primer dígito del código.");
    }
    if (cuenta.naturaleza != "DEBITO" && cuenta.naturaleza != "CREDITO") {
        throw std::runtime_error("La naturaleza debe ser Débito o Crédito.");
    }
}

int registroIdParaCodigo(const std::string& codigo) {
    int id = 0;
    const auto result = std::from_chars(codigo.data(), codigo.data() + codigo.size(), id);
    return result.ec == std::errc{} && result.ptr == codigo.data() + codigo.size() ? id : 0;
}

void validarPermisos(Db::Database& db, int usuarioId) {
    Db::Statement query(db, R"(
        SELECT r.nombre
        FROM usuarios u
        INNER JOIN roles r ON r.id = u.rol_id
        WHERE u.id = ? AND u.activo = 1
    )");
    query.bind(1, usuarioId);
    if (!query.executeStep()) {
        throw std::runtime_error("El usuario no existe o está inactivo.");
    }
    const std::string rol = query.getColumn(0).getString();
    if (rol != "ADMIN" && rol != "CONTADOR") {
        throw std::runtime_error("Solo Administrador y Contador pueden administrar cuentas contables.");
    }
}

std::string obtenerIpCliente(Db::Database& db) {
    Db::Statement query(db, "SELECT COALESCE(inet_client_addr()::text, 'LOCAL')");
    if (!query.executeStep()) {
        return "NO DISPONIBLE";
    }
    return query.getColumn(0).getString();
}

void insertarAuditoria(Db::Database& db,
                       int usuarioId,
                       const std::string& accion,
                       const std::string& codigo,
                       bool exitosa,
                       const std::string& detalle) {
    const std::string ip = obtenerIpCliente(db);
    const std::string resultado = exitosa ? "EXITOSA" : "FALLIDA";
    const std::string registro =
        "Cuenta " + codigo
        + " | Resultado: " + resultado
        + " | Fecha/hora solicitud: "
        + QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString()
        + " | IP: " + ip
        + (detalle.empty() ? "" : " | Detalle: " + detalle);

    Db::Statement audit(db, R"(
        INSERT INTO auditoria_logs (
            usuario_id, modulo, accion, tabla_afectada, registro_id, detalle
        ) VALUES (?, 'CONTABILIDAD', ?, 'puc_cuentas', ?, ?)
    )");
    audit.bind(1, usuarioId);
    audit.bind(2, accion);
    audit.bind(3, registroIdParaCodigo(codigo));
    audit.bind(4, registro);
    audit.exec();
}

template<typename Operation>
void ejecutarAuditoria(int usuarioId,
                       const std::string& accion,
                       const std::string& codigo,
                       Operation operation) {
    auto& db = DatabaseManager::instance().getDb();
    std::string detalle;
    try {
        Db::Transaction transaction(db);
        validarPermisos(db, usuarioId);
        operation(db, detalle);
        insertarAuditoria(db, usuarioId, accion, codigo, true, detalle);
        transaction.commit();
    } catch (const std::exception& error) {
        try {
            Db::Transaction auditTransaction(db);
            insertarAuditoria(db, usuarioId, accion, codigo, false, error.what());
            auditTransaction.commit();
        } catch (const std::exception& auditError) {
            qCritical() << "No se pudo registrar el intento de administración del PUC:"
                        << auditError.what();
        }
        throw;
    }
}

std::string nombreNivel(int nivel) {
    switch (nivel) {
    case 2: return "GRUPO ";
    case 3: return "CUENTA ";
    case 4: return "SUBCUENTA ";
    case 5: return "AUXILIAR ";
    default: return {};
    }
}

std::vector<std::string> asegurarPadres(Db::Database& db, const PucCuenta& cuenta) {
    std::vector<std::string> creados;
    const ClaseCuenta clase = claseParaCodigo(cuenta.codigo);
    const int nivel = nivelParaCodigo(cuenta.codigo);

    for (int indice = 0; indice < nivel - 1; ++indice) {
        const int longitud = nivelesCodigo[static_cast<std::size_t>(indice)];
        const std::string codigoPadre = cuenta.codigo.substr(0, static_cast<std::size_t>(longitud));
        const int nivelPadre = indice + 1;

        Db::Statement query(db, "SELECT clase, nivel FROM puc_cuentas WHERE codigo = ?");
        query.bind(1, codigoPadre);
        if (query.executeStep()) {
            if (query.getColumn(0).getString() != clase.nombre
                || query.getColumn(1).getInt() != nivelPadre) {
                throw std::runtime_error(
                    "La cuenta padre " + codigoPadre
                    + " tiene una clase o nivel incompatible."
                );
            }
            continue;
        }

        const std::string padreSuperior = nivelPadre == 1
            ? std::string{}
            : cuenta.codigo.substr(
                0, static_cast<std::size_t>(nivelesCodigo[static_cast<std::size_t>(indice - 1)])
            );
        std::string nombre = nombreNivel(nivelPadre) + codigoPadre
            + " (creado automáticamente)";
        if (nivelPadre == 1) {
            nombre = clase.nombre;
        }

        Db::Statement insert(db, R"(
            INSERT INTO puc_cuentas (
                codigo, nombre, clase, naturaleza, nivel, cuenta_padre,
                permite_movimiento, exige_tercero
            ) VALUES (?, ?, ?, ?, ?, ?, 0, 0)
        )");
        insert.bind(1, codigoPadre);
        insert.bind(2, nombre);
        insert.bind(3, clase.nombre);
        insert.bind(4, clase.naturaleza);
        insert.bind(5, nivelPadre);
        if (padreSuperior.empty()) {
            insert.bind(6);
        } else {
            insert.bind(6, padreSuperior);
        }
        insert.exec();
        creados.push_back(codigoPadre);
    }
    return creados;
}

void guardarCuenta(Db::Database& db,
                   const PucCuenta& cuenta,
                   std::string& detalle,
                   const std::string& codigoOriginal = {}) {
    validarCuenta(cuenta);
    const int nivel = nivelParaCodigo(cuenta.codigo);
    const std::vector<std::string> padresCreados = asegurarPadres(db, cuenta);
    detalle = "Nombre='" + cuenta.nombre
        + "'; Clase=" + cuenta.clase
        + "; Naturaleza=" + cuenta.naturaleza
        + "; Nivel=" + std::to_string(nivel)
        + "; Padre=" + (nivel == 1
            ? std::string("ninguno")
            : cuenta.codigo.substr(
                0, static_cast<std::size_t>(nivelesCodigo[static_cast<std::size_t>(nivel - 2)])
            ))
        + "; Permite movimiento=" + (cuenta.permiteMovimiento ? "sí" : "no")
        + "; Exige tercero=" + (cuenta.exigeTercero ? "sí" : "no");
    if (!codigoOriginal.empty() && codigoOriginal != cuenta.codigo) {
        detalle += "; Código anterior=" + codigoOriginal;
    }

    Db::Statement save(db, codigoOriginal.empty()
        ? R"(
            INSERT INTO puc_cuentas (
                codigo, nombre, clase, naturaleza, nivel, cuenta_padre,
                permite_movimiento, exige_tercero
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        )"
        : R"(
            UPDATE puc_cuentas
            SET codigo = ?, nombre = ?, clase = ?, naturaleza = ?, nivel = ?,
                cuenta_padre = ?, permite_movimiento = ?, exige_tercero = ?
            WHERE codigo = ?
        )");
    save.bind(1, cuenta.codigo);
    save.bind(2, cuenta.nombre);
    save.bind(3, cuenta.clase);
    save.bind(4, cuenta.naturaleza);
    save.bind(5, nivel);
    if (nivel == 1) {
        save.bind(6);
    } else {
        save.bind(6, cuenta.codigo.substr(
            0, static_cast<std::size_t>(nivelesCodigo[static_cast<std::size_t>(nivel - 2)])
        ));
    }
    save.bind(7, cuenta.permiteMovimiento ? 1 : 0);
    save.bind(8, cuenta.exigeTercero ? 1 : 0);
    if (!codigoOriginal.empty()) {
        save.bind(9, codigoOriginal);
    }
    save.exec();

    if (!padresCreados.empty()) {
        std::string lista;
        for (const std::string& codigoPadre : padresCreados) {
            if (!lista.empty()) {
                lista += ", ";
            }
            lista += codigoPadre;
        }
        detalle += "; Cuentas padre creadas automáticamente: " + lista;
    }
}

void verificarSinMovimientos(Db::Database& db, const std::string& codigo) {
    Db::Statement account(db, "SELECT codigo FROM puc_cuentas WHERE codigo = ? FOR UPDATE");
    account.bind(1, codigo);
    if (!account.executeStep()) {
        throw std::runtime_error("La cuenta ya no existe.");
    }

    Db::Statement query(db,
        "SELECT COUNT(*) FROM movimientos_contables WHERE cuenta_codigo = ?");
    query.bind(1, codigo);
    query.executeStep();
    if (query.getColumn(0).getInt64() > 0) {
        throw std::runtime_error(
            "La cuenta tiene movimientos contables registrados y no puede modificarse ni eliminarse."
        );
    }
}

}  // namespace

std::vector<PucCuenta> PucCuentasRepository::listar(const std::string& filtro) {
    auto& db = DatabaseManager::instance().getDb();
    std::vector<PucCuenta> cuentas;
    Db::Statement query(db, R"(
        SELECT codigo, nombre, clase, naturaleza, nivel,
               COALESCE(cuenta_padre, ''), permite_movimiento, exige_tercero
        FROM puc_cuentas
        WHERE codigo ILIKE ? OR nombre ILIKE ?
        ORDER BY codigo
    )");
    const std::string patron = "%" + filtro + "%";
    query.bind(1, patron);
    query.bind(2, patron);
    while (query.executeStep()) {
        PucCuenta cuenta;
        cuenta.codigo = query.getColumn(0).getString();
        cuenta.nombre = query.getColumn(1).getString();
        cuenta.clase = query.getColumn(2).getString();
        cuenta.naturaleza = query.getColumn(3).getString();
        cuenta.nivel = query.getColumn(4).getInt();
        cuenta.cuentaPadre = query.getColumn(5).getString();
        cuenta.permiteMovimiento = query.getColumn(6).getInt() != 0;
        cuenta.exigeTercero = query.getColumn(7).getInt() != 0;
        cuentas.push_back(std::move(cuenta));
    }
    return cuentas;
}

void PucCuentasRepository::crear(const PucCuenta& cuenta, int usuarioId) {
    ejecutarAuditoria(usuarioId, "CREAR", cuenta.codigo,
        [&cuenta](Db::Database& db, std::string& detalle) {
            guardarCuenta(db, cuenta, detalle);
        });
}

void PucCuentasRepository::actualizar(const std::string& codigoOriginal,
                                      const PucCuenta& cuenta,
                                      int usuarioId) {
    ejecutarAuditoria(usuarioId, "MODIFICAR", cuenta.codigo,
        [&codigoOriginal, &cuenta](Db::Database& db, std::string& detalle) {
            if (codigoOriginal.empty()) {
                throw std::runtime_error("No se indicó la cuenta que se desea modificar.");
            }
            verificarSinMovimientos(db, codigoOriginal);
            if (codigoOriginal != cuenta.codigo) {
                Db::Statement children(db,
                    "SELECT COUNT(*) FROM puc_cuentas WHERE cuenta_padre = ?");
                children.bind(1, codigoOriginal);
                children.executeStep();
                if (children.getColumn(0).getInt() > 0) {
                    throw std::runtime_error(
                        "No se puede cambiar el código porque existen cuentas hijas asociadas."
                    );
                }
            }
            guardarCuenta(db, cuenta, detalle, codigoOriginal);
        });
}

void PucCuentasRepository::eliminar(const std::string& codigo, int usuarioId) {
    ejecutarAuditoria(usuarioId, "ELIMINAR", codigo,
        [codigo](Db::Database& db, std::string& detalle) {
            verificarSinMovimientos(db, codigo);

            Db::Statement children(db,
                "SELECT COUNT(*) FROM puc_cuentas WHERE cuenta_padre = ?");
            children.bind(1, codigo);
            children.executeStep();
            if (children.getColumn(0).getInt() > 0) {
                throw std::runtime_error("No se puede eliminar la cuenta porque tiene cuentas hijas.");
            }

            Db::Statement remove(db, "DELETE FROM puc_cuentas WHERE codigo = ?");
            remove.bind(1, codigo);
            remove.exec();
            if (db.getChanges() == 0) {
                throw std::runtime_error("La cuenta ya no existe.");
            }
            detalle = "Cuenta eliminada sin movimientos ni cuentas hijas.";
        });
}
