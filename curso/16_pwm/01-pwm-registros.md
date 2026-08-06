# PWM a nivel registro

El periférico **PWM1** es, por dentro, un **timer de 32 bits** (el mismo bloque del [módulo 8](../08_timers/))
con hardware extra para generar señales en pines. Comparte la mecánica de **prescaler + match**, así
que si entendiste timers, esto es un paso corto. Lo que agrega el PWM es lógica que convierte los
*eventos de match* en flancos sobre 6 pines de salida, con un mecanismo de actualización segura
(el latch) para que cambiar el duty al vuelo no genere glitches.

> El nombre completo en el manual es **PWM1**: es el único PWM "de propósito general" del chip (está
> basado en el bloque Timer, pero de este bloque solo las salidas PWM llegan a pines). Aparte existe el
> **Motor Control PWM** (MCPWM, capítulo 25), un periférico distinto pensado para motores trifásicos:
> 3 canales con salidas complementarias (MCOA/MCOB, en pines de P1), dead-time por hardware y entrada
> de abort. Tiene otros registros y otra API; en este módulo trabajamos con PWM1, que alcanza y sobra
> para LEDs, servos y motores DC.

## La arquitectura por dentro

El corazón es un contador con prescaler, idéntico al timer:

- **`PCLK`** entra al **`PC`** (Prescale Counter). El `PC` cuenta pulsos de `PCLK`.
- Cuando `PC` llega a **`PR`** (Prescale Register), se reinicia y el **`TC`** (Timer Counter) avanza
  **uno**. Es decir: el `TC` avanza una vez cada **`PR+1`** pulsos de `PCLK`. El `PR` define la base
  de tiempo (la "resolución temporal" de un tick).
- El `TC` se compara permanentemente contra **siete** match registers: **`MR0`...`MR6`**.

Esos siete comparadores son la clave de todo:

- **`MR0` fija el PERÍODO.** Configurás el `MCR` para que el `TC` se **resetee** cuando llega a `MR0`.
  Así el `TC` cuenta `0 → MR0` una y otra vez: cada vuelta es un período de PWM.
- **`MR1`...`MR6` fijan los puntos de conmutación** de los 6 canales de salida (PWM1.1 a PWM1.6).

```
 PCLK ─► [ PC ] ──(cada PR+1)──► [ TC: 32 bits ] ──┬─ == MR0 ─► reset (período)
                                                   ├─ == MR1 ─► flanco canal 1
                                                   ├─ == MR2 ─► flanco canal 2
                                                   │      ...
                                                   └─ == MR6 ─► flanco canal 6
```

Todos los canales comparten el mismo `TC` y el mismo `MR0`: por eso **los 6 canales tienen el mismo
período** (la misma "repetition rate"). Lo único independiente por canal es *dónde* conmuta dentro de
ese período. Esto es exactamente lo que querés para, por ejemplo, un LED RGB o varios motores que
giran a la misma frecuencia pero con duty distinto.

> **Detalle fino que el datasheet menciona al pasar:** en modo PWM el `TC` no resetea a 0 sino **a 1**
> (TCR bit 3 dice *"counter resets to 1"*). Para los cálculos de frecuencia esto es despreciable
> (1 tick entre miles), pero explica por qué el período "efectivo" es `MR0` ticks y no `MR0+1`.

## Single-edge vs double-edge: las dos formas de generar la señal

Acá está la riqueza real del PWM1, y lo que lo distingue de "un timer con match externo". Cada canal
(salvo el 1) puede trabajar en uno de dos modos, seleccionado por el bit `PWMSELn` del `PCR`.

### Single-edge (el modo común, el del duty)

En single-edge **el flanco de subida es siempre a inicio de período** (cuando el `TC` se resetea por
`MR0`) y el **flanco de bajada lo pone el match del propio canal**. Las reglas exactas del manual:

1. Todas las salidas single-edge **van a alto al comienzo del ciclo**, salvo que su match valga 0
   (en ese caso quedan siempre en bajo: duty 0%).
2. Cada salida **va a bajo cuando el `TC` alcanza su match**. Si el match es mayor que `MR0`, el match
   nunca ocurre y la salida **queda siempre en alto** (duty 100%).

```
 TC:  0 ........ MR1 ........ MR0  (0) ........ MR1 ........ MR0
                  │                              │
 PWM1.1: ─────────┐              ┌───────────────┐
        (alto)    └──────────────┘  (alto otra vez)
                  duty = MR1 / MR0
```

