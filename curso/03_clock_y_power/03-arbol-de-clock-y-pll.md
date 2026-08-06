# El árbol de clock y la PLL0 (cómo llegamos a 100 MHz)

> Esta página es **más avanzada** y, en la práctica, lo hace `SystemInit()` por vos. Pero entender de
> dónde sale `CCLK` te saca dudas, te deja calcular cualquier frecuencia, y te permite cambiar el
> clock si alguna vez lo necesitás (más lento para ahorrar, o a 120 MHz para exprimir el LPC1769).

## El problema

El LPC1769 puede correr hasta 120 MHz, pero todas las fuentes de reloj disponibles son lentas. Hay
**tres osciladores** en el chip:

| Oscilador | Frecuencia | Precisión | Para qué |
|-----------|-----------|-----------|----------|
| **IRC** (RC interno) | **4 MHz** | ~±1% (impreciso) | arranque y fallback; no sirve para USB |
| **Main** (cristal externo) | **1–25 MHz** (en la placa: **12 MHz**) | exacta (cristal) | la fuente "buena" para correr rápido |
| **RTC** | **32.768 kHz** | muy estable | reloj de tiempo real; también puede ser fuente de la PLL0 (no si el USB cuelga de la PLL0) |

¿Cómo pasamos de un cristal de 12 MHz a un CPU de 100 MHz? Con una **PLL** (Phase-Locked Loop), un
circuito que **multiplica** la frecuencia enganchándose en fase a la entrada.

## El árbol de clock completo

```
                      ┌─ IRC 4 MHz ──────────┐
   Cristal 12 MHz ────┤                       │   CLKSRCSEL
    (Main osc) ───────┤─ Main osc ───────────┼──▶ (elige fuente)
                      └─ RTC 32.768 kHz ──────┘        │
                                                       ▼
                                        ┌──────────────────────────┐
                                        │  PLL0:  Fcco = 2·M·Fin/N │
                                        │  (Fcco entre 275–550 MHz)│
                                        └──────────────────────────┘
                                                       │ Fcco = 400 MHz
                                                       ▼
                                          [ ÷ CCLKCFG ]  (÷4)
                                                       │
                                                       ▼
                                              CCLK = 100 MHz  ◀── reloj del CPU
                                                       │
                                          [ ÷ PCLKSEL ] por periférico
                                                       │
                                                       ▼
                                          PCLK (25 MHz por defecto)
```

La fuente para la PLL0 (y, si la PLL está apagada, para el CPU directo) la elige **`CLKSRCSEL`**
(`0x400FC10C`): `0`=IRC, `1`=Main oscillator, `2`=RTC. En la placa usamos `1` (el cristal de 12 MHz).

## Paso 1: arrancar el oscilador principal (SCS)

El cristal externo no está habilitado tras reset (el chip arranca con el IRC). Para usar el cristal
hay que tocar el registro **`SCS`** (System Controls and Status, `0x400FC1A0`):

| Bit | Símbolo | Qué hace |
|----:|---------|----------|
| 4 | `OSCRANGE` | rango del oscilador: `0` = 1–20 MHz, `1` = 15–25 MHz |
| 5 | `OSCEN`    | `1` = habilita el oscilador principal |
| 6 | `OSCSTAT`  | (solo lectura) `1` cuando el cristal ya está estable y se puede usar |

Como nuestro cristal es de **12 MHz**, entra en el rango bajo → `OSCRANGE = 0`. La secuencia es:
habilitar (`OSCEN`) y **esperar** a que `OSCSTAT` se ponga en 1. El cristal tarda en estabilizarse
(el oscilador cuenta unos 4096 ciclos internamente antes de levantar `OSCSTAT`); arrancar la PLL
antes de que esté estable es pedir problemas.

```c
LPC_SC->SCS = (1u << 5);                 // OSCRANGE=0 (12 MHz), OSCEN=1
while ((LPC_SC->SCS & (1u << 6)) == 0);  // esperar OSCSTAT
```

> Si tu cristal fuera de más de 20 MHz (p. ej. un módulo de 24 MHz), tendrías que poner
> `OSCRANGE = 1`. Poner mal el rango hace que el oscilador no enganche o sea inestable. Con 12 MHz,
> rango bajo, y listo.

## Paso 2: la PLL0 y la fórmula `Fcco = 2·M·Fin/N`

La PLL0 toma la entrada `Fin` (los 12 MHz del cristal) y genera internamente una frecuencia alta
`Fcco` (Current Controlled Oscillator) con un multiplicador **M** y un pre-divisor **N**:

```
Fcco = (2 × M × Fin) / N
```

