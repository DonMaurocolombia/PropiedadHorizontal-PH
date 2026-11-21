CREATE TABLE IF NOT EXISTS facturas_servicio_migraciones (
    version INTEGER PRIMARY KEY,
    aplicada_en TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);
