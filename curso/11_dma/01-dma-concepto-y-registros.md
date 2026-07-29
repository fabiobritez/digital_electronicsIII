# DMA: concepto y registros

## El problema que resuelve

Mover datos cuesta tiempo de CPU. Si querés copiar 1000 muestras del ADC a un buffer, lo "normal" es
un bucle donde el CPU lee el registro del ADC y lo escribe en RAM, mil veces. Durante todo ese rato el
CPU **no hace otra cosa**: cada palabra que se mueve es una instrucción `LDR` y una `STR` que ocupan el
núcleo. Y si los datos llegan rápido (audio a 48 kHz, muestreo de ADC a cientos de kHz), el CPU no solo
queda ocupado: corre el riesgo de **perder datos** si tarda en atender una interrupción.

El **GPDMA** (General Purpose Direct Memory Access) es un bloque de hardware separado, con su propio
acceso al bus AHB, que hace esas copias **por su cuenta**, en paralelo con el CPU. Le decís "copiá N
datos de la dirección A a la B, con tal ancho, y avisame cuando termines", y el CPU queda libre para
otra cosa mientras el DMA trabaja dato a dato. Tiene **8 canales** independientes.

### La intuición que el datasheet no te da

Pensá en el GPDMA como un **segundo "mini-CPU" que solo sabe copiar**. No ejecuta tu programa: ejecuta
una receta muy chiquita ("origen, destino, cuántos, de a cuánto") que vos le cargás en unos registros.
Comparte el bus AHB con el Cortex-M3, así que cuando el DMA está moviendo datos, el CPU puede tener que
esperar un ciclo si los dos quieren el mismo bus al mismo tiempo (el árbitro del bus decide). No sale
gratis: descarga al CPU de *hacer* la copia, pero el ancho de banda del bus es compartido. Para
las transferencias que importan (ráfagas a un periférico, bloques grandes) esa pequeña competencia es
despreciable comparada con tener al CPU dando vueltas en un `for`.

## Los 8 canales y la prioridad

El GPDMA tiene 8 canales (`LPC_GPDMACH0` … `LPC_GPDMACH7`). Cada uno es una "copiadora" independiente
con su propio juego de registros: podés tener, por ejemplo, el ADC llenando un buffer en el canal 0 y a
la vez una tabla saliendo al DAC en el canal 1.

La prioridad es **fija por número**: **el canal 0 es el de mayor prioridad** y el 7 el de menor. Si dos
canales piden el bus al mismo tiempo, gana el de número más bajo. Por eso, al periférico más sensible al
tiempo (el que no puede esperar, p. ej. una entrada que se desborda) conviene asignarle un canal bajo.

Dos detalles del manual sobre esto:

- El cambio no es instantáneo: si un canal de mayor prioridad pide mientras otro está transfiriendo,
  el controlador termina primero lo que ya tenía delegado (hasta 4 words, el tamaño del FIFO interno de
  cada canal) y recién ahí atiende al de mayor prioridad.
- Las transferencias **M2M van en un canal de baja prioridad** (el manual lo recomienda explícitamente).
  Como la memoria siempre "está lista", un M2M nunca suelta el bus por su cuenta: en un canal alto
  bloquea a los demás canales (y a otros masters del AHB) hasta terminar.

## Tipos de transferencia y flow control

Hay cuatro tipos según de dónde a dónde van los datos:

| Tipo (macro CMSIS) | De → a | Ejemplo típico |
|--------------------|--------|----------------|
| **M2M** `GPDMA_TRANSFERTYPE_M2M` | memoria → memoria | copiar un bloque grande de RAM, o de Flash a RAM |
| **M2P** `GPDMA_TRANSFERTYPE_M2P` | memoria → periférico | una tabla de seno al DAC, un buffer a la UART/SSP |
| **P2M** `GPDMA_TRANSFERTYPE_P2M` | periférico → memoria | llenar un buffer con muestras del ADC o de la SSP |
| **P2P** `GPDMA_TRANSFERTYPE_P2P` | periférico → periférico | poco común (p. ej. ADC → DAC directo) |

