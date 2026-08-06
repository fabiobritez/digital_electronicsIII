# QEI: Interfaz de encoder en cuadratura

El **QEI** (*Quadrature Encoder Interface*) lee **encoders rotativos en cuadratura**, los sensores que
te dicen **cuánto y hacia dónde** giró un eje. Son la base del control de motores con realimentación de
posición y velocidad: brazos robóticos, ruedas de un robot, perillas digitales, posicionamiento de mesas
CNC. Hacerlo "a mano" con interrupciones por GPIO es posible a baja velocidad, pero a RPM altas la CPU se
satura contando flancos. El QEI lo decodifica todo por hardware: vos solo leés registros.

Capítulo 26 del manual. PCONP bit **18** (`PCQEI`). Se complementa muy bien con el **PWM** (módulo 16)
para cerrar un lazo de control de motor, y comparte pines con la realimentación del **MCPWM**.

## Qué es un encoder en cuadratura

Un encoder de cuadratura genera **dos señales** desfasadas 90°, llamadas **A** (`PhA`) y **B** (`PhB`).
Por el **orden** en que cambian, sabés el sentido de giro; por la **cantidad** de flancos, cuánto giró:

```
giro horario (forward):   A:  _▁▔▔▁▁▔▔▁▁     A adelanta a B
                          B:  ▁_▁▔▔▁▁▔▔▁

giro antihorario (rev):   A:  _▁▔▔▁▁▔▔▁▁     B adelanta a A
                          B:  ▔▔▁▁▔▔▁▁▔▔
```

La regla del manual es exacta: **cuando los flancos de PhA adelantan a los de PhB, la posición sube;
cuando los de PhB adelantan a los de PhA, baja.** Si se ve un par de flancos (subida + bajada) en una
fase sin que cambie la otra, hubo **cambio de sentido**. Cualquier otra transición es **ilegal** y
levanta el bit de error (ERR).

Las dos señales recorren una secuencia de 4 estados en código Gray (un solo bit cambia por paso). El
manual los numera así:

| Estado | PhA | PhB |
|--------|-----|-----|
| 1 | 1 | 0 |
| 2 | 1 | 1 |
| 3 | 0 | 1 |
| 4 | 0 | 0 |

La secuencia 1→2→3→4→1 es un sentido (positivo) y 1→4→3→2→1 el otro (negativo). El decodificador del QEI
sigue esa máquina de estados y, en cada transición válida, manda un pulso de "clock" con su "direction"
al contador de posición.

Muchos encoders tienen una tercera señal, **index** (también `PhZ` o `IDX`), que da **un pulso por vuelta**
completa, útil para tener un cero absoluto de referencia y para contar vueltas enteras.

## Resolución: 2X vs 4X (y por qué no hay "1X")

Acá hay una sutileza que el material superficial suele saltear. Un encoder se especifica por su **PPR**
(*pulses per revolution*, pulsos por vuelta de **una** fase, también llamado **CPR**, *cycles per
revolution*). Pero el QEI no cuenta "pulsos": cuenta **flancos**, y eso multiplica la resolución:

- **CAPMODE = 0 → modo 2X:** cuenta los flancos (subida y bajada) **solo de PhA**. Resolución = `2 × PPR`.
- **CAPMODE = 1 → modo 4X:** cuenta los flancos **de PhA y de PhB**. Resolución = `4 × PPR`. Es la máxima.

No existe un modo "1X" en este hardware: el contador siempre usa al menos los dos flancos de PhA. El
4X te da el doble de resolución de posición que el 2X, **a costa de menos rango** del contador de 32 bits
(llega antes al máximo). En la práctica con 32 bits el rango sobra casi siempre, así que **4X es el
default razonable**.

> Cuidado con la nomenclatura: lo que la página vieja llamaba "x4 = cuenta los 4 flancos del ciclo" es el
> 4X de acá. Pero el bit de configuración es `CAPMODE`, **no** un campo de 3 valores. Solo hay dos modos.

## Los dos modos de señal de entrada (SIGMODE)

