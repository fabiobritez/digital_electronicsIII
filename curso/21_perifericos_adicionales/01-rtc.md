# RTC: Reloj de tiempo real

El **RTC** (*Real-Time Clock*) mantiene **fecha y hora** (segundos, minutos, horas, día, mes, año) de
forma autónoma, contando a partir de un cristal de **32.768 kHz**. Su gracia es doble: el hardware hace
todo el trabajo de calendario (sabe que después del 28 de febrero viene marzo, maneja años bisiestos,
los acarreos de hora a día a mes), y consume **menos de 1 microampere**, así que puede seguir contando
con una pila de respaldo aunque el resto del micro esté apagado por completo. Es lo que tenés adentro
de cualquier datalogger, reloj, o sistema que necesite un *timestamp* confiable.

Capítulo 27 del manual. Es un periférico distinto a todos los demás del LPC1769, porque tiene su
**propio dominio de alimentación** (pin `VBAT`), su **propio oscilador** (`RTCX1`/`RTCX2`) y está
**aislado** del resto del chip. Por eso sobrevive a cosas que apagan a todos los demás.

## Lo que lo hace especial: dominio de potencia propio

Antes de tocar un solo registro, entendé esto, porque cambia la forma de pensar el periférico.

El RTC vive en una **isla de potencia** separada. Esa isla puede alimentarse de dos fuentes:

- **`VDD(REG)(3V3)`**: la alimentación normal del micro. Mientras el LPC esté encendido, el RTC come
  de acá.
- **`VBAT`**: un pin dedicado al que típicamente conectás una **pila de litio de 3 V** (una CR2032,
  por ejemplo). Si se corta la alimentación principal, el RTC se pasa solo a la pila.

Un *power selector* interno elige automáticamente la fuente disponible. La consecuencia práctica:

- Mientras haya alimentación en **cualquiera** de las dos (`VDD(REG)(3V3)` o `VBAT`), el RTC **sigue
  contando la hora** y los **backup registers** (`GPREG0–4`) conservan su contenido.
- Si se cortan **las dos a la vez**, se pierde todo: la hora y los backup registers. Cuando vuelva la
  energía, vas a tener que reconfigurar la fecha desde cero.

Un dato que sorprende a todos: si no vas a usar el RTC, podés dejar `VBAT` **flotando** y los pines
`RTCX1`/`RTCX2` **al aire**. El micro arranca igual. Pero si querés que el reloj sobreviva a apagones,
`VBAT` necesita su pila y `RTCX1`/`RTCX2` su cristal de 32.768 kHz con sus capacitores.

> **Sobre `PCONP`.** Casi todos los periféricos arrancan apagados y los prendés con un bit de `PCONP`.
> El RTC es la excepción: **en reset ya viene habilitado** (`PCRTC`, bit 9, viene en 1). Igual conviene
> escribirlo explícito para que el código sea auto-documentado y porque el driver lo toca. Ese bit
> controla el acceso del CPU a los registros, no el conteo en sí: aunque apagues `PCRTC`, el reloj
> sigue contando con su oscilador.

### El reset que casi nunca pasa

Esto es central y el datasheet lo repite varias veces: **un reset del chip NO resetea el RTC**. Los
valores de reset que vas a ver en las tablas (la columna dice "NC", *Not Changed*) solo aplican a un
**power-up del bloque RTC**, que ocurre únicamente cuando estuvieron ausentes `VDD(REG)(3V3)` **y**
`VBAT`, y después vuelve alguna de las dos. En cualquier otro reset (botón de reset, watchdog,
power-on del micro con la pila puesta), los contadores del RTC **quedan como estaban**.

Por eso, después de un reset normal, **no debés reinicializar la hora a ciegas**: si el RTC ya venía
contando, la pisarías. El patrón correcto es: arrancá, fijate si la fecha que tenés guardada es
plausible (o usá un *magic number* en un `GPREG`), y solo seteá la hora si hace falta.

## Cómo funciona por dentro

El oscilador de 32.768 kHz alimenta un **divisor** que produce un **tick de 1 Hz**. Ese 1 Hz es el
único reloj que el RTC usa para contar tiempo. (El número 32768 = 2^15 no es casual: dividiendo por
potencias de dos llegás exacto a 1 Hz.)

Con ese segundo, el hardware incrementa una **cadena de contadores** con acarreo automático:

