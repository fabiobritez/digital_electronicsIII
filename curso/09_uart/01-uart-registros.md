# UART a nivel registro

La **UART** (Universal Asynchronous Receiver/Transmitter) manda y recibe bytes en serie por dos
cables: **TXD** (transmisión) y **RXD** (recepción). "Asíncrona" significa que no hay un cable de reloj
compartido: emisor y receptor no comparten clock, así que tienen que ponerse de acuerdo de antemano en
la velocidad (**baudrate**) y en el formato. El formato típico es **8N1**: 8 bits de datos, sin
paridad (N), 1 bit de stop.

## Cómo es un frame en la línea

Entender el frame ayuda a entender por qué existen los registros que vienen después. La línea en
reposo (idle) está en alto. Un carácter se transmite así:

```
  idle    START   D0  D1  D2  D3  D4  D5  D6  D7   [PAR]  STOP   idle
 ───────┐      ┌───┬───┬───┬───┬───┬───┬───┬───┬──────┬──────┌──────
        │      │                                            │
        └──────┘  (bit de arranque, siempre 0)              (vuelve a alto)
```

- El **bit de start** (un 0) avisa al receptor que arranca un carácter; por su flanco de bajada el
  receptor sincroniza su muestreo.
- Siguen los **bits de datos**, del menos significativo (D0) al más significativo. La UART puede
  mandar 5, 6, 7 u 8 bits por carácter.
- Opcionalmente un **bit de paridad** (par, impar, o fijo) para detección de errores.
- Uno o dos **bits de stop** (en alto) que cierran el carácter.

El receptor muestrea cada bit aproximadamente en el centro de su ventana temporal. Por eso el
periférico corre internamente a **16× el baudrate**: usa esos 16 "ticks" por bit para ubicar el centro
y para descartar ruido. Ese factor 16 es el que vas a ver en la fórmula del baudrate.

Para que dos dispositivos se entiendan, ambos deben usar **el mismo baudrate** y **el mismo formato**.
Si el baudrate no coincide, el receptor muestrea en momentos equivocados y recibe basura (y muy
probablemente *framing errors*). La tolerancia práctica es de alrededor de ±2% acumulado entre los dos
extremos; por eso después vamos a calcular el % de error del divisor.

Usamos `LPC_UART0` de ejemplo. Sus pines son **TXD0 = P0.2** y **RXD0 = P0.3** (función 1 del PINSEL).
El manual advierte que los pines de RX **no deben tener pull-down habilitado** (`PINMODE`): la línea en
reposo es un 1, y un pull-down te puede meter basura o un falso start.

## El mapa de registros (y la trampa del DLAB)

El bloque UART del LPC1769 hereda el diseño del viejo 16550, y eso explica su rareza principal:
**varios registros comparten la misma dirección** y se distinguen por dos cosas: si la operación es de
lectura o de escritura, y el valor del bit **DLAB** (Divisor Latch Access Bit) del registro `LCR`.

| Offset | DLAB | Lectura | Escritura |
|--------|------|---------|-----------|
| 0x00 | 0 | `RBR` (byte recibido) | `THR` (byte a transmitir) |
| 0x00 | 1 | `DLL` (divisor, byte bajo) | `DLL` |
| 0x04 | 0 | `IER` (habilitar interrupciones) | `IER` |
| 0x04 | 1 | `DLM` (divisor, byte alto) | `DLM` |
| 0x08 | - | `IIR` (identif. de interrupción) | `FCR` (control de FIFO) |
| 0x0C | - | `LCR` (formato + DLAB) | `LCR` |
| 0x14 | - | `LSR` (estado de la línea) | - |
| 0x1C | - | `SCR` (scratch pad) | `SCR` |
| 0x20 | - | `ACR` (auto-baud) | `ACR` |
| 0x24 | - | `ICR` (IrDA; UART0/2/3, no UART1) | `ICR` |
| 0x28 | - | `FDR` (divisor fraccional) | `FDR` |
| 0x30 | - | `TER` (transmit enable) | `TER` |
| 0x58 | - | `FIFOLVL` (nivel de FIFOs; figura en el header CMSIS, no en el mapa del manual) | - |

