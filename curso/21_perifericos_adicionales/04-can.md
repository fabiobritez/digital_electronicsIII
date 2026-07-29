# CAN: bus robusto multinodo (protocolo, bit timing y controlador)

El **CAN** (*Controller Area Network*) es un bus serial pensado para ambientes **eléctricamente
hostiles** y para conectar **muchos nodos** que se hablan entre sí sin un maestro central. Nació en la
industria automotriz (todo auto moderno tiene varias redes CAN: motor, frenos, tablero, confort) y se
usa muchísimo en maquinaria industrial, agrícola y médica. Su fama viene de ser **robusto**: detecta
errores, los corrige reintentando, y un nodo defectuoso se "desconecta" solo para no tirar abajo la red.

Capítulo 16 del manual. El LPC1769 tiene **dos** controladores CAN (CAN1, CAN2), compatibles con la
**CAN 2.0B** (ISO 11898-1), más un **Acceptance Filter** global compartido por ambos. PCONP bits **13**
y **14** (`PCAN1`, `PCAN2`).

Es un tema grande, así que va en **dos páginas**:

1. **Esta**: el protocolo (frame, arbitraje), el **bit timing** real (`CANBTR`), los registros del
   controlador, los modos y el manejo de errores.
2. [04b - CAN: filtro de aceptación y driver](./04b-can-filtro.md): el **Acceptance Filter** a fondo
   (FullCAN, Standard/Extended, individual y grupos), la recepción por interrupción, el transceiver
   externo y el driver CMSIS de punta a punta.

## Las ideas que lo hacen distinto

- **Diferencial y multipunto:** dos cables (`CAN_H`, `CAN_L`) que llevan la señal como **diferencia** de
  tensión, lo que lo hace muy inmune al ruido. Todos los nodos cuelgan del mismo par (un *bus*), con
  resistencias de terminación de 120 Ω en las puntas.
- **Mensajes con identificador, no con dirección de destino.** En CAN no le hablás a "el nodo 5": ponés
  en el bus un **mensaje con un ID** (por ejemplo, "RPM del motor = 3000") y **todos** lo escuchan; cada
  nodo decide si le interesa según el ID. Es comunicación **por contenido**, no por destinatario. De ahí
  que el broadcast y el multicast salgan gratis.
- **Arbitraje sin colisiones.** Si dos nodos transmiten a la vez, el de **ID más bajo** (más prioritario)
  gana automáticamente y el otro se calla y reintenta, sin que se corrompa ningún mensaje. Esto lo hace
  el hardware bit a bit (lo vemos abajo).
- **Detección de errores fuerte:** CRC, acuse (ACK), chequeos de forma, *bit stuffing*. Un mensaje con
  error se retransmite solo, y los nodos llevan contadores de error que aíslan al que está fallando.

Velocidades típicas: hasta **1 Mbit/s** (a distancias cortas, decenas de metros), menos a más largo
(125 kbit/s para cientos de metros). La velocidad la fija el **bit timing**, no es libre: todos los
nodos del bus tienen que estar a la **misma** baudrate.

## La estructura de un frame CAN

Un *data frame* CAN lleva, en orden, estos campos (simplificado):

```
 SOF | Identificador (11 o 29 bits) | RTR | control(DLC) | Datos (0..8 bytes) | CRC | ACK | EOF
  └ start of frame (1 bit dominante que arranca todo)
```

- **Identificador (ID):** es a la vez el **nombre** del mensaje y su **prioridad**. Dos formatos:
  - **Estándar (11 bits):** hasta 2048 IDs. Es el más común.
  - **Extendido (29 bits):** millones de IDs; se distingue por el bit **IDE**. Convive con el estándar
    en el mismo bus.
- **RTR (Remote Transmission Request):** marca un **remote frame**, un pedido. Un nodo puede mandar un
  frame con RTR=1, DLC=N y **sin datos**: significa "quien tenga el dato del ID X, mandalo". El nodo
  dueño del dato responde con un *data frame* del mismo ID. Es un "pull" en un bus que normalmente es
  "push".
- **DLC (Data Length Code):** 4 bits que dicen **cuántos bytes** de datos vienen (0 a 8). Valores 9..15
  se interpretan como 8.
