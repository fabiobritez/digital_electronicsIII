# Ethernet: del EMAC a TCP/IP (PHY, RMII y lwIP)

En la [página anterior](./06-ethernet.md) vimos la mitad **hardware**: el EMAC, los descriptores y el DMA.
Pero el EMAC es solo el **MAC**. Para que haya red de verdad faltan dos piezas: el **PHY** (el chip que
habla con el cable) y la **pila TCP/IP** (el software que convierte tramas en "conexiones"). Esta página
cierra el círculo.

## El PHY externo: quién toca el cable

El LPC1769 trae el **MAC**, pero **no** la electrónica que pone bits en el par trenzado. Eso lo hace un
chip aparte en la placa, el **PHY** (Physical layer transceiver): por ejemplo un **DP83848** (el de la
MCB1700) o un **LAN8720**. El PHY se ocupa de lo analógico/físico: niveles eléctricos, codificación de
línea, detectar si hay **cable enchufado** (link), negociar **10 vs 100 Mbit** y **half vs full duplex**.
En la placa, entre el PHY y el RJ45 hay además un **transformador de aislación** (magnetics) y el conector
con sus LEDs.

El reparto, en una frase: **EMAC = lógica de tramas; PHY = señal en el cable.** Hablan por dos buses:

- **RMII** (o MII): el bus de **datos**, por donde van los bits de las tramas en tiempo real.
- **MDIO/MDC** (la interfaz **MIIM**): un bus serie lento, aparte, para que el EMAC le **lea y escriba
  registros de control** al PHY (configurarlo, preguntarle si hay link, a qué velocidad negoció).

### RMII vs MII y los pines

**MII** clásico usa ~16-18 pines (4 bits de datos por sentido + varios controles). **RMII** (*Reduced MII*)
hace lo mismo con **la mitad**: 2 bits de datos por sentido a doble velocidad de reloj, compartiendo un
único reloj de referencia de **50 MHz**. El LPC1769 en placas como la MCB1700 usa **RMII**, que es lo
normal hoy. Se activa con `EMAC_CR_RMII` en `Command` (lo hace `EMAC_Init`).

Los pines RMII viven todos en **P1, función 1**, y se configuran por `PINSEL`:

| Pin | Señal | Sentido |
|-----|-------|---------|
| P1.0  | `ENET_TXD0`    | TX dato 0 |
| P1.1  | `ENET_TXD1`    | TX dato 1 |
| P1.4  | `ENET_TX_EN`   | habilitación de TX |
| P1.8  | `ENET_CRS`     | carrier sense / RX data valid |
| P1.9  | `ENET_RXD0`    | RX dato 0 |
| P1.10 | `ENET_RXD1`    | RX dato 1 |
| P1.14 | `ENET_RX_ER`   | error de recepción |
| P1.15 | `ENET_REF_CLK` | reloj de referencia 50 MHz |
| P1.16 | `ENET_MDC`     | reloj de la gestión MIIM |
| P1.17 | `ENET_MDIO`    | dato bidireccional de la gestión MIIM |

```c
PINSEL_CFG_Type p = { .Portnum = 1, .Funcnum = 1, .Pinmode = 0, .OpenDrain = 0 };
uint8_t rmii[] = {0,1,4,8,9,10,14,15,16,17};
for (int i = 0; i < 10; i++) { p.Pinnum = rmii[i]; PINSEL_ConfigPin(&p); }
```

### Hablarle al PHY por MDIO (la interfaz MIIM)

El EMAC no accede al PHY con punteros: usa la **máquina MIIM**, una especie de "I2C propietario" de
Ethernet. Vos cargás dirección y dato en registros del EMAC y el hardware serializa la transacción por
MDC/MDIO. Los registros son:

- `MCFG`: configuración (divisor del reloj MDC, ≤ 2.5 MHz; reset de la lógica MII).
- `MADR`: a quién y a qué, dirección del PHY (campo `PHY_ADR`) y registro del PHY (`REG_ADR`).
- `MWTD`: dato a escribir. `MRDD`: dato leído.
- `MCMD`: comando (`READ`, `SCAN` continuo).
- `MIND`: indicadores (`BUSY`, `NOT_VAL`, `LINK_FAIL`). Sondeás `BUSY` para saber cuándo terminó.

