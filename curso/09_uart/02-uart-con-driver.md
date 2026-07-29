# UART con el driver CMSIS

El driver `lpc17xx_uart` resuelve lo más tedioso de la página anterior: el **cálculo del divisor de
baudrate** (incluido el fraccional `FDR`, que minimiza el error) y el manejo del `DLAB`. Vos pedís
"115200 8N1" y el driver elige `DLL`/`DLM`/`FDR`, configura `LCR` y enciende el `PCONP`.

## Inicialización con el driver

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void uart0_init(void) {
    // Pines: el driver NO configura PINSEL, lo hacés vos.
    PINSEL_CFG_Type pin;
    pin.Funcnum   = 1;                      // función 1 = TXD0/RXD0
    pin.OpenDrain = 0;
    pin.Pinmode   = 0;                      // pull-up por defecto
    pin.Portnum   = 0;
    pin.Pinnum    = 2; PINSEL_ConfigPin(&pin);   // P0.2 = TXD0
    pin.Pinnum    = 3; PINSEL_ConfigPin(&pin);   // P0.3 = RXD0

    // Configuración de la UART
    UART_CFG_Type cfg;
    UART_ConfigStructInit(&cfg);            // defaults: 9600, 8 bits, sin paridad, 1 stop
    cfg.Baud_rate = 115200;                 // lo cambiamos a 115200
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &cfg);   // PCONP + LCR + DLL/DLM/FDR

    // FIFOs: UART_Init las resetea pero las deja DESHABILITADAS (FCR=0), y el
    // manual exige FIFO enable = 1 para que la UART opere bien. Este paso no es opcional.
    UART_FIFO_CFG_Type fifo;
    UART_FIFOConfigStructInit(&fifo);       // FIFO ON, trigger 1 char, resetea RX y TX
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifo);

    UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);   // habilitar el transmisor (TER)
}
```

`UART_ConfigStructInit` carga los valores típicos (9600, 8N1); después ajustás `Baud_rate`, `Parity`,
`Databits` o `Stopbits` con los enums de `lpc17xx_uart.h` (`UART_PARITY_EVEN`, `UART_DATABIT_7`,
`UART_STOPBIT_2`, etc.). `UART_Init` hace casi todo lo de la página anterior, incluido el barrido del
fraccional para minimizar el error de baudrate (si el mejor error supera el 3%, la función **falla
silenciosamente** y la UART queda mal: por eso conviene conocer tu PCLK). Lo que **no** hace: dejar
las FIFOs habilitadas (de hecho las deshabilita) ni encender el transmisor (deja `TER` en 0). Por eso
el trío `UART_Init` + `UART_FIFOConfig` + `UART_TxCmd` va siempre junto.

## El caste `(LPC_UART_TypeDef *)LPC_UART0`: qué pasa de verdad

Vas a ver ese caste en todos lados y conviene entenderlo bien, porque el material viejo lo explicaba al
revés. En `LPC17xx.h` hay **tres** tipos de UART:

```c
#define LPC_UART0  ((LPC_UART0_TypeDef *) ...)   // tipo propio
#define LPC_UART1  ((LPC_UART1_TypeDef *) ...)   // tipo DISTINTO (modem/RS485)
#define LPC_UART2  ((LPC_UART_TypeDef  *) ...)   // tipo "genérico"
#define LPC_UART3  ((LPC_UART_TypeDef  *) ...)   // tipo "genérico"
```

- `LPC_UART0_TypeDef` y `LPC_UART_TypeDef` tienen **exactamente el mismo layout** (mismos offsets,
  mismos registros, los dos incluyen `ICR`). La única diferencia es el *nombre del tipo*. Por eso el
  caste `(LPC_UART_TypeDef *)LPC_UART0` es **puramente cosmético**: solo calla el warning
  "incompatible pointer type"; el código generado es idéntico. NXP le dio a UART0 un nombre propio por
  consistencia histórica, no porque el hardware difiera.
- `LPC_UART1_TypeDef` **sí es genuinamente distinto**: tiene `MCR`, `MSR`, `RS485CTRL`, `ADRMATCH` y
  `RS485DLY` que las otras no tienen, y **no** tiene `ICR`. Los offsets de varios registros se
  corren. Por eso las funciones de UART1 (modem y RS-485) reciben `LPC_UART1_TypeDef *` y **no** se
  castean a `LPC_UART_TypeDef`. Si forzás ese caste en UART1, leés/escribís en offsets equivocados.

Resumen: para UART0/2/3 el caste a `LPC_UART_TypeDef *` es seguro y necesario solo por el warning;
para UART1 usá el tipo real `LPC_UART1_TypeDef *` (página 3).

## Las funciones que vas a usar

| Función | Equivale a (a registro) |
|---------|-------------------------|
| `UART_Init(UARTx, &cfg)` | PCONP + reset + `LCR` + `DLL`/`DLM`/`FDR` |
| `UART_ConfigStructInit(&cfg)` | rellena `cfg` con 9600 8N1 |
| `UART_TxCmd(UARTx, ENABLE)` | `TER` bit TXEN |
| `UART_SendByte(UARTx, c)` | esperar THRE + escribir `THR` |
| `UART_Send(UARTx, buf, n, flag)` | enviar `n` bytes |
| `UART_ReceiveByte(UARTx)` | leer `RBR` (devuelve el byte) |
| `UART_Receive(UARTx, buf, n, flag)` | recibir hasta `n` bytes |
| `UART_GetLineStatus(UARTx)` | leer `LSR` |
| `UART_GetIntId(UARTx)` | leer `IIR` |
| `UART_FIFOConfig(UARTx, &fifo)` | escribir `FCR` |
| `UART_IntConfig(UARTx, tipo, ENABLE)` | bits de `IER` |
| `UART_CheckBusy(UARTx)` | mira `TEMT` (¿terminó de transmitir?) |
| `UART_ForceBreak(UARTx)` | `LCR` bit BC (manda un break) |

## Enviar y recibir

```c
uint8_t msg[] = "Hola desde el LPC1769\r\n";
UART_Send((LPC_UART_TypeDef *)LPC_UART0, msg, sizeof(msg)-1, BLOCKING);
```

`BLOCKING` espera a transmitir todo (con un time-out de seguridad interno) y devuelve cuántos bytes
mandó; `NONE_BLOCKING` carga lo que entre
en la FIFO (hasta 16) y vuelve enseguida con la cantidad cargada, sin esperar. Para recibir:

```c
uint8_t c;
uint32_t n = UART_Receive((LPC_UART_TypeDef *)LPC_UART0, &c, 1, NONE_BLOCKING);
if (n) { /* llegó un byte en c */ }
```

`NONE_BLOCKING` en recepción es el modo natural para combinar con interrupciones: leés lo que haya y
seguís.

## El trigger de la FIFO: una decisión que importa

`UART_FIFOConfig` configura, entre otras cosas, el **nivel de disparo** de la RX FIFO (`FIFO_Level`):

| Constante | Dispara a |
|-----------|-----------|
| `UART_FIFO_TRGLEV0` | 1 carácter |
| `UART_FIFO_TRGLEV1` | 4 caracteres |
| `UART_FIFO_TRGLEV2` | 8 caracteres |
| `UART_FIFO_TRGLEV3` | 14 caracteres |

El trigger es cuántos bytes deben acumularse en la RX FIFO antes de generar la interrupción "Receive
Data Available". Hay un compromiso:

- **Trigger bajo (1 char)**: interrumpís por cada byte. Latencia mínima, pero muchas interrupciones
  (carga de CPU alta a baudrates altos).
- **Trigger alto (14 chars)**: una sola interrupción por cada 14 bytes. Eficiente, pero si llegan
  menos de 14 y se cortan, nunca llega al trigger... salvo por el **Character Time-out (CTI)**: si
  pasan ~3.5-4.5 tiempos de carácter sin que llegue nada y hay datos en la FIFO sin alcanzar el
  trigger, el hardware interrumpe igual. Por eso un trigger alto no te deja "colgado": el CTI vacía
  la cola. La página 3 detalla el CTI.

Para una consola interactiva (un byte por tecla) usá trigger 1. Para un stream continuo a alta
velocidad, un trigger más alto baja la carga.

## Redirigir `printf` a la UART (muy útil para depurar)

Si tu toolchain lo permite, enganchás la salida estándar a la UART implementando la función de bajo
nivel que usa `printf`:

```c
int __io_putchar(int ch) {           // newlib / arm-none-eabi: a veces es _write()
    UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)ch);
    return ch;
}
// luego: printf("ADC = %d\r\n", valor);  -> sale por la UART
```

Esto convierte la UART en tu consola de depuración (ver [módulo 12](../12_debug/)).

## Errores comunes

| Error | Corrección |
|-------|-----------|
| Baudrate distinto en los dos extremos | mismo baudrate y formato (8N1) en ambos |
| Olvidar `UART_TxCmd(ENABLE)` | habilitar el transmisor antes de enviar |
| No configurar PINSEL de TXD/RXD | `UART_Init` no toca pines; hacelo vos |
| `PCLK` equivocado: `UART_Init` falla en silencio | conocé el PCLK real (módulo 3) |
| Castear `LPC_UART1` a `LPC_UART_TypeDef *` | UART1 usa su propio tipo; no lo castees |
| Falta `\r` (solo `\n`) y el terminal no salta de línea | usar `"\r\n"` |
| Recibir por polling y perder bytes mientras hacés otra cosa | recibir por interrupción (pág. 3) |

## Ejercicios

1. Reescribí la inicialización **a registro** para 115200 baud con PCLK=25 MHz usando el fraccional, y
   verificá el % de error contra lo que elige `UART_Init`.
2. Mandá por UART el valor de un ADC cada 500 ms (combina con módulos 6, 8 y 10).
3. Hacé una función `uart_printf_min` propia (sin `printf` de la libc) que mande un `uint32_t` en
   decimal por la UART.

> Material original con más detalle: [`_origen/06_UART.md`](./_origen/06_UART.md).

---

**Anterior:** [01 - UART a nivel registro](./01-uart-registros.md) ·
**Siguiente:** [03 - UART por interrupción, RS-485 y flow control](./03-uart-interrupcion-y-rs485.md)
