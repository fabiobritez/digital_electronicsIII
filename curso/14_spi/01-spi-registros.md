# SPI: el protocolo, los 4 modos y el SPI legacy a registro

El **SPI** (Serial Peripheral Interface) es un bus serial síncrono full-duplex. "Síncrono"
significa que hay una línea de reloj dedicada: a diferencia de la UART, no hay que ponerse de
acuerdo en un baudrate ni recuperar el reloj del dato; el maestro lo genera y todos lo siguen.
"Full-duplex" significa que en cada pulso de reloj viaja un bit en cada dirección a la vez.

En el LPC1769 hay **tres** controladores que hablan SPI:

- **SPI legacy** (`LPC_SPI`): el viejo, sin FIFO. Útil para entender el protocolo desnudo, pero
  en proyectos reales casi siempre se prefiere el SSP.
- **SSP0** (`LPC_SSP0`) y **SSP1** (`LPC_SSP1`): los Synchronous Serial Port, modernos, con FIFOs
  de 8 niveles, DMA y tres formatos de trama. Los vemos en las páginas
  [02](./02-ssp-con-driver.md) y [03](./03-ssp-interrupciones-dma.md).

Esta página arranca por el protocolo (que es común a los tres) y baja a registro con el SPI legacy,
porque es el más simple y deja ver el mecanismo sin FIFOs de por medio.

## Cómo funciona el SPI

El maestro genera el reloj (SCK) y, en cada flanco activo, **intercambia un bit en las dos
direcciones a la vez**: saca un bit por MOSI y entra un bit por MISO. No hay "transmitir" y "recibir"
por separado: cada transferencia es las dos cosas en simultáneo. Si solo querés recibir, igual tenés
que "mandar" algo (un byte basura, *dummy*) para generar el reloj que hace girar al esclavo. Si solo
querés enviar, igual entra algo por MISO que normalmente descartás.

```
Maestro                 Esclavo
  SCK  ───────────────▶ SCK    (reloj, siempre lo manda el maestro)
  MOSI ───────────────▶ SI     (Master Out, Slave In)
  MISO ◀─────────────── SO     (Master In, Slave Out)
  SSEL ───────────────▶ SS     (Slave Select, activo en bajo)
```

Las cuatro señales:

| Señal | Nombre | Quién la maneja | Para qué |
|-------|--------|-----------------|----------|
| **SCK** | Serial Clock | maestro | marca el ritmo de los bits |
| **MOSI** | Master Out Slave In | maestro | dato del maestro al esclavo |
| **MISO** | Master In Slave Out | esclavo | dato del esclavo al maestro |
| **SSEL / CS** | Slave Select / Chip Select | maestro | "te estoy hablando a vos", activo en bajo |

Mentalmente: imaginate un **anillo de desplazamiento**. El registro de 8 bits del maestro y el del
esclavo están conectados en un lazo a través de MOSI y MISO. Cada pulso de SCK rota el anillo un bit:
lo que sale del maestro entra al esclavo y viceversa. Después de 8 pulsos, los registros se
intercambiaron por completo. Esa es la razón física de por qué SPI es siempre full-duplex y de por
qué recibir exige transmitir.

### Varios esclavos

No hay direcciones como en I2C. Para distinguir esclavos hay **una línea SSEL por cada uno**:

```
                 ┌──────────┐
        ┌── CS0 ─▶ Esclavo A │
        │        └──────────┘
Maestro │        ┌──────────┐
   SCK ─┼── CS1 ─▶ Esclavo B │   (SCK, MOSI, MISO compartidos)
  MOSI ─┤        └──────────┘
  MISO ◀┘
```

El maestro baja el CS del que quiere, hace la transferencia y lo sube. Los demás, con su CS en alto,
ponen su MISO en alta impedancia e ignoran el bus. Esto cuesta un pin por esclavo.