El driver lo encapsula en dos funciones internas que ilustran el patrón exacto:

```c
static int32_t write_PHY(uint32_t reg, uint16_t val) {
    LPC_EMAC->MADR = EMAC_DEF_ADR | reg;     // dirección del PHY + registro
    LPC_EMAC->MWTD = val;                    // dato
    while (LPC_EMAC->MIND & EMAC_MIND_BUSY)  // esperar a que termine (con timeout real)
        ;
    return 0;
}
static int32_t read_PHY(uint32_t reg) {
    LPC_EMAC->MADR = EMAC_DEF_ADR | reg;
    LPC_EMAC->MCMD = EMAC_MCMD_READ;
    while (LPC_EMAC->MIND & EMAC_MIND_BUSY)
        ;
    LPC_EMAC->MCMD = 0;
    return LPC_EMAC->MRDD;
}
```

`EMAC_DEF_ADR` (0x0100) lleva la **dirección del PHY** en el bus (varios PHY podrían colgar del mismo MDIO;
acá hay uno solo). Es el mismo mecanismo que vas a ver en cualquier MAC: el bus MDIO es estándar, los
**números de registro del PHY** también lo son (IEEE 802.3 define los registros 0..15 obligatorios).

### Los registros estándar del PHY y la autonegociación

Los primeros registros del PHY son **iguales en todos los PHY** (DP83848, LAN8720, KSZ8721, etc.):

| Reg | Nombre | Para qué |
|-----|--------|----------|
| 0 | **BMCR** (Basic Mode Control) | reset, power-down, elegir velocidad/duplex, activar autonegociación |
| 1 | **BMSR** (Basic Mode Status) | link up/down, autonegociación completa, capacidades |
| 2/3 | **IDR1/IDR2** | identificador del fabricante/modelo (así el driver confirma "esto es un DP83848") |
| 4 | **ANAR** | qué le ofrezco al otro lado en la autonegociación |
| 5 | **ANLPAR** | qué me ofreció el otro lado |

La **autonegociación** es la danza por la que ambos extremos se ponen de acuerdo en la mayor velocidad y
duplex que los dos soportan, sin que vos elijas nada. El flujo en el driver (`EMAC_SetPHYMode` con
`EMAC_MODE_AUTO`):

1. Resetear el PHY: `write_PHY(BMCR, EMAC_PHY_BMCR_RESET)` y esperar a que el bit se autolimpie.
2. Confirmar el modelo leyendo `IDR1/IDR2` (compara contra `EMAC_DP83848C_ID`).
3. Lanzar autonegociación: `write_PHY(BMCR, EMAC_PHY_AUTO_NEG)`.
4. Esperar `BMSR.AUTO_DONE` (con timeout): la negociación terminó.
5. **Leer el resultado** y **espejarlo en el EMAC**: esto es `EMAC_UpdatePHYStatus`, que mira link, speed
   y duplex en el registro de status del PHY y ajusta `MAC2`, `Command` (`FULL_DUP`), `IPGT` y `SUPP`
   (bit de velocidad RMII) **en consecuencia**. Si el PHY negoció 100M full-duplex, el MAC tiene que
   quedar en 100M full-duplex: no se enteran solos, los sincronizás vos por MDIO.

```c
// modos posibles que le pedís al PHY (lpc17xx_emac.h)
EMAC_MODE_AUTO        // autonegociar (lo normal)
EMAC_MODE_10M_FULL  EMAC_MODE_10M_HALF
EMAC_MODE_100M_FULL EMAC_MODE_100M_HALF   // forzar, p. ej. si el otro lado no negocia
```

### Link up / link down

`BMSR.LINK_STATUS` (o el status extendido del PHY) te dice si **hay cable y portadora**. Esto cambia en
caliente: alguien desenchufa el patch cord y se cae el link. En una app robusta sondeás el link
periódicamente (o por la IRQ del PHY, si tu PHY la tiene y la cableás) y, cuando vuelve, **rehacés la
autonegociación** y volvés a llamar `EMAC_UpdatePHYStatus`. La pila TCP/IP (lwIP) tiene callbacks de
`netif` para avisar "link up/down" hacia arriba.

