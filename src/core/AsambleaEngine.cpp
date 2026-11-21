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


#include "AsambleaEngine.hpp"
#include "DbManager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <QPrinter>
#include <QTextDocument>
#include <QPageSize>
#include <QString>
#include <stdexcept>
#include <format>

int AsambleaEngine::crearAsamblea(const Asamblea& a, int usuarioId) {
    if (a.fecha.empty() || a.horaInicio.empty()) {
        throw std::runtime_error("La fecha y hora de inicio son obligatorias.");
    }
    if (a.quorumRequerido <= 0.0 || a.quorumRequerido > 1.0) {
        throw std::runtime_error("El quórum requerido debe estar entre 1% y 100%.");
    }

    int newId = 0;
    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Transaction transaction(db);

        SQLite::Statement ins(db, R"(
            INSERT INTO asambleas (tipo, fecha, hora_inicio, quorum_requerido, quorum_alcanzado, estado)
            VALUES (?, ?, ?, ?, 0.0, 'EN_CURSO')
        )");
        ins.bind(1, a.tipo);
        ins.bind(2, a.fecha);
        ins.bind(3, a.horaInicio);
        ins.bind(4, a.quorumRequerido);
        ins.exec();

        newId = static_cast<int>(db.getLastInsertRowid());

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'ASAMBLEAS', 'CREAR', 'asambleas', ?, ?)
        )");
        audit.bind(1, usuarioId);
        audit.bind(2, newId);
        audit.bind(3, std::format("Apertura Asamblea {} fecha {}", a.tipo, a.fecha));
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al crear asamblea: " << e.what();
    }

    return newId;
}

std::vector<Asamblea> AsambleaEngine::listarAsambleas() {
    std::vector<Asamblea> lista;
    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement q(db, R"(
            SELECT id, tipo, fecha, hora_inicio, quorum_requerido, quorum_alcanzado, estado
            FROM asambleas
            ORDER BY id DESC
        )");

        while (q.executeStep()) {
            lista.push_back({
                q.getColumn(0).getInt(),
                q.getColumn(1).getString(),
                q.getColumn(2).getString(),
                q.getColumn(3).getString(),
                q.getColumn(4).getDouble(),
                q.getColumn(5).getDouble(),
                q.getColumn(6).getString()
            });
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error en el motor de asamblea: " << e.what();
    }

    return lista;
}

void AsambleaEngine::cambiarEstadoAsamblea(int asambleaId, const std::string& nuevoEstado, int usuarioId) {
    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Transaction transaction(db);

        SQLite::Statement upd(db, "UPDATE asambleas SET estado = ? WHERE id = ?");
        upd.bind(1, nuevoEstado);
        upd.bind(2, asambleaId);
        upd.exec();

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'ASAMBLEAS', 'EDITAR', 'asambleas', ?, ?)
        )");
        audit.bind(1, usuarioId);
        audit.bind(2, asambleaId);
        audit.bind(3, "Cambio de estado asamblea a " + nuevoEstado);
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al cambiar el estado de la asamblea: " << e.what();
    }
}

