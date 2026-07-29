# PINMODE a fondo, open-drain, I²C y tolerancia a 5 V

Esta página cierra el bloque PINCON con lo que más se confunde y lo que más rompe hardware: el
**modo de entrada** (resistencias), el **modo de salida** (open-drain), el **caso especial de I²C0**,
y la **tolerancia a 5 V**. Es la diferencia entre un pin que mide bien, un bus que arranca, y un pin
quemado.

## PINMODE: las cuatro maneras de tratar una entrada

`PINMODE` decide qué pasa con un pin **cuando nadie lo está manejando** (entrada, o salida
open-drain en estado alto). Cuatro modos, 2 bits:

| Bits | Modo | Qué hace |
|------|------|----------|
| `00` | pull-up | resistencia interna hacia el positivo: el pin "tiende a 1" |
| `01` | repeater | engancha el último nivel (ver abajo) |
| `10` | none (tri-state) | sin resistencias: el pin flota |
| `11` | pull-down | resistencia interna a GND: el pin "tiende a 0" |

> El orden vuelve a ser contraintuitivo: `00` es pull-up (no "nada"), y el "nada" es `10`. En el
> driver no hay constante para repeater (lo vemos al final); las que existen son
> `PINSEL_PINMODE_PULLUP` (0), `PINSEL_PINMODE_TRISTATE` (2) y `PINSEL_PINMODE_PULLDOWN` (3).

### Pull-up y pull-down: para entradas con un solo extremo activo

Una entrada digital "suelta" (sin conectar) no vale 0 ni 1: **flota** y lee basura (ruido, 50 Hz de
la red, lo que toque). La resistencia interna le da un valor de reposo definido:

- **Pull-up (`00`)**: reposo en 1. Es lo que querés para un **botón a GND**: en reposo el pull-up
  mantiene 1; al apretar, el botón corta a GND y leés 0. Este es el patrón más común y es el modo por
  defecto tras reset. Detalle fino del cap. 7: el pull-up interno **no llega a 3,3 V**; sostiene el
  pin en unos **2,3–2,6 V**, que alcanza de sobra para leer un 1 (las entradas son de niveles TTL).
- **Pull-down (`11`)**: reposo en 0. Para un **botón a VCC**: en reposo leés 0; al apretar, sube a 1.

> El error de "el botón anda a veces sí, a veces no" casi siempre es PINMODE en `10` (tri-state) en
> una entrada con botón: el pin flota y leés ruido. Solución: pull-up (botón a GND) o pull-down
> (botón a VCC). Y al revés, no pongas pull-up interno si ya tenés uno externo en la placa: quedan en
> paralelo (no rompe nada, pero baja la resistencia equivalente y consume más).

### El modo repeater (`01`): el más raro y el que nadie explica

Repeater es un **pull-up y un pull-down conmutados según el nivel actual** del pin:

- si el pin está en **alto**, se activa el **pull-up** (lo sostiene en alto),
- si el pin está en **bajo**, se activa el **pull-down** (lo sostiene en bajo).

El efecto: el pin **retiene su último estado conocido** mientras nadie lo maneja, sin fijar un valor
de reposo arbitrario. ¿Para qué sirve esto?

- **Evitar consumo por pin flotante en bajo consumo.** Un pin de entrada CMOS que flota a media
  tensión (ni 0 ni 1 claro) hace que la etapa de entrada conduzca y **chupe corriente**. En un diseño
  a batería eso importa. Repeater lo "ancla" al nivel donde estaba, cortando ese consumo, sin gastar
  corriente permanente como gastaría un pull-up fijo si el pin terminara en el nivel opuesto.
- **Buses multipunto** donde el último que manejó la línea la dejó en un nivel y querés que se quede
  ahí hasta que otro la mueva.

> Detalle del manual: la retención de estado de repeater **no aplica** en Deep Power-down. Y es un
> modo de nicho: en un TP normal casi nunca lo vas a necesitar; pull-up/pull-down/tri-state cubren el
> 99 % de los casos.

### None / tri-state (`10`): obligatorio para señales analógicas

Cuando el pin se usa como **entrada de ADC** (o salida de DAC), cualquier resistencia interna
**altera la medición**: un pull-up de unas decenas de kΩ forma un divisor con la fuente de la señal y
te corre el valor leído. Por eso, para un canal AD0.x configurás siempre PINMODE en `10` (tri-state):

```c
// AD0.0 = P0.23 func 1, analógico -> tri-state
LPC_PINCON->PINSEL1  &= ~(0x3u << 14);   // P0.23 en PINSEL1 bits 15:14
LPC_PINCON->PINSEL1  |=  (0x1u << 14);   // func 1 = AD0.0
LPC_PINCON->PINMODE1 &= ~(0x3u << 14);
LPC_PINCON->PINMODE1 |=  (0x2u << 14);   // 10 = tri-state (sin resistencias)
```

