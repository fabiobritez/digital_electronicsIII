# Capture, counter y match externo: medir y generar señales

Las dos primeras páginas usan el timer para **generar tiempo** (match). Esta es la otra mitad: el timer
mirando el mundo externo. Tres mecanismos, los tres por hardware:

- **Capture** (`CCR`, `CR0`/`CR1`): copiar el `TC` en el instante exacto de un flanco en un pin. Mide
  ancho de pulso, período, frecuencia, y hace *timestamping* de eventos.
- **Counter mode** (`CTCR`): que el `TC` avance con flancos de un pin externo en vez de con `PCLK`.
  Cuenta eventos (vueltas, dientes, pulsos) en vez de tiempo.
- **External match** (`EMR`, pines `MATn.x`): cambiar un pin físico al hacer match, sin CPU. Genera
  señales (incluido un PWM rudimentario) enteramente por hardware.

## Capture: la foto del contador

La idea es simple y potente. Configurás un pin `CAPn.x` y le decís al timer: "cuando veas un flanco
acá, copiá el valor actual del `TC` en `CRn` y, si querés, interrumpime". El hardware lo hace **en el
mismo ciclo del flanco**, sin latencia de software. Por eso captura es mucho más preciso que medir con
GPIO + interrupción + leer un timer a mano: ahí la latencia de la ISR contamina la medición; con
capture, el timestamp ya quedó congelado antes de que tu código se entere.

En el LPC1769 hay **dos canales de capture por timer**: `CR0` (pin `CAPn.0`) y `CR1` (pin `CAPn.1`).
Ojo: aunque otros chips de la familia tienen CR2/CR3, en el LPC176x la struct solo define `CR0` y `CR1`
(el resto es `RESERVED`). Los registros `CRn` son **solo lectura**.

### CCR: Capture Control Register

3 bits por canal:

| Sufijo | Bit | Qué hace |
|--------|-----|----------|
| `CAPnRE` | `3n + 0` | capturar en **flanco de subida** (rising) del pin |
| `CAPnFE` | `3n + 1` | capturar en **flanco de bajada** (falling) |
| `CAPnI`  | `3n + 2` | **interrumpir** cuando ocurre la captura (levanta el bit `4+n` de `IR`) |

```
bit:  5    4    3  | 2    1    0
     CAP1I FE   RE | CAP0I FE   RE
```

Combinaciones:

- **Solo RE** (o solo FE): un timestamp por período → medís el **período** (restás capturas
  consecutivas) y de ahí la frecuencia.
- **RE + FE** en el mismo canal: capturás *ambos* flancos en el mismo `CRn`. Cada flanco pisa el
  anterior, así que para medir **ancho de pulso** conviene leer en cada interrupción y guardar el valor
  vos, distinguiendo qué flanco fue (leyendo el estado del pin, o asumiendo que se alternan).
- **Dos canales sobre la misma señal:** `CAP0` en RE y `CAP1` en FE (mismo pin no se puede, pero si el
  pin admite las dos funciones o cableás la señal a dos pines) te da subida y bajada en registros
  separados, sin ambigüedad. La forma "de libro" de medir ancho de pulso es esta cuando el hardware lo
  permite.

> **Límite de frecuencia (el dato que el datasheet esconde en un párrafo).** El pin de capture se
> muestrea con `PCLK`: hacen falta **dos flancos de subida de `PCLK` para reconocer un flanco** en el
> `CAP`. Por eso *la frecuencia del CAP no puede superar 1/4 del `PCLK`*, y la duración de un nivel
> alto/bajo no puede ser menor a `1/(2 × PCLK)`. A 25 MHz eso es: señal de hasta ~6.25 MHz, niveles no
> más cortos que 20 ns. Si medís algo más rápido, las cuentas dan cualquier cosa.

### Resolución de la medición y el wrap-around

