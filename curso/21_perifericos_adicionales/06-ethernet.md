# Ethernet: el EMAC y los descriptores (avanzado)

El LPC1769 trae un **controlador Ethernet** (EMAC, *Ethernet Media Access Controller*) de **10/100
Mbit/s**: el periférico que te deja conectar el micro a una **red** y, con software encima, hablar
TCP/IP (servir una página de configuración, postear datos a un servidor, integrarte a un sistema IoT).
Es, de lejos, el periférico más **complejo** del chip. No por la cantidad de registros (no son tantos),
sino porque por sí solo no "hace internet": maneja la **capa de enlace** y nada más; el resto es una
**pila de protocolos** por software que va encima (eso lo vemos en la [página siguiente](./07-ethernet-tcpip.md)).

Capítulo 10 del manual. PCONP bit **30** (`PCENET`). Tratalo como **proyecto grande**, no como un
periférico de una tarde.

Esta página es la mitad **hardware**: qué hay adentro del EMAC, cómo mueve tramas por DMA y, sobre todo,
cómo funcionan los **descriptores** (que es el concepto que de verdad importa entender). La página
siguiente cierra con el **PHY**, la **pila TCP/IP** (lwIP/uIP) y cómo se integra todo.

## Las dos mitades del problema

1. **El hardware (lo que hace el EMAC).** Arma y desarma las **tramas Ethernet** (direcciones MAC,
   EtherType, relleno, CRC), las transmite y recibe por el cable, y mueve los datos por **DMA** entre la
   RAM y la red usando **descriptores** (estructuras en memoria que dicen "este buffer es una trama").
   Esto lo hace el silicio; vos lo configurás una vez y después no tocás cada byte.

2. **El software (la pila TCP/IP).** El EMAC te entrega/recibe **tramas crudas**. Convertir eso en "abrir
   una conexión TCP al puerto 80 y mandar HTTP" requiere ARP, IP, ICMP, UDP, TCP y arriba DHCP/HTTP/MQTT.
   Es **mucho** código y no lo escribís vos: se reusa una pila ya hecha (lwIP/uIP). Lo desarrollamos en la
   [página siguiente](./07-ethernet-tcpip.md).

Esta página vive en la mitad 1.

## Arquitectura del EMAC

Adentro, el EMAC son cuatro bloques que conviene distinguir porque sus registros están agrupados así:

- **El MAC propiamente dicho.** Implementa IEEE 802.3: preámbulo, detección de portadora, inter-packet
  gap, generación/chequeo de CRC, relleno de tramas cortas, half/full duplex, control de flujo (PAUSE).
  Sus registros son `MAC1`, `MAC2`, `IPGT`, `IPGR`, `CLRT`, `MAXF`, `SUPP`, `TEST` y las direcciones de
  estación `SA0/SA1/SA2`.
- **La gestión MII (MIIM).** El canalcito serie por el que el EMAC le lee y escribe registros al **PHY
  externo** (autonegociación, link, velocidad). Registros `MCFG`, `MCMD`, `MADR`, `MWTD`, `MRDD`, `MIND`.
- **El control / DMA.** El motor que recorre los descriptores en RAM y mueve tramas solo. Registros
  `Command`, `Status`, los punteros `RxDescriptor`/`RxStatus`/`TxDescriptor`/`TxStatus`, los `*Number` y
  los índices productor/consumidor `RxProduceIndex`/`RxConsumeIndex`/`TxProduceIndex`/`TxConsumeIndex`.
- **El filtro de recepción.** Decide qué tramas se quedan y cuáles se descartan antes de tocar la RAM
  (unicast/broadcast/multicast, hash, promiscuo, perfect match). Registros `RxFilterCtrl`,
  `HashFilterL/H`, más los de Wake-on-LAN.

Y aparte, el bloque de **control de módulo**: interrupciones (`IntStatus`/`IntEnable`/`IntClear`/`IntSet`),
`PowerDown` y `Module_ID`.

En CMSIS todo esto es la struct `LPC_EMAC_TypeDef` (en `LPC17xx.h`), y se accede como `LPC_EMAC->MAC1`,
`LPC_EMAC->Command`, etc. Los nombres de los campos son **exactamente** los de arriba.