## La otra mitad: la pila TCP/IP

El EMAC + PHY ya te dan **tramas crudas** que entran y salen del cable. Pero una trama Ethernet con un
payload no es "internet". Para llegar a "abrí una conexión TCP al puerto 80" hace falta una **torre de
protocolos**:

```
   Tu aplicación  (HTTP, MQTT, tu protocolo)
   ─────────────────────────────────────────
   TCP / UDP        (puertos, confiabilidad, control de flujo)
   IP / ICMP        (direcciones IP, ruteo, ping)
   ARP              (¿qué MAC tiene esta IP?)
   ─────────────────────────────────────────
   Ethernet (EMAC + PHY)  ← acá termina el hardware
```

Cada capa **encapsula** a la de abajo: tu byte de HTTP va adentro de un segmento TCP, adentro de un
paquete IP, adentro de una trama Ethernet. En recepción, al revés: el EMAC te da la trama, ARP/IP/TCP la
desarman y tu callback recibe el byte. Todo eso es **software**, y es **mucho**.

### Por qué NO escribís tu propia pila

TCP solo (retransmisiones, ventanas deslizantes, slow-start, manejo de estados, fragmentación) es miles de
líneas con décadas de casos de borde. Reimplementarlo bien es un proyecto en sí mismo, y uno mal hecho te
va a fallar de formas sutiles bajo carga real. **La regla de oro: usá una pila probada, no la escribas.**

Las dos clásicas para este micro:

- **lwIP** (*lightweight IP*): la más usada en embebidos. Soporta TCP, UDP, DHCP, DNS, etc. Anda
  **bare-metal** (sin RTOS, modo "raw API" con callbacks) o **con RTOS** (FreeRTOS, con su API de sockets
  estilo BSD). Es la opción por defecto si querés un servidor web o un cliente TCP serio. NXP provee
  ports de lwIP para LPC17xx.
- **uIP**: más chica y vieja, una conexión a la vez, pensada para RAM mínima. Hay un ejemplo en
  `examples/EMAC/uIP/`. Buena para entender los protocolos; corta para algo serio.

Y un punto intermedio, el ejemplo **Easy_Web** (`examples/EMAC/Easy_Web/`): un mini-stack TCP/IP didáctico
que sirve una página y lee el ADC. Sirve para ver "todo junto" sin el peso de lwIP, pero no es una base
para producción.

### Cómo se integra el EMAC con lwIP

lwIP no sabe de registros del LPC. La integración es un **driver de `netif`** (network interface) chiquito
que vos (o el port de NXP) escribís, con esencialmente dos funciones de pegamento:

- **`low_level_output(netif, pbuf)`**: lwIP te pasa una trama ya armada (Ethernet + IP + TCP); vos la
  copiás al buffer de TX y hacés `EMAC_WritePacketBuffer` + `EMAC_UpdateTxProduceIndex`. Eso es "mandala".
- **`low_level_input` / la ISR**: en la `ENET_IRQHandler`, cuando entra una trama (`RX_DONE`), copiás el
  payload a un `pbuf` y se lo entregás a lwIP (`netif->input`, que arranca el desarmado ARP/IP/TCP). Liberás
  el descriptor con `EMAC_UpdateRxConsumeIndex`.

```c
// pegamento (esquema): TX
static err_t low_level_output(struct netif *n, struct pbuf *p) {
    EMAC_PACKETBUF_Type pkt = { .pbDataBuf = (uint32_t*)p->payload, .ulDataLen = p->tot_len };
    EMAC_WritePacketBuffer(&pkt);
    EMAC_UpdateTxProduceIndex();
    return ERR_OK;
}

// pegamento: RX, dentro de ENET_IRQHandler tras EMAC_INT_RX_DONE
if (EMAC_CheckReceiveIndex() && !EMAC_CheckReceiveDataStatus(EMAC_RINFO_ERR_MASK)) {
    uint32_t len = EMAC_GetReceiveDataSize() - 3;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    EMAC_PACKETBUF_Type rx = { .pbDataBuf = (uint32_t*)p->payload, .ulDataLen = len };
    EMAC_ReadPacketBuffer(&rx);
    netif->input(p, netif);      // entregar a la pila
}
EMAC_UpdateRxConsumeIndex();
```

