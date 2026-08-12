# 06 - Depurar en serio

En un sistema embebido no suele haber una consola disponible por defecto. Para entender qué ocurre
hay que elegir una señal observable, medirla y conocer cuánto altera al programa.

Las partes anteriores explicaron **cómo funciona** el hardware de depuración
([parte 02](../02_protocolos_y_debug_en_el_chip/)) y **qué hay entre tu PC y el micro**
([parte 03](../03_debug_probes/)). Esta parte es la práctica: qué hacés vos, con esas
herramientas, cuando algo no anda.

## Recorrido

1. [01 - Imprimir para depurar](./01-imprimir-para-depurar.md)
   El LED de diagnóstico, la UART como consola, `printf` por serial y el debug framework de
   NXP (`_DBG`, `_DBD`, `_DBH`).
2. [02 - El método del "no anda"](./02-el-metodo-del-no-anda.md)
   Breakpoints, watchpoints, registros y un checklist para diagnosticar periféricos sin cambiar
   cosas al azar.
3. [03 - Hard faults](./03-hard-faults.md)
   Qué ocurre cuando el micro entra en un fault y cómo pasar de "se colgó" a "falló en esta línea por esta causa":
   el marco apilado, `CFSR`, `BFAR` y `addr2line`.
4. [04 - La consola por el cable del debugger (RTT)](./04-consola-por-el-debugger-rtt.md)
   Una consola en RAM leída mediante la sonda: configuración, costos medidos, convivencia con GDB y
   criterios para elegirla frente a la UART.

---

## La idea central

Cada herramienta muestra una parte del sistema y también puede perturbarla. Los mensajes revelan el
flujo que decidiste instrumentar; el debugger muestra estado interno, pero al detener el núcleo
altera los tiempos; el osciloscopio y el analizador lógico observan señales externas.

Para un periférico que no responde, verificá de manera ordenada:

1. alimentación, reset y habilitación;
2. clock y cálculos derivados;
3. función y modo eléctrico de los pines;
4. banderas y habilitación de interrupciones;
5. comunicación segura entre ISR y código principal;
6. conexiones, niveles y temporización fuera del chip.

El capítulo 2 traduce este esquema a los registros del LPC1769 y explica cómo adaptarlo a otro
microcontrolador.

## Se apoya en

Del curso: [módulo 3](../../curso/03_clock_y_power/) (PCONP y PCLKSEL),
[4](../../curso/04_pinsel/) (PINSEL), [7](../../curso/07_interrupciones/) (interrupciones),
[9](../../curso/09_uart/) (UART) y
[0 cap. 12](../../curso/00_lenguaje_c/12-volatile-y-tipos-para-hardware.md) (`volatile`).

De esta unidad: [02 - Protocolos y debug en el chip](../02_protocolos_y_debug_en_el_chip/)
para saber qué está pasando por debajo, y
[05 - Del código al binario](../05_del_codigo_al_binario/) para entender los stack overflow.

## Manual

Depuración por hardware (núcleo Cortex-M3): capítulos
[33](../../manual/ch33_jtag-serial-wire-debug-and-trace.pdf) y
[34](../../manual/ch34_appendix-cortex-m3-user-guide.pdf). El framework de debug de NXP, en
[`_origen/`](./_origen/).

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [05 - Del código al binario](../05_del_codigo_al_binario/) ·
**Siguiente parte:** [07 - LPC1769](../07_lpc1769/)