Tres pares pisan la misma dirección:

- **0x00**: leés `RBR`, escribís `THR`. Con DLAB=1 ese mismo offset es `DLL`.
- **0x04**: `IER` con DLAB=0, `DLM` con DLAB=1.
- **0x08**: leés `IIR`, escribís `FCR`. Son registros distintos en la misma dirección, uno de solo
  lectura y otro de solo escritura.

Por eso el header de CMSIS define el periférico con `union`s en vez de campos normales. Mirá
`LPC_UART0_TypeDef` en `LPC17xx.h`: `RBR/THR/DLL` comparten un `union`, y `DLM/IER` comparten otro.
Vos accedés con el nombre que corresponde al modo en que dejaste el DLAB.

**La trampa clásica**: si te olvidás de poner DLAB=0 después de cargar los divisores, cuando creas que
estás escribiendo en `THR` en realidad estás escribiendo en `DLL`, y no transmite nada (o transmite a
un baudrate que vos cambiaste sin querer). Regla de oro: **DLAB=1 solo el instante en que cargás
`DLL`/`DLM`, y lo bajás enseguida.**

## Los registros uno por uno

### LCR: Line Control Register (formato del frame)

Define cómo es cada carácter. Los nombres de bits salen de `lpc17xx_uart.h`:

| Bits | Nombre | Función |
|------|--------|---------|
| 1:0 | WLS (Word Length Select) | `00`=5, `01`=6, `10`=7, `11`=8 bits de datos |
| 2 | SBS (Stop Bit Select) | `0`=1 stop, `1`=2 stops (1.5 si son 5 bits) |
| 3 | PE (Parity Enable) | habilita la paridad |
| 5:4 | PS (Parity Select) | `00`=impar, `01`=par, `10`=fija "1", `11`=fija "0" |
| 6 | BC (Break Control) | fuerza TXD a 0 (manda un *break*) mientras esté en 1 |
| 7 | **DLAB** | abre el acceso a los divisores (`DLL`/`DLM`) |

Para 8N1 escribís `0x03` (WLS=8, sin paridad, 1 stop, DLAB=0). Ojo con la paridad: PE **y** PS son
dos cosas separadas; "par" es PE=1 y PS=01.

### DLL / DLM y FDR: el generador de baudrate

`DLL` y `DLM` (accesibles con DLAB=1) forman un divisor de 16 bits: `DL = 256×DLM + DLL`. `FDR`
(Fractional Divider Register) agrega un ajuste fraccional con dos campos: `DIVADDVAL` (bits 3:0) y
`MULVAL` (bits 7:4). Lo detallamos en la sección de cálculo.

### IER: Interrupt Enable Register (DLAB=0)

Cada bit habilita una fuente de interrupción. Los vemos en serio en la
[página 3](./03-uart-interrupcion-y-rs485.md); por ahora alcanza con saber que existen:

| Bit | Nombre | Habilita |
|-----|--------|----------|
| 0 | RBRIE | hay dato recibido (Receive Data Available) |
| 1 | THREIE | el `THR` quedó vacío (espacio para transmitir) |
| 2 | RLSIE | error de línea (Receive Line Status) |

### IIR / FCR: identificación de interrupción y control de FIFO

`IIR` (lectura) te dice **por qué** interrumpió, y `FCR` (escritura, misma dirección) controla las
FIFOs. También en la página 3.

### LSR: Line Status Register (el termómetro de la línea)

Es el registro que más vas a leer en polling. Todos sus flags:

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 0 | RDR | Receive Data Ready: hay al menos un byte en la RX FIFO listo para leer en `RBR` |
| 1 | OE | Overrun Error: llegó un byte nuevo y la FIFO ya estaba llena; **se perdió un dato** |
| 2 | PE | Parity Error: el bit de paridad del carácter del frente de la FIFO no cierra |
| 3 | FE | Framing Error: el bit de stop no estaba en alto (típico de baudrate mal ajustado) |
| 4 | BI | Break Interrupt: la línea estuvo en 0 todo un frame completo |
| 5 | THRE | Transmit Holding Register Empty: se puede cargar otro byte a transmitir |
| 6 | TEMT | Transmitter Empty: `THR` **y** el shift register están vacíos (terminó de verdad) |
| 7 | RXFE | RX FIFO Error: hay al menos un carácter con PE/FE/BI en la FIFO |

