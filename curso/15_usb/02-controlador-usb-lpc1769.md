# El controlador USB del LPC1769 (a nivel registro)

Esta página baja al **silicio**: cómo es por dentro el USB device controller del LPC1769, qué
registros tiene y cómo se habla con él. No vas a escribir esto a mano en la práctica (para eso está el
stack, [página 03](./03-usb-en-la-practica.md)), pero entenderlo es lo que te permite **leer el stack,
configurarlo y depurar** cuando algo no enumera. Todos los nombres son los exactos de
`LPC17xx.h` (struct `LPC_USB_TypeDef`) y `usbreg.h`.

> Esta es una página avanzada/"plus". El objetivo es que el código del driver (`usbhw.c`) deje de ser
> una caja negra: que reconozcas cada registro y cada comando.

## La arquitectura por dentro

El periférico se organiza así:

```
   D+ / D−  ─►  Analog Transceiver  ─►  SIE  ─►  EP RAM (endpoint buffers)
                (PHY full-speed)        │            │
                                   comandos       Slave (FIFO por USBRxData/USBTxData)
                                   por registros        ó
                                                    DMA (lee/escribe RAM principal solo)
```

- **Transceiver (PHY):** la parte analógica que maneja el par diferencial D+/D−, el *pull-up* de
  full-speed y la detección de estados del bus.
- **SIE (Serial Interface Engine):** el corazón. Hace en hardware todo lo de bajo nivel del protocolo:
  *bit stuffing*, CRC, NAK/ACK, *toggle* de DATA0/DATA1, detección de SETUP/reset. No le hablás por un
  registro de datos común, sino con un **juego de comandos** (ver abajo).
- **EP RAM:** una memoria interna de **4 KB** donde viven los buffers de cada endpoint. Tiene
  capacidad para **32 endpoints físicos** (= **16 lógicos**, cada uno con su IN y su OUT). El doble
  buffering de los endpoints bulk e isochronous (los de datos masivos) también sale de acá.
- Dos formas de mover los bytes entre la EP RAM y tu programa: **Slave mode** (la CPU copia byte a byte
  con los registros `USBRxData`/`USBTxData`) o **DMA** (un motor copia solo, sin CPU). El stack del
  curso usa Slave por defecto.

### Endpoints físicos vs lógicos

Esto confunde, así que fijalo: un endpoint **lógico** es lo que ves en USB (endpoint 2, por ejemplo).
Cada lógico tiene **dos físicos**: el OUT y el IN. La numeración física del LPC es:

```
físico = (lógico << 1) | dir       (dir: 0 = OUT, 1 = IN)
```

Entonces: EP0 OUT = físico 0, EP0 IN = físico 1, EP1 OUT = físico 2, EP1 IN = físico 3, EP2 OUT =
físico 4, EP2 IN = físico 5… hasta el físico 31. En el código vas a ver exactamente esta cuenta:

```c
/* de usbhw.c: dirección física a partir de la dirección de endpoint USB */
uint32_t EPAdr (uint32_t EPNum) {
  uint32_t val;
  val = (EPNum & 0x0F) << 1;     /* número lógico * 2 */
  if (EPNum & 0x80) val += 1;    /* bit 7 = IN  ->  +1 */
  return (val);
}
```

Otro detalle del silicio: el **tipo** de cada endpoint está **fijo** en el hardware (Tabla 186 del
manual, "Fixed endpoint configuration"). El patrón se repite de a tres: EP1 interrupt, EP2 bulk,
EP3 isochronous, EP4 interrupt, EP5 bulk, EP6 iso… y así hasta EP14 (el EP15 es bulk). En el
descriptor no elegís el tipo con libertad: tenés que usar un número de endpoint cuyo tipo coincida.
Por eso el CDC de la [página 03](./03-usb-en-la-practica.md) usa justo el EP1 para las notificaciones
(interrupt) y el EP2 para los datos (bulk).

## Clocking: prender el USB a 48 MHz