El QEI acepta dos tipos de encoder, según el bit `SIGMODE` de `QEICONF`:

- **SIGMODE = 0 → cuadratura.** PhA y PhB son las dos señales en cuadratura; el decodificador interno
  produce el clock y la dirección a partir de ellas. Es el caso normal.
- **SIGMODE = 1 → clock/direction.** Algunos encoders (y algunos controladores) ya entregan la señal
  **decodificada**: PhA pasa a ser la **dirección** y PhB el **clock**. El decodificador de cuadratura
  se *bypassea*. Útil cuando otro chip ya hizo la cuenta o el "encoder" es en realidad un generador de
  pasos.

En **ambos** modos, la señal de dirección pasa todavía por el bit `DIRINV`: si tu cableado de A y B está
intercambiado y el motor "cuenta al revés", en vez de re-soldar ponés `DIRINV = 1` y el QEI **complementa**
el bit DIR. La tabla de dirección resultante es:

| DIR | DIRINV | Dirección |
|-----|--------|-----------|
| 0 | 0 | forward |
| 1 | 0 | reverse |
| 0 | 1 | reverse |
| 1 | 1 | forward |

## Pines: MCI0 / MCI1 / MCI2

| Señal | Pin del QEI | Función |
|-------|-------------|---------|
| PhA | `MCI0` | entrada Phase A |
| PhB | `MCI1` | entrada Phase B |
| IDX | `MCI2` | entrada Index |

Son los **mismos pines** que la realimentación del MCPWM (Motor Control PWM): el QEI es una alternativa a
realimentar directo al MCPWM. En la placa del curso, el ejemplo oficial usa **P1.20 (MCI0), P1.23 (MCI1)
y P1.24 (MCI2)** con **función 01** (`PINSEL3`). Acordate de elegir también el **modo de pin** (`PINMODE`):
si el encoder es open-collector necesitás pull-up; si ya trae su propio pull-up, dejalo sin resistencia
interna.

## El bloque por dentro

Vale la pena tener el diagrama mental del capítulo 26:

```
 PhA ─┐
 PhB ─┤→ FILTRO DIGITAL → DECODER CUADRATURA ─┬→ CONTADOR POSICIÓN ─→ QEIPOS
 IDX ─┘                    (clk + dir)         │   ↑ compara con CMPOS0/1/2 → POSn_Int
                                               │   ↑ reset por MAXPOS o por index (RESPI)
                                               └→ CONTADOR VELOCIDAD ─→ QEIVEL
                                                   ↑ ventana = TIMER VELOCIDAD (QEILOAD→QEITIME)
                                                   al overflow: QEIVEL→QEICAP, compara con VELCOMP
            IDX → CONTADOR INDEX (INXCNT) ─→ compara con INXCMP → REV_Int
```

Hay tres sub-bloques que trabajan en paralelo: **posición**, **velocidad** e **índice (vueltas)**. Cada
uno tiene su contador, su comparador y su interrupción.

## Mapa de registros

| Registro | Acceso | Para qué |
|----------|--------|----------|
| `QEICON`   | WO | Comandos de reset (posición, velocidad, índice, reset-en-index) |
| `QEICONF`  | R/W | Configuración: `DIRINV`, `SIGMODE`, `CAPMODE`, `INVINX` |
| `QEISTAT`  | RO | Estado: bit `DIR` (sentido actual) |
| `QEIPOS`   | RO | **Posición actual** (sube/baja con el giro) |
| `QEIMAXPOS`| R/W | Posición máxima para el **wrap circular** |
| `CMPOS0/1/2`| R/W | Tres comparadores de posición (disparan `POSn_Int`) |
| `INXCNT`   | RO | Contador de **vueltas** (pulsos de index) |
| `INXCMP`   | R/W | Comparador de índice (dispara `REV_Int`) |
| `QEILOAD`  | R/W | Recarga del timer de velocidad (define la **ventana**) |
| `QEITIME`  | RO | Valor actual del timer de velocidad |
| `QEIVEL`   | RO | Pulsos contados en la ventana **en curso** |
| `QEICAP`   | RO | Pulsos de la ventana **terminada** (se lee al overflow) |
| `VELCOMP`  | R/W | Umbral de **velocidad baja** (dispara `VELC_Int`) |
| `FILTER`   | R/W | Largo del filtro digital antirruido (en clocks de PCLK) |
| `QEIINTSTAT` | RO | Flags de interrupción pendientes |
| `QEISET` / `QEICLR` | WO | Setear / limpiar flags de `QEIINTSTAT` |
| `QEIIE`    | RO | Máscara de habilitación de interrupciones |
| `QEIIES` / `QEIIEC` | WO | Habilitar / deshabilitar interrupciones (set/clear de `QEIIE`) |

