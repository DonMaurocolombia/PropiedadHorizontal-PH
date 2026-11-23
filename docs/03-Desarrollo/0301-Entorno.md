# Entorno de desarrollo y despliegue PostgreSQL

## Arquitectura

El proyecto se compila en dos módulos:

- `ph_database_module`: conexión PostgreSQL/libpqxx, esquema, repositorios y servicios de datos.
- `ph_presentation_module`: widgets e interfaz Qt.

El ejecutable `PropiedadHorizontalPH` enlaza ambos módulos. El proceso de PostgreSQL solo se inicia cuando `DB_MODE=server`; `DB_MODE=client` conecta a un servidor existente y nunca ejecuta `initdb` ni `pg_ctl`.

## vcpkg y VS Code

Requisitos: Git, CMake, Qt 6 (Core, Gui, Widgets y PrintSupport), un compilador C++20 y las herramientas cliente/servidor PostgreSQL. vcpkg instala libpqxx y sus dependencias.

Descarga y prepara la versión fijada de vcpkg desde la raíz:

- Linux y macOS: `sh scripts/bootstrap-vcpkg.sh`
- Windows: `powershell -ExecutionPolicy Bypass -File scripts/bootstrap-vcpkg.ps1`

Después abre la carpeta en VS Code y selecciona el preset CMake `development`. El preset activa el toolchain de `.vcpkg`, usa el manifiesto `vcpkg.json` y escribe los artefactos en `build/vcpkg/`.

## Configuración de conexión

Crea `datos.env` a partir de `datos.env.example`. El archivo no se versiona; conserva `DB_PASSWORD` fuera del control de versiones.

### Servidor local

Usa `DB_MODE=server`, `DB_PORT=5433` y `DB_HOST=127.0.0.1` (o `localhost`). En el primer inicio, la aplicación crea `postgres-data/` junto al ejecutable, ejecuta `initdb`, arranca el proceso con `pg_ctl`, crea `DB_NAME` y carga `db/schema.sql` y `db/puc_colombia.sql` en una sola transacción. El usuario superadministrador interno tiene una contraseña aleatoria guardada con permisos privados en ese directorio; `DB_USER` se crea como propietario sin privilegios de superusuario.

En modo servidor, el botón **Salir** cierra la aplicación y deja PostgreSQL funcionando. El botón **Salir y apagar PostgreSQL** solicita confirmación y apaga el servidor local; los clientes conectados perderán la conexión.

`initdb`, `pg_ctl` y `pg_dump` deben estar en el `PATH`, en `postgresql/bin/` junto al ejecutable, o en el directorio indicado por `DB_BIN_DIR`. El servidor escucha por defecto solo en loopback. Para clientes de la red, configura `DB_LISTEN_ADDRESSES` y `DB_ALLOWED_CIDRS` con interfaces y rangos confiables, y limita el puerto 5433 mediante el firewall.

### Cliente remoto

En cada cliente configura `DB_MODE=client`, `DB_HOST` con la dirección del servidor, `DB_PORT=5433` y las credenciales de esa base. El cliente no necesita los binarios del servidor ni crea carpetas locales de datos; solo requiere acceso de red y libpqxx.

Los respaldos desde la interfaz se generan con `pg_dump` en formato custom (`.backup`). En los clientes, instala las herramientas cliente de PostgreSQL para habilitar esta función.

## Compilar y ejecutar en Windows

### Requisitos

- Windows de 64 bits.
- Visual Studio 2022 con la carga de trabajo **Desarrollo para el escritorio con C++**.
- Git y CMake disponibles en `PATH`.
- Qt 6.8.3 para MSVC 2022 de 64 bits. Por ejemplo, `C:\Qt\6.8.3\msvc2022_64`.

Desde PowerShell, en la raíz del proyecto, ejecuta:

```powershell
.\scripts\build-windows.ps1 -QtRoot "C:\Qt\6.8.3\msvc2022_64"
```

El script prepara la versión fijada de vcpkg, compila la configuración `Release` con Visual Studio 2022, reúne las DLL de Qt y libpqxx, y crea `build\package\PropiedadHorizontalPH-windows-x64.zip`.

También se puede generar el paquete en GitHub: sube los cambios al repositorio, abre **Actions > Windows release > Run workflow** y descarga el artefacto `PropiedadHorizontalPH-windows-x64` cuando termine. El flujo también se ejecuta al subir una etiqueta cuyo nombre comience por `v`.

### Preparar el paquete

Extrae el ZIP en una carpeta y copia `datos.env.example` como `datos.env`. Para usar una base local (`DB_MODE=server`), instala PostgreSQL para Windows y asegúrate de que `initdb.exe` y `pg_ctl.exe` estén en `PATH` o configura `DB_BIN_DIR` con la carpeta que los contiene. Para conectarte a un servidor existente, usa `DB_MODE=client` y configura su dirección, puerto y credenciales. La aplicación no incluye el servidor PostgreSQL.

Ejecuta `PropiedadHorizontalPH.exe` desde esa carpeta. No muevas el ejecutable fuera de ella: debe permanecer junto a las DLL, los plugins de Qt y `datos.env`.

### Nuevos módulos
Para que un módulo nuevo aparezca y funcione con la lógica actual, hay que registrarlo en dos lugares:

1. **En el código**: crear e integrar su widget en `MainWindow.cpp`, asociándolo con un código único, como ***CUENTAS_CONTABLES***.
2. **En la base de datos**: agregar ese código a paginas_aplicacion, definir su orden y asignar permisos iniciales a los perfiles autorizados en roles_permisos_pagina.

También hay que actualizar los scripts de esquema y población para instalaciones nuevas, y crear una migración para las bases existentes. Si se registra en la base pero no se asocia a un widget en MainWindow, la aplicación lo omitirá.

Por último, los permisos de página controlan qué módulos se muestran; las acciones sensibles dentro del módulo deben conservar sus propias validaciones de autorización.

