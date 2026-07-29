# La función de los pines: PINSEL, PINMODE, PINMODE_OD

En el LPC1769 la mayoría de los pines pueden cumplir más de una función. Por ejemplo, un mismo pin
puede ser:

- entrada/salida digital (GPIO),
- entrada de un periférico (UART, I²C, SPI, ADC…),
- salida de un periférico (PWM, DAC…).

Para decidir qué función cumple cada pin hay **multiplexores internos**, controlados por tres
familias de registros, todas dentro del bloque **PINCON** (base `0x4002C000`):

| Registro | Decide… |
|----------|---------|
| **PINSEL** | la **función** del pin (GPIO, UART, ADC…) |
| **PINMODE** | el modo de la entrada: pull-up / pull-down / repeater / nada |
| **PINMODE_OD** | si la salida es **open-drain** o push-pull |

## PINSEL: elegir la función

Cada pin tiene **2 bits** en algún registro PINSEL. Como 2 bits dan 4 combinaciones, cada pin tiene
hasta 4 funciones:

| Bits | Función |
|------|---------|
| `00` | función primaria (GPIO por defecto) |
| `01` | primera función alternativa |
| `10` | segunda función alternativa |
| `11` | tercera función alternativa |

Como cada pin usa 2 bits y un registro tiene 32, **cada registro PINSEL controla 16 pines**. Por eso
hay varios: `PINSEL0` controla P0.0–P0.15, `PINSEL1` controla P0.16–P0.31, `PINSEL2` el puerto 1,
etc.

| Registro | Dirección | Pines |
|----------|-----------|-------|
| `PINSEL0` | `0x4002C000` | P0.0 – P0.15 |
| `PINSEL1` | `0x4002C004` | P0.16 – P0.31 |
| `PINSEL2` | `0x4002C008` | P1.0 – P1.15 |
| … | … | … |

> El valor **tras el reset es `00`** para todos → todos los pines arrancan como GPIO. Por eso en el
> módulo 1 pudimos usar GPIO sin tocar PINSEL.

> Hay un detalle de numeración que despista: los PINSEL **no son consecutivos en su uso**. Existen
> `PINSEL0`…`PINSEL10` en la struct, pero `PINSEL5` (P2 alto), `PINSEL6` (P3 bajo) y `PINSEL8`
> (P4 bajo) están *reservados* porque esos pines no existen físicamente en el LPC1769. Y `PINSEL10` no controla
> ningún puerto: solo su **bit 3** habilita el puerto de traza (debug). El mapa completo, con la
> fórmula para sacar registro y corrimiento de cualquier pin, está en la
> [página 03](./03-mapa-de-registros.md).

**Ejemplo:** el pin **P0.0** tiene estas funciones (Capítulo 7 del manual):

| Bits PINSEL0[1:0] | Función de P0.0 |
|-------------------|-----------------|
| `00` | GPIO P0.0 |
| `01` | RD1 (CAN) |
| `10` | TXD3 (UART3 TX) |
| `11` | SDA1 (I²C1) |

> **Siempre** verificá en las tablas del Capítulo 7
> ([`manual/ch07...`](../../manual/ch07_pin-configuration.pdf)) qué función corresponde a cada pin.
> No te las aprendas de memoria: se consultan.

Dos detalles del capítulo 8 que conviene conocer:

- **La entrada GPIO nunca se desconecta.** Aunque el pin esté en otra función (UART, ADC…), la
  entrada digital GPIO sigue conectada: podés leer el estado del pin por software o usarlo para la
  interrupción de GPIO. Útil para depurar ("¿está moviéndose la línea TXD?").
- **Una misma función puede mapearse a más de un pin.** Si es una *salida*, aparece en todos los
  pines que la seleccionen. Si es una *entrada*, el periférico la toma del pin de **menor puerto y
  menor número**; los demás quedan de adorno. No dependas de esto: elegí un solo pin por función.

### Cómo escribir PINSEL a mano

Patrón "limpiar y poner" (igual que en clock/power). Para poner P0.0 en función TXD3 (`10`):

```c
#include <LPC17xx.h>

LPC_PINCON->PINSEL0 &= ~(0x3u << 0);   // limpiar los bits 1:0 de P0.0
LPC_PINCON->PINSEL0 |=  (0x2u << 0);   // escribir 10 = TXD3
```