- **Datos:** de **0 a 8 bytes**. Esa carga útil chica obliga a diseñar bien qué mandás: CAN es para
  **muchos mensajes cortos y frecuentes**, no para transferir archivos.
- **CRC:** 15 bits de chequeo. Si no cuadra, el frame se descarta y se pide retransmisión.
- **ACK:** el transmisor manda este bit *recesivo* y **cualquier** receptor que recibió bien el frame lo
  pisa con un bit *dominante*. Así el transmisor sabe que **al menos uno** lo escuchó. (Ojo con esto si
  hay un solo nodo en el bus: ver casos de borde más abajo.)
- **Bit stuffing:** después de 5 bits iguales seguidos, el transmisor inserta uno opuesto para mantener
  la sincronización de reloj (CAN usa NRZ, sin línea de clock separada). El receptor los quita. Un error
  de stuffing es uno de los errores que detecta el protocolo.

## El arbitraje bit a bit (lo más elegante de CAN)

En CAN, un bit puede ser:

- **Dominante (0):** "gana". En el cable, lleva la línea al estado activo.
- **Recesivo (1):** "cede". Es el estado de reposo del bus.

Si **un solo** nodo pone dominante y otro recesivo **al mismo tiempo**, el bus queda en **dominante**.
Es un AND cableado: el 0 manda. Cada transmisor, mientras manda, **escucha** el bus.

Cuando dos nodos arrancan a transmitir juntos, van enviando su ID bit a bit, del más significativo al
menos significativo. Mientras los dos mandan lo mismo, no pasa nada. En el primer bit en que difieren,
uno manda recesivo (1) y otro dominante (0): el bus queda en 0. El que mandó **1 pero leyó 0** se da
cuenta de que **perdió** el arbitraje, se calla **sin corromper nada** y reintenta cuando el bus se
libere. El que mandó 0 sigue, sin enterarse siquiera de que hubo competencia.

Resultado: **gana el ID numéricamente más bajo** (más ceros arriba = más prioritario), sin colisión y
sin perder tiempo. Por eso en CAN la prioridad se diseña **eligiendo los IDs**: a los mensajes urgentes
(un freno) les ponés IDs bajos; a los de relleno (temperatura de cabina), IDs altos.

> Consecuencia de diseño que el datasheet no te dice: **dos nodos no pueden usar el mismo ID** para
> mensajes distintos. Si dos transmiten el mismo ID con datos diferentes, ganan el arbitraje los dos
> (los IDs son iguales) y se destruyen entre sí en el campo de datos -> error. Un ID = un emisor.

## El bit timing en detalle (`CANBTR`)

Esta es la parte que el material superficial esconde detrás del driver, pero conviene entenderla porque
es **la** fuente de errores cuando un bus "no anda".

Cada bit del bus se divide en **time quanta** (`tq`), pedacitos de tiempo iguales. El `tq` sale del
reloj del periférico (`PCLK_CAN`) dividido por un **prescaler**:

```
t_q = (BRP + 1) / PCLK_CAN
```

Un bit nominal se arma con varios `tq`, repartidos en segmentos:

```
| SYNC_SEG | TSEG1 (=PROP+PHASE1) | TSEG2 (=PHASE2) |
   1 tq       (TESG1+1) tq          (TESG2+1) tq
            ^---- aquí está el sample point (entre TSEG1 y TSEG2)
```

- **SYNC_SEG:** siempre **1 tq**. Donde se espera el flanco de la señal; sirve para sincronizar.
- **TSEG1:** retardo desde el sync hasta el **punto de muestreo**. Absorbe el tiempo de propagación por
  el cable más una fase de ajuste.
- **TSEG2:** del punto de muestreo al siguiente sync.
- **Sample point:** el instante en que **todos** los nodos leen el valor del bit. Suele ponerse al
  **75–87,5 %** del bit. Si lo ponés muy temprano o muy tarde, el bus es frágil a la propagación.
- **SJW (Synchronization Jump Width):** cuántos `tq` puede el nodo estirar o encoger un bit para
  **re-sincronizarse** con los flancos del bus y tolerar la deriva entre osciladores. Típico 1.
