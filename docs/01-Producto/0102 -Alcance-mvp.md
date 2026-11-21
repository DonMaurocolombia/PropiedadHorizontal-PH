# Alcance del MVP — Administrador PH

**Versión:** 0.1
**Estado:** Propuesta para validación  
**Producto:** Administrador PH  
**Tipo:** Plataforma standalone de gestión de propiedad horizontal.

MVP: Siglas de Minimum Viable Product o en español Producto Mínimo Viable, es un producto con suficientes características para satisfacer a los clientes iniciales, y proporcionar retroalimentación para el desarrollo futuro. (https://es.wikipedia.org/wiki/Producto_viable_m%C3%ADnimo)


## 1. Propósito

Definir el alcance funcional de la primera versión de Administrador PH, MVP.

El objetivo es construir una plataforma que permita a una administración gestionar la información y los procesos esenciales de una copropiedad desde un único lugar.

La solución deberá construirse sobre una arquitectura modular, escalable y flexible, que permita incorporar funcionalidades progresivamente y atender múltiples copropiedades.


## 2. Usuarios objetivo

### 2.1. Usuario principal: Administración

Será el usuario central del MVP.

Necesita:
- Administrar una o varias copropiedades.
- Gestionar propietarios, residentes e inmuebles.
- Consultar información financiera y administrativa.
- Organizar documentos.
- Realizar seguimiento a procesos.
- Generar información y reportes.

### 2.2. Usuarios complementarios

Se contempla la participación de auxiliares administrativos, contadores y otros colaboradores, según los permisos que se definan.


## 3. Alcance funcional del MVP

Las siguientes funcionalidades constituyen una propuesta de alcance. Las casillas representan elementos por validar, no funcionalidades ya implementadas.

### 3.1. Gestión de la propiedad horizontal

- [ ] Registrar la propiedad horizontal.
- [ ] Registrar parámetros particulares.
- [ ] Registrar torres, bloques, casas y sectores.
- [ ] Consultar y actualizar información general.
- [ ] Configurar datos de identificación y contacto.

### 3.2. Usuarios y seguridad

- [ ] Implementar autenticación.
- Crear cuentas de usuarios:
  - [ ] Administrador.
  - [ ] Contador.
  - [ ] Revisores fiscal.
  - [ ] Tesorero.
- [ ] Gestionar roles y permisos.
- [ ] Controlar el acceso a módulos y funcionalidades.
- [ ] Registrar operaciones relevantes.
- [ ] Gestionar recuperación de contraseñas.

### 3.3. Gestión de personas

- [ ] Registrar propietarios.
- [ ] Registrar residentes.
- [ ] Consultar y actualizar información personal.
- [ ] Gestionar datos de contacto.
- [ ] Relacionar personas con uno o varios inmuebles.
- [ ] Identificar el tipo de relación con cada inmueble.
- [ ] Evitar registros duplicados.
- [ ] Consultar relaciones vigentes.
- [ ] Conservar historial de relaciones.

### 3.4. Gestión de inmuebles

- [ ] Registrar inmuebles.
- [ ] Registrar nomenclatura.
- [ ] Registrar coeficientes de copropiedad.
- [ ] Asociar propietarios y residentes.
- [ ] Consultar información individual del inmueble.
- [ ] Consultar historial de ocupación y propiedad.

### 3.5. Cartera y obligaciones

- [ ] Configurar conceptos de cobro.
- [ ] Configurar cuotas de administración.
- [ ] Definir periodos de facturación.
- [ ] Configurar fechas de vencimiento.
- [ ] Configurar descuentos por pronto pago.
- [ ] Configurar reglas de mora.
- [ ] Generar obligaciones por inmueble.
- [ ] Consultar saldos pendientes.
- [ ] Consultar estados de cuenta.
- [ ] Identificar inmuebles en mora.
- [ ] Registrar acuerdos de pago.
- [ ] Consultar historial de obligaciones.
- [ ] Generar reportes de cartera.

### 3.6. Pagos y recaudos

- [ ] Registrar pagos manualmente.
- [ ] Asociar pagos con inmuebles.
- [ ] Aplicar pagos a obligaciones.
- [ ] Gestionar pagos parciales.
- [ ] Registrar fecha, valor y medio de pago.
- [ ] Adjuntar comprobantes.
- [ ] Consultar historial de pagos.
- [ ] Identificar pagos pendientes de aplicación.
- [ ] Importar información desde hoja electrónica o formato CSV (*Comma-Separated Values o Valores Separados por Comas*).
- [ ] Importar extractos bancarios.
- [ ] Facilitar conciliación bancaria.
- [ ] Identificar posibles pagos duplicados.

### 3.7. Gestión financiera y contable

- [ ] Registrar ingresos.
- [ ] Registrar egresos.
- [ ] Clasificar movimientos financieros.
- [ ] Organizar soportes de ingresos y gastos.
- [ ] Consultar movimientos por periodo.
- [ ] Gestionar presupuestos.
- [ ] Comparar presupuesto y ejecución.
- [ ] Generar informes financieros.

### 3.8. Comunicaciones y convivencia

- [ ] Elaborar comunicados.
- [ ] Gestionar plantillas de comunicados.
- [ ] Elaborar llamados de atención.
- [ ] Asociar comunicaciones con inmuebles y personas.
- [ ] Registrar novedades con fechas y motivos.
- [ ] Adjuntar evidencias y soportes.
- [ ] Consultar historial de comunicaciones.
- [ ] Generar documentos listos para impresión o distribución.

### 3.9. Solicitudes y PQR

- [ ] Registrar solicitudes.
- [ ] Registrar peticiones, quejas y reclamos.
- [ ] Identificar solicitantes.
- [ ] Clasificar solicitudes.
- [ ] Asignar responsables.
- [ ] Registrar fechas y estados.
- [ ] Adjuntar documentos.
- [ ] Registrar respuestas.
- [ ] Consultar historial y seguimiento.

### 3.10. Proveedores y contratos

- [ ] Registrar proveedores.
- [ ] Gestionar información de contacto.
- [ ] Registrar contratos.
- [ ] Adjuntar contratos y soportes.
- [ ] Registrar vigencias.
- [ ] Consultar historial de proveedores.
- [ ] Realizar seguimiento a vencimientos.

### 3.11. Mantenimiento y operación

- [ ] Registrar novedades de zonas comunes.
- [ ] Registrar mantenimientos preventivos.
- [ ] Registrar mantenimientos correctivos.
- [ ] Asignar responsables.
- [ ] Registrar fechas y estados.
- [ ] Adjuntar evidencias.
- [ ] Consultar historial de mantenimiento.
- [ ] Realizar seguimiento a actividades pendientes.

### 3.12. Reuniones y decisiones administrativas

- [ ] Registrar reuniones.
- [ ] Organizar convocatorias.
- [ ] Elaborar y almacenar actas.
- [ ] Registrar decisiones.
- [ ] Registrar compromisos.
- [ ] Asignar responsables.
- [ ] Hacer seguimiento a compromisos.

### 3.13. Informes y reportes

- [ ] Consultar información de personas e inmuebles.
- [ ] Consultar cartera.
- [ ] Consultar pagos y recaudos.
- [ ] Consultar movimientos financieros.
- [ ] Generar reportes administrativos.
- [ ] Exportar información.
- [ ] Consultar indicadores básicos.
- [ ] Generar informes por periodos.

## 4. Requisitos transversales

Estas características deberán contemplarse desde el diseño inicial:

- [ ] Arquitectura modular.
- [ ] Separación de información entre copropiedades.
- [ ] Control de acceso y permisos.
- [ ] Protección de datos personales.
- [ ] Validación de información.
- [ ] Manejo de errores.
- [ ] Trazabilidad de operaciones.
- [ ] Diseño adaptable a diferentes pantallas.
- [ ] Gestión segura de archivos.
- [ ] Copias de seguridad y recuperación.
- [ ] Importación y exportación de información.
- [ ] Pruebas funcionales y de seguridad.
- [ ] Documentación técnica.

## 5. Criterios de aceptación generales del MVP

La primera versión deberá cumplir, como mínimo, con las siguientes condiciones:

- [ ] La administración puede autenticarse de manera segura.
- [ ] Puede registrar y gestionar una copropiedad.
- [ ] Puede administrar personas e inmuebles.
- [ ] Puede consultar las relaciones entre personas e inmuebles.
- [ ] Puede gestionar los procesos de cartera y pagos aprobados para el MVP.
- [ ] Puede organizar y consultar documentos.
- [ ] Puede consultar información administrativa centralizada.
- [ ] Los usuarios solo acceden a la información autorizada.
- [ ] Las operaciones principales cuentan con validaciones.
- [ ] Las funcionalidades implementadas cuentan con pruebas.
- [ ] Existe documentación suficiente para mantener y evolucionar el sistema.

## 6. Criterios para cerrar el alcance definitivo

Antes de iniciar el desarrollo se deberá:

- [ ] Validar las necesidades de administración.
- [ ] Revisar los procesos actuales y sus dificultades.
- [ ] Identificar los datos existentes que deberán migrarse.
- [ ] Definir los módulos indispensables.
- [ ] Identificar dependencias entre funcionalidades.
- [ ] Definir reglas administrativas y financieras.
- [ ] Establecer prioridades.
- [ ] Completar las historias de usuario.
- [ ] Aprobar formalmente el alcance del MVP.

## 8. Control de alcance

El alcance podrá modificarse durante la etapa de análisis.

Toda modificación deberá reflejarse en:
- La matriz de priorización.
- Las historias de usuario.
- Las reglas del negocio.
- El modelo de datos.
- La arquitectura, cuando corresponda.

**Estado final:** Pendiente de validación y aprobación.

Administrador PH se encuentra actualmente en etapa de documentación y análisis. No existe todavía una aplicación funcional implementada.