> Los canales analógicos del LPC1769 son: **AD0.0=P0.23, AD0.1=P0.24, AD0.2=P0.25, AD0.3=P0.26,
> AD0.4=P1.30, AD0.5=P1.31, AD0.6=P0.3, AD0.7=P0.2**. La salida del DAC (**AOUT**) sale por
> **P0.26** (que también es AD0.3). Todos ellos: tri-state cuando los usás en modo analógico.

## PINMODE_OD: salida open-drain por pin

`PINMODE_OD` (1 bit por pin, un registro por puerto; ver la fórmula en la
[página 03](./03-mapa-de-registros.md)) cambia cómo es la **salida**:

| Bit | Salida | Comportamiento |
|-----|--------|----------------|
| `0` | normal (push-pull) | el pin fuerza activamente 0 **y** 1 |
| `1` | open-drain | el pin **solo tira a 0**; para el 1 apaga el driver y queda en alta impedancia |

En open-drain, escribir un 0 conecta el pin a GND (lo tira abajo); escribir un 1 **no pone 3,3 V**,
sino que **suelta** el pin. Para que el "1" sea un 1 de verdad hace falta una **resistencia de pull-up
externa** que levante la línea. Esto, que parece una limitación, habilita tres cosas:

- **I²C y otros buses de drenaje abierto.** Varios dispositivos comparten una línea (SDA/SCL). Si
  todos fueran push-pull y uno pusiera 0 y otro 1, habría cortocircuito. Con open-drain, cualquiera
  puede tirar la línea a 0 sin pelearse con los demás; el 1 lo da el pull-up común del bus.
- **Wired-AND.** Como cualquier nodo puede forzar 0 y el 1 es "pasivo", la línea vale 1 **solo si
  todos** sueltan: es un AND por cableado, sin compuerta. Útil para señales de "todos listos" o
  alarmas compartidas.
- **Adaptar niveles / manejar cargas a otra tensión.** Como en estado alto el pin queda flotando, el
  pull-up externo lo puede llevar a una tensión distinta de 3,3 V (por ejemplo 5 V), **siempre que el
  pin sea tolerante a 5 V** (ver más abajo). Así un pin de 3,3 V puede manejar una entrada de lógica
  de 5 V sin level-shifter, dentro de los límites del pin.

### El detalle fino: PINMODE en un pin open-drain

El manual aclara algo sutil. Normalmente PINMODE solo aplica cuando el pin es entrada. Pero en
open-drain, **cuando el pin está sacando un 0**, su driver está conduciendo, así que PINMODE no
aplica. **Cuando saca un 1** (driver apagado), el pin queda en alta impedancia y **ahí PINMODE sí
aplica**. Esto te deja una combinación útil: open-drain **con pull-up interno**, donde el pull-up solo
actúa cuando el pin no se está tirando a 0 a sí mismo. Es un pull-up "que no estorba" mientras manejás
la línea.

## El caso especial I²C0: P0.27 / P0.28 y `I2CPADCFG`

Acá hay una trampa para el desprevenido. **P0.27 (SDA0) y P0.28 (SCL0)** no son pines open-drain
"normales": son pines **open-drain de hardware, compatibles con I²C de verdad**, con filtro de glitch
y control de slew rate incorporados. Consecuencias:

- **No tienen resistencias configurables.** Sus bits en `PINMODE0`/`PINMODE1` no hacen nada para esos
  pines. No les pongas pull-up interno: no existe.
- **Sus bits en `PINMODE_OD0` se ignoran.** Ya son open-drain por construcción; no hace falta (ni
  sirve) marcarlos en PINMODE_OD.
