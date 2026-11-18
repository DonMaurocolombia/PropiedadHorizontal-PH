-- Activar llaves foráneas en SQLite (ejecutar siempre al abrir conexión en C++)
PRAGMA foreign_keys = ON;

-- ============================================================================
-- 1. CONFIGURACIÓN DE LA COPROPIEDAD, USUARIOS Y AUDITORÍA
-- ============================================================================

CREATE TABLE copropiedad_config (
    id                      INTEGER PRIMARY KEY CHECK (id = 1), -- Solo permite un registro
    nit                     TEXT NOT NULL,
    razon_social            TEXT NOT NULL,
    direccion               TEXT NOT NULL,
    telefono                TEXT,
    email                   TEXT,
    representante_legal     TEXT NOT NULL,
    contador_nombre         TEXT,
    contador_tp             TEXT, -- Tarjeta profesional
    revisor_fiscal_nombre   TEXT,
    revisor_fiscal_tp       TEXT,
    tasa_mora_mensual       REAL NOT NULL DEFAULT 0.018, -- Ej: 1.8% mensual
    dia_limite_pronto_pago  INTEGER DEFAULT 10,
    porcentaje_descuento_pronto_pago REAL DEFAULT 0.0,
    orientacion_menu        INTEGER DEFAULT 0 CHECK (orientacion_menu IN (0, 1, 2, 3)),
    -- 0:Superior, 1:Inferior, 2:Izquierda, 3:Derecha
    ruta_logo               TEXT
);

CREATE TABLE roles (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre      TEXT NOT NULL UNIQUE, -- 'ADMIN', 'CONTADOR', 'REVISOR_FISCAL', 'ASISTENTE', 'RECEPCION'
    descripcion TEXT
);

CREATE TABLE usuarios (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    rol_id          INTEGER NOT NULL,
    username        TEXT NOT NULL UNIQUE,
    password_hash   TEXT NOT NULL, -- Hash Argon2 o bcrypt
    nombre_completo TEXT NOT NULL,
    email           TEXT,
    activo          INTEGER NOT NULL DEFAULT 1 CHECK (activo IN (0, 1)),
    ultimo_acceso   TEXT,
    creado_en       TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (rol_id) REFERENCES roles(id)
);

CREATE TABLE auditoria_logs (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    usuario_id      INTEGER NOT NULL,
    modulo          TEXT NOT NULL,       -- Ej: 'CONTABILIDAD', 'FACTURACION', 'UNIDADES'
    accion          TEXT NOT NULL,       -- Ej: 'CREAR', 'EDITAR', 'ANULAR'
    tabla_afectada  TEXT NOT NULL,
    registro_id     INTEGER NOT NULL,
    detalle         TEXT,               -- JSON o texto descriptivo del cambio
    fecha_hora      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (usuario_id) REFERENCES usuarios(id)
);

-- ============================================================================
-- 2. TERCEROS (PROPIETARIOS, INQUILINOS, PROVEEDORES) E INMUEBLES
-- ============================================================================

CREATE TABLE terceros (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tipo_documento TEXT NOT NULL CHECK (tipo_documento IN ('CC', 'NIT', 'CE', 'PASAPORTE', 'TI')),
    numero_documento TEXT NOT NULL UNIQUE,
    dv TEXT, -- Dígito de verificación para NIT
    nombre_razon_social TEXT NOT NULL,
    telefono TEXT,
    celular TEXT,
    email TEXT,
    direccion_correspondencia TEXT,
    es_propietario INTEGER NOT NULL DEFAULT 0 CHECK (es_propietario IN (0, 1)),
    es_inquilino INTEGER NOT NULL DEFAULT 0 CHECK (es_inquilino IN (0, 1)),
    es_proveedor INTEGER NOT NULL DEFAULT 0 CHECK (es_proveedor IN (0, 1)),
    activo INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE unidades_privadas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    torre_bloque TEXT, -- Ej: 'Torre 1', 'Manzana A'
    nomenclatura TEXT NOT NULL UNIQUE, -- Ej: 'Apto 302', 'Local 01'
    tipo TEXT NOT NULL CHECK (tipo IN ('APARTAMENTO', 'CASA', 'LOCAL', 'OFICINA')),
    matricula_inmobiliaria TEXT UNIQUE,
    area_privada_m2 REAL NOT NULL,
    coeficiente REAL NOT NULL, -- Ej: 0.012540 (La suma total debe dar 1.0)
    propietario_principal_id INTEGER NOT NULL,
    inquilino_actual_id INTEGER, -- NULL si la habita el propietario o está desocupada
    enviar_cobro_a TEXT NOT NULL DEFAULT 'PROPIETARIO' CHECK (enviar_cobro_a IN ('PROPIETARIO', 'INQUILINO', 'AMBOS')),
    estado TEXT NOT NULL DEFAULT 'ACTIVA' CHECK (estado IN ('ACTIVA', 'EN_CONSTRUCCION', 'INACTIVA')),
    FOREIGN KEY (propietario_principal_id) REFERENCES terceros(id),
    FOREIGN KEY (inquilino_actual_id) REFERENCES terceros(id)
);