Para un pin más arriba, el corrimiento cambia. P0.5 usa los bits `11:10` de PINSEL0
(`2 × número_de_pin_dentro_del_registro`):

```c
LPC_PINCON->PINSEL0 &= ~(0x3u << 10);  // bits de P0.5
LPC_PINCON->PINSEL0 |=  (0x1u << 10);  // función 01 de P0.5
```

> **Tip:** La cuenta del corrimiento es `2 × (pin % 16)`, y el registro es `PINSEL(2*puerto + pin/16)`.
> Equivocarse en este cálculo es uno de los errores más comunes; por eso conviene el driver (pág. 2).

## PINMODE: resistencias de entrada

Una vez elegida la función, `PINMODE` controla cómo se comporta el pin como **entrada**, con sus
resistencias internas. También 2 bits por pin:

| Bits | Modo |
|------|------|
| `00` | pull-up habilitado (por defecto) |
| `01` | repeater |
| `10` | sin pull-up ni pull-down (tri-state / alta impedancia) |
| `11` | pull-down habilitado |

- **Pull-up** (`00`): para botones que se conectan a GND (el pin queda en 1 hasta que se aprieta).
- **Pull-down** (`11`): para entradas que se conectan a VCC.
- **Tri-state** (`10`): para señales analógicas (ADC): no querés resistencias que alteren la
  medición.
- **Repeater** (`01`): mantiene el último estado para que el pin no flote; útil en bajo consumo.

> Ojo con un detalle que confunde: el orden de los valores **no** es "creciente". `00` es pull-up,
> `11` es pull-down, y el "ni una ni otra" (tri-state) es el `10`, no el `00`. Es el opuesto de la
> intuición de muchos micros. El modo repeater (`01`) y el porqué de cada modo lo desarrollamos a
> fondo en la [página 04](./04-pinmode-opendrain-tolerancia.md).

## PINMODE_OD: salida open-drain

`PINMODE_OD` (1 bit por pin) elige cómo es la **salida**:

| Bit | Salida |
|-----|--------|
| `0` | normal (push-pull): puede forzar 0 **y** 1 |
| `1` | open-drain: solo puede tirar a 0; el 1 lo da una resistencia externa |

El modo open-drain es **obligatorio para I²C** (bus de un cable compartido) y útil para "wired-AND" y
niveles mixtos. `PINMODE_OD0` está en `0x4002C068`. Hay un caso especial: los pines de I²C0
(P0.27/P0.28) son open-drain "de verdad" por hardware y no se tocan acá sino con `I2CPADCFG`. Todo
esto, más qué pines toleran 5 V y cuáles se dañan, en la
[página 04](./04-pinmode-opendrain-tolerancia.md).

## Ejemplo completo a registro: P0.0 como TXD3, sin resistencias, push-pull

```c
#include <LPC17xx.h>

int main(void) {
    // 1) Función: TXD3 (10 en PINSEL0 bits 1:0)
    LPC_PINCON->PINSEL0    &= ~(0x3u << 0);
    LPC_PINCON->PINSEL0    |=  (0x2u << 0);

    // 2) Sin pull-up ni pull-down (10 en PINMODE0 bits 1:0)
    LPC_PINCON->PINMODE0   &= ~(0x3u << 0);
    LPC_PINCON->PINMODE0   |=  (0x2u << 0);

    // 3) Push-pull, no open-drain (bit 0 = 0)
    LPC_PINCON->PINMODE_OD0 &= ~(1u << 0);

    while (1) { /* UART3 se configura aparte (módulo 9) */ }
}
```

Acá ya estamos usando `LPC_PINCON->PINSEL0`, la struct de CMSIS del módulo 1. Eso ya es bastante
cómodo. Pero calcular a mano "qué registro y qué corrimiento" para cada pin es tedioso y propenso a
errores. En la [próxima página](./02-configurar-pines-con-cmsis.md) vemos cómo el **driver PINSEL**
te deja describir el pin con una struct y se ocupa de los cálculos.

---

**Módulo:** [PINSEL](./README.md) ·
**Siguiente:** [02 - Configurar pines con CMSIS](./02-configurar-pines-con-cmsis.md)
