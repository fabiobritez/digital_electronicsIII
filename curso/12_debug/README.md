# Módulo 12: Debug

> **Este módulo se mudó.** Todo lo de depuración está ahora en la unidad
> [**Herramientas**](../../herramientas/), fuera de la secuencia del curso, junto con los
> protocolos de depuración, las sondas y el toolchain.
>
> **Andá a [`herramientas/06_depurar_en_serio/`](../../herramientas/06_depurar_en_serio/).**

Se mudó porque no es contenido de la materia: es contenido sobre **las herramientas** con las
que se trabaja, y se aplica igual a cualquier microcontrolador. Tenerlo aparte evita mezclarlo
con los periféricos del LPC1769, que es lo que sí entra en los parciales.

## Dónde quedó cada cosa

| Lo que buscabas | Ahora está en |
|---|---|
| Imprimir para depurar (LED, UART, `_DBG`) | [06-01 - Imprimir para depurar](../../herramientas/06_depurar_en_serio/01-imprimir-para-depurar.md) |
| El checklist del "no anda" y los breakpoints | [06-02 - El método del "no anda"](../../herramientas/06_depurar_en_serio/02-el-metodo-del-no-anda.md) |
| Hard faults | [06-03 - Hard faults](../../herramientas/06_depurar_en_serio/03-hard-faults.md) |
| La consola por el cable del debugger (RTT) | [06-04 - RTT](../../herramientas/06_depurar_en_serio/04-consola-por-el-debugger-rtt.md) |
| Cómo funciona el debugger por dentro (SWD, JTAG, CoreSight) | [02 - Protocolos y debug en el chip](../../herramientas/02_protocolos_y_debug_en_el_chip/) |
| Qué es una sonda de depuración | [03 - Debug probes](../../herramientas/03_debug_probes/) |
| El framework de debug de NXP | [`_origen/`](../../herramientas/06_depurar_en_serio/_origen/) |

## Lo mínimo que hay que saber, si estás apurado

La mayoría de los "no funciona" de esta materia son **una de seis cosas**:

1. el periférico **sin encender** (`PCONP`, [módulo 3](../03_clock_y_power/)),
2. **sin clock**, o con el `PCLK` mal calculado ([módulo 3](../03_clock_y_power/)),
3. los **pines** mal configurados (`PINSEL`, [módulo 4](../04_pinsel/)),
4. una **bandera de interrupción sin limpiar** ([módulo 7](../07_interrupciones/)),
5. una variable compartida con una ISR **sin `volatile`**
   ([módulo 0, cap. 12](../00_lenguaje_c/12-volatile-y-tipos-para-hardware.md)),
6. una **cuenta de tiempo o de baudrate** mal hecha.

El checklist desarrollado está en
[06-02](../../herramientas/06_depurar_en_serio/02-el-metodo-del-no-anda.md).

---

**Anterior:** [11 - DMA](../11_dma/) · **Siguiente:** [13 - I2C](../13_i2c/) ·
**La unidad completa:** [Herramientas](../../herramientas/)
