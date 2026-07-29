# Timers a nivel registro

Un timer del LPC1769 es, en el fondo, un **contador de 32 bits** (`TC`, Timer Counter) que avanza con
el reloj del periférico. Sobre esa cuenta se construye todo lo demás. El manual lo resume así: el
Timer/Counter "cuenta ciclos del PCLK o de un reloj externo, y puede generar interrupciones o realizar
otras acciones a valores específicos del contador, en base a cuatro registros de match". Esas "otras
acciones" son las que vamos a desmenuzar.

Cuatro usos, todos derivados del mismo contador:

- **Medir tiempo / generar demoras** precisas (leés el `TC`, o esperás un match).
- **Generar eventos periódicos** (interrumpir cada X tiempo): modo *match* con reset.
- **Generar señales** en un pin por hardware, sin CPU: *external match* (`EMR` + pines `MAT`).
- **Medir una señal externa** (ancho de pulso, período, frecuencia, timestamping): modo *capture*.

Hay **4 timers idénticos** (`LPC_TIM0`..`LPC_TIM3`), solo cambia la dirección base. Manual: capítulo 21.

> Detalle del manual que conviene tener presente desde ya: **al reset, Timer0 y Timer1 quedan
> encendidos** (PCTIM0/1 = 1 en `PCONP`), y **Timer2/3 apagados** (PCTIM2/3 = 0). O sea: si usás TIM2 o
> TIM3 y te olvidás de `PCONP`, no anda y no hay error visible. Con TIM0/1 a veces "anda de casualidad".

## La arquitectura: TC, PR y PC

El timer recibe el `PCLK` del módulo 3 (por defecto CCLK/4 = 25 MHz). Para no contar tan rápido hay un
**prescaler** de dos piezas:

- `PR` (Prescale Register): el valor máximo del prescaler. Lo fijás vos.
- `PC` (Prescale Counter): un contador auxiliar que sube con cada `PCLK`. Cuando `PC == PR`, en el
  próximo `PCLK` el `TC` se incrementa y el `PC` vuelve a 0.

```
PCLK ─▶ [PC: cuenta 0,1,...,PR] ──(cada PR+1 pulsos)──▶ [TC: +1 de 32 bits]
```

La frase exacta del manual: *"el TC se incrementa cada PR+1 ciclos de PCLK"*. De ahí salen las dos
fórmulas que conviene memorizar:

```
t_tick = (PR + 1) / PCLK          ← cuánto dura UN incremento del TC (la resolución)
T_total = (PR + 1) × (N) / PCLK   ← cuánto tarda el TC en contar N ticks
```

El prescaler es exactamente el compromiso **resolución vs. alcance**. Con `PR = 0` el `TC` avanza con
cada `PCLK` (máxima resolución: 40 ns a 25 MHz) pero "se llena" antes (40 ns × 2³² ≈ 172 s). Subiendo
`PR` ganás alcance y perdés resolución. El truco habitual es elegir `PR` para que cada tick valga una
unidad cómoda (1 µs, 100 µs) y después pensar `MR`/`CR` en esas unidades.

Como el `TC` es de 32 bits, **cuenta hasta 0xFFFFFFFF y da la vuelta a 0** (wrap-around). Ese desborde
*no* genera interrupción por sí mismo; el manual aclara que, si lo necesitás detectar, podés usar un
match. Esto importa en *capture* cuando restás dos timestamps y el contador dio la vuelta en el medio.

## El mapa de registros

Esta es la struct real del LPC1769 (`LPC17xx.h`, `LPC_TIM_TypeDef`). Ojo: en este chip hay **solo CR0 y
CR1** (no CR2/CR3, son `RESERVED`), y entre `CR1` y `EMR` / `EMR` y `CTCR` hay huecos reservados.

```c
typedef struct {
  __IO uint32_t IR;        // Interrupt Register   (banderas de match y capture)
  __IO uint32_t TCR;       // Timer Control        (enable / reset)
  __IO uint32_t TC;        // Timer Counter        (la cuenta de 32 bits)
  __IO uint32_t PR;        // Prescale Register
  __IO uint32_t PC;        // Prescale Counter
  __IO uint32_t MCR;       // Match Control        (interrumpir/resetear/parar por match)
  __IO uint32_t MR0, MR1, MR2, MR3;   // Match Registers
  __IO uint32_t CCR;       // Capture Control
  __I  uint32_t CR0, CR1;  // Capture Registers (solo lectura)
       uint32_t RESERVED0[2];
  __IO uint32_t EMR;       // External Match       (pines MATn.x por hardware)
       uint32_t RESERVED1[12];
  __IO uint32_t CTCR;      // Count Control        (timer vs counter)
} LPC_TIM_TypeDef;
```

