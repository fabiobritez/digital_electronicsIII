# Timers con el driver CMSIS

El driver `lpc17xx_timer` envuelve los registros `PR`, `MR`, `MCR`, `TCR`, `IR`, etc. en structs y
funciones. Lo bueno: te deja pensar el prescaler **en microsegundos** en vez de calcular `PR` a mano, y
te encapsula el ritual de encendido (`PCONP` + `PCLKSEL`). Lo que vale la pena entender es **qué hace por
abajo**, porque ahí están las decisiones (y un par de trampas).

## Lo que `TIM_Init` hace de verdad

Mirando `lpc17xx_timer.c`, `TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg)`:

1. Enciende el timer en `PCONP` (`CLKPWR_ConfigPPWR`).
2. **Fuerza `PCLK_timer = CCLK/4`** (`CLKPWR_SetPCLKDiv(..., CLKPWR_PCLKSEL_CCLK_DIV_4)`). Esto es clave:
   no importa cómo tengas el `PCLKSEL`, el driver lo deja en /4. Con `CCLK = 100 MHz`, eso es 25 MHz.
3. Quiere poner `CTCR` en modo timer... pero acá hay un **bug del driver**: escribe los bits de modo
   en `CCR` en vez de `CTCR` (`TIMx->CCR &= ~TIM_CTCR_MODE_MASK;` en `lpc17xx_timer.c`). En modo timer
   el daño colateral es que **borra los bits 1:0 de `CCR`** (los flancos del capture 0). Consecuencia
   práctica: `TIM_ConfigCapture` va **después** de `TIM_Init`, nunca antes. Después resetea
   `TC`/`PC`/`PR` y da el pulso de reset por `TCR`.
4. Calcula `PR`:
   - con `TIM_PRESCALE_TICKVAL`: `PR = PrescaleValue − 1` (valor absoluto en ticks de PCLK).
   - con `TIM_PRESCALE_USVAL`: `PR = (PCLK × us / 1e6) − 1`, es decir, `PR` para que **un tick dure
     `PrescaleValue` microsegundos**. Por eso después podés expresar `MatchValue` en µs directos.
5. Limpia todas las banderas de `IR`.

Nota importante: `TIM_Init` **no** habilita el NVIC ni arranca el timer. Eso es tuyo
(`NVIC_EnableIRQ` + `TIM_Cmd(..., ENABLE)`).

## Las structs y funciones

```c
#include "lpc17xx_timer.h"

// Configuración general del timer (el prescaler)
TIM_TIMERCFG_Type cfg;
cfg.PrescaleOption = TIM_PRESCALE_USVAL;  // el prescaler se expresa en µs
cfg.PrescaleValue  = 1;                   // 1 tick = 1 µs

// Configuración de un canal de match
TIM_MATCHCFG_Type match;
match.MatchChannel        = 0;                     // MR0
match.IntOnMatch          = ENABLE;                // MR0I: interrumpir al hacer match
match.ResetOnMatch        = ENABLE;                // MR0R: y resetear el TC (periódico)
match.StopOnMatch         = DISABLE;               // MR0S
match.ExtMatchOutputType  = TIM_EXTMATCH_NOTHING;  // EMR: no tocar ningún pin MAT
match.MatchValue          = 500000;                // 500000 µs = 500 ms

TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg);  // enciende PCONP, fija PCLK/4 y aplica el prescaler
TIM_ConfigMatch(LPC_TIM0, &match);         // escribe MRx, MCR y EMR
TIM_Cmd(LPC_TIM0, ENABLE);                 // TCR bit 0: arrancar
```

| Función | Equivale a (registro) |
|---------|-----------------------|
| `TIM_Init(TIMx, modo, &cfg)` | encender `PCONP` + `PCLKSEL=/4` + fijar `PR` (modo timer o counter) |
| `TIM_ConfigStructInit(modo, &cfg)` | rellenar la struct con valores por defecto (1 µs) |
| `TIM_ConfigMatch(TIMx, &match)` | escribir `MRx`, `MCR` y `EMR` del canal |
| `TIM_UpdateMatchValue(TIMx, ch, val)` | cambiar un `MRx` en caliente (sin tocar `MCR`) |
| `TIM_SetMatchExt(TIMx, modo)` | **cuidado:** está declarada en el `.h` pero no implementada en el `.c` de esta versión → error de linker |
| `TIM_Cmd(TIMx, ENABLE/DISABLE)` | bit 0 de `TCR` |
| `TIM_ResetCounter(TIMx)` | pulso del bit 1 de `TCR` (`TC=PC=0`) |
| `TIM_ClearIntPending(TIMx, TIM_MR0_INT)` | escribir 1 en el bit del enum en `IR` (sirve para match **y** capture) |
| `TIM_ClearIntCapturePending(TIMx, canal)` | escribir 1 en el bit `4 + canal` de `IR`: recibe el **canal** (0 o 1), no `TIM_CRn_INT` |
| `TIM_GetIntStatus / GetIntCaptureStatus` | leer un bit de `IR` |
| `TIM_ConfigCapture(TIMx, &cap)` | configurar `CCR` |
| `TIM_GetCaptureValue(TIMx, ch)` | leer `CR0`/`CR1` |
| `TIM_DeInit(TIMx)` | parar y apagar en `PCONP` |