- **SAM:** 0 = se muestrea **una** vez (recomendado para buses rápidos); 1 = **tres** veces y se decide
  por mayoría (filtra spikes en buses lentos).

El **tiempo de bit nominal** (de aquí sale la baudrate) es, en `tq`:

```
NBT (en tq) = 1 (SYNC) + (TESG1 + 1) + (TESG2 + 1) = TESG1 + TESG2 + 3
baudrate = PCLK_CAN / ( (BRP+1) * (TESG1 + TESG2 + 3) )
```

Fijate que `TESG1 + TESG2 + 3` es **exactamente** la fórmula que usa el driver (la llama `NT`, *nominal
time*). Despejar `CANBTR` para una baudrate dada es: elegís un `NT` (entre ~8 y 24 `tq`), comprobás que
`PCLK_CAN / baudrate` sea múltiplo de `NT` para que `BRP` salga entero, y repartís `NT` entre `TSEG1` y
`TSEG2` cuidando el sample point (regla práctica: `TSEG1 >= 2*TSEG2`).

**Layout del registro `CANBTR`** (confirmado en el manual y en `LPC17xx.h`):

| Bits | Campo | Significado |
|------|-------|-------------|
| 9:0  | `BRP`   | Prescaler; el `tq` es `(BRP+1)` ciclos de `PCLK_CAN` |
| 15:14| `SJW`   | Ancho de salto de sincronización, `(SJW+1)` `tq` |
| 19:16| `TESG1` | Largo de TSEG1, `(TESG1+1)` `tq` |
| 22:20| `TESG2` | Largo de TSEG2, `(TESG2+1)` `tq`; define el bit nominal |
| 23   | `SAM`   | 0 = 1 muestra, 1 = 3 muestras |

> Cuidado: `CANBTR` **solo se puede escribir en Reset Mode** (`MOD.RM = 1`). Si intentás cambiar la
> baudrate "en caliente", el write se ignora. El driver entra a reset, escribe y vuelve a normal.

Ejemplo a mano, **500 kbit/s** con `PCLK_CAN = 12,5 MHz` (CCLK 100 MHz dividido por 8; ver clock abajo):
`12.5e6 / 500e3 = 25` -> elegimos `NT = 25`... no es par; probá `PCLK_CAN = 50 MHz` (CCLK/2):
`50e6 / 500e3 = 100`. Con `NT = 20`: `BRP = 100/20 - 1 = 4`. Repartiendo `NT-1 = 19` -> `TESG2 = 19/3 -
1 = 5`, `TESG1 = 19 - 6 - 1 = 12`. Sample point ≈ `(1+13)/20 = 70 %`. En general **dejás que el driver
calcule esto**, pero ahora sabés qué está haciendo (y por qué a veces "no encuentra" una combinación
exacta: si `PCLK_CAN/baudrate` no es múltiplo de ningún `NT` válido, el driver se cuelga en un `while(1)`,
ver casos de borde).

## Clock y pines del CAN

- **Encendido:** `PCONP` bits 13 (`PCAN1`) y 14 (`PCAN2`).
- **Clock del periférico:** `PCLKSEL0` define `PCLK_CAN1`, `PCLK_CAN2` y `PCLK_ACF` (el del filtro de
  aceptación). **Los tres tienen que tener el MISMO divisor.** El driver los pone todos a CCLK/2. Y si
  vas a más de 100 kbit/s, **no** uses el IRC como fuente de reloj del sistema: su precisión no alcanza.
- **Pines** (función 1 en `PINSEL`):

| Señal | Opciones |
|-------|----------|
| CAN1 RD (RX) | P0.0 o P0.21 |
| CAN1 TD (TX) | P0.1 o P0.22 |
| CAN2 RD (RX) | P0.4 o P2.7 |
| CAN2 TD (TX) | P0.5 o P2.8 |

`RD`/`TD` van al **transceiver** externo, no al cable diferencial directamente (ver la página 04b).

## Los registros del controlador

Cada controlador (CAN1 en `0x40044000`, CAN2 en `0x40048000`) tiene este mapa, que en `LPC17xx.h` es la
struct `LPC_CAN_TypeDef`:

| Registro | Campo struct | Para qué |
|----------|--------------|----------|
| `CANMOD`  | `MOD`  | Modo de operación (reset, listen-only, self-test, sleep, ...) |
| `CANCMR`  | `CMR`  | Comandos (write-only): pedir transmisión, abortar, liberar buffer Rx, etc. |
| `CANGSR`  | `GSR`  | Estado global + **contadores de error** RXERR/TXERR |
| `CANICR`  | `ICR`  | Interrupt + Capture: qué interrumpió, *Arbitration Lost* y *Error Code* |
| `CANIER`  | `IER`  | Habilitación de cada interrupción |
| `CANBTR`  | `BTR`  | **Bit timing** (la sección de arriba) |
| `CANEWL`  | `EWL`  | Error Warning Limit (default 96) |
| `CANSR`   | `SR`   | Status detallado por buffer (Rx y los 3 Tx) |
| `CANRFS` / `CANRID` / `CANRDA` / `CANRDB` | `RFS/RID/RDA/RDB` | **Buffer de recepción**: info de frame, ID, datos 1-4 y 5-8 |
| `CANTFI1..3` / `CANTID1..3` / `CANTDA1..3` / `CANTDB1..3` | `TFI/TID/TDA/TDB` (×3) | **3 buffers de transmisión** |

Además hay registros **centrales** compartidos (`LPC_CANCR_TypeDef` en `0x40040000`): `CANTxSR`,
`CANRxSR`, `CANMSR` te dan el estado de Tx/Rx/errores de **ambos** controladores de un vistazo.

### CANMOD: los modos

`MOD` (en reset vale 1, o sea **arranca en Reset Mode**). Bits:

| Bit | Símbolo | Modo |
|-----|---------|------|
| 0 | `RM`  | **Reset Mode**: el controlador se desconecta del bus. **Obligatorio** para escribir `BTR`, `EWL` y los contadores de error. Casi toda la config se hace acá. |
| 1 | `LOM` | **Listen Only**: escucha pero **no manda ACK ni error flags**. Ideal para *sniffear* un bus sin perturbarlo, o para auto-detectar baudrate. |
| 2 | `STM` | **Self Test Mode**: permite recibir el propio mensaje **sin** ACK de otros nodos (con *Self Reception Request*). Para probar un nodo solo. Solo se setea en Reset Mode. |
| 3 | `TPM` | **Transmit Priority Mode**: con los 3 buffers Tx ocupados, decide el orden por el campo de prioridad (`TFI.PRIO`) en vez de por número de buffer. |
| 4 | `SM`  | **Sleep Mode**: bajo consumo; despierta por actividad en el bus o por el bus. |
| 5 | `RPM` | **Receive Polarity Mode**: invierte la polaridad del pin RD (raro). |
| 7 | `TM`  | **Test Mode**: control directo del pin TD (fábrica/test). |

El flujo típico: entrás a Reset (`MOD=RM`), configurás `BTR` y filtros, salís a normal (`MOD=0`). Eso
hace `CAN_Init`.

### CANCMR: comandos

`CMR` es write-only; escribís un bit para **disparar** una acción:

| Bit | Símbolo | Acción |
|-----|---------|--------|
| 0 | `TR`   | Transmission Request: arrancá a transmitir el buffer seleccionado |
| 1 | `AT`   | Abort Transmission |
| 2 | `RRB`  | Release Receive Buffer (libera el buffer Rx para el próximo mensaje) |
| 3 | `CDO`  | Clear Data Overrun |
| 4 | `SRR`  | **Self Reception Request** (transmitir y recibir el propio frame) |
| 5/6/7 | `STB1/STB2/STB3` | seleccionar cuál de los 3 buffers Tx se usa |

Para transmitir: cargás un buffer Tx libre (TFI/TID/TDA/TDB), y mandás `CMR = STBn | TR`. Para
self-test/self-reception: `CMR = STBn | SRR`.

### CANGSR y los contadores de error

`GSR` (read-only en operación) trae flags de estado (`RBS` hay mensaje recibido, `TBS` buffer Tx libre,
`ES` error status, `BS` bus status) y, lo más importante, **los contadores de error**:

```
RXERR = (CANxGSR & 0x00FF0000) >> 16
TXERR = (CANxGSR & 0xFF000000) >> 24
```

