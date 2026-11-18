-- ============================================================================
-- ROLES INICIALES Y CONCEPTOS DE COBRO
-- ============================================================================
INSERT INTO roles (id, nombre, descripcion) VALUES
(0, 'SISTEMA', 'Configuración inicial del sistema'),
(1, 'ADMIN', 'Administrador general con acceso total'),
(2, 'CONTADOR', 'Gestión contable, presupuesto, cierres y reportes'),
(3, 'REVISOR_FISCAL', 'Consulta de auditoría, libros y estados financieros (solo lectura)'),
(4, 'ASISTENTE', 'Facturación, cartera, residentes y reservas');

-- ============================================================================
-- PLAN ÚNICO DE CUENTAS (PUC) - PROPIEDAD HORIZONTAL COLOMBIA (OT-15 CTCP)
-- ============================================================================

-- CLASE 1: ACTIVO
INSERT INTO puc_cuentas (codigo, nombre, clase, naturaleza, nivel, cuenta_padre, permite_movimiento, exige_tercero) VALUES
('1',      'ACTIVO', 'ACTIVO', 'DEBITO', 1, NULL, 0, 0),
('11',     'EFECTIVO Y EQUIVALENTES AL EFECTIVO', 'ACTIVO', 'DEBITO', 2, '1', 0, 0),
('1105',   'CAJA', 'ACTIVO', 'DEBITO', 3, '11', 0, 0),
('110505', 'Caja General', 'ACTIVO', 'DEBITO', 4, '1105', 1, 1),
('110510', 'Caja Menor', 'ACTIVO', 'DEBITO', 4, '1105', 1, 1),
('1110',   'BANCOS', 'ACTIVO', 'DEBITO', 3, '11', 0, 0),
('111005', 'Cuenta Corriente Principal', 'ACTIVO', 'DEBITO', 4, '1110', 1, 1),
('1120',   'CUENTAS DE AHORRO', 'ACTIVO', 'DEBITO', 3, '11', 0, 0),
('112005', 'Cuenta Ahorros Operaciones', 'ACTIVO', 'DEBITO', 4, '1120', 1, 1),
('112010', 'Cuenta Ahorros Fondo de Imprevistos (Ley 675)', 'ACTIVO', 'DEBITO', 4, '1120', 1, 1),

('13',     'DEUDORES (CARTERA COPROPIETARIOS)', 'ACTIVO', 'DEBITO', 2, '1', 0, 0),
('1305',   'CUOTAS DE ADMINISTRACIÓN POR COBRAR', 'ACTIVO', 'DEBITO', 3, '13', 0, 0),
('130505', 'Cuotas Ordinarias de Administración', 'ACTIVO', 'DEBITO', 4, '1305', 1, 1),
('130510', 'Cuotas Extraordinarias', 'ACTIVO', 'DEBITO', 4, '1305', 1, 1),
('130515', 'Intereses de Mora por Cobrar', 'ACTIVO', 'DEBITO', 4, '1305', 1, 1),
('130520', 'Sanciones y Multas por Cobrar', 'ACTIVO', 'DEBITO', 4, '1305', 1, 1),
('130525', 'Alquiler Zonas Comunes y Parqueaderos', 'ACTIVO', 'DEBITO', 4, '1305', 1, 1),
('1399',   'DETERIORO DE CARTERA (PROVISIÓN)', 'ACTIVO', 'CREDITO', 3, '13', 0, 0),
('139905', 'Deterioro Acumulado Cuotas Administración', 'ACTIVO', 'CREDITO', 4, '1399', 1, 1);

