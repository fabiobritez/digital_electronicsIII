# DMA con el driver CMSIS

El driver `lpc17xx_gpdma` arma todos los bits de `DMACCControl`/`DMACCConfig` a partir de una struct
legible. Vos describís "de acá, para allá, tantos datos, de este tipo, con este periférico", y el driver
lo traduce, elige el burst y el ancho óptimos para el periférico, configura el `DMAREQSEL` cuando hace
falta y prende el controlador.

## La struct de configuración

```c
#include "lpc17xx_gpdma.h"

GPDMA_Channel_CFG_Type cfg;
cfg.ChannelNum    = 0;                          // canal 0..7 (0 = mayor prioridad)
cfg.TransferSize  = 256;                        // cuántos ELEMENTOS (no bytes). Máx 4095
cfg.TransferWidth = GPDMA_WIDTH_WORD;           // ancho: solo se usa en M2M
cfg.SrcMemAddr    = (uint32_t)buffer_origen;    // dir. de origen, si el origen es memoria (M2M/M2P)
cfg.DstMemAddr    = (uint32_t)buffer_destino;   // dir. de destino, si el destino es memoria (M2M/P2M)
cfg.TransferType  = GPDMA_TRANSFERTYPE_M2M;     // M2M / M2P / P2M / P2P
cfg.SrcConn       = 0;                          // request line de la fuente (si P2M/P2P)
cfg.DstConn       = 0;                          // request line del destino (si M2P/P2P)
cfg.DMALLI        = 0;                           // 0 = transferencia simple; o puntero a un LLI
```

Detalles que el comentario corto no aclara:

- **`TransferWidth` solo importa en M2M.** En M2P/P2M/P2P el driver **ignora** ese campo y usa el ancho
  óptimo del periférico (de una tabla interna: ADC e I2S → word, DAC/UART/SSP → byte). Si querés otro
  ancho para un periférico, tenés que armar el `Control` a mano (ver página 3).
- **`SrcMemAddr`/`DstMemAddr`** solo se leen del lado que es memoria. En M2P el destino lo pone el driver
  (la dirección del registro FIFO del periférico, p. ej. `&DACR`); en P2M el origen lo pone el driver.
  El lado "periférico" lo dejás en 0.
- **`SrcConn`/`DstConn`** son las **request lines**, no "el periférico". Las constantes son
  `GPDMA_CONN_*` (lista completa abajo). En M2M no se usan (poné 0).

## Las DMA request lines (`GPDMA_CONN_*`)

El header define exactamente estas fuentes de request (el número es el valor que va en
`SrcPeripheral`/`DestPeripheral`):

| Constante | Nº | Constante | Nº |
|-----------|----|-----------|----|
| `GPDMA_CONN_SSP0_Tx` | 0 | `GPDMA_CONN_UART1_Tx` | 10 |
| `GPDMA_CONN_SSP0_Rx` | 1 | `GPDMA_CONN_UART1_Rx` | 11 |
| `GPDMA_CONN_SSP1_Tx` | 2 | `GPDMA_CONN_UART2_Tx` | 12 |
| `GPDMA_CONN_SSP1_Rx` | 3 | `GPDMA_CONN_UART2_Rx` | 13 |
| `GPDMA_CONN_ADC` | 4 | `GPDMA_CONN_UART3_Tx` | 14 |
| `GPDMA_CONN_I2S_Channel_0` | 5 | `GPDMA_CONN_UART3_Rx` | 15 |
| `GPDMA_CONN_I2S_Channel_1` | 6 | `GPDMA_CONN_MAT0_0` … `MAT3_1` | 16…23 |
| `GPDMA_CONN_DAC` | 7 | | |
| `GPDMA_CONN_UART0_Tx` | 8 | | |
| `GPDMA_CONN_UART0_Rx` | 9 | | |

### El multiplexado de fuentes: `DMAREQSEL`

Acá hay una sutileza que el header esconde: las request lines **8 a 15 están compartidas**. La misma
línea de hardware la usan o bien una UART, o bien una salida *match* de timer:

- línea 8: UART0 Tx **o** MAT0.0 (timer 0, match 0)
- línea 9: UART0 Rx **o** MAT0.1
- … y así hasta la 15 (UART3 Rx **o** MAT3.1)

¿Quién gana? Lo decide el registro `LPC_SC->DMAREQSEL` (un bit por cada línea 8..15): 0 = UART, 1 =
timer match. El driver lo configura **solo** según la `GPDMA_CONN_*` que elegiste: si pedís
`GPDMA_CONN_UART0_Tx` pone el bit en 0; las constantes `GPDMA_CONN_MAT*` (16..23) hacen que el driver
ponga el bit en 1 y use la misma línea física 8..15. Por eso en el header `MAT0_0 = 16` aunque
físicamente sea la request line 8: el driver hace la cuenta (`conn - 8`) para el campo `SrcPeripheral`.

Moraleja: **no podés usar al mismo tiempo UART0 Tx por DMA y MAT0.0 por DMA**, comparten línea. Si los
necesitás juntos, hay que elegir otra combinación.

## Funciones del driver

