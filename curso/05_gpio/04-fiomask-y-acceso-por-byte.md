# FIOMASK y acceso por byte (trucos avanzados de GPIO)

Las páginas 01 y 02 te dieron lo que usás el 95% del tiempo. Esta cierra el módulo con dos
características del *fast GPIO* que valen oro cuando manejás **un grupo de pines como una unidad** (un
bus de datos de un display, 8 LEDs que muestran un byte, los pines de un teclado matricial): el
registro **`FIOMASK`** y el **acceso por byte y half-word**. Son opcionales, pero conocerlas te evita
escribir lazos de bits a mano y te saca de encima una clase entera de bugs.

## El problema: escribir un grupo de pines sin tocar a los vecinos

Supongamos que en el puerto 2 tenés un bus de datos de 8 bits en `P2.0..P2.7`, y en `P2.8` un LED de
estado que **no** querés tocar. Querés volcar el valor `0x5A` al bus. La forma ingenua:

```c
LPC_GPIO2->FIOPIN = 0x5A;        // MAL: pone P2.8 (y todo el resto) en 0
```

Eso escribe **los 32 bits** del puerto: tu `0x5A` va a `P2.0..P2.7`, pero `P2.8` en adelante quedan en
0: apagaste el LED de estado sin querer. La alternativa con read-modify-write
(`FIOPIN = (FIOPIN & ~0xFF) | 0x5A`) funciona, pero **no es atómica**: si una interrupción toca otro
pin del puerto entre la lectura y la escritura, se pierde su cambio (es la misma carrera que vimos con
`FIOSET`/`FIOCLR` en la página 01). `FIOMASK` resuelve las dos cosas de un saque.

## FIOMASK: lo que está en 1 queda protegido

`FIOMASK` define, por cada bit del puerto, si ese pin **participa** o no de los accesos vía `FIOPIN`,
`FIOSET` y `FIOCLR`:

- **bit en 0** → el pin **participa** (se lee y se escribe normalmente). Es el valor de reset.
- **bit en 1** → el pin queda **protegido**: las escrituras a `FIOPIN`/`FIOSET`/`FIOCLR` **lo ignoran**,
  y al leer `FIOPIN` ese bit aparece como 0.

O sea: en `FIOMASK` marcás con 1 lo que querés **dejar quieto**. La regla mnemotécnica: *masked = oculto*.

Volviendo al ejemplo del bus de 8 bits con el LED en `P2.8`:

```c
// Proteger todo MENOS los 8 bits del bus: 1 en los pines a ocultar, 0 en P2.0..P2.7
LPC_GPIO2->FIOMASK = ~0xFFu;     // = 0xFFFFFF00: solo P2.0..P2.7 quedan habilitados

LPC_GPIO2->FIOPIN = 0x5A;        // escribe SOLO P2.0..P2.7; P2.8 y el resto, intactos
// ... más operaciones sobre el bus, todas seguras ...

LPC_GPIO2->FIOMASK = 0;          // IMPORTANTE: liberar la máscara al terminar
```

Con la máscara puesta, ese `FIOPIN = 0x5A` afecta **únicamente** los pines habilitados y es **atómico**
respecto de los pines protegidos: una ISR que toque `P2.8` no se pisa con esto. `FIOSET` y `FIOCLR`
también respetan `FIOMASK`.

> **El gotcha número uno de GPIO.** Un `FIOMASK` que quedó en un valor de antes es la causa clásica de
> "este pin no responde y no entiendo por qué": lo dejaste protegido en una rutina anterior y te
> olvidaste de liberarlo. Si no estás usando la máscara **a propósito**, dejala en **0** (su valor de
> reset). Ante un GPIO que "no anda", revisá `FIOMASK` antes que nada.

`FIODIR` **no** se ve afectado por `FIOMASK`: la dirección de los pines se configura siempre completa.
La máscara solo filtra los accesos de datos (`FIOPIN`/`FIOSET`/`FIOCLR`).

## Acceso por byte y por half-word

Cada registro del *fast GPIO* (de 32 bits) está además **mapeado en pedazos** más chicos, para que
puedas tocar 8 o 16 pines sin máscara y sin read-modify-write. Para cada puerto:

- **Por byte (8 bits):** `FIODIR0/1/2/3`, `FIOPIN0/1/2/3`, `FIOSET0/1/2/3`, `FIOCLR0/1/2/3`,
  `FIOMASK0/1/2/3`. El sufijo es el byte: `0` = bits 0–7, `1` = bits 8–15, `2` = bits 16–23, `3` =
  bits 24–31.
