# Watchdog (WDT): el perro guardián

El **watchdog** es un temporizador que, si lo dejás llegar a cero, **resetea el micro**. Suena raro
querer que algo te resetee, pero es justo lo contrario: es un seguro. Tu firmware tiene que
"alimentar" al perro (reiniciar su cuenta) cada tanto; si el programa se **cuelga** (un bucle infinito
imprevisto, un *deadlock*, un puntero que se fue de viaje, un periférico que nunca devuelve el ACK que
esperabas) deja de alimentarlo, el perro llega a cero y **reinicia el sistema** para sacarlo del
cuelgue. Es clave para la **robustez** de cualquier dispositivo que tiene que andar sin un humano al
lado: un datalogger en un poste, una bomba, un nodo en una red.

Capítulo 28 del manual. El WDT tiene una particularidad importante respecto del resto de los
periféricos: **no** tiene bit en `PCONP` (está siempre disponible, no se puede apagar para ahorrar) y
su clock se elige aparte con `WDCLKSEL`, no con `PCLKSEL`. Eso es a propósito: un perro guardián que se
pudiera apagar por error no serviría de mucho.

## La idea, sin vueltas

El watchdog es un **contador descendente de 32 bits** con un **pre-escaler fijo de ÷4** adelante. Vos
cargás un valor de recarga en `WDTC`; cada *feed* (alimentación) recarga el contador con ese valor; el
contador baja con cada tick del clock del watchdog (WDCLK, ya dividido por 4). Si llega a underflow
(pasa de 0), dispara la acción configurada: reset, interrupción, o las dos.

Tu trabajo es ejecutar la **secuencia de feed** periódicamente, más rápido que el timeout. Mientras
alimentes a tiempo, el contador nunca llega a cero. Si te colgás y dejás de alimentar, expira y te
rescata.

```
        WDTC (recarga)
           |
           v
WDCLK --[ ÷4 ]--> [ contador 32 bits descendente ] --underflow--> reset / interrupción
                          ^
                          | feed (0xAA, 0x55)
```

### La fórmula del timeout

Por el pre-escaler de ÷4, cada decremento del contador toma 4 ciclos de WDCLK. Si cargás `WDTC = N`:

```
Twdt = N × 4 / fwdclk
```

donde `fwdclk` es la frecuencia de la fuente elegida en `WDCLKSEL`. El **÷4** no es opcional ni
configurable: es ese pre-escaler fijo. Por eso el manual describe el rango de intervalos como
"múltiplos de `TWDCLK × 4`": no podés tener una resolución más fina que 4 ciclos de WDCLK.

- **Mínimo:** `WDTC` no puede ser menor a `0xFF` (256). Si escribís algo menor, el hardware **carga
  0xFF igual**. Entonces el intervalo mínimo es `256 × 4 / fwdclk`.
- **Máximo:** `WDTC` llega a `0xFFFFFFFF`, así que el intervalo máximo es `2³² × 4 / fwdclk`.

Ejemplo con el IRC (4 MHz en el LPC176x): para 1 segundo necesitás
`N = Twdt × fwdclk / 4 = 1 × 4 000 000 / 4 = 1 000 000` → `WDTC = 1000000` (0x000F4240).
Para el mínimo con IRC: `256 × 4 / 4 000 000 = 256 µs`.

> Ojo con un error sutil del material viejo: el IRC del LPC176x corre a **4 MHz**, no a 1 MHz. El driver
> CMSIS usa `pclk_wdt = 4000000` para la fuente IRC. Si calculás con 1 MHz te van a dar timeouts 4 veces
> más largos de lo real.

## Los registros

El struct en `LPC17xx.h` es chico y dice mucho con los tipos de acceso:

```c
typedef struct {
  __IO uint8_t  WDMOD;        // modo y estado (lectura/escritura, 8 bits)
       uint8_t  RESERVED0[3];
  __IO uint32_t WDTC;         // valor de recarga (timeout)
  __O  uint8_t  WDFEED;       // secuencia de feed (SOLO escritura)
       uint8_t  RESERVED1[3];
  __I  uint32_t WDTV;         // valor actual del contador (SOLO lectura)
  __IO uint32_t WDCLKSEL;     // fuente de clock + lock
} LPC_WDT_TypeDef;
```