| Función | Qué hace |
|---------|----------|
| `GPDMA_Init()` | enciende el GPDMA (`PCONP`), resetea los 8 `DMACCConfig` y limpia todos los flags |
| `GPDMA_Setup(&cfg)` | configura un canal: arma `Control`/`Config`, ajusta `DMAREQSEL`, prende el controlador. **No arranca el canal** |
| `GPDMA_ChannelCmd(ch, ENABLE)` | pone `DMACCConfig.E`: arranca (o frena con `DISABLE`) el canal |
| `GPDMA_IntGetStatus(tipo, ch)` | en el handler: consulta estado (INT, INTTC, INTERR, etc.) de un canal |
| `GPDMA_ClearIntPending(tipo, ch)` | limpia la IRQ (`GPDMA_STATCLR_INTTC` o `GPDMA_STATCLR_INTERR`) |

Dos cosas importantes del comportamiento de `GPDMA_Setup`:

- Si el canal **ya estaba habilitado**, `GPDMA_Setup` devuelve `ERROR` y no toca nada. Hay que frenarlo
  primero (`GPDMA_ChannelCmd(ch, DISABLE)`).
- Configura el canal con las **máscaras de IRQ de fin y error ya activadas** (`IE` e `ITC`), pero deja
  `E = 0`. Por eso el orden es siempre: `GPDMA_Setup(&cfg)` → `NVIC_EnableIRQ(DMA_IRQn)` →
  `GPDMA_ChannelCmd(ch, ENABLE)`.

## Ejemplo M2M completo (con interrupción de fin)

```c
#include "lpc17xx_gpdma.h"
#define N 256
uint32_t origen[N], destino[N];
volatile uint8_t terminado = 0, hubo_error = 0;

void DMA_IRQHandler(void) {
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {   // ¿terminó el canal 0?
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
        terminado = 1;
    }
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {  // ¿error en el canal 0?
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
        hubo_error = 1;
    }
}

int main(void) {
    for (int i = 0; i < N; i++) origen[i] = i;       // llenar el origen

    GPDMA_Init();

    GPDMA_Channel_CFG_Type cfg;
    cfg.ChannelNum    = 0;
    cfg.TransferSize  = N;
    cfg.TransferWidth = GPDMA_WIDTH_WORD;            // M2M sí usa el ancho
    cfg.SrcMemAddr    = (uint32_t)origen;
    cfg.DstMemAddr    = (uint32_t)destino;
    cfg.TransferType  = GPDMA_TRANSFERTYPE_M2M;
    cfg.SrcConn       = 0;
    cfg.DstConn       = 0;
    cfg.DMALLI        = 0;
    GPDMA_Setup(&cfg);

    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);                     // arrancar la copia

    while (!terminado && !hubo_error) {
        /* el CPU podría hacer otra cosa útil acá */
    }
    // 'destino' ya es copia de 'origen', sin que el CPU copiara word a word
    while (1) { }
}
```

Ejemplo en el repo: [`../ejemplos/dma/m2m.c`](../ejemplos/dma/).

Nota: acá usamos el canal 0 por simplicidad, pero el manual recomienda poner los M2M en un canal de
**baja** prioridad (p. ej. el 7): un M2M nunca suelta el bus por su cuenta y en un canal alto bloquea
al resto (ver página 1).

## P2M: llenar un buffer desde el ADC sin CPU

```c
GPDMA_Channel_CFG_Type cfg;
cfg.ChannelNum    = 0;
cfg.TransferSize  = 100;                  // 100 muestras
cfg.TransferWidth = 0;                    // ignorado en P2M
cfg.SrcMemAddr    = 0;                    // el origen es un periférico, no memoria
cfg.DstMemAddr    = (uint32_t)buffer_adc; // a dónde van las muestras
cfg.TransferType  = GPDMA_TRANSFERTYPE_P2M;
cfg.SrcConn       = GPDMA_CONN_ADC;       // el ADC dispara cada transferencia
cfg.DstConn       = 0;
cfg.DMALLI        = 0;
GPDMA_Setup(&cfg);
GPDMA_ChannelCmd(0, ENABLE);
```

Cada vez que el ADC termina una conversión, su *DMA request* hace que el DMA copie `ADGDR` al buffer,
hasta juntar 100 muestras: el CPU ni se entera.

**Lo que NO se ve en este código y suele faltar:** hay que habilitar el modo DMA *en el ADC*. Configurar
el GPDMA no alcanza; si el ADC no levanta la request, el canal queda esperando para siempre. En la
struct del ADC eso es activar la conversión por hardware/burst y el bit de DMA correspondiente
(`ADCR`/`ADINTEN`). El GPDMA es el chofer; el ADC es quien toca el timbre.

Otro detalle: cada resultado del ADC es una *word* (incluye el canal y los flags en `ADGDR`); por eso el
driver usa ancho word y vas a tener que enmascarar los 12 bits de dato al leer el buffer. Ejemplo:
[`../ejemplos/dma/adc_dma_simple.c`](../ejemplos/dma/).

## M2P + LLI: generar un seno continuo por el DAC

