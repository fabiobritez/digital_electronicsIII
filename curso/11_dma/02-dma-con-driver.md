# DMA con el driver modernizado

El driver `lpc17xx_gpdma` traduce una estructura legible a los registros
`DMACCControl`/`DMACCConfig`. La versión usada por el curso proviene del repositorio
`LPC17xx-CMSIS-Driver-Enhancement`; el commit importado y los ajustes locales están registrados en
[`library/CMSISv2p00_LPC17xx/UPSTREAM.md`](../../library/CMSISv2p00_LPC17xx/UPSTREAM.md).

## La estructura de un canal

```c
GPDMA_Channel_CFG_T cfg = {
    .channelNum   = GPDMA_CH_7,
    .transferSize = 256,             // transferencias del destino; 1..4095
    .type         = GPDMA_M2M,

    .srcMemAddr   = (uint32_t)(uintptr_t)origen,
    .dstMemAddr   = (uint32_t)(uintptr_t)destino,
    .srcConn      = GPDMA_ADC,       // ignorado si el origen es memoria
    .dstConn      = GPDMA_ADC,       // ignorado si el destino es memoria

    .src = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},
    .dst = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},

    .intTC        = ENABLE,
    .intErr       = ENABLE,
    .linkedList   = 0,
};
```

Las decisiones importantes quedan separadas:

- `type`: `GPDMA_M2M`, `GPDMA_M2P`, `GPDMA_P2M` o `GPDMA_P2P`.
- `srcMemAddr`/`dstMemAddr`: se usan solamente en las puntas que son memoria.
- `srcConn`/`dstConn`: seleccionan request y registro en las puntas periféricas.
- `src` y `dst`: cada extremo tiene su propio ancho, burst e incremento.
- `intTC`: habilita el bit `I` del primer tramo y la máscara `ITC` del canal.
- `linkedList`: apunta al descriptor posterior al tramo cargado en registros; 0 significa sin LLI.

El driver acepta `GPDMA_WIDTH_AUTO` y `GPDMA_BURST_AUTO`, pero en material didáctico conviene escribir
los valores explícitos: así se ve por qué una UART usa byte/burst 1 y el DAC usa word/burst 1.

## Conexiones y requests

| Conexión | Nº | Uso válido habitual |
|---|---:|---|
| `GPDMA_ADC` | 4 | fuente P2M |
| `GPDMA_DAC` | 7 | destino M2P |
| `GPDMA_UART0_Tx` … `GPDMA_UART3_Tx` | 8, 10, 12, 14 | destino M2P/P2P |
| `GPDMA_UART0_Rx` … `GPDMA_UART3_Rx` | 9, 11, 13, 15 | fuente P2M/P2P |
| `GPDMA_MAT0_0` … `GPDMA_MAT3_1` | 16…23 | disparo periódico |

El enum contiene otras conexiones que se estudian más adelante; este capítulo no las usa.

### El multiplexado `DMAREQSEL`

Las líneas físicas 8 a 15 están compartidas: la línea 8 es UART0 Tx o MAT0.0, la 9 es UART0 Rx o
MAT0.1, y así hasta la 15 (UART3 Rx o MAT3.1). Cada bit de `LPC_SC->DMAREQSEL` elige UART (0) o timer
match (1). El driver lo ajusta a partir de `srcConn`/`dstConn`.

No se pueden usar simultáneamente, por ejemplo, UART0 Tx y MAT0.0 como requests DMA: son dos nombres
para la misma línea física.

Una limitación de la API debe quedar visible: para una conexión MAT, el driver asocia también la
dirección `&TIMx->MRy`. Si MAT solo da el ritmo para otra dirección —por ejemplo RAM→`FIOPIN` o
`FIOPIN`→RAM— request y endpoint se programan por registros. Está mostrado en
[`02_gpio_y_timer/main.c`](../ejemplos/dma/configs/02_gpio_y_timer/main.c).

## Funciones del driver

| Función | Efecto |
|---|---|
| `GPDMA_Init()` | enciende el bloque, resetea canales, limpia flags y pone `DMACConfig.E` |
| `GPDMA_SetupChannel(&cfg)` | valida y configura un canal; no lo arranca |
| `GPDMA_ChannelStart(ch)` | pone `E` y comienza o queda esperando requests |
| `GPDMA_ChannelStop(ch)` | parada inmediata; puede perder datos del FIFO |
| `GPDMA_ChannelGracefulStop(ch)` | Halt, drena el FIFO y deshabilita el canal |
| `GPDMA_ChannelPause/Resume(ch)` | pausa y continúa conservando el estado |
| `GPDMA_IntGetStatus(tipo, ch)` | consulta TC, error, estados raw o canal habilitado |
| `GPDMA_ClearIntPending(tipo, ch)` | limpia `GPDMA_CLR_INTTC` o `GPDMA_CLR_INTERR` |
| `DMA_SoftRequest(conn)` / `DMA_SoftBurstRequest(conn)` | genera una request single/burst desde software |

Los registros de request `last` existen en el bloque PL080, pero UM10360 aclara que los periféricos
del LPC1769 no soportan ese tipo de request. No forman parte del catálogo útil.

