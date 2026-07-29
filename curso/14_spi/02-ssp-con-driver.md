# SSP a registro y con el driver CMSIS

El **SSP** (Synchronous Serial Port) es el SPI "bueno" del LPC1769: tiene dos instancias (`LPC_SSP0`
y `LPC_SSP1`), FIFOs de 8 niveles en cada dirección, soporte DMA y tres formatos de trama. Esta
página lo baja a registro (para que entiendas qué configura el driver) y después muestra el driver
CMSIS, que es lo que usás en la práctica. Las interrupciones y el DMA tienen su propia página:
[03 - SSP por interrupción y DMA](./03-ssp-interrupciones-dma.md).

## Los registros del SSP

Tipo `LPC_SSP_TypeDef` en `LPC17xx.h`. Nombres exactos del header:

| Registro | Función |
|----------|---------|
| `CR0` | Control 0: tamaño de dato (`DSS`), formato (`FRF`), `CPOL`/`CPHA`, divisor fino (`SCR`) |
| `CR1` | Control 1: habilitar (`SSE`), maestro/esclavo (`MS`), loopback (`LBM`), slave-out-disable (`SOD`) |
| `DR` | Data Register: escribir = encolar en FIFO TX; leer = sacar de FIFO RX |
| `SR` | Status: flags de FIFO `TFE`/`TNF`/`RNE`/`RFF`/`BSY` |
| `CPSR` | Clock Prescale: divisor grueso del reloj (número par) |
| `IMSC` / `RIS` / `MIS` / `ICR` | interrupciones (página 03) |
| `DMACR` | habilitar DMA TX/RX (página 03) |

### CR0 (Control Register 0)

| Bits | Nombre | Significado |
|------|--------|-------------|
| 3:0 | `DSS` | Data Size Select: tamaño del dato, **4 a 16 bits**. Se programa como (n−1): para 8 bits, `DSS=0b0111` (7). El header usa `SSP_CR0_DSS(8)` que ya hace el (n−1) |
| 5:4 | `FRF` | Frame Format: `00`=SPI Motorola, `01`=TI, `10`=Microwire |
| 6 | `CPOL` | polaridad del reloj (ver la trampa abajo) |
| 7 | `CPHA` | fase del reloj |
| 15:8 | `SCR` | Serial Clock Rate: divisor fino, 0 a 255 |

**La trampa del CPOL en el SSP.** El header CMSIS define, contra toda intuición:

```c
#define SSP_CPOL_HI   ((uint32_t)(0))            // bit CPOL = 0
#define SSP_CPOL_LO   SSP_CR0_CPOL_HI            // bit CPOL = 1  (1<<6)
```

El comentario del propio header lo aclara: si el bit `CPOL` (bit 6 de CR0) está en **0**, el SSP
mantiene el reloj en **bajo** entre tramas; si está en **1**, lo mantiene en **alto**. O sea, el bit
sigue la convención clásica (bit 0 → reposo bajo → modo 0/1), pero los *nombres* `SSP_CPOL_HI` /
`SSP_CPOL_LO` de NXP están al revés de lo que uno esperaría: refieren al estado **activo** del reloj
(reposo bajo = pulsos altos = "HI"), no al reposo. Para no marearte:

- Querés **modo 0 / modo 1** (reloj reposa en bajo) → bit CPOL = 0 → usá `SSP_CPOL_HI`.
- Querés **modo 2 / modo 3** (reloj reposa en alto) → bit CPOL = 1 → usá `SSP_CPOL_LO`.

CPHA es directo: `SSP_CPHA_FIRST` (=0, muestrea en el primer flanco) o `SSP_CPHA_SECOND` (=1, segundo
flanco). Para el clásico **modo 0**: `CPOL` bit = 0 y `CPHA` = 0 → `SSP_CPOL_HI` + `SSP_CPHA_FIRST`.

