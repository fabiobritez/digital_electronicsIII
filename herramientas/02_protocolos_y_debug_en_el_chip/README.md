# 02 - Protocolos y debug adentro del chip

Un depurador puede frenar un programa, inspeccionar memoria y reanudarlo sin que la
aplicación implemente una consola ni un comando especial. Eso requiere hardware dentro del
chip, además de la sonda y del software del host.

Esta parte es sobre ese hardware y sobre los dos protocolos con los que se llega a él.

## Recorrido

1. [01 - JTAG](./01-jtag.md)
   El estándar que nació para **probar interconexiones** y después se aprovechó como puerto
   de depuración. La máquina de estados TAP, la cadena de chips, y por qué
   sigue existiendo.
2. [02 - SWD](./02-swd.md)
   La alternativa de Arm para llegar al mismo sistema de depuración con **dos señales**. Cómo funciona el protocolo, por qué es
   lo que se usa en Cortex-M, y cómo conviven los dos sobre los mismos pines.
3. [03 - El debug adentro del Cortex-M3](./03-adentro-del-cortex-m3.md)
   **El capítulo central de esta parte.** Qué bloques trae el núcleo: el DAP (y sus DP y AP),
   el FPB de breakpoints, el DWT de watchpoints, el ITM de traza. Por qué se puede leer la
   memoria sin frenar el CPU, y qué se frena y qué no cuando sí lo frenás.
4. [04 - Las vías de salida de datos](./04-vias-de-salida-de-datos.md)
   Las cuatro formas de que un micro te cuente algo: UART, semihosting, SWO/ITM y RTT.
   Cuánto cuesta cada una y en qué se diferencian de verdad.

## Lo que hay que llevarse de esta parte

Tres ideas, y con eso alcanza:

**1. El puerto de depuración no es un "protocolo de grabación".** Da acceso al sistema de
debug y, mediante un Access Port, al bus del chip. Leer variables, controlar el núcleo y
programar la FLASH se construyen sobre esos accesos y sobre algoritmos específicos del
target.

**2. Ese acceso no depende de una rutina de tu aplicación.** Por eso puede funcionar con la
FLASH vacía o con el CPU detenido. Aun así, la alimentación, los clocks, el reset, el bajo
consumo y la protección de lectura pueden impedir la conexión.

**3. La capacidad final la limita la cadena completa.** En el LPC1769 hay salida SWO, seis
comparadores de breakpoint y cuatro comparadores del DWT. Si la sonda, su firmware o el
software del host no soportan una función, no alcanza con que el chip la implemente.

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [01 - Panorama](../01_panorama/) ·
**Siguiente parte:** [03 - Debug probes](../03_debug_probes/)