Dos sutilezas importantes:

- **THRE vs TEMT**: `THRE` se levanta apenas el byte pasa del holding register al shift register; en
  ese momento ya podés cargar el siguiente, pero el carácter anterior **todavía se está transmitiendo
  en la línea**. `TEMT` solo se levanta cuando no queda nada por mandar. Si vas a apagar la UART o
  cambiar de dirección en RS-485, esperá `TEMT`, no `THRE`.
- **PE/FE/BI acompañan al carácter del frente de la FIFO**: describen al byte que está por salir de
  `RBR`. Por eso el orden correcto es leer `LSR` y *después* leer `RBR`, para no atribuir un error al
  byte equivocado. `OE`, en cambio, no está asociado a ningún carácter: se levanta apenas ocurre el
  overrun. Los cuatro flags de error (OE/PE/FE/BI) se limpian al leer `LSR`.

### SCR, TER, ACR, ICR

- `SCR` (Scratch Pad): un byte de RAM sin función fija. Te lo regala el hardware para guardar lo que
  quieras (un flag, un contador). No afecta a la UART.
- `TER` (Transmit Enable Register): el bit 7 (`TXEN`) habilita el transmisor. Arranca en 1, pero si
  alguien lo bajó (o el driver lo resetea durante el init), no sale nada aunque cargues `THR`. Su
  propósito de diseño es el **control de flujo por software**: si bajás `TXEN`, la UART termina de
  mandar el carácter en curso y se detiene; al volver a subirlo retoma. Es la forma cruda de frenar
  la transmisión sin perder datos (el equivalente "a mano" del auto-CTS de UART1).
- `ACR` (Auto-baud Control): mide el baudrate de un carácter entrante (el protocolo del manual se basa
  en el "AT" de los módems: espera una `'A'` o `'a'`) y ajusta `DLL`/`DLM` solo. Útil cuando no sabés
  a qué velocidad te van a hablar. Lo mencionamos en la página 3.
- `ICR` (IrDA Control): en UART0/2/3 (UART1 no lo tiene), para infrarrojo. No lo usamos.

## Cálculo del baudrate (el corazón del asunto)

La fórmula completa, con divisor fraccional incluido, es:

```
                              PCLK_uart
baudrate = ─────────────────────────────────────────────────
            16 × (256×DLM + DLL) × (1 + DIVADDVAL/MULVAL)
```

Con las restricciones de hardware (UM10360, sección del FDR): `1 ≤ MULVAL ≤ 15`,
`0 ≤ DIVADDVAL ≤ 14` y `DIVADDVAL < MULVAL`. Hay una restricción extra que el manual marca como
importante: si el fraccional está activo (`DIVADDVAL > 0`) y `DLM = 0`, entonces `DLL` debe ser
**mayor que 2**. Notá que `DIVADDVAL = 0` **desactiva** el fraccional
(la línea queda en `(1 + 0/MULVAL) = 1`). Si querés
desactivar el fraccional, ponés `FDR = 0x10` (DIVADDVAL=0, MULVAL=1), que hace `(1 + 0/1) = 1` y la
fórmula colapsa a la versión simple:

```
baudrate = PCLK_uart / (16 × DL)        con DL = 256×DLM + DLL
```

**Lo primero que tenés que saber es el `PCLK_uart`** (módulo 3). Por reset el periférico corre a
`CCLK/4`. Si `CCLK = 100 MHz`, entonces `PCLK_uart = 25 MHz`. Si calculás el divisor con el PCLK
equivocado, el baudrate sale mal y recibís basura: este es uno de los bugs más comunes de UART.

### Ejemplo 1: 9600 baud con PCLK = 25 MHz, sin fraccional

