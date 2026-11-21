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

#include "models/UsuarioSesion.hpp"
#include <vector>
#include <string>
#include <optional>

class AuthManager {
public:
    // Verifica si no hay usuarios en la BD y crea el usuario 'admin' por defecto (clave: 'Admin123*')
    static void asegurarUsuarioAdminInicial();

    // Valida credenciales y retorna la sesión del usuario si son correctas
    static std::optional<UsuarioSesion> iniciarSesion(const std::string& username,
                                                      const std::string& password);

    // Gestión de usuarios (CRUD)
    static std::vector<UsuarioSesion> listarUsuarios();
    static int crearUsuario(const UsuarioSesion& user, const std::string& password, int adminId);
    static void actualizarUsuario(const UsuarioSesion& user, const std::string& nuevaPasswordOpcional, int adminId);

    // Consulta de logs de auditoría
    static std::vector<AuditoriaLogItem> listarAuditoria(const std::string& filtroModulo = "TODOS", int limite = 200);

    // Utilidad de hashing seguro con Salt (SHA-256)
    static std::string generarPasswordHash(const std::string& password);
    static bool verificarPassword(const std::string& password, const std::string& hashAlmacenado);

private:
    static void contrasenaSystemInicial();
};