### CR1 (Control Register 1)

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 0 | `LBM` | Loop Back Mode: conecta internamente MOSI con MISO; sirve para test sin hardware externo |
| 1 | `SSE` | SSP Enable: habilita el periférico. Hay que configurar CR0/CPSR **antes** de poner SSE=1 |
| 2 | `MS` | Master/Slave: 0 = maestro, 1 = esclavo |
| 3 | `SOD` | Slave Output Disable: en modo esclavo, deshabilita MISO (para esclavos en bus compartido) |

### SR (Status Register): los flags de FIFO

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 0 | `TFE` | TX FIFO Empty: la FIFO de transmisión está vacía |
| 1 | `TNF` | TX FIFO Not Full: hay lugar para encolar otro dato a transmitir |
| 2 | `RNE` | RX FIFO Not Empty: hay al menos un dato recibido para leer |
| 3 | `RFF` | RX FIFO Full: la FIFO de recepción se llenó (cuidado: si no la vaciás, el próximo dato es overrun) |
| 4 | `BSY` | Busy: hay una transferencia en curso o la FIFO TX no está vacía |

### Las FIFOs de 8 niveles

Acá está la ventaja grande del SSP sobre el legacy. Hay una **FIFO de 8 entradas para TX** y otra
**de 8 para RX**. Escribir en `DR` encola en TX; leer `DR` desacola de RX. El hardware va sacando de
la FIFO TX, transmitiendo, y metiendo lo recibido en la FIFO RX, todo en paralelo con tu código.

Esto cambia el patrón de uso. En el legacy escribías un byte y esperabas. Con el SSP podés **precargar
hasta 8 bytes** en la FIFO TX de un saque, y mientras el hardware los transmite, vos leés los que ya
llegaron a la RX. Eso mantiene el bus saturado sin huecos, que es lo que da el throughput alto.

El precio: como TX y RX corren juntas, **por cada byte que metés en TX vas a recibir un byte en RX**.
Si transmitís 8 y no vaciás la RX, a partir del 9no entra overrun (`ROR`). La regla de oro del SSP en
polling: por cada escritura de `DR`, en algún momento hacé una lectura de `DR`, aunque descartes el
valor. El driver lo hace por vos.

### CPSR y la velocidad de SCK

El SSP tiene divisor de **dos etapas**: el prescaler grueso `CPSR` (par) y el divisor fino `SCR`
(dentro de CR0):

```
f_SCK = PCLK_ssp / (CPSDVSR × (1 + SCR))
```

donde `CPSDVSR` es el valor de `CPSR` (par, de 2 a 254) y `SCR` va de 0 a 255. Tenés dos perillas:
una gruesa (CPSR) y una fina (SCR), lo que da muchísimas frecuencias posibles.

**Ejemplo numérico.** `PCLK_ssp = 25 MHz`, querés ~1 MHz:

- Con `CPSR = 2` y `SCR = 0`: `f = 25e6 / (2 × 1) = 12.5 MHz`. Demasiado.
- Con `CPSR = 2` y `SCR = 11`: `f = 25e6 / (2 × 12) ≈ 1.04 MHz`. Cerca.
- Con `CPSR = 4` y `SCR = 5`: `f = 25e6 / (4 × 6) ≈ 1.04 MHz`. Otra combinación válida.

La estrategia que usa el driver CMSIS (función `setSSPclock` en `lpc17xx_ssp.c`) es: usar el
**prescaler más chico posible** (arranca en `CPSR=2`) y subir `SCR` hasta bajar de la frecuencia
objetivo; si `SCR` se desborda de 255, recién ahí sube `CPSR` en 2. Así maximiza la resolución.

`PCLK_ssp` sale de `PCLKSEL` (PCLKSEL0 para SSP1 en bits 21:20, PCLKSEL1 para SSP0 en bits 11:10). Por
defecto es `CCLK/4`. Con `CCLK = 100 MHz`, `PCLK_ssp = 25 MHz`. Los límites que fija el manual
(§18.6.5): como **maestro**, el SCK máximo es `PCLK/2` (CPSR=2, SCR=0 → 12.5 MHz con PCLK de 25 MHz);
como **esclavo**, el reloj que te manda el maestro no debe superar `PCLK/12` (y ahí `CPSR` no juega).
Mucho más rápido que el I2C en cualquier caso; en la práctica el límite real suele ser tu esclavo y la
calidad del cable.

