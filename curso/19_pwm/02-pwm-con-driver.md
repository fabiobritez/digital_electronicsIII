# PWM con el driver CMSIS

El driver `lpc17xx_pwm` configura el prescaler, el match, el `PCR` y el `LER` por vos. Vos pensás en
**período y duty**; el driver traduce a registros. Acá vemos la API completa, la correspondencia con
los registros de la [página anterior](./01-pwm-registros.md), un *fade* de LED, double-edge y la
interrupción por período.

## Qué hace `PWM_Init` por debajo (importante)

`PWM_Init(LPC_PWM1, PWM_MODE_TIMER, &cfg)` no es solo "encender". En el `.c` del driver hace:

- `PCONP` bit `PCPWM1` (enciende el periférico),
- **fija `PCLK_PWM1 = CCLK/4`** (con `CCLK = 100 MHz` queda **25 MHz**),
- limpia `IR, TCR, CTCR, MCR, CCR, PCR, LER`,
- carga el `PR` según el prescaler que pediste.

El prescaler tiene dos modos en `PrescaleOption`:

- `PWM_TIMER_PRESCALE_TICKVAL`: `PrescaleValue` es el valor absoluto; el driver pone `PR = valor - 1`.
- `PWM_TIMER_PRESCALE_USVAL`: `PrescaleValue` está en **microsegundos por tick**; el driver calcula el
  `PR` solo. Con `PrescaleValue = 1` y `PCLK = 25 MHz`, sale `PR = 24` (un tick = 1 us). Es el modo
  cómodo para servos.

## Inicialización y configuración (single-edge, fade de LED a 1 kHz)

```c
#include "lpc17xx_pwm.h"
#include "lpc17xx_pinsel.h"

void pwm_init(void) {
    // Pin PWM1.1 = P2.0, función 1 (el driver NO toca PINSEL: lo hacés vos)
    PINSEL_CFG_Type pin;
    pin.Portnum = 2; pin.Pinnum = 0; pin.Funcnum = 1;
    pin.Pinmode = PINSEL_PINMODE_TRISTATE; pin.OpenDrain = 0;
    PINSEL_ConfigPin(&pin);

    // Prescaler: cada tick = 1 us (modo microsegundos)
    PWM_TIMERCFG_Type cfg;
    cfg.PrescaleOption = PWM_TIMER_PRESCALE_USVAL;
    cfg.PrescaleValue  = 1;
    PWM_Init(LPC_PWM1, PWM_MODE_TIMER, &cfg);   // PCONP + PCLK=CCLK/4 + PR

    // Período: MR0 = 1000 us -> 1 kHz. Reset on match para que sea periódico.
    PWM_MATCHCFG_Type m;
    m.MatchChannel = 0; m.IntOnMatch = DISABLE; m.StopOnMatch = DISABLE; m.ResetOnMatch = ENABLE;
    PWM_ConfigMatch(LPC_PWM1, &m);                          // MCR: MR0R
    PWM_MatchUpdate(LPC_PWM1, 0, 1000, PWM_MATCH_UPDATE_NOW); // MR0 + LER

    // Canal 1: single-edge (en este canal es fijo: no hay nada que configurar), duty inicial 50%
    PWM_MatchUpdate(LPC_PWM1, 1, 500, PWM_MATCH_UPDATE_NOW);  // MR1 = 500 us -> duty 50%
    PWM_ChannelCmd(LPC_PWM1, 1, ENABLE);                     // PCR: PWMENA1 (salida del pin)

    PWM_ResetCounter(LPC_PWM1);    // TCR: pulso de reset
    PWM_CounterCmd(LPC_PWM1, ENABLE); // TCR bit 0
    PWM_Cmd(LPC_PWM1, ENABLE);     // TCR bit 3: habilita modo PWM (arranca a sacar señal)
}
```

> Ojo: **no llames `PWM_ChannelConfig` con el canal 1**. La función solo admite canales 2..6
> (`PARAM_PWM1_EDGE_MODE_CHANNEL`), y como la librería viene compilada con `DEBUG` definido
> (`lpc17xx_libcfg_default.h`), pasarle el canal 1 la deja **colgada en `check_failed()`** (un
> `while(1)`). Tampoco hace falta: PWM1.1 es single-edge fijo. `PWM_ChannelConfig` se usa solo para
> elegir single/double edge en los canales 2..6.

### Correspondencia driver ↔ registro

| Driver | Registro (página 1) |
|--------|---------------------|
| `PWM_Init(.., PWM_MODE_TIMER, ..)` | `PCONP` + `PCLKSEL0` + `PR` |
| `PWM_ConfigMatch` (canal 0, `ResetOnMatch`) | `MCR` (bit `MR0R`) |
| `PWM_ConfigMatch` (`IntOnMatch`) | `MCR` (bit `MRnI`) |
| `PWM_MatchUpdate(.., ch, val, ..)` | escribir `MRch` **y** setear `LER` (los dos pasos) |
| `PWM_ChannelConfig(.., ch, mode)` | `PCR` bit `PWMSELch` (single/double) |
| `PWM_ChannelCmd(.., ch, ENABLE)` | `PCR` bit `PWMENAch` (salida física) |
| `PWM_CounterCmd` | `TCR` bit 0 (counter enable) |
| `PWM_Cmd` | `TCR` bit 3 (PWM enable) |
| `PWM_ResetCounter` | `TCR` bit 1 (pulso de reset) |
| `PWM_GetIntStatus` / `PWM_ClearIntPending` | leer / limpiar `IR` |