```
DL = PCLK / (16 × baudrate) = 25e6 / (16 × 9600) = 162.76  ≈ 163
```

→ `DLM = 0`, `DLL = 163`, `FDR = 0x10`. Baudrate real:

```
25e6 / (16 × 163) = 9585.9 baud      error = (9600−9585.9)/9600 = 0.15%
```

0.15% está holgadamente dentro de tolerancia. Para 9600 el fraccional no hace falta.

### Ejemplo 2: 115200 baud con PCLK = 25 MHz

Sin fraccional:

```
DL = 25e6 / (16 × 115200) = 13.57  ≈ 14
25e6 / (16 × 14) = 111607 baud
error = (115200−111607)/115200 = 3.1%   → DEMASIADO, no engancha
```

Con `DL = 13`: `25e6/(16×13) = 120192` baud, error +4.3%. Ninguno de los dos enteros sirve: con 3-4%
ya empezás a ver *framing errors*. **Acá es donde el divisor fraccional salva el día.** Buscamos `DL`,
`MULVAL` y `DIVADDVAL` que minimicen el error. La mejor combinación es:

```
DL = 10,  MULVAL = 14,  DIVADDVAL = 5
baudrate = 25e6 / (16 × 10 × (1 + 5/14)) = 25e6 / (16 × 10 × 19/14)
         = 25e6 / 217.14 = 115131.6 baud
error = (115131.6 − 115200) / 115200 = −0.06%
```

Pasamos de 3-4% (inusable) a 0.06% (perfecto) solo activando el fraccional. **Este es exactamente el
trabajo que hace `UART_Init` del driver por vos**: barre todas las combinaciones de `MULVAL` (1..15) y
`DIVADDVAL` (0..15) buscando la de menor error, y si el mejor error queda por debajo del 3% configura
los registros; si no, falla. Lo vas a ver en la [página 2](./02-uart-con-driver.md).

> Intuición sobre por qué a 115200 cuesta: el divisor entero es chiquito (13-14), entonces cada unidad
> de `DL` mueve el baudrate un montón (pasos gruesos). A 9600 el divisor es grande (163), los pasos son
> finos y caés cerca del valor exacto sin esfuerzo. **Cuanto más alto el baudrate, más necesitás el
> fraccional.** Otra salida es elegir un `PCLK` que divida exacto: con `PCLK = 100 MHz` (CCLK/1),
> `DL = 100e6/(16×115200) = 54.25`... tampoco entero. Algunos diseños eligen un cristal "raro" (p. ej.
> para que dé múltiplos de 1.8432 MHz) justamente para que los baudrates estándar salgan sin error.

## Inicialización a registro (8N1, 9600 baud, polling)

```c
#include <LPC17xx.h>

void uart0_init(void) {
    // 1) Encender UART0 (PCONP bit 3). Por reset ya viene encendida, pero
    //    hacerlo explícito es buena costumbre.
    LPC_SC->PCONP |= (1u << 3);

    // 2) (Clock) Dejamos PCLK_uart0 en CCLK/4 (valor por reset). Si lo
    //    cambiaste en el módulo 3, ajustá el cálculo del divisor.

    // 3) PINSEL: P0.2 = TXD0, P0.3 = RXD0 (función 1)
    LPC_PINCON->PINSEL0 &= ~((0x3u << 4) | (0x3u << 6));
    LPC_PINCON->PINSEL0 |=  ((0x1u << 4) | (0x1u << 6));

    // 4) Formato 8N1 y abrir acceso a los divisores (DLAB=1)
    LPC_UART0->LCR = (0x3u << 0)    // WLS = 8 bits
                   | (1u << 7);     // DLAB = 1

    // 5) Baudrate 9600 con PCLK = 25 MHz -> DL = 163, sin fraccional
    LPC_UART0->DLM = 0;
    LPC_UART0->DLL = 163;
    LPC_UART0->FDR = (1u << 4);     // MULVAL=1, DIVADDVAL=0  -> fraccional neutro (0x10)

    // 6) Cerrar el acceso a divisores (DLAB=0) para poder usar RBR/THR
    LPC_UART0->LCR = (0x3u << 0);   // 8N1, DLAB=0  <-- ¡NO te olvides de este paso!

    // 7) Habilitar y limpiar las FIFOs (bit0=enable, bit1=reset RX, bit2=reset TX)
    LPC_UART0->FCR = (1u << 0) | (1u << 1) | (1u << 2);

    // 8) Habilitar el transmisor (TER bit 7). Viene en 1 por reset, pero
    //    lo dejamos explícito.
    LPC_UART0->TER = (1u << 7);
}
```

