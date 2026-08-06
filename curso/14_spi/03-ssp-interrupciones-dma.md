# SSP por interrupción y DMA

El polling de las páginas anteriores es perfecto para transacciones cortas: pedir un ID, mandar un
comando a un display, leer un registro de un sensor. Pero si tenés que mover **bloques grandes** (un
sector de SD de 512 bytes, un framebuffer de TFT) y no querés que la CPU se quede dando vueltas en un
`while`, hay dos mecanismos: **interrupciones** y **DMA**. Esta página los cubre a registro y con el
driver.

Repaso de las FIFOs (de la página 02): el SSP tiene 8 niveles de TX y 8 de RX. La idea de las
interrupciones y el DMA es no esperar byte por byte, sino que el hardware te avise (o mueva los datos
solo) cuando las FIFOs llegan a cierto umbral.

## Interrupciones del SSP

Cuatro registros gobiernan las interrupciones (nombres exactos del `LPC_SSP_TypeDef`):

| Registro | Función |
|----------|---------|
| `IMSC` | Interrupt Mask Set/Clear: habilita cada fuente (1 = habilitada) |
| `RIS` | Raw Interrupt Status: estado crudo, antes del enmascarado |
| `MIS` | Masked Interrupt Status: estado ya enmascarado (lo que realmente dispara la IRQ) |
| `ICR` | Interrupt Clear: limpia las interrupciones que se pueden limpiar a mano |

Las cuatro fuentes (mismos bits en `IMSC`/`RIS`/`MIS`):

| Bit | Nombre | Cuándo dispara |
|-----|--------|----------------|
| 0 | `ROR` (RORIM) | Receive Overrun: llegó un dato con la FIFO RX llena (se perdió un dato) |
| 1 | `RT` (RTIM) | Receive Timeout: hay datos en la FIFO RX que nadie leyó por un tiempo (32 períodos de bit, a la tasa PCLK/(CPSDVSR×(SCR+1))) |
| 2 | `RX` (RXIM) | RX FIFO al menos **medio llena** (≥4 niveles): vaciá la RX |
| 3 | `TX` (TXIM) | TX FIFO al menos **medio vacía** (≤4 niveles): recargá la TX |

Detalle importante: `RX` y `TX` se limpian **solos** al vaciar/llenar la FIFO (no hay que tocar
`ICR`). En cambio `ROR` y `RT` se limpian escribiendo 1 en el bit correspondiente de `ICR`. Por eso
`ICR` solo tiene los bits 0 (`ROR`) y 1 (`RT`).

El `RT` (timeout) es la solución a un problema sutil: si tu transferencia no es múltiplo de 4, pueden
quedar 1, 2 o 3 bytes en la FIFO RX que nunca llegan a "medio llena" y por lo tanto nunca disparan
`RX`. El `RT` salta tras un rato de inactividad y te avisa "che, vaciá lo que quedó".

### Patrón de recepción por interrupción (a registro)

```c
#include <LPC17xx.h>

#define SSP_IMSC_RX  (1u << 2)
#define SSP_IMSC_RT  (1u << 1)
#define SSP_IMSC_ROR (1u << 0)
#define SSP_SR_RNE   (1u << 2)
#define SSP_ICR_RT   (1u << 1)
#define SSP_ICR_ROR  (1u << 0)

volatile uint8_t  rx_buf[256];
volatile uint32_t rx_idx;

void ssp0_irq_recepcion_init(void) {
    // (se asume SSP0 ya configurado y habilitado, ver pagina 02)
    LPC_SSP0->IMSC = SSP_IMSC_RX | SSP_IMSC_RT | SSP_IMSC_ROR;  // habilitar RX, timeout, overrun
    NVIC_EnableIRQ(SSP0_IRQn);
}

void SSP0_IRQHandler(void) {
    uint32_t mis = LPC_SSP0->MIS;

    if (mis & SSP_IMSC_ROR) {            // overrun: perdimos datos
        LPC_SSP0->ICR = SSP_ICR_ROR;     // limpiar
        // aca podrias marcar un error
    }
    if (mis & (SSP_IMSC_RX | SSP_IMSC_RT)) {
        while (LPC_SSP0->SR & SSP_SR_RNE) {        // vaciar TODO lo que haya en la RX
            if (rx_idx < sizeof(rx_buf))
                rx_buf[rx_idx++] = (uint8_t) LPC_SSP0->DR;
            else
                (void) LPC_SSP0->DR;               // descartar si el buffer se lleno
        }
        if (mis & SSP_IMSC_RT)
            LPC_SSP0->ICR = SSP_ICR_RT;            // limpiar el timeout
    }
}
```

La clave: en la ISR vaciás la FIFO RX en un `while (RNE)`, no un solo `DR`. Si leés un solo byte y la
FIFO tenía 4, la interrupción `RX` se vuelve a disparar enseguida, pero podés perder eficiencia o, en
el peor caso, no drenar a tiempo y caer en overrun. Siempre drenar completo.

### Con el driver

El driver da una capa más alta. Para configurar fuentes:

```c
SSP_IntConfig(LPC_SSP0, SSP_INTCFG_RX, ENABLE);   // habilitar RX (half-full)
SSP_IntConfig(LPC_SSP0, SSP_INTCFG_TX, ENABLE);   // habilitar TX (half-empty)
SSP_ClearIntPending(LPC_SSP0, SSP_INTCLR_ROR);    // limpiar overrun
```