```c
// Esqueleto del bloque, tal cual está en LPC17xx.h
typedef struct {
  __IO uint32_t MAC1, MAC2, IPGT, IPGR, CLRT, MAXF, SUPP, TEST;  // MAC
  __IO uint32_t MCFG, MCMD, MADR;  __O uint32_t MWTD;  __I uint32_t MRDD, MIND;  // MIIM
       uint32_t RESERVED0[2];
  __IO uint32_t SA0, SA1, SA2;     uint32_t RESERVED1[45];
  __IO uint32_t Command;  __I uint32_t Status;                   // Control / DMA
  __IO uint32_t RxDescriptor, RxStatus, RxDescriptorNumber;
  __I  uint32_t RxProduceIndex;  __IO uint32_t RxConsumeIndex;
  __IO uint32_t TxDescriptor, TxStatus, TxDescriptorNumber, TxProduceIndex;
  __I  uint32_t TxConsumeIndex;   uint32_t RESERVED2[10];
  __I  uint32_t TSV0, TSV1, RSV;  uint32_t RESERVED3[3];
  __IO uint32_t FlowControlCounter;  __I uint32_t FlowControlStatus;  uint32_t RESERVED4[34];
  __IO uint32_t RxFilterCtrl, RxFilterWoLStatus, RxFilterWoLClear;  uint32_t RESERVED5;
  __IO uint32_t HashFilterL, HashFilterH;  uint32_t RESERVED6[882];
  __I  uint32_t IntStatus;  __IO uint32_t IntEnable;  __O uint32_t IntClear, IntSet;
       uint32_t RESERVED7;  __IO uint32_t PowerDown;  uint32_t RESERVED8;  __IO uint32_t Module_ID;
} LPC_EMAC_TypeDef;
```

## La trama Ethernet, en una línea

Lo que viaja por el cable es una **trama**:

```
[ MAC destino 6B ][ MAC origen 6B ][ EtherType/Len 2B ][ payload 46..1500B ][ FCS/CRC 4B ]
```

- **MAC destino / origen**: 6 bytes cada una. La de origen es la tuya (`SA0/SA1/SA2`). La de destino
  decide a quién va: unicast (un equipo), broadcast (`FF:FF:FF:FF:FF:FF`, a todos) o multicast (un grupo).
- **EtherType**: qué hay arriba. `0x0800` = IPv4, `0x0806` = ARP, `0x86DD` = IPv6. (Si el valor es ≤ 1500
  se interpreta como longitud, formato 802.3 viejo.)
- **Payload**: 46 a 1500 bytes. Si tu dato es más corto, el MAC lo **rellena** (padding) hasta 46
  (eso lo hace por hardware si activás `EMAC_MAC2_PAD_EN`).
- **FCS**: el CRC-32 de toda la trama. El MAC lo **calcula y agrega** él solo si activás
  `EMAC_MAC2_CRC_EN` (y por descriptor con `EMAC_TCTRL_CRC`), y lo **chequea** al recibir.

Intuición que el datasheet no te grita: vos casi nunca tocás MAC origen/destino, EtherType ni CRC a mano.
La pila TCP/IP (en TX) arma esos campos, y el MAC pone el CRC. En RX, el MAC ya validó el CRC y filtró por
MAC destino antes de avisarte. Vos trabajás con el **payload**.

## El corazón: descriptores y DMA en RAM

Acá está la idea central, y es la misma que ya viste en otros periféricos con DMA (módulo 11), pero con
una vuelta de tuerca propia del EMAC: hay **dos arrays por dirección** (descriptor + status) y un modelo
**productor/consumidor** con índices en hardware.

### Cuatro arrays en RAM

El EMAC no tiene una FIFO grande adentro: usa **buffers en tu RAM**, descritos por **descriptores**. Para
recepción hay dos arrays paralelos, y para transmisión otros dos:

```c
// Recepción
typedef struct { uint32_t Packet; uint32_t Ctrl;    } RX_Desc;  // tú lo escribís
typedef struct { uint32_t Info;   uint32_t HashCRC; } RX_Stat;  // el EMAC lo escribe

// Transmisión
typedef struct { uint32_t Packet; uint32_t Ctrl; } TX_Desc;     // tú lo escribís
typedef struct { uint32_t Info;                  } TX_Stat;     // el EMAC lo escribe
```

