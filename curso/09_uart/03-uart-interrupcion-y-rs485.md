# UART por interrupción, RS-485 y flow control

Hasta acá transmitimos y recibimos por polling: el `main` se queda esperando. Esta página cierra el
módulo con lo que hace a la UART usable en serio: recibir/transmitir **por interrupción** (sin
bloquear), detectar y manejar **errores de línea**, y los modos de UART1 (**flow control RTS/CTS** y
**RS-485 multidrop**).

## Interrupciones: IER, IIR y la prioridad interna

Tres registros entran en juego:

- **`IER`** (Interrupt Enable, DLAB=0): habilitás qué fuentes pueden interrumpir.
- **`IIR`** (Interrupt Identification, lectura): cuando salta la interrupción, leés `IIR` para saber
  **por qué**. El periférico tiene un solo vector (`UART0_IRQn`, etc.), así que `IIR` es la forma de
  desambiguar.
- **`FCR`** (FIFO Control, escritura, misma dirección que `IIR`): habilita las FIFOs, fija el trigger
  de RX y permite resetearlas.

### Las fuentes de `IER`

| Constante (driver) | Bit | Interrumpe cuando |
|--------------------|-----|-------------------|
| `UART_INTCFG_RBR` | 0 | hay datos para leer (RDA, según el trigger) o por time-out (CTI) |
| `UART_INTCFG_THRE` | 1 | el `THR` quedó vacío (espacio para transmitir) |
| `UART_INTCFG_RLS` | 2 | error de línea (overrun, parity, framing, break) |

Con el driver: `UART_IntConfig(UARTx, UART_INTCFG_RBR, ENABLE);`

### Identificación en `IIR` y el orden de prioridad

`IIR` te da, en los bits 3:1, el código de la causa **de mayor prioridad pendiente**. El bit 0
(`INTSTAT`) es **activo en bajo**: vale 0 cuando hay una interrupción pendiente. Las causas, de mayor a
menor prioridad:

| Prioridad | Código (`IIR[3:1]`) | Constante driver | Causa | Cómo se limpia |
|-----------|---------------------|------------------|-------|----------------|
| 1 (alta) | `011` | `UART_IIR_INTID_RLS` | Receive Line Status (error) | leer `LSR` |
| 2 | `010` | `UART_IIR_INTID_RDA` | Receive Data Available (llegó al trigger) | leer `RBR` hasta bajar del trigger |
| 2 (compartida) | `110` | `UART_IIR_INTID_CTI` | Character Time-out Indicator | leer `RBR` |
| 3 (baja) | `001` | `UART_IIR_INTID_THRE` | Transmit Holding Register Empty | leer `IIR` o escribir `THR` |

RDA y CTI **comparten** el segundo nivel de prioridad (son dos caras de "hay datos en la RX FIFO");
THRE es la de menor prioridad.

Detalles que importan:

- **Leer `IIR` puede limpiar la causa THRE**. Por eso, en el handler, leé `IIR` una sola vez al
  principio y guardalo; no lo leas dos veces o perdés información.
- El **CTI (Character Time-out)** es el seguro contra trigger alto: si en la RX FIFO quedaron bytes
  sin alcanzar el nivel de disparo y no hubo actividad durante 3.5 a 4.5 tiempos de carácter, salta
  CTI para que no queden datos atrapados. **En el handler, RDA y CTI se tratan igual**: en ambos casos vaciás la RX
  FIFO leyendo. Por eso vas a ver `if (id == RDA || id == CTI) recibir();`.
- El código `000` (`UART1_IIR_INTID_MODEM`, prioridad la más baja) solo existe en UART1: cambió una
  señal de modem (CTS/DSR/RI/DCD).

### Manejo de errores de línea (RLS)

Cuando `IIR` indica `RLS`, leés `LSR` y mirás los flags de error: `OE` (overrun, perdiste un byte),
`PE` (paridad), `FE` (framing, casi siempre baudrate mal), `BI` (break), `RXFE` (hay un carácter con
error en la FIFO). Leer `LSR` **limpia** los cuatro flags de error (OE/PE/FE/BI). PE/FE/BI viajan
pegados al carácter del frente de la FIFO, así que para identificarlos contra el byte correcto: leé
`LSR`, después `RBR`.

```c
void uart_handle_error(uint8_t lsr) {
    if (lsr & UART_LSR_OE) { /* overrun: se perdió un byte; quizás subir el trigger o ir más rápido leyendo */ }
    if (lsr & UART_LSR_PE) { /* parity: ruido o paridad mal acordada */ }
    if (lsr & UART_LSR_FE) { /* framing: casi seguro baudrate mal en algún extremo */ }
    if (lsr & UART_LSR_BI) { /* break: el otro extremo mantuvo la línea en 0 */ }
}
```

