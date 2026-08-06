# El mapa de memoria del LPC1769

## La gran idea: todo es memoria

El ARM Cortex-M3 que hay dentro del LPC1769 "ve" un único espacio de direcciones de **32 bits**.
Eso son 2³² = 4.294.967.296 direcciones posibles, de `0x00000000` a `0xFFFFFFFF`. En cada dirección
hay un byte.

Lo interesante: en ese mismo espacio conviven cosas muy distintas, todas direccionables igual:

- la **memoria Flash** donde está tu programa,
- la **RAM** donde están tus variables,
- y los **periféricos** (GPIO, UART, timers, ADC…).

A esto se lo llama **memory-mapped I/O** (entrada/salida mapeada en memoria): los periféricos no se
controlan con instrucciones especiales, sino **leyendo y escribiendo direcciones de memoria**, igual
que una variable. Un "registro de control del UART" es, físicamente, una dirección a la que el
hardware del UART está conectado: si escribís ahí, cambiás el comportamiento del UART; si leés ahí,
te enterás de su estado.

> Por eso, programar el micro a bajo nivel es esencialmente: **"poné este número en esta dirección"**.

## El mapa del LPC1769

El espacio de 4 GB está dividido en regiones. Estas son las que importan en la materia:

| Región | Rango de direcciones | Qué hay |
|--------|----------------------|---------|
| **Flash (código)** | `0x0000_0000` – `0x0007_FFFF` | Tu programa (512 KB). Es donde arranca el micro. |
| **SRAM local (CPU)** | `0x1000_0000` – `0x1000_7FFF` | RAM principal (32 KB). Variables, stack. |
| **Boot ROM** | `0x1FFF_0000` – `0x1FFF_1FFF` | ROM de NXP (8 KB): bootloader y servicios de grabación de la Flash. |
| **SRAM AHB** | `0x2007_C000` – `0x2008_3FFF` | RAM extra (2×16 KB), usable por periféricos/DMA, Ethernet, USB. |
| **Periféricos GPIO (AHB)** | `0x2009_C000` – `0x2009_FFFF` | **GPIO rápido** (FIODIR, FIOSET, FIOCLR, FIOPIN…). |
| **Periféricos APB0** | `0x4000_0000` – `0x4007_FFFF` | WDT, Timers 0/1, UART0/1, PWM, I2C0, SPI, RTC, **PINCON (PINSEL)**, ADC, CAN… |
| **Periféricos APB1** | `0x4008_0000` – `0x400F_FFFF` | SSP0, **DAC**, Timers 2/3, UART2/3, I2C2, **System Control (PCONP, PLL, PCLKSEL)**… |
| **Periféricos AHB** | `0x5000_0000` – `0x501F_FFFF` | Ethernet, **GPDMA**, USB. |
| **Private Peripheral Bus** | `0xE000_0000` – `0xE00F_FFFF` | Lo del núcleo ARM: **NVIC**, **SysTick**, SCB, debug. |

> El mapa completo está en el **Capítulo 2** del manual:
> [`manual/ch02_memory-map.pdf`](../../manual/ch02_memory-map.pdf).

### Cosas que conviene notar

- **Cada periférico tiene un "bloque" de direcciones propio.** Por ejemplo, todo lo del UART0
  empieza en `0x4000_C000`. Dentro de ese bloque, cada registro está en un *offset* fijo
  (UART0 + 0x00 = registro de datos, UART0 + 0x14 = registro de estado, etc.). Vas a ver que esto
  es justo lo que permite describir un periférico con una `struct` (módulo 3).

- **Los buses no son iguales de rápidos.** El GPIO "rápido" está en el bus **AHB** (`0x2009_C000`),
  que es más veloz que el bus **APB** donde viven la mayoría de los periféricos. Por eso sus
  registros se llaman "FIO" (Fast I/O): el nombre viene de la serie LPC23xx, que además tenía un
  GPIO lento en APB; en el LPC1769 quedó solo la versión rápida.

- **Tocar direcciones que no existen no es inocuo.** Acceder a una zona marcada "reserved" en el
  mapa genera una excepción de **Bus Fault** (§2.6 del cap. 2): así se manifiestan los punteros
  descontrolados. También da Bus Fault intentar *ejecutar* código desde un periférico o escribir
  directo a la Flash (la Flash se graba solo vía la Boot ROM). El caso contrario engaña: una
  dirección no documentada *dentro* del bloque de 16 KB de un periférico existente **no** genera
  excepción, y puede terminar accediendo a un registro real repetido ("aliased") en otra posición
  del bloque; el manual avisa que ese aliasing no es una característica soportada (§2.3). Moraleja:
  usá solo direcciones documentadas.

- **El núcleo ARM también se controla por memoria.** El NVIC (interrupciones) y el SysTick están en
  `0xE000_E000`. No son "del LPC" sino del Cortex-M3; por eso su documentación está en el
  **Capítulo 34** (Appendix Cortex-M3), no en los capítulos de NXP.

## ¿Por qué arranca el programa en la Flash?

Cuando el micro se resetea, el Cortex-M3 hace dos cosas leyendo los **primeros 8 bytes** que ve en
la dirección `0x0000_0000`:

1. En `0x0000_0000` lee el **valor inicial del stack pointer** (dónde empieza la pila en RAM).
2. En `0x0000_0004` lee la dirección de la primera instrucción a ejecutar (el **reset handler**).

Por eso tu programa, una vez compilado y linkeado, se graba en la Flash empezando en `0x0`. El
*linker script* (`.ld`) es el que decide qué va en Flash y qué en RAM (esto lo maneja MCUXpresso por
vos, pero ahora sabés qué hay detrás).

> Detalle fino que documenta el manual (§2.4): inmediatamente después de un reset por hardware, en
> la dirección 0 no está tu Flash sino la **Boot ROM**, mapeada ahí de forma temporal. El bootloader
> de NXP corre primero (entre otras cosas verifica si hay código de usuario válido y si se pide
> grabación por ISP) y recién después el mapa queda apuntando a tu programa. Es transparente para
> vos, pero explica cómo el chip puede reprogramarse incluso con la Flash vacía.

## Un mapa mental para el resto del curso

Cada vez que veas un periférico nuevo, en el fondo vas a hacer siempre lo mismo:

1. **Encenderlo / clockearlo** → escribir un bit en `PCONP` y configurar `PCLKSEL` (módulo 3).
2. **Conectar sus pines** → escribir `PINSEL`/`PINMODE` (módulo 4).
3. **Configurar su comportamiento** → escribir sus registros de control.
4. **Usarlo** → leer/escribir sus registros de datos y estado.

Todo "escribir registros" = "poner números en direcciones". Vamos a hacerlo literalmente en la
[siguiente página](./02-acceso-a-registros-desde-c.md).

---

**Módulo:** [Arquitectura y acceso a registros](./README.md) ·
**Siguiente:** [02 - Cómo se accede a un registro desde C](./02-acceso-a-registros-desde-c.md)