La resolución de tu medida es **un tick del `TC`** = `(PR+1)/PCLK`. Para medir bien una señal lenta
querés ticks finos (más resolución) pero con cuidado del desborde: si entre dos capturas el `TC` da la
vuelta en 2³², la resta directa **igual da bien** gracias a la aritmética sin signo de 32 bits...
**siempre que haya dado a lo sumo una vuelta**. Si dio más de una, perdés la cuenta. Con tick de 1 µs el
`TC` tarda ~4294 s en dar la vuelta, así que en la práctica una sola vuelta es lo único a considerar.

```c
uint32_t delta = capN - capN_1;   // sin signo: correcto aunque el TC haya dado UNA vuelta
```

### Medir frecuencia: frecuencímetro

Capturamos flancos de subida sucesivos; la diferencia entre dos es el período en ticks. Con tick de
1 µs, `f[Hz] = 1e6 / periodo_us`.

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

volatile uint32_t periodo_us = 0;
volatile uint8_t  hay_dato   = 0;

void TIMER0_IRQHandler(void) {
    static uint32_t prev = 0;
    static uint8_t  primera = 1;
    // Ojo: las funciones "de capture" reciben el CANAL (0), no TIM_CR0_INT.
    // Internamente suman 4 para llegar al bit 4+canal de IR.
    if (TIM_GetIntCaptureStatus(LPC_TIM0, 0)) {
        TIM_ClearIntCapturePending(LPC_TIM0, 0);
        uint32_t ahora = TIM_GetCaptureValue(LPC_TIM0, 0);   // lee CR0
        if (!primera) {
            periodo_us = ahora - prev;   // resta sin signo: tolera UNA vuelta del TC
            hay_dato = 1;
        }
        primera = 0;
        prev = ahora;
    }
}

int main(void) {
    PINSEL_CFG_Type pin = { .Portnum=1, .Pinnum=26, .Funcnum=3, .Pinmode=0, .OpenDrain=0 };
    PINSEL_ConfigPin(&pin);                                  // P1.26 = CAP0.0

    TIM_TIMERCFG_Type cfg = { .PrescaleOption = TIM_PRESCALE_USVAL, .PrescaleValue = 1 };
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg);        // SIEMPRE antes que ConfigCapture:
    TIM_ConfigCapture(LPC_TIM0, &(TIM_CAPTURECFG_Type){   // Init pisa los bits 1:0 de CCR (bug)
        .CaptureChannel = 0, .RisingEdge = ENABLE, .FallingEdge = DISABLE, .IntOnCaption = ENABLE });

    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);

    while (1) {
        if (hay_dato) {
            hay_dato = 0;
            uint32_t f = (periodo_us > 0) ? (1000000u / periodo_us) : 0;  // Hz
            // ... mandar f por UART (módulo 9)
            (void)f;
        }
    }
}
```

El ejemplo de NXP (`examples/TIMER/FreqMeasure`) hace exactamente esto, con un detalle de robustez que
vale la pena copiar: **descarta las primeras N capturas** (resetea el contador en cada una) hasta que la
señal se "estabiliza", y recién entonces toma la medición buena. Es una forma barata de filtrar
transitorios de arranque o rebotes.

### Medir ancho de pulso

Capturamos subida y bajada y restamos. Si el pin admite las dos funciones, una forma limpia es subida
en `CAP0.0` y bajada en `CAP0.1` cableando la misma señal a `P1.26` y `P1.27`:

```c
// P1.26 = CAP0.0 (rising), P1.27 = CAP0.1 (falling), misma señal cableada a ambos
TIM_ConfigCapture(LPC_TIM0, &(TIM_CAPTURECFG_Type){
    .CaptureChannel=0, .RisingEdge=ENABLE,  .FallingEdge=DISABLE, .IntOnCaption=ENABLE });
TIM_ConfigCapture(LPC_TIM0, &(TIM_CAPTURECFG_Type){
    .CaptureChannel=1, .RisingEdge=DISABLE, .FallingEdge=ENABLE,  .IntOnCaption=ENABLE });