| Registro | Función | Bits que importan |
|----------|---------|-------------------|
| `IR` | bandera por canal; se **limpia escribiendo 1** | 0-3 match MR0-3, 4-5 capture CR0-1 |
| `TCR` | bit 0 = **habilitar**, bit 1 = **reset** (mantiene `TC`/`PC` en 0) | 0, 1 |
| `TC`/`PC`/`PR` | contador, prescaler-counter, prescaler-reload | 32 bits c/u |
| `MCR` | qué hacer cuando `TC == MRx`: interrumpir/resetear/parar | 3 bits por canal |
| `MR0..MR3` | valores a comparar contra `TC` | 32 bits c/u |
| `CCR` | con qué flanco capturar y si interrumpe | 3 bits por canal (CR0, CR1) |
| `CR0`/`CR1` | copia del `TC` en el instante de la captura (solo lectura) | 32 bits |
| `EMR` | estado y modo de los pines `MATn.x` por hardware | EM0-3 + EMC0-3 (2 bits c/u) |
| `CTCR` | modo timer (cuenta `PCLK`) o counter (cuenta flancos de un pin) | 1:0 modo, 3:2 pin |

### TCR: el arranque y el reset

Solo dos bits útiles:

- **bit 0 (Counter Enable):** en 1, `TC` y `PC` cuentan. En 0, congelados.
- **bit 1 (Counter Reset):** en 1, fuerza `TC = PC = 0` en el próximo flanco de `PCLK` y los **mantiene
  en 0 mientras el bit siga en 1**. Para resetear hay que escribir 1 y después 0 (un pulso). Por eso el
  driver hace `TCR |= 2; TCR &= ~2;`.

El patrón de arranque clásico: pulso de reset y después enable.

```c
LPC_TIM0->TCR = (1u << 1);   // reset: TC=PC=0
LPC_TIM0->TCR = (1u << 0);   // enable: arranca a contar desde 0
```

### MCR: el corazón del modo match

`MCR` tiene **3 bits por cada uno de los 4 canales de match**. El manual los nombra `MRnI`, `MRnR`,
`MRnS`:

| Sufijo | Bit dentro del canal n | Qué hace cuando `TC == MRn` |
|--------|------------------------|------------------------------|
| `MRnI` | `3n + 0` | **Interrumpe** (levanta el bit n de `IR`) |
| `MRnR` | `3n + 1` | **Resetea** el `TC` a 0 |
| `MRnS` | `3n + 2` | **Para** el timer (`TC`/`PC` se frenan y `TCR[0]` se pone en 0) |

Layout completo:

```
bit:  11 10  9 | 8  7  6 | 5  4  3 | 2  1  0
     MR3S R  I |MR2S R I |MR1S R I |MR0S R I
```

Combinaciones que vas a usar de verdad:

- **MR0I + MR0R** → tic periódico: cuenta hasta `MR0`, interrumpe, vuelve a 0, repite. **Es el patrón
  base del 90% de los usos.** Si te olvidás del `MR0R`, el timer interrumpe una sola vez (en `MR0`) y
  después sigue contando hasta dar la vuelta en 2³²: parece "que dejó de andar".
- **MR0I + MR0S** → one-shot: dispara una vez y se frena solo. Útil para un timeout único.
- **Varios canales a la vez:** `MR0R` define el *período* (reset) y `MR1`/`MR2`/`MR3` marcan *instantes
  intermedios* dentro de ese período (cada uno con su `MRnI`). Es la base para generar varias señales o
  varios eventos de fase distinta con un solo timer. Importante: **el reset suele colgarse de MR0** (es
  el match más grande del ciclo); si ponés `MRnR` en un canal que matchea antes que otro, el `TC` se
  reinicia antes y el canal "tardío" nunca llega a matchear.

> Sutileza que el datasheet muestra en una figura y no en palabras: con reset-on-match, el ciclo dura
> **`MR0 + 1` ticks**, no `MR0`. El `TC` recorre `0, 1, ..., MR0` (eso es `MR0 + 1` valores) y recién en
> el ciclo siguiente al match se resetea. Por eso en las cuentas siempre restamos 1 (ver más abajo).