```
1 Hz → SEC → MIN → HOUR → DOM (día del mes) → MONTH → YEAR
                    ↘ DOW (día de semana)
                    ↘ DOY (día del año)
```

Cada contador "habilita" al siguiente: cuando `SEC` pasa de 59 a 0, incrementa `MIN`; cuando `HOUR`
pasa de 23 a 0, incrementa `DOM`, `DOW` y `DOY` a la vez, etc. `MONTH` y `DOY` controlan cuándo
`DOM` vuelve a 1 (acá entra el largo del mes y el año bisiesto).

Hay tres bloques de registros que conviene tener en la cabeza:

- **Time counters** (`SEC`…`YEAR`): la hora y fecha actuales. Se leen y escriben.
- **Alarm registers** (`ALSEC`…`ALYEAR`): un valor objetivo. Cuando la hora coincide con la alarma
  (según una máscara), dispara una interrupción.
- **Consolidated Time Registers** (`CTIME0/1/2`): una vista de solo lectura que empaqueta varios
  contadores en pocas palabras, para leerlos **coherentes** (ya vamos a ver por qué importa).

### El detalle de los rangos válidos

Los contadores **no se calculan, se incrementan**. El hardware no valida lo que escribís: si ponés un
`DOM = 31` en febrero, el RTC lo acepta y el calendario queda inconsistente hasta el próximo acarreo.
Sos vos quien tiene que escribir valores con sentido. Estos son los rangos:

| Contador | Bits | Mínimo | Máximo | Notas |
|----------|------|--------|--------|-------|
| `SEC`    | 6 | 0 | 59 | |
| `MIN`    | 6 | 0 | 59 | |
| `HOUR`   | 5 | 0 | 23 | formato 24 h, no hay AM/PM |
| `DOM`    | 5 | 1 | 28/29/30/31 | depende del mes y del bisiesto |
| `DOW`    | 3 | 0 | 6 | día de semana; **vos** decidís si 0 = domingo o lunes |
| `DOY`    | 9 | 1 | 365 / 366 | día del año |
| `MONTH`  | 4 | 1 | 12 | |
| `YEAR`   | 12 | 0 | 4095 | |

Cuidado con dos cosas:

1. **`DOW` y `DOY` no se derivan solos del resto.** El RTC los lleva como contadores independientes.
   Si seteás `DOM/MONTH/YEAR` pero te olvidás de `DOW`, el día de semana va a estar mal hasta que el
   tiempo lo "alcance". Si te importan, calculalos vos al setear la fecha.
2. **El año bisiesto es ingenuo.** El RTC mira solo los **dos bits bajos** de `YEAR`: si son cero
   (año divisible por 4), lo considera bisiesto. Eso es correcto de 1901 a 2099, pero **falla en
   2100**, que no es bisiesto (regla de los siglos) y el RTC lo va a tratar como tal. Para los
   proyectos de la cursada da igual, pero es bueno saber que el hardware miente ahí.

## A nivel registro

Veamos el `LPC_RTC_TypeDef` y cada registro con su rol exacto.

### Registros misceláneos (control e interrupciones)

| Registro | Bits usados | Para qué |
|----------|-------------|----------|
| `CCR`  | `CLKEN` (0), `CTCRST` (1), `CCALEN` (4) | Control del reloj |
| `ILR`  | `RTCCIF` (0), `RTCALF` (1) | Flags de interrupción; se limpian escribiendo 1 |
| `CIIR` | 1 bit por contador (`IMSEC`…`IMYEAR`) | Habilita interrupción **por incremento** de ese campo |
| `AMR`  | 1 bit por contador (`AMRSEC`…`AMRYEAR`) | **Máscara** de alarma; 1 = ignorar ese campo |
| `RTC_AUX` | `RTC_OSCF` (4) | Flag de falla del oscilador |
| `RTC_AUXEN` | `RTC_OSCFEN` (4) | Habilita interrupción por falla del oscilador |

**`CCR` (Clock Control Register)** es el corazón. Sus tres bits:

- **`CLKEN` (bit 0):** 1 = los contadores cuentan; 0 = congelados (así los inicializás "en frío").
- **`CTCRST` (bit 1):** 1 = resetea el **divisor** que genera el 1 Hz y lo mantiene en reset mientras
  esté en 1. Es la forma de arrancar el conteo desde un borde de segundo limpio, sin un offset
  fraccionario heredado. El patrón es: poner `CTCRST=1`, después `CTCRST=0` para soltar el divisor.
