# Visión del producto — Administrador PH

**Versión:** 0.1
**Estado:** Borrador inicial  
**Nombre del producto:** Administrador PH  
**Tipo de solución:** Solución Standalone para la gestión de la propiedad horizontal


## 1. Propósito

Administrador PH busca facilitar y organizar la administración de copropiedades mediante una plataforma que centralice la información, los documentos y los procesos administrativos, bajo la **Licencia GPL-3.0-or-later**.

El proyecto nace de una necesidad real de administración y se plantea desde el inicio con una estructura que pueda crecer y adaptarse a las necesidades de distintas copropiedades.

Teniendo en cuenta que se ofrecerá bajo la licencia GPL-3.0-or-later, el desarrollo se elaborará sobre el **Sistema Operativo Linux**, especificamente la distribución **CachyOS** [https://www.cachyos.org/], aunque se puede ejecutar el cualquier distro basada en **Arch Linux** y con algunas pequeñas modificaciones en otras distros basadas en **Linux**.


## 2. Problema que busca resolver

La información y las tareas administrativas pueden estar distribuidas entre archivos, hojas de cálculo, documentos y procesos manuales. Esto dificulta mantener la información organizada, consultar los datos y dar continuidad a las actividades de la administración.

Administrador PH busca reducir esa dispersión y ofrecer un espacio centralizado para gestionar la información y apoyar los procesos de la copropiedad.


## 3. Usuarios

### Primera etapa: Administración, Contador, Revisor fiscal y Tesorero.

La primera etapa estará enfocada en las personas encargadas de administrar la copropiedad. La plataforma deberá ayudarles a organizar la información y realizar sus tareas desde un mismo lugar.

### Segunda etapa: Porteria

En la segunda etapa se contempla el desarrollo de una **Intranet**, que permita a los colaboradores de la copropiedad (Seguridad, Conserge, Aseo, Servicios varios y todo aquel), tener acceso a la información pertinente para desempeñar su función sin violar la privacidad y seguridad de la información de los copropietarios y residentes.

### Tercera etapa: Consejo de administración y Residentes

En una etapa posterior se contempla incorporar funcionalidades de acceso a los residentes, personas que conformen el consejo de administración y el comite de convivencia.

El alcance y los permisos de este perfil se definirán más adelante.


## 4. Objetivos

- Código 100% gratis, bajo la licencia **Licencia GPL-3.0-or-later**, sobre plataforma 100% gratis como lo sons las **distros Linux**.
- Centralizar la información administrativa de la copropiedad.
- Organizar los datos de propietarios, residentes e inmuebles y sus relaciones.
- Apoyar el manejo de cartera, pagos y documentos.
- Facilitar la elaboración y gestión de comunicados y otros documentos administrativos.
- Permitir que las reglas del negocio y las plantillas puedan adaptarse cuando cambien las necesidades de la copropiedad.
- Construir una base escalable que permita incorporar nuevos módulos y atender a más copropiedades.


## 5. Alcance inicial

La primera versión se enfocará en la administración y en las necesidades identificadas hasta ahora:

- Gestión de personas e inmuebles.
  - Ingreso y modificación de los inmuebles de la copropiedad, los cuales incluyen:
    - Bienes privados, los cuales pueden inclir:
      - Viviendas.
      - Parqueaderos.
      - Oficinas.
      - Locales comerciales.
      - Bien que tenga una escritura pública vigente.
    - Bienes comunes
      - De uso exclusivo.
      - Esenciales.
      - No esenciales.
  - Ingreso, modificación y eliminación de copropietarios.
  - Ingreso, modificación y eliminación de residentes.
  - Ingreso, modificación y eliminación de vehículos.
    - Automóviles
    - Motocicletas
    - Ciclas (mecánicas o electricas)
    - Patinetas
- Organización de información de cartera y pagos.
- Gestión de copropiedades.
  - Generar cuentas de cobro.
  - Generar facturas de cobro (Parqueaderos, arriendo salón comunal y las que apliquen).
  - Reporte de viviendas en mora y edades de mora.
- Apoyo a procesos administrativos mediante información centralizada.
  - Reporte de mantenimientos.
  - Reporte de actividades de la comunidad.

El alcance exacto de cada módulo, sus reglas y sus prioridades se definirá en la documentación de requisitos.


## 6. Principios del producto

**Legales:** Deben estar ceñidas a las normas de la copropiedad y las regulaciones vigentes Colombiana.

**Organización:** la información debe estar estructurada y ser fácil de consultar.

**Flexibilidad:** las reglas y plantillas no deben quedar rígidamente fijadas si pueden cambiar por decisiones de la copropiedad.

**Escalabilidad:** la plataforma debe poder crecer en funcionalidades y en número de copropiedades.

**Claridad:** la interfaz y los procesos deben ser comprensibles para las personas que administran una copropiedad.

**Trazabilidad:** las decisiones sobre información, permisos y cambios deberán definirse de manera que se pueda comprender cómo se gestionan los datos.


## 7. Alcance confirmado

Se consideran decisiones tomadas, auque no cerradas por completo.

- **Sistema Operativo (SO):** El desarrollo y documentación de puesta en producción se realizará para el ***SO Linux***
- **Lenguaje de programación:** Se desarrollará en lenguaje C++ ya que ofrece manejo seguro de memoria con smart pointers, std::format, fechas con std::chrono, rapidéz y eficienda de la solución.
- *Interfaz Gráfica (GUI):* Qt 6 (Widgets o QML). Es el estándar de la industria en C++ para aplicaciones empresariales de escritorio; permite crear tablas, formularios complejos y exportar a PDF fácilmente.
- **Base de Datos (DB):** En la primera etapa será embebida con SQLite 3, usando la librería wrapper SQLiteCpp. Al ser un solo archivo .db, facilita las copias de seguridad y hace que la aplicación sea 100% portable.
- **Generación de Reportes y PDF:** QtPdf / QPrinter, para generar cuentas de cobro y estados financieros en PDF directamente desde plantillas HTML/CSS, o PoDoFo.
- **Cifrado y Seguridad:** OpenSSL o libsodium para hashear contraseñas de los perfiles de usuario (Argon2 o bcrypt).
- El modelo de permisos detallado.
- Las reglas contables, financieras o legales específicas.


## 8. Fuera del alcance confirmado

<u>**No**</u> se consideran decisiones cerradas todavía:

- La arquitectura y las tecnologías definitivas para la segunta y tercera etapa.
- La incorporación de acceso a residentes en la primera versión.
- Las integraciones con servicios externos (por ejemplo: Facturción electrónica usando DIAN).
- La modalidad comercial y los planes de servicio.
- Gestión de documentos y plantillas administrativas.

Estos puntos se evaluarán durante el análisis de requisitos y arquitectura.


## 9. Visión a futuro

Administrador PH aspira a convertirse en una plataforma escalable para la gestión de propiedad horizontal, que permita a las administraciones organizar sus procesos y que, progresivamente, pueda ofrecer servicios y herramientas a residentes y otras copropiedades.

