# SysTick a nivel registro

## Lo primero que tenés que entender: SysTick NO es un periférico de NXP

Hasta ahora, cada periférico que viste (GPIO, y los que vienen) es un bloque que NXP le agregó
alrededor del núcleo. Por eso todos arrancan apagados y hay que prenderlos en `PCONP` y elegirles el
reloj en `PCLKSEL`. **SysTick es distinto: es parte del propio núcleo Cortex-M3.** Lo diseñó ARM, no
NXP, y está *dentro* del procesador.

Consecuencias prácticas de esto, que conviene grabar:

- **No tiene bit en `PCONP` ni en `PCLKSEL`.** No lo prendés con energía de periférico ni le ponés
  divisor por `PCLKSEL`. Ya está ahí.
- **Es portable.** El mismo código de SysTick anda igual en un LPC1769, en un STM32, en un Kinetis o
  en cualquier Cortex-M3/M4/M7. Sus registros viven siempre en la misma dirección (`0xE000E010`) y se
  llaman igual. Esto es muy distinto de un `LPC_TIM0`, que es propio de NXP.
- **Está documentado en el lado ARM del manual** (Capítulo 34, *Appendix: Cortex-M3 User Guide*,
  sección 34.4.4). El Capítulo 23 de NXP existe, pero es corto y te remite al 34 para el detalle.

Por eso este módulo viene antes que el resto de los timers: es el más simple y el más universal.

## Qué es: un contador descendente de 24 bits

SysTick es muy simple:

1. Cargás un valor inicial en el registro de **recarga** (`LOAD`).
2. El contador **cuenta hacia abajo** desde ese valor, un paso por cada pulso de su reloj.
3. Cuando llega a **0**, levanta una bandera (`COUNTFLAG`) y, si está habilitada, genera una
   **excepción** (el `SysTick_Handler`).
4. Se **recarga solo** al valor de `LOAD` en el siguiente flanco y vuelve a empezar. No hay que
   rearmarlo: es automático, "libre" (*free-running*).

```
LOAD = 999 ─┐
            ▼
   999 → 998 → ... → 1 → 0 → (COUNTFLAG=1, ¡excepción!) → recarga a 999 → 999 → ...
```

### ¿Por qué 24 bits y no 32?

Es una decisión de ARM: el contador es de **24 bits**, así que el valor máximo de recarga es
2²⁴ − 1 = **16.777.215** (`0x00FFFFFF`). Los bits [31:24] de `LOAD` y de `VAL` están reservados:
escribir ahí no hace nada. Esto importa porque fija el **período máximo** de un solo ciclo, y vas a
chocar con ese límite seguido (lo vemos abajo).

## Los registros (en `0xE000E010`)

| Registro | Dirección | Acceso | Reset | Qué hace |
|----------|-----------|--------|-------|----------|
| `CTRL`  | `0xE000E010` | R/W | `0x00000004` | Control y estado |
| `LOAD`  | `0xE000E014` | R/W | `0x00000000` | Valor de recarga (24 bits) |
| `VAL`   | `0xE000E018` | R/W | `0x00000000` | Valor actual del contador |
| `CALIB` | `0xE000E01C` | R   | `0x000F423F` | Calibración (solo lectura) |

> Los nombres que usa NXP en su capítulo (`STCTRL`, `STRELOAD`, `STCURR`, `STCALIB`) son los mismos
> registros. CMSIS los expone como `SysTick->CTRL`, `SysTick->LOAD`, `SysTick->VAL`, `SysTick->CALIB`
> (struct `SysTick_Type` en `core_cm3.h`). Usá siempre los nombres de CMSIS.

### `CTRL`: control y estado (campo por campo)

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 0  | `ENABLE`    | 1 = el contador anda. 0 = detenido (conserva `VAL`). |
| 1  | `TICKINT`   | 1 = al llegar a 0 genera la excepción `SysTick_Handler`. 0 = solo levanta `COUNTFLAG`, sin interrumpir. |
| 2  | `CLKSOURCE` | Fuente de reloj. **1 = reloj del procesador (CCLK). 0 = reloj externo de referencia (STCLK).** |
| 16 | `COUNTFLAG` | Vale 1 si el contador llegó a 0 desde la última vez que se leyó. **Se borra al leer `CTRL`** (y también al escribir `VAL`). |

Notá que `ENABLE` y `TICKINT` son independientes: podés tener el contador andando sin interrupción
(`ENABLE=1`, `TICKINT=0`) y consultar `COUNTFLAG` por *polling*. O las dos cosas juntas, que es lo
normal para una base de tiempo.