`GPDMA_Init()` se llama **una sola vez**. Repetirla al agregar otro canal borra los anteriores.

## M2M corto

```c
GPDMA_Init();

GPDMA_Channel_CFG_T cfg = {
    .channelNum = GPDMA_CH_7,          // M2M en prioridad baja
    .transferSize = N,
    .type = GPDMA_M2M,
    .srcMemAddr = (uint32_t)(uintptr_t)origen,
    .dstMemAddr = (uint32_t)(uintptr_t)destino,
    .srcConn = GPDMA_ADC, .dstConn = GPDMA_ADC, // ignorados
    .src = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},
    .dst = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_32, .increment = ENABLE},
    .intTC = ENABLE, .intErr = ENABLE, .linkedList = 0,
};

if (GPDMA_SetupChannel(&cfg) == SUCCESS) {
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelStart(GPDMA_CH_7);
}
```

`origen` y `destino` deben estar alineados a 4 bytes. Como ambos anchos son word,
`transferSize = N` significa N words, no `N*4` bytes. Si los anchos difieren, el campo cuenta las
transferencias realizadas sobre el bus de destino; el GPDMA empaqueta o desempaqueta los datos. M2M
arranca inmediatamente y por eso no debe ocupar un canal de prioridad alta.

## P2M corto: ADC a memoria

```c
ADC_IntEnable(ADC_INT_CH0);             // DONE genera request; no habilitar ADC_IRQn

GPDMA_Channel_CFG_T cfg = {
    .channelNum = GPDMA_CH_0,
    .transferSize = 100,
    .type = GPDMA_P2M,
    .dstMemAddr = (uint32_t)(uintptr_t)buffer_adc,
    .srcConn = GPDMA_ADC,
    .src = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = DISABLE},
    .dst = {.width = GPDMA_WORD, .burst = GPDMA_BSIZE_1, .increment = ENABLE},
    .intTC = ENABLE, .intErr = ENABLE, .linkedList = 0,
};

GPDMA_SetupChannel(&cfg);
GPDMA_ChannelStart(GPDMA_CH_0);         // primero queda escuchando
ADC_BurstEnable();                      // después comienza el productor
```

La word guardada es `ADGDR` completa: resultado en bits 15:4, número de canal y flags. El DMA copia;
no transforma ni desplaza el dato.

## Combinaciones del nivel

| Objetivo | Tipo | Punta fija | Punta incremental | Request |
|---|---|---|---|---|
| RAM/Flash → RAM | M2M | — | origen y destino | ninguna |
| llenar RAM con un valor | M2M | origen | destino | ninguna |
| ADC → buffer | P2M | `ADGDR` | destino | `GPDMA_ADC` |
| buffer → DAC | M2P | `DACR` | origen | `GPDMA_DAC` |
| buffer → UART Tx | M2P | `THR` | origen | `GPDMA_UARTx_Tx` |
| UART Rx → buffer | P2M | `RBR` | destino | `GPDMA_UARTx_Rx` |
| UART Rx → otra UART Tx | P2P | ambas | ninguna | ambas UART |
| RAM ↔ GPIO periódico | M2P/P2M | `FIOPIN` | memoria | MATx.y |

P2P no significa que cualquier pareja sea útil. UART Rx→UART Tx funciona porque ambos extremos usan
bytes. ADC→DAC directo no produce una conversión correcta: `ADGDR` y `DACR` tienen layouts distintos y
el DMA no aplica `>>4`, saturación ni `<<6`.

## Arranque y finalización

1. `GPDMA_Init()` una vez.
2. Configurar el periférico, manteniendo detenido el productor de requests.
3. `GPDMA_SetupChannel()`.
4. Habilitar la IRQ del GPDMA, si se usa.
5. `GPDMA_ChannelStart()`.
6. Arrancar ADC/timer o habilitar las requests del DAC.

Un bloque finito deshabilita el canal al llegar a cero. Para un anillo, usar
`GPDMA_ChannelGracefulStop()`; no limpiar `E` directamente con datos en el FIFO.

## Errores frecuentes

| Error | Consecuencia |
|---|---|
| tamaño 0 o mayor que 4095 | configuración inválida; dividir mediante LLI |
| confundir transferencias de destino con bytes | cantidad transferida incorrecta, sobre todo con anchos distintos |
| incrementar la dirección del periférico | se acceden registros vecinos |
| usar byte para `DACR` | no se escribe correctamente `VALUE[15:6]` |
| olvidar FIFO DMA en UART o request del DAC/ADC | canal que espera para siempre |
| iniciar el productor antes que el canal | se pierde la primera muestra/request |
| poner M2M en canal 0 | puede hambrear transferencias sensibles |
| buffer o LLI local que sale de alcance | el DMA usa memoria inválida |
| no limpiar TC/error en la ISR | interrupción permanente |

El catálogo compilable de recetas está en [`curso/ejemplos/dma/configs/`](../ejemplos/dma/configs/).

---

**Anterior:** [01 - DMA: concepto y registros](./01-dma-concepto-y-registros.md) ·
**Siguiente:** [03 - Linked lists y transferencias circulares](./03-linked-lists.md)