Hay **dos restricciones de hardware** que no podés violar:

1. **`Fin` entre 32 kHz y 50 MHz** (los 12 MHz cumplen de sobra).
2. **`Fcco` entre 275 MHz y 550 MHz.** El CCO físico solo es estable en esa ventana. Esta es la
   restricción que más se olvida.

¿Cómo se eligen M y N para llegar a **100 MHz de CPU** desde **12 MHz**?

- Necesitás que después del divisor de CPU quede 100 MHz. Probemos con `Fcco = 400 MHz` y un divisor
  de CPU de 4: `400 / 4 = 100`. Sirve.
- Con `N = 6` (pre-divide 12 MHz a 2 MHz de referencia) y `M = 100`:

  `Fcco = (2 × 100 × 12 MHz) / 6 = 400 MHz` → cae dentro de 275–550 MHz. **Sirve.**

Estos son exactamente los valores que usa CMSIS. En el registro **`PLL0CFG`** (`0x400FC084`), M va en
los bits 14:0 **menos 1**, y N en los bits 23:16 **menos 1**:

```
M = 100 → campo MSEL = 99 = 0x63
N = 6   → campo NSEL = 5  = 0x05
PLL0CFG = (5 << 16) | 0x63 = 0x00050063   <- PLL0CFG_Val en system_LPC17xx.c
```

> **El "−1" que arruina parciales:** los campos guardan el valor *menos uno*. Si querés M=100 escribís
> 99. Equivocarte acá te da una frecuencia distinta sin error visible. La fórmula del header CMSIS lo
> refleja: `__M = (PLL0CFG & 0x7FFF) + 1`, `__N = ((PLL0CFG>>16)&0xFF) + 1`.

### Cómo elegir M y N para otra frecuencia

El truco general: despejá `M = (Fcco × N) / (2 × Fin)` y elegí `N` chico (típicamente 1 o el que dé
una referencia razonable) y un `Fcco` que caiga en 275–550 MHz **y** que dividido por un entero te dé
el CCLK que querés. Para 120 MHz, por ejemplo, `Fcco = 480 MHz` (en rango) con divisor de CPU 4.

## Paso 3: el divisor de CPU (CCLKCFG)

`Fcco` es demasiado rápido para el CPU. El registro **`CCLKCFG`** (`0x400FC104`) lo divide:

```
CCLK = Fcco / (CCLKCFG + 1)
```

CMSIS usa `CCLKCFG_Val = 0x03` → divide por **4** → `400 / 4 = 100 MHz`. Hay una regla del hardware
(Tabla 38 del manual): **con la PLL0 conectada no está permitido dividir por 1 ni por 2**: como
`Fcco` es al menos 275 MHz, hasta `Fcco/2` quedaría por encima de los 120 MHz máximos del CPU. La
división mínima es **por 3**, o sea `CCLKCFG ≥ 2`. `system_LPC17xx.c` hasta tiene un `#error` que
exige `CCLKCFG_Val ≥ 2` cuando se usa la PLL0.

## La secuencia EXACTA de encendido de la PLL0 (con el feed)

Acá está el corazón de la página. La PLL **no se activa con solo escribir sus registros**: cada
cambio en `PLL0CON` o `PLL0CFG` recién surte efecto cuando escribís una **secuencia de alimentación
("feed")**: `0xAA` seguido de `0x55` en `PLL0FEED` (`0x400FC08C`), **sin nada en el medio**.

Es una protección de hardware: si un programa colgado escribe basura en los registros de la PLL, sin
la secuencia mágica exacta el cambio no se aplica, y el reloj del sistema no se rompe por accidente.

El orden completo (tal cual lo hace `SystemInit`) es:

```c
// (pre-requisitos: oscilador estable, CLKSRCSEL ya elegido, CCLKCFG y PCLKSEL ya seteados)

LPC_SC->PLL0CFG = 0x00050063;   // 1) M=100, N=6  (con la PLL TODAVÍA apagada)
LPC_SC->PLL0FEED = 0xAA;        //    feed para que tome la config
LPC_SC->PLL0FEED = 0x55;

LPC_SC->PLL0CON = 0x01;         // 2) PLLE=1: ENABLE (todavía NO conectada al CPU)
LPC_SC->PLL0FEED = 0xAA;
LPC_SC->PLL0FEED = 0x55;

while (!(LPC_SC->PLL0STAT & (1u << 26)));  // 3) esperar PLOCK0 (la PLL "enganchó")

LPC_SC->PLL0CON = 0x03;         // 4) PLLE=1 y PLLC=1: ENABLE + CONNECT
LPC_SC->PLL0FEED = 0xAA;
LPC_SC->PLL0FEED = 0x55;

// (CPU ahora corre desde la PLL0)
```