> El driver te salva del error número uno: `PWM_MatchUpdate` escribe el match **y** el latch (`LER`)
> en una sola llamada. Nunca te "no pasa nada" por olvidar el `LER`.

### El detalle de `PWM_MATCH_UPDATE_NOW` contra `NEXT_RST`

`PWM_MatchUpdate` recibe un tipo de update:

- `PWM_MATCH_UPDATE_NEXT_RST`: escribe el match y setea el `LER`. El valor entra **al próximo fin de
  período** (comportamiento normal, sin glitch). Es lo que querés casi siempre.
- `PWM_MATCH_UPDATE_NOW`: además del `LER`, **pulsa un reset del contador** para que el cambio se vea
  *ya*. Esto reinicia el período en el momento: cómodo en configuración inicial, pero si lo usás en
  caliente cada vez que cambiás el duty, **estás reiniciando el período continuamente** y podés
  introducir el mismo jitter que el latch venía a evitar. Para *fade* fino, preferí `NEXT_RST`.

## Variar el duty: fade de un LED

Lo lindo del PWM es cambiar el duty en caliente. Para un LED que "respira", variás `MR1` en un bucle:

```c
int main(void) {
    pwm_init();                       // período 1000 us (1 kHz)
    int duty = 0, paso = 10;
    while (1) {
        PWM_MatchUpdate(LPC_PWM1, 1, duty, PWM_MATCH_UPDATE_NEXT_RST); // nuevo duty, sin glitch
        duty += paso;
        if (duty >= 1000 || duty <= 0) paso = -paso;   // ir y volver (respiración)
        for (volatile int i = 0; i < 30000; i++);      // demora entre pasos
    }
}
```

`duty` va de 0 (apagado) a 1000 (= `MR0`, brillo full) y vuelve. Cambiá el pin a uno con LED de tu
placa y lo ves. Como el ojo responde de forma logarítmica, una rampa lineal se ve "rápida arriba,
lenta abajo"; si querés un respirado más natural, hacé los pasos no lineales (cuadráticos).

## Double-edge con el driver: pulso con fase

Para mostrar la diferencia, configuremos el canal 2 en double-edge. Acá el pulso lo definen **dos**
matches: `MR1` (sube) y `MR2` (baja). Mover ambos desplaza el pulso *sin* cambiar su ancho (control
de fase) o cambia el ancho según cómo los muevas.

```c
void pwm2_double_edge(void) {
    // ... PWM_Init y MR0 (período) como antes, MR0 = 1000 ...

    PWM_ChannelConfig(LPC_PWM1, 2, PWM_CHANNEL_DUAL_EDGE);  // PCR: PWMSEL2 = 1

    // Canal 2 double-edge: sube en MR1, baja en MR2 (Table 444 del manual)
    PWM_MatchUpdate(LPC_PWM1, 1, 300, PWM_MATCH_UPDATE_NOW); // flanco de subida a t=300 us
    PWM_MatchUpdate(LPC_PWM1, 2, 700, PWM_MATCH_UPDATE_NOW); // flanco de bajada a t=700 us
    // MR1 y MR2 NO deben tener ResetOnMatch (solo MR0 resetea el período):
    PWM_MATCHCFG_Type m = {0};
    m.MatchChannel = 1; PWM_ConfigMatch(LPC_PWM1, &m);
    m.MatchChannel = 2; PWM_ConfigMatch(LPC_PWM1, &m);

    PWM_ChannelCmd(LPC_PWM1, 2, ENABLE);                    // PWMENA2
    // ... ResetCounter + CounterCmd + Cmd ...
}
```

Resultado: el pulso del canal 2 está **alto solo entre 300 us y 700 us** del período de 1000 us, no
anclado al inicio. Para mover la fase manteniendo el ancho (400 us), desplazás ambos a la vez con un
solo latch:

```c
LPC_PWM1->MR1 = 100; LPC_PWM1->MR2 = 500;
LPC_PWM1->LER = (1u << 1) | (1u << 2);  // pulso 100..500 us: misma ancho, fase distinta
```

> Recordá: para double-edge usá canales **2, 4, 6** (los pares). El canal 1 no puede; los canales 3 y
> 5 funcionan pero "desperdician" pares de match. Ver tabla en la [página 1](./01-pwm-registros.md).

## Interrupción por match: cargar el próximo valor en cada período

Para cosas como generar una forma de onda (senoidal por PWM) o un control en lazo cerrado a frecuencia
fija, querés ejecutar código **una vez por período**. Habilitás interrupt-on-match en `MR0`:

```c
void pwm_int_init(void) {
    // ... pwm_init() con MR0 = período ...
    PWM_MATCHCFG_Type m;
    m.MatchChannel = 0; m.ResetOnMatch = ENABLE; m.StopOnMatch = DISABLE;
    m.IntOnMatch = ENABLE;                       // MCR: MR0I + MR0R
    PWM_ConfigMatch(LPC_PWM1, &m);

    NVIC_SetPriority(PWM1_IRQn, 1);
    NVIC_EnableIRQ(PWM1_IRQn);
}

extern const uint16_t tabla[256];   // tabla de duty (p. ej. una senoidal)
volatile uint32_t idx = 0;

void PWM1_IRQHandler(void) {
    if (PWM_GetIntStatus(LPC_PWM1, PWM_INTSTAT_MR0)) {
        PWM_ClearIntPending(LPC_PWM1, PWM_INTSTAT_MR0);  // limpiar IR (escribir 1)
        idx = (idx + 1) & 0xFF;
        PWM_MatchUpdate(LPC_PWM1, 1, tabla[idx], PWM_MATCH_UPDATE_NEXT_RST); // próximo duty
    }
}
```

El handler se llama una vez por período. **Limpiá siempre el flag de `IR`** (escribiendo un 1, que es
lo que hace `PWM_ClearIntPending`) o la interrupción se redispara para siempre. Notá el detalle del
[mapeo de bits](./01-pwm-registros.md): el driver lo abstrae con `PWM_INTSTAT_MRn`, así no tenés que
acordarte de que MR4..6 viven en los bits 8..10. (Hay una rareza en el header CMSIS: las constantes
`PWM_INTSTAT_MR5` y `PWM_INTSTAT_MR6` están definidas cruzadas respecto del nombre; para MR0/MR1/MR4
no te afecta, pero si usás interrupción en MR5/MR6 verificá contra el `IR` real.)

## Aplicaciones por canal

Como hay 6 canales con el mismo período, manejás varias cosas a la vez:

| Canal | Pin | Uso típico |
|-------|-----|-----------|
| PWM1.1 | P2.0 | LED / motor 1 |
| PWM1.2 | P2.1 | LED / motor 2 |
| PWM1.3–1.6 | P2.2–P2.5 | más LEDs / un LED RGB (3 canales) |

Un **LED RGB** se controla con 3 canales (uno por color): variando los tres duty hacés cualquier
color, todos a la misma frecuencia (sin batimientos visibles entre colores).

## Errores comunes

| Error | Síntoma | Corrección |
|-------|---------|-----------|
| No habilitar **PWM mode** (`TCR` bit 3 / `PWM_Cmd`) | el pin no saca nada aunque "todo está configurado" | llamá `PWM_Cmd(LPC_PWM1, ENABLE)` |
| Olvidar el `LER` (a registro) | cambiás `MRn` y "no pasa nada" | usá `PWM_MatchUpdate`, que latchea solo |
| `MR0R` no seteado | sale un flanco y nunca repite | `ResetOnMatch = ENABLE` en el canal 0 |
| `MR0` con valor malo (0 o no cargado antes de PWM Enable) | nada toma efecto (el latch nunca dispara) | cargá `MR0` **antes** de `PWM_Cmd` |
| Confundir `PWMENA` con `PWMSEL` | el modo cambia pero el pin sigue mudo | `PWM_ChannelCmd` (ENA) saca señal; `ChannelConfig` (SEL) solo elige edge |
| `MRn > MR0` | el canal queda 100% (siempre alto) | duty = `MRn/MR0`, mantené `MRn <= MR0` |
| Frecuencia muy baja en un LED | parpadeo visible | usá > 100 Hz |
| No configurar PINSEL del pin | el pin sigue como GPIO | `PINSEL_ConfigPin` con `Funcnum = 1` (el driver no lo hace) |
| `PWM_ChannelConfig` con canal 1 | el programa queda colgado en `check_failed()` (la lib viene con `DEBUG`) | canal 1 es single-edge fijo; `ChannelConfig` solo para canales 2..6 |
| No limpiar `IR` en la ISR | la interrupción se redispara sin fin | `PWM_ClearIntPending` al entrar |

## Ejercicios
1. **Fade** de un LED (el de arriba); probá distintas velocidades y frecuencias, y una rampa cuadrática.
2. **Servo:** barré un servo de 0 a 180 grados y de vuelta, suavemente.
3. **LED RGB:** con 3 canales, hacé una transición de colores (rojo → verde → azul).
4. **Control por ADC:** usá un potenciómetro (módulo 10) para fijar el brillo de un LED por PWM.
5. **Senoidal por PWM:** con la interrupción de `MR0` y una tabla de 256 valores, modulá el duty para
   aproximar una senoidal; filtrala con un RC y miralo en el osciloscopio.
6. **Double-edge:** generá en el canal 2 un pulso de ancho fijo y desplazá su fase con `MR1`/`MR2`.

---

**Anterior:** [01 - PWM a nivel registro](./01-pwm-registros.md) ·
**Siguiente módulo:** [20 - Hardware y placa](../20_hardware_y_placa/)