| Registro   | Acceso | Reset    | Para qué |
|------------|--------|----------|----------|
| `WDMOD`    | R/W    | 0        | Modo y estado: bits `WDEN`, `WDRESET`, `WDTOF`, `WDINT` |
| `WDTC`     | R/W    | 0xFF     | Valor de recarga (timeout). Mínimo efectivo 0xFF |
| `WDFEED`   | WO     | -        | Acá va la secuencia `0xAA` → `0x55` para recargar |
| `WDTV`     | RO     | 0xFF     | Valor actual del contador |
| `WDCLKSEL` | R/W    | 0        | Fuente de clock (bits 1:0) + `WDLOCK` (bit 31) |

### WDMOD: el corazón, leé esto despacio

Cuatro bits, con semánticas distintas y trampas en cada uno:

- **`WDEN` (bit 0): habilita el watchdog.** Es **set-only**: una vez que lo ponés en 1 y completás un
  feed válido, **no se puede apagar por software**. Sólo lo limpia un reset externo o un underflow del
  propio watchdog. Esto es deliberado: si el firmware se vuelve loco, no querés que "se le ocurra"
  apagar al perro. Consecuencia práctica: **no existe un `wdt_stop()`**. Encendiste el watchdog, te
  comprometiste a alimentarlo hasta el próximo reset.

- **`WDRESET` (bit 1): el underflow resetea el chip.** También **set-only**, mismas reglas que `WDEN`.
  Si está en 0, el underflow sólo dispara la interrupción (modo interrupción). Si está en 1, el
  underflow resetea el micro (modo reset).

- **`WDTOF` (bit 2): flag de timeout.** Se pone en 1 cuando el watchdog **causó un reset**. Sobrevive
  al reset justamente para que, al arrancar, puedas preguntar "¿me reinició el watchdog?". **Sólo se
  limpia por software** (escribiendo 0). Si no lo limpiás, queda pegado y no vas a poder distinguir el
  próximo reset normal de uno por watchdog. En un POR (power-on reset) arranca en 0; tras un reset por
  watchdog, arranca en 1.

- **`WDINT` (bit 3): flag de interrupción.** Se pone en 1 cuando el watchdog expira. Es
  **read-only y NO se puede limpiar por software** directamente; se limpia con cualquier reset. Por eso,
  cuando atendés la interrupción del watchdog, la única forma de que no se vuelva a disparar
  infinitamente es **deshabilitar la IRQ en el NVIC** (`NVIC_DisableIRQ(WDT_IRQn)`), como hacen todos
  los ejemplos. La interrupción existe sobre todo para **depurar** la actividad del watchdog sin
  resetear el equipo.

Tabla de modos según la combinación (tabla 525 del manual):

| WDEN | WDRESET | Modo |
|------|---------|------|
| 0    | X       | Watchdog detenido (debug / operar sin perro) |
| 1    | 0       | **Modo interrupción**: el underflow setea `WDINT` y dispara la IRQ, no resetea |
| 1    | 1       | **Modo reset**: el underflow resetea el micro |

Detalle fino que confunde a todos: en **modo reset también se habilita la interrupción** (es la misma
fuente), pero **nunca la vas a ver**, porque el reset es tan inmediato que limpia `WDINT` antes de que
el handler corra. No esperes que tu `WDT_IRQHandler` se ejecute en modo reset.

> Una nota del manual fácil de pasar por alto: **cualquier cambio a `WDMOD` recién toma efecto después
> de un feed.** No alcanza con escribir el bit; tenés que alimentar para que el cambio "entre".

### WDTC: el valor de recarga

32 bits, reset value `0xFF`. Cada feed recarga el contador con este valor. Escribir algo menor a `0xFF`
hace que el hardware cargue `0x000000FF` igual (clamp al mínimo). El timeout sale de la fórmula de
arriba.

### WDFEED: la secuencia atómica