> Ojo con el patrón set/clear: a `QEIIE` (la máscara) **no se le escribe directo**; se la prende con
> `QEIIES` y se la apaga con `QEIIEC`. Y los flags de `QEIINTSTAT` se limpian escribiendo en `QEICLR`.
> Es el mismo idioma "atómico" que ya viste en otros periféricos.

## QEICON: los comandos de reset

`QEICON` es write-only y **autoclears** (se borra solo cuando el contador correspondiente quedó en cero):

| Bit | Símbolo | Efecto |
|-----|---------|--------|
| 0 | `RESP`  | Resetea el contador de **posición** a cero |
| 1 | `RESPI` | Habilita que el **pulso de index** resetee la posición a cero |
| 2 | `RESV`  | Resetea el contador de **velocidad** y recarga el timer (sin generar `TIM_Int`) |
| 3 | `RESI`  | Resetea el contador de **índice** (vueltas) a cero |

`RESV` te sirve para "tirar" una ventana de velocidad mala (por ejemplo justo después de arrancar el
motor) sin esperar el overflow y sin que se dispare la interrupción de velocidad.

## A nivel registro

```c
#include "LPC17xx.h"

void qei_init(void)
{
    LPC_SC->PCONP |= (1u << 18);            // encender QEI (PCQEI)

    // PINSEL3: P1.20=MCI0(PhA), P1.23=MCI1(PhB), P1.24=MCI2(IDX), función 01
    LPC_PINCON->PINSEL3 &= ~((3u<<8) | (3u<<14) | (3u<<16));
    LPC_PINCON->PINSEL3 |=  ((1u<<8) | (1u<<14) | (1u<<16));

    // Configuración: cuadratura, 4X, sin invertir dirección ni index
    //   bit0 DIRINV=0, bit1 SIGMODE=0, bit2 CAPMODE=1 (4X), bit3 INVINX=0
    LPC_QEI->QEICONF = (1u << 2);           // 0x4 → 4X

    LPC_QEI->QEIMAXPOS = 0xFFFFFFFF;        // sin wrap (rango completo de 32 bits)

    // Resetear posición, velocidad e índice antes de empezar
    LPC_QEI->QEICON = (1u<<0) | (1u<<2) | (1u<<3);  // RESP | RESV | RESI
}

int32_t qei_posicion(void)
{
    return (int32_t)LPC_QEI->QEIPOS;        // sube/baja sola con el giro
}

uint32_t qei_direccion(void)
{
    return LPC_QEI->QEISTAT & 0x1;          // bit DIR
}
```

El hardware mantiene `QEIPOS` al día sin gastar CPU, gires a la velocidad que gires. `QEIPOS` es de
32 bits sin signo: si trabajás con posiciones relativas que pueden ir "hacia atrás" de cero, conviene
castear a `int32_t` o usar el wrap circular (abajo).

## Wrap circular: QEIMAXPOS

Para aplicaciones **rotativas** (una rueda, una perilla sin tope) querés que la posición "dé la vuelta" en
vez de crecer sin fin. Para eso está `QEIMAXPOS`:

- En sentido **forward**, cuando `QEIPOS` pasaría de `QEIMAXPOS`, se **resetea a 0** (y se incrementa el
  contador de vueltas `INXCNT`).
- En sentido **reverse**, cuando `QEIPOS` bajaría de 0, **carga `QEIMAXPOS`** (y decrementa `INXCNT`).