## Una transferencia a registro

A diferencia del I2C, el SSP no es una máquina de estados: escribís en `DR`, esperás, leés `DR`. Por
eso una transferencia a mano es directa.

```c
#include <LPC17xx.h>

#define SSP_TFE  (1u << 0)
#define SSP_TNF  (1u << 1)
#define SSP_RNE  (1u << 2)
#define SSP_BSY  (1u << 4)

void ssp0_init(void) {
    LPC_SC->PCONP |= (1u << 21);          // encender SSP0 (PCONP bit 21 = PCSSP0)

    // Pines funcion 2: SCK0=P0.15, MISO0=P0.17, MOSI0=P0.18 (CS por GPIO)
    LPC_PINCON->PINSEL0 &= ~(0x3u << 30);
    LPC_PINCON->PINSEL0 |=  (0x2u << 30);             // P0.15 SCK0
    LPC_PINCON->PINSEL1 &= ~((0x3u<<2) | (0x3u<<4));
    LPC_PINCON->PINSEL1 |=  ((0x2u<<2) | (0x2u<<4));  // P0.17 MISO0, P0.18 MOSI0

    // CR0: 8 bits (DSS=7), formato SPI (FRF=00), modo 0 (CPOL bit=0, CPHA=0), SCR=11
    LPC_SSP0->CR0  = (7u << 0) | (11u << 8);
    LPC_SSP0->CPSR = 2;                    // con SCR=11 -> ~1 MHz a PCLK 25 MHz
    LPC_SSP0->CR1  = (1u << 1);            // SSE=1: habilitar como maestro (MS=0)
}

uint8_t ssp0_transfer(uint8_t dato) {
    while (!(LPC_SSP0->SR & SSP_TNF)) { }     // esperar lugar en FIFO TX
    LPC_SSP0->DR = dato;                       // encolar para transmitir
    while (!(LPC_SSP0->SR & SSP_RNE)) { }      // esperar el byte recibido (full-duplex)
    return (uint8_t) LPC_SSP0->DR;             // leer lo recibido y vaciar la RX
}
```

Notá que esperamos `RNE` (hay dato recibido), no solo `BSY`. En full-duplex el byte recibido es la
señal más confiable de que la transferencia terminó: si esperás `RNE` y vaciás la RX en cada vuelta,
nunca acumulás overrun.

El CS se maneja como GPIO común: bajarlo antes, subirlo al terminar.

```c
#define CS  (1u << 16)   // P0.16 como GPIO de chip-select

LPC_GPIO0->FIODIR |= CS;          // CS como salida (una vez, en el setup)
LPC_GPIO0->FIOSET  = CS;          // reposo en alto

LPC_GPIO0->FIOCLR = CS;           // seleccionar el esclavo
uint8_t r = ssp0_transfer(0x9F);  // mandar comando, recibir respuesta
LPC_GPIO0->FIOSET = CS;           // soltar el esclavo
```

Sobre los pines: SSP0 también sale por P1.20 (SCK0), P1.21 (SSEL0), P1.23 (MISO0) y P1.24 (MOSI0)
con función 3, útil si P0.15–18 están ocupados. SSP1 usa P0.6 (SSEL1), P0.7 (SCK1), P0.8 (MISO1) y
P0.9 (MOSI1) con función 2.

## El driver CMSIS `lpc17xx_ssp`

El driver configura `CR0`/`CR1`/`CPSR` a partir de una struct legible y transfiere buffers enteros
con una llamada (maneja la FIFO por vos).