-- Historial de copropietarios (por si hay varios dueños por porcentaje o cambios de dueño)
CREATE TABLE unidad_propietarios_historial (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    unidad_id INTEGER NOT NULL,
    tercero_id INTEGER NOT NULL,
    porcentaje_propiedad REAL NOT NULL DEFAULT 100.0,
    fecha_inicio TEXT NOT NULL,
    fecha_fin TEXT, -- NULL si es el propietario vigente
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id),
    FOREIGN KEY (tercero_id) REFERENCES terceros(id)
);

-- ============================================================================
-- 3. PARQUEADEROS, BODEGAS Y VEHÍCULOS
-- ============================================================================

CREATE TABLE parqueaderos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    numero TEXT NOT NULL UNIQUE, -- Ej: 'P-104', 'S1-22'
    nivel_sotano TEXT,
    tipo_tenencia TEXT NOT NULL CHECK (tipo_tenencia IN ('PRIVADO', 'USO_EXCLUSIVO', 'COMUN_ALQUILER', 'VISITANTES')),
    matricula_inmobiliaria TEXT, -- Solo si es PRIVADO
    coeficiente REAL DEFAULT 0.0, -- Solo si es PRIVADO con coeficiente propio
    unidad_asignada_id INTEGER,  -- Unidad a la que pertenece o se le asigna
    canon_mensual INTEGER DEFAULT 0, -- Si es COMUN_ALQUILER
    estado TEXT NOT NULL DEFAULT 'DISPONIBLE' CHECK (estado IN ('OCUPADO', 'DISPONIBLE', 'MANTENIMIENTO')),
    FOREIGN KEY (unidad_asignada_id) REFERENCES unidades_privadas(id)
);

CREATE TABLE bodegas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    numero TEXT NOT NULL UNIQUE, -- Ej: 'B-05'
    ubicacion TEXT,
    tipo_tenencia TEXT NOT NULL CHECK (tipo_tenencia IN ('PRIVADA', 'USO_EXCLUSIVO', 'COMUN_ALQUILER')),
    matricula_inmobiliaria TEXT,
    coeficiente REAL DEFAULT 0.0,
    unidad_asignada_id INTEGER,
    canon_mensual INTEGER DEFAULT 0,
    FOREIGN KEY (unidad_asignada_id) REFERENCES unidades_privadas(id)
);

CREATE TABLE vehiculos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    unidad_id INTEGER NOT NULL,
    parqueadero_id INTEGER,
    placa TEXT NOT NULL UNIQUE,
    tipo TEXT NOT NULL CHECK (tipo IN ('AUTOMOVIL', 'CAMIONETA', 'MOTOCICLETA', 'BICICLETA')),
    marca_modelo TEXT,
    color TEXT,
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id),
    FOREIGN KEY (parqueadero_id) REFERENCES parqueaderos(id)
);

-- ============================================================================
-- 4. CONTABILIDAD (PUC PROPIEDAD HORIZONTAL COLOMBIA - NIIF GRUPO 3)
-- ============================================================================

CREATE TABLE puc_cuentas (
    codigo TEXT PRIMARY KEY, -- Ej: '111005', '130505', '417005', '513505'
    nombre TEXT NOT NULL,
    clase TEXT NOT NULL CHECK (clase IN ('ACTIVO', 'PASIVO', 'PATRIMONIO', 'INGRESOS', 'GASTOS', 'COSTOS')),
    naturaleza TEXT NOT NULL CHECK (naturaleza IN ('DEBITO', 'CREDITO')),
    nivel INTEGER NOT NULL, -- 1: Clase, 2: Grupo, 3: Cuenta, 4: Subcuenta, 5: Auxiliar
    cuenta_padre TEXT,
    permite_movimiento INTEGER NOT NULL DEFAULT 0 CHECK (permite_movimiento IN (0, 1)),
    exige_tercero INTEGER NOT NULL DEFAULT 1 CHECK (exige_tercero IN (0, 1)),
    FOREIGN KEY (cuenta_padre) REFERENCES puc_cuentas(codigo)
);