Si tu encoder tiene 1000 pulsos por vuelta y vas a 4X, una vuelta son 4000 flancos, así que ponés
`QEIMAXPOS = 3999` y la posición vive siempre en `[0, 3999]` representando el ángulo dentro de la vuelta.

## Comparadores de posición CMPOS0/1/2

Tenés **tres** comparadores de posición independientes. Cada uno guarda un valor; cuando `QEIPOS` lo
iguala, se levanta `POS0_Int` / `POS1_Int` / `POS2_Int`. Son ideales para "avisame cuando el eje pase por
tal ángulo" sin *pollear* la posición: disparás un evento por hardware. Combinados con el contador de
vueltas, los flags `POSnREV_Int` se levantan solo cuando **a la vez** coincide la posición y la vuelta
(útil para un setpoint absoluto del tipo "vuelta 3, ángulo 90°").

## Índice y conteo de vueltas (INXCNT / INXCMP)

El pulso de **index** (una vez por vuelta) alimenta el contador `INXCNT`. Sube cuando la posición hace
*overflow* de `QEIMAXPOS` (forward) y baja cuando hace *underflow* de cero (reverse). Si activás `RESPI`,
además el index **recalibra** la posición a cero en cada vuelta: así se compensa la deriva acumulada por
ruido o flancos perdidos, y el cero queda anclado a la marca física del encoder.

`INXCMP` es un comparador de índice: cuando `INXCNT` lo iguala, se dispara `REV_Int`. Sirve para "frená
después de N vueltas". El bit `INVINX` de `QEICONF` invierte el sentido del pulso de index, por si tu
encoder lo entrega activo-bajo.

## Medición de velocidad a fondo

Esta es la parte que más se suele explicar mal, así que vamos despacio. El QEI tiene un **timer de
velocidad** que cuenta clocks de `PCLK`. Funciona como una ventana de tiempo fija:

1. `QEITIME` cuenta hacia arriba desde 0 con cada tick de `PCLK`.
2. En paralelo, `QEIVEL` cuenta los **flancos del encoder** (con la misma resolución 2X/4X que la
   posición) que ocurrieron en la ventana actual.
3. Cuando `QEITIME` **desborda** (llega al valor cargado en `QEILOAD`), pasan cuatro cosas, todas de
   golpe:
   - `QEIVEL` se copia a `QEICAP` (la "foto" de la ventana que terminó),
   - `QEIVEL` se pone a cero para empezar a contar la ventana siguiente,
   - `QEITIME` se recarga desde `QEILOAD`,
   - se dispara la interrupción `TIM_Int`.

Entonces: **`QEIVEL` es la cuenta en vivo; `QEICAP` es el resultado consolidado de la última ventana.**
Para velocidad siempre leés `QEICAP` (típicamente desde el handler de `TIM_Int`).

### Dimensionar la ventana (QEILOAD ↔ PCLK)

La ventana dura `(QEILOAD + 1)` ticks de `PCLK`, o sea **`T_ventana = (QEILOAD + 1) / PCLK` segundos**.
(El `+1` importa: el driver CMSIS lo tiene en cuenta tanto al cargar como al calcular RPM.) Elegís
`QEILOAD` según el compromiso clásico:

- **Ventana larga** → más flancos por ventana → mejor resolución de velocidad, pero respuesta más lenta.
- **Ventana corta** → reacciona rápido, pero a baja RPM podés contar muy pocos flancos (cuantización
  grosera) o cero.

Si querés muestrear cada `T` segundos con un `PCLK` dado: `QEILOAD = PCLK × T − 1`.
Por ejemplo, para muestrear cada 250 ms (¼ de segundo) con `PCLK = 25 MHz`: `QEILOAD = 25e6 × 0.25 − 1 =
6 249 999`.

> Aclaración importante sobre el clock: el driver CMSIS, al inicializar, fija `PCLK_QEI = CCLK` (divisor
> 1), **no** CCLK/2 como dice el comentario del código. Si configurás los registros a mano, definí el
> divisor del QEI en `PCLKSEL0` y usá ese mismo `PCLK` en todas las cuentas. Mezclar el `PCLK` real con el
> que asumís en la fórmula es **el error número uno** al pasar a RPM.

