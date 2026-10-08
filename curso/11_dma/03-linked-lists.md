# Linked Lists: cadenas, anillos y ping-pong

Una transferencia simple termina al completar `transferSize` transferencias del bus de destino. Una
Linked List Item (LLI) resuelve tres casos distintos:

1. más de 4095 transferencias de destino;
2. buffers no contiguos (*scatter/gather*);
3. transferencias continuas mediante un anillo.

## El descriptor

```c
typedef struct {
    uint32_t srcAddr;
    uint32_t dstAddr;
    uint32_t nextLLI;
    uint32_t control;
} GPDMA_LLI_T;
```

El layout son exactamente cuatro words en ese orden. La LLI debe permanecer en RAM, alineada a 4
bytes y viva mientras el canal pueda leerla. Una variable automática de una función no sirve si la
función retorna antes de terminar el DMA; se usan descriptores `static` o globales.

`control` tiene el mismo formato que `DMACCControl`: tamaño, burst de ambos extremos, anchos,
incrementos y bit `I`. Cada descriptor puede decidir por separado si genera terminal count.

## El “descriptor cero” vive en los registros

`GPDMA_SetupChannel()` carga el primer tramo directamente en los registros del canal. El campo
`linkedList` **no apunta al primer tramo**: apunta al descriptor que se cargará después.

```text
registros del canal (tramo A) -> linkedList -> LLI B -> LLI C -> 0
```

Si `linkedList` apunta otra vez a A, A se ejecuta dos veces al comienzo. En un anillo esto puede ser
intencional, pero en una cadena finita casi siempre es un error.

## Cadena finita: juntar tres buffers

```c
static GPDMA_LLI_T lli[2];

uint32_t control_sin_irq =
    N
    | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_32)
    | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_32)
    | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
    | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
    | GPDMA_DMACCxControl_SI
    | GPDMA_DMACCxControl_DI;

lli[0] = (GPDMA_LLI_T){
    .srcAddr = (uint32_t)(uintptr_t)buffer_b,
    .dstAddr = (uint32_t)(uintptr_t)&destino[N],
    .nextLLI = (uint32_t)(uintptr_t)&lli[1],
    .control = control_sin_irq,
};
lli[1] = (GPDMA_LLI_T){
    .srcAddr = (uint32_t)(uintptr_t)buffer_c,
    .dstAddr = (uint32_t)(uintptr_t)&destino[2*N],
    .nextLLI = 0,
    .control = control_sin_irq | GPDMA_DMACCxControl_I,
};

GPDMA_Channel_CFG_T cfg = {
    .channelNum = GPDMA_CH_7,
    .transferSize = N,                 // tramo A
    .type = GPDMA_M2M,
    .srcMemAddr = (uint32_t)(uintptr_t)buffer_a,
    .dstMemAddr = (uint32_t)(uintptr_t)destino,
    .src = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},
    .dst = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},
    .intTC = DISABLE,                  // A no interrumpe
    .intErr = ENABLE,
    .linkedList = (uint32_t)(uintptr_t)&lli[0], // después viene B
};
```

Solo C tiene `I=1`, así que hay una IRQ cuando termina la cadena completa. Para copiar 10000 words se
usa el mismo patrón con tramos 4095 + 4095 + 1810.

Ejemplo compilable: [`01_m2m/main.c`](../ejemplos/dma/configs/01_m2m/main.c).

## Anillo de una LLI: DAC continuo

El último descriptor puede apuntar al primero. Un descriptor que se apunta a sí mismo repite siempre
la misma tabla:

```c
static GPDMA_LLI_T anillo;

anillo = (GPDMA_LLI_T){
    .srcAddr = (uint32_t)(uintptr_t)tabla_dacr,
    .dstAddr = (uint32_t)(uintptr_t)&LPC_DAC->DACR,
    .nextLLI = (uint32_t)(uintptr_t)&anillo,
    .control = N
             | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
             | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
             | GPDMA_DMACCxControl_SI,
};

GPDMA_Channel_CFG_T cfg = {
    .channelNum = GPDMA_CH_1,
    .transferSize = N,
    .type = GPDMA_M2P,
    .srcMemAddr = (uint32_t)(uintptr_t)tabla_dacr,
    .dstConn = GPDMA_DAC,
    .src = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = ENABLE},
    .dst = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = DISABLE},
    .intTC = DISABLE, .intErr = ENABLE,
    .linkedList = (uint32_t)(uintptr_t)&anillo,
};
```

Las muestras ya deben tener el formato de `DACR`: `DAC_VALUE(valor)` coloca los 10 bits en 15:6. El
DMA no llama funciones ni convierte formatos. El timeout interno del DAC produce una request por
muestra y la frecuencia de la forma de onda es `f_request/N`.

El orden evita perder la primera request: configurar el DAC con request deshabilitada, configurar y
arrancar GPDMA, y recién entonces habilitar `DACCTRL.DMA_ENA`.

Ejemplo compilable: [`04_dac_m2p/main.c`](../ejemplos/dma/configs/04_dac_m2p/main.c).

## Anillo de dos LLI: ADC ping-pong

Dos buffers permiten procesar uno mientras el DMA llena el otro:

```text
registros llenan A -> LLI B -> LLI A -> LLI B -> ...
```

Ambos controles llevan `I=1`. Cada IRQ indica que terminó un bloque; el software alterna qué buffer
procesa. El primer `linkedList` debe apuntar a B porque A ya está cargado en los registros.

```c
lli_a.nextLLI = (uint32_t)(uintptr_t)&lli_b;
lli_b.nextLLI = (uint32_t)(uintptr_t)&lli_a;
cfg.dstMemAddr = (uint32_t)(uintptr_t)buffer_a;
cfg.linkedList = (uint32_t)(uintptr_t)&lli_b;
```

Ejemplos compilables:

- ADC word a word: [`03_adc_p2m/main.c`](../ejemplos/dma/configs/03_adc_p2m/main.c).
- UART byte a byte: [`06_uart_rx_ping_pong/main.c`](../ejemplos/dma/configs/06_uart_rx_ping_pong/main.c).

## Parar o pausar un anillo

Un anillo nunca llega por sí solo a `nextLLI = 0`:

```c
GPDMA_ChannelPause(canal);          // Halt=1; conserva el estado
GPDMA_ChannelResume(canal);         // Halt=0; continúa
GPDMA_ChannelGracefulStop(canal);   // Halt, drena FIFO y limpia Enable
```

Después de `GracefulStop` hay que ejecutar de nuevo `GPDMA_SetupChannel()` antes de reutilizar el
canal. Una parada inmediata con `GPDMA_ChannelStop()` puede descartar hasta cuatro words pendientes.

## Errores frecuentes

| Error | Resultado |
|---|---|
| descriptor en el stack que deja de existir | el DMA lee datos basura |
| `nextLLI` sin inicializar | salto a una dirección impredecible |
| puntero LLI no alineado a 4 | bits reservados no nulos/error |
| `linkedList` apunta al tramo ya cargado | se repite el primer bloque |
| anillo con `I=1` sin necesidad | tormenta periódica de IRQ |
| modificar un descriptor que el DMA está por leer | carrera y transferencia corrupta |
| asumir que LLI cambia el tipo de flujo | imposible: `DMACCConfig` no forma parte de la LLI |

Ese último punto es central: una cadena puede cambiar direcciones, tamaño, ancho, burst, incrementos e
IRQ, pero todo el canal conserva M2M/M2P/P2M/P2P y las mismas conexiones periféricas.

---

**Anterior:** [02 - DMA con el driver](./02-dma-con-driver.md) ·
**Siguiente módulo:** [12 - Debug](../12_debug/)