void AsambleaEngine::registrarAsistencia(
    int asambleaId, int unidadId, const std::string& asistenteNombre,
    bool esApoderado, int usuarioId
) {
    if (asambleaId <= 0) throw std::runtime_error("Seleccione una asamblea activa.");
    if (unidadId <= 0) throw std::runtime_error("Seleccione una unidad privada.");
    if (asistenteNombre.empty()) throw std::runtime_error("Ingrese el nombre del propietario o apoderado asistente.");

    try {
        auto& db = DatabaseManager::instance().getDb();

        // Validar que la asamblea no esté cerrada
        SQLite::Statement stAsam(db, "SELECT estado FROM asambleas WHERE id = ?");
        stAsam.bind(1, asambleaId);
        if (!stAsam.executeStep()) throw std::runtime_error("Asamblea no encontrada.");
        if (stAsam.getColumn(0).getString() == "CERRADA") {
            throw std::runtime_error("La asamblea ya se encuentra CERRADA y no admite nuevos registros.");
        }

        // Validar que la unidad no haya sido registrada previamente
        SQLite::Statement checkDup(db, "SELECT COUNT(*) FROM asamblea_asistencia WHERE asamblea_id = ? AND unidad_id = ?");
        checkDup.bind(1, asambleaId);
        checkDup.bind(2, unidadId);
        checkDup.executeStep();
        if (checkDup.getColumn(0).getInt() > 0) {
            throw std::runtime_error("Esta unidad privada ya fue registrada en el quórum de esta asamblea.");
        }

        // Obtener coeficiente de la unidad privada
        SQLite::Statement unitStmt(db, "SELECT coeficiente, nomenclatura FROM unidades_privadas WHERE id = ? AND estado = 'ACTIVA'");
        unitStmt.bind(1, unidadId);
        if (!unitStmt.executeStep()) {
            throw std::runtime_error("La unidad privada no existe o no está ACTIVA.");
        }
        double coeficiente = unitStmt.getColumn(0).getDouble();
        std::string nomenclatura = unitStmt.getColumn(1).getString();

        SQLite::Transaction transaction(db);

        SQLite::Statement ins(db, R"(
            INSERT INTO asamblea_asistencia (
                asamblea_id, unidad_id, asistente_nombre, es_apoderado,
                coeficiente_representado, hora_registro
            ) VALUES (?, ?, ?, ?, ?, time('now', 'localtime'))
        )");
        ins.bind(1, asambleaId);
        ins.bind(2, unidadId);
        ins.bind(3, asistenteNombre);
        ins.bind(4, esApoderado ? 1 : 0);
        ins.bind(5, coeficiente);
        ins.exec();

        // Recalcular suma total de coeficientes en la asamblea
        SQLite::Statement updQuorum(db, R"(
            UPDATE asambleas
            SET quorum_alcanzado = COALESCE((
                SELECT SUM(coeficiente_representado) FROM asamblea_asistencia WHERE asamblea_id = ?
            ), 0.0)
            WHERE id = ?
        )");
        updQuorum.bind(1, asambleaId);
        updQuorum.bind(2, asambleaId);
        updQuorum.exec();

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'ASAMBLEAS', 'CREAR', 'asamblea_asistencia', ?, ?)
        )");
        audit.bind(1, usuarioId);
        audit.bind(2, asambleaId);
        audit.bind(3, std::format("Registro quórum {} ({:.4f}%) - Asistente: {}",
                                nomenclatura, coeficiente * 100.0, asistenteNombre));
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al registrar asistencia: " << e.what();
    }
}

void AsambleaEngine::eliminarAsistencia(int asistenciaId, int usuarioId) {
    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement getStmt(db, "SELECT asamblea_id FROM asamblea_asistencia WHERE id = ?");
        getStmt.bind(1, asistenciaId);
        if (!getStmt.executeStep()) return;
        int asambleaId = getStmt.getColumn(0).getInt();

        SQLite::Transaction transaction(db);

        SQLite::Statement del(db, "DELETE FROM asamblea_asistencia WHERE id = ?");
        del.bind(1, asistenciaId);
        del.exec();

        SQLite::Statement updQuorum(db, R"(
            UPDATE asambleas
            SET quorum_alcanzado = COALESCE((
                SELECT SUM(coeficiente_representado) FROM asamblea_asistencia WHERE asamblea_id = ?
            ), 0.0)
            WHERE id = ?
        )");
        updQuorum.bind(1, asambleaId);
        updQuorum.bind(2, asambleaId);
        updQuorum.exec();

        SQLite::Statement audit(db, R"(
            INSERT INTO auditoria_logs (usuario_id, modulo, accion, tabla_afectada, registro_id, detalle)
            VALUES (?, 'ASAMBLEAS', 'ELIMINAR', 'asamblea_asistencia', ?, 'Retiro de unidad del quórum')
        )");
        audit.bind(1, usuarioId);
        audit.bind(2, asistenciaId);
        audit.exec();

        transaction.commit();
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al eliminar asistencia: " << e.what();
    }
}