El bloque tiene su propio control de reloj. Pasos (lo hace `USB_Init` en `usbhw.c`):

1. **Power:** prender el periférico en `LPC_SC->PCONP`, bit `PCUSB` (bit 31).
2. **Reloj USB:** hay que tener los **48 MHz** desde la PLL (PLL0 o la USB PLL, módulo 3 y
   `system_LPC17xx.c`). El periférico recibe ese reloj.
3. **Clock control del bloque:** `USBClkCtrl` (mismo registro que `OTGClkCtrl`) enciende los relojes
   internos: *Device clock*, *AHB clock* y *PortSelect*. Después se **espera** a que `USBClkSt`
   confirme que están estables:

```c
LPC_SC->PCONP   |= (1UL<<31);                 /* prende el periférico USB */
LPC_USB->USBClkCtrl = 0x1A;                   /* Dev clock + AHB clock + PortSel */
while ((LPC_USB->USBClkSt & 0x1A) != 0x1A);   /* esperar a que estén listos */
```

El `0x1A` = `0b11010` no es arbitrario: son tres bits del registro. La Tabla 190 (capítulo 11)
documenta dos: **bit 1 `DEV_CLK_EN`** (device clock) y **bit 4 `AHB_CLK_EN`** (clock del bus AHB); el
**bit 3** ahí figura como reservado, pero en la vista OTG del mismo registro (`OTGClkCtrl`, Tabla 262
del capítulo 13) es **`OTG_CLK_EN`**: el reloj de la lógica OTG que selecciona el puerto, por eso el
comentario "PortSel" en el código. El `while` con la misma máscara espera a que `USBClkSt` confirme
los tres estables antes de seguir.

Si te salteás el `while` o el reloj no es de 48 MHz exactos, el device puede prender pero **no
enumerar**. Es la causa nº 1 de "no aparece nada al enchufar".

### Pines: D+/D−, SoftConnect, VBUS, GoodLink

El USB usa pines fijos del LPC1769, que hay que poner en su función con `PINSEL` (módulo 4):

| Señal | Pin | Para qué |
|-------|-----|----------|
| **USB_D+** | P0.29 | par diferencial |
| **USB_D−** | P0.30 | par diferencial |
| **USB_CONNECT** (SoftConnect) | P2.9 | controla el *pull-up* de D+ por software |
| **USB_VBUS** | P1.30 | *VBUS sensing*: detectar que hay 5 V del host |
| **USB_UP_LED** (GoodLink) | P1.18 | LED de estado: prendido fijo cuando el device quedó configurado, apagado en suspend |

```c
/* de USB_Init: configurar los pines */
LPC_PINCON->PINSEL1 &= ~((3<<26)|(3<<28));    /* P0.29 D+, P0.30 D- */
LPC_PINCON->PINSEL1 |=  ((1<<26)|(1<<28));
LPC_PINCON->PINSEL3 &= ~((3<< 4)|(3<<28));    /* P1.18 GoodLink, P1.30 VBUS */
LPC_PINCON->PINSEL3 |=  ((1<< 4)|(2<<28));
LPC_PINCON->PINSEL4 &= ~((3<<18));            /* P2.9 SoftConnect */
LPC_PINCON->PINSEL4 |=  ((1<<18));
```

Detalle fino: la secuencia de inicialización del manual (sección 11.13, paso 5) pide además dejar el
pin VBUS **sin pull-up ni pull-down** (vía `PINMODE`). El `USB_Init` del stack no lo hace y en la
mayoría de las placas funciona igual, pero si perseguís un problema de detección de VBUS, ahí tenés
un sospechoso.

