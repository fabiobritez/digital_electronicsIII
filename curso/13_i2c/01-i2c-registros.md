# I2C: el protocolo y la máquina de estados (a registro)

El I2C (Inter-Integrated Circuit, "i-cuadrado-ce") es el periférico más distinto de todos los que
viste hasta acá. No se maneja escribiendo un valor y listo: es un **protocolo paso a paso**, y el
hardware del LPC1769 implementa una **máquina de estados** que vos tenés que ir empujando estado por
estado. Esta página te explica el protocolo a fondo y después la máquina de estados a registro, que es
lo que el driver de la página siguiente te esconde. Entenderla acá es lo que te va a salvar cuando el
bus se cuelgue o un sensor no conteste.

## El bus físico: dos cables, open-drain y por qué las pull-ups

El I2C usa dos líneas compartidas por todos los dispositivos:

- **SCL** (Serial Clock): el reloj. Lo genera el **maestro**.
- **SDA** (Serial Data): los datos, bidireccional.

Las dos líneas son **open-drain** (acordate del módulo 4): cada chip solo puede **tirar la línea a 0**
(conecta a masa por un transistor) o **soltarla** (alta impedancia). Ninguno puede "poner un 1"
activamente. El 1 lo ponen **resistencias de pull-up externas** (típicamente 4.7 kΩ a 3.3 V, una en
SDA y otra en SCL).

¿Por qué así y no push-pull como un GPIO normal? Por tres razones que conviene tener claras:

1. **Varios dispositivos pueden compartir el cable sin destruirse.** En push-pull, si un chip pone 1
   y otro pone 0, se cortocircuitan y se queman. En open-drain, si uno tira a 0 y otro suelta, la
   línea queda en 0 y no pasa nada malo. Es un **AND cableado**: la línea está en 1 solo si *todos*
   la sueltan.
2. **Habilita el arbitraje** (lo vemos abajo): si dos maestros hablan a la vez, el que pone 0 le gana
   al que pone 1, y el que perdió se entera leyendo la línea.
3. **Habilita el clock stretching**: un esclavo lento puede mantener SCL en 0 para frenar al maestro.

Consecuencia práctica número uno, la que más cuelga proyectos: **sin pull-ups el bus no funciona**.
Las líneas se quedan en un nivel indefinido o siempre en bajo, no hay ACK, y parece que el sensor está
roto. No están dentro del LPC: las ponés vos en la placa.

## Anatomía de una transacción

```
[START] [SLA + R/W] [ACK] [dato] [ACK] ... [dato] [ACK/NACK] [STOP]
```

Las condiciones especiales se definen por **qué hace SDA mientras SCL está en alto** (durante los
datos, SDA solo cambia con SCL en bajo):

1. **START (S):** el maestro tira SDA a 0 **con SCL en alto**. Significa "atención, arranca algo" y
   ocupa el bus. A partir de acá el bus está "tomado".
2. **Dirección + R/W (SLA+W o SLA+R):** el maestro manda 7 bits de dirección del esclavo y 1 bit de
   dirección de datos: **0 = Write (escribir al esclavo), 1 = Read (leer del esclavo)**. Por eso se
   escribe `SLA+W` o `SLA+R`. El esclavo que reconoce su dirección contesta.
3. **ACK / NACK:** el receptor confirma cada byte tirando SDA a 0 durante el noveno pulso de SCL
   (**ACK** = "lo recibí"). Si en cambio deja SDA en alto, es un **NACK** = "no hay nadie en esa
   dirección" o "no quiero más datos". Ojo con la dirección del ACK: cuando el maestro **escribe**,
   el ACK lo da el **esclavo**; cuando el maestro **lee**, el ACK de cada byte lo da el **maestro**
   (y manda NACK en el último byte para decir "ya está, soltá").
