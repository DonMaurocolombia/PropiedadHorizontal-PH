# Facturación y cobro de servicios

El módulo permite preparar cobros locales a propietarios, inquilinos, residentes o visitantes registrados. Cada cobro identifica la unidad privada visitada, aunque el responsable de pago sea otra persona, y puede incluir una o más líneas en COP. Los conceptos y sus cuentas PUC se administran en **Conceptos de cobro de servicios**; se pueden crear, editar o eliminar, pero no se pueden eliminar ni cambiar sus cuentas cuando ya tienen facturas o cargos asociados.

Los conceptos iniciales enlazan parqueaderos de visitantes con ingresos `4017030` y cartera `1030530`; salón comunal con `4017025` y `1030525`; y bodegas o depósitos con `4017035` y `1030535`. Las cuentas pueden configurarse en el módulo de conceptos para cobros adicionales, por ejemplo, zonas de asados.

El cobro puede finalizar de dos maneras:

- **Confirmar pago:** registra el recibo y un comprobante contable que debita la cuenta de caja o bancos elegida y acredita la cuenta de ingresos del concepto.
- **Cobrar en próxima cuenta de la unidad:** deja el cargo pendiente para la siguiente facturación mensual de la unidad visitada. Al generarla, añade el concepto y su valor al detalle de la cuenta, contabiliza débito a cartera y crédito a ingresos, y marca el cargo como `INCLUIDA_EN_CUENTA`. El recaudo normal de cuentas de cobro puede aplicarse después.

Los comprobantes contabilizados alimentan los reportes financieros existentes. Los borradores por sí solos no generan movimientos contables. Los cobros de servicios no calculan ni muestran IVA en esta etapa.

La referencia `BORRADOR-<id>` es local y no es un consecutivo autorizado. El módulo todavía no genera XML UBL, firma documentos, calcula CUFE, transmite a la DIAN ni recibe validaciones. Para implementar transmisión se requiere completar la habilitación del software propio ante la DIAN y validar con el contador las obligaciones tributarias aplicables a cada servicio.
