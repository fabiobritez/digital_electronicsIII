# 05 - Del código al binario

Cuando apretás "compilar" se ejecutan varias etapas que suelen quedar ocultas: `main` **no es
lo primero que corre**, las variables globales aparecen inicializadas y el programa conoce la
distribución de la FLASH y la RAM.

Esta parte destapa esa caja. Es la continuación natural del
[módulo 1 del curso](../../curso/01_arquitectura_y_acceso_a_registros/): ahí se vio que el
micro arranca leyendo la FLASH; acá se ve **cómo tu código termina ahí y cómo arranca de
verdad**.

Entender estas piezas permite diagnosticar problemas que el botón de Build no explica. Como
caso de estudio, en el
[módulo 2](../../curso/02_arma_tu_propia_libreria/src/build/) el curso ya construyó un
firmware real (`mygpio`) con su propio `startup.c` y su linker script, compilado y linkeado de
verdad. Esta parte lo explica.

## Recorrido

1. [01 - De código a binario: las secciones](./01-de-codigo-a-binario.md)
   Las etapas del build aplicadas al micro, el `.elf` contra el `.bin`, y las secciones
   `.text` / `.rodata` / `.data` / `.bss`: qué va a FLASH, qué va a RAM, y por qué.
2. [02 - El linker script y el startup](./02-linker-y-startup.md)
   El `.ld` que ubica todo en memoria, y el código de arranque que copia `.data`, pone `.bss`
   en cero y salta a `main`, con el ejemplo real de `mygpio`.
3. [03 - El arranque paso a paso: de 0 V a `main()`](./03-el-arranque-paso-a-paso.md)
   La secuencia completa, sin saltear eslabones: el POR y el brown-out, el oscilador interno
   (y por qué el cristal no participa), los temporizadores de la FLASH, qué decide la boot
   ROM y en qué orden, las dos lecturas que hace el Cortex-M3, y `RSID` para saber por qué se
   reseteó. Con los tiempos del manual y los registros medidos sobre la placa.

## Por qué importa, en concreto

- Entender por qué una variable global mal usada se "pisa" (el mapa de memoria).
- Saber qué es el stack, dónde está, y qué pasa cuando se desborda. Es la base de los
  [hard faults](../06_depurar_en_serio/03-hard-faults.md).
- Reconocer qué debe cambiar al portar: mapa de memoria, startup, bibliotecas y configuración
  del target.
- Dejar de tenerle miedo al `startup_*.c` y al `.ld` que genera el IDE.

## Las versiones completas

Los archivos explicados acá son **mínimos a propósito**. Las versiones completas usadas por
el repositorio están en la
[plantilla](../../plantilla/), comentadas línea por línea:

| Archivo | Qué agrega sobre la versión mínima |
|---------|-----------------------------------|
| [`linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld) | las dos SRAM del bus AHB, heap y stack con verificación de que la RAM alcanza, `.init_array`, `.ARM.exidx` |
| [`startup/startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c) | la tabla de vectores **completa** (35 IRQ del LPC1769), `SystemInit()`, `__libc_init_array()` |
| [`Makefile`](../../plantilla/Makefile) | el build entero: dependencias automáticas, `--gc-sections`, mapa de memoria, grabado y depuración |
| [`tools/lpc_checksum.py`](../../plantilla/tools/lpc_checksum.py) | el checksum del vector 7 que exige la boot ROM |

## Cuándo leerlo

Se puede leer después del
[módulo 1 del curso](../../curso/01_arquitectura_y_acceso_a_registros/) o consultar cuando un
error de link, startup o arranque requiera mirar debajo del IDE.

Y si venís del [panorama](../01_panorama/01-el-mapa-completo.md), esta parte es el detalle de
los pasos 4 y 5 de aquel diagrama.

## Manual

Complementa con el capítulo [34](../../manual/ch34_appendix-cortex-m3-user-guide.pdf)
(Cortex-M3, tabla de vectores y reset), el [2](../../manual/ch02_memory-map.pdf) (mapa de
memoria y boot ROM) y el [32](../../manual/ch32_flash-memory-interface-and-programming.pdf)
(el bootloader: cómo decide si tu código es válido).

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [04 - Toolchains](../04_toolchains/) ·
**Siguiente parte:** [06 - Depurar en serio](../06_depurar_en_serio/)