-- CLASE 2: PASIVO
INSERT INTO puc_cuentas (codigo, nombre, clase, naturaleza, nivel, cuenta_padre, permite_movimiento, exige_tercero) VALUES
('2',      'PASIVO', 'PASIVO', 'CREDITO', 1, NULL, 0, 0),
('23',     'CUENTAS POR PAGAR', 'PASIVO', 'CREDITO', 2, '2', 0, 0),
('2335',   'COSTOS Y GASTOS POR PAGAR', 'PASIVO', 'CREDITO', 3, '23', 0, 0),
('233505', 'Honorarios (Contador / Revisor / Administrador)', 'PASIVO', 'CREDITO', 4, '2335', 1, 1),
('233510', 'Servicios de Vigilancia y Seguridad', 'PASIVO', 'CREDITO', 4, '2335', 1, 1),
('233515', 'Servicios de Aseo y Jardinería', 'PASIVO', 'CREDITO', 4, '2335', 1, 1),
('233520', 'Mantenimiento y Reparaciones Locativas', 'PASIVO', 'CREDITO', 4, '2335', 1, 1),
('233525', 'Servicios Públicos por Pagar', 'PASIVO', 'CREDITO', 4, '2335', 1, 1),
('2365',   'RETENCIÓN EN LA FUENTE POR PAGAR', 'PASIVO', 'CREDITO', 3, '23', 0, 0),
('236515', 'Retefuente Honorarios (10% / 11%)', 'PASIVO', 'CREDITO', 4, '2365', 1, 1),
('236525', 'Retefuente Servicios (4% / 6%)', 'PASIVO', 'CREDITO', 4, '2365', 1, 1),
('236540', 'Retefuente Compras (2.5% / 3.5%)', 'PASIVO', 'CREDITO', 4, '2365', 1, 1),
('28',     'OTROS PASIVOS (ANTICIPOS)', 'PASIVO', 'CREDITO', 2, '2', 0, 0),
('2805',   'ANTICIPOS Y AVANCES RECIBIDOS', 'PASIVO', 'CREDITO', 3, '28', 0, 0),
('280505', 'Cuotas Pagadas por Anticipado Copropietarios', 'PASIVO', 'CREDITO', 4, '2805', 1, 1),
('280510', 'Depósitos en Garantía Salón Social / Obras', 'PASIVO', 'CREDITO', 4, '2805', 1, 1);

-- CLASE 3: PATRIMONIO
INSERT INTO puc_cuentas (codigo, nombre, clase, naturaleza, nivel, cuenta_padre, permite_movimiento, exige_tercero) VALUES
('3',      'PATRIMONIO', 'PATRIMONIO', 'CREDITO', 1, NULL, 0, 0),
('31',     'FONDO SOCIAL / RESERVAS', 'PATRIMONIO', 'CREDITO', 2, '3', 0, 0),
('3105',   'FONDO DE IMPREVISTOS (ART. 35 LEY 675)', 'PATRIMONIO', 'CREDITO', 3, '31', 0, 0),
('310505', 'Reserva Fondo de Imprevistos Acumulado', 'PATRIMONIO', 'CREDITO', 4, '3105', 1, 0),
('36',     'RESULTADOS DEL EJERCICIO', 'PATRIMONIO', 'CREDITO', 2, '3', 0, 0),
('3605',   'EXCEDENTES DEL EJERCICIO', 'PATRIMONIO', 'CREDITO', 3, '36', 0, 0),
('360505', 'Excedentes del Periodo', 'PATRIMONIO', 'CREDITO', 4, '3605', 1, 0),
('3610',   'DÉFICIT DEL EJERCICIO', 'PATRIMONIO', 'DEBITO', 3, '36', 0, 0),
('361005', 'Déficit del Periodo', 'PATRIMONIO', 'DEBITO', 4, '3610', 1, 0);

