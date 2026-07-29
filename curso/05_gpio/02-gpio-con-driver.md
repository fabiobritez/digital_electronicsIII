# GPIO con el driver CMSIS

El driver `lpc17xx_gpio` envuelve los registros FIO en funciones. Como con PINSEL, **no hace nada
nuevo**: adentro escribe `FIODIR`, `FIOSET`, `FIOCLR`, `FIOPIN` por vos. Lo valioso es que el código
queda legible y que ves la correspondencia uno a uno con la página anterior.

## Las funciones principales (estilo GPIO)

```c
#include "lpc17xx_gpio.h"

// Dirección: dir = 1 -> salida, 0 -> entrada. 'bitValue' es una máscara de pines.
void GPIO_SetDir(uint8_t portNum, uint32_t bitValue, uint8_t dir);

// Poner pines en alto (equivale a FIOxSET = bitValue)
void GPIO_SetValue(uint8_t portNum, uint32_t bitValue);

// Poner pines en bajo (equivale a FIOxCLR = bitValue)
void GPIO_ClearValue(uint8_t portNum, uint32_t bitValue);

// Leer el puerto entero (equivale a leer FIOxPIN)
uint32_t GPIO_ReadValue(uint8_t portNum);
```

La correspondencia es directa (lo podés confirmar abriendo `lpc17xx_gpio.c`):

| Driver | Registro que toca |
|--------|-------------------|
| `GPIO_SetDir(0, m, 1)` | `LPC_GPIO0->FIODIR \|= m` |
| `GPIO_SetDir(0, m, 0)` | `LPC_GPIO0->FIODIR &= ~m` |
| `GPIO_SetValue(0, m)` | `LPC_GPIO0->FIOSET = m` |
| `GPIO_ClearValue(0, m)` | `LPC_GPIO0->FIOCLR = m` |
| `GPIO_ReadValue(0)` | `LPC_GPIO0->FIOPIN` |

`portNum` va de 0 a 4; `bitValue` es **una máscara de pines** (podés tocar varios a la vez con un OR de
`#define`s), no un número de pin. O sea `GPIO_SetValue(0, (1<<22)|(1<<21))` prende dos LEDs de una.

> Como `GPIO_SetValue`/`GPIO_ClearValue` van a `FIOSET`/`FIOCLR`, **heredan la atomicidad** que vimos en
> la página 01: son seguras frente a interrupciones. `GPIO_ReadValue` no hace read-modify-write, solo
> lee. Ojo: el driver **no toca** `FIOMASK` (solo `FIO_SetMask` lo hace); como tras reset vale 0, estas
> funciones tocan el puerto completo tal cual esperás, salvo que vos hayas dejado una máscara puesta.

## El mismo ejemplo (LED que sigue al botón), con driver

```c
#include "lpc17xx_gpio.h"

#define LED    (1u << 22)   // P0.22
#define BOTON  (1u << 10)   // P2.10

int main(void) {
    GPIO_SetDir(0, LED, 1);    // P0.22 salida
    GPIO_SetDir(2, BOTON, 0);  // P2.10 entrada

    while (1) {
        if (GPIO_ReadValue(2) & BOTON) {
            GPIO_ClearValue(0, LED);   // botón suelto -> LED apagado
        } else {
            GPIO_SetValue(0, LED);     // botón apretado -> LED prendido
        }
    }
}
```

Compará línea por línea con el ejemplo a registro de la página anterior: es **la misma lógica**, solo
cambian los nombres. Si entendés uno, entendés el otro: esa es la idea de todo el curso.

## Las otras familias del driver: FIO_ y los accesos parciales

El header `lpc17xx_gpio.h` declara más que estas cuatro funciones. Conviene saber que existen:

- **Estilo `FIO_`** (`FIO_SetDir`, `FIO_SetValue`, `FIO_ClearValue`, `FIO_ReadValue`): son **idénticas**
  a las `GPIO_*` (de hecho `FIO_SetDir` llama a `GPIO_SetDir`). Es solo otro nombre por compatibilidad.
- **`FIO_SetMask(portNum, bitValue, maskValue)`:** la forma del driver de tocar `FIOMASK`. Con
  `maskValue = 1` protege los pines de `bitValue`; con `0` los habilita. Es lo que usás para el truco de
  bus que se explica en la [página 04](./04-fiomask-y-acceso-por-byte.md).
- **Acceso por half-word y por byte:** `FIO_HalfWordSetValue`, `FIO_ByteSetValue`, etc. permiten tocar
  16 u 8 pines de un puerto sin máscara. También en la [página 04](./04-fiomask-y-acceso-por-byte.md).
- **Interrupción por GPIO:** `GPIO_IntCmd`, `GPIO_GetIntStatus`, `GPIO_ClearInt`. Estas **no** tocan los
  registros FIO sino el bloque aparte `GPIOINT` (puertos 0 y 2). Las dejamos para el
  [módulo 7](../07_interrupciones/).

## Blink con base de tiempo (anticipo de SysTick)

Un parpadeo "de verdad" no usa un `for` vacío para la demora (depende del compilador y la frecuencia).
Se usa un timer. Como anticipo, así se vería con una demora por SysTick (módulo 6):

```c
#include "lpc17xx_gpio.h"
extern void delay_ms(uint32_t ms);   // la implementás con SysTick en el módulo 6

#define LED (1u << 22)

int main(void) {
    GPIO_SetDir(0, LED, 1);
    while (1) {
        GPIO_SetValue(0, LED);
        delay_ms(500);
        GPIO_ClearValue(0, LED);
        delay_ms(500);
    }
}
```

## Buenas prácticas

- **Definí máscaras con nombre** (`#define LED (1u<<22)`) en vez de números mágicos repartidos por el
  código.
- **Una función de init por placa:** juntá todos los `GPIO_SetDir` (y PINSEL/PINMODE) en una
  `board_init()`, así el `main` queda limpio.
- **SET/CLR para salidas, ReadValue para entradas.** Evitá leer-modificar-escribir `FIOPIN` salvo que
  uses `FIOMASK` a propósito.
- **Driver para legibilidad, registro para entender y para el camino crítico.** En un lazo que conmuta
  un pin millones de veces por segundo, el acceso directo a `FIOSET`/`FIOCLR` evita el costo de la
  llamada a función.

## Ejercicios

1. **LED RGB:** controlá 3 LEDs (P0.22, P0.21, P0.20) para mostrar 8 colores en secuencia.
2. **Contador binario:** mostrá un contador 0–255 en 8 LEDs (P2.0–P2.7) usando `FIO_SetMask` +
   `GPIO_SetValue` o el acceso por byte (`FIO_ByteSetValue`); ver [página 04](./04-fiomask-y-acceso-por-byte.md).
3. **Antirrebote:** el ejemplo del botón "rebota". Resolvelo con lo de la
   [página 03](./03-debounce-y-filtrado-de-entradas.md).
4. Reescribí el ejercicio 1 **a registro** (sin driver) y compará el tamaño del binario.

---

**Anterior:** [01 - GPIO a nivel registro](./01-gpio-registros.md) ·
**Siguiente:** [03 - Debounce y filtrado de entradas](./03-debounce-y-filtrado-de-entradas.md)
