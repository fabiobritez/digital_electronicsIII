# NVIC, el modelo de excepciones y la tabla de vectores

Una interrupción es el hardware diciéndote "pará lo que estás haciendo, pasó algo". El bloque que
coordina todo eso en el Cortex-M3 es el **NVIC** (Nested Vectored Interrupt Controller). Las tres
palabras del nombre cuentan la historia:

- **Nested** (anidado): una interrupción más urgente puede interrumpir a otra que ya se está
  atendiendo.
- **Vectored** (vectorizado): cada fuente salta **directo** a su handler vía una tabla de
  direcciones; no hay un único handler genérico que después pregunte "¿quién fue?". Eso baja la
  latencia.
- **Interrupt Controller**: habilita, prioriza y enruta las interrupciones hacia el CPU.

El NVIC está **integrado en el núcleo** (no es un periférico de NXP): vive en la región del System
Control Space, en `0xE000E000`. Por eso es idéntico en cualquier Cortex-M3, y por eso CMSIS lo maneja
con las funciones `NVIC_*` (las vemos abajo). NXP solo decide **cuántas** IRQs conecta y **cuántos
bits de prioridad** implementa.

## El modelo de excepciones: excepciones del sistema vs IRQs

Para el Cortex-M3, una interrupción es un caso particular de algo más general: una **excepción**.
Cada excepción tiene un **número de excepción** y, salvo el Reset, un **handler**. Hay dos familias:

**Excepciones del sistema** (del núcleo, número de IRQ **negativo** en CMSIS):

| Excepción | Nº exc. | IRQn (CMSIS) | Qué es |
|-----------|--------:|-------------:|--------|
| Reset | 1 | (no tiene) | Arranque. Prioridad fijísima. |
| NMI | 2 | `NonMaskableInt_IRQn` = −14 | No enmascarable. En el LPC va al pin P2.10/EINT0. Prioridad fija −2. |
| HardFault | 3 | (no tiene; `-13`) | Falla grave (o falla durante otra excepción). Prioridad fija −1. |
| MemManage | 4 | `MemoryManagement_IRQn` = −12 | Violación de la MPU. |
| BusFault | 5 | `BusFault_IRQn` = −11 | Error de bus (acceso a memoria inválido). |
| UsageFault | 6 | `UsageFault_IRQn` = −10 | Instrucción/estado ilegal (y, si se habilita el trap, división por cero). |
| SVCall | 11 | `SVCall_IRQn` = −5 | `SVC`: llamada a supervisor (usado por RTOS). |
| DebugMonitor | 12 | `DebugMonitor_IRQn` = −4 | Debug. |
| PendSV | 14 | `PendSV_IRQn` = −2 | Excepción "pedible por software", la usa el RTOS para context switch. |
| SysTick | 15 | `SysTick_IRQn` = −1 | El timer del sistema (módulo 6). |

**IRQs del NVIC** (periféricos de NXP, número **positivo** 0..N):

| IRQn | Handler | IRQn | Handler |
|----:|---------|----:|---------|
| 0 | `WDT_IRQHandler` | 17 | `RTC_IRQHandler` |
| 1 | `TIMER0_IRQHandler` | 18 | `EINT0_IRQHandler` |
| 2 | `TIMER1_IRQHandler` | 19 | `EINT1_IRQHandler` |
| 3 | `TIMER2_IRQHandler` | 20 | `EINT2_IRQHandler` |
| 4 | `TIMER3_IRQHandler` | 21 | `EINT3_IRQHandler` (¡y GPIO!) |
| 5 | `UART0_IRQHandler` | 22 | `ADC_IRQHandler` |
| 6 | `UART1_IRQHandler` | 23 | `BOD_IRQHandler` |
| ... | ... | 26 | `DMA_IRQHandler` |

El LPC176x soporta **35 interrupciones vectorizadas** (IRQ 0 a 34). La lista completa con sus números
está en el `enum IRQn_Type` de `LPC17xx.h`; los nombres de los handlers, en el startup de CMSIS.

> ¿Por qué números negativos para las del sistema? Porque el **número de excepción** del núcleo
> (1=Reset, 2=NMI, …, 15=SysTick, 16=primera IRQ) es una cosa, y el **IRQn** que usa CMSIS es otra:
> `IRQn = nº_excepción − 16`. Así la primera IRQ de periférico (WDT, excepción 16) queda en
> `IRQn 0`, y todo lo del núcleo cae en negativo. Esto importa porque `NVIC_SetPriority` trata
> distinto a los dos grupos: las del sistema van a los registros `SCB->SHP`, las de periférico a
> `NVIC->IP`. La función ya decide cuál por el signo del IRQn; vos solo pasás el nombre.