-- CLASE 4: INGRESOS
INSERT INTO puc_cuentas (codigo, nombre, clase, naturaleza, nivel, cuenta_padre, permite_movimiento, exige_tercero) VALUES
('4',      'INGRESOS', 'INGRESOS', 'CREDITO', 1, NULL, 0, 0),
('41',     'INGRESOS OPERACIONALES COPROPIEDAD', 'INGRESOS', 'CREDITO', 2, '4', 0, 0),
('4170',   'EXPENSAS COMUNES Y OTROS INGRESOS', 'INGRESOS', 'CREDITO', 3, '41', 0, 0),
('417005', 'Cuotas Ordinarias de Administración', 'INGRESOS', 'CREDITO', 4, '4170', 1, 1),
('417010', 'Cuotas Extraordinarias', 'INGRESOS', 'CREDITO', 4, '4170', 1, 1),
('417015', 'Intereses de Mora Expensas Comunes', 'INGRESOS', 'CREDITO', 4, '4170', 1, 1),
('417020', 'Multas y Sanciones Asamblea / Consejo', 'INGRESOS', 'CREDITO', 4, '4170', 1, 1),
('417025', 'Explotación Zonas Comunes y Parqueaderos', 'INGRESOS', 'CREDITO', 4, '4170', 1, 1),
('42',     'INGRESOS NO OPERACIONALES', 'INGRESOS', 'CREDITO', 2, '4', 0, 0),
('4210',   'FINANCIEROS', 'INGRESOS', 'CREDITO', 3, '42', 0, 0),
('421005', 'Rendimientos Financieros Cuentas Ahorro', 'INGRESOS', 'CREDITO', 4, '4210', 1, 1);

-- CLASE 5: GASTOS
INSERT INTO puc_cuentas (codigo, nombre, clase, naturaleza, nivel, cuenta_padre, permite_movimiento, exige_tercero) VALUES
('5',      'GASTOS', 'GASTOS', 'DEBITO', 1, NULL, 0, 0),
('51',     'GASTOS DE ADMINISTRACIÓN Y OPERACIÓN', 'GASTOS', 'DEBITO', 2, '5', 0, 0),
('5110',   'HONORARIOS', 'GASTOS', 'DEBITO', 3, '51', 0, 0),
('511005', 'Honorarios Administración', 'GASTOS', 'DEBITO', 4, '5110', 1, 1),
('511010', 'Honorarios Contabilidad', 'GASTOS', 'DEBITO', 4, '5110', 1, 1),
('511015', 'Honorarios Revisoría Fiscal', 'GASTOS', 'DEBITO', 4, '5110', 1, 1),
('5130',   'SEGUROS (ART. 15 LEY 675)', 'GASTOS', 'DEBITO', 3, '51', 0, 0),
('513005', 'Póliza Obligatoria Áreas Comunes (Incendio/Terremoto)', 'GASTOS', 'DEBITO', 4, '5130', 1, 1),
('5135',   'SERVICIOS CONTRATADOS', 'GASTOS', 'DEBITO', 3, '51', 0, 0),
('513505', 'Servicio de Vigilancia y Seguridad Privada', 'GASTOS', 'DEBITO', 4, '5135', 1, 1),
('513510', 'Servicio de Aseo, Cafetería y Jardinería', 'GASTOS', 'DEBITO', 4, '5135', 1, 1),
('513525', 'Acueducto y Alcantarillado Áreas Comunes', 'GASTOS', 'DEBITO', 4, '5135', 1, 1),
('513530', 'Energía Eléctrica Áreas Comunes', 'GASTOS', 'DEBITO', 4, '5135', 1, 1),
('5145',   'MANTENIMIENTO Y REPARACIONES', 'GASTOS', 'DEBITO', 3, '51', 0, 0),
('514505', 'Mantenimiento Ascensores y Escaleras', 'GASTOS', 'DEBITO', 4, '5145', 1, 1),
('514510', 'Mantenimiento Motobombas, Tanques y Plantas', 'GASTOS', 'DEBITO', 4, '5145', 1, 1),
('514515', 'Reparaciones Locativas Zonas Comunes', 'GASTOS', 'DEBITO', 4, '5145', 1, 1),
('53',     'GASTOS NO OPERACIONALES', 'GASTOS', 'DEBITO', 2, '5', 0, 0),
('5305',   'FINANCIEROS', 'GASTOS', 'DEBITO', 3, '53', 0, 0),
('530505', 'Comisiones Bancarias y GMF (4x1000)', 'GASTOS', 'DEBITO', 4, '5305', 1, 1),
('530535', 'Descuentos Comerciales Condicionados (Pronto Pago)', 'GASTOS', 'DEBITO', 4, '5305', 1, 1);

