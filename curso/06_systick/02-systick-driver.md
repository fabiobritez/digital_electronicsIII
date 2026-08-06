# SysTick con CMSIS: `SysTick_Config` y el driver

En la página anterior configuramos `LOAD`, `VAL` y `CTRL` a mano. Eso está perfecto para entender qué
pasa por dentro, pero en la práctica vas a usar una de dos formas más cómodas: la función estándar de
ARM (`SysTick_Config`) o el driver de NXP (`lpc17xx_systick`). Las dos hacen exactamente lo mismo que
escribiste a registro; cambia el envoltorio.

## Opción A: `SysTick_Config()`, la estándar de ARM (todo en uno)

Esta función vive en `core_cm3.h` (CMSIS-Core), así que sirve en **cualquier** Cortex-M. Es `static
inline`, o sea que no agrega una llamada real: el compilador la mete en línea. Hace cinco cosas en
una:

```c
#include "LPC17xx.h"

volatile uint32_t ticks_ms = 0;

void SysTick_Handler(void) {
    ticks_ms++;               // con SysTick_Config NO hay que limpiar bandera
}

int main(void) {
    SysTick_Config(SystemCoreClock / 1000);   // interrupción cada 1 ms
    while (1) { /* ... */ }
}
```

`SysTick_Config(ticks)` recibe **cuántos pulsos** entre interrupciones (no milisegundos). Por eso
`SystemCoreClock / 1000` = pulsos en 1 ms. Devuelve `0` si OK, `1` si `ticks` no entra en 24 bits.

Mirá lo que hace por dentro (código de `core_cm3.h` con los nombres exactos; los comentarios están
traducidos):

```c
static __INLINE uint32_t SysTick_Config(uint32_t ticks)
{
  if (ticks > SysTick_LOAD_RELOAD_Msk)  return (1);            // no entra en 24 bits
  SysTick->LOAD  = (ticks & SysTick_LOAD_RELOAD_Msk) - 1;      // LOAD = ticks - 1
  NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  // prioridad mínima
  SysTick->VAL   = 0;                                          // limpiar contador
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |                // CCLK
                   SysTick_CTRL_TICKINT_Msk   |                // interrupción
                   SysTick_CTRL_ENABLE_Msk;                    // arrancar
  return (0);
}
```

Punto por punto, esto es lo de la página anterior más una cosa nueva:

1. **Valida los 24 bits** (`> SysTick_LOAD_RELOAD_Msk`, que es `0xFFFFFF`). Si pedís un período que
   no entra, no rompe nada: devuelve 1 y *no* configura. Por eso conviene chequear el valor de
   retorno.
2. **`LOAD = ticks − 1`**: la resta del "menos uno" la hace ella. Vos pasás los pulsos crudos.
3. **Baja la prioridad de la excepción** a `(1<<__NVIC_PRIO_BITS)-1` vía `SCB->SHP` (lo charlamos en
   la página anterior). En el LPC176x eso es 31, la mínima. Esto es lo que *no* hace la versión a
   registro de la página 01, que deja la prioridad de reset (alta).
4. **Usa siempre CCLK** (`CLKSOURCE_Msk`). `SysTick_Config` no te deja elegir STCLK; si querés el
   reloj externo, tenés que ir a registro o al driver de NXP.
5. **Habilita interrupción y arranca** en una sola escritura a `CTRL`.

Por eso con `SysTick_Config` **no limpiás `COUNTFLAG` en el handler**: la excepción ya consumió el
evento, y `COUNTFLAG` igual se autolimpia al leer `CTRL`. No la toques.

## Opción B: el driver de NXP `lpc17xx_systick`

NXP da funciones que piensan en **milisegundos** directamente, así no calculás el `LOAD` vos:

```c
#include "lpc17xx_systick.h"

volatile uint32_t ticks_ms = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();   // con este driver SÍ se limpia la bandera
    ticks_ms++;
}

int main(void) {
    SYSTICK_InternalInit(1);      // 1 ms, usando el reloj del CPU (CCLK)
    SYSTICK_IntCmd(ENABLE);       // habilitar interrupción (TICKINT)
    SYSTICK_Cmd(ENABLE);          // arrancar contador (ENABLE)
    while (1) { }
}
```