**Por qué este orden y no otro:**

1. **Configurás M/N con la PLL apagada.** No tiene sentido (ni es válido) cambiar el multiplicador
   con la PLL ya conectada alimentando el CPU: te quedarías sin clock a mitad de camino.
2. **Enable sin connect (`PLL0CON = 0x01`).** Primero la prendés pero **no** la enganchás al CPU. La
   PLL empieza a oscilar, pero el CPU sigue corriendo con la fuente vieja (el cristal). Esto es a
   propósito: la PLL todavía no es confiable, su salida está variando mientras "busca" el lock.
3. **Esperás `PLOCK0`** (bit 26 de `PLL0STAT`, `0x400FC088`). Ese bit se pone en 1 cuando la PLL
   **enganchó** y su salida es estable a la frecuencia objetivo. **Conectar antes del lock es el
   error más peligroso:** alimentarías el CPU con un reloj que aún está variando → cuelgue o reset.
4. **Connect (`PLL0CON = 0x03`).** Recién ahora ponés `PLLC=1` para que `Fcco/CCLKCFG` pase a ser el
   reloj del CPU. A partir de acá, el CPU corre a 100 MHz.

Cada uno de esos cambios de `PLL0CON` necesita **su propio feed** (`0xAA`,`0x55`). Por eso ves la
secuencia repetida.

> **Detalle de seguridad fino:** el feed tiene que ser dos escrituras *consecutivas*. El manual es
> más estricto de lo que parece: entre el `0xAA` y el `0x55` **no puede haber ningún otro acceso a
> registros del bloque System Control** (todo el espacio `0x400FC000`–`0x400FFFFF`, donde viven
> PCONP, PCLKSEL, SCS, etc.). Si una interrupción se mete en el medio y toca cualquiera de esos
> registros, el feed se invalida y el cambio no se aplica. Por eso este código corre **muy
> temprano**, antes de habilitar interrupciones. Si alguna vez reconfigurás la PLL en caliente,
> deshabilitá interrupciones alrededor del feed (`__disable_irq()` / `__enable_irq()`).

### Cambiar la frecuencia "en caliente": la secuencia segura completa

La secuencia de arriba es la que usa `SystemInit` y funciona porque, **tras reset, la PLL0 ya está
apagada y desconectada** (arranca del IRC): por eso `SystemInit` puede ir directo a configurar y
enganchar. Pero si querés **cambiar la frecuencia con el micro ya corriendo desde la PLL0** (subir a
120 MHz, o bajar para ahorrar), no podés saltar pasos: el manual da una secuencia de 9 pasos
(sección "PLL0 setup sequence") que hay que respetar al pie de la letra. En orden:

1. **Desconectá** la PLL0 (un feed). El CPU vuelve a correr de la fuente directa (cristal/IRC).
2. **Deshabilitá** la PLL0 (un feed). *No se puede tocar `PLL0CFG` con la PLL habilitada.*
3. (Opcional) ajustá `CCLKCFG` para correr más rápido **sin** PLL mientras reconfigurás.
4. Cambiá `CLKSRCSEL` si hace falta otra fuente.
5. Escribí el nuevo `PLL0CFG` (M/N) y aplicalo con **un** feed.
6. **Habilitá** la PLL0 (un feed), todavía sin conectar.
7. **Poné `CCLKCFG` con el divisor de la frecuencia final.** El manual marca esto como *crítico*:
   tenés que dejar el divisor correcto **antes** de conectar, para que en el instante en que conectás
   el CPU ya quede a la frecuencia buena y no a una intermedia ilegal.
8. **Esperá `PLOCK0`.**
9. **Conectá** la PLL0 (un feed).

> **Regla del manual, palabra por palabra:** *"es muy importante no fusionar ningún paso. Por ejemplo,
> no actualices `PLL0CFG` y habilites la PLL en el mismo feed."* Cada cambio de `PLL0CON`/`PLL0CFG`
> lleva su propio `0xAA`/`0x55`.

**¿Y el flash en todo esto?** Acordate del orden de los wait states (página
[04](./04-flash-accelerator.md)): **al SUBIR** la frecuencia, configurá el flash para la frecuencia
ALTA *antes* de conectar la PLL (paso 9); **al BAJAR**, primero conectás a la frecuencia baja y
recién después podés relajar los wait states. Así nunca hay un instante con `CCLK` alto y wait states
de menos (que cuelga el micro al leer instrucciones corruptas de la flash).