El caso estrella: una tabla de seno en memoria, el DMA copiándola al `DACR` en bucle (LLI en anillo),
disparado por el timer interno del DAC. Salida analógica continua, cero CPU. Como tiene varias sutilezas
(el formato de `DACR`, el timeout del DAC, el LLI que se apunta a sí mismo), va completo en la página
siguiente: [03 - Linked lists y transferencias circulares](./03-linked-lists.md). Ejemplos:
[`../ejemplos/dma/dac_dma_sin.c`](../ejemplos/dma/) y [`lli_example.c`](../ejemplos/dma/).

## Combos reales (qué `TransferType` y qué `Conn`)

| Quiero… | TransferType | SrcConn | DstConn | Memoria en |
|---------|--------------|---------|---------|------------|
| ADC → buffer | P2M | `GPDMA_CONN_ADC` | - | `DstMemAddr` |
| SSP/SPI Rx → buffer | P2M | `GPDMA_CONN_SSP0_Rx` | - | `DstMemAddr` |
| buffer → DAC (onda) | M2P | - | `GPDMA_CONN_DAC` | `SrcMemAddr` |
| buffer → SSP/SPI Tx | M2P | - | `GPDMA_CONN_SSP0_Tx` | `SrcMemAddr` |
| buffer → UART Tx | M2P | - | `GPDMA_CONN_UART0_Tx` | `SrcMemAddr` |
| UART Rx → buffer | P2M | `GPDMA_CONN_UART0_Rx` | - | `DstMemAddr` |
| copiar RAM/Flash → RAM | M2M | - | - | ambas |

## Cuándo usar DMA (y cuándo no)

**Sí:** transferencias grandes o de alta frecuencia (audio, muestreo rápido del ADC, generación de
señales por DAC, buffers de comunicación SSP/UART/I2S). Libera el CPU y, sobre todo, **evita perder
datos** cuando el flujo es más rápido de lo que el CPU puede atender por interrupción dato a dato.

**No hace falta:** prender un LED, leer un botón, una conversión de ADC ocasional. El DMA tiene un costo
de configuración (varios registros, IRQ, manejo de errores) que no se justifica para transferencias
chicas y esporádicas. Si es un dato cada tanto, una IRQ común es más simple.

## Errores comunes

| Error | Síntoma | Corrección |
|-------|---------|-----------|
| Olvidar `GPDMA_Init()` o `DMACConfig.E` | ningún canal anda | inicializar el controlador antes de cualquier canal |
| No habilitar el DMA **en el periférico** | el canal queda esperando, nunca interrumpe | prender el modo DMA del ADC/DAC/UART, no solo el GPDMA |
| Incremento (SI/DI) al revés | datos repetidos o pisados | memoria incrementa, periférico no |
| `TransferSize` en bytes en vez de elementos | copia 4× de más/menos | es **cantidad de elementos**, del tamaño de `Width` |
| Pasar de 4095 elementos | se trunca en silencio: el driver enmascara con `0xFFF` (5000 → 904) | partir con LLI (página 3) |
| `SrcConn`/`DstConn` sin setear en P2M/M2P | el canal no sabe quién lo dispara | indicar la `GPDMA_CONN_*` correcta |
| No limpiar la IRQ en el handler | la IRQ se vuelve a disparar sin fin | `GPDMA_ClearIntPending(...)` |
| Buffer/LLI en una RAM no accesible | IRQ de **error** (`GPDMA_STAT_INTERR`) | ojo con la RAM AHB/USB; ver nota abajo |
| Bajar `E` de golpe con FIFO lleno | se pierden datos a medio volcar | usar Halt + esperar Active, o dejar terminar |

**Sobre la RAM:** el GPDMA es un master del bus AHB. Llega bien a la RAM principal (`0x1000_0000`) y a
las RAM AHB (`0x2007_C000` / `0x2008_0000`, las "USB RAM" que los ejemplos reusan como scratch). En
sleep mode el GPDMA **no puede acceder a la Flash** (lo aclara el ejemplo GPDMA_Sleep): si una fuente
M2M está en Flash y el chip duerme, falla. Para datos constantes que el DMA tenga que leer dormido,
copialos antes a RAM.

## Ejercicios

1. Copiá un arreglo de 1024 `uint32_t` con M2M y medí (con un timer) cuánto tardó, contra un `for`
   equivalente. ¿Cuánto del tiempo del `for` era puro mover datos?
2. Llená un buffer de 256 muestras de ADC por P2M y mandá los valores por UART. Acordate de habilitar el
   DMA en el ADC y de enmascarar los 12 bits útiles de cada muestra.
3. Generá un seno continuo por DAC con M2P + LLI en anillo y miralo en un osciloscopio (página 3).
4. Probá pedir `TransferSize = 5000` en una sola transferencia y observá qué pasa (pista: 12 bits).

> Material original: [`_origen/08_DMA.md`](./_origen/08_DMA.md). Ejemplos completos:
> [`../ejemplos/dma/`](../ejemplos/dma/).

---

**Anterior:** [01 - DMA: concepto y registros](./01-dma-concepto-y-registros.md) ·
**Siguiente:** [03 - Linked lists y transferencias circulares](./03-linked-lists.md)