Este registro es a la vez el corazón y el peligro del WDT. Escribir **`0xAA` y enseguida `0x55`** recarga el contador. La secuencia
es rara a propósito: un programa descontrolado, ejecutando basura, casi no tiene chance de producir
exactamente esos dos valores en esos dos registros en orden. Eso evita que el perro se "auto-alimente"
por accidente cuando ya estás colgado.

Tres cosas que el datasheet dice y que son fáciles de ignorar:

1. **El feed también es lo que arranca el watchdog.** Setear `WDEN` en `WDMOD` **no alcanza**: el
   watchdog recién empieza a contar (y recién puede generar reset) cuando completás un feed válido
   después de habilitarlo. Hasta ese primer feed, el watchdog ignora errores de feed.

2. **Entre el `0xAA` y el `0x55` NO puede haber ninguna otra escritura a un registro del WDT.** Si
   después de escribir `0xAA` accedés a cualquier otro registro del watchdog (¡incluso una lectura mal
   ubicada según implementación, pero sobre todo una escritura!) antes del `0x55`, el hardware
   interpreta que la secuencia se rompió y **dispara un reset/interrupción inmediato** (en el segundo
   ciclo de PCLK siguiente al acceso indebido). Es una secuencia de feed mal hecha = el watchdog asume
   que algo anda mal = te resetea.

3. **Por eso la secuencia debe hacerse con interrupciones deshabilitadas.** Si una ISR salta justo
   entre el `0xAA` y el `0x55`, y esa ISR toca algún registro del WDT (por ejemplo, otra rutina que
   también alimenta, o que lee `WDTV`), te rompe la secuencia y te resetea sin razón aparente. El driver
   CMSIS lo resuelve envolviendo el feed en `__disable_irq()` / `__enable_irq()`:

   ```c
   void WDT_Feed (void) {
       __disable_irq();
       LPC_WDT->WDFEED = 0xAA;
       LPC_WDT->WDFEED = 0x55;
       __enable_irq();
   }
   ```

   Es una sección crítica corta y barata. Copiá ese patrón si escribís tu propio feed.

### WDTV: el valor actual

Solo lectura. Te dice cuánto le falta al contador para llegar a cero. Útil para diagnosticar (ver
cuánto margen te queda) o, en modo interrupción de debug, para mostrar la cuenta. Detalle de hardware:
hay **dos dominios de clock** (PCLK para acceder a los registros, WDCLK para contar) y leer `WDTV`
requiere sincronizar entre ellos, lo que toma hasta **6 ciclos de WDCLK + 6 de PCLK**. Resultado: el
valor que leés está **levemente atrasado** respecto del valor real del contador. No lo uses para
lógica de timing fino, solo como referencia.

Esa misma sincronización explica por qué escribir `WDMOD` o `WDTC` **tarda 3 ciclos de WDCLK** en
tomar efecto del lado del contador. Si la fuente es el RTC a 32 kHz, esos 3 ciclos son ~90 µs: no es
instantáneo.

### WDCLKSEL: fuente de clock y candado

Bits 1:0 (`WDSEL`) eligen de dónde sale WDCLK; el bit 31 (`WDLOCK`) congela el registro:

| Valor | Fuente | `fwdclk` | Cuándo usarla |
|-------|--------|----------|---------------|
| `0x0` | IRC (RC interno) | ~4 MHz | **Default y la más segura.** No depende de cristal externo ni de la PLL |
| `0x1` | PCLK (clock de periféricos APB) | depende del divisor | Cuando querés timeouts atados al clock del sistema |
| `0x2` | RTC (oscilador de 32 kHz) | 32768 Hz | Timeouts largos y muy bajo consumo |
| `0x3` | Reservado | - | No usar |

Dos cosas importantes:

- **`WDLOCK` (bit 31).** Una vez que lo seteás en 1, la fuente de clock **queda congelada hasta el
  próximo reset**: no se puede cambiar más por software. Es la misma filosofía que `WDEN`: blindar la
  configuración del perro contra firmware descarriado. Arranca siempre desbloqueado tras un reset.