En M2M la copia es **continua**: el DMA mueve los N datos lo más rápido que puede, porque la memoria
siempre "está lista". Pero cuando hay un periférico de por medio (M2P, P2M, P2P) no podés ir a full: el
DAC tarda en convertir, la UART tarda en transmitir un byte, el ADC tarda en muestrear. Hace falta que
**alguien marque el ritmo**, y ahí entra el concepto de *flow control* y las *DMA request*.

### DMA request: el periférico marca el pulso

Un periférico que trabaja con DMA tiene una línea de pedido (*DMA request*). Cada vez que está listo
para entregar o recibir un dato, **levanta la request** y el DMA hace una transferencia (un dato o una
ráfaga) en respuesta. Ejemplos:

- El ADC, al terminar una conversión, pide "vení a buscar el resultado" → el DMA copia `ADGDR` al
  buffer. Una request por muestra.
- El DAC, según un timer interno (`DACCTRL`/timeout), pide "dame el próximo valor" → el DMA copia el
  siguiente elemento de la tabla a `DACR`. Una request por muestra de salida.
- La UART pide cuando su FIFO de transmisión tiene lugar (Tx) o cuando recibió datos (Rx).

Sin esa request, en M2P/P2M **el DMA no mueve nada**, aunque el canal esté habilitado. Por eso uno de
los errores más comunes es olvidar habilitar el DMA *en el periférico* (no alcanza con configurar el
GPDMA): hay que prender el bit de "modo DMA" del ADC (`ADCR`), del DAC (`DACCTRL.DMA_ENA`), de la UART,
etc. El GPDMA es el chofer; el periférico es quien dice cuándo arrancar.

### Flow control: ¿quién decide cuándo termina?

El *flow controller* es quien sabe la cantidad de datos y decide el fin de la transferencia. En este
curso usamos siempre **el DMA como flow controller** (las cuatro macros de arriba terminan en
"DMA control"). Eso significa: vos le decís cuántos datos (`TransferSize`), el DMA los cuenta y, al
llegar a cero, frena y dispara la interrupción de *terminal count*.

El encoding del campo `TransferType` deja lugar para otras combinaciones (100–111) en las que el flow
controller sería el periférico (para cuando no sabés de antemano cuántos datos hay), pero en el
LPC176x/5x **no existen**: el manual las marca como reservadas ("do not use") y fija que **el flow
controller es siempre el DMA**. Quedate con la idea: **el largo lo fijás vos y el GPDMA cuenta**.

## Mapa de registros

El GPDMA tiene **registros globales** (uno para todo el bloque, `LPC_GPDMA`, base `0x5000_4000`) y un
**bloque de registros por cada canal** (`LPC_GPDMACH0` en `0x5000_4100`, y de a `0x20` por canal). Los
nombres de abajo son los **exactos** del header `LPC17xx.h` (CMSIS). Un detalle del bus: estos
registros solo aceptan accesos de **32 bits** (un acceso de 8 o 16 bits genera una excepción).

### Registros globales (`LPC_GPDMA->...`)

| Registro | Tipo | Función |
|----------|------|---------|
| `DMACIntStat` | R | resumen: qué canal tiene una interrupción pendiente (TC **o** error). Es el OR de los dos de abajo, ya enmascarado |
| `DMACIntTCStat` | R | qué canal terminó (terminal count) y tiene la IRQ de fin pendiente |
| `DMACIntTCClear` | W | escribir un 1 en el bit del canal **limpia** su IRQ de terminal count |
| `DMACIntErrStat` | R | qué canal tuvo un **error** (p. ej. dirección inaccesible) |
| `DMACIntErrClr` | W | limpiar la IRQ de error del canal |
| `DMACRawIntTCStat` | R | igual que TCStat pero **sin** enmascarar (estado crudo) |
| `DMACRawIntErrStat` | R | error crudo, sin enmascarar |
| `DMACEnbldChns` | R | qué canales están actualmente habilitados/activos |
| `DMACSoftBReq` | RW | disparar por software una request de **ráfaga** (burst) |
| `DMACSoftSReq` | RW | disparar por software una request **simple** (single) |
| `DMACSoftLBReq` | RW | request de software de **última ráfaga** |
| `DMACSoftLSReq` | RW | request de software de **último single** |
| `DMACConfig` | RW | habilita el controlador entero. Bit `E` (enable) y bit `M` (endianness del master AHB) |
| `DMACSync` | RW | controla la lógica de sincronización de las DMA request (anti-metaestabilidad). Resetea en 0 = sync **habilitada** por defecto; poner un bit en 1 la **deshabilita** para ese grupo. Casi nunca se toca |