Entonces el **duty del canal n = MRn / MR0**. Para un LED a media intensidad: `MR1 = MR0/2`. Para
apagar: `MR1 = 0`. Para 100%: `MR1 > MR0`. Un solo match por canal; perfecto para LEDs, motores y
servos, donde solo importa el ancho del pulso, no su posición.

### Double-edge (PWM con fase: los dos flancos a voluntad)

En double-edge **un par de match controla ambos flancos** del pulso. Esto permite mover el pulso
dentro del período (control de **fase**), e incluso hacer pulsos "negativos" (bajada antes que
subida). El uso clásico es **control de motores multifase** (tres salidas con pulsos que no se solapan,
cada uno con ancho *y* posición propios), que con single-edge no podés lograr.

La asignación de qué match controla qué flanco no es libre: la fija el hardware (Table 444 del manual).
Para el canal `n`, los flancos los dan `MR(n-1)` y `MRn`:

| Canal | Single-edge: pone alto / pone bajo | Double-edge: pone alto / pone bajo |
|-------|-----------------------------------|-----------------------------------|
| PWM1.1 | MR0 / MR1 | (no puede: su vecino es MR0) |
| PWM1.2 | MR0 / MR2 | MR1 / MR2 |
| PWM1.3 | MR0 / MR3 | MR2 / MR3  *(desaconsejado)* |
| PWM1.4 | MR0 / MR4 | MR3 / MR4 |
| PWM1.5 | MR0 / MR5 | MR4 / MR5  *(desaconsejado)* |
| PWM1.6 | MR0 / MR6 | MR5 / MR6 |

Tres consideraciones que **el datasheet sí dice pero que es fácil pasar por alto**:

- **PWM1.1 nunca puede ser double-edge:** su match "vecino" sería `MR0`, que es el período. Por eso el
  driver CMSIS rechaza el canal 1 para modo edge (`PARAM_PWM1_EDGE_MODE_CHANNEL` exige 2..6).
- **Usá canales pares (2, 4, 6) para double-edge.** Como cada salida double-edge consume *dos* match
  registers consecutivos, usar el canal 3 o el 5 "parte" un par y reduce cuántas salidas double-edge
  te entran. Con 2/4/6 conseguís hasta **3 salidas double-edge** con los 6 match libres (MR1..MR6).
- En double-edge, si el flanco de bajada ocurre **antes** que el de subida, el pulso es "negativo"
  (la salida arranca en alto y baja en medio). Las reglas 1-5 del manual cubren los casos de borde
  (match igual a 0, igual al período, fuera de rango): básicamente *si un match queda fuera de `[0,
  MR0]` ese flanco no ocurre*, y *si subida y bajada caen en el mismo tick, gana la bajada*.

> **Intuición:** single-edge = "ancho del pulso anclado al inicio del período". Double-edge = "pulso
> flotante, le decís dónde empieza y dónde termina". El 95% de los proyectos (LED, motor DC, servo)
> es single-edge. Double-edge aparece en puentes H trifásicos y conversores con dead-time. Para el
> curso, dominá single-edge y entendé que double-edge existe y para qué.

## Los registros, uno por uno

Nombres **exactos** según `LPC17xx.h` (`LPC_PWM_TypeDef`). El struct expone, en orden:
`IR, TCR, TC, PR, PC, MCR, MR0..MR3, CCR, CR0..CR3, MR4..MR6, PCR, LER, CTCR`.

| Registro | Función |
|----------|---------|
| `TC` / `PC` | contador principal y contador del prescaler (no los tocás a mano normalmente) |
| `PR` | prescaler: el `TC` avanza cada `PR+1` pulsos de `PCLK` (define la base de tiempo) |
| `MR0` | **período** del PWM (el `TC` se resetea acá) |
| `MR1..MR6` | puntos de conmutación de los canales 1 a 6 |
| `MCR` | qué hacer en cada match: **interrupt / reset / stop** (3 bits por match) |
| `PCR` | habilita la **salida física** de cada canal (`PWMENA`) y elige **single/double edge** (`PWMSEL`) |
| `LER` | **Latch Enable**: confirma los nuevos valores de match (ver abajo) |
| `TCR` | control: habilitar contador (bit 0), reset (bit 1) y **modo PWM** (bit 3) |
| `IR` | flags de interrupción (match y captura); se limpian escribiendo un 1 |
| `CTCR` | timer vs counter mode (casi siempre timer; lo dejás en 0) |
| `CCR` / `CR0..3` | captura (entradas PCAP); no se usa para generar PWM |

