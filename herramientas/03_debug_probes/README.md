# 03 - Debug probes (sondas de depuración)

El build no depende de la sonda, pero grabar y depurar sí: el software del host debe poder
usar el hardware que conecta la PC con el target. Ese hardware es el *debug probe* o sonda de
depuración.

Esta parte comienza con una idea clave:

> **Una sonda no es un cable pasivo: es hardware activo. En muchos casos es otro
> microcontrolador con firmware propio.**

## Recorrido

1. [01 - Una sonda suele ser otro micro](./01-un-probe-es-otro-micro.md)
   Qué hay dentro del circuito de depuración integrado en una placa, qué ejecuta y qué
   consecuencias prácticas tiene que sea un micro y no un cable.
2. [02 - CMSIS-DAP](./02-cmsis-dap.md)
   El estándar abierto de ARM: qué significa la sigla, qué define exactamente, la diferencia
   entre v1 y v2, y **cómo se carga** en una sonda.
3. [03 - Catálogo de sondas](./03-catalogo-de-probes.md)
   Las familias más comunes: CMSIS-DAP, J-Link, ST-Link, LPC-Link, MCU-Link, Black Magic y
   FT2232. Cuáles usan protocolos abiertos o propietarios y qué ofrece cada una.
4. [04 - El software del lado de la PC](./04-el-software-del-host.md)
   OpenOCD, pyOCD, LinkServer, las herramientas de SEGGER. La arquitectura del gdbserver, que
   es la misma en todos y explica por qué son intercambiables.
5. [05 - Armarte tu propia sonda](./05-armarte-tu-propia-sonda.md)
   Cómo convertir una Raspberry Pi Pico en una sonda CMSIS-DAP v2 para SWD y UART, y qué
   límites tienen las demás alternativas.

## La conclusión práctica, por adelantado

Si tu sonda implementa **CMSIS-DAP** y la herramienta soporta el target, OpenOCD o pyOCD
pueden manejarla sin el protocolo propietario del fabricante. Si no, las opciones habituales
son usar la herramienta correspondiente, cargar un firmware alternativo compatible o grabar
mediante el bootloader del chip.

## Lo que no cambia, sea cual sea tu sonda

- **El código, el `Makefile`, el linker script y el startup.** La sonda no interviene en la
  compilación.
- **El comando.** Con la [plantilla](../../plantilla/) es siempre `make flash`, que detecta
  qué grabador tenés instalado. También se puede forzar: `make flash FLASHER=pyocd`.
- **La interfaz de GDB.** En los flujos más comunes, GDB se conecta a un servidor mediante
  el Remote Serial Protocol. Cambia el servidor y su configuración; el firmware no.

Por eso conviene mantener separada la configuración de la sonda: pertenece al entorno de
desarrollo y no debería condicionar el código de la aplicación.

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [02 - Protocolos y debug en el chip](../02_protocolos_y_debug_en_el_chip/) ·
**Siguiente parte:** [04 - Toolchains](../04_toolchains/) ·
**La sonda de la cátedra:** [07-01](../07_lpc1769/01-la-placa-y-su-sonda.md)