## La tabla de vectores: ¿cómo sabe el CPU a qué función saltar?

La **tabla de vectores** es un arreglo al principio de la Flash. En cada posición hay la **dirección
del handler** de una excepción, ordenadas por número de excepción. Lo especial: la **primera**
entrada no es un handler sino el **valor inicial del stack pointer**.

```
Dirección   Contenido (tabla de vectores)
0x00000000  valor inicial del SP (MSP)      <- el núcleo lo carga en el reset
0x00000004  Reset_Handler                   <- acá arranca el micro
0x00000008  NMI_Handler
0x0000000C  HardFault_Handler
   ...
0x0000003C  SysTick_Handler                 <- excepción 15
0x00000040  WDT_IRQHandler                  <- IRQ 0  (excepción 16)
0x00000044  TIMER0_IRQHandler               <- IRQ 1
   ...
0x00000054  UART0_IRQHandler                <- IRQ 5
```

Cada entrada ocupa 4 bytes, así que la dirección del vector se calcula como
`(16 + IRQn) × 4`. El offset que ves en el manual (Timer0 en `0x44`) sale de ahí: `(16+1)×4 = 0x44`.

Cuando ocurre una excepción, el núcleo: (1) calcula el número de excepción, (2) **lee la dirección
del handler** de la tabla (un "vector fetch"), (3) salta ahí. Todo en hardware.

### VTOR: reubicar la tabla

Al resetear, la tabla está en `0x00000000`. Pero el registro **VTOR** (Vector Table Offset Register,
`SCB->VTOR`) permite **moverla**: a cualquier dirección del primer GB del mapa, alineada (en el
LPC176x/5x) a una frontera de 1 KB (256 words, dice el manual). Esto se usa para:

- Correr código desde RAM (bootloaders) y tener una tabla de vectores en RAM.
- Un bootloader que después salta a la app y le cede su propia tabla.

CMSIS lo deja en 0 por defecto. Si alguna vez tus interrupciones "no andan" después de un bootloader,
sospechá de VTOR mal seteado. El manual incluso advierte: si cambiás una entrada de la tabla en
caliente y después habilitás esa excepción, poné una instrucción `DMB` en el medio.

### Por qué el nombre del handler es sagrado

El startup de CMSIS llena la tabla con los nombres estándar (`SysTick_Handler`, `TIMER0_IRQHandler`,
`EINT3_IRQHandler`, …) declarados como **weak**. "Weak" significa: si vos definís una función con
**ese mismo nombre exacto**, la tuya pisa al default y queda enganchada en la tabla. Si no la definís,
queda el default (un bucle infinito).

```c
void TIMER0_IRQHandler(void) { ... }   // se engancha en la tabla por su NOMBRE
```

Por eso un error de tipeo (`Timer0_IRQHandler`, `TIM0_IRQHandler`) **no da error de compilación** y
simplemente tu handler nunca corre: el linker dejó el weak default. Es uno de los bugs más
frustrantes y silenciosos. (Conexión con el anexo A: el startup y la tabla los vas a ver de cerca
ahí.)

## El stacking automático: por qué un ISR es una función C normal

Esto es lo que permite escribir un handler como una función C cualquiera, sin
ensamblador, sin guardar registros a mano. Cuando el núcleo toma la excepción, **antes** de saltar al
handler **apila solo** ocho words en el stack actual (el "stack frame"):

```
   SP más alto
   ...
   xPSR          <- el program status (flags, etc.)
   PC            <- dirección de retorno (a dónde volver)
   LR            <- link register previo
   R12
   R3
   R2
   R1
   R0            <- SP apunta acá al entrar al handler
```

Esos son justamente los registros **caller-saved** del ABI de ARM: los que una función C puede pisar
libremente. Como el hardware ya los salvó, tu handler puede usar R0-R3/R12 sin romper nada, y al
volver el núcleo los restaura. Por eso:

- **No necesitás prólogo/epílogo especial.** `void XXX_IRQHandler(void)` alcanza.
- **El retorno es automático.** El núcleo escribe un valor especial (`EXC_RETURN`) en el LR al
  entrar; cuando tu handler hace `return` (un `BX LR` normal), ese valor le dice al núcleo "esto es
  un retorno de excepción", y desapila el frame. No hay una instrucción "return from interrupt"
  distinta.
- **En paralelo al apilado, el núcleo hace el vector fetch.** Apilar y buscar la dirección del
  handler ocurren a la vez: latencia mínima (12 ciclos típicos).