Notas que conviene fijar:

- En todos los registros de estado/clear, **cada bit es un canal** (bit 0 = canal 0, …, bit 7 = canal
  7). Por eso en el código verás `(1 << canal)`.
- Ojo: en los cuatro `SoftXReq` y en `DMACSync` cada bit **no** es un canal sino una **request line**
  (0..15, las de la tabla de la página 2). Además, los periféricos de este chip no usan las request
  "last" (`SoftLBReq`/`SoftLSReq` quedan de adorno).
- `DMACConfig`: hay que poner `E = 1` (`GPDMA_DMACConfig_E`) **una vez** para encender todo el bloque,
  antes de que cualquier canal funcione. El bit `M` (`GPDMA_DMACConfig_M`) selecciona big/little endian
  del master; lo dejamos en 0 (little endian, que es lo que usa el Cortex-M3).
- Los `SoftBReq`/`SoftSReq` permiten "fingir" la request de un periférico desde software (el manual
  recomienda no mezclarlas con las request de hardware de la misma línea). Sirven para debug o para
  mover datos con ritmo controlado por software; en uso normal no se tocan.

### Registros por canal (`LPC_GPDMACH0->...`, idem CH1…CH7)

Acá está el detalle que faltaba. Ojo con los nombres: en el header tienen **doble C**
(`DMACC...`), porque es "DMA Channel C-register".

| Registro | Función |
|----------|---------|
| `DMACCSrcAddr` | dirección de **origen** |
| `DMACCDestAddr` | dirección de **destino** |
| `DMACCLLI` | puntero al **siguiente descriptor** (Linked List Item), alineado a 4 (los bits 1:0 son reservados y van en 0). 0 = no hay más, transferencia simple |
| `DMACCControl` | el **qué** y el **cómo**: cuántos datos, anchos, burst sizes, incrementos, IRQ de fin |
| `DMACCConfig` | el **con quién**: habilita el canal, elige periféricos origen/destino, tipo de transferencia, máscaras de IRQ |

#### El registro `DMACCControl` (el corazón de la transferencia)

Define qué se mueve y de a cuánto. Campos (con la macro CMSIS que los arma):

| Bits | Campo | Macro | Significado |
|------|-------|-------|-------------|
| 11:0 | TransferSize | `GPDMA_DMACCxControl_TransferSize(n)` | cuántos **elementos** copiar. **Máximo 0xFFF = 4095** (campo de 12 bits) |
| 14:12 | SBSize | `GPDMA_DMACCxControl_SBSize(n)` | tamaño de la **ráfaga de origen** (1,4,8,…,256) |
| 17:15 | DBSize | `GPDMA_DMACCxControl_DBSize(n)` | tamaño de la ráfaga de destino |
| 20:18 | SWidth | `GPDMA_DMACCxControl_SWidth(n)` | ancho del dato de origen: byte / half-word / word |
| 23:21 | DWidth | `GPDMA_DMACCxControl_DWidth(n)` | ancho del dato de destino |
| 26 | SI | `GPDMA_DMACCxControl_SI` | **Source Increment**: la dirección de origen avanza tras cada dato |
| 27 | DI | `GPDMA_DMACCxControl_DI` | **Dest Increment**: la dirección de destino avanza tras cada dato |
| 28-30 | Prot1/2/3 | `GPDMA_DMACCxControl_Prot1..3` | flags de protección AHB (user/priv, bufferable, cacheable). Casi siempre 0 |
| 31 | I | `GPDMA_DMACCxControl_I` | habilita la **interrupción de terminal count** de este descriptor |

Los puntos finos:

- **TransferSize cuenta elementos, no bytes.** Si copiás 256 words con `SWidth = WORD`, ponés
  `TransferSize = 256`, no 1024. El "elemento" tiene el tamaño de `SWidth`.