Estos contadores son el corazón del **manejo de errores** de CAN.

### CANICR / CANIER: interrupciones

`IER` habilita; `ICR` te dice qué pasó (y **se limpia al leerlo**, así que se lee una sola vez por ISR).
Las fuentes: `RI` (recepción), `TI1/TI2/TI3` (transmisión por buffer), `EI` (error warning), `DOI` (data
overrun), `WUI` (wake-up), `EPI` (error passive), `ALI` (arbitration lost) y `BEI` (bus error). `ICR`
captura además, en sus bits altos, **en qué bit** se perdió el arbitraje (*Arbitration Lost Capture*) y
**qué tipo** de error y dónde (*Error Code Capture*): oro puro para depurar un bus que falla.

## Estados y manejo de errores (lo que hace a CAN "confiable")

CAN no tira un mensaje malo y se olvida: lleva la cuenta. Cada nodo tiene `TXERR` y `RXERR`, y según
ellos está en uno de tres estados:

- **Error Active (normal):** errores por debajo de ~127. Cuando detecta un error, manda un *active error
  flag* (6 bits dominantes) que aborta el frame en todo el bus, forzando la retransmisión. Participa
  plenamente.
- **Error Passive:** algún contador pasó **127**. Sigue funcionando, pero ahora manda *passive error
  flags* (recesivos), que **no** molestan al resto: el nodo "sospechoso" baja el perfil. Se genera la
  *Error Passive Interrupt* (`EPI`) si está habilitada.
- **Bus Off:** `TXERR` pasó **255**. El nodo se **desconecta del bus** por completo (no transmite ni
  recibe), para no contaminar la red con un hardware fallado. Esta es la "auto-desconexión" que da fama
  de robusto a CAN.

**Recuperación de Bus Off** (importante y poco intuitiva): el controlador entra en Reset Mode y pone
`TXERR = 127`. Queda ahí hasta que **el CPU baje el Reset Mode**. Recién entonces el controlador espera
la cuenta protocolar (**128 ocurrencias de 11 bits recesivos seguidos**, "bus free") descontando
`TXERR`; cuando llega a 0, vuelve a Bus-On. O sea: de Bus Off **no se sale solo**, tu firmware tiene que
intervenir limpiando el Reset Mode (típicamente desde la ISR de bus error). Leer `TXERR` durante ese
proceso te dice cuánto falta para recuperar.

El `EWL` (Error Warning Limit, default **96**) marca el umbral en que se levanta `ES` y, si está
habilitada, la *Error Warning Interrupt*: un "aviso temprano" antes de caer en passive.

## Lo que te llevás (parte 1)

- CAN es un **bus multinodo** por **contenido**: mensajes con **ID** (= nombre y prioridad), no destino.
- El **arbitraje bit a bit** (dominante 0 gana a recesivo 1) hace que el ID más bajo gane **sin
  colisión**. La prioridad la diseñás eligiendo IDs, y **un ID = un emisor**.
- El **frame** lleva ID (11/29 bits), RTR, DLC, 0–8 bytes, CRC, ACK y bit stuffing.
- El **bit timing** (`CANBTR`: BRP, SJW, TSEG1, TSEG2, SAM) reparte cada bit en `tq` y fija la baudrate;
  solo se escribe en Reset Mode, y todos los nodos van a la misma velocidad.
- Los **contadores de error** llevan al nodo de *error active* a *passive* a **Bus Off**; de Bus Off se
  sale **con ayuda del firmware**.
- Faltan el **filtro de aceptación**, la recepción por interrupción, el transceiver y el driver: eso está
  en la página siguiente.

## Referencias
- Manual, Cap. 16: [`../../manual/ch16_can1-2.pdf`](../../manual/ch16_can1-2.pdf)
- Header CMSIS: [`lpc17xx_can.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_can.h)
- Ejemplos: [`../../library/examples/CAN/`](../../library/examples/CAN/)

---

**Anterior:** [03 - QEI](./03-qei.md) · **Módulo:** [Periféricos adicionales](./README.md) ·
**Siguiente:** [04b - CAN: filtro de aceptación y driver](./04b-can-filtro.md)