El **reset de `CTRL` es `0x00000004`**, o sea `CLKSOURCE=1` por defecto: arranca apuntando a CCLK.

### `CLKSOURCE`: CCLK vs el reloj externo (ojo con esto)

Acá hay un punto donde mucho material simplifica de más. Vamos con cuidado:

- **`CLKSOURCE = 1` → reloj del procesador (CCLK).** Es lo que vas a usar el 99% del tiempo. En
  nuestra placa típica, CCLK = 100 MHz. Es la opción de `SysTick_Config` y de
  `SYSTICK_InternalInit`.
- **`CLKSOURCE = 0` → reloj externo de referencia, que en el LPC176x es la entrada STCLK (pin
  P3.26).** Y acá está la corrección importante: **STCLK no es "CCLK/8".** STCLK es una entrada de
  reloj *externa y arbitraria*: el contador avanza con la frecuencia que vos le metas por ese pin.
  Para usarla tenés que (a) seleccionar la función STCLK del pin P3.26 en `PINSEL7` (bits 21:20 =
  `01`; ver sección 8.5.6 del manual), y (b)
  alimentarlo con un reloj. La única restricción que pone NXP es que **la frecuencia de STCLK no
  puede pasar de 1/4 de CCLK**.

  > ¿De dónde sale el mito de "CCLK/8"? En varios Cortex-M de otros fabricantes, cuando `CLKSOURCE=0`
  > el timer toma CCLK dividido (por 8, típicamente) de forma interna. En el **LPC176x no es así**:
  > el "reloj de referencia" es lisa y llanamente la entrada STCLK del pin. Si en tu placa no hay
  > nada conectado a P3.26, con `CLKSOURCE=0` SysTick **no cuenta**. Por eso, salvo que tengas una
  > razón concreta (un reloj externo de precisión, un cristal de 32.768 kHz para un RTC por
  > software), dejá `CLKSOURCE=1` y usá CCLK.

### `LOAD`: valor de recarga

Bits [23:0]: el valor que se carga en `VAL` cuando el contador llega a 0 (y cuando arranca). Rango
útil: `0x000001` a `0xFFFFFF`. Un `LOAD = 0` es legal pero inútil: la excepción y `COUNTFLAG` se
disparan al pasar de 1 a 0, así que con `LOAD=0` nunca se activa nada.

### `VAL`: valor actual

Bits [23:0]: leerlo te da el valor instantáneo del contador (va bajando). Tiene una particularidad
clave: **escribir cualquier cosa en `VAL` lo pone en 0 y, de paso, limpia `COUNTFLAG`.** No importa
qué valor escribas; el efecto es "reiniciá el contador". Por eso en la inicialización escribimos
`SysTick->VAL = 0;`: deja el contador limpio antes de prenderlo.

### `CALIB`: calibración (y por qué no te fiés ciegamente)

`CALIB` es de solo lectura y trae una sugerencia de fábrica para generar un tick de 10 ms:

| Bit | Nombre | Significado |
|-----|--------|-------------|
| 23:0 | `TENMS` | Valor de recarga que da 10 ms **si el reloj de SysTick es de 100 MHz**. En el LPC176x viene de fábrica en `0x0F423F` = **999.999** (= 100 MHz × 10 ms − 1). |
| 30 | `SKEW`  | 1 = el `TENMS` de arriba es una **aproximación**, no exacto. 0 = es preciso. |
| 31 | `NOREF` | 1 = **no hay** reloj de referencia externo disponible. 0 = sí lo hay. |

> El manual aclara que `TENMS`, `SKEW` y `NOREF` traen un valor "de fábrica" del LPC176x y que son
> válidos **cuando el reloj (CCLK o STCLK) es de 100 MHz**. En el LPC176x los dos bits vienen en 0:
> `SKEW=0` (el `TENMS` es preciso, a 100 MHz) y `NOREF=0` (sí hay referencia externa disponible).
> `NOREF` se refiere justamente al reloj de referencia externo del que hablamos arriba (la entrada
> STCLK): te dice si la implementación tiene o no una referencia separada disponible. Es coherente
> con lo que vimos: el "reloj de referencia" del LPC176x es STCLK, no un CCLK/8 interno.
>
> Detalle curioso: la tabla de registros del capítulo 23 de NXP lista `STCALIB` como R/W, pero ARM
> lo define de **solo lectura** (capítulo 34) y CMSIS lo declara `__I` (const). Tratalo como de solo
> lectura.