**Sin RTOS (raw API):** lwIP corre en el contexto principal. La ISR solo señaliza "llegó algo"; en el
`while(1)` llamás `sys_check_timeouts()` (timers de TCP/ARP/DHCP) y procesás lo recibido. Tus handlers son
**callbacks** (`tcp_recv`, `tcp_sent`, ...). No bloquea, pero tenés que estructurar tu lógica como máquina
de estados.

**Con RTOS:** lwIP corre en su propia tarea (`tcpip_thread`), la ISR le manda un mensaje, y tu aplicación
usa la **API de sockets** estilo BSD (`socket()`, `bind()`, `recv()`, `send()`), que bloquea como en una PC.
Más cómodo, a costo de RAM y un RTOS.

### DHCP, IP estática y el "primer ping"

Tu micro necesita una **dirección IP**. O se la ponés fija (IP estática, simple para una red controlada) o
la pide por **DHCP** al router (lwIP trae cliente DHCP). El hito que confirma que **todo** funciona (EMAC,
PHY, link, ARP, IP) es que la placa **responda un `ping`** (ICMP echo). Si pinguea, el 90% del camino está
hecho; de ahí para arriba es agregar tu lógica (un servidor HTTP, un cliente MQTT).

## El flujo de trabajo real

No arranques de cero. El camino sensato:

1. Tomá un **ejemplo de lwIP de NXP para LPC17xx** (o el port que tengas), ponele tu MAC e IP/DHCP.
2. Hacelo **compilar y andar** hasta que responda `ping`. Ese es el hito.
3. Recién ahí agregá tu aplicación (servidor web de config, cliente que postea datos, etc.).
4. Pelearte con los **registros del EMAC a mano** solo tiene sentido si estás **portando** la pila o
   depurando muy abajo (link que no levanta, tramas que no llegan).

## Casos de borde y consideraciones

- **Alineación de buffers.** Los buffers de descriptor son accedidos por el DMA: tienen que estar
  **alineados** (el driver usa `aligned(8)` para los status y word-align para los datos). Buffers
  desalineados = corrupción silenciosa o hard fault.
- **Tamaño del pool.** Pocos descriptores RX (el default son 4) y, bajo ráfaga, el EMAC se queda sin
  buffers libres → `RX_OVERRUN`, tramas perdidas. Si tu app procesa lento o hay mucho tráfico, **subí**
  `EMAC_NUM_RX_FRAG`. Cuesta RAM (1536 B por fragmento), y el LPC1769 tiene RAM acotada: hay un
  bloque de SRAM pensado para Ethernet (AHB SRAM) justo para esto.
- **Latencia y el handler.** El `ENET_IRQHandler` tiene que ser **corto**: copiar y liberar el descriptor,
  no procesar TCP adentro de la ISR. El procesamiento pesado va al main loop o a la tarea de lwIP.
- **Throughput.** El EMAC da 100 Mbit en el cable, pero el cuello de botella real es la **CPU copiando** y
  la pila procesando. No esperes 100 Mbit útiles; para telemetría/web está de sobra.
- **Endianness y MAC.** La MAC se carga en `SA0/SA1/SA2` en orden particular (ver `setEmacAddr`); usá el
  helper del driver y no inviertas bytes a ojo.
- **El "menos uno"** de los tamaños y `*DescriptorNumber` (ver página anterior) es la causa nº 1 de tramas
  corridas un byte.

## Esqueleto de inicialización realista

Juntando las dos páginas, levantar Ethernet "a mano" con el driver es:

```c
#define MY_MAC { 0x00, 0x1F, 0xE0, 0x12, 0x1D, 0x0C }

void eth_init(void) {
    // 1) Pines RMII (P1, función 1): TXD0/1, TX_EN, CRS, RXD0/1, RX_ER, REF_CLK, MDC, MDIO
    PINSEL_CFG_Type p = { .Portnum=1, .Funcnum=1, .Pinmode=0, .OpenDrain=0 };
    uint8_t rmii[] = {0,1,4,8,9,10,14,15,16,17};
    for (int i = 0; i < 10; i++) { p.Pinnum = rmii[i]; PINSEL_ConfigPin(&p); }

    // 2) EMAC_Init: PCONP bit 30, reset del MAC, MIIM, RMII, reset + autoneg del PHY,
    //    MAC address, descriptores RX/TX, filtro, IRQ RX_DONE/TX_DONE
    uint8_t mac[] = MY_MAC;
    EMAC_CFG_Type cfg = { .Mode = EMAC_MODE_AUTO, .pbEMAC_Addr = mac };
    while (EMAC_Init(&cfg) == ERROR) { /* link no listo: reintentar tras un delay */ }

    // 3) Filtro: unicast (perfect) + broadcast (ARP/DHCP) + multicast
    EMAC_SetFilterMode(EMAC_RFC_PERFECT_EN | EMAC_RFC_BCAST_EN | EMAC_RFC_MCAST_EN, ENABLE);

    // 4) Habilitar todas las IRQ que te importan y prender ENET_IRQn
    EMAC_IntCmd(EMAC_INT_RX_DONE | EMAC_INT_RX_ERR | EMAC_INT_RX_OVERRUN |
                EMAC_INT_TX_DONE | EMAC_INT_TX_ERR, ENABLE);
    NVIC_SetPriority(ENET_IRQn, 1);
    NVIC_EnableIRQ(ENET_IRQn);

    // 5) De acá en adelante: arrancar lwIP (netif_add con el pegamento TX/RX) y DHCP.
}
```

`EMAC_Init` ya hace casi todo lo de la mitad hardware (lo viste en la página anterior). Lo que agregás vos
es: pines, filtro a gusto, IRQ y, la parte grande, **enganchar lwIP**.

## Lo que te llevás

- El **PHY** (DP83848/LAN8720) es un chip aparte que toca el cable; el EMAC le habla por **RMII** (datos) y
  **MDIO/MIIM** (control). Vos inicializás el PHY por MDIO, disparás **autonegociación** y **espejás** el
  resultado (speed/duplex/link) al EMAC.
- Los **pines RMII** van en P1 función 1; el reloj MIIM (MDC) ≤ 2.5 MHz.
- Arriba del EMAC va una **pila TCP/IP**: **no la escribas**, usá **lwIP** (o uIP). Se integra con un
  pegamento `netif` de dos funciones (TX por `WritePacketBuffer`, RX en la ISR hacia `netif->input`).
- Bare-metal (callbacks, `sys_check_timeouts`) o con RTOS (sockets BSD): elegís según comodidad vs RAM.
- El hito de validación es **responder `ping`**. Arrancá de un ejemplo de lwIP de NXP y construí desde ahí.
- Cuidá **alineación de buffers**, **tamaño del pool RX** (overrun) y mantené la **ISR corta**.

## Referencias
- Manual, Cap. 10: [`../../manual/ch10_ethernet.pdf`](../../manual/ch10_ethernet.pdf)
- Driver: [`lpc17xx_emac.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_emac.h) /
  [`lpc17xx_emac.c`](../../library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_emac.c)
- Ejemplos: [`Easy_Web`](../../library/examples/EMAC/Easy_Web/) (mini-stack didáctico),
  [`uIP`](../../library/examples/EMAC/uIP/) (pila chica),
  [`EmacRaw`](../../library/examples/EMAC/EmacRaw/) (EMAC sin pila). La integración con **lwIP** está en los
  paquetes de ejemplo de NXP para LPC17xx.

---

**Anterior:** [06 - Ethernet: el EMAC y los descriptores](./06-ethernet.md) ·
**Módulo:** [Periféricos adicionales](./README.md) · **Volver al** [índice del curso](../README.md)