### `TCR` (Timer Control Register): los tres bits que arrancan todo

| Bit | Nombre | Qué hace |
|-----|--------|----------|
| 0 | Counter Enable | habilita el `TC` y el `PC` para contar |
| 1 | Counter Reset | mientras esté en 1, mantiene `TC` y `PC` en reset (lo pulsás para resetear) |
| 3 | **PWM Enable** | **habilita el modo PWM**: conecta los shadow registers y la lógica de salida |

El **bit 3 es el más olvidado**. Sin él, el bloque funciona como timer común y **los pines no sacan
señal**, por más que hayas configurado todo. Y hay un orden que el manual remarca: **`MR0` tiene que
estar cargado antes de habilitar PWM mode**, porque la lógica de latch necesita que ocurra un evento
"Match 0" para volcar los shadow registers; si `MR0` es 0 o nunca matchea, nada toma efecto.

### `MCR` (Match Control Register): 3 bits por match

Por cada match `n` hay tres bits consecutivos: **`MRnI`** (interrupt on match), **`MRnR`** (reset on
match) y **`MRnS`** (stop on match). La posición del bit es `3*n + {0,1,2}`. Para PWM periódico te
interesa **uno solo**: `MR0R` (reset on match 0), que reinicia el `TC` al final de cada período.

```c
LPC_PWM1->MCR = (1u << 1);   // MR0R: resetear el TC cuando TC == MR0 (período periódico)
```

> **Error clásico:** olvidar `MR0R`. Sin reset, el `TC` sigue contando hasta desbordar a los 32 bits
> y nunca repite el período: sale un solo flanco y después nada útil.

### `PCR` (PWM Control Register): ENA contra SEL, no los confundas

Dos grupos de bits, **conceptualmente distintos**:

- **`PWMSELn`** (bits 2..6): elige el **modo** del canal `n`. `0` = single-edge, `1` = double-edge.
  Solo existe para canales 2..6 (los bits 0-1 son "unused, always zero": por eso PWM1.1 es siempre
  single-edge).
- **`PWMENAn`** (bits 9..14): **habilita la salida física** del canal `n`. El bit está en la posición
  `8 + n`. Sin esto, el pin no saca nada aunque el match conmute internamente.

```c
LPC_PWM1->PCR |= (1u << 9);    // PWMENA1: habilitar salida del canal 1 (PWM1.1 = P2.0)
// canal n: ENA en bit (8+n); SEL en bit n (solo n=2..6)
```

> **Confusión típica:** poner `PWMSEL` creyendo que habilita la salida. No: `SEL` solo cambia el modo
> (single/double); `ENA` es el que saca la señal al pin. Para single-edge dejás `SEL=0` (su valor de
> reset) y solo tocás `ENA`.

### El `LER` (Latch Enable Register): por qué tus cambios "no pasan" hasta que latcheás

Este es el concepto que más confunde y el que el datasheet explica mejor que la mayoría de los
tutoriales, así que vale entenderlo bien.

Cada `MRn` tiene detrás un **shadow register**. Cuando escribís `LPC_PWM1->MRn = valor` estando en
modo PWM, **ese valor no entra al comparador todavía**: queda "capturado" en una sala de espera. El
hardware solo lo vuelca al registro activo cuando ocurren **dos** cosas:

1. ocurre un **evento Match 0** (fin del período / reset del `TC`), y
2. el bit `n` del `LER` está en 1.

Cuando se da el volcado, el `LER` **se autolimpia** (todos sus bits vuelven a 0).

¿Por qué tanta ceremonia? **Para no generar glitches.** Si cambiaras el duty justo en medio de un
pulso, podrías acortar o alargar ese pulso de forma espuria (un flanco a destiempo). El latch
garantiza que el valor nuevo **entra siempre al inicio de un período limpio**, y que si cambiás dos
matches (típico en double-edge), **ambos entran a la vez** en el mismo borde de período: nunca medio
cambio.

```c
LPC_PWM1->MR1 = nuevo_duty;     // 1) escribir el valor nuevo (queda en el shadow)
LPC_PWM1->LER = (1u << 1);      // 2) "latch MR1": cargalo al inicio del próximo período
```

