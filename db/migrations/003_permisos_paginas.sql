CREATE TABLE IF NOT EXISTS paginas_aplicacion (
    codigo      TEXT PRIMARY KEY,
    nombre      TEXT NOT NULL,
    orden       INTEGER NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS roles_permisos_pagina (
    rol_id          INTEGER NOT NULL,
    pagina_codigo   TEXT NOT NULL,
    habilitado      INTEGER NOT NULL DEFAULT 0 CHECK (habilitado IN (0, 1)),
    PRIMARY KEY (rol_id, pagina_codigo),
    FOREIGN KEY (rol_id) REFERENCES roles(id),
    FOREIGN KEY (pagina_codigo) REFERENCES paginas_aplicacion(codigo)
);

INSERT INTO paginas_aplicacion (codigo, nombre, orden) VALUES
('FACTURAS_SERVICIO', 'Facturas de servicios', 1),
('UNIDADES', 'Unidades privadas', 2),
('TERCEROS', 'Terceros', 3),
('PARQUEADEROS', 'Parqueaderos y bodegas', 4),
('FACTURACION', 'Cuentas de cobro', 5),
('RECAUDO', 'Recaudo y paz y salvos', 6),
('EGRESOS', 'Egresos y proveedores', 7),
('PRESUPUESTO', 'Presupuesto', 8),
('REPORTES', 'Reportes financieros', 9),
('ASAMBLEAS', 'Asambleas y quórum', 10),
('SEGURIDAD', 'Usuarios y auditoría', 11),
('CONCEPTOS_COBRO', 'Conceptos de cobro', 12),
('CONFIGURACION', 'Configuración', 13),
('CUENTAS_CONTABLES', 'Cuentas contables', 14),
('GESTION_PERMISOS', 'Permisos por perfil', 15)
ON CONFLICT (codigo) DO NOTHING;

INSERT INTO roles_permisos_pagina (rol_id, pagina_codigo, habilitado)
SELECT r.id, p.codigo,
    CASE
        WHEN r.nombre = 'ADMIN' THEN 1
        WHEN r.nombre = 'CONTADOR' THEN
            CASE WHEN p.codigo = 'GESTION_PERMISOS' THEN 0 ELSE 1 END
        WHEN r.nombre = 'REVISOR_FISCAL' THEN
            CASE WHEN p.codigo IN (
                'FACTURACION', 'RECAUDO', 'EGRESOS', 'CONFIGURACION',
                'CONCEPTOS_COBRO', 'CUENTAS_CONTABLES', 'GESTION_PERMISOS'
            ) THEN 0 ELSE 1 END
        WHEN r.nombre = 'ASISTENTE' THEN
            CASE WHEN p.codigo IN (
                'EGRESOS', 'PRESUPUESTO', 'SEGURIDAD', 'CONFIGURACION',
                'CONCEPTOS_COBRO', 'CUENTAS_CONTABLES', 'GESTION_PERMISOS'
            ) THEN 0 ELSE 1 END
        ELSE 0
    END
FROM roles r
CROSS JOIN paginas_aplicacion p
WHERE r.nombre <> 'SISTEMA'
ON CONFLICT (rol_id, pagina_codigo) DO NOTHING;