4. **Datos:** bytes de 8 bits, cada uno seguido de su bit de ACK/NACK.
5. **Repeated START (Sr):** un START nuevo **sin haber soltado el bus con STOP**. Sirve para cambiar
   de dirección de datos sin perder el bus (clásico: escribo el número de registro del sensor, hago
   repeated START y leo). Si soltaras con STOP entre medio, otro maestro podría meterse.
6. **STOP (P):** el maestro suelta SDA **con SCL en alto** → "termino, bus libre".

> La **dirección de 7 bits** sale del datasheet del *chip esclavo*, no del LPC. Un LM75 suele estar en
> `0x48`. Cuidado con una confusión muy común: muchos datasheets dan la dirección ya **corrida un bit
> e incluyendo el R/W** (dirección de "8 bits", ej. `0x90` para escribir / `0x91` para leer). En el
> LPC y en CMSIS se trabaja con la de **7 bits** (`0x48`); el bit R/W lo agrega el hardware. Si pasás
> `0x90` donde va `0x48`, hablás con el chip equivocado. Regla mental: `dir_8bit = dir_7bit << 1`.

### Direccionamiento de 10 bits

El estándar permite direcciones de 10 bits para tener más dispositivos. No es un modo de hardware
aparte: se hace por software mandando un primer byte especial `11110xx` (los dos bits altos de la
dirección) seguido de un segundo byte con los 8 bits bajos. El LPC no tiene soporte dedicado; si
necesitás 10 bits, los armás a mano con la máquina de estados. En la práctica del curso casi todo es
de 7 bits.

### Clock stretching (el esclavo frena al maestro)

Como SCL es open-drain, **un esclavo lento puede mantener SCL en 0** aunque el maestro quiera
soltarlo. El maestro, antes de seguir, espera a ver SCL realmente en alto; mientras el esclavo lo
sostenga en bajo, el bus queda en pausa. Eso es **clock stretching**: un mecanismo de handshake para
que un esclavo que todavía no tiene el dato listo gane tiempo. El manual lo lista como "serial clock
synchronization usada como mecanismo de handshake para suspender y reanudar la transferencia". No hace
falta que programes nada para tolerarlo del lado maestro; es transparente.

### Arbitraje y multi-maestro

Pueden coexistir varios maestros en el mismo bus. Si dos arrancan a transmitir casi a la vez, el bus
se sincroniza solo (cada uno mira el SCL real) y arranca el **arbitraje**: ambos transmiten y al mismo
tiempo **leen** SDA. Como es open-drain (AND cableado), el que manda un 0 gana sobre el que manda un
1. El maestro que quiso poner 1 pero lee un 0 detecta que **perdió el arbitraje**, larga el bus de
inmediato y, si está habilitado como esclavo, pasa a modo esclavo por si justo lo estaban llamando.
No se pierde ni se corrompe ningún dato: el que ganó sigue como si nada. En el LPC esto aparece como
el código de estado **`0x38`** (arbitration lost). El registro `MMCTRL` no es para esto sino para el
**modo monitor** (espiar el bus); el arbitraje lo maneja el hardware solo.

## Los registros del I2C

En `LPC17xx.h`, todos los I2C comparten el mismo `LPC_I2C_TypeDef` (lo usás como `LPC_I2C0`,
`LPC_I2C1`, `LPC_I2C2`):

| Registro | Acceso | Para qué |
|----------|--------|----------|
| `I2CONSET` | R/W | **Poner** bits de control (I2EN, STA, STO, SI, AA). Escribir 1 setea; 0 no cambia |
| `I2CONCLR` | WO  | **Borrar** esos bits (escribir 1 borra; 0 no cambia) |
| `I2STAT`   | RO  | Código de **estado** actual (bits 7:3); en qué punto del protocolo está el hardware |
| `I2DAT`    | R/W | Dato a transmitir / dato recibido |
| `I2ADR0..3`| R/W | Hasta 4 direcciones propias (cuando el LPC actúa de **esclavo**) |
| `I2MASK0..3`| R/W | Máscara de cada dirección de esclavo (qué bits ignorar al comparar) |
| `I2SCLH`   | R/W | Ciclos de PCLK que SCL está en **alto** → velocidad y duty |
| `I2SCLL`   | R/W | Ciclos de PCLK que SCL está en **bajo** |
| `MMCTRL`   | R/W | Control del modo monitor (espiar todo el tráfico del bus) |