> **Error clásico número uno del PWM:** cambiar `MRn` y que **no pase nada** porque faltó el `LER`. El
> valor queda en la sala de espera para siempre. Con el driver, `PWM_MatchUpdate` hace los dos pasos
> por vos (ver [página 2](./02-pwm-con-driver.md)).

Para cambiar **varios** matches de forma atómica (caso double-edge o varios canales que deben moverse
juntos), escribís todos los `MRn` y después un solo `LER` con todos los bits prendidos:

```c
LPC_PWM1->MR1 = subida;
LPC_PWM1->MR2 = bajada;
LPC_PWM1->LER = (1u << 1) | (1u << 2);   // ambos entran en el mismo borde de período
```

### `IR` (Interrupt Register): flags de match

Nueve flags (7 de match y 2 de captura) repartidos en los primeros 11 bits, y **ojo con el mapeo no
lineal**: match 0..3 están en los bits 0..3, captura 0..1 en los bits 4..5, los bits 6..7 están
reservados, y match **4..6 en los bits 8..10**. Se limpian **escribiendo un 1** en el bit
(escribir 0 no hace nada). Lo ves en la página 2 cuando hagamos una interrupción por período.

## Ejemplo completo a registro: un servo (PWM1.1 en P2.0)

Un servo se controla con un período de **20 ms** y un pulso de ~1 a 2 ms (1.5 ms = centro). Vamos a
generarlo a registro, single-edge.

> Cada canal sale por dos pines posibles: los de P2 (P2.0..P2.5, función 01) y los alternativos de P1
> (P1.18, P1.20, P1.21, P1.23, P1.24, P1.26, función 10). Acá usamos P2.0; si tu placa lo tiene
> ocupado, PWM1.1 también está en P1.18.

```c
#include <LPC17xx.h>

void servo_init(void) {
    LPC_SC->PCONP |= (1u << 6);          // PCPWM1: encender el periférico PWM1

    // PINSEL: P2.0 como PWM1.1 (función 1)
    LPC_PINCON->PINSEL4 &= ~(0x3u << 0);
    LPC_PINCON->PINSEL4 |=  (0x1u << 0);

    // Prescaler: que el TC avance 1 vez por microsegundo.
    // Si PCLK_PWM1 = 25 MHz -> 25 pulsos = 1 us -> PR = 24 (recordá: avanza cada PR+1).
    LPC_PWM1->PR  = 24;
    LPC_PWM1->MR0 = 20000;               // período = 20000 us = 20 ms
    LPC_PWM1->MR1 = 1500;                // pulso = 1500 us = 1.5 ms (centro del servo)
    LPC_PWM1->MCR = (1u << 1);           // MR0R: resetear el TC al llegar a MR0 (periódico)
    LPC_PWM1->LER = (1u << 0) | (1u << 1); // latch MR0 y MR1 (primera carga)
    LPC_PWM1->PCR = (1u << 9);           // PWMENA1: habilitar la salida del canal 1

    LPC_PWM1->TCR = (1u << 0) | (1u << 3); // Counter Enable + PWM Enable (bit 3!)
}

// Mover el servo: posición en us (1000 = 0 grados, 1500 = centro, 2000 = 180 grados)
void servo_set(uint16_t us) {
    LPC_PWM1->MR1 = us;
    LPC_PWM1->LER = (1u << 1);           // latch: aplica al próximo período (sin glitch)
}

int main(void) {
    servo_init();
    while (1) {
        servo_set(1000);  for (volatile int i=0;i<2000000;i++);  // un extremo
        servo_set(2000);  for (volatile int i=0;i<2000000;i++);  // el otro
    }
}
```

Notá el patrón: configurás período y duty **una vez**, y después solo cambiás `MRn` + `LER`. Ese par
"escribir match, latchear" es todo el secreto de variar el duty en caliente.

> El valor real de `PCLK_PWM1` depende de `PCLKSEL0`. En reset, el LPC1769 corre el periférico a
> `CCLK/4`. El driver CMSIS, de hecho, fija `PWM1` a `CCLK/4` (con `CCLK = 100 MHz` da **25 MHz**), por
> eso `PR = 24` da exactamente 1 us. Si cambiás `PCLKSEL0`, recalculá el `PR`.

## Cálculo de frecuencia y resolución