**SoftConnect** es importante conceptualmente: el *pull-up* de D+ (lo que le avisa al host "estoy acá,
soy full-speed") no está cableado fijo: es una resistencia de **1.5 kΩ** entre D+ y 3.3 V que el pin
USB_CONNECT conecta a través de una llave externa en la placa. Como lo controlás por software, podés
terminar de inicializar todo y **recién entonces** "conectarte" al host (con el comando *Set Device
Status* → `DEV_CON`). Eso es lo que hace `USB_Connect(TRUE)` en el `main`. Te evita que el host empiece a
enumerar antes de que estés listo.

## Cómo se le habla al SIE: el protocolo de comandos

Acá está la idea central del LPC. No hay "un registro por cada cosa". Para casi todo (poner la
dirección, configurar, seleccionar un endpoint, validar un buffer…) le mandás un **comando** al SIE por
`USBCmdCode` y, si corresponde, datos por el mismo registro o leés respuesta por `USBCmdData`. La
sincronización es por flags en **`USBDevIntSt`**:

- **`CCEMTY` (Command Code Empty):** el registro de comando quedó libre, podés mandar el próximo.
- **`CDFULL` (Command Data Full):** hay un dato de respuesta listo en `USBCmdData`.

El patrón está en tres funciones de `usbhw.c`:

```c
void WrCmd (uint32_t cmd) {                       /* mandar un comando */
  LPC_USB->USBDevIntClr = CCEMTY_INT;
  LPC_USB->USBCmdCode = cmd;
  while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
}

void WrCmdDat (uint32_t cmd, uint32_t val) {      /* comando + dato de escritura */
  LPC_USB->USBDevIntClr = CCEMTY_INT;
  LPC_USB->USBCmdCode = cmd;
  while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
  LPC_USB->USBDevIntClr = CCEMTY_INT;
  LPC_USB->USBCmdCode = val;
  while ((LPC_USB->USBDevIntSt & CCEMTY_INT) == 0);
}

uint32_t RdCmdDat (uint32_t cmd) {                /* comando + leer respuesta */
  LPC_USB->USBDevIntClr = CCEMTY_INT | CDFULL_INT;
  LPC_USB->USBCmdCode = cmd;
  while ((LPC_USB->USBDevIntSt & CDFULL_INT) == 0);
  return (LPC_USB->USBCmdData);
}
```

Los **comandos del SIE** más importantes (constantes de `usbreg.h`):

| Comando | Constante | Para qué |
|---------|-----------|----------|
| Set Address | `CMD_SET_ADDR` | fijar la dirección del device (paso 4 de la enumeración) |
| Configure Device | `CMD_CFG_DEV` | poner el device en estado configurado |
| Set Device Status | `CMD_SET_DEV_STAT` | conectar/desconectar (`DEV_CON`), suspend… |
| Get Device Status | `CMD_GET_DEV_STAT` | leer reset/connect/suspend (en la ISR) |
| Select Endpoint | `CMD_SEL_EP(x)` | seleccionar un endpoint físico para operarlo |
| Set Endpoint Status | `CMD_SET_EP_STAT(x)` | stall / unstall / disable de un endpoint |
| Clear Buffer | `CMD_CLR_BUF` | liberar el buffer OUT ya leído (lo vuelve a habilitar) |
| Validate Buffer | `CMD_VALID_BUF` | marcar el buffer IN como "listo para que el host lo lleve" |
| Read Error Status | `CMD_RD_ERR_STAT` | diagnóstico (CRC, bit stuffing, timeout…) |

Notá que `CMD_SET_ADDR` lleva el dato `DEV_EN | adr`: el bit `DEV_EN` *habilita* la dirección. Y que
"conectarse" al host es literalmente `WrCmdDat(CMD_SET_DEV_STAT, DAT_WR_BYTE(DEV_CON))`.

Un detalle del protocolo que se ve mirando `usbreg.h`: `CMD_SET_DEV_STAT` y `CMD_GET_DEV_STAT` tienen
**el mismo código** (`0x00FE0500`). No es un error: el comando del SIE es el mismo "Set/Get Device
Status"; lo que decide *set* vs *get* es si después escribís un dato (`WrCmdDat`) o leés la respuesta
(`RdCmdDat`). Varios comandos del SIE comparten este patrón.

## Mover los datos: registros de transferencia (Slave mode)

Para los **datos** de los endpoints (no comandos), hay registros aparte:

| Registro | Dirección | Qué hace |
|----------|-----------|----------|
| `USBCtrl` | - | habilita lectura/escritura de un endpoint (`CTRL_RD_EN` / `CTRL_WR_EN`) y elige cuál |
| `USBRxData` | lectura | sacar de a 32 bits del buffer OUT seleccionado |
| `USBRxPLen` | lectura | largo del paquete recibido (`PKT_LNGTH_MASK`) + flag `PKT_RDY` |
| `USBTxData` | escritura | meter de a 32 bits al buffer IN |
| `USBTxPLen` | escritura | largo del paquete que vas a transmitir |

El flujo de **leer un paquete OUT** (host → device), de `USB_ReadEP`:

```c
LPC_USB->USBCtrl = ((EPNum & 0x0F) << 2) | CTRL_RD_EN;   /* habilitar lectura de ese EP */
do { cnt = LPC_USB->USBRxPLen; } while ((cnt & PKT_RDY) == 0);  /* esperar paquete */
cnt &= PKT_LNGTH_MASK;                                    /* cuántos bytes hay */
for (n = 0; n < (cnt+3)/4; n++) { *(uint32_t*)pData = LPC_USB->USBRxData; pData += 4; }
LPC_USB->USBCtrl = 0;
WrCmdEP(EPNum, CMD_CLR_BUF);          /* liberar el buffer para el próximo paquete */
```

Y **escribir un paquete IN** (device → host), de `USB_WriteEP`:

```c
LPC_USB->USBCtrl = ((EPNum & 0x0F) << 2) | CTRL_WR_EN;
LPC_USB->USBTxPLen = cnt;                                 /* cuántos bytes mando */
for (n = 0; n < (cnt+3)/4; n++) { LPC_USB->USBTxData = *(uint32_t*)pData; pData += 4; }
LPC_USB->USBCtrl = 0;
WrCmdEP(EPNum, CMD_VALID_BUF);        /* "buffer listo, host: vení a buscarlo" */
```

El `CMD_VALID_BUF` final es la clave de la asimetría host/device: vos **no mandás** el dato, lo dejás
validado y el host lo levanta en el próximo polling IN. Lo mismo `CMD_CLR_BUF` al leer: hasta que no lo
liberás, ese buffer OUT no acepta el siguiente paquete (y el device responde NAK al host).

## Interrupciones: cómo te enterás de las cosas

El USB es **fuertemente basado en interrupciones** (módulo 7). El registro maestro es `USBDevIntSt`
(con su `USBDevIntEn`, `USBDevIntClr`, `USBDevIntSet`). Bits clave (de `usbreg.h`):

| Bit | Constante | Qué señala |
|-----|-----------|------------|
| Frame | `FRAME_INT` | llegó un SOF (Start of Frame), cada 1 ms |
| EP Fast / EP Slow | `EP_FAST_INT` / `EP_SLOW_INT` | actividad en algún endpoint (a cuál de los dos va cada endpoint lo decide `USBEpIntPri`; el stack manda todos a EP Slow) |
| Device Status | `DEV_STAT_INT` | cambió el estado del bus: **reset**, connect, suspend/resume |
| CCEMTY / CDFULL | `CCEMTY_INT` / `CDFULL_INT` | sincronización del protocolo de comandos (vistos arriba) |
| EP Realized | `EP_RLZED_INT` | terminó de "realizarse" (configurarse) un endpoint |
| Error | `ERR_INT` | hubo un error de protocolo (mirá `CMD_RD_ERR_STAT`) |

Para saber **qué endpoint** generó la interrupción está `USBEpIntSt` (un bit por endpoint físico), con
su `En`/`Clr`/`Set`. La ISR (`USB_IRQHandler`) hace, en esencia:

1. Si vino `DEV_STAT_INT`: leer estado del device; si hay **reset de bus**, reinicializar todo
   (`USB_Reset`): esto pasa al principio de cada enumeración.
2. Si vino `EP_SLOW_INT`: recorrer `USBEpIntSt`, y por cada endpoint con actividad, distinguir si es un
   paquete **SETUP** (control, bit `EP_SEL_STP`), un **OUT** o un **IN**, y llamar al *callback* de ese
   endpoint (`USB_P_EP[m]` con `USB_EVT_SETUP` / `USB_EVT_OUT` / `USB_EVT_IN`). Ahí engancha el stack de
   nivel superior.

Esa tabla `USB_P_EP[]` de callbacks por endpoint es **el punto de enganche** entre el driver de
hardware y la lógica de la clase. La clase CDC, por ejemplo, registra ahí su handler del endpoint
bulk.

## Realización de endpoints y EP RAM

Antes de usar un endpoint hay que "**realizarlo**": decirle al periférico que existe y de qué tamaño es
su buffer en la EP RAM. Eso se hace con `USBReEp` (qué endpoints están realizados), `USBEpInd`
(seleccionar cuál) y `USBMaxPSize` (su tamaño máximo de paquete), esperando el flag `EP_RLZED_INT`:

```c
/* de USB_ConfigEP: realizar un endpoint según su descriptor */
num = EPAdr(pEPD->bEndpointAddress);
LPC_USB->USBReEp |= (1 << num);
LPC_USB->USBEpInd = num;
LPC_USB->USBMaxPSize = pEPD->wMaxPacketSize;
while ((LPC_USB->USBDevIntSt & EP_RLZED_INT) == 0);
LPC_USB->USBDevIntClr = EP_RLZED_INT;
```

El `wMaxPacketSize` viene **directo del endpoint descriptor**. Por eso, si en el descriptor declarás un
endpoint de 64 bytes pero el host manda 64 y tu buffer quedó de 32, **rompe**. Los tamaños del
descriptor y la realización tienen que coincidir. Los tamaños válidos dependen del tipo (Tabla 186):
8/16/32/64 para control y bulk, hasta 64 para interrupt, hasta 1023 para isochronous.

## El modo DMA (panorama)

En **Slave mode** (lo que usa el curso, `USB_DMA = 0` en `usbcfg.h`) la CPU copia cada byte con
`USBRxData`/`USBTxData`. Funciona y es simple, pero la CPU "se gasta" copiando.

En **DMA mode**, un motor de DMA del propio periférico mueve los datos entre la EP RAM y la **RAM
principal** sin la CPU. Para eso necesita:

- La **UDCA** (USB Device Communication Area): una tabla en RAM (apuntada por `USBUDCAH`) con un puntero
  de *DMA descriptor* por cada endpoint físico.
- **DMA descriptors:** estructuras en RAM que describen cada transferencia (dirección del buffer, largo,
  modo, estado).
- Registros propios: `USBEpDMAEn`/`USBEpDMADis` (qué endpoints usan DMA), `USBDMAIntSt`/`En`,
  `USBEoTIntSt` (End of Transfer), `USBNDDRIntSt` (New DD Request), `USBSysErrIntSt`.

El DMA conviene para endpoints de **mucho throughput** (MSC, audio) y **no está disponible para los
endpoints de control** (físicos 0 y 1): el endpoint 0 siempre va en Slave. Para CDC/HID el Slave mode
está más que bien y es más fácil de seguir. La diferencia, para el curso, es conceptual: DMA = la CPU
queda libre mientras se mueven los datos; el costo es manejar los descriptores en RAM.

---

Ya tenés el mapa del silicio. La [próxima página](./03-usb-en-la-practica.md) muestra cómo el stack de
NXP arma todo esto en capas, cómo enganchás tu código con los callbacks, y un ejemplo concreto de CDC
("eco serial por USB").

---

**Anterior:** [01 - Cómo funciona el USB](./01-usb-conceptos.md) ·
**Módulo:** [USB](./README.md) ·
**Siguiente:** [03 - USB en la práctica](./03-usb-en-la-practica.md)