- **`CCALEN` (bit 4):** habilita el contador de **calibración**. **Ojo con la lógica invertida:**
  `CCALEN = 0` lo **habilita** y deja contar; `CCALEN = 1` lo **deshabilita** y lo resetea a cero.
  Esto confunde a todo el mundo y explica por qué el driver, en `RTC_CalibCounterCmd(..., ENABLE)`,
  escribe un **cero** en ese bit (lo vemos más abajo).

Los bits 3:2 de `CCR` son de test interno y **tienen que quedar en 0** para operación normal.

**`ILR` (Interrupt Location Register):** te dice **quién** generó la interrupción (`RTCCIF` =
incremento, `RTCALF` = alarma) y es **write-1-to-clear**. El idiom clásico es leerlo y escribirlo de
vuelta tal cual: limpiás exactamente los flags que viste seteados, sin pisar uno que llegó entre la
lectura y la escritura.

**`CIIR` vs `AMR`: dos fuentes de interrupción distintas, no las mezcles:**

- `CIIR` genera una interrupción **cada vez que un campo se incrementa**. ¿Querés un tick cada
  segundo? `CIIR = IMSEC`. ¿Cada minuto? `IMMIN`. Sirve como base de tiempo periódica de bajísima
  frecuencia. Es **periódica**.
- `AMR` es la **máscara de alarma**: define un instante específico. La lógica es **negativa**: el bit
  en 1 significa "**no** compares este campo". Para una alarma "a las 07:30:00 exactas" desenmascarás
  `SEC`, `MIN`, `HOUR` (bits en 0) y enmascarás el resto (bits en 1). La interrupción salta una sola
  vez, **en la transición de no-coincide a coincide**. Si ponés **todos** los bits de `AMR` en 1, la
  alarma queda deshabilitada (por eso `RTC_Init` deja `AMR = 0xFF`).

### Detección de falla de oscilador (`RTC_AUX` / `RTC_AUXEN`)

`RTC_OSCF` (bit 4 de `RTC_AUX`) se pone en 1 si el oscilador del RTC **deja de oscilar**, y también
**al primer encendido** del RTC. Por eso su valor de reset es 0x10: un `RTC_OSCF` seteado recién
arrancado es la forma del hardware de avisarte "estuve sin energía, mi hora no es confiable". Es
*write-1-to-clear*. Si habilitás `RTC_OSCFEN` (bit 4 de `RTC_AUXEN`), esa falla genera la
interrupción `RTC_IRQn`.

Este es el mecanismo correcto para detectar "se cortó la pila": leés `RTC_OSCF` al arranque; si está
en 1, sabés que perdiste la hora y tenés que pedirle al usuario (o a una fuente externa) que la fije.

### Lectura coherente: por qué existe `CTIME`

Imaginá que leés a las **00:59:59** y querés la hora completa. Leés `HOUR` (0), después `MIN` (59),
después `SEC`… pero entre medio el reloj pasó a **01:00:00**. Ahora `SEC` te da 0 y armaste
**00:59:00**, una hora que nunca existió. Es la clásica **condición de carrera de medianoche** (y
ocurre en cualquier acarreo, no solo a medianoche).

La solución del hardware son los **Consolidated Time Registers**, de solo lectura, que capturan
varios campos en una **sola lectura atómica**:

- **`CTIME0`**: segundos, minutos, horas, día de semana.
- **`CTIME1`**: día del mes, mes, año.
- **`CTIME2`**: día del año.

Leyendo `CTIME0` con un único acceso, segundos/minutos/horas son **mutuamente consistentes**: no hay
ventana para que cambien entre medio. Cada campo está empaquetado en su lugar y lo extraés con
máscara y shift. Las máscaras (de `lpc17xx_rtc.h`):

```c
// CTIME0
#define RTC_CTIME0_SECONDS_MASK   (0x3F)        // bits 5:0
#define RTC_CTIME0_MINUTES_MASK   (0x3F00)      // bits 13:8
#define RTC_CTIME0_HOURS_MASK     (0x1F0000)    // bits 20:16
#define RTC_CTIME0_DOW_MASK       (0x7000000)   // bits 26:24
// CTIME1
#define RTC_CTIME1_DOM_MASK       (0x1F)        // bits 4:0
#define RTC_CTIME1_MONTH_MASK     (0xF00)       // bits 11:8
#define RTC_CTIME1_YEAR_MASK      (0xFFF0000)   // bits 27:16
// CTIME2
#define RTC_CTIME2_DOY_MASK       (0xFFF)       // bits 11:0
```