- **El IRC es la única fuente que sigue viva en Deep Sleep.** El manual lo dice textual: en *Sleep*
  cualquier fuente sigue contando, pero en *Deep Sleep* **solo el IRC** mantiene al watchdog corriendo (y
  una interrupción del WDT en Sleep o Deep Sleep despierta al micro). Por eso, si querés usar el watchdog
  como *wake-up* de Deep Sleep (o que te rescate aun durmiendo), tenés que elegir IRC. Con PCLK o RTC, en
  Deep Sleep el perro se queda sin clock y no cuenta.

> Sobre el "windowed watchdog": algunos micros tienen un watchdog *con ventana*, donde alimentar
> **demasiado pronto** también es falta y te resetea (detecta tanto cuelgues como un firmware que
> alimenta enloquecidamente sin hacer su trabajo real). El watchdog del LPC176x **no** es windowed: solo
> le importa que no se te pase el límite superior. La única "ventana" que tiene es la restricción de la
> secuencia de feed (no romper el `0xAA`/`0x55`).

## A nivel registro

Configuración mínima en modo reset, con IRC y timeout de ~1 segundo:

```c
#include "LPC17xx.h"

void wdt_init_1s(void)
{
    // 1) Fuente de clock: IRC (4 MHz). Default, pero lo dejamos explícito.
    LPC_WDT->WDCLKSEL = 0x00;               // WDSEL = IRC, sin lock

    // 2) Valor de recarga: Twdt = WDTC * 4 / fwdclk
    //    1 s con IRC 4 MHz -> WDTC = 1*4e6/4 = 1_000_000
    LPC_WDT->WDTC = 1000000u;               // 0x000F4240

    // 3) Modo: WDEN (habilita) + WDRESET (resetea al expirar).
    //    OJO: estos bits son set-only; no hay vuelta atrás sin reset.
    LPC_WDT->WDMOD = (1u << 0)              // WDEN
                   | (1u << 1);             // WDRESET

    // 4) Primer feed: ARRANCA el watchdog. Sin esto, WDEN no hace nada.
    //    Con IRQs deshabilitadas para no romper la secuencia.
    __disable_irq();
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDFEED = 0x55;
    __enable_irq();
}

// Llamar SEGUIDO en el superloop, más rápido que el timeout.
void wdt_feed(void)
{
    __disable_irq();
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDFEED = 0x55;
    __enable_irq();
}
```

Detectar tras un reset si fue el watchdog (y limpiar el flag para no confundir el próximo arranque):

```c
int wdt_reseteo_anterior(void)
{
    if (LPC_WDT->WDMOD & (1u << 2)) {       // WDTOF
        LPC_WDT->WDMOD &= ~(1u << 2);       // limpiar (solo se limpia por software)
        return 1;                           // sí: el reset anterior fue por watchdog
    }
    return 0;                               // no: fue POR, reset externo, etc.
}
```

El superloop típico:

```c
int main(void)
{
    SystemInit();

    if (wdt_reseteo_anterior()) {
        registrar_evento("recuperado de un cuelgue");  // info de oro para el campo
    }

    wdt_init_1s();

    while (1) {
        hacer_trabajo();    // si esto se cuelga y no vuelve...
        wdt_feed();         // ...nunca se llega acá, y el WDT resetea el micro
    }
}
```

> **El anti-patrón clásico:** poner el `wdt_feed()` dentro de una interrupción periódica (por ejemplo un
> tick de SysTick) que sigue corriendo aunque el `main` esté colgado. Así el perro se alimenta solo y
> **nunca** detecta el cuelgue del flujo principal. La regla: alimentá el watchdog desde el **camino que
> realmente querés vigilar**. Si tu sistema tiene varias tareas críticas, un patrón sólido es que cada
> tarea ponga su propia bandera "estoy viva" y que un único punto central haga el feed **solo si todas
> las banderas están puestas**; así un cuelgue en cualquier tarea corta la alimentación.

## Con el driver CMSIS

`lpc17xx_wdt` encapsula la configuración y la secuencia de feed. Su API toma el timeout en
**microsegundos** y se encarga de calcular `WDTC` según la fuente:

```c
#include "lpc17xx_wdt.h"

void wdt_setup(void)
{
    // Modo reset, fuente IRC. WDT_MODE_RESET habilita interrupción Y reset.
    WDT_Init(WDT_CLKSRC_IRC, WDT_MODE_RESET);
    // Timeout en microsegundos: 1 s. Esto setea WDTC, pone WDEN y hace el primer feed.
    WDT_Start(1000000);
}

// en el superloop:
WDT_Feed();
```

Lo que hace por dentro, mapeado a lo que ya viste:

- `WDT_Init(clk, modo)` selecciona la fuente en `WDCLKSEL` y, **solo si el modo es RESET**, setea
  `WDRESET`. (En modo `WDT_MODE_INT_ONLY` deja `WDRESET` en 0.) Notá que `WDT_Init` todavía **no
  habilita** el watchdog.
- `WDT_Start(us)` calcula `WDTC` con la fórmula `WDTC = (fwdclk/1e6) * (us/4)`, setea `WDEN` y hace el
  primer `WDT_Feed()`. Recién acá arranca el perro.
- `WDT_Feed()` hace la secuencia `0xAA`/`0x55` con IRQs deshabilitadas (ver arriba).
- `WDT_ReadTimeOutFlag()` lee `WDTOF`; `WDT_ClrTimeOutFlag()` lo limpia.
- `WDT_GetCurrentCount()` devuelve `WDTV`.
- `WDT_UpdateTimeOut(us)` recalcula `WDTC` y alimenta (cambiar el timeout en caliente).

Tras un reset podés preguntar **si fue el watchdog** quien reinició y reaccionar:

```c
if (WDT_ReadTimeOutFlag()) {
    WDT_ClrTimeOutFlag();
    // registrar el incidente: "me colgué y me recuperé"
}
```

Eso te dice "el equipo se colgó y se auto-rescató", información de oro para depurar un dispositivo que
falla cada tanto en el campo, donde no tenés un debugger conectado.

> **Cuidado con la fórmula del driver y el clamp.** Por dentro, `WDT_Start` calcula `(fwdclk/1e6) * (us/4)`
> con división entera y se apoya en `WDT_SetTimeOut`, que **solo escribe `WDTC` si el resultado cae dentro
> de `[0xFF, 0xFFFFFFFF]`**; si queda fuera de rango, `WDT_SetTimeOut` retorna `ERROR`. Lo grave es que
> **`WDT_Start` es `void` e ignora ese retorno**: pase lo que pase, igual setea `WDEN` y hace el primer
> feed. O sea, si tu timeout no entra, no es que "no configura nada", sino que **arranca el watchdog con el
> `WDTC` que hubiera quedado de antes** (su valor de reset `0xFF`, o el último válido). Un timeout muy chico
> con RTC a 32 kHz, por ejemplo, no entra y te deja el perro corriendo con un período equivocado, sin
> ningún aviso. Verificá tus números con la fórmula a mano antes de llamar a `WDT_Start`.

## Modo interrupción y wake-up de bajo consumo

Hasta acá usamos el **modo reset**, que es el rol principal del watchdog: rescatar un sistema colgado.
Pero el watchdog también tiene un **modo interrupción** (`WDRESET = 0`), donde el underflow no resetea
sino que dispara la `WDT_IRQHandler`. Tiene dos usos:

1. **Depurar la actividad del watchdog** sin resetear el equipo: ponés un breakpoint en el handler y ves
   exactamente cuándo y por qué se te venció el perro, en vez de que el micro se reinicie y pierdas el
   contexto.

2. **Despertar de Deep Sleep.** Dormís el micro con `CLKPWR_DeepSleep()` y el watchdog (con fuente
   **IRC**, la única que sigue corriendo en Deep Sleep) lo despierta cada cierto tiempo. Esquema típico
   de bajo consumo: despertar, medir, dormir, repetir. El ejemplo `PWR/WDT_DeepSleep` lo muestra:

```c
void WDT_IRQHandler(void)
{
    NVIC_DisableIRQ(WDT_IRQn);   // imprescindible: WDINT no se limpia por software,
                                 // si no deshabilitás la IRQ se redispara para siempre
    WDT_ClrTimeOutFlag();
}

// ...
WDT_Init(WDT_CLKSRC_IRC, WDT_MODE_INT_ONLY);
NVIC_EnableIRQ(WDT_IRQn);
WDT_Start(2000000);              // despierta a los 2 s
CLKPWR_DeepSleep();              // a dormir; el WDT lo despierta
```