// ancho_alto = CR1 - CR0  (bajada menos subida), en ticks
```

Con un solo canal RE+FE también se puede, pero hay que alternar a mano qué flanco fue. Para un
**HC-SR04** (ultrasonido), el ancho del eco en `CAP` te da la distancia; para un **encoder/sensor de
RPM**, el período entre dientes te da la velocidad. La precisión es la del tick, fija y conocida, algo
que con polling de GPIO no tenés.

## Counter mode: contar eventos en vez de tiempo

Con `CTCR` el `TC` deja de avanzar con `PCLK` y avanza con **flancos de un pin `CAP`**. Sirve para
contar pulsos: vueltas de una rueda, dientes de un encoder simple, pulsos de un caudalímetro.

### CTCR: Count Control Register

| Bits | Campo | Valores |
|------|-------|---------|
| 1:0 | modo | `00` timer (cuenta PCLK) · `01` counter rising · `10` counter falling · `11` counter ambos flancos |
| 3:2 | pin de entrada | `00` = `CAPn.0` · `01` = `CAPn.1` (10/11 reservados) |

```c
// Contar flancos de subida en CAP0.0:
LPC_TIM0->CTCR = (1u << 0);          // bits 1:0 = 01 (rising), bits 3:2 = 00 (CAP0.0)
// ... y el TC ahora cuenta pulsos externos; lo leés con LPC_TIM0->TC
```

En teoría, con el driver el modo va en `TIM_Init(LPC_TIM0, TIM_COUNTER_RISING_MODE, &ccfg)`. **En la
práctica, en esta versión de la librería el modo counter por driver está roto:** `TIM_Init` escribe la
configuración en `CCR` en vez de `CTCR` (mismo bug que vimos en la [página 2](./02-timers-con-driver.md))
y nunca escribe los bits de modo, así que el timer queda en modo timer; peor, si elegís `CAP1` como
entrada, de yapa enciende `CAP0I` en `CCR` (interrupción de captura espuria). Para counter mode, andá
directo al registro: `TIM_Init` con `TIM_TIMER_MODE` para el encendido (`PCONP`/`PCLK`) si querés, y
después `CTCR` a mano como arriba, con `PR = 0`.

Reglas y advertencias del manual:

- **Mismo límite de frecuencia que capture:** la entrada se muestrea con `PCLK`, así que el pin no puede
  ir más rápido que `PCLK/4`. No sirve para frecuencias altísimas.
- **Si un `CAP` está en counter mode, sus 3 bits en `CCR` deben quedar en 000.** No podés usar el mismo
  pin para counter y capture al mismo tiempo. Los *otros* canales `CAP` del mismo timer sí pueden seguir
  capturando/interrumpiendo normalmente.
- En counter mode el prescaler `PR` igual existe pero rara vez se usa (lo dejás en 0): querés contar
  cada flanco, no cada `PR+1` flancos.
- Para **encoders en cuadratura** (dos canales A/B con sentido de giro) hay un periférico dedicado, el
  **QEI** (cap. 26). El counter mode del timer cuenta pulsos pero no te da dirección.

Patrón típico: contás pulsos durante una ventana de tiempo fija (que te da *otro* timer en modo match) y
de la cuenta sacás la frecuencia/RPM. Es el método de "conteo por ventana", complementario al de
"período por captura": captura es mejor a baja frecuencia (mide un solo período con resolución de tick),
conteo por ventana es mejor a alta frecuencia (acumula muchos pulsos).

## External match: generar señales por hardware

`EMR` conecta cada match a un **pin físico `MATn.x`**. Cuando `TC == MRn`, el pin puede ponerse en 0,
en 1, togglear o no hacer nada: **todo por hardware, la CPU ni se entera**. Es lo que te deja generar
PWM, ondas cuadradas o pulsos de timing exacto sin gastar ciclos ni sufrir jitter de interrupciones.

### EMR: External Match Register

- Bits **0-3** (`EM0`..`EM3`): el **estado actual** del pin de match (lectura/escritura). En toggle, es
  el bit que se invierte.
- Bits **5:4, 7:6, 9:8, 11:10** (`EMC0`..`EMC3`): el **modo** de cada canal, 2 bits:

| `EMCn` | Acción al hacer match `TC == MRn` |
|--------|-----------------------------------|
| `00` | no hacer nada |
| `01` | poner el pin en **0** (LOW) |
| `10` | poner el pin en **1** (HIGH) |
| `11` | **togglear** el pin |

Con el driver, esto va en el campo `ExtMatchOutputType` de `TIM_MATCHCFG_Type`:
`TIM_EXTMATCH_NOTHING` / `TIM_EXTMATCH_LOW` / `TIM_EXTMATCH_HIGH` / `TIM_EXTMATCH_TOGGLE`.

**Importante:** además de configurar `EMR`, tenés que **rutear el pin en PINSEL** a su función `MAT`
(función 3 en los pines típicos). Si no, el `EMR` cambia su bit interno pero no sale a ningún pin.

### Onda cuadrada de frecuencia fija (toggle)

La más simple: un solo match con `MR0R` (reset) y `EMC0 = toggle`. El pin se invierte en cada match, así
que sale una cuadrada al 50% cuya **frecuencia es la mitad de la del match**.

```c
PINSEL_CFG_Type pin = { .Portnum=1, .Pinnum=28, .Funcnum=3, .Pinmode=0, .OpenDrain=0 };
PINSEL_ConfigPin(&pin);                                  // P1.28 = MAT0.0