- **`*_Desc` (descriptor)**: lo escribís **vos**. `Packet` es el puntero al buffer de datos (la trama);
  `Ctrl` lleva el tamaño y unos flags (ver abajo).
- **`*_Stat` (status)**: lo escribe **el EMAC** cuando termina con ese descriptor. Te dice cómo le fue:
  tamaño recibido, errores, si fue broadcast/multicast, etc. En RX trae además el `HashCRC` que usó el
  filtro.

Cada array es un **anillo** (ring buffer): cuando llegás al último entrás de nuevo al primero. El driver
CMSIS los declara estáticos y alineados:

```c
// de lpc17xx_emac.c: N fragmentos de 1536 bytes c/u
#define EMAC_NUM_RX_FRAG  4     // 4 * 1536 = 6.0 kB de RX
#define EMAC_NUM_TX_FRAG  3     // 3 * 1536 = 4.6 kB de TX
#define EMAC_ETH_MAX_FLEN 1536

static RX_Desc Rx_Desc[EMAC_NUM_RX_FRAG];
static RX_Stat Rx_Stat[EMAC_NUM_RX_FRAG];
static TX_Desc Tx_Desc[EMAC_NUM_TX_FRAG];
static TX_Stat Tx_Stat[EMAC_NUM_TX_FRAG];
static uint32_t rx_buf[EMAC_NUM_RX_FRAG][EMAC_ETH_MAX_FLEN>>2];   // los buffers reales
static uint32_t tx_buf[EMAC_NUM_TX_FRAG][EMAC_ETH_MAX_FLEN>>2];
```

### Dónde le decís al EMAC dónde están

Después de armar los arrays, le pasás las **bases** y el **número de descriptores** por registros. Ojo con
el detalle: `*DescriptorNumber` es **N - 1**, no N (es "índice del último", no "cantidad").

```c
// init de recepción (rx_descr_init en el driver)
for (i = 0; i < EMAC_NUM_RX_FRAG; i++) {
    Rx_Desc[i].Packet = (uint32_t)&rx_buf[i];
    Rx_Desc[i].Ctrl   = EMAC_RCTRL_INT | (EMAC_ETH_MAX_FLEN - 1);  // pedí IRQ + tamaño-1
    Rx_Stat[i].Info = 0;  Rx_Stat[i].HashCRC = 0;
}
LPC_EMAC->RxDescriptor       = (uint32_t)&Rx_Desc[0];   // base del array de descriptores
LPC_EMAC->RxStatus           = (uint32_t)&Rx_Stat[0];   // base del array de status
LPC_EMAC->RxDescriptorNumber = EMAC_NUM_RX_FRAG - 1;    // ¡N - 1!
LPC_EMAC->RxConsumeIndex     = 0;
```

Detalle que muerde a todo el mundo: el campo de **tamaño** en `Ctrl` se programa como **tamaño - 1**
(`EMAC_RCTRL_SIZE(n)` toma `n & 0x7FF`, y el código pasa `EMAC_ETH_MAX_FLEN - 1`). Lo mismo en TX: el
tamaño se carga como `len - 1`. Si te olvidás del "menos uno", todo se corre un byte y te volvés loco.

### El modelo productor / consumidor

Acá está la elegancia. Cada anillo tiene **dos índices**: uno lo mueve el hardware, el otro el software.
Forman una cola sin locks.

**Recepción** (el EMAC produce, vos consumís):

- `RxProduceIndex` (solo lectura para vos): lo **avanza el EMAC** cada vez que deposita una trama nueva en
  el siguiente buffer.
- `RxConsumeIndex` (lo escribís vos): lo **avanzás vos** cuando terminaste de leer esa trama y liberás el
  buffer.
- Hay datos para procesar mientras `RxConsumeIndex != RxProduceIndex`. Cuando son iguales, el anillo está
  vacío.

**Transmisión** (vos producís, el EMAC consume), los roles se invierten:

- `TxProduceIndex` (lo escribís vos): lo avanzás cuando cargaste una trama lista para mandar.
- `TxConsumeIndex` (solo lectura): lo avanza el EMAC cuando terminó de transmitirla.
- El anillo está lleno (no podés cargar más) cuando avanzar `TxProduceIndex` lo haría chocar contra
  `TxConsumeIndex`.

