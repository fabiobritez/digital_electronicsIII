# Linked lists y transferencias circulares

Una transferencia DMA simple tiene un final: copia sus `TransferSize` elementos y se frena. Eso sirve
para un bloque puntual, pero deja afuera dos necesidades muy comunes:

1. **Transferencias más largas que 4095 elementos** (el límite del campo `TransferSize`).
2. **Transferencias que no terminan nunca** (un seno que sale por el DAC para siempre, un doble buffer de
   ADC que se rellena en loop).

La solución para las dos es la misma: las **Linked Lists** (listas enlazadas de descriptores), también
llamadas *scatter-gather*.

## La idea: descriptores encadenados

Cada canal tiene un registro `DMACCLLI`. Cuando una transferencia termina, **antes de frenar** el DMA
mira ese registro:

- Si vale 0 → no hay más, frena y dispara la IRQ de terminal count.
- Si apunta a una dirección de RAM → el DMA **lee de ahí un nuevo descriptor** (origen, destino, próximo
  LLI, control), lo carga en sus registros de canal y sigue copiando, **sin intervención del CPU**.

Un descriptor es exactamente esta struct del header (`GPDMA_LLI_Type`), cuatro words en RAM:

```c
typedef struct {
    uint32_t SrcAddr;   // dirección de origen de ESTE tramo
    uint32_t DstAddr;   // dirección de destino de ESTE tramo
    uint32_t NextLLI;   // dirección del SIGUIENTE descriptor (0 = fin)
    uint32_t Control;   // el mismo formato que DMACCControl: size, width, burst, SI/DI, I
} GPDMA_LLI_Type;
```

El orden y el tamaño importan: el hardware espera **exactamente** {Src, Dst, Next, Control} en ese orden.
La struct CMSIS ya está en ese layout.

Detalle fino: el `Control` de cada descriptor es **independiente**. Cada tramo puede tener su propio
tamaño, ancho, incrementos y su propio bit `I`. Eso es lo que permite, por ejemplo, juntar tres buffers
dispersos en uno solo (scatter-gather de verdad) o disparar una IRQ solo cada cierto bloque.

## Cómo arranca la cadena con el driver

El detalle clave: **los registros del canal son el "descriptor cero"**. Lo que `GPDMA_Setup` carga
desde la `GPDMA_Channel_CFG_Type` (`SrcMemAddr`/`DstMemAddr`/`TransferSize`) describe el **primer
tramo**, y el registro `DMACCLLI` (que `Setup` copia del campo `DMALLI`) apunta al descriptor que se
carga **después** de ese primer tramo. Cuando el tramo de los registros termina, el DMA carga *entero*
el descriptor apuntado (origen, destino, próximo LLI, control) y lo ejecuta. Así que el patrón es:

1. armás en RAM los descriptores de los tramos **1 en adelante** (el tramo 0 no necesita descriptor en
   RAM: vive en los registros del canal);
2. en la `GPDMA_Channel_CFG_Type` describís el **tramo 0** y ponés `DMALLI` apuntando al descriptor del
   **tramo 1**;
3. de ahí en más manda la cadena.

Error clásico: apuntar `DMALLI` a un descriptor que describe el *mismo* primer tramo. No "arranca ahí":
el DMA ya ejecutó el tramo de los registros y, al terminar, carga ese descriptor y **repite el primer
tramo**. (La excepción es el anillo de un solo descriptor, donde repetir es exactamente lo que querés;
lo vemos abajo.)

## Caso 1: scatter-gather (juntar dos buffers en uno)

Esto es lo que hace el ejemplo de link list del repo: copiar `DMASrc_Buffer1` y `DMASrc_Buffer2`
(16 words cada uno) a un único `DMADest_Buffer` de 32, en un solo arranque de DMA. El tramo 0
(Buffer1) va en los registros vía `cfg`; el tramo 1 (Buffer2) es un descriptor en RAM.