## Eco por interrupción (el patrón mínimo)

Versión simple: trigger de 1 carácter, RX por interrupción, TX por polling dentro del handler. Sigue
los patrones del [módulo 7](../07_interrupciones/): variables `volatile`, handler corto, la causa se
limpia leyendo el registro de datos.

```c
#include "lpc17xx_uart.h"

volatile uint8_t rx_dato;
volatile uint8_t rx_listo = 0;

void UART0_IRQHandler(void) {
    uint32_t id = UART_GetIntId((LPC_UART_TypeDef *)LPC_UART0) & UART_IIR_INTID_MASK;

    if (id == UART_IIR_INTID_RLS) {                  // error: hay que limpiarlo SIEMPRE
        uint8_t lsr = UART_GetLineStatus((LPC_UART_TypeDef *)LPC_UART0);
        (void)lsr;                                   // leer LSR limpia OE/BI
    }
    else if (id == UART_IIR_INTID_RDA || id == UART_IIR_INTID_CTI) {
        rx_dato  = UART_ReceiveByte((LPC_UART_TypeDef *)LPC_UART0);  // leer RBR limpia la causa
        rx_listo = 1;
    }
}

int main(void) {
    uart0_init();                                    // como en la página 2
    UART_IntConfig((LPC_UART_TypeDef *)LPC_UART0, UART_INTCFG_RBR, ENABLE);
    UART_IntConfig((LPC_UART_TypeDef *)LPC_UART0, UART_INTCFG_RLS, ENABLE);  // que avise de errores
    NVIC_EnableIRQ(UART0_IRQn);

    while (1) {
        if (rx_listo) {
            rx_listo = 0;
            UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, rx_dato);   // eco, sin haber bloqueado nunca
        }
        // ...el main queda libre para otras tareas...
    }
}
```

## TX por interrupción y el ring buffer

El caso anterior transmite por polling, que para un eco está bien. Pero si querés mandar un bloque
grande sin bloquear, usás la interrupción **THRE** junto a un **ring buffer** (cola circular). La idea:

1. Tu función `enviar()` mete los bytes en el buffer de TX, arranca la transmisión cargando el primer
   bloque en la FIFO y habilita la interrupción THRE.
2. Cada vez que el `THR` queda vacío, el handler saca bytes del buffer y los carga en la FIFO (hasta
   16 de una). Cuando el buffer se vacía, **desactiva** la THRE: ya no hay nada que cargar y esa
   interrupción no aporta.

Este es exactamente el diseño del ejemplo CMSIS `examples/UART/Interrupt/uart_interrupt_test.c`:
ring buffers separados de RX y TX, y la THRE se habilita/deshabilita dinámicamente. El detalle clave a
recordar:

> **La interrupción THRE se limpia escribiendo `THR` o leyendo `IIR`** (cuando es la causa que `IIR`
> reporta); el flag `LSR.THRE`, en cambio, sigue en 1 mientras no haya nada que mandar. Ojo con otro
> detalle del manual: la lógica del THRE mete un retardo de arranque de ~1 carácter, así que habilitar
> la interrupción con el transmisor ya vacío **no garantiza un disparo inmediato**. Por eso el ejemplo
> CMSIS "ceba" la transmisión a mano (carga el primer bloque en la FIFO con `UART_IntTransmit()`) y
> recién después habilita THRE; y la deshabilita en cuanto el buffer de salida queda vacío.

Esqueleto reducido:

```c
#define BUFSZ 256
volatile uint8_t txbuf[BUFSZ];
volatile uint32_t tx_head = 0, tx_tail = 0;

static int tx_empty(void) { return tx_head == tx_tail; }

void uart0_send_async(const uint8_t *p, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        txbuf[tx_head] = p[i];
        tx_head = (tx_head + 1) % BUFSZ;
    }
    // Si el TX estaba ocioso, "cebar": cargar el primer bloque en la FIFO a mano
    // (misma lógica que la rama THRE del handler) y recién ahí habilitar THRE.
    cargar_fifo_desde_buffer();
    UART_IntConfig((LPC_UART_TypeDef *)LPC_UART0, UART_INTCFG_THRE, ENABLE);
}

// dentro de UART0_IRQHandler, rama THRE:
//   while (!tx_empty() && hay_lugar_en_FIFO) cargar THR desde el buffer;
//   if (tx_empty()) UART_IntConfig(..., UART_INTCFG_THRE, DISABLE);
```

## Recepción por DMA (cuando el stream es alto y constante)