CREATE TABLE comprobantes_contables (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tipo_comprobante TEXT NOT NULL CHECK (tipo_comprobante IN ('INGRESO', 'EGRESO', 'NOTA_CONTABLE', 'FACTURACION', 'CIERRE')),
    consecutivo INTEGER NOT NULL,
    fecha TEXT NOT NULL, -- Formato 'YYYY-MM-DD'
    descripcion TEXT NOT NULL,
    estado TEXT NOT NULL DEFAULT 'CONTABILIZADO' CHECK (estado IN ('BORRADOR', 'CONTABILIZADO', 'ANULADO')),
    usuario_id INTEGER NOT NULL,
    creado_en TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    UNIQUE(tipo_comprobante, consecutivo),
    FOREIGN KEY (usuario_id) REFERENCES usuarios(id)
);

CREATE TABLE movimientos_contables (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    comprobante_id INTEGER NOT NULL,
    cuenta_codigo TEXT NOT NULL,
    tercero_id INTEGER NOT NULL,
    unidad_id INTEGER, -- Opcional, para trazabilidad por inmueble
    detalle TEXT NOT NULL,
    debito INTEGER NOT NULL DEFAULT 0 CHECK (debito >= 0),
    credito INTEGER NOT NULL DEFAULT 0 CHECK (credito >= 0),
    FOREIGN KEY (comprobante_id) REFERENCES comprobantes_contables(id),
    FOREIGN KEY (cuenta_codigo) REFERENCES puc_cuentas(codigo),
    FOREIGN KEY (tercero_id) REFERENCES terceros(id),
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id)
);

-- ============================================================================
-- 5. PRESUPUESTO ANUAL Y EJECUCIÓN PRESUPUESTAL (INCLUYE FONDO IMPREVISTOS)
-- ============================================================================

CREATE TABLE presupuestos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    anio INTEGER NOT NULL UNIQUE,
    fecha_aprobacion_asamblea TEXT NOT NULL,
    acta_asamblea_numero TEXT,
    valor_total_anual INTEGER NOT NULL,
    porcentaje_fondo_imprevistos REAL NOT NULL DEFAULT 1.0, -- Mínimo 1% Ley 675
    estado TEXT NOT NULL DEFAULT 'VIGENTE' CHECK (estado IN ('BORRADOR', 'VIGENTE', 'CERRADO'))
);

CREATE TABLE presupuesto_rubros (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    presupuesto_id INTEGER NOT NULL,
    cuenta_codigo TEXT NOT NULL, -- Vinculado al PUC (Ingresos 41xx o Gastos 51xx)
    nombre_rubro TEXT NOT NULL,
    tipo TEXT NOT NULL CHECK (tipo IN ('INGRESO', 'GASTO', 'FONDO_IMPREVISTOS')),
    valor_anual_aprobado INTEGER NOT NULL,
    valor_mensual_proyectado INTEGER NOT NULL,
    FOREIGN KEY (presupuesto_id) REFERENCES presupuestos(id),
    FOREIGN KEY (cuenta_codigo) REFERENCES puc_cuentas(codigo)
);

-- ============================================================================
-- 6. FACTURACIÓN, CUENTAS DE COBRO Y CARTERA
-- ============================================================================

CREATE TABLE conceptos_cobro (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    codigo TEXT NOT NULL UNIQUE, -- Ej: 'CUOTA_ORD', 'CUOTA_EXT', 'MORA', 'PARQ_ALQ', 'SALON_SOC', 'MULTA'
    nombre TEXT NOT NULL,
    cuenta_ingreso_codigo TEXT NOT NULL, -- Cuenta PUC 41xx
    cuenta_cartera_codigo TEXT NOT NULL, -- Cuenta PUC 13xx
    genera_interes_mora INTEGER NOT NULL DEFAULT 1 CHECK (genera_interes_mora IN (0, 1)),
    aplica_iva INTEGER NOT NULL DEFAULT 0 CHECK (aplica_iva IN (0, 1)),
    porcentaje_iva REAL DEFAULT 0.0,
    FOREIGN KEY (cuenta_ingreso_codigo) REFERENCES puc_cuentas(codigo),
    FOREIGN KEY (cuenta_cartera_codigo) REFERENCES puc_cuentas(codigo)
);