-- ============================================================================
-- CONCEPTOS DE COBRO PREDEFINIDOS
-- ============================================================================
INSERT INTO conceptos_cobro (codigo, nombre, cuenta_ingreso_codigo, cuenta_cartera_codigo, genera_interes_mora, aplica_iva, porcentaje_iva) VALUES
('CUOTA_ORD', 'Cuota Ordinaria de Administración', '417005', '130505', 1, 0, 0.0),
('CUOTA_EXT', 'Cuota Extraordinaria', '417010', '130510', 1, 0, 0.0),
('MORA', 'Intereses de Mora', '417015', '130515', 0, 0, 0.0),
('MULTA', 'Multa / Sanción Convivencia', '417020', '130520', 0, 0, 0.0),
('PARQ_ALQ', 'Alquiler Parqueadero Común', '417025', '130525', 1, 1, 19.0),
('SALON_SOC', 'Alquiler Salón Social / Zona BBQ', '417025', '130525', 0, 1, 19.0);


-- ============================================================================
-- CREA USUARIO SISTEMA PARA PODER INCLUIR EL HISTORIAL DE TASAS DE MORA
-- ============================================================================
INSERT INTO usuarios (id, rol_id, username, password_hash, nombre_completo) VALUES
(0, 0, 'SISTEMA', 'Sistema123*', 'SISTEMA');