TIM_TIMERCFG_Type cfg = { .PrescaleOption = TIM_PRESCALE_USVAL, .PrescaleValue = 1 };
TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg);

TIM_MATCHCFG_Type m = {
    .MatchChannel = 0, .IntOnMatch = DISABLE, .ResetOnMatch = ENABLE, .StopOnMatch = DISABLE,
    .ExtMatchOutputType = TIM_EXTMATCH_TOGGLE,
    .MatchValue = 500            // toggle cada 500 µs -> cuadrada de 1 kHz al 50%
};
TIM_ConfigMatch(LPC_TIM0, &m);
TIM_Cmd(LPC_TIM0, ENABLE);
// La CPU queda libre: la señal de 1 kHz sale sola por P1.28.
```

Toggle cada 500 µs → período de 1000 µs → **1 kHz** (de nuevo el factor 2 del toggle).

### ¿Y un PWM de duty arbitrario? El límite del external match

Uno querría lo obvio: subir el pin al inicio del ciclo y bajarlo en el duty. Pero **cada canal tiene un
solo match y una sola acción por ciclo**: `EMC0` actúa sobre `MAT0.0` cuando matchea `MR0`, `EMC1`
sobre `MAT0.1` cuando matchea `MR1`: cada acción vive en *su* pin. No hay forma, por hardware puro,
de que un mismo pin `MAT` reciba "HIGH en un match y LOW en otro". (Y repartirlo en dos pines tampoco
sirve: `MAT0.0` con `EMC0 = HIGH` sube en el primer match y **queda alto para siempre**; nadie lo baja.)

Lo que sí se puede, con una ayuda mínima de la CPU: **toggle + recarga del match**. El pin queda en
`EMC0 = toggle` con `MR0R` (reset), y la ISR recarga `MR0` alternando la duración de la fase alta y la
baja. El flanco lo sigue poniendo el hardware en el instante exacto (sin jitter de ISR); la CPU solo
escribe un registro una vez por flanco.

```c
// MAT0.0 con EMC0 = toggle, MR0R (reset) y MR0I (interrupción). Ticks de 1 µs.
volatile uint32_t alto = 300, bajo = 700;   // período 1000 µs (1 kHz), duty 30%