CREATE TABLE cuentas_cobro (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    numero_documento TEXT NOT NULL UNIQUE, -- Ej: 'CC-202611-0001'
    unidad_id INTEGER NOT NULL,
    tercero_id INTEGER NOT NULL, -- A quien se emite (propietario o inquilino)
    periodo TEXT NOT NULL, -- Ej: '2026-11'
    fecha_emision TEXT NOT NULL,
    fecha_vencimiento TEXT NOT NULL,
    fecha_limite_descuento TEXT,
    saldo_anterior INTEGER NOT NULL DEFAULT 0,
    valor_periodo INTEGER NOT NULL,
    valor_descuento_pronto_pago INTEGER NOT NULL DEFAULT 0,
    total_a_pagar INTEGER NOT NULL,
    saldo_pendiente INTEGER NOT NULL, -- Se reduce al registrar recibos de caja
    estado TEXT NOT NULL DEFAULT 'PENDIENTE' CHECK (estado IN ('PENDIENTE', 'PAGO_PARCIAL', 'PAGADA', 'VENCIDA', 'ANULADA')),
    comprobante_contable_id INTEGER,
    UNIQUE(unidad_id, periodo),
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id),
    FOREIGN KEY (tercero_id) REFERENCES terceros(id),
    FOREIGN KEY (comprobante_contable_id) REFERENCES comprobantes_contables(id)
);

CREATE TABLE cuentas_cobro_detalle (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cuenta_cobro_id INTEGER NOT NULL,
    concepto_id INTEGER NOT NULL,
    descripcion TEXT NOT NULL,
    valor INTEGER NOT NULL,
    iva INTEGER NOT NULL DEFAULT 0,
    saldo_concepto INTEGER NOT NULL, -- Permite saber exactamente cuánto debe de mora vs capital
    FOREIGN KEY (cuenta_cobro_id) REFERENCES cuentas_cobro(id),
    FOREIGN KEY (concepto_id) REFERENCES conceptos_cobro(id)
);

CREATE TABLE recibos_caja (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    numero_recibo TEXT NOT NULL UNIQUE, -- Ej: 'RC-000145'
    unidad_id     INTEGER NOT NULL,
    tercero_id    INTEGER NOT NULL,
    fecha_pago    TEXT NOT NULL,
    forma_pago    TEXT NOT NULL CHECK (forma_pago IN ('TRANSFERENCIA', 'CONSIGNACION', 'PSE', 'EFECTIVO', 'DATAFONO')),
    cuenta_banco_codigo     TEXT NOT NULL, -- Cuenta PUC 1110xx o 1105xx
    referencia_bancaria     TEXT,
    valor_total_pagado      INTEGER NOT NULL,
    observaciones           TEXT,
    comprobante_contable_id INTEGER,
    usuario_id              INTEGER NOT NULL,
    FOREIGN KEY (unidad_id)               REFERENCES unidades_privadas(id),
    FOREIGN KEY (tercero_id)              REFERENCES terceros(id),
    FOREIGN KEY (cuenta_banco_codigo)     REFERENCES puc_cuentas(codigo),
    FOREIGN KEY (comprobante_contable_id) REFERENCES comprobantes_contables(id),
    FOREIGN KEY (usuario_id)              REFERENCES usuarios(id)
);

-- Tabla de Historial Tasas de Mora
CREATE TABLE tasas_mora_historial (
    id                    INTEGER PRIMARY KEY AUTOINCREMENT,
    periodo               TEXT NOT NULL UNIQUE, -- Formato 'YYYY-MM-DD' (Ej: '2026-11')
    tasa_efectiva_anual   REAL NOT NULL,        -- Tasa EA certificada por Superfinanciera (Ej: 0.2781 = 27.81%)
    tasa_mensual_aplicada REAL NOT NULL,        -- Tasa mensual equivalente o definida por Asamblea (Ej: 0.0206)
    resolucion_sfc        TEXT,                 -- Ej: 'Resolución SFC 1890 de 2026'
    registrado_por_id     INTEGER NOT NULL,
    fecha_registro        TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (registrado_por_id) REFERENCES usuarios(id)
);

-- Cruce de pagos contra conceptos (siguiendo prelación legal: 1° Mora, 2° Capital antiguo)
CREATE TABLE recibos_caja_aplicacion (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    recibo_caja_id INTEGER NOT NULL,
    cuenta_cobro_detalle_id INTEGER NOT NULL,
    valor_aplicado INTEGER NOT NULL,
    FOREIGN KEY (recibo_caja_id) REFERENCES recibos_caja(id),
    FOREIGN KEY (cuenta_cobro_detalle_id) REFERENCES cuentas_cobro_detalle(id)
);


