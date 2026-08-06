# I2C con el driver CMSIS

El driver `lpc17xx_i2c` esconde la máquina de estados de la página anterior. Vos describís la
transacción (a quién, qué mandás, qué querés recibir) y el driver corre el `switch (I2STAT)` por vos:
genera el START, manda `SLA+W`, los datos, hace el repeated START, lee, y cierra con STOP. Esta página
es la que vas a usar el 95% de las veces.

## Qué hace `I2C_Init` por dentro

```c
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"

void i2c0_init(void)
{
    // Pines: el driver NO toca PINSEL, lo hacés vos.
    PINSEL_CFG_Type pin;
    pin.Funcnum   = 1;                          // P0.27/P0.28 -> función I2C0
    pin.Pinmode   = PINSEL_PINMODE_TRISTATE;    // sin pull interno
    pin.OpenDrain = PINSEL_PINMODE_OPENDRAIN;   // imprescindible en I2C1/I2C2; en
                                                // P0.27/P0.28 no hace nada (los pads
                                                // ya son open-drain por hardware)
    pin.Portnum   = 0;
    pin.Pinnum = 27; PINSEL_ConfigPin(&pin);    // SDA0
    pin.Pinnum = 28; PINSEL_ConfigPin(&pin);    // SCL0

    I2C_Init(LPC_I2C0, 100000);   // 100 kHz
    I2C_Cmd(LPC_I2C0, ENABLE);    // setea I2EN
}
```

`I2C_Init(I2Cx, clockrate)` hace tres cosas que vos harías a registro:

1. **Enciende el periférico** (`PCONP`, vía `CLKPWR_ConfigPPWR`).
2. **Fija el PCLK del I2C en CCLK/2** (`CLKPWR_SetPCLKDiv(..., CLKPWR_PCLKSEL_CCLK_DIV_2)`). Esto
   importa para el cálculo: con `CCLK = 100 MHz`, queda `PCLK_I2C = 50 MHz`.
3. **Calcula `I2SCLH` e `I2SCLL`** a partir de ese PCLK y el `clockrate` pedido:
   `temp = PCLK_I2C / clockrate; I2SCLH = temp/2; I2SCLL = temp - I2SCLH`. O sea, reparte ~50/50. Para
   100 kHz con PCLK 50 MHz: `temp = 500 → I2SCLH = 250, I2SCLL = 250`. Coincide con lo que calculamos
   a mano en la página 01.

`I2C_Cmd(I2Cx, ENABLE)` simplemente setea `I2EN`. Importante: **el driver no configura los pines**
(no toca PINSEL ni PINMODE_OD): eso corre por tu cuenta. Con I2C0 en P0.27/P0.28 los pads ya son
open-drain por hardware (ahí el `OpenDrain` no cambia nada); con I2C1/I2C2, que usan pines comunes,
el open-drain lo ponés vos sí o sí.

## La transacción: `I2C_MasterTransferData`

El corazón del driver. Llenás una `I2C_M_SETUP_Type` que describe qué transmitir y qué recibir, y una
sola llamada hace toda la máquina de estados. Lo elegante del I2C es que permite **escribir y leer en
una misma transacción** sin soltar el bus, usando un **repeated START**: escribís el número de
registro del sensor y, sin STOP de por medio, leés su valor. La struct cubre exactamente ese caso.

```c
I2C_M_SETUP_Type t;
t.sl_addr7bit = 0x48;        // dirección de 7 BITS del esclavo (no la de 8)
t.tx_data     = tx_buf;      // bytes a escribir (ej.: el registro a leer)
t.tx_length   = 1;
t.rx_data     = rx_buf;      // dónde guardar lo recibido
t.rx_length   = 2;           // cuántos bytes leer
t.retransmissions_max = 3;   // reintentos ante NACK/arbitraje perdido
I2C_MasterTransferData(LPC_I2C0, &t, I2C_TRANSFER_POLLING);
```