Ejemplo de extracción a mano:

```c
uint32_t c0 = LPC_RTC->CTIME0;       // una sola lectura atómica
uint8_t  seg  = (c0 & RTC_CTIME0_SECONDS_MASK);
uint8_t  min  = (c0 & RTC_CTIME0_MINUTES_MASK) >> 8;
uint8_t  hora = (c0 & RTC_CTIME0_HOURS_MASK)   >> 16;
uint8_t  dow  = (c0 & RTC_CTIME0_DOW_MASK)     >> 24;
```

> Importante: `CTIME` es **solo lectura**. Para **escribir** la hora siempre usás los time counters
> (`SEC`…`YEAR`).

### Inicialización y seteo a registro

```c
#include "LPC17xx.h"

void rtc_init_y_set(void)
{
    LPC_SC->PCONP |= (1u << 9);          // PCRTC: habilita acceso a registros (en reset ya está en 1)

    // Congelar para inicializar en frío y resetear el divisor de 1 Hz
    LPC_RTC->CCR = (1u << 1);            // CTCRST = 1 (CLKEN = 0): divisor en reset, contadores parados

    // Cargar fecha/hora inicial: 24/06/2026, 10:30:00
    LPC_RTC->SEC   = 0;
    LPC_RTC->MIN   = 30;
    LPC_RTC->HOUR  = 10;
    LPC_RTC->DOM   = 24;
    LPC_RTC->DOW   = 3;                  // miércoles (con tu convención 0=domingo)
    LPC_RTC->MONTH = 6;
    LPC_RTC->YEAR  = 2026;

    // Arrancar: CLKEN = 1, CTCRST = 0 (suelta el divisor, empieza a contar desde un segundo limpio)
    LPC_RTC->CCR = (1u << 0);
}

void rtc_leer_coherente(uint8_t *h, uint8_t *m, uint8_t *s)
{
    uint32_t c0 = LPC_RTC->CTIME0;       // lectura atómica, sin race de acarreo
    *s = (c0 & RTC_CTIME0_SECONDS_MASK);
    *m = (c0 & RTC_CTIME0_MINUTES_MASK) >> 8;
    *h = (c0 & RTC_CTIME0_HOURS_MASK)   >> 16;
}
```

Fijate que escribir directamente `SEC`/`MIN`/`HOUR` por separado al **leer** es propenso a la race;
por eso la lectura usa `CTIME0`. Para escribir no hay problema: el reloj está congelado con `CTCRST`.

> **Nota sobre el clock de acceso.** Los registros del RTC se acceden a `CCLK/8`. No tiene impacto en
> el conteo (que es 1 Hz desde el oscilador propio), pero explica que el RTC sea un periférico
> "lento" de configurar comparado con el bus principal. No hace falta que hagas nada al respecto.

### La interrupción `RTC_IRQn`

Hay **un solo vector** para el RTC (`RTC_IRQn`), compartido entre incremento, alarma y falla de
oscilador. En la rutina de atención tenés que **distinguir la fuente** leyendo `ILR` (y `RTC_AUX` si
usás la detección de oscilador) y **limpiar el flag** que atendiste, o la interrupción se redispara
para siempre:

```c
void RTC_IRQHandler(void)
{
    if (LPC_RTC->ILR & RTC_IRL_RTCCIF) {     // incremento (CIIR)
        // ... tu tick periódico ...
        LPC_RTC->ILR = RTC_IRL_RTCCIF;       // write-1-to-clear
    }
    if (LPC_RTC->ILR & RTC_IRL_RTCALF) {     // alarma (AMR)
        // ... tu evento de alarma ...
        LPC_RTC->ILR = RTC_IRL_RTCALF;       // write-1-to-clear
    }
}
```

Acordate de habilitar el vector en el NVIC (`NVIC_EnableIRQ(RTC_IRQn)`); habilitar `CIIR`/`AMR` sin
el NVIC no te llama a la ISR.

## Calibración: corregir la deriva del cristal