Para flujos rápidos y continuos, hasta el handler por byte puede ser demasiada carga. La UART tiene
soporte de **DMA** (GPDMA, módulo 11): activás el bit `DMAMODE` en `FCR` (`UART_FCR_DMAMODE_SEL`, o
`FIFO_DMAMode = ENABLE` en `UART_FIFOConfig`) y configurás un canal del GPDMA con la UART como
periférico fuente (RX) o destino (TX). El DMA mueve los bytes a/desde memoria sin CPU; vos solo
atendés la interrupción de "transferencia completa" del DMA. El ejemplo de referencia es
`examples/UART/DMA/uart_dma_test.c`. Lo dejamos para el módulo 11; acá basta saber que existe y cuándo
conviene: muchos datos, a alta velocidad, de forma sostenida.

## Auto-baud (ACR): cuando no sabés a qué velocidad te hablan

El registro `ACR` permite que el hardware **mida** el baudrate de un carácter entrante y cargue
`DLL`/`DLM` solo. El protocolo del manual se basa en el "AT" de los módems Hayes: el emisor manda una
`'A'` (0x41) o `'a'` (0x61) y la UART cronometra los flancos de bajada del start bit y del LSB
(modo 0), o solo el ancho del start bit (modo 1). Ojo: **el divisor fraccional queda desconectado
durante el auto-baud** (el hardware no toca `FDR`), así que conviene tenerlo desactivado
(`DIVADDVAL = 0`) cuando lo uses. Con el
driver: cargás `UART_AB_CFG_Type` (modo 0 o 1, con o sin auto-restart) y llamás `UART_ABCmd(...)`. El
fin de la medición avisa por la interrupción `ABEO` (end of auto-baud) o, si falló, `ABTO`
(time-out); se habilitan con los bits 8 y 9 del `IER` (`UART_INTCFG_ABEO`/`UART_INTCFG_ABTO` en el
driver) y se limpian con los bits de clear del propio `ACR`. Es útil cuando un mismo firmware
tiene que hablar con equipos a velocidades distintas sin reconfigurar.
Ejemplo: `examples/UART/AutoBaud/uart_autobaud_test.c`.

---

## UART1: lo que las otras no tienen

UART1 agrega control de **módem** (señales RTS/CTS/DSR/DTR/DCD/RI) y modo **RS-485**. Por eso usa el
tipo propio `LPC_UART1_TypeDef` (con `MCR`, `MSR`, `RS485CTRL`, `ADRMATCH`, `RS485DLY`) y **no se
castea** a `LPC_UART_TypeDef`.

### Flow control por hardware (RTS/CTS)

El control de flujo evita el overrun cuando el receptor no da abasto: en vez de perder bytes, le pide
al emisor que **pare**. Dos señales:

- **RTS** (Request To Send): salida. La UART la activa para decir "podés mandarme".
- **CTS** (Clear To Send): entrada. La UART solo transmite si CTS está activo.

UART1 puede manejarlas **automáticamente** (auto-RTS y auto-CTS), sin que el firmware mueva pines:

- **Auto-RTS**: el hardware desactiva RTS cuando la RX FIFO llega a su trigger, frenando al otro
  extremo, y la reactiva cuando la FIFO baja al nivel de trigger **anterior** (con trigger en 8, por
  ejemplo, reactiva al llegar a 4; no espera a que se vacíe). Vos no hacés nada.
- **Auto-CTS**: el transmisor chequea CTS antes de cada byte: si CTS se desactivó, termina el
  carácter en curso, se detiene, y retoma cuando CTS vuelve.

Con el driver (pines TXD1/RXD1/CTS1/RTS1 en función 2 del PINSEL, p. ej. P2.0/P2.1/P2.2/P2.7):

```c
UART_FullModemConfigMode((LPC_UART1_TypeDef *)LPC_UART1, UART1_MODEM_MODE_AUTO_RTS, ENABLE);
UART_FullModemConfigMode((LPC_UART1_TypeDef *)LPC_UART1, UART1_MODEM_MODE_AUTO_CTS, ENABLE);
```

`MCR` (Modem Control Register) es donde viven estos bits: `AUTO_RTS_EN` (6), `AUTO_CTS_EN` (7), más
control manual de DTR/RTS y un modo `LOOPBACK` (4) para auto-test (TX se conecta internamente a RX).
`MSR` (Modem Status Register) refleja el estado de las entradas (CTS/DSR/RI/DCD) y sus transiciones
("delta"); se lee con `UART_FullModemGetStatus`. Referencia:
`examples/UART/HWFlowControl/uart_hw_flow_control.c`.

### RS-485 / EIA-485: multidrop con un solo par