- **El límite de 4095 es real y muerde.** Si necesitás copiar más de 4095 elementos en una sola tirada,
  no entra: hay que partirlo en varios descriptores encadenados con LLI (lo vemos en la página 3).
- **SI / DI: el detalle más importante.** Indican si la dirección avanza después de cada dato:
  - Copiar un buffer a otro (M2M): **ambas** incrementan.
  - Volcar una tabla al `DACR` (M2P): el origen incrementa (recorre la tabla), el **destino NO** (siempre
    el mismo registro DAC).
  - Leer el ADC a un buffer (P2M): el origen **NO** incrementa (siempre el mismo `ADGDR`), el destino sí.
  - Regla mnemónica: **la punta "memoria" incrementa; la punta "periférico" no.** Un periférico es un
    registro fijo; si lo incrementaras, escribirías/leerías registros de al lado (desastre).
- **Burst size (SBSize/DBSize) y ancho (SWidth/DWidth) y el throughput.** El ancho dice cuántos bytes
  por elemento; el burst dice cuántos elementos mueve el DMA de un tirón antes de soltar el bus. Burst
  más grande = menos arbitraje = más throughput, pero el burst tiene que tener sentido para el
  periférico: un FIFO de 16 entradas no tolera un burst de 256. Para periféricos, el driver CMSIS elige
  el burst y el ancho "óptimos" de una tabla interna (p. ej. SSP usa burst 4, el DAC burst 1). Para M2M
  el driver usa burst 32. Regla: si el periférico tiene FIFO, un burst ≈ medio FIFO va bien; si entrega
  de a un dato (DAC, UART), burst 1.
- **Alineación.** Las direcciones de origen y destino **deben** estar alineadas a su ancho (lo exige
  el manual): con `WORD`, a 4 bytes; con half-word, a 2.

#### El registro `DMACCConfig` (a quién escucha el canal)

Define el contexto del canal: tipo de transferencia, qué periféricos lo disparan, y el estado.

| Bits | Campo | Macro | Significado |
|------|-------|-------|-------------|
| 0 | E | `GPDMA_DMACCxConfig_E` | **enable** del canal. Ponerlo en 1 lo arranca |
| 5:1 | SrcPeripheral | `GPDMA_DMACCxConfig_SrcPeripheral(n)` | número de DMA request line de la fuente (si es periférico) |
| 10:6 | DestPeripheral | `GPDMA_DMACCxConfig_DestPeripheral(n)` | número de request line del destino (si es periférico) |
| 13:11 | TransferType | `GPDMA_DMACCxConfig_TransferType(n)` | M2M / M2P / P2M / P2P |
| 14 | IE | `GPDMA_DMACCxConfig_IE` | máscara de la **interrupción de error** (1 = la deja pasar) |
| 15 | ITC | `GPDMA_DMACCxConfig_ITC` | máscara de la IRQ de **terminal count** (1 = la deja pasar) |
| 16 | L | `GPDMA_DMACCxConfig_L` | Lock: marca las transferencias como "lockeadas" en el bus |
| 17 | A | `GPDMA_DMACCxConfig_A` | **Active** (solo lectura): hay datos en el FIFO del canal (4 words) sin volcar |
| 18 | H | `GPDMA_DMACCxConfig_H` | **Halt**: 1 = ignorar nuevas request (frena ordenadamente sin perder lo que ya tomó) |

Puntos finos:

- **Hay dos niveles de enable.** El global (`DMACConfig.E`) prende el bloque; el del canal
  (`DMACCConfig.E`) prende ese canal. Faltando cualquiera, no se mueve nada.
- **`SrcPeripheral`/`DestPeripheral` son las request lines, no "el periférico" en abstracto.** Cada una
  corresponde a una fuente de DMA request del chip (ver la tabla de la página 2). En M2M no se usan (no
  hay periférico). El bit `I` de `Control` y el `ITC` de `Config` tienen que estar **ambos** en 1 para
  que la IRQ de fin llegue al NVIC.
- **Active y Halt** sirven para parar un canal con elegancia: poné `H = 1`, esperá a que `A = 0` (FIFO
  vaciado) y recién ahí limpiás `E`. Si bajás `E` de golpe podés perder los datos que el canal ya
  agarró pero no volcó.