### IR: las banderas y el write-1-to-clear

`IR` junta 6 banderas: bits 0-3 para match MR0-3, bits 4-5 para capture CR0-1. Reglas de oro:

- Una bandera se levanta sola cuando ocurre su evento.
- **Se limpia escribiendo un 1 en su posición.** Escribir 0 no hace nada. Por eso en la ISR ponés
  `LPC_TIM0->IR = (1u << 0);` (asignación directa, no `|=`, aunque ambas funcionan: solo limpiás los
  bits que están en 1).
- Si **no** la limpiás, la interrupción queda pendiente y la ISR se vuelve a disparar apenas salís:
  reentrás para siempre. Es *el* error clásico.
- Un mismo timer tiene **un solo vector de interrupción** (`TIMERn_IRQn`) compartido por sus 4 matches
  y 2 captures. Dentro de la ISR tenés que **leer `IR` para saber qué evento fue** si usás más de uno.

### EMR y CTCR

Son la base de *external match* (generar señales por hardware) y del *modo counter* (contar flancos en
vez de tiempo). Ambos tienen su propia página porque dan para mucho:
[03 - Capture, counter y match externo](./03-capture-y-medicion.md).

## Cálculo de tiempo (esto entra en el parcial)

Con reset-on-match en `MR0`, el período entre interrupciones es:

```
T = (MR0 + 1) × (PR + 1) / PCLK_timer
```

Y despejando el valor a cargar para un período `T` dado:

```
MR0 = T × PCLK_timer / (PR + 1) − 1
```

**Ejemplo:** interrumpir cada **1 ms** con `PCLK = 25 MHz`. Elijo `PR = 24` (el `TC` avanza cada 1 µs:
`(24+1)/25e6 = 1 µs`), necesito 1000 µs:

```
MR0 = 0.001 × 25e6 / 25 − 1 = 1000 − 1 = 999
```

Verificación: `(999 + 1) × (24 + 1) / 25e6 = 1000 × 25 / 25e6 = 0.001 s`. Correcto.

**El error que arruina el 4×:** si asumís `PCLK = 100 MHz` (CCLK) cuando en realidad está en /4 = 25
MHz, te da todo 4 veces mal. El primer paso siempre es **fijar y conocer el `PCLK` del timer** (módulo
3). En el LPC1769 con el setup típico de la placa, `CCLK = 100 MHz` y `PCLK_timer = CCLK/4 = 25 MHz`.

## Ejemplo completo: LED a 1 Hz por interrupción de match (a registro)

```c
#include <LPC17xx.h>
#define LED (1u << 22)   // P0.22 (LED de la placa)

void TIMER0_IRQHandler(void) {
    LPC_TIM0->IR = (1u << 0);      // limpiar bandera de match 0 (write-1-to-clear)
    LPC_GPIO0->FIOPIN ^= LED;      // invertir el LED
}

int main(void) {
    LPC_GPIO0->FIODIR |= LED;

    // 1) Encender Timer0 (PCONP bit 1).  PCLK queda por defecto (CCLK/4 = 25 MHz)
    LPC_SC->PCONP |= (1u << 1);

    // 2) Prescaler: el TC avanza cada 1 µs  ->  PR = 25 - 1 = 24  (a 25 MHz)
    LPC_TIM0->PR = 24;

    // 3) Match: medio período = 500000 µs = 500 ms.  MR0 = 500000 - 1
    LPC_TIM0->MR0 = 500000 - 1;

    // 4) Al hacer match en MR0: interrumpir (bit0=MR0I) y resetear (bit1=MR0R)
    LPC_TIM0->MCR = (1u << 0) | (1u << 1);

    // 5) Habilitar en el NVIC y arrancar
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = (1u << 1);     // pulso de reset (TC=0)
    LPC_TIM0->TCR = (1u << 0);     // habilitar la cuenta

    while (1) { /* el LED parpadea solo desde la interrupción */ }
}
```

El "ritual": encender (`PCONP`) → prescaler (`PR`) → match (`MR0`) → acciones (`MCR`) → NVIC →
arrancar (`TCR`). Y la regla de toda ISR: **limpiar la bandera** (`IR`) o reentrás para siempre.

> Por qué togglear da 1 Hz con `MR0` = 500 ms: cada match invierte el LED, así que un ciclo completo
> (encendido + apagado) son **dos** períodos de match = 1 s = 1 Hz. Confundir "período de match" con
> "período de la señal visible" es un clásico; casi siempre hay un factor 2 cuando togglés.