Ningún cristal de 32.768 kHz es perfecto. Una desviación de pocas ppm parece nada, pero a lo largo de
un día son **segundos**, y a lo largo de un mes, minutos. El RTC trae un mecanismo para **compensar
esa deriva por software**, sin tener que trimear el cristal con capacitores.

La idea: hay un **calibration counter** que cuenta a 1 Hz hasta un valor `CALVAL` que vos cargás en el
registro `CALIBRATION`. Cuando llega a `CALVAL`, ocurre un **calibration match** y el RTC aplica una
corrección de **un segundo**, en una de dos direcciones (`CALDIR`):

- **Forward (`CALDIR = 0`):** en el match, los contadores **saltan de a 2** en vez de 1. Adelanta el
  reloj. Lo usás si tu cristal va **lento**.
- **Backward (`CALDIR = 1`):** en el match, los contadores **se quedan quietos un segundo** (no
  incrementan). Atrasa el reloj. Lo usás si tu cristal va **rápido**.

El registro `CALIBRATION`:

```c
#define RTC_CALIBRATION_CALVAL_MASK   (0x1FFFF)   // bits 16:0: cuenta hasta acá
#define RTC_CALIBRATION_LIBDIR        (1<<17)     // CALDIR: dirección
```

`CALVAL` máximo es 131072 (~36.4 horas). Si `CALVAL = 0`, la calibración queda **deshabilitada**.
La resolución que conseguís es de hasta **1 seg/día**, bastante fina.

**Cómo elegir `CALVAL`:** medís cuánto deriva tu reloj (por ejemplo observando la frecuencia real del
oscilador con la función CLKOUT, sin perturbarlo) y calculás cuántos segundos pasan antes de que el
reloj se desvíe **un segundo entero**. Ese número es `CALVAL`. Ejemplo: si tu reloj atrasa 1 segundo
cada 6 horas, ponés `CALVAL = 6*3600 = 21600` y `CALDIR = 0` (forward) para reponer ese segundo.

Detalles finos que el hardware maneja por vos: en backward, si la alarma cae en el mismo ciclo que el
match, la interrupción de alarma se retrasa un ciclo para no disparar dos veces; en forward, el bit
bajo de `ALSEC` se fuerza a 1 para que no te "saltees" una alarma al brincar un segundo.

## Backup registers: 20 bytes que sobreviven al apagón

`GPREG0` a `GPREG4` son **cinco registros de 32 bits** (20 bytes en total) de **propósito general**.
No tienen nada que ver con el tiempo: son RAM no volátil "gratis" que vive en el dominio del RTC, así
que **conservan su valor mientras haya `VDD(REG)(3V3)` o `VBAT`**, y **no se borran con un reset del
chip**.

¿Para qué sirven? Para guardar estado crítico que tiene que sobrevivir a un apagón o reset:

- Un *magic number* para saber si la hora ya fue configurada alguna vez (ej.: si `GPREG0 != 0xCAFE`,
  asumís primer arranque y pedís fecha).
- Contadores de eventos, último estado de una máquina de estados, flags de "reiniciá tal cosa".
- En combinación con deep power-down: información que el firmware necesita recuperar al despertar.

Acceso directo: `LPC_RTC->GPREG0 = valor;` y `valor = LPC_RTC->GPREG2;`. No hace falta secuencia
especial.

## Con el driver CMSIS

`lpc17xx_rtc` trabaja sobre una estructura de tiempo (`RTC_TIME_Type`, con campos `SEC`, `MIN`,
`HOUR`, `DOM`, `DOW`, `DOY`, `MONTH`, `YEAR`) y un enum de "tipo de campo" (`RTC_TIMETYPE_SECOND`,
`..._MINUTE`, `..._HOUR`, `..._DAYOFWEEK`, `..._DAYOFMONTH`, `..._DAYOFYEAR`, `..._MONTH`,
`..._YEAR`). Oculta las máscaras y los flags:

```c
#include "lpc17xx_rtc.h"

RTC_TIME_Type t;

void rtc_setup(void)
{
    RTC_Init(LPC_RTC);                        // PCRTC on; ILR=0, CCR=0, CIIR=0, AMR=0xFF, CALIBRATION=0
    RTC_ResetClockTickCounter(LPC_RTC);       // pulso de CTCRST: arranca el divisor desde cero

    t.YEAR = 2026; t.MONTH = 6;  t.DOM = 24;  t.DOW = 3;
    t.HOUR = 10;   t.MIN   = 30; t.SEC = 0;   t.DOY = 175;
    RTC_SetFullTime(LPC_RTC, &t);

    RTC_Cmd(LPC_RTC, ENABLE);                 // CLKEN = 1: arrancar
}

void rtc_imprimir(void)
{
    RTC_GetFullTime(LPC_RTC, &t);             // lee con las máscaras correctas
    // t.HOUR, t.MIN, t.SEC, t.DOM, t.MONTH, t.YEAR ya están actualizados
}
```