## Mecánica de un M2M a nivel registro

Una copia de 256 *words* (1024 bytes) de `origen` a `destino`, sin driver, queda así. Notá los nombres
exactos del header:

```c
#include "LPC17xx.h"

#define N 256
uint32_t origen[N], destino[N];

void dma_m2m_crudo(void) {
    LPC_SC->PCONP |= (1u << 29);              // encender el GPDMA (PCONP bit 29 = PCGPDMA)
    LPC_GPDMA->DMACConfig = (1u << 0);        // habilitar el controlador (bit E), little endian
    while (!(LPC_GPDMA->DMACConfig & 1u));    // esperar a que quede habilitado

    LPC_GPDMA->DMACIntTCClear = (1u << 0);    // limpiar flags viejos del canal 0
    LPC_GPDMA->DMACIntErrClr  = (1u << 0);

    LPC_GPDMACH0->DMACCSrcAddr  = (uint32_t)origen;
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)destino;
    LPC_GPDMACH0->DMACCLLI      = 0;          // transferencia simple, sin lista enlazada

    LPC_GPDMACH0->DMACCControl  =
          (N & 0xFFF)        // TransferSize = 256 elementos (cabe en 12 bits)
        | (4u << 12)         // SBSize  = burst 32  (índice 4)
        | (4u << 15)         // DBSize  = burst 32
        | (2u << 18)         // SWidth  = word (4 bytes)
        | (2u << 21)         // DWidth  = word
        | (1u << 26)         // SI: incrementar origen
        | (1u << 27)         // DI: incrementar destino
        | (1u << 31);        // I: IRQ de terminal count habilitada en este descriptor

    LPC_GPDMACH0->DMACCConfig  =
          (1u << 0)          // E: habilitar el canal (arranca ya, M2M no espera request)
        | (0u << 11)         // TransferType = M2M
        | (1u << 14)         // IE: dejar pasar IRQ de error
        | (1u << 15);        // ITC: dejar pasar IRQ de terminal count
    // El DMA copia las 256 words solo; al terminar interrumpe (DMA_IRQHandler).
}
```

Como ves, es bastante más enredado que un GPIO: hay que armar a mano dos registros de bits empaquetados.
Por eso **el DMA es de los pocos periféricos donde casi siempre se usa el driver**, incluso aprendiendo:
arma todos estos bits a partir de una struct legible y elige burst/ancho razonables por vos. Lo vemos en
la [próxima página](./02-dma-con-driver.md).

## El ritual de arranque (resumido)

1. `PCONP |= (1<<29)`: encender el GPDMA. **No necesita un PCLK propio**: corre con el reloj del AHB,
   no aparece en `PCLKSEL0/1`. Solo el bit de `PCONP`.
2. `DMACConfig.E = 1`: habilitar el controlador.
3. Limpiar flags viejos del canal (`DMACIntTCClear`/`DMACIntErrClr`): una operación anterior pudo
   dejar interrupciones colgadas.
4. (Si hay periférico) habilitar el modo DMA **en el periférico** y, si aplica, el `DMAREQSEL`.
5. Cargar `DMACCSrcAddr`, `DMACCDestAddr`, `DMACCLLI`, `DMACCControl`, `DMACCConfig` del canal.
6. `DMACCConfig.E = 1`: arrancar el canal.
7. Atender la IRQ de fin/error en `DMA_IRQHandler` y limpiar con `DMACIntTCClear`/`DMACIntErrClr`.

## Caso de borde clave: el límite de 4095

`TransferSize` es de 12 bits. Si tu transferencia es de más de 4095 elementos, **no entra en un solo
descriptor**. La salida no es un `for`: es **encadenar descriptores con LLI** (cada uno mueve hasta 4095
y apunta al siguiente). Y para señales que no terminan nunca (un seno continuo al DAC), el truco también
son los LLI, pero en **anillo**. Eso tiene su propia página:
[03 - Linked lists y transferencias circulares](./03-linked-lists.md).

---

**Módulo:** [DMA](./README.md) · **Siguiente:** [02 - DMA con el driver](./02-dma-con-driver.md)