> **Cuidado con las banderas de capture.** Las funciones genéricas (`TIM_GetIntStatus`,
> `TIM_ClearIntPending`) usan el valor del enum como número de bit de `IR`: con `TIM_CR0_INT` (= 4)
> tocan el bit 4, que **es** la bandera de capture 0: funcionan bien para match y para capture. Las
> funciones "de capture" (`TIM_GetIntCaptureStatus`, `TIM_ClearIntCapturePending`), en cambio, **le
> suman 4 solas** al argumento: esperan el **número de canal** (0 o 1). Si les pasás `TIM_CR0_INT`
> terminan en el bit 8 (reservado): no leen ni limpian nada y la ISR reentra para siempre. Regla: a las
> genéricas, el enum; a las de capture, el canal. (Otro bug de esta versión, ya que estamos:
> `TIM_DeInit(LPC_TIM3)` apaga `PCTIM2` en `PCONP`, o sea que "des-inicializar" el Timer3 te mata el
> Timer2.)

### Cómo funciona `PrescaleOption = TIM_PRESCALE_USVAL`

Con `PrescaleValue = 1` el driver calcula `PR` para que **el TC avance cada 1 µs**, leyendo el `PCLK`
real del timer. Por eso `MatchValue` queda directamente en microsegundos **sin que vos sepas el PCLK**.
Es la forma cómoda de no pelearse con la frecuencia... con dos asteriscos:

- Si pedís un tick más fino que un ciclo de `PCLK` (p. ej. `PrescaleValue` tal que `PR` daría < 0), no
  se puede: la resolución mínima es `1/PCLK` (40 ns a 25 MHz).
- `MatchValue` en µs sigue siendo un entero de 32 bits; el máximo representable es ~4294 s con tick de
  1 µs. Para tiempos largos, subí el tamaño del tick (`PrescaleValue` mayor).

## El mismo LED a 1 Hz, con driver

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#define LED (1u << 22)   // P0.22

void TIMER0_IRQHandler(void) {
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);   // limpiar bandera primero
    LPC_GPIO0->FIOPIN ^= LED;                     // togglear el LED
}

int main(void) {
    TIM_TIMERCFG_Type cfg = { .PrescaleOption = TIM_PRESCALE_USVAL, .PrescaleValue = 1 };
    TIM_MATCHCFG_Type m = {
        .MatchChannel = 0, .IntOnMatch = ENABLE, .ResetOnMatch = ENABLE,
        .StopOnMatch = DISABLE, .ExtMatchOutputType = TIM_EXTMATCH_NOTHING,
        .MatchValue = 500000          // 500 ms
    };
    GPIO_SetDir(0, LED, 1);
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg);
    TIM_ConfigMatch(LPC_TIM0, &m);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
    while (1) { }
}
```

Compará con el ejemplo a registro de la página anterior: misma lógica, mismos pasos (encender,
prescaler, match, NVIC, arrancar), solo que el driver hace las cuentas del `PR` y el toggle de PCONP.

## Dos eventos con un solo timer (MR0 + MR1)

Acá se ve por qué tener 4 matches es útil. `MR0` con reset define el período; `MR1` (sin reset) marca un
instante dentro de ese período. Resultado: dos eventos a distinta fase, un solo timer, un solo vector.

```c
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {   // ¿fue MR0?
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        LPC_GPIO0->FIOPIN ^= (1u << 22);             // LED A
    }
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {   // ¿fue MR1?
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
        LPC_GPIO0->FIOPIN ^= (1u << 23);             // LED B (a otra fase)
    }
}

// en main, después de TIM_Init:
TIM_MATCHCFG_Type m0 = { .MatchChannel=0, .IntOnMatch=ENABLE, .ResetOnMatch=ENABLE,
                         .StopOnMatch=DISABLE, .ExtMatchOutputType=TIM_EXTMATCH_NOTHING,
                         .MatchValue=1000000 };   // período 1 s