Punto crítico que se ve en el handler: como **`WDINT` no se puede limpiar por software**, si no hacés
`NVIC_DisableIRQ(WDT_IRQn)` el handler se vuelve a llamar indefinidamente. Eso vale para los dos usos
del modo interrupción.

## Casos de borde y errores típicos

- **Olvidarse el primer feed.** Seteás `WDEN` y el watchdog "no anda". Es porque `WDEN` sin feed no
  arranca nada. Siempre cerrá la inicialización con un feed.
- **Romper la secuencia de feed.** Una ISR que toca un registro del WDT entre el `0xAA` y el `0x55`, o
  meter cualquier otra escritura al WDT en el medio: reset inmediato. Solución: feed con IRQs
  deshabilitadas.
- **Alimentar desde una ISR independiente.** El perro se alimenta solo y no detecta el cuelgue del
  `main`. Alimentá desde el flujo que querés vigilar.
- **No limpiar `WDTOF`.** El flag queda pegado y arrastrás un falso "fue el watchdog" en todos los
  arranques siguientes. Limpialo en cuanto lo leés.
- **No deshabilitar la IRQ en el handler (modo interrupción).** Como `WDINT` no se limpia por software,
  la interrupción se redispara para siempre. `NVIC_DisableIRQ(WDT_IRQn)` en el handler.
- **Calcular con el IRC a 1 MHz.** Es 4 MHz; tus timeouts saldrían 4× más largos.
- **Esperar ver la ISR en modo reset.** No corre: el reset limpia `WDINT` antes.
- **Querer apagar el watchdog.** No se puede por software una vez habilitado. Si tu diseño necesita
  apagarlo, repensalo: probablemente quieras un timeout más largo, no apagarlo.
- **Timeout fuera de rango con la fuente elegida.** Sobre todo con RTC (32 kHz). `WDT_Start` es `void`:
  si el `WDTC` calculado cae fuera de `[0xFF, 0xFFFFFFFF]` no escribe el valor, pero **igual habilita y
  alimenta** el perro, que queda corriendo con un período equivocado y sin aviso. Verificá `WDTC` a mano.

## Lo que te llevás

- El watchdog **resetea el micro si dejás de alimentarlo** a tiempo: tu seguro contra cuelgues.
- Timeout `Twdt = WDTC × 4 / fwdclk`, con pre-escaler fijo ÷4 y `WDTC` mínimo 0xFF.
- La alimentación es la secuencia `0xAA` → `0x55` en `WDFEED`, **con interrupciones deshabilitadas** para
  no romperla, y es **lo que arranca** el watchdog (no `WDEN` solo).
- Una vez habilitado (`WDEN`), **no se puede apagar por software**: solo un reset lo desactiva.
- Alimentalo desde el **flujo principal**, nunca desde una ISR aislada (si no, no detecta nada).
- Tras el reset, `WDTOF` te dice si fue el watchdog; **limpialo** o arrastrás un falso positivo.
- Modo interrupción para debug y para **wake-up de Deep Sleep** (con fuente IRC); en ese modo, deshabilitá
  la IRQ en el handler porque `WDINT` no se limpia por software.

## Referencias

- Manual, Cap. 28: [`../../manual/ch28_watchdog-timer.pdf`](../../manual/ch28_watchdog-timer.pdf)
- Driver CMSIS: [`lpc17xx_wdt.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_wdt.h) ·
  [`lpc17xx_wdt.c`](../../library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_wdt.c)
- Ejemplos: [`WDT/RESET`](../../library/examples/WDT/RESET/),
  [`WDT/INTERRUPT`](../../library/examples/WDT/INTERRUPT/),
  [`PWR/WDT_DeepSleep`](../../library/examples/PWR/WDT_DeepSleep/)

---

**Anterior:** [01 - RTC](./01-rtc.md) · **Módulo:** [Periféricos adicionales](./README.md) ·
**Siguiente:** [03 - QEI](./03-qei.md)