### Por qué SET y CLEAR son registros separados

`I2CONSET` e `I2CONCLR` son la rareza más importante de este periférico. Apuntan al **mismo registro
de control físico**, pero:

- Escribir un 1 en un bit de `I2CONSET` **lo pone en 1**. Un 0 no toca nada.
- Escribir un 1 en un bit de `I2CONCLR` **lo pone en 0**. Un 0 no toca nada.

¿Por qué este lío en vez de un `|=` y `&=~` normales? Porque el I2C se atiende casi siempre **por
interrupción**, y el flag `SI` lo va levantando el hardware **en cualquier momento**, en paralelo con
tu código. Si hicieras `I2CON |= (1<<5)` con un read-modify-write, podrías leer el registro,
modificarlo, y entre medio el hardware setear `SI`; al escribir de vuelta, **pisarías** ese `SI` sin
querer. Con SET y CLEAR separados, cada operación toca *solo* los bits que vos indicás y deja los
demás intactos, sin condición de carrera. Es el patrón de "bit set/clear atómico".

Los bits de control:

| Bit | Nombre | En `I2CONSET` (poner) | En `I2CONCLR` (borrar) |
|-----|--------|------------------------|-------------------------|
| 6 | **I2EN** | habilitar el periférico | deshabilitarlo |
| 5 | **STA**  | "generá un START" (o repeated START) | cancelar el pedido de START |
| 4 | **STO**  | "generá un STOP" | (se autolimpia; el STOP no se cancela manualmente) |
| 3 | **SI**   | (no se setea a mano) | **limpiar el flag** → "ya atendí, seguí" |
| 2 | **AA**   | responder ACK al próximo byte/dirección | responder NACK |

En CMSIS estos bits están como `I2C_I2CONSET_STA` (0x20), `_STO` (0x10),
`_SI` (0x08), `_AA` (0x04), `_I2EN` (0x40), y los de clear `I2C_I2CONCLR_STAC`, `_SIC`,
`_AAC`, `_I2ENC`. No hay `STOC`: en `I2CONCLR` el bit 4 es reservado, coherente con que el STOP
se autolimpia.

## La máquina de estados: `I2STAT` como hilo conductor

Acá está el corazón. Cada vez que el hardware **completa una etapa** del protocolo (mandó el START,
mandó la dirección, recibió un ACK, recibió un byte…) hace dos cosas:

1. Levanta el flag **`SI`** (Serial Interrupt). El bus queda **congelado** ahí, esperándote: nada
   avanza hasta que vos limpies `SI`.
2. Pone en **`I2STAT`** un **código de estado** de 5 bits (en los bits 7:3, por eso siempre múltiplos
   de 8: `0x08`, `0x18`, `0x28`…).

Tu trabajo es: leer `I2STAT`, decidir qué hacer (cargar un dato, pedir STOP, leer un byte…), preparar
los bits STA/STO/AA según corresponda, y **limpiar `SI`** para que el hardware dé el próximo paso. Por
eso el patrón canónico es un `switch (I2STAT)` dentro de la interrupción de I2C.

### Tabla de códigos de estado

Los códigos, y qué hacer en cada uno, salen de las Tablas 399 a 403 del manual. Estos son los que vas
a ver:

**Master transmitter (vos escribís a un esclavo):**