> Sobre `PLOCK0` en el paso 8: el bit puede no ser confiable si la frecuencia de referencia (`FREF =
> Fin/N`) es menor a 100 kHz o mayor a 20 MHz. En esos casos el manual recomienda esperar un tiempo
> fijo (500 µs si `FREF > 400 kHz`) en vez de mirar el bit. Con nuestro cristal de 12 MHz y N=6,
> `FREF = 2 MHz`: cae en zona buena, así que el `while (PLOCK0)` es confiable.

### Los bits de PLL0STAT que vale la pena conocer

| Bits | Significado |
|------|-------------|
| 14:0  | MSEL actual (M−1) |
| 23:16 | NSEL actual (N−1) |
| 24 | `PLLE0_STAT`: PLL habilitada |
| 25 | `PLLC0_STAT`: PLL conectada (es la fuente del CPU) |
| 26 | `PLOCK0`: PLL enganchada y estable |

`SystemCoreClockUpdate()` lee justamente estos bits para recalcular `SystemCoreClock` en runtime, sin
asumir nada.

## ¿Quién hace todo esto? `SystemInit()`

No tenés que programar la PLL a mano en cada proyecto. Cuando el micro arranca, **antes** de tu
`main()`, el startup de CMSIS llama a `SystemInit()` (en
[`library/CMSISv2p00_LPC17xx/src/system_LPC17xx.c`](../../library/CMSISv2p00_LPC17xx/src/system_LPC17xx.c)),
que hace, en este orden:

1. Habilita el oscilador del cristal (`SCS`) y espera `OSCSTAT`.
2. Setea el divisor de CPU (`CCLKCFG`) y los `PCLKSEL`.
3. Elige la fuente con `CLKSRCSEL`.
4. Configura y engancha la PLL0 con la secuencia de feed de arriba (espera `PLOCK0`, conecta).
5. (Opcional) configura la PLL1 para el USB.
6. Setea `PCONP` (qué periféricos quedan encendidos) y `CLKOUTCFG`.
7. **Ajusta el flash accelerator (`FLASHCFG`)** para los wait states de 100 MHz (ver la
   [página 04](./04-flash-accelerator.md)).

Además, la variable global `SystemCoreClock` queda con el valor de `CCLK` en Hz (`100000000`): en
esta versión de CMSIS viene calculada en tiempo de compilación a partir de los mismos `#define`, y
si tocás el clock en runtime la recalculás llamando a `SystemCoreClockUpdate()`.

> Por eso, cuando llegás a `main()`, **el micro ya corre a 100 MHz**, el flash ya está bien
> configurado, y podés leer `SystemCoreClock` para saber tu frecuencia en vez de asumirla.

## Errores típicos con la PLL

- **Conectar antes del lock:** saltarte el `while (PLOCK0)`. El CPU recibe un reloj inestable → cuelga.
- **`Fcco` fuera de 275–550 MHz:** elegir M/N que dan, por ejemplo, 200 MHz de Fcco. La PLL no
  engancha o lo hace de forma inestable; `PLOCK0` puede no levantarse nunca y el `while` queda
  colgado para siempre.
- **Olvidar el feed** (o que una interrupción lo parta): escribís `PLL0CON`/`PLL0CFG` y "no pasa
  nada". El registro no toma el valor sin la secuencia `0xAA`,`0x55` intacta.
- **El "−1" de M y N:** poner 100 en vez de 99 en MSEL da otra frecuencia, sin avisar.
- **Cambiar M/N con la PLL ya conectada:** te quedás sin clock a mitad de la operación. Desconectá
  primero (volvé a la fuente directa), reconfigurá, re-enganchá.

## ¿Cuándo tocarías esto?

Casi nunca en la materia: `SystemInit()` te deja a 100 MHz, que está bien para todo. Pero podrías
querer cambiarlo para:
- **bajar consumo** (correr más lento a propósito, subiendo el `CCLKCFG`),
- **alimentar el USB** (necesita exactamente 48 MHz; sale de la PLL1 o de dividir la PLL0, ver
  [página 05](./05-usb-clock-y-clkout.md)),
- **maximizar performance** subiendo a 120 MHz (recordá entonces el flash a 6 wait states o 5 para el
  '69; ver [página 04](./04-flash-accelerator.md)).

Lo importante es que entiendas el árbol: **oscilador → CLKSRCSEL → PLL0 (×M/÷N → Fcco) → ÷CCLKCFG →
CCLK → ÷PCLKSEL → PCLK**. Con eso, ninguna frecuencia del micro es un misterio.

---

**Anterior:** [02 - Clock de periféricos: PCLKSEL](./02-clock-pclksel.md) ·
**Siguiente:** [04 - El flash accelerator y los wait states](./04-flash-accelerator.md)