- **Por half-word (16 bits):** `FIODIRL`/`FIODIRH` (Lower = bits 0–15, High/Upper = bits 16–31) y sus
  equivalentes `FIOPINL/H`, `FIOSETL/H`, `FIOCLRL/H`, `FIOMASKL/H`.

> **Ojo con el nombre de la mitad alta.** El manual la llama `FIOxDIRU` (*Upper*), pero en `LPC17xx.h`
> el campo del struct se llama `FIODIRH` (*High*). Para que compile, en C usás **`FIODIRH`/`FIOPINH`/…**;
> el `...U` del datasheet es solo el nombre en la tabla de registros. La mitad baja sí coincide: `...L`.

Son **vistas del mismo registro físico**: escribir `LPC_GPIO2->FIOPIN0` toca exactamente `P2.0..P2.7`
y nada más, sin afectar `P2.8..P2.31` y sin necesidad de `FIOMASK`. El bus de 8 bits del ejemplo
anterior se reduce a:

```c
LPC_GPIO2->FIOPIN0 = 0x5A;       // escribe P2.0..P2.7; el resto del puerto, intacto. Sin máscara.
```

Más limpio, una sola instrucción, atómico respecto del resto del puerto. Por eso, cuando tu grupo de
pines **cae justo** en un byte o un half-word del puerto, el acceso parcial le gana a `FIOMASK` en
simplicidad. `FIOMASK` queda para cuando el grupo **no** está alineado a un byte (p. ej. `P0.4..P0.11`,
que cruza el límite de byte).

> En estos structs CMSIS, los registros por byte son `uint8_t` y los half-word `uint16_t`, así que el
> compilador emite la escritura del ancho justo. No hace falta desplazar ni enmascarar a mano.

## Con el driver CMSIS

El driver `lpc17xx_gpio` envuelve todo esto (lo confirmás abriendo `lpc17xx_gpio.c`):

```c
#include "lpc17xx_gpio.h"

// FIOMASK: maskValue con 1 protege, 0 habilita (igual que el registro)
FIO_SetMask(2, ~0xFFu, 1);       // proteger todo menos P2.0..P2.7
LPC_GPIO2->FIOPIN = 0x5A;        // o GPIO_SetValue/ClearValue, ahora "filtrados"
FIO_SetMask(2, 0xFFFFFFFFu, 0);  // liberar

// Acceso por byte y half-word, sin tocar máscara
FIO_ByteSetValue(2, 0, 0x5A);    // setea (en alto) los bits indicados del byte 0 de P2
FIO_ByteClearValue(2, 0, 0xFF);  // limpia el byte 0 entero
FIO_HalfWordSetValue(2, 0, 0x1234); // half-word lower de P2
```

`FIO_ByteSetValue`/`FIO_ByteClearValue` van a `FIOSETx`/`FIOCLRx`, así que **heredan la atomicidad** de
set/clear: setean o limpian los bits que les pasás sin un read-modify-write. Es la forma idiomática de
manejar un bus de 8 pines.

## Cuándo usar cada cosa

| Situación | Herramienta |
|-----------|-------------|
| Un solo pin, encender/apagar | `FIOSET`/`FIOCLR` (página 01) |
| Grupo de pines **alineado** a un byte o half-word | acceso parcial `FIOPIN0`/`FIOSETL`/… |
| Grupo de pines **no alineado** (cruza bytes) | `FIOMASK` + `FIOPIN` |
| Volcar un valor a un puerto que usás **entero** | `FIOPIN = valor` directo |

Y la regla de oro que se repite: si usaste `FIOMASK`, **acordate de volver a ponerlo en 0**.

## Lo que te llevás
- `FIOMASK` protege pines: **1 = oculto** (no se lee ni se escribe vía FIOPIN/SET/CLR), 0 = participa.
  Permite tratar un subconjunto de pines como grupo, de forma atómica respecto del resto.
- Cada registro FIO se accede también **por byte** (`FIOPIN0..3`) y **half-word** (`FIOPINL/H`): tocar
  8 o 16 pines sin máscara ni read-modify-write.
- Para un grupo alineado, el acceso parcial es lo más simple; para uno no alineado, `FIOMASK`.
- Un `FIOMASK` olvidado es una causa típica de "el pin no responde": dejalo en 0 salvo que lo uses a
  propósito.

---

**Anterior:** [03 - Debounce y filtrado de entradas](./03-debounce-y-filtrado-de-entradas.md) ·
**Módulo:** [GPIO](./README.md) · **Siguiente módulo:** [06 - SysTick](../06_systick/)