-- ============================================================================
-- TASAS DE MORA HISTORICAS DESDE ABRIL DE 2016
-- ============================================================================
INSERT INTO tasas_mora_historial (id, periodo, tasa_efectiva_anual, tasa_mensual_aplicada, resolucion_sfc, registrado_por_id) VALUES
(1,  '2016-04', 0.2054, 0.02263, '0334', 0),
(2,  '2016-05', 0.2054, 0.02263, '0334', 0),
(3,  '2016-06', 0.2054, 0.02263, '0334', 0),
(4,  '2016-07', 0.2134, 0.02341, '0811', 0),
(5,  '2016-08', 0.2134, 0.02341, '0811', 0),
(6,  '2016-09', 0.2134, 0.02341, '0811', 0),
(7,  '2016-10', 0.2199, 0.02404, '1233', 0),
(8,  '2016-11', 0.2199, 0.02404, '1233', 0),
(9,  '2016-12', 0.2199, 0.02404, '1233', 0),
(10, '2017-01', 0.2234, 0.02438, '1612', 0),
(11, '2017-02', 0.2234, 0.02438, '1612', 0),
(12, '2017-03', 0.2234, 0.02438, '1612', 0),
(13, '2017-04', 0.2233, 0.02437, '488',  0),
(14, '2017-05', 0.2233, 0.02437, '488',  0),
(15, '2017-06', 0.2233, 0.02437, '488',  0),
(16, '2017-07', 0.2198, 0.02403, '907',  0),
(17, '2017-08', 0.2198, 0.02403, '907',  0),
(18, '2017-09', 0.2148, 0.02355, '1155', 0),
(19, '2017-10', 0.2115, 0.02323, '1298', 0),
(20, '2017-11', 0.2096, 0.02304, '1447', 0),
(21, '2017-12', 0.2077, 0.02286, '1619', 0),
(22, '2018-01', 0.2069, 0.02278, '1890', 0),
(23, '2018-02', 0.2101, 0.02309,  '131', 0),
(24, '2018-03', 0.2068, 0.02277,  '259', 0),
(25, '2018-04', 0.2048, 0.02257,  '398', 0),
(26, '2018-05', 0.2044, 0.02254,  '527', 0),
(27, '2018-06', 0.2028, 0.02238,  '687', 0),
(28, '2018-07', 0.2003, 0.02213,  '820', 0),
(29, '2018-08', 0.1994, 0.02205,  '954', 0),
(30, '2018-09', 0.1981, 0.02192, '1112', 0),
(31, '2018-10', 0.1963, 0.02174, '1294', 0),
(32, '2018-11', 0.1949, 0.0216,  '1521', 0),
(33, '2018-12', 0.194,  0.02151, '1708', 0),
(34, '2019-01', 0.1916, 0.02128, '1872', 0),
(35, '2019-02', 0.197,  0.02181,  '111', 0),
(36, '2019-03', 0.1937, 0.02148,  '263', 0),
(37, '2019-04', 0.1932, 0.02143,  '389', 0),
(38, '2019-05', 0.1934, 0.02145,  '574', 0),
(39, '2019-06', 0.193,  0.02141,  '697', 0),
(40, '2019-07', 0.1928, 0.02139,  '829', 0),
(41, '2019-08', 0.1932, 0.02143, '1018', 0),
(42, '2019-09', 0.1932, 0.02143, '1145', 0),
(43, '2019-10', 0.191,  0.02122, '1293', 0),
(44, '2019-11', 0.1903, 0.02115, '1474', 0),
(45, '2019-12', 0.1891, 0.02103, '1603', 0),
(46, '2020-01', 0.1877, 0.02089, '1768', 0),
(47, '2020-02', 0.1906, 0.02118,   '94', 0),
(48, '2020-03', 0.1895, 0.02107,  '205', 0),
(49, '2020-04', 0.1869, 0.02081,  '351', 0),
(50, '2020-05', 0.1819, 0.02031,  '437', 0),
(51, '2020-06', 0.1812, 0.02024,  '505', 0),
(52, '2020-07', 0.1812, 0.02024,  '605', 0),
(53, '2020-08', 0.1829, 0.02041,  '685', 0),
(54, '2020-09', 0.1835, 0.02047,  '769', 0),
(55, '2020-10', 0.1809, 0.02021,  '869', 0),
(56, '2020-11', 0.1784, 0.01996,  '947', 0),
(57, '2020-12', 0.1746, 0.01957, '1034', 0),
(58, '2021-01', 0.1732, 0.01943, '1215', 0),
(59, '2021-02', 0.1754, 0.01965,   '64', 0),
(60, '2021-03', 0.1741, 0.01952,  '161', 0),
(61, '2021-04', 0.1731, 0.01942,  '305', 0),
(62, '2021-05', 0.1722, 0.01933,  '407', 0),
(63, '2021-06', 0.1721, 0.01932,  '509', 0),
(64, '2021-07', 0.1718, 0.01929,  '622', 0);