TIM_MATCHCFG_Type m1 = { .MatchChannel=1, .IntOnMatch=ENABLE, .ResetOnMatch=DISABLE,
                         .StopOnMatch=DISABLE, .ExtMatchOutputType=TIM_EXTMATCH_NOTHING,
                         .MatchValue=300000 };    // a 300 ms del inicio del ciclo
TIM_ConfigMatch(LPC_TIM0, &m0);
TIM_ConfigMatch(LPC_TIM0, &m1);
```

La clave: **el reset vive solo en MR0** (el match más grande del ciclo). Si pusieras `ResetOnMatch` en
MR1, el `TC` se reiniciaría a los 300 ms y MR0 nunca llegaría a 1 s.

## El timer que dispara el ADC

Un uso muy típico que la página superficial no mencionaba: muestrear el ADC a frecuencia fija **sin
CPU**. El ADC del LPC1769 (módulo 10) puede arrancar una conversión con un **match del timer** (`MAT0.1`,
`MAT0.3`, etc., según el campo `START` del registro `ADCR`). El timer corre con `MR0R` para dar el
período de muestreo, el match arranca la conversión, y la conversión terminada puede a su vez disparar
**DMA** (módulo 11) para guardar la muestra. Resultado: un sistema de adquisición a, digamos, 10 kHz que
no toca la CPU salvo cuando el buffer está lleno. La cadena es: **Timer match → trigger ADC → DMA**.

Además, el propio timer puede pedir **DMA** directamente: el manual aclara que un match de **MR0 o MR1**
(solo esos dos: *"up to two match conditions can be used to generate timed DMA requests"*) genera un
*DMA request*, independiente de los pines `MAT`. Para usarlo hay que seleccionar el timer como fuente en
`DMAREQSEL` (sección 31.5.15) y, ojo, **escribir 1 en la bandera de `IR` antes de arrancar** para no
arrastrar un request espurio inicial: el manual dice que *"when a timer is initially set up to generate a
DMA request, the request may already be asserted"*, y que **limpiar la interrupción de match también
limpia el DMA request asociado** (*"the act of clearing an interrupt for a timer match also clears any
corresponding DMA request"*). Por eso el mismo write-1-to-clear que usás en la ISR sirve para borrar el
request fantasma antes de habilitar el DMA.

## Errores comunes

| Error | Corrección |
|-------|-----------|
| Olvidar encender `PCONP` (sobre todo TIM2/TIM3, que arrancan apagados) | `TIM_Init` lo hace; a registro, acordate vos |
| No limpiar `IR` en el handler | `TIM_ClearIntPending(...)` o `...CapturePending(...)` siempre, primero |
| Pasar `TIM_CR0_INT` a las funciones `...Capture...` | esas esperan el **canal** (0 o 1); el enum es para las genéricas |
| Llamar `TIM_ConfigCapture` antes de `TIM_Init` | `TIM_Init` pisa los bits 1:0 de `CCR` (bug) y limpia `IR`: capture se configura después |
| Olvidar `MR0R` (reset on match) | sin él, interrumpe una vez y después cuenta hasta 2³² |
| Calcular el período con el `PCLK` equivocado | conocé el `PCLK` real (módulo 3) o usá `USVAL` |
| `MatchValue` sin restar 1 (a registro) | el `TC` cuenta de 0 a MR: son `MR+1` ticks |
| Confundir "período de match" con "período de la señal" al togglear | togglear da medio período: hay factor 2 |
| Poner el reset en un MR que no es el más grande del ciclo | el reset va en MR0; los demás solo interrumpen |
| Variable compartida con la ISR sin `volatile` | marcala `volatile` |
| Confundir match (genero tiempo) con capture (mido tiempo externo) | match compara `TC` con un valor fijo; capture copia `TC` ante un flanco externo |

## Ejercicios

1. Dos LEDs a frecuencias distintas con **dos canales de match** (MR0 reset + MR1) de un mismo timer.
2. Generá una señal de 1 kHz al 50% en MAT0.0 usando match externo (sin CPU en el lazo). Ver
   [página 3](./03-capture-y-medicion.md).
3. **Frecuencímetro:** medí la frecuencia de una señal en CAP0.0 y mostrala por UART. Ver
   [página 3](./03-capture-y-medicion.md).
4. Reescribí el ejercicio 1 **a registro** y compará claridad y tamaño con la versión con driver.
5. Configurá un timer para que dispare el ADC cada 1 ms (sin tocar la CPU en el lazo de muestreo).

> Material original con más ejemplos: [`_origen/04_TIMER.md`](./_origen/04_TIMER.md).

---

**Anterior:** [01 - Timers a nivel registro](./01-timers-registros.md) ·
**Siguiente:** [03 - Capture, counter y match externo](./03-capture-y-medicion.md)