| Función | Qué hace |
|---------|----------|
| `SYSTICK_InternalInit(ms)` | Configura el período en ms con el reloj del CPU (CCLK). Setea `CLKSOURCE=1` y `LOAD`. |
| `SYSTICK_ExternalInit(freq, ms)` | Igual pero con el reloj externo STCLK (pin P3.26); le pasás la frecuencia real de STCLK en Hz. |
| `SYSTICK_Cmd(ENABLE/DISABLE)` | Arranca/detiene el contador (bit `ENABLE`) |
| `SYSTICK_IntCmd(ENABLE/DISABLE)` | Habilita/deshabilita la interrupción (bit `TICKINT`) |
| `SYSTICK_GetCurrentValue()` | Lee `VAL` |
| `SYSTICK_ClearCounterFlag()` | Limpia `COUNTFLAG` |

Detalles del driver que conviene saber (los sacamos del `.c`):

- `SYSTICK_InternalInit` calcula `LOAD = (SystemCoreClock/1000)*ms - 1`. O sea, usa **CCLK** directo,
  igual que `SysTick_Config`. **Cuidado con un mito frecuente:** este driver *no* divide CCLK por 8.
  El "reloj interno" del LPC176x es CCLK, punto.
- Si el `ms` que pedís no entra en 24 bits (a 100 MHz, más de 167 ms), el driver entra en un
  `while(1)` (se cuelga). No devuelve error como `SysTick_Config`: directamente te traba. Por eso,
  con períodos grandes, calculá antes si entran.
- `SYSTICK_ExternalInit(freq, ms)` no adivina la frecuencia de STCLK: se la tenés que pasar vos en Hz
  (es un reloj externo arbitrario). Calcula `LOAD = (freq/1000)*ms - 1` y pone `CLKSOURCE=0`. Si en
  P3.26 entran 5 kHz, pasás `freq = 5000`. (El ejemplo `SysTick/STCLK` de la librería hace justamente
  esto.)
- **`SYSTICK_ExternalInit` NO configura el pin P3.26.** El driver solo toca `CTRL` y `LOAD`; la
  función STCLK del pin la tenés que seleccionar vos *antes*, por `PINSEL` (en el ejemplo de la
  librería, con `PINSEL_ConfigPin` y `Funcnum = 1`: en `PINSEL7`, la función `01` de P3.26 es
  STCLK). Si te olvidás de habilitar la función STCLK del
  pin, `CLKSOURCE=0` apunta a una entrada que no tiene reloj y SysTick no cuenta. Esto vale también si
  configurás STCLK a registro: primero el pin, después el SysTick.

> **Ojo, diferencia importante:** con `SysTick_Config` **no** limpiás la bandera en el handler; con el
> driver de NXP, su handler de ejemplo **sí** llama a `SYSTICK_ClearCounterFlag()`. En rigor, si la
> excepción la disparó `TICKINT`, no es estrictamente necesario limpiarla (la entrada a la excepción
> ya consume el evento). Pero como el driver lo hace en sus ejemplos, seguí esa convención cuando uses
> el driver, y no la uses cuando uses `SysTick_Config`. Mezclar las dos formas en el mismo proyecto es
> un error típico de parcial.

## ¿Cuál uso?

- `SysTick_Config(SystemCoreClock / 1000)` para casi todo: es portable, valida los 24 bits, baja la
  prioridad sola y es una línea. Es la recomendación por defecto.
- El driver de NXP si querés pensar en milisegundos sin calcular pulsos, o si necesitás STCLK (reloj
  externo), que `SysTick_Config` no soporta.
- A registro (página anterior) cuando querés control total o estás aprendiendo qué pasa por dentro.

## Patrones reales

### Delay no bloqueante (mejor que `delay_ms`)
En vez de quedarte esperando en un `while`, dejás que el `main` haga otras cosas y solo actuás cuando
pasó el tiempo. Esta es la diferencia entre un firmware de juguete y uno que hace varias cosas a la
vez:

```c
volatile uint32_t ticks_ms = 0;
void SysTick_Handler(void) { ticks_ms++; }

int main(void) {
    SysTick_Config(SystemCoreClock / 1000);   // 1 ms
    uint32_t t_led = 0;
    while (1) {
        if (ticks_ms - t_led >= 500) {   // cada 500 ms, sin bloquear
            t_led = ticks_ms;
            LPC_GPIO0->FIOPIN ^= (1u << 22);
        }
        // ...acá el main sigue libre para atender botones, UART, etc...
    }
}
```