| Código | Significado | Qué hacés |
|--------|-------------|-----------|
| `0x08` | START transmitido | cargar `SLA+W` en `I2DAT`, limpiar STA y SI |
| `0x10` | repeated START transmitido | cargar `SLA+W` o `SLA+R`, limpiar STA y SI |
| `0x18` | `SLA+W` enviado, **ACK** recibido | cargar primer dato, limpiar SI |
| `0x20` | `SLA+W` enviado, **NACK** recibido | no hay nadie en esa dir. → STOP (o reintentar) |
| `0x28` | dato enviado, **ACK** recibido | cargar el próximo dato; si no hay más, STOP |
| `0x30` | dato enviado, **NACK** recibido | el esclavo no quiere más → STOP |
| `0x38` | **arbitraje perdido** | soltar; reintentar el START cuando el bus quede libre |

**Master receiver (vos leés de un esclavo):**

| Código | Significado | Qué hacés |
|--------|-------------|-----------|
| `0x40` | `SLA+R` enviado, **ACK** recibido | si vas a leer ≥2 bytes, AA=1 (ACK al primero); si leés 1 solo, AA=0 ya acá; limpiar SI |
| `0x48` | `SLA+R` enviado, **NACK** recibido | nadie contestó → STOP |
| `0x50` | dato recibido, **ACK** devuelto por vos | leer `I2DAT`; si al próximo le queda 1 byte, poné AA=0 para que ese sea el último |
| `0x58` | dato recibido, **NACK** devuelto por vos | leer `I2DAT` (último byte) → STOP (o repeated START) |

Detalle fino del master-receiver: como el ACK de cada byte lo da el maestro, para el **último** byte
tenés que mandar **NACK** (AA=0) para avisarle al esclavo "no leo más". La regla general: dejás AA=1
mientras vas a seguir leyendo, y bajás AA a 0 **un paso antes** de recibir el último byte (en `0x40`
si leés 1 solo; en `0x50` cuando solo falta 1). Recibido ese último byte llegás a `0x58` (NACK
devuelto) y cerrás con STOP.

**Slave receiver (el LPC es esclavo y lo escriben):**

| Código | Significado |
|--------|-------------|
| `0x60` | tu propia dirección + W recibida, ACK devuelto |
| `0x68` | arbitraje perdido como maestro; te llamaron por tu dir.+W, ACK |
| `0x70` | **General Call** recibido, ACK devuelto |
| `0x78` | arbitraje perdido como maestro; llegó un General Call, ACK devuelto |
| `0x80` | dato recibido (estando direccionado por tu dir.), ACK devuelto |
| `0x88` | dato recibido, NACK devuelto |
| `0x90` | dato recibido (direccionado por General Call), ACK devuelto |
| `0x98` | dato recibido (direccionado por General Call), NACK devuelto |
| `0xA0` | STOP o repeated START recibido mientras estabas direccionado |

**Slave transmitter (el LPC es esclavo y lo leen):**

| Código | Significado |
|--------|-------------|
| `0xA8` | tu dir.+R recibida, ACK devuelto → cargar dato a entregar |
| `0xB0` | arbitraje perdido como maestro; te llamaron por tu dir.+R, ACK → cargar dato a entregar |
| `0xB8` | dato transmitido, ACK recibido → cargar el siguiente |
| `0xC0` | dato transmitido, NACK recibido → el maestro no quiere más |
| `0xC8` | último dato (con AA=0) transmitido, ACK recibido |

Y dos comodines: `0x00` = error de bus (START/STOP en posición ilegal; se sale seteando STO y
limpiando SI: el hardware libera las líneas sin transmitir un STOP real), y `0xF8` = "sin
información relevante" (`I2C_I2STAT_NO_INF`): `SI` no está levantado, no hay nada que hacer.

### El esqueleto, paso a paso, en polling

Así se ve un master-transmitter mínimo a registro, para que veas cómo se empuja la máquina. (En la
práctica esto va por interrupción; lo ves en [la página 03](./03-i2c-interrupcion-esclavo.md).)