```
   RX:  [b0][b1][b2][b3]          TX:  [b0][b1][b2]
         ^Consume  ^Produce             ^Consume ^Produce
        (vos leés) (EMAC llena)       (EMAC manda)(vos cargás)
```

El driver lo envuelve en cuatro helpers que es lo que usás en la práctica:

```c
Bool EMAC_CheckReceiveIndex(void);    // ¿hay trama nueva? (Consume != Produce)
void EMAC_UpdateRxConsumeIndex(void); // listo, liberá este buffer de RX
Bool EMAC_CheckTransmitIndex(void);   // ¿hay lugar para mandar?
void EMAC_UpdateTxProduceIndex(void); // listo, mandá lo que cargué
```

La gran ventaja de este esquema: el DMA del EMAC y la CPU trabajan **en paralelo** sin pisarse. Mientras
la CPU procesa la trama del buffer 0, el EMAC ya está llenando el buffer 1. No hay copia extra ni espera
activa: la red llega "sola" a la RAM y vos vas a buscarla cuando podés.

### Leer y escribir el payload

Para no manejar índices a mano, el driver da dos funciones que copian entre tu buffer y el buffer del
descriptor actual:

```c
// TX: copia tu trama al buffer en TxProduceIndex y marca el descriptor como LAST + IRQ
EMAC_PACKETBUF_Type pkt = { .pbDataBuf = (uint32_t*)txbuf, .ulDataLen = len };
EMAC_WritePacketBuffer(&pkt);   // setea Tx_Desc[idx].Ctrl = (len-1) | TCTRL_INT | TCTRL_LAST
EMAC_UpdateTxProduceIndex();    // ¡ahora sí, mandala!

// RX: copia la trama del buffer en RxConsumeIndex a tu buffer
EMAC_PACKETBUF_Type rx = { .pbDataBuf = (uint32_t*)rxbuf, .ulDataLen = EMAC_GetReceiveDataSize()-3 };
EMAC_ReadPacketBuffer(&rx);
EMAC_UpdateRxConsumeIndex();    // liberá el buffer para que el EMAC lo reuse
```

Detalle fino del tamaño en RX: `EMAC_GetReceiveDataSize()` devuelve el tamaño en formato "menos uno"
(de `Info & EMAC_RINFO_SIZE`), e incluye los 4 bytes de CRC. Por eso en los ejemplos verás restas tipo
`size - 3` para quedarte con el payload sin FCS. Es un detalle del hardware, no un error.

### Flags de los descriptores (lo que de verdad usás)

En `Ctrl` del descriptor TX (`EMAC_TCTRL_*`):

| Flag | Qué hace |
|------|----------|
| `EMAC_TCTRL_SIZE` (bits 0..10) | tamaño - 1 del buffer |
| `EMAC_TCTRL_LAST` | último fragmento de la trama (casi siempre lo querés) |
| `EMAC_TCTRL_INT` | generá interrupción TxDone al terminar este descriptor |
| `EMAC_TCTRL_CRC` | que el hardware agregue el CRC/FCS |
| `EMAC_TCTRL_PAD` | rellená tramas cortas a 64 bytes |
| `EMAC_TCTRL_OVERRIDE` | usá estos flags en vez de los defaults de MAC2 |

En `Ctrl` del descriptor RX: `EMAC_RCTRL_SIZE(n)` (tamaño-1 del buffer) y `EMAC_RCTRL_INT` (pedir IRQ).

Y en el **status** que escribe el EMAC, lo más útil es el `Info` de RX (`EMAC_RINFO_*`): `SIZE` (tamaño),
`LAST_FLAG` (última frag.), `ERR` (OR de todos los errores), `CRC_ERR`, `ALIGN_ERR`, `LEN_ERR`,
`RANGE_ERR`, `SYM_ERR`, `OVERRUN`, `NO_DESCR`, `FAIL_FILT`, más `BCAST`/`MCAST`. El driver agrupa los
errores **reales** en `EMAC_RINFO_ERR_MASK` (que es `FAIL_FILT | CRC_ERR | SYM_ERR | LEN_ERR | ALIGN_ERR |
OVERRUN`) para chequearlos de un saque. Ojo: `RANGE_ERR` queda **afuera** de esa máscara a propósito (ver
abajo, no es un error de verdad) y `NO_DESCR` tampoco está, porque no es un defecto de la trama sino aviso
de que te quedaste sin descriptores libres.