## Demora por polling (sin interrupción)

No siempre querés una ISR. Para una demora bloqueante medís el `TC` directamente, o esperás la bandera
de match sin habilitar el NVIC:

```c
void delay_us(uint32_t us) {
    LPC_TIM0->TCR = 2;             // reset
    LPC_TIM0->PR  = 24;            // 1 tick = 1 µs
    LPC_TIM0->MR0 = us;            // sin reset-on-match: one-shot
    LPC_TIM0->MCR = (1u << 0);     // solo levantar la bandera (sin NVIC)
    LPC_TIM0->IR  = 0xFF;          // limpiar banderas viejas
    LPC_TIM0->TCR = 1;             // arrancar
    while (!(LPC_TIM0->IR & 1)) {} // esperar el match por polling
    LPC_TIM0->IR  = 1;             // limpiar
}
```

Es la idea detrás de los `delay` de la mayoría de las librerías. Notá: la bandera de `IR` se levanta
**aunque el NVIC no esté habilitado**; el NVIC solo decide si eso además salta a la ISR.

## Los 4 timers y sus pines

Los cuatro son idénticos en funcionalidad; lo que cambia es a qué pines salen sus `CAP` y `MAT`. El
manual garantiza al menos 2 entradas de capture y 2 salidas de match por timer, y **Timer2 saca los 4
MAT**. Pines de uso frecuente (función PINSEL 3, salvo donde se indique):

| Señal | Pin | Señal | Pin |
|-------|-----|-------|-----|
| CAP0.0 | P1.26 | MAT0.0 | P1.28 |
| CAP0.1 | P1.27 | MAT0.1 | P1.29 |
| CAP2.0 | P0.4  | MAT2.0 | P0.6  |
| CAP2.1 | P0.5  | MAT3.0 | P0.10 |

(La tabla completa está en el cap. 8, Pin Connect. Cuando varios pines comparten una misma función
`MAT`, **todos se manejan idénticos**; cuando varios comparten un `CAP`, el manual usa **el del puerto
de número más bajo**.)

## Comparación rápida: ¿qué uso para generar tiempo?

| Necesidad | Herramienta | Por qué |
|-----------|-------------|---------|
| Tick del sistema (RTOS, scheduler) | **SysTick** (módulo 6) | Está en el núcleo, 24 bits, vector propio, pensado para esto |
| Evento periódico genérico, one-shot, varios canales | **Timer match** | 32 bits, 4 matches, flexible, 4 instancias |
| PWM de varios canales con duty cómodo | **PWM1 dedicado** (módulo 19) | 6 canales, registro de latch, hecho para PWM |
| Contar eventos externos / medir señales | **Timer capture/counter** | único que lee el mundo externo |
| Disparar el ADC a frecuencia fija | **Timer (match en MR0/MR1)** | el ADC puede arrancar con un match del timer |
| Interrupción periódica pelada, sin gastar un TIMERn | **RIT** (cap. 22) | un contador, un compare, nada más |

> **Mención breve: el RIT** (*Repetitive Interrupt Timer*, cap. 22). Además de los 4 timers, el LPC1769
> trae un quinto contador pensado únicamente para interrumpir a intervalos fijos: 32 bits corriendo
> directo de su `PCLK` (sin prescaler), un registro de comparación (`RICOMPVAL`), una máscara
> (`RIMASK`: cada bit en 1 se excluye de la comparación) y un control (`RICTRL`) de 4 bits: `RITINT`
> (bandera, write-1-to-clear), `RITENCLR` (resetear el contador a 0 al comparar → modo periódico),
> `RITENBR` (frenar durante debug) y `RITEN` (habilitar; **arranca en 1 al reset**, igual que
> `RITENBR`). Con `RITENCLR = 1` el período es `(RICOMPVAL + 1) / PCLK_RIT`: la misma cuenta del
> `MR0 + 1`, pero sin prescaler. Sin capture, sin pines, un solo "match": todo lo que hace el RIT lo
> hace también un timer con `MR0I + MR0R`, pero conviene saber que existe (vector `RIT_IRQn`,
> encendido en `PCONP` bit 16).

Profundizamos cada contraste en la [próxima página](./02-timers-con-driver.md) y en la
[tercera](./03-capture-y-medicion.md).

---

**Módulo:** [Timers](./README.md) · **Siguiente:** [02 - Timers con el driver](./02-timers-con-driver.md)