RS-485 es un bus **diferencial** y **half-duplex** donde **varios** nodos comparten el mismo par de
cables: un maestro y muchos esclavos (multidrop). Tiene dos problemas que UART1 resuelve por hardware:

**1. Direccionamiento (¿a qué esclavo le hablo?).** Se usa el **noveno bit**: el frame lleva paridad
configurada como "fija", y se aprovecha el bit de paridad como marca de "esto es una dirección, no un
dato". Cuando el bit está en 1, el byte es una **dirección de esclavo**; cuando está en 0, es un dato.

- **Normal Multidrop Mode (NMM)**: ante un byte-dirección, el hardware interrumpe y tu firmware decide
  si esa dirección es la suya; si lo es, habilita el receptor para los datos que siguen.
- **Auto Address Detection (AAD)**: cargás tu dirección en `ADRMATCH` y el hardware **filtra solo**:
  ignora todo hasta que llega una dirección que coincide, recién ahí habilita la recepción y te
  interrumpe. Cero trabajo de filtrado en software. Ejemplo:
  `examples/UART/RS485_Slave/rs485_slave.c`.

**2. Dirección del transceiver (¿transmito o escucho?).** Como el bus es half-duplex, hay un pin que
controla si el chip transceiver (típico MAX485) maneja la línea o la escucha. UART1 lo maneja con
**Auto Direction Control**: el hardware activa el pin de dirección (RTS o DTR) justo antes de
transmitir y lo libera al terminar, con un retardo configurable en `RS485DLY` (en períodos de baud
clock) para dar margen al transceiver. Esto evita el clásico bug de "corté el driver antes de que
saliera el último bit": por eso internamente espera a `TEMT`, no a `THRE`.

Configuración del maestro con el driver (`examples/UART/RS485_Master/rs485_master.c`):

```c
UART1_RS485_CTRLCFG_Type rs485;
rs485.AutoDirCtrl_State          = ENABLE;                  // hardware maneja la dirección
rs485.DirCtrlPin                 = UART1_RS485_DIRCTRL_DTR; // pin DTR como dirección
rs485.DirCtrlPol_Level           = SET;                     // polaridad del pin
rs485.DelayValue                 = 50;                      // RS485DLY (margen para el transceiver)
rs485.NormalMultiDropMode_State  = DISABLE;
rs485.AutoAddrDetect_State       = DISABLE;
rs485.MatchAddrValue             = 0;
UART_RS485Config((LPC_UART1_TypeDef *)LPC_UART1, &rs485);

UART_RS485SendSlvAddr((LPC_UART1_TypeDef *)LPC_UART1, 'A');     // manda un byte-dirección (9no bit=1)
UART_RS485SendData((LPC_UART1_TypeDef *)LPC_UART1, msg, len);   // manda datos (9no bit=0)
```

`RS485CTRL` es el registro detrás de todo esto: `NMM_EN` (0), `RX_DIS` (1, deshabilita el receptor),
`AADEN` (2, auto address detect), `SEL_DTR` (3, elige DTR vs RTS para dirección), `DCTRL_EN` (4, auto
direction), `OINV` (5, invierte la polaridad del pin de dirección).

## Casos de borde de esta página

| Síntoma | Causa |
|---------|-------|
| La TX por interrupción nunca arranca | habilitaste THRE sin "cebar" la FIFO; cargá el primer bloque a mano y deshabilitá THRE al vaciar el buffer |
| Se cuelga con trigger alto y pocos bytes | confiá en el CTI; si no llega, revisá que la interrupción RBR esté habilitada |
| RS-485 corta el último carácter | dirección manual del transceiver mal: usá auto-direction (espera TEMT) o esperá TEMT vos |
| RS-485: el esclavo recibe datos que no son para él | falta NMM/AAD, o `ADRMATCH` mal cargado |
| Flow control: igual perdés bytes | el otro extremo no respeta CTS, o no cableaste RTS↔CTS cruzados |
| Error de línea que reaparece y traba la FIFO | en RLS leés `LSR` pero no vaciás `RBR` del byte malo |

## Ejercicios

1. Eco por interrupción que cuente caracteres y, al recibir Enter (`\r`), responda con la cuenta.
2. TX por interrupción con ring buffer: mandá un texto largo (>16 bytes) sin bloquear el `main`.
3. Comando simple no bloqueante: `'1'` prende un LED, `'0'` lo apaga, todo desde el handler de RX.
4. (Avanzado) Dos placas en RS-485: maestro que direcciona a dos esclavos con AAD y recibe sus ACK.

---

**Anterior:** [02 - UART con el driver](./02-uart-con-driver.md) ·
**Módulo:** [UART](./README.md) · **Siguiente módulo:** [10 - ADC/DAC](../10_adc_dac/)
