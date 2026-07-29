# I2C por interrupción y como esclavo

Las dos páginas anteriores hicieron I2C **en polling** y **como maestro**. Esta cierra el módulo con
los dos modos que faltan: atender el bus **por interrupción** (sin bloquear el CPU) y poner al LPC a
trabajar **como esclavo**. Acá es donde la máquina de estados de la página 01 se vuelve indispensable:
una interrupción de I2C es, literalmente, un `switch (I2STAT)`.

## I2C por interrupción: la idea

En polling, `I2C_MasterTransferData(..., I2C_TRANSFER_POLLING)` se queda en un `while` empujando la
máquina de estados hasta terminar. Eso desperdicia CPU: a 100 kHz, leer 2 bytes tarda ~0.5 ms, y en
todo ese tiempo el micro no hace nada más.

Por interrupción es distinto. Cada vez que el hardware completa un paso del protocolo, levanta `SI`,
**dispara la interrupción de I2C** (`I2C0_IRQn`, `I2C1_IRQn`, `I2C2_IRQn` en el NVIC), atendés ese
único paso, limpiás `SI`, y el CPU sigue con lo suyo hasta el próximo paso. La transacción "avanza
sola" en background, peldaño por peldaño.

### Con el driver: `I2C_TRANSFER_INTERRUPT` + callback

El driver ya trae la máquina de estados por interrupción lista (`I2C_MasterHandler`). Solo tenés que:

1. Enganchar el handler del driver a la ISR del NVIC.
2. Habilitar la interrupción.
3. Lanzar la transferencia en modo `I2C_TRANSFER_INTERRUPT` con un `callback`.

```c
#include "lpc17xx_i2c.h"

volatile int listo = 0;
uint8_t rx[2];

void i2c_done(void) { listo = 1; }   // callback: corre al terminar la transacción

// La ISR real del vector: el driver hace todo el switch(I2STAT) por vos
void I2C0_IRQHandler(void) { I2C_MasterHandler(LPC_I2C0); }

void leer_lm75_async(void)
{
    static uint8_t tx = 0x00;
    static I2C_M_SETUP_Type t;            // static: el driver la usa desde la ISR
    t.sl_addr7bit = 0x48;
    t.tx_data = &tx;  t.tx_length = 1;
    t.rx_data = rx;   t.rx_length = 2;
    t.retransmissions_max = 3;
    t.callback = i2c_done;                // se llama al completar

    listo = 0;
    I2C_IntCmd(LPC_I2C0, ENABLE);         // habilita I2C0_IRQn en el NVIC
    I2C_MasterTransferData(LPC_I2C0, &t, I2C_TRANSFER_INTERRUPT);  // NO bloquea
}
```

Dos detalles importantes:

- La `I2C_M_SETUP_Type` tiene que **seguir viva** durante toda la transferencia (la ISR la lee). Por
  eso va `static` o global, **nunca** una variable local que muere al volver de la función.
- `I2C_MasterTransferData` en modo interrupt **vuelve enseguida** (no bloquea). La transacción
  termina más tarde, cuando se dispara tu `callback`. Mientras tanto no toques los buffers.

### A mano: el esqueleto del `switch (I2STAT)` en la ISR

Para que veas qué hace el driver por dentro (y porque a registro vas a tener que escribirlo vos), así
se ve un master-transmitter atendido por interrupción. Es la misma lógica de la página 01, pero en vez
de un `while` que espera `SI`, **cada vuelta es una llamada a la ISR**:

```c
#define STA  (1u<<5)
#define STO  (1u<<4)
#define SI   (1u<<3)
#define AA   (1u<<2)

volatile uint8_t  *tx_ptr;
volatile uint32_t  tx_len, tx_idx;
volatile int       tx_listo, tx_error;

void I2C0_IRQHandler(void)
{
    switch (LPC_I2C0->I2STAT) {

    case 0x08:                              // START enviado
        LPC_I2C0->I2DAT = (DIR7 << 1);      // SLA+W
        LPC_I2C0->I2CONCLR = STA | SI;      // bajar STA, soltar SI
        break;

    case 0x18:                              // SLA+W + ACK
    case 0x28:                              // dato + ACK
        if (tx_idx < tx_len) {
            LPC_I2C0->I2DAT = tx_ptr[tx_idx++];
            LPC_I2C0->I2CONCLR = SI;
        } else {                            // no hay más datos -> STOP
            LPC_I2C0->I2CONSET = STO;
            LPC_I2C0->I2CONCLR = SI;
            tx_listo = 1;
        }
        break;

    case 0x20:                              // SLA+W + NACK (nadie contestó)
    case 0x30:                              // dato + NACK
        LPC_I2C0->I2CONSET = STO;
        LPC_I2C0->I2CONCLR = SI;
        tx_error = 1;
        break;

    case 0x38:                              // arbitraje perdido
        LPC_I2C0->I2CONSET = STA;           // reintentar cuando el bus quede libre
        LPC_I2C0->I2CONCLR = SI;
        break;

    default:
        LPC_I2C0->I2CONCLR = SI;            // estado inesperado: liberar y seguir
        break;
    }
}
```

Fijate el patrón invariable de **cada `case`**: tocás `I2DAT` y/o preparás `STA`/`STO`/`AA`, y siempre
terminás bajando `SI` con `I2CONCLR` (acordate de la página 01: SET y CLEAR separados para no pisar
flags del hardware). Para arrancar la transacción seteás `STA` una vez desde el código principal y
después la ISR la lleva hasta el `STOP` sola.

## El LPC como esclavo

Hasta acá el LPC fue el maestro. Pero también puede ser **esclavo**: otro micro (o una Raspberry, o
un maestro cualquiera) lo direcciona y le escribe o le pide datos. Esto sirve para hacer un
"periférico inteligente", repartir trabajo entre dos placas, etc.

### Configurar la dirección propia: I2ADR0..3 e I2MASK0..3

Un esclavo tiene que **reconocer su propia dirección**. El LPC tiene **4 registros de dirección**
(`I2ADR0` a `I2ADR3`): puede responder a hasta 4 direcciones distintas a la vez. Cada uno trae:

- Bits 7:1 → la dirección de 7 bits propia.
- Bit 0 → **GC** (General Call enable): si está en 1, el esclavo también responde a la dirección
  **General Call** (`0x00`), que es un "broadcast" a todos los esclavos del bus.

Además, cada dirección tiene su **máscara** (`I2MASK0..3`): cada bit en 1 de la máscara hace que ese
bit de la dirección **se ignore** en la comparación. Sirve para responder a un *rango* de direcciones
(por ejemplo, emular varios chips). Si querés una sola dirección exacta, dejá la máscara en 0.

Y un bit clave del control: para entrar en modo esclavo, **`AA` tiene que estar en 1**. Si `AA=0`, el
esclavo no reconoce su dirección ni el General Call (queda "sordo").

### Con el driver

```c
#include "lpc17xx_i2c.h"

#define MI_DIR_7BIT  0x48      // la dirección con la que me van a llamar

uint8_t buf_rx[16];

void i2c_slave_init(void)
{
    // (pines + I2C_Init igual que como maestro)
    I2C_OWNSLAVEADDR_CFG_Type me;
    me.SlaveAddrChannel  = 0;          // usar I2ADR0
    me.SlaveAddr_7bit    = MI_DIR_7BIT;
    me.GeneralCallState  = ENABLE;     // responder también al General Call (0x00)
    me.SlaveAddrMaskValue = 0;         // sin máscara -> dirección exacta
    I2C_SetOwnSlaveAddr(LPC_I2C0, &me);

    I2C_Cmd(LPC_I2C0, ENABLE);
}

void i2c_slave_recibir(void)
{
    I2C_S_SETUP_Type s;
    s.tx_data = NULL;  s.tx_length = 0;
    s.rx_data = buf_rx; s.rx_length = sizeof(buf_rx);
    // En polling, bloquea hasta que un maestro nos direccione y nos escriba:
    I2C_SlaveTransferData(LPC_I2C0, &s, I2C_TRANSFER_POLLING);
}
```