La frecuencia del PWM sale de cuántos ticks dura un período:

```
 f_pwm = PCLK_PWM1 / ( (PR + 1) * MR0 )
```

Y la **resolución del duty** es directamente `MR0`: tenés `MR0` pasos distintos de ancho de pulso, o
sea `log2(MR0)` bits de resolución. Hay un trade-off inevitable: **más resolución (MR0 grande) ⇒
menor frecuencia máxima** para un mismo `PR`. Tres ejemplos numéricos con `PCLK_PWM1 = 25 MHz`:

**LED (brillo).** Querés frecuencia por encima de ~100 Hz para que el ojo no vea parpadeo, y buena
resolución. Con `PR = 0` (sin prescaler) y `MR0 = 25000`:

```
 f = 25 000 000 / (1 * 25 000) = 1000 Hz   ->  1 kHz, invisible al ojo
 resolución = 25000 pasos ~= 14.6 bits      ->  sobra para brillo
```

**Servo (50 Hz, pulso 1-2 ms).** Necesitás período de 20 ms y precisión de microsegundos. Con
`PR = 24` (tick = 1 us) y `MR0 = 20000`:

```
 f = 25 000 000 / (25 * 20000) = 50 Hz      ->  período = 20 ms, exacto
 el pulso útil va de MR1 = 1000 (0 grados) a 2000 (180 grados)
 resolución dentro del rango útil = 1000 pasos para 180 grados ~= 0.18 grados/paso
```

**Motor DC (PWM rápido, p. ej. 20 kHz para que no chille).** Subís la frecuencia bajando `MR0`:

```
 PR = 0, MR0 = 1250  ->  f = 25 000 000 / 1250 = 20 kHz  (inaudible)
 resolución = 1250 pasos ~= 10.3 bits  ->  más que suficiente para velocidad
```

> Regla práctica: elegí primero la **frecuencia** (la impone el actuador) y de ahí salen `PR` y `MR0`.
> Mantené `PR` chico (idealmente 0) para conservar resolución. Como `MR0` es de 32 bits, rango te
> sobra siempre; subir `PR` es una comodidad para que el tick quede en una unidad redonda (el caso
> servo: `PR = 24` deja el tick en 1 us y escribís los match directamente en microsegundos).

## Contraste con el match del Timer: ¿cuándo PWM1 y cuándo alcanza un timer?

En el [módulo 8](../08_timers/) viste que un Timer también tiene match y salida externa (EMR/MAT). De
hecho, **podés generar PWM con un timer común**: usás un match para el período y otro para el toggle.
Entonces, ¿para qué existe PWM1?

| | Timer + match externo (EMR) | PWM1 dedicado |
|---|---|---|
| Salidas con período común | 1, a lo sumo 2 con maniobras | **6 canales**, mismo `MR0` |
| Actualización segura (latch) | no la tiene: cambiar el match puede glitchear | **LER**: cambio sin glitch |
| Double-edge / fase | no | **sí** (motores multifase) |
| Esfuerzo de software | armás vos la lógica de set/reset del pin | el hardware hace todo |
| Bloquea el timer para otra cosa | sí | tiene su propio bloque |

**Cuándo te alcanza un timer:** una sola salida PWM, frecuencia fija, no te molesta un glitch ocasional
al cambiar el duty (o cambiás el duty pocas veces). Por ejemplo, un beep o un único LED de estado.

**Cuándo querés PWM1:** varios canales sincronizados (RGB, varios motores), cambio frecuente del duty
(fade, control en lazo cerrado) donde los glitches arruinarían la señal, o cualquier cosa que pida
double-edge. Para servos y motores, PWM1 es directamente lo correcto.

> **Sobre DMA:** no es típico manejar el PWM1 por DMA en el LPC1769 (los match no son una fuente DMA
> cómoda). El patrón habitual para "tablas de duty" (p. ej. generar una senoidal por PWM) es usar la
> **interrupción de match** para ir cargando el próximo `MRn` desde una tabla, no DMA. Lo vemos en la
> [página 2](./02-pwm-con-driver.md).

En la [próxima página](./02-pwm-con-driver.md) hacemos todo esto con el driver CMSIS, un *fade* de
LED, double-edge con el driver y la interrupción por período.

---

**Módulo:** [PWM](./README.md) · **Siguiente:** [02 - PWM con el driver](./02-pwm-con-driver.md)