INSERT INTO tasas_mora_historial (id, periodo, tasa_efectiva_anual, tasa_mensual_aplicada, resolucion_sfc, registrado_por_id) VALUES
(65,  '2021-08', 0.1724, 0.01935,  '804', 0),
(66,  '2021-09', 0.1719, 0.0193,   '931', 0),
(67,  '2021-10', 0.1708, 0.01919, '1095', 0),
(68,  '2021-11', 0.1727, 0.01938, '1259', 0),
(69,  '2021-12', 0.1746, 0.01957, '1405', 0),
(70,  '2022-01', 0.1766, 0.01978, '1597', 0),
(71,  '2022-02', 0.183,  0.02042,  '143', 0),
(72,  '2022-03', 0.1847, 0.02059,  '256', 0),
(73,  '2022-04', 0.1905, 0.02117,  '382', 0),
(74,  '2022-05', 0.1971, 0.02182,  '498', 0),
(75,  '2022-06', 0.204,  0.0225,   '617', 0),
(76,  '2022-07', 0.2128, 0.02335,  '801', 0),
(77,  '2022-08', 0.2221, 0.02425,  '973', 0),
(78,  '2022-09', 0.235,  0.02548, '1126', 0),
(79,  '2022-10', 0.2461, 0.02653, '1327', 0),
(80,  '2022-11', 0.2578, 0.02762, '1537', 0),
(81,  '2022-12', 0.2764, 0.02933, '1715', 0),
(82,  '2023-01', 0.2884, 0.03041, '1968', 0),
(83,  '2023-02', 0.3018, 0.03161,  '100', 0),
(84,  '2023-03', 0.3084, 0.03219,  '236', 0),
(85,  '2023-04', 0.3139, 0.03268,  '472', 0),
(86,  '2023-05', 0.3027, 0.03169,  '606', 0),
(87,  '2023-06', 0.2976, 0.03123,  '766', 0),
(88,  '2023-07', 0.2936, 0.03088,  '945', 0),
(89,  '2023-08', 0.2875, 0.03033, '1090', 0),
(90,  '2023-09', 0.2803, 0.02968, '1328', 0),
(91,  '2023-10', 0.2653, 0.02831, '1520', 0),
(92,  '2023-11', 0.2552, 0.02738, '1801', 0),
(93,  '2023-12', 0.2504, 0.02693, '2074', 0),
(94,  '2024-01', 0.2332, 0.02531, '2331', 0),
(95,  '2024-02', 0.2331, 0.0253,   '150', 0),
(96,  '2024-03', 0.222,  0.02424,  '400', 0),
(97,  '2024-04', 0.2206, 0.02411,  '598', 0),
(98,  '2024-05', 0.2102, 0.0231,   '872', 0),
(99,  '2024-06', 0.2056, 0.02265, '1143', 0),
(100, '2024-07', 0.1966, 0.02177, '1308', 0),
(101, '2024-08', 0.1947, 0.02158, '1519', 0),
(102, '2024-09', 0.1923, 0.02134, '1688', 0),
(103, '2024-10', 0.1878, 0.0209,  '1901', 0),
(104, '2024-11', 0.186,  0.02072, '2168', 0),
(105, '2024-12', 0.1759, 0.01971, '2394', 0),
(106, '2025-01', 0.1659, 0.01869, '2620', 0),
(107, '2025-02', 0.1753, 0.01964,  '138', 0),
(108, '2025-03', 0.1661, 0.01871,  '352', 0),
(109, '2025-04', 0.1708, 0.01919,  '579', 0),
(110, '2025-05', 0.1731, 0.01942,  '837', 0),
(111, '2025-06', 0.1703, 0.01914, '1078', 0),
(112, '2025-07', 0.1652, 0.01862, '1254', 0),
(113, '2025-08', 0.1678, 0.01888, '1490', 0),
(114, '2025-09', 0.1667, 0.01877, '1658', 0),
(115, '2025-10', 0.1624, 0.01833, '1821', 0),
(116, '2025-11', 0.1666, 0.01876, '1995', 0),
(117, '2025-12', 0.1668, 0.01878, '2134', 0),
(118, '2026-01', 0.1624, 0.01833, '2288', 0),
(119, '2026-02', 0.1682, 0.01893,  '250', 0),
(120, '2026-03', 0.1701, 0.01912,  '405', 0),
(121, '2026-04', 0.1784, 0.01996,  '517', 0),
(122, '2026-05', 0.1878, 0.0209,   '662', 0),
(123, '2026-06', 0.1919, 0.0213,   '823', 0),
(124, '2026-07', 0.1919, 0.0213,   '965', 0),
(125, '2026-08', 0.1977, 0.02188, '1139', 0),
(126, '2026-09', 0.1949, 0.0216,  '1260', 0),
(127, '2026-10', 0.1906, 0.02118, '1472', 0),
(128, '2026-11', 0.1889, 0.02101, '1707', 0);