### La fórmula exacta de RPM

```
        QEICAP × 60 × PCLK
RPM = ─────────────────────────
       QEILOAD × PPR × Edges
```

(En la implementación del driver, `QEILOAD` se toma como `QEILOAD + 1`, y `Edges = 4` si `CAPMODE = 1`,
`2` si `CAPMODE = 0`.) Donde:

- `QEICAP` = flancos contados en la ventana terminada,
- `PCLK` = clock real del bloque QEI,
- `60` = pasaje de "por segundo" a "por minuto",
- `QEILOAD` = define la duración de la ventana,
- `PPR` = pulsos por vuelta **de una fase** del encoder físico,
- `Edges` = 2 (modo 2X) o 4 (modo 4X), los flancos por pulso. `PPR × Edges` es la resolución total.

**Ejemplo del manual:** motor a 600 RPM, encoder de 2048 PPR en 4X → 8192 flancos por vuelta. A 600 RPM el
eje da 10 vueltas/seg → 81 920 flancos/seg. Con un timer a 10 000 Hz y `QEILOAD = 2500` (¼ de segundo),
cada ventana cuenta `QEICAP = 20 480` flancos:

```
RPM = (10000 × 20480 × 60) / (2500 × 2048 × 4) = 600   (verifica)
```

> El ejemplo del manual usa `QEILOAD = 2500` *crudo* en la cuenta (no `+1`). Es coherente porque el
> manual define `QEILOAD` como la duración directa de la ventana. El driver CMSIS, en cambio, guarda
> `QEILOAD = ventana − 1` y por eso `QEI_CalculateRPM` reusa `QEILOAD + 1`: las dos cuentas dan el mismo
> número, solo cambia dónde vive el `−1`. No mezcles las dos convenciones.

Si el motor sube a 3000 RPM, `QEICAP = 102 400` por ventana y la misma fórmula da 3000. El manual aclara
que en la realidad usarás un `PCLK` mucho más alto y probablemente un `QEILOAD` más grande.

### Velocidad baja: VELCOMP y VELC_Int

Después de cada captura, el hardware compara `QEICAP` contra `VELCOMP`. Si la **velocidad capturada es
menor** que el umbral, dispara `VELC_Int` (en el manual aparece también como *low velocity*, `LVEL_Int`).
Es la forma directa de detectar que el motor está **calado** o girando demasiado lento, sin tener que
calcular RPM en cada ventana. Cargás en `VELCOMP` el mínimo aceptable de flancos por ventana y dejás que
el QEI te avise.

## Filtro digital antirruido (FILTER)

Las tres entradas (PhA, PhB, IDX) pasan por un filtro digital. `FILTER` guarda un número de **clocks de
muestreo de PCLK**: para que una transición sea aceptada, la entrada tiene que quedarse en el nuevo estado
durante esa cantidad de clocks seguidos. Rebotes y glitches más cortos se descartan.

- `FILTER = 0` → **bypass**, sin filtro.
- Cuanto más grande, más inmune al ruido, pero introducís más retardo y, si te pasás, podés **perder
  flancos legítimos** a alta velocidad.

Cómo dimensionarlo: estimá el ancho del glitch que querés matar (`t_glitch`) y el período del flanco más
rápido del encoder a la RPM máxima (`t_flanco = 60 / (RPM_max × PPR × Edges)`). Elegí
`t_glitch < FILTER/PCLK < t_flanco`. O sea, el filtro tiene que ser más largo que el ruido pero más corto
que medio período de la señal más rápida. Si no tenés ruido medido, arrancá con un valor chico (unos
microsegundos) y subilo solo si ves flancos espurios.

## Interrupciones: todas las fuentes

`QEIINTSTAT` junta todas las fuentes en un solo vector NVIC (`QEI_IRQn`):