```c
#include "lpc17xx_gpdma.h"
#define DMA_SIZE 32

uint32_t DMASrc_Buffer1[DMA_SIZE/2] = { /* ... 16 words ... */ };
uint32_t DMASrc_Buffer2[DMA_SIZE/2] = { /* ... 16 words ... */ };
uint32_t DMADest_Buffer[DMA_SIZE];

GPDMA_LLI_Type lli_tramo1;   // descriptor del SEGUNDO tramo (el primero vive en los registros)

void armar(void) {
    // Tramo 1: Buffer2 -> segunda mitad del destino
    lli_tramo1.SrcAddr = (uint32_t)DMASrc_Buffer2;
    lli_tramo1.DstAddr = (uint32_t)DMADest_Buffer + (DMA_SIZE/2)*4;  // +16 words en bytes
    lli_tramo1.NextLLI = 0;                              // fin de la cadena
    lli_tramo1.Control = (DMA_SIZE/2)                    // 16 elementos
                       | (2u << 18) | (2u << 21)         // SWidth/DWidth = word
                       | (1u << 26) | (1u << 27)         // SI y DI (ambos memoria)
                       | (1u << 31);                     // I: IRQ al terminar este tramo

    // Tramo 0: lo describe cfg; GPDMA_Setup lo carga directo en los registros del canal
    GPDMA_Channel_CFG_Type cfg;
    cfg.ChannelNum   = 0;
    cfg.SrcMemAddr   = (uint32_t)DMASrc_Buffer1;   // Buffer1 -> primera mitad
    cfg.DstMemAddr   = (uint32_t)DMADest_Buffer;
    cfg.TransferSize = DMA_SIZE/2;                 // 16 elementos: SOLO el tramo 0
    cfg.TransferWidth= GPDMA_WIDTH_WORD;
    cfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
    cfg.SrcConn = 0; cfg.DstConn = 0;
    cfg.DMALLI  = (uint32_t)&lli_tramo1;           // al terminar el tramo 0 sigue acá
    GPDMA_Setup(&cfg);
    GPDMA_ChannelCmd(0, ENABLE);
}
```

El bit `I` de cada descriptor elige qué tramos disparan la IRQ de terminal count. Un detalle del
driver: `GPDMA_Setup` pone **siempre** el bit `I` en el Control del tramo 0 (está fijo en
`lpc17xx_gpdma.c`), así que acá vas a recibir **dos** IRQs: una al terminar el tramo 0 y otra al
terminar el tramo 1. La copia completa termina con la segunda: contalas en el handler, o consultá
`GPDMA_IntGetStatus(GPDMA_STAT_ENABLED_CH, 0)` (el canal se deshabilita solo al agotar la cadena). Si
armás los registros a mano (sin driver), ahí sí elegís libremente en qué tramos va `I`. Ejemplo en el
repo: [`../ejemplos/dma/lli_example.c`](../ejemplos/dma/).

### Partir transferencias > 4095

Mismo patrón: si necesitás copiar 10000 words, el tramo 0 (4095, el máximo) va en `cfg` y armás dos
descriptores más (4095 + 1810), cada uno apuntando al siguiente, el último con `NextLLI = 0`. El DMA
los recorre solo.

## Caso 2: anillo, transferencias que no terminan nunca

Acá está el truco más lindo del DMA. Si el **último** descriptor, en vez de `NextLLI = 0`, apunta **al
primero** (o un único descriptor que se apunta **a sí mismo**), la cadena no termina: el DMA recarga el
mismo tramo una y otra vez, **para siempre, sin CPU**.

```
[tramo A] --NextLLI--> [tramo B] --NextLLI--> [tramo A] --> ...   (anillo)
```

o, con un solo descriptor:

```
        +-----------+
        v           |
    [ descriptor ]--+   (NextLLI se apunta a sí mismo)
```

Esto es lo que permite reproducir una forma de onda continua por el DAC sin un solo corte.

### Generar un seno continuo por el DAC (M2P + LLI en anillo)

La receta, juntando todo:

1. **Tabla de seno en RAM**, en el formato de `DACR`: el valor de 10 bits va en los bits 15:6
   (por eso `<< 6` en la tabla).
2. **Configurar el DAC** para que pida por DMA a un ritmo fijo: activar el contador/timeout
   (`DACCTRL`, `DACCNTVAL`) y el bit `DMA_ENA`. El periférico es quien marca el pulso (una request por
   muestra); el timeout fija la frecuencia de muestreo, y con N muestras por período sale la frecuencia
   de la onda.
3. **Un descriptor que se apunta a sí mismo**, M2P, origen = tabla (incrementa), destino = `&DACR`
   (no incrementa).

```c
#include "lpc17xx_gpdma.h"

#define N_MUESTRAS 60
uint32_t tabla_seno[N_MUESTRAS];      // ya cargada en formato DACR (valor << 6)

GPDMA_LLI_Type lli_anillo;

void salida_seno_continua(void) {
    // El DAC ya debe estar configurado con DMA_ENA y su timeout (ver módulo 10).

    // Descriptor que se apunta a sí mismo -> repite para siempre
    lli_anillo.SrcAddr = (uint32_t)tabla_seno;
    lli_anillo.DstAddr = (uint32_t)&(LPC_DAC->DACR);
    lli_anillo.NextLLI = (uint32_t)&lli_anillo;          // <-- el anillo
    lli_anillo.Control = N_MUESTRAS
                       | (2u << 18) | (2u << 21)         // width word
                       | (1u << 26);                     // SI: origen incrementa; DI NO (DAC fijo)
    // sin bit I: no queremos una IRQ por vuelta (no haría falta, y satura)

    GPDMA_Init();

    GPDMA_Channel_CFG_Type cfg;
    cfg.ChannelNum   = 0;
    cfg.SrcMemAddr   = (uint32_t)tabla_seno;
    cfg.DstMemAddr   = 0;                                // destino lo pone el driver (DAC)
    cfg.TransferSize = N_MUESTRAS;
    cfg.TransferWidth= 0;                                // ignorado en M2P
    cfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    cfg.SrcConn      = 0;
    cfg.DstConn      = GPDMA_CONN_DAC;                   // el DAC dispara cada transferencia
    cfg.DMALLI       = (uint32_t)&lli_anillo;            // arranca el anillo
    GPDMA_Setup(&cfg);
    // El driver programó el tramo 0 con el ancho de su tabla interna (byte para el DAC).
    // Lo pisamos con el mismo Control del descriptor: así TODAS las vueltas son word.
    LPC_GPDMACH0->DMACCControl = lli_anillo.Control;
    GPDMA_ChannelCmd(0, ENABLE);
    // A partir de acá el DAC saca el seno solo, indefinidamente. El CPU queda libre.
}
```

Ejemplo en el repo: [`../ejemplos/dma/dac_dma_sin.c`](../ejemplos/dma/).

Por qué funciona el anillo sin IRQ ni CPU: el DAC pide una muestra a su ritmo (timeout); el DMA copia un
elemento de la tabla a `DACR`; al agotar los 60 elementos, `DMACCLLI` lo manda al mismo descriptor, que
**resetea origen y tamaño**, y vuelve a empezar desde la muestra 0. La frecuencia de la onda es
`f_request / N_MUESTRAS`.

> **Por qué pisamos el `Control` después de `Setup`:** para M2P el driver ignora `TransferWidth` y usa
> el ancho de su tabla interna (`GPDMA_LUTPerWid[]`), que para el DAC es **byte**. Con ese ancho la
> primera vuelta saldría rota dos veces: el origen avanzaría de a 1 byte (recorrería solo el primer
> cuarto de la tabla de `uint32_t`) y a `DACR` llegarían escrituras de un byte, que solo alcanzan los
> bits 7:0, y el valor del DAC vive en los bits **15:6**. Como `cfg` no tiene campo para forzar el
> ancho en M2P, la salida es escribir a mano el `DMACCControl` del canal después de `Setup` (y antes de
> habilitar) con el mismo `Control` del descriptor. De la segunda vuelta en adelante ya no importa: el
> anillo recarga siempre el descriptor.