-- ============================================================================
-- 7. MÓDULOS COMPLEMENTARIOS (ZONAS COMUNES, ASAMBLEAS, PQRS, MANTENIMIENTO)
-- ============================================================================
CREATE TABLE zonas_comunes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre TEXT NOT NULL UNIQUE, -- Ej: 'Salón Social', 'Zona BBQ', 'Cancha Sintética'
    capacidad_personas INTEGER,
    valor_alquiler INTEGER NOT NULL DEFAULT 0,
    valor_deposito_garantia INTEGER NOT NULL DEFAULT 0,
    requiere_paz_y_salvo INTEGER NOT NULL DEFAULT 1 CHECK (requiere_paz_y_salvo IN (0, 1)),
    activa INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE reservas_zonas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zona_id INTEGER NOT NULL,
    unidad_id INTEGER NOT NULL,
    tercero_id INTEGER NOT NULL,
    fecha_reserva TEXT NOT NULL,
    hora_inicio TEXT NOT NULL,
    hora_fin TEXT NOT NULL,
    valor_cobrado INTEGER NOT NULL,
    estado TEXT NOT NULL DEFAULT 'CONFIRMADA' CHECK (estado IN ('PENDIENTE', 'CONFIRMADA', 'CANCELADA', 'FINALIZADA')),
    facturado INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (zona_id) REFERENCES zonas_comunes(id),
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id),
    FOREIGN KEY (tercero_id) REFERENCES terceros(id)
);

CREATE TABLE asambleas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tipo TEXT NOT NULL CHECK (tipo IN ('ORDINARIA', 'EXTRAORDINARIA')),
    fecha TEXT NOT NULL,
    hora_inicio TEXT NOT NULL,
    quorum_requerido REAL NOT NULL DEFAULT 0.500001, -- > 50% en primera convocatoria
    quorum_alcanzado REAL DEFAULT 0.0,
    estado TEXT NOT NULL DEFAULT 'PROGRAMADA' CHECK (estado IN ('PROGRAMADA', 'EN_CURSO', 'CERRADA'))
);

CREATE TABLE asamblea_asistencia (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    asamblea_id INTEGER NOT NULL,
    unidad_id INTEGER NOT NULL,
    asistente_nombre TEXT NOT NULL,
    es_apoderado INTEGER NOT NULL DEFAULT 0,
    coeficiente_representado REAL NOT NULL,
    hora_registro TEXT NOT NULL,
    UNIQUE(asamblea_id, unidad_id),
    FOREIGN KEY (asamblea_id) REFERENCES asambleas(id),
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id)
);

CREATE TABLE pqrs_sanciones (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    radicado TEXT NOT NULL UNIQUE,
    unidad_id INTEGER,
    tercero_id INTEGER NOT NULL,
    tipo TEXT NOT NULL CHECK (tipo IN ('PETICION', 'QUEJA', 'RECLAMO', 'SUGERENCIA', 'PROCESO_SANCIONATORIO')),
    asunto TEXT NOT NULL,
    descripcion TEXT NOT NULL,
    fecha_radicacion TEXT NOT NULL,
    fecha_descargos TEXT,
    valor_multa INTEGER DEFAULT 0,
    estado TEXT NOT NULL DEFAULT 'ABIERTA' CHECK (estado IN ('ABIERTA', 'EN_DESCARGOS', 'SANCIONADA', 'CERRADA')),
    FOREIGN KEY (unidad_id) REFERENCES unidades_privadas(id),
    FOREIGN KEY (tercero_id) REFERENCES terceros(id)
);

CREATE TABLE mantenimientos_contratos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    proveedor_id INTEGER NOT NULL,
    equipo_o_servicio TEXT NOT NULL, -- Ej: 'Ascensores', 'Póliza Zonas Comunes', 'Motobombas'
    periodicidad_dias INTEGER NOT NULL, -- Ej: 30, 180, 365
    ultimo_mantenimiento TEXT NOT NULL,
    proximo_mantenimiento TEXT NOT NULL,
    valor_contrato_mensual INTEGER DEFAULT 0,
    alerta_dias_anticipacion INTEGER NOT NULL DEFAULT 15,
    FOREIGN KEY (proveedor_id) REFERENCES terceros(id)
);


-- ============================================================================
-- 8. ÍNDICES PARA CONSULTAS RÁPIDAS EN C++
-- ============================================================================
CREATE INDEX idx_unidades_propietario ON unidades_privadas(propietario_principal_id);
CREATE INDEX idx_movimientos_cuenta ON movimientos_contables(cuenta_codigo);
CREATE INDEX idx_movimientos_tercero ON movimientos_contables(tercero_id);
CREATE INDEX idx_cuentas_cobro_unidad ON cuentas_cobro(unidad_id, estado);
CREATE INDEX idx_recibos_unidad ON recibos_caja(unidad_id, fecha_pago);