Funciones clave del driver:

- `RTC_Init` / `RTC_DeInit`: encienden/apagan `PCRTC` y dejan los registros en un estado conocido.
- `RTC_Cmd(LPC_RTC, ENABLE/DISABLE)`: `CLKEN`, arranca o para los contadores.
- `RTC_ResetClockTickCounter`: da el pulso de `CTCRST` para arrancar desde un segundo limpio.
- `RTC_SetTime` / `RTC_GetTime`: un campo individual.
- `RTC_SetFullTime` / `RTC_GetFullTime`: toda la estructura de una.
- `RTC_SetAlarmTime` / `RTC_GetAlarmTime` / `RTC_SetFullAlarmTime` / `RTC_GetFullAlarmTime`: alarmas.
- `RTC_CntIncrIntConfig(LPC_RTC, RTC_TIMETYPE_x, ENABLE)`: habilita interrupción por incremento de
  ese campo (escribe `CIIR`).
- `RTC_AlarmIntConfig(LPC_RTC, RTC_TIMETYPE_x, ENABLE)`: **desenmascara** ese campo en `AMR` (recordá
  la lógica negativa: el driver pone el bit en 0 para "comparar este campo").
- `RTC_GetIntPending` / `RTC_ClearIntPending`: leen y limpian `ILR`.
- `RTC_CalibConfig` / `RTC_CalibCounterCmd`: calibración.
- `RTC_WriteGPREG` / `RTC_ReadGPREG`: backup registers por número de canal (0–4).

Para una **alarma** que dispare a los 10 segundos:

```c
RTC_SetAlarmTime(LPC_RTC, RTC_TIMETYPE_SECOND, 10);
RTC_AlarmIntConfig(LPC_RTC, RTC_TIMETYPE_SECOND, ENABLE);   // desenmascara SEC en AMR
NVIC_EnableIRQ(RTC_IRQn);
// ... y atendés RTC_IRQHandler chequeando RTC_INT_ALARM
```

### Dos trampas del driver que conviene conocer

1. **`RTC_CalibCounterCmd` parece al revés.** `ENABLE` escribe un **0** en `CCALEN` y `DISABLE`
   escribe un **1**. No es un bug: es porque en `CCR` la lógica de `CCALEN` está invertida (0 =
   habilitado). Si lees el código del driver sin saber esto, parece que hace lo contrario de lo que
   dice.

2. **Las validaciones de rango son estrictas de menos.** `RTC_SetTime`/`RTC_SetAlarmTime` validan con
   `CHECK_PARAM` usando comparaciones **estrictas**: `TimeValue < RTC_SECOND_MAX` (con `MAX = 59`),
   `> RTC_MONTH_MIN` (con `MIN = 1`), etc. En la práctica eso **rechaza valores válidos en los
   bordes**: segundo 59, minuto 59, hora 23, mes 12, día 1 y mes 1 caen fuera. Pero ojo: `CHECK_PARAM`
   **solo actúa si compilás con `DEBUG`** (sin `DEBUG` es un no-op), así que en release pasa
   desapercibido y el valor se escribe igual. Igual, si querés setear "23:59:59" de forma robusta y
   portable, escribí los time counters directo (`LPC_RTC->HOUR = 23;`) o no compiles esas líneas con
   los asserts activos. Es un detalle del firmware de NXP, no del hardware.

## El combo estrella: RTC + bajo consumo

Acá es donde el RTC brilla. El RTC sigue contando en **Deep Power-down** (módulo 3, pág. 04), el modo
de menor consumo del LPC1769, en el que se apaga casi todo el chip (incluido el regulador principal) y
**solo queda vivo el dominio del RTC**. El consumo cae a microamperes.

El patrón clásico de un dispositivo a pila:

1. Configurás una **alarma** del RTC (o un incremento periódico) para dentro de, digamos, 5 minutos.
2. Mandás el micro a **deep power-down** (`CLKPWR_DeepPowerDown()`).
3. El micro queda consumiendo microamperes. El RTC sigue contando con su oscilador.
4. Cuando llega la alarma, la interrupción del RTC **despierta** al micro. Se reinicia el ciclo de
   wake-up del oscilador principal y el firmware retoma.
5. El micro hace su trabajo (una medición, mandar un dato) y vuelve a dormir.

Con este esquema una pila dura **meses o años**. La interrupción del RTC es de las pocas que pueden
sacar al micro de deep power-down (junto con el reset y un pin de wake-up dedicado).

Notá un detalle del ejemplo oficial `PWR/RTC_DeepPWD`: para esto se usa la **alarma**, no el
incremento periódico, y en la ISR se limpia el flag y se vuelve a configurar. Como los `GPREG`
sobreviven al power-down, son el lugar natural para dejar "dónde iba" antes de dormir, porque al
despertar de deep power-down el micro hace un arranque casi como un reset.

Los ejemplos oficiales `RTC/AlarmCntIncrInterrupt` (alarma + tick por segundo), `RTC/Calibration` y
`PWR/RTC_DeepPWD` muestran exactamente estos tres usos.

## Errores típicos

- **Reinicializar la hora en cada arranque.** Como el RTC sobrevive a resets, pisás una hora válida.
  Chequeá un *magic number* en un `GPREG` o el flag `RTC_OSCF` antes de setear.
- **Leer `SEC`/`MIN`/`HOUR` por separado** y comerte la race de acarreo. Usá `CTIME0`.
- **Olvidar el NVIC.** Habilitar `CIIR`/`AMR` no alcanza; sin `NVIC_EnableIRQ(RTC_IRQn)` no entra a la
  ISR.
- **No limpiar `ILR` en la ISR** (es write-1-to-clear): la interrupción se redispara sin fin.
- **Confundir `CIIR` con alarma.** `CIIR` es periódico (cada incremento); `AMR` es un instante.
- **Mala lógica de `AMR`.** El bit en 1 **enmascara** (ignora) el campo. Para alarmar por un campo
  hay que poner su bit en **0**.
- **Confiar en `DOW`/`DOY` automáticos.** No se derivan de la fecha; los lleva el RTC como contadores
  aparte. Calculalos al setear.
- **Esperar que el RTC tape el bug de 2100.** Su año bisiesto es solo "divisible por 4".
- **Dejar `VBAT` flotando y esperar que sobreviva un apagón.** Sin pila (o sin 3V externos) en `VBAT`,
  cortar la alimentación principal borra hora y `GPREG`.

## Lo que te llevás

- El RTC cuenta **fecha y hora** solo, desde un cristal de 32.768 kHz, manejando el calendario por
  hardware, en un **dominio de potencia aislado** que sobrevive a resets y apagones si tiene `VBAT`.
- A registro: `CCR` (`CLKEN`/`CTCRST`/`CCALEN`) para control, los contadores `SEC`…`YEAR`, alarmas con
  `AMR` (máscara negativa) + `ALxxx`, ticks periódicos con `CIIR`, e `ILR` write-1-to-clear.
- Lectura **coherente** con `CTIME0/1/2`; falla del oscilador con `RTC_AUX`/`RTC_AUXEN`; deriva del
  cristal corregible por **calibración**; 20 bytes de backup en `GPREG0–4`.
- Con driver: `RTC_SetFullTime`/`RTC_GetFullTime` sobre una `RTC_TIME_Type`, más `RTC_AlarmIntConfig`
  y `RTC_CntIncrIntConfig`.
- Su superpoder es el **bajísimo consumo**: combinado con deep power-down, es la base de cualquier
  dispositivo a batería que tiene que "despertar a una hora".

## Referencias
- Manual, Cap. 27: [`../../manual/ch27_real-time-clock-and-backup-registers.pdf`](../../manual/ch27_real-time-clock-and-backup-registers.pdf)
- Ejemplos: [`../../library/examples/RTC/`](../../library/examples/RTC/) y
  [`../../library/examples/PWR/RTC_DeepPWD/`](../../library/examples/PWR/RTC_DeepPWD/)

---

**Módulo:** [Periféricos adicionales](./README.md) · **Siguiente:** [02 - Watchdog](./02-watchdog.md)
</content>
</invoke>