- **El open-drain aplica a *todas* las funciones del pin**, no solo a I²C (cap. 7: *"Open-drain
  configuration applies to all functions on this pin"*). Si usás P0.27/P0.28 como GPIO de salida,
  siguen sin poder forzar un 1: necesitás pull-up externo igual. Es la trampa clásica de "conecté un
  LED a P0.27 y no prende".
- **Se configuran con `I2CPADCFG`** (`0x4002C07C`), un registro aparte solo para estos dos pines.
- Detalle de diseño: si el micro está **sin alimentación**, estos pines quedan flotando y **no cargan
  el bus I²C**; los demás dispositivos pueden seguir conversando.

`I2CPADCFG` tiene 4 bits útiles (nombres exactos de `lpc17xx_pinsel.h`):

| Bit | Macro | Pin | Qué controla |
|-----|-------|-----|--------------|
| 0 | `PINSEL_I2CPADCFG_SDADRV0` | P0.27 SDA0 | drive: 0 = standard, 1 = Fast Mode Plus |
| 1 | `PINSEL_I2CPADCFG_SDAI2C0` | P0.27 SDA0 | 0 = filtro+slew I²C activos, 1 = desactivados |
| 2 | `PINSEL_I2CPADCFG_SCLDRV0` | P0.28 SCL0 | drive: 0 = standard, 1 = Fast Mode Plus |
| 3 | `PINSEL_I2CPADCFG_SCLI2C0` | P0.28 SCL0 | 0 = filtro+slew I²C activos, 1 = desactivados |

Las reglas prácticas:

- **I²C standard o Fast (hasta 400 kHz):** dejá `I2CPADCFG = 0` (el valor de reset). No tenés que
  tocar nada.
- **Fast Mode Plus (1 MHz):** poné `SDADRV0` y `SCLDRV0` en 1 (más corriente de drive para flancos
  rápidos).
- **Uso NO-I²C de esos pines** (los usás como GPIO u otra cosa): poné `SDAI2C0` y `SCLI2C0` en 1 para
  apagar el filtro y el control de slew rate, que si no te deforman señales que no son I²C.

El driver lo hace por vos con `PINSEL_SetI2C0Pins()`:

```c
// I2C0 estándar/Fast, con filtro y slew rate (lo normal)
PINSEL_SetI2C0Pins(PINSEL_I2C_Normal_Mode, ENABLE);

// Fast Mode Plus (1 MHz)
PINSEL_SetI2C0Pins(PINSEL_I2C_Fast_Mode, ENABLE);
```

> Importante: I²C1 e I²C2 **no** usan estos pines especiales. El propio cap. 7 los describe como
> *"this is not an I2C-bus compliant open-drain pin"*: son pines normales, no tienen el hardware
> open-drain dedicado de I²C0. SDA1/SCL1 y SDA2/SCL2 caen en pines normales (por ejemplo SDA1 puede ir
> en P0.0 o P0.19, SDA2 en P0.10) y **sí** necesitan que les pongas open-drain por `PINMODE_OD` a
> mano. El cap. 8 lo dice explícito (nota [3] de la tabla de PINMODE_OD0): *"Port 0 bits 1:0, 11:10, and
> 20:19 may potentially be used for I2C-buses using standard port pins. If so, they should be
> configured for open drain mode via the related bits in PINMODE_OD0"*, es decir P0.0/P0.1,
> P0.10/P0.11 y P0.19/P0.20. Solo I²C0 (P0.27/P0.28) es el caso de hardware dedicado.

## Tolerancia a 5 V: qué pin aguanta y cuál se quema

Este es el punto donde un error **rompe el micro de verdad**, no "no sale nada". El LPC1769 funciona
a 3,3 V, pero la regla del manual es al revés de lo que muchos asumen: **por defecto los pines de E/S
*sí* son tolerantes a 5 V** (con histéresis de entrada), salvo que la tabla los marque distinto.
Textual del capítulo 7 del manual:

> *"I/O pins on the LPC176x/5x are 5V tolerant and have input hysteresis unless indicated in the
> table below. Crystal pins, power pins, and reference voltage pins are not 5V tolerant. In addition,
> when pins are selected to be A to D converter inputs, they are no longer 5V tolerant and must be
> limited to the voltage at the ADC positive reference pin (VREFP)."*

O sea, los que **NO** toleran 5 V (y se dañan si te pasás de ~VDD+0,3 V) son:

- **Pines de cristal, de alimentación y de referencia** (VREFP/VREFN): no son 5 V-tolerant por
  construcción. Estos no son GPIO, no los vas a usar como E/S, pero conviene saberlo.
- **Los pines USB P0.29 (USB_D+) y P0.30 (USB_D−).** Su pad es especial para USB y la tabla de pines
  del cap. 7 lo marca explícito: *"This pad is not 5 V tolerant"*. Vale siempre, aunque los uses como
  GPIO.
- **Cualquier pin *mientras esté seleccionado como entrada del ADC*.** Y acá está el matiz fino que
  hay que entender bien: un pin como **P0.23** (AD0.0) **sí** es 5 V-tolerant cuando lo usás como
  GPIO/digital; **deja de serlo** en el instante en que lo configurás como canal analógico. No es que
  el pad sea "no tolerante" siempre: **pierde la tolerancia al pasar a modo ADC**, y ahí su entrada
  queda limitada a VREFP. Los pines en cuestión son los que comparten con el ADC/DAC: **P0.23, P0.24,
  P0.25, P0.26** (este también es AOUT del DAC) y **P1.30, P1.31, P0.2, P0.3**. Regla simple: si el
  pin está midiendo analógico, tratalo como **no** tolerante y respetá 0–VREFP.

Dos aclaraciones que evitan errores:

- **El modo open-drain no cambia la tolerancia.** La tolerancia a 5 V es una propiedad del *pad*, no
  del modo de salida. Que un pin sea open-drain **no** lo vuelve 5 V-tolerant: solo podés tirar de él
  a 5 V con pull-up externo **si ese pin de por sí ya es tolerante** (la mayoría lo son, pero no en
  modo ADC).
- **Los pines I²C0 (P0.27/P0.28) sí toleran 5 V.** La tabla de pines del cap. 7 describe su pad como
  *"Open-drain 5 V tolerant digital I/O pad, compatible with I2C-bus 400 kHz specification"*. Por eso
  un bus I²C con pull-ups a 5 V es válido en estos pines (los demás pines de I²C1/I²C2 también, por
  ser pads normales tolerantes, salvo, como siempre, los que estén en modo ADC).

> El listado pin por pin (la columna *Type* de la tabla "Pin description", cap. 7, con "5V tolerant"
> sí/no) y las cifras de corriente máxima viven entre el **capítulo 7 del User Manual** y el **data
> sheet** del LPC1769. Regla de seguridad para el laboratorio: **antes de conectar 5 V a un pin,
> verificá su tipo en la tabla; si ese pin lo vas a usar como entrada de ADC, NO le metas 5 V; ante la
> menor duda, divisor resistivo o level-shifter.**

## Corriente de drive de los pines

Los pines de E/S del LPC1769 manejan, por defecto, una corriente modesta (del orden de **4 mA**
nominales por pin como referencia de diseño). Implicaciones prácticas:

- **No cuelgues un LED de alto brillo o un relé directo del pin** esperando que ilumine/accione fuerte:
  o le ponés una resistencia adecuada para quedarte dentro del límite, o usás un transistor/MOSFET de
  potencia como etapa intermedia.
- **Cuidá la suma por banco de pines.** No es solo el límite por pin: hay un límite total de corriente
  que entra y sale del chip. Encender muchas salidas a la vez puede pasarse aunque cada una esté en
  rango.
- Los pines **I²C0 (P0.27/P0.28)** tienen su control de drive aparte (los bits `*DRV0` de `I2CPADCFG`):
  Fast Mode Plus sube la corriente para flancos más rápidos en buses exigentes.

> Igual que la tolerancia, las cifras exactas de drive (IOH/IOL por pin y total) están en el data
> sheet, no en el User Manual. Para el curso alcanza la intuición: el pin **señaliza**, no **alimenta
> potencia**. Si algo consume más que un LED chico, va con etapa de potencia.

## Errores típicos de este tema (y cómo se manifiestan)

| Síntoma | Causa probable | Arreglo |
|---------|----------------|---------|
| Botón lee valores aleatorios | PINMODE en tri-state (`10`) en la entrada | pull-up (`00`) si va a GND, pull-down (`11`) si va a VCC |
| Lectura de ADC corrida/ruidosa | pull-up/down interno en el pin analógico | PINMODE `10` (tri-state) en ese canal |
| I²C0 no arranca | falta el pull-up externo del bus (en P0.27/P0.28 no hay interno, y `PINMODE`/`PINMODE_OD` se ignoran) | pull-ups externos en SDA0/SCL0; `I2CPADCFG` queda en 0 para standard/Fast |
| I²C1/I²C2 no arranca | olvidaste el open-drain en sus pines normales | `PINMODE_OD` = 1 en esos pines (no son los dedicados) |
| Pin/micro dañado tras conectar 5 V | metiste 5 V a un pin **no** tolerante (o analógico) | verificar tolerancia en el data sheet; divisor/level-shifter |
| LED tenue o salida que no "empuja" | esperabas más corriente de la que da el pin | resistencia correcta o etapa de transistor |

## Nota sobre el driver: el repeater no tiene constante

Si mirás `lpc17xx_pinsel.h`, las constantes de modo son solo tres:
`PINSEL_PINMODE_PULLUP` (0), `PINSEL_PINMODE_TRISTATE` (2) y `PINSEL_PINMODE_PULLDOWN` (3).
**No hay `PINSEL_PINMODE_REPEATER`.** Si querés repeater (`01`) con el driver, le pasás el `1`
crudo en el campo `Pinmode`, o lo escribís a registro:

```c
// P0.0 en repeater, a registro (01 en PINMODE0 bits 1:0)
LPC_PINCON->PINMODE0 &= ~(0x3u << 0);
LPC_PINCON->PINMODE0 |=  (0x1u << 0);
```

Para el resto de los modos, las constantes del driver alcanzan y son más legibles que el número
crudo.

---

**Anterior:** [03 - El mapa completo de registros](./03-mapa-de-registros.md) ·
**Módulo:** [PINSEL](./README.md) ·
**Siguiente módulo:** [05 - GPIO](../05_gpio/)