## El filtrado: qué tramas te quedan

Antes de molestar a la CPU, el EMAC filtra por **MAC destino** en hardware. Esto es clave para el
rendimiento: en una red con tráfico, el 99% de las tramas no son para vos, y el EMAC las tira sin
interrumpirte. Se controla con `RxFilterCtrl` (`EMAC_RFC_*`):

- `EMAC_RFC_UCAST_EN`: aceptá unicast (tramas a tu MAC).
- `EMAC_RFC_BCAST_EN`: aceptá broadcast (necesario para ARP, DHCP).
- `EMAC_RFC_MCAST_EN`: aceptá **todo** multicast.
- `EMAC_RFC_PERFECT_EN`: "perfect match", aceptá lo que coincide exacto con tu `SA0/1/2`.
- `EMAC_RFC_UCAST_HASH_EN` / `EMAC_RFC_MCAST_HASH_EN`: filtro por **hash**.

El **hash** resuelve un problema real: querés escuchar varios grupos multicast (p. ej. mDNS, IGMP) sin
aceptar *todo* el multicast ni listar cada MAC. El EMAC toma un CRC de 6 bits de la MAC destino, lo usa
como índice en una tabla de 64 bits (`HashFilterL` + `HashFilterH`) y, si ese bit está en 1, la trama
pasa. Es un filtro **probabilístico**: deja pasar lo que querés más algún falso positivo ocasional (otra
MAC con el mismo hash), que la pila descarta arriba. El driver lo arma solo con
`EMAC_SetHashFilter(mac, ENABLE)`.

**Modo promiscuo**: si querés ver *todo* el tráfico (un sniffer, un bridge), activás `EMAC_MAC1_PASS_ALL`
(de hecho el `EMAC_Init` del driver lo deja en `PASS_ALL`) y abrís los filtros. Costo: tu CPU se come cada
trama de la red, incluso las que no le importan. Para una aplicación normal, **no** lo querés.

```c
// configuración típica de filtro: unicast (perfect) + broadcast + multicast
LPC_EMAC->RxFilterCtrl = EMAC_RFC_PERFECT_EN | EMAC_RFC_BCAST_EN | EMAC_RFC_MCAST_EN;
// o por driver:
EMAC_SetFilterMode(EMAC_RFC_BCAST_EN | EMAC_RFC_PERFECT_EN, ENABLE);
```

## Recepción y transmisión por interrupción

En cualquier diseño serio el EMAC trabaja **por interrupción**, no por polling. La línea es `ENET_IRQn`
(handler `ENET_IRQHandler`). Las fuentes están en `IntStatus`/`IntEnable` (`EMAC_INT_*`):

| Bit | Significado |
|-----|-------------|
| `EMAC_INT_RX_DONE` | llegó una trama (la más importante en RX) |
| `EMAC_INT_RX_FIN` | el EMAC se quedó sin descriptores de RX libres |
| `EMAC_INT_RX_ERR` / `EMAC_INT_RX_OVERRUN` | error / overrun de recepción |
| `EMAC_INT_TX_DONE` | terminó de transmitir una trama |
| `EMAC_INT_TX_FIN` | terminó todos los descriptores de TX |
| `EMAC_INT_TX_ERR` / `EMAC_INT_TX_UNDERRUN` | error / underrun de transmisión |
| `EMAC_INT_WAKEUP` | Wake-on-LAN (despertar de power-down) |

Por defecto, `EMAC_Init` deja habilitadas solo `RX_DONE` y `TX_DONE`. El patrón del handler (sacado del
ejemplo EmacRaw) es: leer `IntStatus & IntEnable`, **limpiar** (`IntClear = int_stat`) y atender cada
fuente. En RX, por cada trama: chequear que haya datos (`EMAC_CheckReceiveIndex`), validar que no tenga
error (`EMAC_RINFO_ERR_MASK`), copiar el payload y **liberar** con `EMAC_UpdateRxConsumeIndex`.