> Intuición que el datasheet no te grita: el stacking es lo que te **permite ignorar** todo el
> ensamblador. Pero también es lo que hace que cada interrupción cueste ~tiempo y ~stack. Si tu ISR
> llama a funciones que usan muchos registros o, peor, hace floating-point, el frame y el trabajo
> crecen. Mantené los handlers chicos también por esto.

### Tail-chaining y late-arrival: el núcleo no apila de gusto

El núcleo tiene dos optimizaciones de hardware que abaratan las interrupciones encadenadas. No tenés
que configurar nada (son automáticas), pero entenderlas explica por qué el Cortex-M3 es tan rápido
atendiendo ráfagas:

- **Tail-chaining (encadenado):** si al terminar un handler hay **otra IRQ pendiente**, el núcleo
  **no desapila** el frame para volver al `main` y volver a apilarlo enseguida. Saltea ese
  pop+push y salta **directo** de un handler al otro (solo re-hace el vector fetch). El manual lo
  resalta: *"Tail-chaining optimization also significantly reduces the overhead when switching from
  one ISR to another"*. Por eso varias IRQs seguidas cuestan mucho menos que la suma de sus entradas
  individuales.
- **Late-arrival (llegada tardía):** si mientras el núcleo todavía está **apilando** para entrar a
  una IRQ llega otra de **mayor** prioridad, aprovecha el mismo stacking ya en curso y atiende
  primero a la más urgente. No tira el trabajo hecho.

La consecuencia práctica: no necesitás "ahorrar" interrupciones por miedo a la latencia de entrada.
El núcleo ya optimiza el caso de muchas IRQs juntas.

## Habilitar una interrupción en el NVIC

Que el periférico genere la interrupción **no alcanza**. Hay **dos llaves** en serie:

1. La del **periférico** (ej. el timer: "interrumpí en el match").
2. La del **NVIC** (ej. "dejá pasar la IRQ del Timer0 al CPU").

A nivel registro, el NVIC habilita con `ISER` (Interrupt **Set**-Enable) y deshabilita con `ICER`
(Interrupt **Clear**-Enable). Son registros "write-1": escribís 1 en el bit de la IRQ para
habilitar/deshabilitar, y escribir 0 no hace nada (por eso podés tocar un bit sin un read-modify-write
y sin riesgo de carrera).

```c
NVIC->ISER[0] = (1u << 1);   // habilitar IRQ 1 (Timer0) a mano
```

Pero lo normal es CMSIS, que calcula registro y bit por vos y es legible:

```c
NVIC_EnableIRQ(TIMER0_IRQn);    // -> escribe ISER
NVIC_DisableIRQ(TIMER0_IRQn);   // -> escribe ICER
```

## Pending y active: el ciclo de vida de una interrupción

Cada excepción está en uno de estos estados:

- **Inactive:** ni pendiente ni atendiéndose.
- **Pending:** ya llegó el evento y espera ser atendida (porque hay algo de mayor prioridad
  corriendo, o porque está enmascarada).
- **Active:** se está ejecutando su handler.
- **Active and pending:** se está atendiendo **y** ya llegó otra del mismo origen esperando.

El NVIC tiene registros para **pending** independientes del enable: `ISPR` (set-pending), `ICPR`
(clear-pending). CMSIS:

```c
NVIC_SetPendingIRQ(EINT3_IRQn);    // forzar por software que quede pendiente -> se atenderá
NVIC_GetPendingIRQ(EINT3_IRQn);    // ¿está pendiente?
NVIC_ClearPendingIRQ(EINT3_IRQn);  // cancelar una pendiente antes de que se atienda
NVIC_GetActive(EINT3_IRQn);        // ¿se está ejecutando ahora?
```

`SetPendingIRQ` es útil para **disparar trabajo diferido**: desde un handler corto, levantás como
pendiente otra IRQ de baja prioridad que hace el trabajo pesado cuando el sistema esté libre (patrón
"bottom half"). (El NVIC también tiene el registro **STIR**: escribís el número de IRQ y queda
pendiente; es la vía alternativa a `ISPR` para generar interrupciones por software.)

Dos detalles de borde. Primero, una IRQ **deshabilitada igual puede quedar pendiente** (el manual
lo advierte): si el periférico interrumpió antes de que la habilites, va a dispararse apenas hagas
`NVIC_EnableIRQ`; por eso el patrón de abajo limpia pendientes viejas antes de habilitar. Segundo,
ojo: el flag de **pending del NVIC** y el flag de
**interrupción del periférico** (ej. `EXTINT`, los `IntStat` del GPIO) son **dos cosas distintas**.
Si limpiás uno y no el otro, podés reentrar. Lo normal es limpiar el del periférico en el handler; el
del NVIC se limpia solo al despachar.