| Bit | Fuente | Significado |
|-----|--------|-------------|
| 0 | `INX_Int`  | Se detectó un pulso de index |
| 1 | `TIM_Int`  | Overflow del timer de velocidad (hay captura nueva en `QEICAP`) |
| 2 | `VELC_Int` | Velocidad capturada menor que `VELCOMP` (motor lento/calado) |
| 3 | `DIR_Int`  | Cambió el sentido de giro |
| 4 | `ERR_Int`  | Error de fase del encoder (transición ilegal de A/B) |
| 5 | `ENCLK_Int`| Se detectó un pulso de clock del encoder |
| 6–8 | `POS0_Int`..`POS2_Int` | `QEIPOS` igualó `CMPOS0/1/2` |
| 9 | `REV_Int`  | `INXCNT` igualó `INXCMP` (llegó a N vueltas) |
| 10–12 | `POS0REV_Int`..`POS2REV_Int` | Coinciden a la vez posición y vuelta |

`ERR_Int` es tu canario: si salta seguido, casi siempre es **ruido eléctrico** (subí el `FILTER`),
cableado de A/B flojo, o un encoder yendo más rápido de lo que el filtro deja pasar. `DIR_Int` te avisa
inversiones sin pollear `QEISTAT`.

## Con el driver CMSIS

El driver `lpc17xx_qei` configura todo con una estructura. Atención a los nombres exactos: la función de
defaults se llama **`QEI_ConfigStructInit`** (no existe ningún `QEI_GetCfgDefault`), y la captura de
velocidad se lee con **`QEI_GetVelocityCap`**.

```c
#include "lpc17xx_qei.h"
#include "lpc17xx_pinsel.h"

#define ENC_PPR    2048UL      // pulsos por vuelta de tu encoder

void qei_setup(void)
{
    PINSEL_CFG_Type pin;
    QEI_CFG_Type cfg;
    QEI_RELOADCFG_Type reload;

    // Pines P1.20=MCI0, P1.23=MCI1, P1.24=MCI2, función 1
    pin.Funcnum = 1; pin.OpenDrain = 0; pin.Pinmode = 0; pin.Portnum = 1;
    pin.Pinnum = 20; PINSEL_ConfigPin(&pin);
    pin.Pinnum = 23; PINSEL_ConfigPin(&pin);
    pin.Pinnum = 24; PINSEL_ConfigPin(&pin);

    QEI_ConfigStructInit(&cfg);            // 4X, cuadratura, sin invertir
    // cfg.CaptureMode  = QEI_CAPMODE_4X;  // o QEI_CAPMODE_2X
    // cfg.SignalMode   = QEI_SIGNALMODE_QUAD;
    // cfg.DirectionInvert = QEI_DIRINV_NONE;
    // cfg.InvertIndex  = QEI_INVINX_NONE;
    QEI_Init(LPC_QEI, &cfg);               // ya enciende PCONP y fija PCLK_QEI

    // Ventana de velocidad de 250 ms (en microsegundos)
    reload.ReloadOption = QEI_TIMERRELOAD_USVAL;
    reload.ReloadValue  = 250000;          // 250 ms
    QEI_SetTimerReload(LPC_QEI, &reload);

    // Interrupciones: overflow de velocidad y cambio de dirección
    QEI_IntCmd(LPC_QEI, QEI_INTFLAG_TIM_Int, ENABLE);
    QEI_IntCmd(LPC_QEI, QEI_INTFLAG_DIR_Int, ENABLE);
    NVIC_EnableIRQ(QEI_IRQn);
}

void QEI_IRQHandler(void)
{
    if (QEI_GetIntStatus(LPC_QEI, QEI_INTFLAG_TIM_Int) == SET) {
        uint32_t cap = QEI_GetVelocityCap(LPC_QEI);          // QEICAP
        uint32_t rpm = QEI_CalculateRPM(LPC_QEI, cap, ENC_PPR);
        (void)rpm;                                            // usar el valor
        QEI_IntClear(LPC_QEI, QEI_INTFLAG_TIM_Int);
    }
    if (QEI_GetIntStatus(LPC_QEI, QEI_INTFLAG_DIR_Int) == SET) {
        // QEI_GetStatus(LPC_QEI, QEI_STATUS_DIR) -> sentido actual
        QEI_IntClear(LPC_QEI, QEI_INTFLAG_DIR_Int);
    }
}
```

