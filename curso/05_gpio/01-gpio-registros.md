# GPIO a nivel registro

Los **GPIO** (*General Purpose Input/Output*) son los pines digitales del micro: con ellos prendés
LEDs, leés botones, manejás relés, generás señales o leés sensores digitales. Cada pin puede ser
**salida** (vos decidís si pone 0 o 1) o **entrada** (leés el nivel que le impone el mundo). Esta
página va al fondo de los registros que controlan eso.

## Fast GPIO (FIO): el bloque que realmente usás

El LPC1769 hereda mucho de la familia LPC2300 (ARM7). Aquellos micros tenían un GPIO **legacy**
mapeado en el bus periférico **APB**, lento: cada acceso costaba varios ciclos porque el APB corre a
una fracción del reloj del núcleo y hay puentes de por medio. NXP agregó después el **Fast GPIO
("FIO")**: el mismo periférico, pero re-mapeado en el bus **AHB** a partir de `0x2009C000`, donde el
Cortex-M3 lo alcanza con un acceso de **un solo ciclo**. En el LPC176x ese GPIO legacy ya ni siquiera
está expuesto de forma útil: **vos siempre usás el FIO**. Por eso todos los registros empiezan con
`FIO` y por eso podés conmutar un pin a la frecuencia del núcleo sin penalización.

Hay **5 puertos** (0 a 4). Cada uno tiene exactamente el mismo juego de registros, separados cada
`0x20` bytes:

| Puerto | Base | CMSIS |
|--------|------|-------|
| 0 | `0x2009C000` | `LPC_GPIO0` |
| 1 | `0x2009C020` | `LPC_GPIO1` |
| 2 | `0x2009C040` | `LPC_GPIO2` |
| 3 | `0x2009C060` | `LPC_GPIO3` |
| 4 | `0x2009C080` | `LPC_GPIO4` |

En CMSIS, `LPC_GPIO0` es un puntero a una `struct LPC_GPIO_TypeDef` (definida en `LPC17xx.h`) con los
campos `FIODIR`, `FIOMASK`, `FIOPIN`, `FIOSET`, `FIOCLR`. Escribir `LPC_GPIO0->FIOSET = ...` es escribir
en `0x2009C018`. No hay nada especial: es un acceso a memoria. Y como es memoria común, los registros
FIO soportan el **bit-banding** del Cortex-M3 (módulo 1) y hasta pueden ser leídos/escritos por el
**GPDMA**.

## Lo que GPIO no necesita

GPIO es cómodo porque varias cosas ya vienen resueltas tras el reset:

- **Power:** el bloque GPIO está **siempre encendido**. No hace falta tocar `PCONP` (módulo 3).
- **PINSEL:** tras el reset **todos** los pines son GPIO (función `00`). Solo tocás `PINSEL` (módulo 4)
  si el pin venía de otra función.
- **Dirección:** tras el reset todos los pines son **entrada con pull-up** activado. O sea, "flotan
  arriba" hasta que vos decidas otra cosa.

## Los cinco registros de cada puerto

Para el puerto 0 (los demás suman `0x20`, `0x40`, …):

| Registro | Dirección | Acceso | Función |
|----------|-----------|--------|---------|
| `FIO0DIR`  | `0x2009C000` | R/W | Dirección: bit en **1 = salida**, **0 = entrada** |
| `FIO0MASK` | `0x2009C010` | R/W | Máscara: bit en **1 = pin protegido** (no se lee ni se escribe vía PIN/SET/CLR) |
| `FIO0PIN`  | `0x2009C014` | R/W | Estado real del pin (leer entradas) / escribe el puerto entero |
| `FIO0SET`  | `0x2009C018` | R/W | Escribir **1 pone el pin en alto**; escribir 0 no lo toca |
| `FIO0CLR`  | `0x2009C01C` | WO  | Escribir **1 pone el pin en bajo**; escribir 0 no lo toca |

La regla mental: **`FIODIR` decide quién manda el pin** (vos o el mundo), y después
`FIOSET`/`FIOCLR`/`FIOPIN` mueven el valor.

## FIODIR: dirección por bit

`FIODIR` es un registro donde **cada bit gobierna un pin**: el bit 0 controla a `Px.0`, el bit 22 a
`Px.22`, etc. Un **1 lo hace salida**, un **0 lo hace entrada**. Como tras reset todo es entrada,
configurar una salida es simplemente poner su bit en 1:

```c
#include <LPC17xx.h>
#define LED  (1u << 22)   // P0.22

LPC_GPIO0->FIODIR |=  LED;   // P0.22 como salida (deja el resto como estaba)
LPC_GPIO0->FIODIR &= ~LED;   // volverlo entrada
```

> Nota de hardware: `P0.29` y `P0.30` están compartidos con `USB_D+`/`USB_D-` y **están obligados a
> tener la misma dirección**. Solo son salidas si **ambos** bits (29 y 30) de `FIO0DIR` están en 1;
> con que uno solo esté en 0, el silicio deja a **los dos** como entrada. Es el único par con esa
> atadura.