| Campo | Para qué |
|-------|----------|
| `sl_addr7bit` | dirección de **7 bits** del esclavo (el driver le agrega el bit R/W) |
| `tx_data` / `tx_length` | bytes a escribir; `NULL`/`0` si solo leés |
| `rx_data` / `rx_length` | buffer y cantidad a leer; `NULL`/`0` si solo escribís |
| `retransmissions_max` | cuántas veces reintenta si un paso falla (NACK, `0x38`…) |
| `status` | (lo escribe el driver) código final + flags de error, si querés diagnosticar |

El driver decide el patrón según qué campos cargaste:

- **Solo TX** (`rx_length=0`): `S → SLA+W → datos → P`.
- **Solo RX** (`tx_length=0`): `S → SLA+R → datos → P` (manda NACK en el último byte solo).
- **TX y RX** (los dos cargados): `S → SLA+W → datos → Sr → SLA+R → datos → P`. Ese `Sr` es el
  repeated START. **Este es el caso típico de un sensor.**

El último parámetro es el modo:

- `I2C_TRANSFER_POLLING`: bloquea hasta terminar (corre el `switch` en un bucle). Simple, ideal para
  empezar.
- `I2C_TRANSFER_INTERRUPT`: no bloquea; el driver atiende cada paso en la IRQ de I2C y te llama un
  `callback` al final. Lo ves en la [página 03](./03-i2c-interrupcion-esclavo.md).

`I2C_MasterTransferData` devuelve `SUCCESS` o `ERROR`. Conviene chequearlo: si da `ERROR`, el esclavo
no contestó (NACK), o se perdió el arbitraje, o el bus está colgado. El campo `t.status` te dice qué
pasó: el driver le hace OR con `I2C_SETUP_STATUS_NOACKF` (1<<9) si hubo un NACK donde esperaba ACK;
en modo interrupción también con `I2C_SETUP_STATUS_ARBF` (1<<8) si perdió el arbitraje (en polling el
arbitraje perdido te queda como el código crudo, `0x38`).

## Ejemplo completo: leer un sensor de temperatura LM75

El LM75 (dirección `0x48`) guarda la temperatura en su registro `0x00`, en 2 bytes. La transacción
típica de "escribir el puntero de registro y leer" es justo el caso TX+RX con repeated START.

```c
#include "lpc17xx_i2c.h"

float leer_temperatura(void)
{
    uint8_t tx = 0x00;          // queremos el registro 0x00 (temperatura)
    uint8_t rx[2];

    I2C_M_SETUP_Type t;
    t.sl_addr7bit = 0x48;
    t.tx_data = &tx;  t.tx_length = 1;     // escribir "puntero = registro 0x00"
    t.rx_data = rx;   t.rx_length = 2;     // ...repeated START y leer 2 bytes
    t.retransmissions_max = 3;

    if (I2C_MasterTransferData(LPC_I2C0, &t, I2C_TRANSFER_POLLING) != SUCCESS) {
        return -999.0f;        // el sensor no respondió (NACK, sin pull-ups, etc.)
    }

    // El LM75 da la temperatura en rx[0]:rx[1] -> 9 bits útiles, 0.5 °C por LSB
    int16_t raw = (int16_t)((rx[0] << 8) | rx[1]) >> 7;
    return raw * 0.5f;
}

int main(void)
{
    i2c0_init();
    while (1) {
        float c = leer_temperatura();
        // mandar 'c' por UART (módulo 9) ...
    }
}
```

### Solo escribir (ej. configurar un expansor de puertos PCA9554)

```c
uint8_t cfg[2] = { 0x03, 0x00 };          // registro 0x03 (config) = 0x00: todo salida
I2C_M_SETUP_Type t = { .sl_addr7bit = 0x20,
                       .tx_data = cfg, .tx_length = 2,
                       .rx_data = NULL,  .rx_length = 0,
                       .retransmissions_max = 3 };
I2C_MasterTransferData(LPC_I2C0, &t, I2C_TRANSFER_POLLING);
```

## Errores comunes (y cómo se ven)