Y `SSP_ReadWrite(LPC_SSP0, &t, SSP_TRANSFER_INTERRUPT)` arranca una transferencia por interrupción:
precarga la FIFO TX, habilita las cuatro fuentes en `IMSC` y vuelve enseguida. Ojo: el driver **no**
trae la ISR hecha; tenés que escribir tu `SSP0_IRQHandler` que siga moviendo los datos con las
funciones del driver (`SSP_GetStatus`, `SSP_SendData`, `SSP_ReceiveData`, `SSP_ClearIntPending`). El
ejemplo oficial `examples/SSP/Slave/ssp_slave.c` tiene ese handler completo (el de `Master` usa
polling).

## DMA del SSP

El DMA es el siguiente nivel: el **GPDMA** mueve los datos entre memoria y la FIFO del SSP **sin que
la CPU toque un solo byte**. Ideal para bloques grandes (SD, displays). La CPU configura el canal una
vez y se va a hacer otra cosa; al final, una interrupción de fin de transferencia (Terminal Count) le
avisa.

El único registro del SSP involucrado es `DMACR`:

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 0 | `RXDMAE` | habilita pedidos de DMA en recepción (RX FIFO → memoria) |
| 1 | `TXDMAE` | habilita pedidos de DMA en transmisión (memoria → TX FIFO) |

Como SPI es full-duplex, lo normal es usar **dos canales de DMA**: uno memoria→SSP_TX (tipo
`M2P`, memory to peripheral) y otro SSP_RX→memoria (`P2M`). Las conexiones de DMA están en el header
del GPDMA: `GPDMA_CONN_SSP0_Tx` y `GPDMA_CONN_SSP0_Rx`.

Esqueleto a registro (la parte del SSP):

```c
#define SSP_DMACR_RXDMAE (1u << 0)
#define SSP_DMACR_TXDMAE (1u << 1)

LPC_SSP0->DMACR = SSP_DMACR_TXDMAE | SSP_DMACR_RXDMAE;   // habilitar DMA TX y RX
// ... configurar los 2 canales del GPDMA (M2P hacia SSP0_Tx, P2M desde SSP0_Rx) ...
// ... habilitar los canales; el GPDMA dispara la IRQ de Terminal Count al terminar ...
```

### Con el driver

El driver del SSP solo expone el control de `DMACR`:

```c
SSP_DMACmd(LPC_SSP0, SSP_DMA_TX, ENABLE);   // TXDMAE = 1
SSP_DMACmd(LPC_SSP0, SSP_DMA_RX, ENABLE);   // RXDMAE = 1
```

Toda la configuración de los canales la hace el driver del GPDMA (`lpc17xx_gpdma`), no el del SSP. El
ejemplo oficial `examples/SSP/dma/ssp_dma.c` arma exactamente esto: SSP0 en loopback (MOSI↔MISO),
canal 0 `M2P` hacia `GPDMA_CONN_SSP0_Tx`, canal 1 `P2M` desde `GPDMA_CONN_SSP0_Rx`, y espera el
Terminal Count de los dos canales en la `DMA_IRQHandler`. Es el mejor punto de partida si vas a mover
bloques grandes.

## Cuándo usar cada mecanismo

| Mecanismo | Cuándo |
|-----------|--------|
| **Polling** | transacciones cortas, latencia baja, código simple (pedir ID, comando de display) |
| **Interrupción** | streams medianos donde no querés bloquear, pero el volumen no justifica DMA |
| **DMA** | bloques grandes (SD 512 B, framebuffer), throughput máximo, CPU libre para otra cosa |

Regla práctica: empezá con polling. Pasá a interrupción si el `while` de espera te está comiendo
tiempo que necesitás para otra cosa. Pasá a DMA solo si movés cientos/miles de bytes seguidos.

## Errores comunes con interrupción/DMA

| Error | Síntoma / corrección |
|-------|----------------------|
| Leer un solo `DR` en la ISR | drená la FIFO RX completa con `while (RNE)`, no un solo byte |
| Olvidar el `RT` (timeout) | los últimos 1–3 bytes (resto no múltiplo de 4) quedan colgados; habilitá `RT` |
| No limpiar `ROR`/`RT` en `ICR` | la IRQ se redispara infinitamente; escribí 1 en el bit de `ICR` |
| Habilitar `TX` (half-empty) y no recargar | la IRQ TX se redispara siempre porque la FIFO queda vacía; deshabilitala al terminar |
| DMA sin habilitar `DMACR` | el GPDMA configurado pero el SSP no pide datos; faltó `SSP_DMACmd`/`DMACR` |
| Un solo canal DMA para full-duplex | el SSP recibe siempre: si el DMA solo alimenta la TX y nadie drena la RX, hay overrun; usá los dos canales (o drená la RX por CPU) |

## Ejercicios

1. Recibí 100 bytes de un esclavo SPI por interrupción y guardalos en un buffer; verificá que el `RT`
   te entrega el último bloque incompleto.
2. Usá el loopback (`LBM`) + DMA para copiar 64 bytes de un buffer a otro a través del SSP, sin
   hardware externo (basate en `examples/SSP/dma/ssp_dma.c`).
3. Compará el tiempo de CPU ocupado al mandar 512 bytes por polling vs por DMA (medí con un GPIO y
   osciloscopio, o con el SysTick).

> Ejemplo oficial de DMA: [`../../library/examples/SSP/dma/`](../../library/examples/SSP/dma/).

---

**Anterior:** [02 - SSP a registro y con driver](./02-ssp-con-driver.md) ·
**Siguiente módulo:** [15 - USB](../15_usb/)