```c
#include <LPC17xx.h>

#define STA  (1u<<5)
#define STO  (1u<<4)
#define SI   (1u<<3)
#define AA   (1u<<2)

// Devuelve 0 si todo ok, !=0 si hubo NACK/error.
int i2c0_escribir(uint8_t addr7, const uint8_t *datos, int n)
{
    // 1) Pedir START
    LPC_I2C0->I2CONSET = STA;
    while (!(LPC_I2C0->I2CONSET & SI)) { }   // esperar a que el hardware lo complete

    if (LPC_I2C0->I2STAT != 0x08) return -1; // 0x08 = START enviado

    // 2) Mandar SLA+W (dirección << 1, R/W = 0)
    LPC_I2C0->I2DAT = (addr7 << 1);
    LPC_I2C0->I2CONCLR = STA | SI;           // bajar STA y SI -> el hardware transmite
    while (!(LPC_I2C0->I2CONSET & SI)) { }

    if (LPC_I2C0->I2STAT != 0x18) {          // 0x18 = SLA+W + ACK; si no, nadie contestó
        LPC_I2C0->I2CONSET = STO;            // soltar el bus
        LPC_I2C0->I2CONCLR = SI;
        return -2;
    }

    // 3) Mandar los datos
    for (int i = 0; i < n; i++) {
        LPC_I2C0->I2DAT = datos[i];
        LPC_I2C0->I2CONCLR = SI;
        while (!(LPC_I2C0->I2CONSET & SI)) { }
        if (LPC_I2C0->I2STAT != 0x28) {      // 0x28 = dato + ACK
            LPC_I2C0->I2CONSET = STO;
            LPC_I2C0->I2CONCLR = SI;
            return -3;
        }
    }

    // 4) STOP
    LPC_I2C0->I2CONSET = STO;
    LPC_I2C0->I2CONCLR = SI;
    return 0;
}
```

Fijate el ritmo invariable: **escribís el dato / preparás STA-STO-AA → limpiás SI → esperás SI de
nuevo → leés I2STAT**. Cada `case` del protocolo es un peldaño. Manejar *todos* los códigos a mano
(las dos docenas de arriba, master y slave juntos) es largo y muy fácil de romper. Por eso el I2C es,
junto con el DMA, **el periférico donde casi siempre se usa el driver**, incluso mientras aprendés. La
[página 02](./02-i2c-con-driver.md) hace exactamente esto pero con el driver, y la
[página 03](./03-i2c-interrupcion-esclavo.md) muestra el `switch (I2STAT)` por interrupción y el modo
esclavo.

## El cálculo de velocidad: I2SCLH e I2SCLL

La frecuencia del bit de SCL sale de dividir el `PCLK_I2C` entre la suma de los dos contadores de duty
(fórmula 13 del manual):

```
                 PCLK_I2C
f_SCL = ---------------------------
            I2SCLH + I2SCLL
```

`I2SCLH` cuenta cuántos ciclos de PCLK está SCL **en alto**, `I2SCLL` cuántos **en bajo**. Dos reglas
del manual:

- **Cada registro debe ser ≥ 4.** Por debajo de eso el temporizado interno no cierra.
- No tienen por qué ser iguales: ajustás el **duty** del reloj poniendo distintos valores (el estándar
  pide tiempos de bajo y alto distintos en fast mode). Si los hacés iguales, queda 50/50.

Por defecto, `I2C_Init` del driver deja `PCLK_I2C = CCLK/2`. Con `CCLK = 100 MHz` queda
`PCLK_I2C = 50 MHz`. Entonces, para una frecuencia objetivo:

```
I2SCLH + I2SCLL = PCLK_I2C / f_SCL
```

**Ejemplo a 100 kHz (standard mode), PCLK_I2C = 50 MHz:**

```
I2SCLH + I2SCLL = 50e6 / 100e3 = 500   →   I2SCLH = 250, I2SCLL = 250  (duty 50/50)
```

**Ejemplo a 400 kHz (fast mode), PCLK_I2C = 50 MHz:**

```
I2SCLH + I2SCLL = 50e6 / 400e3 = 125   →   I2SCLH = 62, I2SCLL = 63
```