std::vector<AsistenciaItem> AsambleaEngine::listarAsistencia(int asambleaId) {
    std::vector<AsistenciaItem> lista;
    try {
        auto& db = DatabaseManager::instance().getDb();

        SQLite::Statement q(db, R"(
            SELECT
                a.id, a.asamblea_id, a.unidad_id, COALESCE(u.torre_bloque, ''),
                u.nomenclatura, t.nombre_razon_social, a.asistente_nombre,
                a.es_apoderado, a.coeficiente_representado, a.hora_registro
            FROM asamblea_asistencia a
            INNER JOIN unidades_privadas u ON a.unidad_id = u.id
            INNER JOIN terceros t ON u.propietario_principal_id = t.id
            WHERE a.asamblea_id = ?
            ORDER BY a.id DESC
        )");
        q.bind(1, asambleaId);

        while (q.executeStep()) {
            lista.push_back({
                q.getColumn(0).getInt(),
                q.getColumn(1).getInt(),
                q.getColumn(2).getInt(),
                q.getColumn(3).getString(),
                q.getColumn(4).getString(),
                q.getColumn(5).getString(),
                q.getColumn(6).getString(),
                q.getColumn(7).getInt() == 1,
                q.getColumn(8).getDouble(),
                q.getColumn(9).getString()
            });
        }
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al listar asistencia: " << e.what();
    }

    return lista;
}

void AsambleaEngine::exportarPlanillaQuorumPDF(int asambleaId, const std::string& rutaPdf) {
    QString razonSocial = "CONJUNTO RESIDENCIAL PH";
    QString nit = "900.000.000-1";
    QString tipo, fecha, hora, estado;
    double reqPct, alcPct;

    try {
        auto& db = DatabaseManager::instance().getDb();
        SQLite::Statement cfg(db, "SELECT razon_social, nit FROM copropiedad_config WHERE id = 1");
        if (cfg.executeStep()) {
            razonSocial = QString::fromStdString(cfg.getColumn(0).getString());
            nit = QString::fromStdString(cfg.getColumn(1).getString());
        }

        SQLite::Statement asam(db, R"(
            SELECT tipo, fecha, hora_inicio, quorum_requerido, quorum_alcanzado, estado
            FROM asambleas WHERE id = ?
        )");
        asam.bind(1, asambleaId);
        if (!asam.executeStep()) throw std::runtime_error("Asamblea no encontrada.");

        tipo   = QString::fromStdString(asam.getColumn(0).getString());
        fecha  = QString::fromStdString(asam.getColumn(1).getString());
        hora   = QString::fromStdString(asam.getColumn(2).getString());
        reqPct = asam.getColumn(3).getDouble() * 100.0;
        alcPct = asam.getColumn(4).getDouble() * 100.0;
        estado = QString::fromStdString(asam.getColumn(5).getString());
    } catch (const SQLite::Exception& e) {
       qCritical() << "Error al exportar planilla de quorum a PDF: " << e.what();
       return;
    }

    auto asistentes = listarAsistencia(asambleaId);
    QString filas;
    for (const auto& item : asistentes) {
        filas += QString(R"(
            %1 - %2
            %3
            %4 (%5)
            %6%
            %7
        )")
            .arg(QString::fromStdString(item.torreBloque))
            .arg(QString::fromStdString(item.unidadNomenclatura))
            .arg(QString::fromStdString(item.propietarioNombre))
            .arg(QString::fromStdString(item.asistenteNombre))
            .arg(item.esApoderado ? "Apoderado" : "Propietario")
            .arg(QString::number(item.coeficienteRepresentado * 100.0, 'f', 4))
            .arg(QString::fromStdString(item.horaRegistro));
    }

    QString html = QString(R"(



        %1
        NIT: %2 | Ley 675 de 2001 (Art. 45 - Quórum y Mayorías)
        REGISTRO DE ASISTENCIA Y CERTIFICACIÓN DE QUÓRUM
        Asamblea %3 | Fecha: %4 | Hora Inicio: %5 | Estado: %6
        Quórum Requerido: %7%   |  
        Quórum Alcanzado: %8%
        |   Unidades Presentes: %9
        %10
        Unidad
        Propietario
        Asistente / Calidad
        Coeficiente
        Hora
        Firma
    )")
        .arg(razonSocial, nit, tipo, fecha, hora, estado)
        .arg(QString::number(reqPct, 'f', 2))
        .arg(QString::number(alcPct, 'f', 4))
        .arg(asistentes.size())
        .arg(filas);
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::Letter));
    printer.setOutputFileName(QString::fromStdString(rutaPdf));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
}