```c
#include "lpc17xx_ssp.h"
#include "lpc17xx_pinsel.h"

void ssp0_init(void) {
    // Pines funcion 2 (el driver no toca PINSEL)
    PINSEL_CFG_Type pin;
    pin.Funcnum = 2; pin.OpenDrain = 0; pin.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin.Portnum = 0;
    pin.Pinnum = 15; PINSEL_ConfigPin(&pin);   // SCK0
    pin.Pinnum = 17; PINSEL_ConfigPin(&pin);   // MISO0
    pin.Pinnum = 18; PINSEL_ConfigPin(&pin);   // MOSI0

    SSP_CFG_Type cfg;
    SSP_ConfigStructInit(&cfg);     // por defecto: maestro, 8 bits, modo 0, formato SPI, 1 MHz
    cfg.ClockRate   = 1000000;      // 1 MHz
    cfg.Databit     = SSP_DATABIT_8;
    cfg.FrameFormat = SSP_FRAME_SPI;
    cfg.CPHA        = SSP_CPHA_FIRST; // CPHA = 0
    cfg.CPOL        = SSP_CPOL_HI;    // bit CPOL = 0 (reloj reposa en bajo). Juntos: modo 0
    cfg.Mode        = SSP_MASTER_MODE;
    SSP_Init(LPC_SSP0, &cfg);        // enciende PCONP y configura CR0/CR1/CPSR
    SSP_Cmd(LPC_SSP0, ENABLE);       // pone SSE=1
}
```

| Campo de `SSP_CFG_Type` | Para qué |
|-------------------------|----------|
| `Databit` | tamaño de palabra: `SSP_DATABIT_4` ... `SSP_DATABIT_16` |
| `CPHA` / `CPOL` | el modo SPI (debe coincidir con el esclavo; ojo con los nombres invertidos de CPOL) |
| `ClockRate` | frecuencia de SCK en Hz; el driver calcula CPSR y SCR |
| `Mode` | `SSP_MASTER_MODE` o `SSP_SLAVE_MODE` |
| `FrameFormat` | `SSP_FRAME_SPI` / `SSP_FRAME_TI` / `SSP_FRAME_MICROWIRE` |

### Los tres formatos de trama (FRF)

El SSP no solo habla SPI Motorola; soporta tres protocolos de trama:

- **SPI (Motorola)**: el clásico de esta página. CS activo en bajo durante la trama, CPOL/CPHA
  aplican. Es el 99% de los casos.
- **TI (Texas Instruments SSI)**: el SSEL es un **pulso de un ciclo** al inicio de cada trama, no un
  nivel sostenido. CPOL/CPHA no aplican (el formato fija el flanco). Se usa con códecs y DSPs de TI.
- **Microwire (National)**: half-duplex orientado a comando-respuesta. El maestro manda un byte de
  control y el esclavo responde; no es full-duplex como el SPI. Hay esclavos viejos (EEPROM 93Cxx)
  que lo usan.

En el curso usamos SPI Motorola salvo que el esclavo pida otra cosa. Los ejemplos oficiales tienen
uno de cada uno: `examples/SSP/Master`, `examples/SSP/TI`, `examples/SSP/MicroWire`.

## Transferir buffers: `SSP_ReadWrite`

Como el SPI es full-duplex, el driver tiene una struct con buffer de TX y de RX a la vez:

```c
SSP_DATA_SETUP_Type t;
uint8_t tx[3] = { 0x9F, 0xFF, 0xFF };   // comando + 2 dummy para generar reloj
uint8_t rx[3];
t.tx_data = tx;
t.rx_data = rx;
t.length  = 3;
SSP_ReadWrite(LPC_SSP0, &t, SSP_TRANSFER_POLLING);
// rx[1], rx[2] tienen lo que respondio el esclavo
```

Si solo querés enviar, dejá `rx_data = NULL`; si solo recibir, `tx_data = NULL` (el driver manda
dummy bytes para generar el reloj). El driver llena la FIFO TX, vacía la RX y respeta los flags
`TNF`/`RNE`, así que no tenés que pensar en la FIFO.

## Ejemplo: leer el JEDEC ID de una flash SPI

Casi todas las flash SPI (ej. W25Q32) responden al comando `0x9F` (JEDEC ID) con 3 bytes de
identificación. El CS lo manejamos como GPIO.