`QEI_CalculateRPM(LPC_QEI, QEICAP, PPR)` aplica exactamente la fórmula de arriba leyendo `PCLK`, `QEILOAD`
y `CAPMODE` por vos. El ejemplo oficial `QEI/QEI_Velo` hace justo esto: genera señales A/B "virtuales" con
un timer (no necesitás encoder físico para probarlo), acumula varias capturas y promedia para imprimir
RPM por UART.

## Cerrando el lazo con PWM

El uso típico: leés velocidad y/o posición con el QEI, las comparás con el *setpoint* y ajustás el **PWM**
(módulo 16) que maneja el driver del motor. Esa es la estructura de un **controlador PID** de motor. El QEI
te da el "ojo" (cuánto giró de verdad y a qué velocidad) y el PWM la "mano" (cuánta potencia entregar). El
`DIR_Int` te avisa inversiones, `VELC_Int` detecta el motor calado, y los `CMPOS` disparan eventos de
posición sin pollear.

## Casos de borde y errores típicos

- **PCLK mal asumido en la fórmula.** El error más común de RPM. El driver pone `PCLK_QEI = CCLK`; si
  cambiás el divisor en `PCLKSEL0`, ajustá la cuenta.
- **Confundir `QEIVEL` con `QEICAP`.** `QEIVEL` es la ventana en curso (cambia todo el tiempo); para
  velocidad usás `QEICAP`, que es la foto consolidada al overflow.
- **`QEIPOS` es sin signo.** En posiciones relativas que cruzan cero, casteá a `int32_t` o usá el wrap.
- **`ERR_Int` recurrente** = ruido o velocidad demasiado alta para el filtro. Revisá `FILTER`, cableado y
  blindaje.
- **Olvidarse del `PINMODE`/pull-up** del encoder (sobre todo si es open-collector): entradas flotando
  generan flancos fantasma.
- **Filtro demasiado largo:** mata el ruido pero también flancos reales a RPM alta → la posición "patina".
- **No setear `QEIMAXPOS`** en aplicaciones rotativas: la posición crece sin fin en vez de dar la vuelta.
- **Tocar `QEIIE` directo:** usá `QEIIES`/`QEIIEC` para prender/apagar y `QEICLR` para limpiar flags.

## Lo que te llevás

- El QEI decodifica A/B por hardware para saber **sentido** y **cantidad** de giro; `QEIPOS` se actualiza
  sola sin gastar CPU.
- Resolución: **2X** (`CAPMODE=0`, solo PhA) o **4X** (`CAPMODE=1`, ambas) → `2×PPR` o `4×PPR`. No hay 1X.
- Dos modos de señal: **cuadratura** (`SIGMODE=0`) o **clock/direction** (`SIGMODE=1`); `DIRINV` invierte
  el sentido sin re-cablear.
- Velocidad: timer de ventana (`QEILOAD`→`QEITIME`), `QEIVEL` cuenta en vivo, `QEICAP` consolida al
  overflow, y `RPM = QEICAP × 60 × PCLK / (QEILOAD × PPR × Edges)`.
- `VELCOMP` detecta motor calado; `CMPOS0/1/2` e `INXCMP` disparan eventos de posición y de vueltas; el
  `FILTER` te protege del ruido si lo dimensionás bien.
- Combinado con **PWM**, es la base del control de motores con realimentación (PID).

## Referencias
- Manual, Cap. 26: [`../../manual/ch26_quadrature-encoder-interface.pdf`](../../manual/ch26_quadrature-encoder-interface.pdf)
- Driver: [`../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_qei.h`](../../library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_qei.h)
- Ejemplos: [`../../library/examples/QEI/QEI_Velo/`](../../library/examples/QEI/QEI_Velo/)

---

**Anterior:** [02 - Watchdog](./02-watchdog.md) · **Módulo:** [Periféricos adicionales](./README.md) ·
**Siguiente:** [04 - CAN](./04-can.md)