## Salidas: SET y CLR, y por qué son atómicos

Para mover una salida ya configurada tenés dos caminos. El correcto:

```c
LPC_GPIO0->FIOSET = LED;   // prende P0.22 (escribe 1 en el bit 22; el resto, 0, no se toca)
LPC_GPIO0->FIOCLR = LED;   // apaga  P0.22
```

Fijate en algo clave: `FIOSET` y `FIOCLR` son **registros de máscara de acción**. Un **1 en el bit
actúa** sobre ese pin; un **0 lo deja como estaba**. Por eso escribís `FIOSET = LED` (asignación, no
`|=`): no necesitás leer nada antes, los ceros del valor ya significan "no toques esos pines". Esto
hace que la operación sea **atómica**: es una **única escritura** a memoria, sin pasos intermedios.

Dos detalles finos del manual: `FIOSET`/`FIOCLR` solo actúan sobre pines configurados como **salida
GPIO** (en una entrada o en un pin con función alternativa, escribir el 1 no hace nada), y **leer**
`FIOSET` no te da el estado físico de los pines sino el contenido del **registro de salida** (lo
último que escribiste vía SET/CLR/PIN). Para el estado real, siempre `FIOPIN`.

Comparalo con el camino tentador pero peligroso:

```c
LPC_GPIO0->FIOPIN |= LED;   // FUNCIONA, pero es read-modify-write
```

Ese `|=` se compila en **tres** pasos: leer `FIOPIN`, hacer el OR, escribir de vuelta el puerto
**entero**. Es más lento, y sobre todo **no es atómico**. Si entre el "leer" y el "escribir" salta una
interrupción (módulo 7) que toca **otro** pin del mismo puerto, esa modificación se pierde cuando tu
escritura pisa el puerto completo con el valor viejo que habías leído. Es la clásica *race condition*:

```
main:   lee FIOPIN  (P0.5 = 0) ............................ escribe FIOPIN  (¡pisa P0.5 con 0!)
ISR:                  ......... pone P0.5 = 1 con FIOSET ...
                                          ^ este cambio se perdió
```

Con `FIOSET`/`FIOCLR` ese problema no existe: cada uno solo toca sus bits y nunca lee ni reescribe los
del vecino. **Para salidas, usá siempre SET/CLR.**

## Entradas: leer FIOPIN

Para leer un pin de entrada lo dejás como entrada (`0` en `FIODIR`, que es el default) y leés
`FIOPIN`, que devuelve el **estado físico real** del pin sin importar para qué esté configurado:

```c
#define BOTON  (1u << 10)        // P2.10

LPC_GPIO2->FIODIR &= ~BOTON;     // entrada (redundante: ya es 0 tras reset)

if (LPC_GPIO2->FIOPIN & BOTON) {
    // P2.10 está en ALTO
} else {
    // P2.10 está en BAJO (ej.: botón a GND apretado, con pull-up)
}
```

> El nivel que leés depende del cableado **y** del `PINMODE` (módulo 4): un botón a GND necesita
> **pull-up** (default), uno a VCC necesita **pull-down**. Sin pull, una entrada al aire "flota" y leés
> ruido. Y ojo: si el pin está en modo **analógico (ADC)**, `FIOPIN` **no es válido**: el ADC
> desconecta la parte digital del pin.

`FIOPIN` también se puede **escribir**, y ahí escribe el **puerto entero** de una (los 0 ponen pines en
bajo, los 1 en alto). Es útil con `FIOMASK` para volcar un valor de varios bits de un saque; lo vemos
en la [página 04](./04-fiomask-y-acceso-por-byte.md).

## FIOMASK: tocar solo un grupo de pines

`FIOMASK` es la herramienta para tratar un **subconjunto** de pines de un puerto como un grupo, sin
molestar a los vecinos. La convención es **invertida y conviene grabársela**:

- bit en **0** → pin **habilitado** (se lee y se escribe normalmente),
- bit en **1** → pin **protegido**: no lo afectan las escrituras a `FIOPIN`/`FIOSET`/`FIOCLR`, y al
  leer `FIOPIN` **devuelve 0** (no refleja el estado físico).

Es decir: en `FIOMASK` marcás con 1 lo que querés **dejar quieto**. Esto sirve, por ejemplo, para
escribir un **bus** de varios pines de un puerto sin tocar a los vecinos. El detalle fino (con el bus
de datos de un LCD como ejemplo) y las trampas están en la
[página 04](./04-fiomask-y-acceso-por-byte.md). Si **no** estás usando la máscara a propósito, dejala
en **0** (su valor de reset): un `FIOMASK` quedado de antes es una causa típica de "este pin no
responde" o "siempre lee 0".

## Corriente por pin: cuánto podés exigirle

Un pin GPIO no es una fuente ideal. Cada salida del LPC1769 entrega o absorbe alrededor de **4 mA**
de forma confiable (con caída de tensión acotada). Dos modos:

- **Source** (la salida en alto "empuja" corriente hacia la carga, p. ej. LED a GND con resistencia):
  el pin entrega corriente.
- **Sink** (la salida en bajo "traga" corriente, p. ej. LED desde VCC con su resistencia al pin): el
  pin absorbe corriente hacia `VSS`.

Consecuencias prácticas:

- Un **LED** va **siempre con resistencia en serie** (típico 220–470 Ω a 3.3 V). Sin ella, el LED pide
  más de lo que el pin tolera y degradás el chip.
- Para **relés, motores o tiras de LEDs** no manejás la carga directo: usás un **transistor** o un
  driver, y el GPIO solo comanda la base/gate.
- Hay un **límite total por chip**: la suma de corrientes que pasan por cada pin de masa (`VSS`) está
  acotada. No prendas 30 LEDs directo "porque cada uno son solo 4 mA".

## Qué pines existen en cada puerto (los huecos)

Importante: **no** existen los 32 pines en los 5 puertos. La distribución real en el LPC1769
(encapsulado LQFP100) es:

| Puerto | Pines disponibles | Huecos / notas |
|--------|-------------------|----------------|
| **P0** | `P0.0`–`P0.30` | **No existen** `P0.12`, `P0.13`, `P0.14` ni `P0.31` |
| **P1** | `P1.0`–`P1.31` | **No existen** `P1.2`, `P1.3`, `P1.5`, `P1.6`, `P1.7`, `P1.11`, `P1.12`, `P1.13` |
| **P2** | `P2.0`–`P2.13` | de `P2.14` para arriba no existen |
| **P3** | solo `P3.25` y `P3.26` | el resto no existe |
| **P4** | solo `P4.28` y `P4.29` | el resto no existe |

Escribir o leer un bit de un pin que no existe simplemente no hace nada útil. Antes de elegir un pin,
verificá en el manual (capítulo 8, *Pin configuration*) que exista **y** qué función trae por defecto.
Solo los puertos **0 y 2** pueden generar interrupciones (lo vemos abajo y en el módulo 7).

## Acceso por byte y half-word (anticipo)

Además del acceso de 32 bits, **cada** registro FIO es accesible de a **1 byte** (`FIODIR0..3`,
`FIOPIN0..3`, etc.) y de a **2 bytes** (`FIODIRL`/`FIODIRH`, …). Por ejemplo, `LPC_GPIO2->FIOPIN0` son
los 8 bits bajos (`P2.0..P2.7`) del puerto 2, y escribir ahí toca solo esos 8 pines, sin necesitar
`FIOMASK`. Esto se profundiza en la [página 04](./04-fiomask-y-acceso-por-byte.md).

## Ejemplo completo: LED que sigue a un botón (a registro)

```c
#include <LPC17xx.h>

#define LED    (1u << 22)   // P0.22 (salida)
#define BOTON  (1u << 10)   // P2.10 (entrada, botón a GND con pull-up)

int main(void) {
    // PINSEL: ambos ya son GPIO por defecto. PINMODE de P2.10: pull-up (default 00).
    LPC_GPIO0->FIODIR |=  LED;     // LED como salida
    LPC_GPIO2->FIODIR &= ~BOTON;   // botón como entrada

    while (1) {
        if (LPC_GPIO2->FIOPIN & BOTON) {
            LPC_GPIO0->FIOCLR = LED;   // botón suelto (alto) -> LED apagado
        } else {
            LPC_GPIO0->FIOSET = LED;   // botón apretado (bajo) -> LED prendido
        }
    }
}
```

Esto ya es un programa GPIO completo a registro. Funciona, pero el botón **rebota**: un solo apretón
puede leerse como varios. Eso lo resolvemos en la
[página 03 (debounce)](./03-debounce-y-filtrado-de-entradas.md). Y si en vez de *pollear* el botón
querés que el micro **reaccione a un flanco**, eso es la **interrupción por GPIO**, que vive en otro
bloque (`GPIOINT`, distinto de estos registros FIO) y se ve en el [módulo 7](../07_interrupciones/).

## Errores típicos

- Usar `FIOPIN |= bit` para una sola salida: anda en demos, pero introduce la *race* con una ISR. Usá
  `FIOSET`/`FIOCLR`.
- **Olvidar `FIODIR`:** configurás todo y el LED no prende porque el pin quedó como entrada.
- **Confundir SET con CLR** (o creer que `FIOCLR = 0` apaga: no, hay que escribir el **1** del bit).
- Un **`FIOMASK` quedado** de una rutina anterior que "tapa" pines.
- Leer `FIOPIN` de un pin en modo **ADC** y creerle al valor.

En la [próxima página](./02-gpio-con-driver.md) reescribimos este mismo ejemplo con el driver de CMSIS
y comparás línea por línea.

---

**Módulo:** [GPIO](./README.md) ·
**Siguiente:** [02 - GPIO con el driver CMSIS](./02-gpio-con-driver.md)