## Transmitir y recibir por polling

Transmitir: esperar a que `THR` esté libre (LSR bit 5, THRE) y escribir el byte. Con la FIFO de TX
activa, en realidad podés cargar hasta 16 bytes seguidos mientras `THRE` esté en 1, porque el byte cae
en la FIFO, no directamente en la línea.

```c
void uart0_send_byte(uint8_t c) {
    while (!(LPC_UART0->LSR & (1u << 5))) { }   // esperar THRE
    LPC_UART0->THR = c;
}

void uart0_send_string(const char *s) {
    while (*s) uart0_send_byte((uint8_t)*s++);
}
```

Recibir: esperar a que haya dato (LSR bit 0, RDR) y leer `RBR`. Leer `RBR` saca el byte de la FIFO y,
si era el último, baja RDR.

```c
uint8_t uart0_read_byte(void) {
    while (!(LPC_UART0->LSR & (1u << 0))) { }   // esperar RDR
    return LPC_UART0->RBR;
}
```

### Recepción con chequeo de errores

En recepción seria conviene mirar los flags de error **antes** de leer el byte, porque PE/FE/BI
describen al carácter que está por salir de `RBR`:

```c
// Devuelve 0 si el byte es válido; !=0 (los flags de error) si hubo problema.
int uart0_read_checked(uint8_t *out) {
    uint8_t s = LPC_UART0->LSR;
    if (!(s & (1u << 0))) return -1;                  // no hay dato
    uint8_t err = s & ((1u<<1)|(1u<<2)|(1u<<3)|(1u<<4)); // OE|PE|FE|BI
    *out = LPC_UART0->RBR;                              // leer SIEMPRE para vaciar la FIFO
    return err;                                         // 0 = ok
}
```

Notá que leemos `RBR` aunque haya error: si no lo hacés, el byte malo se queda y bloquea la FIFO.

### Programa completo: eco serial

```c
int main(void) {
    uart0_init();
    uart0_send_string("UART lista. Escribi algo:\r\n");
    while (1) {
        uint8_t c = uart0_read_byte();   // bloquea hasta que llegue un byte
        uart0_send_byte(c);              // lo devuelve (eco)
    }
}
```

El problema del polling: `uart0_read_byte` **bloquea** el programa esperando. Si el micro tiene que
hacer otra cosa mientras espera datos, conviene recibir **por interrupción**, que vemos en la
[página 3](./03-uart-interrupcion-y-rs485.md). Y para no tener que recordar todos estos pasos a mano,
está el **driver CMSIS** ([página 2](./02-uart-con-driver.md)).

## Casos de borde y errores típicos

| Síntoma | Causa probable |
|---------|----------------|
| No transmite nada | te olvidaste de bajar **DLAB** (escribís en `DLL` creyendo que es `THR`) |
| No transmite nada | `TER` bit 7 (TXEN) en 0 |
| Recibís basura | baudrate mal: PCLK equivocado, o el otro extremo a otra velocidad |
| Framing errors constantes | error de baudrate > ~2-3% (divisor mal elegido sin fraccional) |
| Se pierden bytes | overrun (OE): no leíste `RBR` a tiempo, o trigger de FIFO mal puesto |
| La FIFO se "tranca" | leíste `LSR` pero nunca `RBR` ante un error; el byte malo no se va |
| Cortás transmisión a mitad | esperaste `THRE` en vez de `TEMT` antes de apagar/cambiar dirección |

---

**Módulo:** [UART](./README.md) ·
**Siguiente:** [02 - UART con el driver](./02-uart-con-driver.md)