`I2C_SlaveTransferData` espera a que un maestro nos llame. Cuando el maestro nos **escribe**, lo que
manda queda en `rx_data`; cuando el maestro nos **lee**, el driver entrega lo que pusiste en
`tx_data` (si el maestro pide más bytes de los que tenés, manda `0xFF` de relleno). También funciona
por `I2C_TRANSFER_INTERRUPT` con su `I2C_SlaveHandler` y callback, igual que el maestro.

### Los códigos de estado del esclavo

Por interrupción a registro, el esclavo es otro `switch (I2STAT)`, con los códigos de esclavo de la
página 01 (Tabla 401 para slave-receiver y Tabla 402 para slave-transmitter del manual):

| Código | Qué pasó | Qué hacés |
|--------|----------|-----------|
| `0x60` | te llamaron por tu dir.+W, diste ACK | prepararte para recibir; AA=1, limpiar SI |
| `0x70` | recibiste un **General Call**, ACK | idem (es un broadcast) |
| `0x80` | te llegó un dato (estás direccionado) | leer `I2DAT`, guardarlo; AA=1 si querés más |
| `0x88` | dato recibido y vos diste NACK | buffer lleno; volver a estado "no direccionado" |
| `0xA0` | el maestro hizo STOP o repeated START | transacción terminada; AA=1 para la próxima |
| `0xA8` | te llamaron por tu dir.+R, diste ACK | cargar en `I2DAT` el primer byte a entregar |
| `0xB8` | entregaste un dato y el maestro dio ACK | cargar el siguiente byte |
| `0xC0` | entregaste un dato y el maestro dio NACK | el maestro no quiere más; AA=1, limpiar SI |
| `0xC8` | entregaste el último dato (AA=0) con ACK | volver a estado libre |

Si habilitaste el General Call, sumá las variantes GC: `0x78` (como `0x68` pero por General Call),
`0x90` y `0x98` (como `0x80` y `0x88`); y si además competís como maestro, `0xB0` (perdiste el
arbitraje y te están leyendo). La tabla completa está en la página 01.

Detalle de diseño: como esclavo **no controlás el reloj** (lo pone el maestro), así que tenés que
estar listo para contestar rápido en la ISR. Si sos lento, el hardware hace **clock stretching**
solo (mantiene SCL en bajo) hasta que limpiás `SI`. Por eso un esclavo siempre va por interrupción en
sistemas reales: en polling podés perder una transacción si estabas haciendo otra cosa.

## Multi-maestro: cuando el LPC es las dos cosas

El LPC puede ser **maestro y esclavo a la vez**. Si dejás `AA=1` y configuraste tu `I2ADR`, podés
intentar ser maestro y, si **perdés el arbitraje** (código `0x38`; o `0x68`/`0x78`/`0xB0` si encima
te estaban direccionando), el hardware te pasa a esclavo automáticamente en la misma transferencia,
por si justo el otro maestro te estaba llamando.
No se pierde ningún dato. Esto es lo que hace útil un bus multi-maestro: dos placas que se hablan sin
un árbitro central. El ejemplo oficial `Master_Slave_Interrupt/` muestra exactamente este caso.

> Aparte está el **modo monitor** (`MMCTRL`): el I2C puede *espiar* todo el tráfico del bus sin
> participar (sin dar ACK ni tocar nada), útil para debug. El driver lo expone con
> `I2C_MonitorModeConfig` / `I2C_MonitorHandler`. No lo necesitás para operar, pero es oro para
> diagnosticar por qué un sensor no contesta.

## Ejercicios

1. Convertí el scanner de la página 02 a modo interrupción con callback y verificá que el `main`
   sigue libre mientras escanea.
2. Hacé que **dos LPC** se hablen: uno maestro que escribe un contador cada segundo, otro esclavo
   que lo recibe y lo muestra por UART. Probá `I2C_SetOwnSlaveAddr` con General Call.
3. Configurá una **máscara** (`I2MASK0`) para que tu esclavo responda a un rango de direcciones y
   verificá con el scanner cuáles "aparecen".

> Ejemplos oficiales: [`../../library/examples/I2C/`](../../library/examples/I2C/)
> (`slave/`, `Master_Slave_Interrupt/`, `Monitor/`).

---

**Anterior:** [02 - I2C con el driver CMSIS](./02-i2c-con-driver.md) ·
**Siguiente módulo:** [14 - SPI](../14_spi/)