(El driver hace `I2SCLH = temp/2` e `I2SCLL = temp - I2SCLH`, o sea reparte ~50/50; con `temp=125`
te da 62 y 63.) La tabla 395 del manual da estos valores para varios PCLK: a 50 MHz, `I2SCLL+I2SCLH`
= 500 para 100 kHz y 125 para 400 kHz, que es justo lo de arriba.

> Cuidado: si en tu proyecto cambiás el divisor de PCLK del I2C (`PCLKSEL`), cambia PCLK_I2C y con él
> la velocidad real. Si calculás a mano `I2SCLH/I2SCLL` tenés que usar el PCLK_I2C **real**, no el
> CCLK.

## El ritual de arranque del I2C (a registro)

```c
#include <LPC17xx.h>

void i2c0_init_100k(void)
{
    // 1) ENCENDER: PCONP. I2C0 = bit 7 (I2C1 = bit 19, I2C2 = bit 26).
    //    En reset ya vienen encendidos, pero hacelo explícito.
    LPC_SC->PCONP |= (1u << 7);

    // 2) CLOCKEAR: PCLKSEL elige PCLK_I2C0. Por defecto CCLK/4.
    //    Acá lo dejamos como venga; lo importante es usar ese PCLK al calcular SCLH/L.

    // 3) PINES: SDA0 = P0.27, SCL0 = P0.28, función 1.
    //    P0.27/P0.28 son pads dedicados de I2C: open-drain POR HARDWARE (los bits
    //    27/28 de PINMODE_OD0 no tienen efecto en ellos, y tampoco tienen pulls
    //    internos). Con I2C1/I2C2, que usan pines comunes, ahí sí va PINMODE_OD
    //    en 1 y PINMODE sin pull-up ni pull-down.
    LPC_PINCON->PINSEL1 &= ~((0x3u << 22) | (0x3u << 24));  // limpiar
    LPC_PINCON->PINSEL1 |=  ((0x1u << 22) | (0x1u << 24));  // función I2C (01)
    //    (con pull-ups EXTERNAS en la placa; sin ellas el bus no anda)

    // 4) VELOCIDAD: 100 kHz suponiendo PCLK_I2C0 = 25 MHz -> SCLH+SCLL = 250.
    //    (Ojo: acá NO tocamos PCLKSEL, así que vale el default de reset CCLK/4
    //     = 25 MHz; el driver de la pág 02 en cambio fuerza CCLK/2 = 50 MHz y
    //     entonces para 100 kHz le dan 250/250. Mismo f_SCL, distinto PCLK.)
    LPC_I2C0->I2SCLH = 125;
    LPC_I2C0->I2SCLL = 125;

    // 5) HABILITAR + limpiar STA/AA por las dudas
    LPC_I2C0->I2CONCLR = (1u<<5) | (1u<<4) | (1u<<3) | (1u<<2); // STA STO SI AA
    LPC_I2C0->I2CONSET = (1u << 6);                            // I2EN
}
```

> Los pines de cada controlador difieren: I2C0 está en P0.27/P0.28 (función 1, pads especiales I2C:
> open-drain por hardware, con filtro analógico de glitches, y los únicos que soportan Fast Mode Plus
> a 1 MHz vía `I2CPADCFG`); I2C1 en P0.0/P0.1 o P0.19/P0.20 (función 3 en los dos pares); I2C2 en
> P0.10/P0.11 (función 2). I2C1 e I2C2 usan pines comunes: a esos configuralos vos en open-drain
> (`PINMODE_OD`) y sin pulls internas. Mirá el datasheet antes de cablear. I2C0 no existe en el
> encapsulado de 80 pines.

La inicialización es la parte fácil. Lo bravo, como viste, es la transacción. En la
[próxima página](./02-i2c-con-driver.md) hacemos transacciones completas con el driver, que esconde
toda esta máquina de estados.

---

**Módulo:** [I2C](./README.md) · **Siguiente:** [02 - I2C con el driver](./02-i2c-con-driver.md)