La trampa: `TENMS` solo es correcto **si SysTick corre a 100 MHz**. Si configuraste CCLK a otra
frecuencia (por ejemplo 96 MHz, o el IRC a 4 MHz), `TENMS` mentiría. Por eso, en código portable y
serio, **no se usa `CALIB`**: se calcula el `LOAD` a partir de `SystemCoreClock`, que sí refleja la
frecuencia real. Pensá en `CALIB` como una pista, no como una verdad.

## El cálculo del reload (el corazón del asunto)

Querés un tick cada cierto período `T`. En ese tiempo entran `T × fclk` pulsos del reloj de SysTick.
Como el contador tarda `LOAD + 1` pasos en ir de `LOAD` hasta 0 (incluyendo el paso final a 0), la
fórmula es:

```
LOAD = (T × fclk) − 1
```

donde `fclk` es la frecuencia *del reloj de SysTick* (CCLK si `CLKSOURCE=1`).

**Ejemplo 1: 1 ms con CCLK = 100 MHz.**
`LOAD = (0.001 × 100.000.000) − 1 = 100.000 − 1 = 99.999`.

**Ejemplo 2: el mismo 1 ms pero con un STCLK externo de 12,5 MHz** (un reloj cualquiera que tengas
cableado en P3.26; lo elijo 8 veces más lento que CCLK solo para que se vea el contraste, *no* porque
sea "CCLK/8"; repasá la nota de `CLKSOURCE`):
`LOAD = (0.001 × 12.500.000) − 1 = 12.500 − 1 = 12.499`.
Mismo período, muchos menos pulsos, porque el reloj es más lento.

**Ejemplo 3: 10 ms con CCLK = 100 MHz** (el caso "de fábrica"):
`LOAD = (0.010 × 100.000.000) − 1 = 1.000.000 − 1 = 999.999 = 0x0F423F`.
Coincide exactamente con el `TENMS` de `CALIB`. Por eso ese es el valor de reset de `CALIB`.

El truco de no escribir la frecuencia a mano: en pulsos, "cuántos pulsos hay en 1 ms" es
`fclk / 1000`. Por eso vas a ver siempre `SystemCoreClock / 1000`.

> Ojo: el `− 1` vale para el uso **periódico** (el normal). El manual (sección 34.4.4.2.1) agrega el
> caso raro de **una sola** interrupción después de N pulsos (*one-shot*): ahí indica cargar
> `LOAD = N`, sin restar.

### El límite de 24 bits y el período máximo

Como `LOAD` no puede pasar de 16.777.215 y el período es `LOAD + 1` pulsos, hay un **período
máximo de un solo ciclo**:

```
T_max = 2²⁴ / fclk
```

Con CCLK = 100 MHz: `T_max = 16.777.216 / 100.000.000 ≈ 167,77 ms`. **No llega ni a 0,2 s.**
Con un reloj externo más lento (digamos 12,5 MHz) llegás a `≈ 1,34 s`; con 32.768 kHz, a unos
512 s. Para ticks de 1 ms o 10 ms sobra; para "un segundo" con el reloj del CPU, no alcanza.

**¿Y si necesito un segundo entero, o diez?** No se hace con un `LOAD` gigante (no entra). La
solución estándar es **contar ticks por software**: configurás un tick chico y cómodo (1 ms es lo
habitual), llevás una variable que se incrementa en cada interrupción, y "un segundo" son 1000 de
esos ticks. El timer hace el conteo fino de hardware; vos hacés el conteo grueso en software. Es
exactamente el patrón `millis()` que armamos abajo.

### Una sutileza de redondeo

Si `(T × fclk)` no da un entero exacto, hay error de redondeo y el tick "deriva" un poco respecto del
período ideal. Con CCLK y períodos en milisegundos casi nunca pasa (los números cierran redondos).
Donde sí aparece es con relojes externos raros: por ejemplo, el manual muestra que con STCLK a
32.768 kHz el `LOAD` para 10 ms da 327,6 → se redondea a 327, y el tick queda levemente corrido. Con
CCLK a 100 MHz no tenés ese problema.

## Configurarlo a mano

Para una interrupción cada **1 ms** con el CPU a 100 MHz, juntando todo:

```c
#include <LPC17xx.h>

void systick_init_1ms(void) {
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;   // 100000-1 a 100 MHz
    SysTick->VAL  = 0;                               // limpiar contador y COUNTFLAG
    SysTick->CTRL = (1u << 2)    // CLKSOURCE = 1 -> reloj del CPU (CCLK)
                  | (1u << 1)    // TICKINT   = 1 -> generar excepción
                  | (1u << 0);   // ENABLE    = 1 -> arrancar
}
```

`SystemCoreClock` es la variable global que `SystemInit()` dejó con la frecuencia del CPU en Hz
(módulo 3). Usarla en vez de escribir `100000000` hace el código independiente de la frecuencia: si
mañana corrés a 96 MHz, el `LOAD` se ajusta solo.

> En vez de los números mágicos `(1u<<2)|(1u<<1)|(1u<<0)`, CMSIS te da las máscaras con nombre:
> `SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk`. Es idéntico,
> pero se lee mucho mejor. Las vemos en la próxima página.

## La prioridad de la excepción (SCB->SHP)

SysTick no es una interrupción de periférico común: es una **excepción del sistema** del Cortex-M3
(número −1 en la enumeración: `SysTick_IRQn = -1`). Por eso su prioridad **no** se setea en el NVIC
con `NVIC_SetPriority` sobre el arreglo `IP[]`, sino en el bloque de control del sistema, en el
arreglo `SCB->SHP[]` (*System Handler Priority*). De fábrica queda en prioridad **alta** (valor 0),
pero lo habitual es bajarla a la mínima para que un tick no le pise el turno a una ISR más urgente.

`SysTick_Config` (la función de CMSIS de la próxima página) hace justamente eso por vos: llama a
`NVIC_SetPriority(SysTick_IRQn, (1<<__NVIC_PRIO_BITS)-1)`, que para el LPC176x (`__NVIC_PRIO_BITS`
= 5) deja la prioridad en 31, la **más baja posible**. La idea: la base de tiempo es importante pero
no es lo más urgente; si justo está corriendo el handler de una comunicación crítica, que el tick
espere.

> El detalle de cómo `SCB->SHP[]` mapea al número de excepción y la mecánica completa del NVIC y de
> las prioridades se ven en el [módulo 7](../07_interrupciones/). Por ahora alcanza con saber: SysTick
> es una excepción del sistema, su prioridad vive en `SCB->SHP`, y `SysTick_Config` ya la deja baja.

## El handler

Cuando el contador llega a 0 y `TICKINT` está en 1, el Cortex-M3 salta a una función con un nombre
fijo: **`SysTick_Handler`**. Vos la implementás:

```c
volatile uint32_t ticks_ms = 0;   // volatile: la modifica la ISR (módulo 0, cap. 08)

void SysTick_Handler(void) {
    ticks_ms++;                   // cada 1 ms
}
```

> El nombre `SysTick_Handler` no es casual: está en la **tabla de vectores** del startup de CMSIS.
> Cómo funciona esa tabla y el NVIC se ve en el [módulo 7](../07_interrupciones/). Por ahora alcanza
> con saber que "esta función se ejecuta sola cada vez que SysTick llega a 0".

¿Hay que limpiar `COUNTFLAG` en el handler? Si la excepción se disparó por `TICKINT`, **no hace
falta**: la entrada a la excepción ya "consume" el evento. (El driver de NXP que vemos en la próxima
página igual te ofrece `SYSTICK_ClearCounterFlag()`; ese es para el caso de *polling*, donde sí
necesitás limpiar la bandera a mano.)

## millis() y un `delay_ms` honesto

Con el contador de ms andando, tenés una base de tiempo monótona: `ticks_ms` es tu `millis()`. Una
demora es simplemente "esperar a que pasen N ms":

```c
void delay_ms(uint32_t ms) {
    uint32_t inicio = ticks_ms;
    while ((ticks_ms - inicio) < ms) {
        // esperar; ticks_ms lo incrementa la interrupción
    }
}
```

La resta `(ticks_ms - inicio)` está pensada para que **funcione aun cuando `ticks_ms` desborde** (da
la vuelta a los ~49,7 días en una variable de 32 bits): como la aritmética sin signo es modular, la
diferencia sigue dando el intervalo correcto. No compares nunca `ticks_ms >= inicio + ms`
directamente, porque ahí sí el desborde te rompe la cuenta.

### ¿Por qué este delay es mejor que un `for` vacío?

Un `for (volatile int i=0; i<1000000; i++);` "demora" porque el CPU pierde tiempo iterando, pero:

- **No sabés cuánto demora.** Depende del nivel de optimización del compilador, de si el código está
  en flash o RAM, de los *wait states*. Cambiás un flag de compilación y tu "delay de 1 ms" pasa a
  ser 0,3 ms.
- **No escala.** Si subís CCLK, el `for` se acelera y todos tus tiempos cambian.

El delay por SysTick, en cambio, está atado al **reloj real**: 500 ms son 500 ticks, pase lo que pase
con el compilador. Es preciso, predecible, y además se puede hacer **no bloqueante** (ver próxima
página). Su único "costo": ocupa el SysTick (que normalmente es justo lo que querés que haga) y
necesita que las interrupciones estén habilitadas.

### Programa completo: blink a 1 Hz, todo a registro

```c
#include <LPC17xx.h>
#define LED (1u << 22)   // P0.22

volatile uint32_t ticks_ms = 0;
void SysTick_Handler(void) { ticks_ms++; }

void delay_ms(uint32_t ms) {
    uint32_t t0 = ticks_ms;
    while ((ticks_ms - t0) < ms) { }
}

int main(void) {
    LPC_GPIO0->FIODIR |= LED;                      // LED como salida

    SysTick->LOAD = (SystemCoreClock / 1000) - 1;  // 1 ms
    SysTick->VAL  = 0;
    SysTick->CTRL = (1u<<2)|(1u<<1)|(1u<<0);       // CCLK, int, enable

    while (1) {
        LPC_GPIO0->FIOSET = LED;  delay_ms(500);
        LPC_GPIO0->FIOCLR = LED;  delay_ms(500);
    }
}
```

Este `delay_ms` es **el que referenciamos en el módulo 5 (GPIO)** para el blink.

## Casos de borde (para no caer en la trampa)

- **`LOAD > 0xFFFFFF` es imposible.** A nivel registro no te avisa nadie: simplemente se escriben los
  24 bits de abajo y perdés los de arriba, y tu tick sale con un período cualquiera. `SysTick_Config`
  sí valida esto y devuelve 1.
- **Olvidarte de `SysTick->VAL = 0`.** Si no limpiás `VAL`, el contador arranca desde el valor que
  tuviera y tu primer período es más corto de lo esperado. Solo el primero; después se normaliza,
  pero es una fuente de bugs de "el primer tick llegó antes".
- **`COUNTFLAG` se limpia al leer `CTRL`.** Si hacés *polling* y leés `CTRL` "para mirar otra cosa",
  borrás la bandera sin querer y perdés el evento. Leela una sola vez y guardá el resultado.
- **Leer `VAL` dos veces seguidas da valores distintos** (el contador no para). Si necesitás un valor
  estable, leelo una vez. Y recordá que **cualquier escritura en `VAL` lo reinicia a 0**: no es un
  registro "de configuración" que puedas tocar sin consecuencias.
- **Reentrancia del handler.** Si tu `SysTick_Handler` tarda *más* que el período del tick, cuando
  termina ya hay otro tick pendiente y nunca salís: el `main` se muere de hambre. Regla de oro: el
  handler hace lo mínimo (incrementar, levantar una bandera) y nada más.
- **Cambiar `CLKSOURCE` en caliente rompe todos los tiempos.** Si pasás de CCLK a STCLK (o cambiás
  CCLK con el PLL) sin recalcular `LOAD`, el `LOAD` viejo ya no corresponde al período que creías.
  Siempre que toques el reloj, recalculá `LOAD` con la nueva `fclk`.
- **En bajo consumo, si se para el reloj del CPU, se para SysTick.** El manual lo advierte en sus
  *design hints* (34.4.4.5): si un modo de ahorro de energía detiene el clock del procesador mientras
  SysTick cuenta con él, el contador se frena. Un delay por SysTick no te va a despertar de un modo
  que apaga su propio reloj.
- **En *debug*, con el CPU detenido en un breakpoint, SysTick no cuenta.** El manual lo dice
  explícito: "cuando el procesador está halted, el contador no decrementa". Es lo correcto (no querés
  que `millis()` salte 5 segundos porque paraste en un breakpoint), pero te puede confundir si medís
  tiempos paso a paso con el debugger.

En la [próxima página](./02-systick-driver.md) vemos que CMSIS empaqueta toda esta configuración en
una sola línea (`SysTick_Config`), el driver de NXP, y los patrones reales (delay no bloqueante,
timeouts, muestreo, el tick de un RTOS).

---

**Módulo:** [SysTick](./README.md) · **Siguiente:** [02 - SysTick con CMSIS](./02-systick-driver.md)