```c
#include "lpc17xx_ssp.h"
#include "lpc17xx_gpio.h"
#define CS (1u << 16)   // P0.16 como GPIO

void leer_id_flash(uint8_t id[3]) {
    uint8_t tx[4] = { 0x9F, 0, 0, 0 };
    uint8_t rx[4];
    SSP_DATA_SETUP_Type t = { .tx_data = tx, .rx_data = rx, .length = 4 };

    GPIO_ClearValue(0, CS);                          // bajar CS
    SSP_ReadWrite(LPC_SSP0, &t, SSP_TRANSFER_POLLING);
    GPIO_SetValue(0, CS);                            // subir CS
    // los primeros bits de rx[0] son basura (mientras se transmitia 0x9F)
    id[0] = rx[1]; id[1] = rx[2]; id[2] = rx[3];     // los 3 bytes de ID
}

int main(void) {
    ssp0_init();
    GPIO_SetDir(0, CS, 1);
    GPIO_SetValue(0, CS);          // CS en reposo (alto)
    uint8_t id[3];
    leer_id_flash(id);             // ej. {0xEF, 0x40, 0x16} para una W25Q32
    while (1) { }
}
```

El patrón "**escribir para leer**" está acá: para recibir los 3 bytes de ID hay que transmitir 3
bytes dummy (los ceros), porque sin reloj el esclavo no devuelve nada. El byte recibido durante el
`0x9F` es basura; los útiles son `rx[1..3]`.

## SPI vs I2C: cuándo cada uno

| | I2C | SPI/SSP |
|--|-----|---------|
| Cables | 2 (SDA, SCL) | 3 + 1 CS por esclavo |
| Velocidad | 100 k–400 k (–3.4 M) | varios MHz (mucho más rápido) |
| Direccionamiento | por dirección de 7 bits | por línea CS dedicada |
| Full-duplex | no | sí |
| Pull-ups externas | sí | no |
| Ideal para | muchos sensores lentos en pocos cables | datos rápidos (SD, flash, displays) |

## Errores comunes

| Error | Síntoma / corrección |
|-------|----------------------|
| Modo CPOL/CPHA distinto al del esclavo | datos corridos un bit o basura; usá el modo del datasheet (suele ser 0) |
| Confundir los nombres `SSP_CPOL_HI/LO` | recordá: `SSP_CPOL_HI` = bit 0 = reloj reposa en bajo = modo 0/1 |
| No vaciar la FIFO RX | a partir del 9no byte hay overrun (`ROR`); leé `DR` por cada `DR` escrito |
| Leer `DR` sin chequear `RNE` | leés basura de una FIFO vacía; esperá `RNE` antes de leer |
| Configurar CR0/CPSR con SSE ya en 1 | configurá todo y recién después poné `SSE=1` |
| Olvidar manejar el CS | bajar CS antes, subir después de cada transacción completa |
| No mandar dummy bytes al solo recibir | sin reloj no hay respuesta; mandá algo para clockear |
| Frecuencia mayor a la del esclavo o cable largo | bajá `ClockRate`; un cable largo no aguanta varios MHz |
| MISO/MOSI cruzados | MOSI del maestro va al MOSI/SI del esclavo, MISO al SO |

## Ejercicios

1. Leé el JEDEC ID de una memoria flash SPI y mostralo por UART.
2. Escribí y leé un sector de una flash SPI (write-enable `0x06`, page program `0x02`, read `0x03`).
3. Manejá un display por SPI (ej. ST7735 TFT o MAX7219 de 7 segmentos).
4. Probá el `LBM` (loopback): habilitalo en CR1, transmití un patrón y verificá que lo recibís igual,
   sin ningún esclavo conectado.

> Ejemplos oficiales: [`../../library/examples/SSP/`](../../library/examples/SSP/) (Master, Slave, TI,
> MicroWire, dma).

---

**Anterior:** [01 - SPI: protocolo y SPI legacy](./01-spi-registros.md) ·
**Siguiente:** [03 - SSP por interrupción y DMA](./03-ssp-interrupciones-dma.md)