```c
void ENET_IRQHandler(void) {
    uint32_t st;
    while ((st = LPC_EMAC->IntStatus & LPC_EMAC->IntEnable) != 0) {
        LPC_EMAC->IntClear = st;                 // 1) limpiar SIEMPRE primero
        if (st & EMAC_INT_RX_DONE) {
            if (EMAC_CheckReceiveIndex()) {
                if (EMAC_CheckReceiveDataStatus(EMAC_RINFO_LAST_FLAG) &&
                    !EMAC_CheckReceiveDataStatus(EMAC_RINFO_ERR_MASK)) {
                    // copiar payload a un buffer y entregárselo a la pila TCP/IP
                }
                EMAC_UpdateRxConsumeIndex();     // 2) liberar el buffer pase lo que pase
            }
        }
        if (st & EMAC_INT_TX_DONE)   { /* contabilizar / despertar al que esperaba TX */ }
        if (st & EMAC_INT_RX_OVERRUN){ /* error: subí el contador, quizá reiniciá RX  */ }
    }
}
```

Una sutileza que el manual documenta y conviene saber: el EMAC marca `RANGE_ERR` ("length out of range")
en tramas IP/ARP perfectamente válidas, porque el campo type/length lleva un EtherType (≥ 1536) que el chip
interpreta como una longitud "fuera de rango". El propio manual aclara que ese bit *"is not an error
indication, but simply a statement by the chip"* (Cap. 10, descripción de `StatusInfo`). Por eso `RANGE_ERR`
**no** está en `EMAC_RINFO_ERR_MASK`: **no es un error de verdad**, es ruido del hardware y se ignora aparte.

## Encendido y clock

Como todo periférico: `PCONP` bit 30 (`PCENET`) para alimentarlo. El EMAC corre del bus AHB a la
frecuencia del CPU (`CCLK`); no tiene un divisor `PCLKSEL` propio como los periféricos APB. Lo que sí
configurás es el reloj del canal **MIIM** hacia el PHY, que debe quedar **≤ 2.5 MHz**: se elige un divisor
en `MCFG` (`EMAC_MCFG_CLK_SEL`) a partir de `SystemCoreClock`. El driver lo calcula solo:

```c
CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCENET, ENABLE);   // PCONP bit 30
// ... el driver elige el divisor de MCFG para que el MDC quede <= 2.5 MHz
```

La interfaz de datos al PHY en esta placa es **RMII** (10 pines en vez de los ~18 de MII): se activa con
`EMAC_CR_RMII` en `Command`. Los pines RMII viven todos en el puerto P1, función 1, y los configurás por
`PINSEL` (lo vemos en detalle en la página siguiente junto al PHY).

## Lo que te llevás

- El **EMAC** es la mitad **hardware** de Ethernet: arma/desarma tramas (MAC, EtherType, CRC), filtra por
  MAC destino y mueve todo por **DMA** con descriptores. No "hace internet" solo.
- El concepto central son los **descriptores en RAM**: cuatro arrays (RX y TX, cada uno con descriptor +
  status) en anillo, y un modelo **productor/consumidor** con `Produce`/`Consume` index donde hardware y
  CPU trabajan en paralelo sin lock.
- Cuidado con el "menos uno": los tamaños y el `*DescriptorNumber` se programan como **N - 1**.
- El **filtro por hardware** (perfect/broadcast/multicast/hash) te ahorra interrupciones; el promiscuo te
  las regala todas.
- Recepción y transmisión van **por interrupción** (`ENET_IRQn`): `RX_DONE`/`TX_DONE` son las claves;
  limpiá `IntClear` primero y liberá el descriptor siempre.

## Referencias
- Manual, Cap. 10: [`../../manual/ch10_ethernet.pdf`](../../manual/ch10_ethernet.pdf)
- Driver: [`lpc17xx_emac.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_emac.h) /
  [`lpc17xx_emac.c`](../../library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_emac.c)
- Ejemplo crudo (descriptores + IRQ, sin pila): [`../../library/examples/EMAC/EmacRaw/`](../../library/examples/EMAC/EmacRaw/)

---

**Anterior:** [05 - I2S](./05-i2s.md) · **Siguiente:** [07 - Ethernet: del EMAC a TCP/IP](./07-ethernet-tcpip.md) ·
**Módulo:** [Periféricos adicionales](./README.md) · **Volver al** [índice del curso](../README.md)