## Prioridades a fondo

El número de prioridad decide **quién interrumpe a quién**. Tres reglas de base:

1. **Menor número = más urgente.** 0 es la prioridad más alta. (Suena al revés; pensalo como "puesto
   en la fila": el puesto 0 atiende primero.)
2. En el LPC176x hay **5 bits de prioridad implementados → 32 niveles (0..31)**.
3. Reset, NMI y HardFault tienen prioridades **fijas y negativas** (−3, −2, −1): siempre ganan a
   cualquier IRQ configurable.

### Los 5 bits y dónde viven

Cada IRQ tiene un byte de prioridad de 8 bits, pero **solo se implementan los 5 más significativos**
(bits [7:3]); los **3 menos significativos [2:0] no se usan** en el LPC176x y leen 0. Por eso hay 32
niveles y no 256. CMSIS abstrae esto con `__NVIC_PRIO_BITS`, que en `LPC17xx.h` vale **5**:

```c
#define __NVIC_PRIO_BITS  5     // en LPC17xx.h
```

`NVIC_SetPriority` toma tu número 0..31 y lo **corre a la izquierda** para alinearlo a los bits altos:

```c
// Lo que hace CMSIS por dentro (simplificado):
NVIC->IP[IRQn] = (priority << (8 - __NVIC_PRIO_BITS)) & 0xFF;   // << 3
```

Por eso **siempre pasá el número lógico 0..31**, no un valor ya corrido. Si escribís el registro a
mano, acordate del `<< 3`, o vas a setear prioridades equivocadas (un clásico error de borde).

```c
NVIC_SetPriority(TIMER0_IRQn, 0);    // máxima urgencia
NVIC_SetPriority(UART0_IRQn, 10);    // menos urgente
```

### Group priority vs subpriority y el PRIGROUP

Acá entra la parte fina. Esos 5 bits se pueden **partir** en dos campos con el registro **AIRCR**
(`SCB->AIRCR`), campo **PRIGROUP**:

- **Group priority (preemption priority):** la parte alta. **Es la única que decide preempción.**
  Una IRQ solo interrumpe a otra si su *group priority* es **estrictamente mayor** (número menor).
- **Subpriority:** la parte baja. **No preempta.** Solo desempata: si dos IRQs del mismo grupo están
  pendientes a la vez, se atiende primero la de subprioridad menor. Y si empatan también ahí, gana la
  de **IRQn más bajo**.

La tabla del PRIGROUP para el LPC (5 bits, [7:3]):

| PRIGROUP | Split [7:3] | Group bits | Subpriority bits | Niveles de preempción | Subniveles |
|:--------:|-------------|:----------:|:----------------:|:---------------------:|:----------:|
| 0..2 (b010) | `xxxxx` | [7:3] | ninguno | 32 | 1 |
| 3 (b011) | `xxxx.y` | [7:4] | [3] | 16 | 2 |
| 4 (b100) | `xxx.yy` | [7:5] | [4:3] | 8 | 4 |
| 5 (b101) | `xx.yyy` | [7:6] | [5:3] | 4 | 8 |
| 6 (b110) | `x.yyyy` | [7] | [6:3] | 2 | 16 |
| 7 (b111) | `.yyyyy` | ninguno | [7:3] | 1 | 32 |

Por defecto (el AIRCR resetea con PRIGROUP = 0: los 5 bits son group priority, sin subprioridad):
**los 32 niveles preemptan**, que es lo que querés el 95% del tiempo y lo más simple de razonar. Si necesitás subgrupos
(ej. "estas tres IRQs no se preemptan entre sí pero tienen un orden de atención"), configurás:

```c
NVIC_SetPriorityGrouping(5);   // 4 grupos de preempción, 8 subniveles c/u
uint32_t pg = NVIC_GetPriorityGrouping();
NVIC_SetPriority(UART0_IRQn, NVIC_EncodePriority(pg, /*preempt*/1, /*sub*/3));
```

`NVIC_EncodePriority(grouping, preempt, sub)` arma el número combinado correcto según el split;
`NVIC_DecodePriority` hace lo inverso. Usalos en vez de calcular bits a mano.

> Recomendación práctica para los parciales: **no toques el PRIGROUP.** Dejá los 32 niveles de
> preempción y asigná prioridades enteras con `NVIC_SetPriority`. El grouping fino casi solo aparece
> en RTOS.

### Preempción anidada: el "Nested" en acción

Si una IRQ de group priority alta llega mientras corre una de group priority baja, la baja **se
pausa** (su frame ya está en el stack), se atiende la alta, y al volver retoma la baja. Eso son
interrupciones **anidadas**, y por eso el stack crece: cada nivel anidado apila su propio frame.
Cuidado con anidar muchos niveles si tenés poco RAM.

### Inversión de prioridad: el problema que el datasheet no te explica

Un peligro real de un mal diseño de prioridades. Imaginá:

- Una ISR de **alta** prioridad que necesita un dato que el `main` (prioridad "ninguna", siempre
  preemptible) está actualizando dentro de una sección crítica (con las interrupciones apagadas).
- O dos ISRs que comparten un recurso protegido.

Si la tarea urgente queda **esperando** un recurso que tiene tomado una menos urgente, se "invirtió"
la prioridad: la urgente no avanza por culpa de la lenta. En un micro bare-metal el caso típico es
una **sección crítica demasiado larga en una ISR de baja prioridad o en el main**, que retrasa a una
ISR urgente. La defensa: secciones críticas cortísimas, no compartir estado, y prioridades pensadas.
Esto se conecta de lleno con la [página 03](./03-secciones-criticas-y-atomicidad.md).

### Enmascarar sin tocar prioridades: PRIMASK y BASEPRI

Dos registros del núcleo para bloquear interrupciones temporalmente:

- **PRIMASK:** un bit. `__disable_irq()` lo pone (bloquea **todas** las IRQs configurables; NMI y
  HardFault no). `__enable_irq()` lo limpia. Es el martillo de las secciones críticas (página 03).
- **BASEPRI:** un umbral. Bloquea solo las IRQs de prioridad **igual o menor** (número igual o mayor)
  al valor que le pongas, dejando pasar las más urgentes. Es el bisturí: `__set_BASEPRI(n)`. Ideal
  cuando querés proteger algo de las IRQs comunes pero **sin** cegar a una crítica. Ojo con la
  convención del registro: `BASEPRI = 0` **desactiva** el filtro (no bloquea nada); un valor distinto
  de cero es el umbral. Y otro ojo: como los `IP`, BASEPRI implementa **solo los bits [7:3]** en el
  LPC176x/5x, y `__set_BASEPRI` escribe el registro crudo: pasale el nivel ya corrido
  (`__set_BASEPRI(n << 3)`).
- **FAULTMASK:** como PRIMASK pero más drástico: bloquea **todo excepto el NMI** (incluido el
  HardFault). El núcleo lo limpia solo al salir de cualquier handler que no sea el del NMI, así que es
  para usos muy puntuales del manejo de fallas; en bare-metal casi nunca lo tocás. `__set_FAULTMASK(n)`.

## Anatomía de un handler

```c
volatile uint32_t cuentas = 0;

void TIMER0_IRQHandler(void) {
    // 1) limpiar la bandera del periférico, o reentrás para siempre
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

    // 2) hacer lo mínimo
    cuentas++;
}
```

Reglas de oro:

- **Cortita.** Hacé lo mínimo (incrementar, levantar una bandera, encolar) y dejá el trabajo pesado
  al `main`. Mientras estás en la ISR, las de igual o menor *group priority* esperan.
- **Limpiar la bandera del periférico** antes de salir, o apenas retornás **volvés a entrar** → bucle
  infinito. (SysTick con `SysTick_Config` es la excepción: su flag se autolimpia al leerse.) Ojo con
  el **delay de escritura**: en algunos periféricos, si limpiás el flag en la última línea del
  handler, la escritura puede no haber "llegado" al salir y el núcleo reentra. Limpiá temprano, o leé
  el registro después para forzar el orden.
- **`volatile` en lo compartido.** Toda variable que la ISR escribe y el `main` lee (o viceversa) va
  `volatile`. Y si los dos escriben, o el dato es > 32 bits, leé la [página 03](./03-secciones-criticas-y-atomicidad.md).

## El patrón completo para habilitar cualquier interrupción

1. Configurar el periférico para que **genere** la interrupción (su bit de int-enable).
2. **Limpiar** banderas pendientes viejas (del periférico y, si hace falta, `NVIC_ClearPendingIRQ`).
3. `NVIC_SetPriority(XXX_IRQn, prioridad);` (opcional pero recomendable).
4. `NVIC_EnableIRQ(XXX_IRQn);`
5. Definir `void XXX_IRQHandler(void)` con el **nombre exacto**, que limpia la bandera y atiende.

En la [próxima página](./02-eint-y-gpio.md) aplicamos esto al caso más usado: reaccionar al cambio de
un pin (un botón) por interrupción.

---

**Módulo:** [Interrupciones](./README.md) ·
**Siguiente:** [02 - EINT y GPIO](./02-eint-y-gpio.md)