| Síntoma | Causa probable | Corrección |
|---------|----------------|-----------|
| Nada responde, todo da `ERROR` | Faltan pull-ups en SDA/SCL | I2C es open-drain: pull-ups externas ~4.7 kΩ obligatorias |
| Idem, con pull-ups puestas (I2C1/I2C2) | Olvidaste el open-drain de esos pines | configurá `PINMODE_OD` (el driver no lo hace; I2C0 en P0.27/28 ya es open-drain) |
| Habla con el chip equivocado | Usaste la dirección de **8 bits** del datasheet | usá la de 7 bits; `dir_7bit = dir_8bit >> 1` |
| NACK en `SLA+W` (`status` con NOACKF) | El esclavo no está, mala dir., o sin alimentar | verificá dirección con el scanner; revisá VCC del chip |
| Funciona y de golpe se cuelga | Bus colgado: SDA quedó en bajo | recuperalo con pulsos de clock (ver abajo) |
| Lecturas raras/intermitentes | Velocidad mayor a la que soporta el esclavo, o cables largos | bajá a 100 kHz; acortá cables; pull-ups más fuertes |
| Dos chips no responden | Dos esclavos con la misma dirección | cambiá la dirección por hardware (pines A0/A1/A2) |

### Recuperar un bus colgado (SDA pegado en bajo)

Si un esclavo se quedó a mitad de un byte (por un reset del micro en mal momento, por ejemplo), puede
estar **manteniendo SDA en bajo** esperando más pulsos de clock, y el bus queda muerto: ningún START
funciona. El truco estándar es, antes de inicializar el I2C, manejar SCL como **GPIO** y mandar hasta
**9 pulsos de clock** a mano. Eso obliga al esclavo a terminar su byte, liberar SDA, y después
generás un STOP manual. Recién ahí inicializás el periférico I2C normalmente. Es un buen ejercicio y
te saca de más de un apuro real.

## Cómo encontrar un dispositivo en el bus (scanner)

Si no sabés en qué dirección está un chip, recorré las direcciones válidas y fijate cuáles contestan
con ACK. Una transacción de escritura mínima que devuelve `SUCCESS` significa que alguien reconoció
esa dirección. Ojo: con este driver no podés sondear con `tx_length = 0` (en ese caso ni manda la
dirección), así que el mínimo real es **escribir 1 byte**; un `0x00` es inofensivo en la mayoría de
los chips (suele quedar como puntero de registro).

```c
for (uint8_t a = 0x08; a <= 0x77; a++) {
    uint8_t dummy = 0;
    I2C_M_SETUP_Type t = { .sl_addr7bit = a,
                           .tx_data = &dummy, .tx_length = 1,
                           .rx_data = NULL, .rx_length = 0,
                           .retransmissions_max = 0 };
    if (I2C_MasterTransferData(LPC_I2C0, &t, I2C_TRANSFER_POLLING) == SUCCESS) {
        // hay alguien en la dirección 'a' -> reportar por UART
    }
}
```

## Ejercicios

1. **Scanner I2C:** recorré 0x08–0x77 y reportá por UART qué direcciones responden.
2. Leé un sensor real (LM75, BMP280, MPU6050…) y mostrá el valor por UART.
3. Escribí y leé una EEPROM I2C (ej. 24LC256): guardá un valor, apagá, encendé y verificá que
   persiste. Pista: en estos chips la **dirección de memoria** son los primeros 1-2 bytes del TX.
4. Implementá la rutina de **recuperación de bus colgado** (9 pulsos de clock por GPIO) y corrila al
   arranque, antes de `I2C_Init`.

> Ejemplos oficiales: [`../../library/examples/I2C/`](../../library/examples/I2C/)
> (`master/`, `slave/`, `Master_Slave_Interrupt/`, `Monitor/`).

---

**Anterior:** [01 - I2C: protocolo y máquina de estados](./01-i2c-registros.md) ·
**Siguiente:** [03 - I2C por interrupción y como esclavo](./03-i2c-interrupcion-esclavo.md)