## Caso 3: doble buffer (ping-pong) para el ADC

Para muestrear sin parar y a la vez procesar lo ya capturado, se usa el patrón **ping-pong**: dos
descriptores en anillo (A → B → A → …), cada uno con su `I` activado. Mientras el DMA llena el buffer B,
vos procesás el A; cuando salta a A, procesás el B. La IRQ de cada tramo te avisa "este buffer ya está
lleno, procesalo".

```c
GPDMA_LLI_Type lli[2];

// A -> B -> A -> ...   (anillo de dos)
lli[0].SrcAddr = (uint32_t)&LPC_ADC->ADGDR;   // origen fijo (ADC)
lli[0].DstAddr = (uint32_t)bufferA;
lli[0].NextLLI = (uint32_t)&lli[1];
lli[0].Control = N | (2u<<18) | (2u<<21) | (1u<<27) | (1u<<31);  // DI (memoria), I

lli[1].SrcAddr = (uint32_t)&LPC_ADC->ADGDR;
lli[1].DstAddr = (uint32_t)bufferB;
lli[1].NextLLI = (uint32_t)&lli[0];           // vuelve a A -> anillo
lli[1].Control = N | (2u<<18) | (2u<<21) | (1u<<27) | (1u<<31);  // DI, I
```

En la `cfg` describís el tramo A (`DstMemAddr = bufferA`, `TransferSize = N`) y ponés
`DMALLI = (uint32_t)&lli[1]` (el descriptor de **B**), porque los registros del canal ya hacen A una
vez. Si apuntaras a `lli[0]`, A se llenaría dos veces al arranque y se te desfasa la alternancia.

En el handler distinguís cuál se llenó (podés alternar una bandera, ya que la IRQ no te dice el tramo).
Así no perdés ni una muestra: siempre hay un buffer recibiendo mientras el otro se procesa.

## Errores típicos con LLI

| Error | Qué pasa | Corrección |
|-------|----------|-----------|
| LLI en una región de RAM que el DMA no alcanza | IRQ de error o cuelgue | poné los descriptores en RAM accesible por el master AHB |
| Olvidar `NextLLI` (queda basura) | el DMA salta a una dirección random | inicializá los 4 campos; el último a 0 o al primero |
| Apuntar `NextLLI` a memoria no alineada a 4 | los bits 1:0 del LLI son reservados y deben ir en 0 | mantené los descriptores alineados a word |
| `DMALLI` apuntando al descriptor del tramo 0 | el primer tramo se ejecuta dos veces | `DMALLI` apunta al descriptor **siguiente** al que describe `cfg` |
| Tramo 0 con otro ancho que el resto (M2P/P2M) | la primera pasada usa el ancho de la LUT del driver | pisá `DMACCControl` tras `Setup` con el `Control` del descriptor |
| Esperar que un anillo "termine" | nunca dispara fin | para frenarlo: Halt el canal, esperar Active=0, deshabilitar |
| Modificar la tabla mientras el anillo corre | glitch en la onda | actualizá en un buffer libre y recién ahí cambiá el puntero |

## Resumen mental

- **Los registros del canal son el descriptor cero** (lo carga `Setup` desde `cfg`); `DMALLI` apunta
  al que sigue.
- **LLI simple (cadena que termina en 0):** scatter-gather, o partir transferencias largas (>4095).
- **LLI en anillo (último apunta al primero, o uno a sí mismo):** transferencias infinitas: onda
  continua al DAC, doble buffer de ADC.
- El `Control` por descriptor te deja variar tamaño/ancho/incrementos/IRQ tramo a tramo.
- El bit `I` controla en qué tramos te interrumpe: ponelo donde necesites procesar; sacalo donde no.

---

**Anterior:** [02 - DMA con el driver](./02-dma-con-driver.md) ·
**Siguiente módulo:** [12 - Debug](../12_debug/)