La alternativa para ahorrar pines es el **daisy chain**: se encadenan los esclavos en serie (MISO de
uno al MOSI del siguiente) y comparten un único SSEL. El dato atraviesa toda la cadena como un tren.
Es elegante para tiras de drivers tipo registros de desplazamiento (74HC595, tiras LED), pero exige
que los esclavos lo soporten y complica el software. En la práctica de un curso, lo normal es **un CS
por esclavo manejado como GPIO**.

### SSEL: hardware vs software

El SSEL se puede manejar de dos formas:

- **Por hardware**: usás el pin SSEL del periférico (P0.16 para SSP0) y el controlador lo baja/sube
  solo. Ojo: el SPI legacy en modo maestro **no genera SSEL automático**; ese pin es una **entrada**
  para detección de mode-fault (el manual aclara que "el SSEL debe estar siempre inactivo cuando el
  SPI controla como maestro", §17). El SSP sí lo maneja, pero lo baja por cada *frame*, no por toda la
  transacción.
- **Por software (lo recomendado)**: configurás el pin de CS como **GPIO** común y lo bajás vos antes
  de la transferencia y lo subís al terminar. Así controlás exactamente cuándo empieza y termina la
  transacción completa, que es lo que casi todos los esclavos esperan (mantener CS bajo durante todo
  el comando + datos). Es el patrón que usamos en todo el módulo.

## Los 4 modos: CPOL y CPHA

El SPI **no estandariza** en qué flanco del reloj se muestrea el dato. Cada chip esclavo elige, y el
maestro tiene que igualarlo o lee los bits corridos. Dos bits definen el "modo":

- **CPOL** (Clock Polarity): nivel del reloj en reposo (cuando no hay transferencia).
  - `CPOL=0`: el reloj reposa en **bajo**; los flancos van bajo→alto (subida) y alto→bajo (bajada).
  - `CPOL=1`: el reloj reposa en **alto**; el primer flanco es de bajada.
- **CPHA** (Clock Phase): en cuál de los dos flancos de cada bit se **muestrea** (se lee).
  - `CPHA=0`: se muestrea en el **primer** flanco de cada ciclo; el dato se debe poner *antes*, en el
    flanco anterior (o al bajar el CS para el primer bit).
  - `CPHA=1`: se muestrea en el **segundo** flanco; el dato cambia en el primero.

Las combinaciones dan los modos 0 a 3:

| Modo | CPOL | CPHA | Reposo del reloj | Se muestrea en... |
|------|------|------|------------------|-------------------|
| 0 | 0 | 0 | bajo | flanco de subida (1ro) |
| 1 | 0 | 1 | bajo | flanco de bajada (2do) |
| 2 | 1 | 0 | alto | flanco de bajada (1ro) |
| 3 | 1 | 1 | alto | flanco de subida (2do) |

La regla práctica que sirve siempre:

- **CPHA=0** → el dato se captura en el **primer** flanco después de que SCK se mueve. Eso obliga a
  que el primer bit ya esté presente cuando se baja el CS (el esclavo lo pone "adelantado").
- **CPHA=1** → el dato se captura en el **segundo** flanco. Más cómodo para el esclavo, que cambia el
  dato en el primer flanco y lo deja estable para el segundo.

Diagrama de los cuatro (la `^` marca el flanco donde se muestrea el bit):

```
        Modo 0  (CPOL=0, CPHA=0)            Modo 1  (CPOL=0, CPHA=1)
SCK  ___|‾|_|‾|_|‾|_|‾|___           SCK  ___|‾|_|‾|_|‾|_|‾|___
muestreo ^   ^   ^   ^   (subida)    muestreo  ^   ^   ^   ^  (bajada)

        Modo 2  (CPOL=1, CPHA=0)            Modo 3  (CPOL=1, CPHA=1)
SCK  ‾‾‾|_|‾|_|‾|_|‾|_|‾‾‾           SCK  ‾‾‾|_|‾|_|‾|_|‾|_|‾‾‾
muestreo ^   ^   ^   ^   (bajada)    muestreo  ^   ^   ^   ^  (subida)
```

**Cómo elegir el modo:** lo dice el datasheet de tu chip esclavo, en el diagrama de tiempos o
literalmente "SPI Mode 0". El más común con diferencia es el **modo 0** (CPOL=0, CPHA=0). Muchas
flash, displays y ADC usan modo 0 o modo 3. Si te equivocás de modo, el síntoma típico es que recibís
todo desplazado un bit, o "casi" bien (un modo adyacente a veces funciona por casualidad a baja
velocidad y falla al subir el clock). Ante datos raros, lo primero a revisar es CPOL/CPHA.

> Cuidado con la nomenclatura de NXP en el SSP: el bit `CPOL` del SSP está **invertido** respecto de
> la intuición. Lo vemos en la [página 02](./02-ssp-con-driver.md); por ahora, en el SPI legacy
> `CPOL` sigue la convención clásica (0 = reposo bajo).

## El SPI legacy a registro

El SPI legacy se mapea en `LPC_SPI` (tipo `LPC_SPI_TypeDef` en `LPC17xx.h`). Sus registros, con los
nombres **exactos** del header:

| Registro | Campo | Función |
|----------|-------|---------|
| `SPCR` | Control | configuración: bits/transfer, CPHA, CPOL, maestro, orden, interrupción |
| `SPSR` | Status | flags `SPIF`, `WCOL`, `ROVR`, `MODF`, `ABRT` |
| `SPDR` | Data | escribir = transmitir; leer = recibir (los dos al mismo registro) |
| `SPCCR` | Clock Counter | divisor del reloj de SCK |
| `SPINT` | Interrupt | flag de interrupción |

### SPCR (SPI Control Register)

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 2 | `BitEnable` (BIT_EN) | 1 = el campo BITS controla cuántos bits por transferencia (8–16). Si es 0, son 8 fijos |
| 3 | `CPHA` | fase del reloj |
| 4 | `CPOL` | polaridad del reloj. El manual (§17) lo define como "SCK active high/low": `CPOL=0` → SCK activo en alto → **reposo en bajo** (modo 0/1); `CPOL=1` → SCK activo en bajo → **reposo en alto** (modo 2/3). O sea el legacy **sí** sigue la convención clásica del bit (a diferencia de los *nombres* invertidos del header del SSP) |
| 5 | `MSTR` | 1 = maestro, 0 = esclavo |
| 6 | `LSBF` | 1 = LSB primero, 0 = MSB primero (lo habitual es MSB) |
| 7 | `SPIE` | 1 = habilita la interrupción de SPI |
| 11:8 | `BITS` | cantidad de bits por transferencia (8 a 16); con `BitEnable=1` |

`BITS` se codifica raro: `0b1000`=8, `0b1001`=9, ..., `0b1111`=15 y `0b0000`=16. El header CMSIS lo
encapsula en `SPI_SPCR_BITS(n)`.

### SPSR (SPI Status Register)

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 3 | `ABRT` | Slave Abort: el esclavo perdió el CS a mitad de transferencia |
| 4 | `MODF` | Mode Fault: el pin SSEL del maestro se fue a bajo (otro maestro tomó el bus) |
| 5 | `ROVR` | Read Overrun: llegó un dato nuevo y no leíste el anterior |
| 6 | `WCOL` | Write Collision: escribiste `SPDR` mientras había una transferencia en curso |
| 7 | `SPIF` | SPI transfer complete: terminó la transferencia (se limpia leyendo SPSR y luego SPDR) |

**El detalle importante (y trampa) del legacy:** `SPIF`, `WCOL` y `MODF` tienen una secuencia de
limpieza específica y delicada. `SPIF` y `WCOL` se limpian leyendo `SPSR` y accediendo *después* a
`SPDR` (lectura o escritura). `MODF` es distinto: se limpia leyendo `SPSR` y *escribiendo* después
`SPCR`. Si invertís el orden, o leés `SPDR` antes que `SPSR`, los flags no
se limpian bien y la próxima transferencia se cuelga o pisás un overrun. Esta fragilidad en el manejo
de flags es una de las razones históricas por las que NXP empujó al SSP. En el SSP no existe este
ritual: hay FIFOs y flags de estado claros.

### SPCCR (SPI Clock Counter Register)

Define la frecuencia de SCK:

```
f_SCK = PCLK_SPI / SPCCR
```

Reglas duras del hardware:

- `SPCCR` debe ser **par** (bit 0 siempre en 0).
- `SPCCR` debe ser **≥ 8**.

Entonces la frecuencia máxima de SCK en el legacy es `PCLK/8`. Si `PCLK = 25 MHz`, lo más rápido es
`25/8 ≈ 3.125 MHz`. Para 1 MHz: `SPCCR = 25e6/1e6 = 25` → no es par, redondeás a 26 → `f ≈ 0.96 MHz`.

### Init y transferencia a registro (legacy)

```c
#include <LPC17xx.h>

void spi_init(void) {
    LPC_SC->PCONP |= (1u << 8);            // encender el SPI legacy (PCONP bit 8 = PCSPI)

    // PCLKSEL0 bits 17:16 = PCLK_SPI. 00 = CCLK/4 (default). Lo dejamos así.

    // Pines del SPI legacy: SCK=P0.15, SSEL=P0.16, MISO=P0.17, MOSI=P0.18 (funcion 3)
    LPC_PINCON->PINSEL0 &= ~(0x3u << 30);
    LPC_PINCON->PINSEL0 |=  (0x3u << 30);              // P0.15 SCK  (func 3)
    LPC_PINCON->PINSEL1 &= ~((0x3u<<0)|(0x3u<<2)|(0x3u<<4));
    LPC_PINCON->PINSEL1 |=  ((0x3u<<2)|(0x3u<<4));     // P0.17 MISO, P0.18 MOSI (CS por GPIO)

    // SPCR: maestro (MSTR), 8 bits, MSB primero, modo 0 (CPOL=0/CPHA=0)
    LPC_SPI->SPCR = (1u << 5);             // MSTR=1; BitEnable=0 -> 8 bits fijos
    LPC_SPI->SPCCR = 8;                     // par y >=8 -> f_SCK = PCLK/8 (lo mas rapido)
}

uint8_t spi_transfer(uint8_t dato) {
    LPC_SPI->SPDR = dato;                              // arranca la transferencia
    while (!(LPC_SPI->SPSR & (1u << 7))) { }           // esperar SPIF (bit 7)
    return (uint8_t) LPC_SPI->SPDR;                    // leer SPDR limpia SPIF y devuelve lo recibido
}
```

El CS lo manejás como GPIO (lo bajás antes, lo subís después). Fijate la diferencia esencial con el
SSP: acá **no hay FIFO**. Escribís un byte en `SPDR`, esperás a `SPIF`, leés. Byte por byte, sin
solapamiento. Por eso el legacy es más lento en streams largos: el SSP puede tener 8 bytes "en vuelo"
mientras el legacy procesa de a uno con espera entre cada uno.

## Por qué se prefiere el SSP

| | SPI legacy (`LPC_SPI`) | SSP (`LPC_SSP0/1`) |
|--|------------------------|--------------------|
| FIFO | no (1 byte) | sí, 8 niveles TX y 8 RX |
| DMA | no | sí (TX y RX) |
| Formatos de trama | solo SPI Motorola | SPI, TI, Microwire |
| Manejo de flags | frágil (ritual SPIF/WCOL) | claro (TFE/TNF/RNE/RFF/BSY) |
| Instancias | 1 | 2 (SSP0 y SSP1) |
| Tamaño de dato | 8–16 bits | 4–16 bits |

En resumen: el SSP hace lo mismo y mejor. **Usá SPI legacy solo si te lo piden explícitamente.** Ni
siquiera sirve para sumar un tercer bus: el manual aclara que SSP0 es la alternativa moderna del SPI
legacy y que **solo uno de los dos puede usarse a la vez** (§18.1); el máximo real son dos buses
simultáneos (SSP0 + SSP1). Para todo lo demás, SSP. De acá en adelante trabajamos con SSP.

---

**Módulo:** [SPI/SSP](./README.md) · **Siguiente:** [02 - SSP a registro y con driver](./02-ssp-con-driver.md)
