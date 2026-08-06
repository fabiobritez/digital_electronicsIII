# Electrónica mínima para no romper el micro

El LPC1769 es robusto, pero tiene **límites eléctricos**. Pasarse de uno puede dar lecturas raras,
resets espontáneos, o quemar un pin (o el chip). Estas son las reglas que evitan el 90% de los
desastres de hardware.

## 1. El micro trabaja a 3.3 V, no a 5 V

El LPC1769 se alimenta y opera a **3.3 voltios**. Eso significa:

- Un **1 lógico** en una salida son ~3.3 V (no 5 V).
- En una **entrada**, aplicarle 5 V **puede dañar el pin** si no es "tolerante a 5 V".

> Casi todos los pines digitales del LPC1769 son **5 V-tolerant** (aguantan 5 V en la entrada), pero
> hay excepciones: un pin **configurado como entrada de ADC** pierde la tolerancia (no debe pasar de
> 3.3 V), y **P0.29/P0.30** (los pines de USB) **no son tolerantes nunca**. Antes de conectar algo de
> 5 V (un sensor, otro micro, un módulo Arduino), **revisá en el datasheet o en el capítulo 7 del
> User Manual** qué pin es tolerante. Ante la duda, usá un **divisor resistivo** o un **conversor de
> nivel** (level shifter) para bajar 5 V a 3.3 V.

Conectar dos dispositivos de distinto voltaje sin pensar es una causa típica de "funcionaba y de
repente murió".

## 2. Corriente máxima por pin: el micro no maneja potencia

Un pin de GPIO puede entregar o absorber **poca corriente**: del orden de **4 mA** de forma
recomendada (hasta ~40 mA absolutos por pin según el datasheet, pero no te acerques a eso). Con eso:

- Un **LED** (que consume unos pocos mA con su resistencia) → **sí**, directo.
- Un **motor**, un **relé**, una **tira de LEDs**, un **buzzer** grande → **NO directo**. Consumen
  cientos de mA o más: le pedís al pin mucho más de lo que puede dar, y se cae la tensión, se resetea
  el micro, o se quema el pin.

Para manejar algo que consume corriente, **no lo conectás al pin directamente**: el pin controla un
**transistor** (o un MOSFET, o un driver / un relé), y el transistor maneja la corriente desde la
fuente:

```
                      +V (fuente del motor)
                       │
                    [ motor ]
                       │
   P0.22 ──[ R ]── base/gate
                       │ transistor (hace de "interruptor")
                      GND
```

El pin solo "abre y cierra" el transistor con muy poca corriente; la potencia la entrega la fuente.
Regla: **el micro decide, el transistor (o driver) hace la fuerza.**

## 3. Un LED siempre lleva resistencia

Un LED conectado directo a un pin (sin resistencia) deja pasar demasiada corriente: se quema el LED,
el pin, o ambos. La **resistencia en serie limita la corriente**. El valor sale de la ley de Ohm:

```
R = (V_pin - V_led) / I_led
```

Para un LED rojo típico (V_led ≈ 2 V, I deseada ≈ 5 mA) con el pin a 3.3 V:

```
R = (3.3 - 2) / 0.005 = 260 Ω  ->  usás 270 Ω o 330 Ω (valor comercial cercano)
```

> Regla rápida: para LEDs indicadores en 3.3 V, **220–470 Ω** casi siempre anda bien. Más resistencia
> = menos brillo pero más seguro.

## 4. Pull-up y pull-down: que las entradas no "floten"

Un pin de **entrada** sin nada conectado **flota**: capta ruido y lee 0 y 1 al azar. Una resistencia
de **pull-up** (a VCC) o **pull-down** (a GND) le da un valor definido cuando nada lo está manejando:

- **Pull-up**: el pin queda en 1 en reposo. Para botones que conectan a GND.
- **Pull-down**: el pin queda en 0 en reposo. Para botones que conectan a VCC.

El LPC1769 tiene pull-ups/pull-downs **internos** (los configurás con PINMODE, módulo 4), así que para
un botón simple no necesitás resistencia externa. Pero para buses (I2C) o señales largas, a veces se
usan **externos** de un valor específico.

Un detalle que confunde al medir: el pull-up interno del LPC1769 **no** lleva el pin a 3.3 V, sino a
un nivel de **2.3 a 2.6 V** (lo dice el capítulo 7 del User Manual). Igual se lee como 1 lógico; si
medís ~2.5 V en una entrada en reposo, no es una falla.

> Síntoma de pull faltante: una entrada (botón, sensor) que "tiembla" o cambia sola. Solución: activá
> el pull interno o poné uno externo.

## 5. Capacitores de desacople (los que "no hacen nada" pero son críticos)

Vas a ver, cerca de cada chip, capacitores chicos (100 nF) entre VCC y GND. Son los **capacitores de
desacople** (*decoupling*). Sirven para **estabilizar la alimentación**: cuando el chip cambia de
estado consume picos de corriente, y estos capacitores los absorben para que la tensión no "tiemble".

- En una placa comercial ya vienen puestos.
- Si armás algo en protoboard, **agregá un 100 nF entre VCC y GND cerca del micro** (y un electrolítico
  de ~10 µF en la entrada de alimentación). Sin desacople, un micro puede tener resets aleatorios y
  comportamiento errático imposible de depurar por software.

## 6. GND común: la regla de oro

Cualquier cosa que conectes al micro (sensor, módulo, otra placa) **tiene que compartir el GND**
(masa) con el micro. Las señales son tensiones **respecto de GND**; sin una referencia común, "alto" y
"bajo" no significan nada. Es el error #1 de quien arma su primer circuito: conectan la señal pero
olvidan el GND, y nada anda.

## Checklist antes de conectar algo

1. ¿Es de **3.3 V** o de 5 V? Si es 5 V, ¿el pin lo tolera, o necesito bajar el nivel?
2. ¿Cuánta **corriente** consume? Si es más que un LED, va con transistor/driver, no directo.
3. Si es un LED, ¿tiene **resistencia**?
4. Si es una entrada, ¿tiene **pull** (interno o externo)?
5. ¿Comparte **GND** con el micro?
6. Si es protoboard, ¿hay **desacople** cerca del micro?

Seguir esto evita casi todos los "se me quemó / se resetea / lee cualquier cosa". En la
[próxima página](./03-instrumentos-de-medicion.md): cómo **medir** y confirmar que todo está como
esperás.

---

**Anterior:** [01 - Tu placa por dentro](./01-tu-placa-por-dentro.md) ·
**Siguiente:** [03 - Instrumentos de medición](./03-instrumentos-de-medicion.md)