Notá la resta `ticks_ms - t_led` de nuevo: es resistente al desborde de `ticks_ms`, por la misma
razón que en la página anterior.

### Muestreo periódico de un sensor
```c
volatile uint8_t flag_muestreo = 0;
void SysTick_Handler(void) { flag_muestreo = 1; }

int main(void) {
    SysTick_Config(SystemCoreClock / 10);   // cada 100 ms (10 Hz)
    while (1) {
        if (flag_muestreo) {
            flag_muestreo = 0;
            uint16_t v = leer_adc();    // muestrear FUERA de la ISR
            procesar(v);
        }
    }
}
```

El handler solo levanta una bandera; el trabajo pesado (leer el ADC, procesar) va en el `main`. Es la
regla de oro de las interrupciones.

### Timeout para una comunicación
```c
volatile uint32_t timeout = 0;
void SysTick_Handler(void){ if (timeout) timeout--; }

// ... al iniciar la espera:  timeout = 1000;   // 1 s
// while (!hay_dato() && timeout);  -> si timeout llegó a 0, hubo timeout
```

### El tick de un RTOS
Si en algún momento usás un sistema operativo de tiempo real (FreeRTOS, por ejemplo), vas a ver que
**su latido (el "tick" del scheduler) casi siempre es SysTick.** No es casualidad: como SysTick es
parte del núcleo, está en *todos* los Cortex-M, así que el RTOS puede confiar en que existe sin
depender del fabricante. FreeRTOS, de hecho, define su propio handler de SysTick y le pone una
prioridad cuidadosamente elegida. Por eso, si usás un RTOS, **no configures vos también el SysTick**:
ya es del sistema operativo. Pelearse por el SysTick con el RTOS es un bug clásico.

## Errores comunes (los mismos que en el parcial)

| Error | Arreglo |
|-------|---------|
| Configurar y no habilitar (`Cmd`/`IntCmd`) → no pasa nada | Habilitar contador **e** interrupción (o usá `SysTick_Config`, que hace todo) |
| Olvidar `SYSTICK_ClearCounterFlag()` *cuando usás el driver de NXP* | Limpiar la bandera en el handler del driver |
| Limpiar la bandera *con* `SysTick_Config` (no hace falta) | No la toques con `SysTick_Config` |
| Variable de ISR sin `volatile` → el `main` no la "ve" | `volatile uint32_t ...` |
| Pedir > ~167 ms en un solo período con CCLK a 100 MHz (no entra en 24 bits) | Usar período chico (1 ms) y contar ticks por software |
| Comparar `ticks >= t0 + N` (se rompe en el desborde) | Usar la resta `ticks - t0 >= N` |
| Handler largo (procesar todo dentro de la ISR) | Handler corto: solo incrementar o levantar una bandera |
| Configurar SysTick a mano teniendo un RTOS corriendo | Dejarle el SysTick al RTOS |

> La regla de oro de cualquier interrupción: **el handler hace lo mínimo** (incrementar, levantar una
> bandera) y el trabajo pesado va en el `main`. Lo profundizamos en el
> [módulo 7](../07_interrupciones/).

## Ejercicios
1. **Cronómetro** en décimas de segundo, mostrado por UART.
2. **Antirrebote** de 50 ms para un botón, usando el contador de ms.
3. **3 LEDs** parpadeando a 250/500/1000 ms con un solo SysTick (sin bloquear).
4. Reescribí el delay no bloqueante **a registro** (sin driver) y compará con la versión `SysTick_Config`.
5. Calculá el período máximo de un solo ciclo de SysTick con CCLK a 100 MHz, a 48 MHz y con un STCLK
   externo de 32.768 kHz. ¿Para cuál te alcanza un único `LOAD` para hacer un tick de 1 s?

> El material original [`_origen/03_SYSTICK.md`](./_origen/03_SYSTICK.md) tiene más ejemplos
> (reloj externo STCLK, reloj de tiempo real por software, etc.).

---

**Anterior:** [01 - SysTick a nivel registro](./01-systick-registros.md) ·
**Siguiente módulo:** [07 - Interrupciones](../07_interrupciones/)