void TIMER0_IRQHandler(void) {
    static uint8_t nivel = 1;               // el primer toggle deja el pin en 1 (EM0 arranca en 0)
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    // programar la duración de la fase que ACABA de empezar:
    TIM_UpdateMatchValue(LPC_TIM0, 0, (nivel ? alto : bajo) - 1);
    nivel ^= 1;
}

// En main (tras TIM_Init y el PINSEL de P1.28): TIM_ConfigMatch con MatchChannel=0,
// IntOnMatch=ENABLE, ResetOnMatch=ENABLE, ExtMatchOutputType=TIM_EXTMATCH_TOGGLE,
// MatchValue = bajo - 1 (la primera fase, con el pin aún en 0, es la baja).
```

Funciona mientras la latencia de la ISR sea mucho menor que la fase más corta (acá, 300 µs: sobra).
**Para PWM en serio está el periférico PWM1 dedicado** (módulo 19): comparte la arquitectura match del
timer pero agrega salida single-edge/double-edge sobre *un* pin por canal, 6 canales, y un registro de
*latch* (`LER`) para cambiar el duty de forma atómica sin glitches. El external match del timer es
perfecto para **una o dos señales sueltas de timing exacto** (cuadradas, pulsos, triggers); cuando
querés duty arbitrario por hardware puro o varios canales, PWM1.

## Cuándo cada cosa: el contraste completo

| Quiero... | Uso | Por qué no los otros |
|-----------|-----|----------------------|
| Tick del SO / scheduler | **SysTick** (mód. 6) | está en el núcleo, vector propio, no "gasta" un TIMERn |
| Evento periódico, one-shot, multi-canal | **Timer match** | 32 bits, 4 matches, 4 instancias |
| Una/dos señales de timing exacto por hardware | **Timer external match** | sin CPU, sin jitter |
| PWM multicanal con duty cómodo y sin glitch | **PWM1** (mód. 19) | latch atómico, 6 canales, un pin por canal |
| Medir período/frecuencia/ancho de una señal | **Timer capture** | timestamp por hardware, sin latencia de ISR |
| Contar pulsos externos | **Timer counter (CTCR)** | el TC avanza con el pin, no con PCLK |
| Velocidad + sentido de un encoder en cuadratura | **QEI** (mód. 26) | el counter mode no da dirección |
| Interrupción periódica pelada, sin gastar un TIMERn | **RIT** (cap. 22, ver página 1) | un contador + un compare, nada más |
| Muestrear el ADC a frecuencia fija | **Timer match → trigger ADC → DMA** | cadena sin CPU |

## Errores típicos de esta página

| Error | Corrección |
|-------|-----------|
| Configurar `EMR` y olvidar el `PINSEL` del pin `MAT` | sin rutear el pin, no sale nada afuera |
| Medir una señal más rápida que `PCLK/4` con capture/counter | está fuera de spec: subí `PCLK` o cambiá de método |
| Pasar `TIM_CR0_INT` a las funciones `...Capture...` del driver | reciben el **canal** (0 o 1): suman 4 solas para llegar al bit `4+n` de `IR` |
| `TIM_ConfigCapture` antes de `TIM_Init` | `TIM_Init` pisa los bits 1:0 de `CCR` (bug del driver): capture se configura después |
| Usar el mismo `CAP` para counter y capture | si está en counter mode, sus bits de `CCR` van en 000 |
| Olvidar que toggle da la mitad de la frecuencia | un período de señal = dos matches |
| Restar capturas asumiendo que el `TC` no da la vuelta | la resta sin signo tolera UNA vuelta; más de una, no |
| Esperar dirección de giro del counter mode | para cuadratura, QEI (cap. 26) |

---

**Anterior:** [02 - Timers con el driver](./02-timers-con-driver.md) ·
**Módulo:** [Timers](./README.md) ·
**Siguiente módulo:** [09 - UART](../09_uart/)
