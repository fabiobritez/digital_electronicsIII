# Modos de bajo consumo

Hasta acá usamos "Power" para **encender** periféricos (`PCONP`). La otra cara de la moneda es
**ahorrar energía**: si tu dispositivo anda a batería, que dure días o meses depende de cuánto duerma
el micro cuando no tiene nada que hacer. El Cortex-M3 y el LPC1769 ofrecen varios **modos de bajo
consumo**.

## La idea: dormir hasta que pase algo

La mayoría de los programas embebidos pasan **casi todo el tiempo esperando** (un botón, un timer, un
dato). En vez de un `while(1)` que quema CPU sin parar, hacés que el micro **se duerma** y lo despierte
una **interrupción** cuando hay algo que hacer.

```c
while (1) {
    atender_eventos();   // procesar lo que haya
    __WFI();             // "Wait For Interrupt": dormir hasta la próxima interrupción
}
```

`__WFI()` (función de CMSIS) **detiene el CPU** hasta que llega cualquier interrupción. Mientras
duerme, consume mucho menos. Cuando la interrupción llega, se ejecuta el handler y el programa sigue
justo después del `__WFI()`. Es la forma más simple y efectiva de bajar el consumo, y casi no cambia
tu código.

## Los cuatro modos (de menos a más ahorro)

Cuanto más profundo el sueño, **más ahorrás** pero **más cosas se apagan** y más cuesta despertar:

| Modo | Qué sigue prendido | Cómo despierta | Consumo |
|------|--------------------|----------------|---------|
| **Sleep** | el clock del CPU se para, pero los periféricos siguen | cualquier interrupción | bajo |
| **Deep Sleep** | se para el clock principal; los periféricos sin clock (el IRC y el RTC siguen) | NMI, EINT, GPIO, RTC, BOD, WDT (si corre del IRC), actividad CAN/USB, Ethernet WoL | más bajo |
| **Power-down** | se apagan también el IRC y la Flash | igual que Deep Sleep pero **sin** el WDT (el IRC está parado) | muy bajo |
| **Deep Power-down** | casi todo apagado; solo el RTC y un poquito de RAM de respaldo | reset, o el wake-up del RTC | mínimo (µA) |

## Cómo se eligen (a registro)

El modo se selecciona con **dos cosas que trabajan juntas**:

1. El bit **`SLEEPDEEP`** del registro `SCR` del núcleo (`SCB->SCR`, bit 2): `0` = Sleep, `1` = uno de
   los modos profundos.
2. Los bits **`PM1:PM0`** del registro **`PCON`** del LPC (`LPC_SC->PCON`, dirección `0x400FC0C0`,
   bits 1 y 0): cuando `SLEEPDEEP=1`, eligen *cuál* de los modos profundos. La codificación exacta
   (manual, Tabla 45) es:

| `PM1:PM0` | Con `SLEEPDEEP=0` | Con `SLEEPDEEP=1` |
|:---------:|-------------------|-------------------|
| `00` | **Sleep** | **Deep Sleep** |
| `01` | Sleep | **Power-down** |
| `10` | – | **Reservado (no usar)** |
| `11` | – | **Deep Power-down** |

> Dos cosas clave de esa tabla: (1) **`SLEEPDEEP` manda primero**: si está en 0, sea cual sea
> `PM1:PM0`, entrás a Sleep. Recién con `SLEEPDEEP=1` los bits `PM` deciden. (2) **`10` está
> reservado**: nunca pongas `PM1=1, PM0=0`. Para Deep Power-down van **los dos bits en 1** (`11`),
> no solo `PM1`. Es el error más fácil de cometer si suponés "un bit por modo".

```c
#include <LPC17xx.h>

void entrar_sleep(void) {
    SCB->SCR &= ~(1u << 2);     // SLEEPDEEP = 0 -> Sleep simple
    __WFI();                    // dormir
}

void entrar_deep_sleep(void) {
    SCB->SCR |= (1u << 2);                          // SLEEPDEEP = 1
    LPC_SC->PCON = (LPC_SC->PCON & ~0x3u) | 0x0u;   // PM1:PM0 = 00 -> Deep Sleep
    __WFI();
}

void entrar_power_down(void) {
    SCB->SCR |= (1u << 2);
    LPC_SC->PCON = (LPC_SC->PCON & ~0x3u) | 0x1u;   // PM1:PM0 = 01 -> Power-down
    __WFI();
}

void entrar_deep_power_down(void) {
    SCB->SCR |= (1u << 2);
    LPC_SC->PCON = (LPC_SC->PCON & ~0x3u) | 0x3u;   // PM1:PM0 = 11 -> Deep Power-down
    __WFI();
}
```

> El driver CMSIS `lpc17xx_clkpwr` también ofrece `CLKPWR_Sleep()`, `CLKPWR_DeepSleep()`,
> `CLKPWR_PowerDown()` que envuelven esto.

> **Las flags de PCON (para depurar):** además de los bits de control, `PCON` tiene cuatro **flags de
> solo escritura-1-para-borrar** que te dicen de qué modo venís al despertar: `SMFLAG` (bit 8, Sleep),
> `DSFLAG` (9, Deep Sleep), `PDFLAG` (10, Power-down) y `DPDFLAG` (11, Deep Power-down). El hardware
> setea la que corresponde al entrar al modo; tu código las lee y las limpia escribiendo un 1. Sirven
> para distinguir un arranque "normal" de uno "post-wake-up". (Hay también un bit `BODRPM` para apagar
> el Brown-Out Detect y ahorrar todavía un poco más en Power-down / Deep Sleep, a costa de no poder
> usar el BOD como fuente de wake-up.)

## Cómo despertar

Lo clave: **antes de dormir, configurá una interrupción que pueda despertarte**. Si dormís sin una
fuente de wake-up válida para ese modo, el micro **no se despierta más** (salvo reset). Fuentes
típicas:

- **EINT0–3** (módulo 7): un botón externo despierta de casi cualquier modo. Es la fuente de wake-up
  más usada.
- **RTC** (alarma): "despertame dentro de 10 minutos". Ideal para dispositivos que miden cada tanto.
- **Watchdog** (solo hasta Deep Sleep, y clockeado por el IRC): para despertar periódicamente.

> **Detalle del capítulo 3 que muerde:** si despertás por EINTx, tu código post-wake-up tiene que
> **limpiar el flag en `EXTINT`** (escribiendo un 1 en su bit). Si lo dejás en 1, el próximo intento
> de entrar a Power-down **falla**: el micro "no se duerme" y nadie te avisa por qué.

> En Deep Sleep y más profundos, los periféricos normales (timers, UART) **no** pueden despertarte
> porque su clock está apagado. Por eso la lista de fuentes se reduce a las que tienen clock propio
> (RTC) o son asíncronas (EINT, GPIO, BOD, actividad de CAN/USB). Detalle fino: en Deep Sleep, el
> **watchdog** sí puede despertarte **si está clockeado por el IRC** (que sigue vivo en Deep Sleep).

### El gotcha más grande: reconfigurar el clock al despertar

Esto el manual lo subraya y casi todos lo aprenden a los golpes. Cuando entrás a **Deep Sleep,
Power-down o Deep Power-down** (todos los modos profundos, no solo Power-down), el hardware
**apaga las PLL y las desconecta**, y además **resetea a cero los divisores `CCLKCFG` y
`USBCLKCFG`**. Al despertar no volvés a tus 100 MHz: tu PLL0 ya no está. Si tu código asume que
sigue a 100 MHz (baudrates, delays, etc.), todo va a estar **mal** después del wake-up. La diferencia
entre modos es de dónde salís corriendo:

- **De Deep Sleep:** el hardware rearranca solo el oscilador que usabas (si era el cristal, espera
  los 4096 ciclos de estabilización) y salís corriendo de esa fuente directa: con el cristal de la
  placa, a 12 MHz.
- **De Power-down / Deep Power-down:** salís corriendo del **IRC a 4 MHz** (los osciladores se
  apagaron).

Por eso, **lo primero que tiene que hacer tu rutina de wake-up de un modo profundo es volver a
levantar el oscilador (si hace falta), reconfigurar la PLL0 y los divisores** (lo de la
[página 03](./03-arbol-de-clock-y-pll.md)) y, si lo bajaste, el flash. Y con la secuencia
**completa**: el manual advierte que no intentes "revivir" la PLL con un simple feed al despertar:
eso la habilitaría y conectaría a la vez, antes del lock. En la práctica: encapsulá la secuencia
de `SystemInit` (o llamá a una función equivalente) apenas salís del modo profundo, antes de tocar
cualquier periférico. El único modo que se salva es **Sleep**: ahí la PLL sigue prendida y todo
continúa como estaba.

## Estrategias de ahorro, combinadas

1. **Apagá lo que no usás:** borrá en `PCONP` los periféricos inactivos
   ([página 01](./01-power-pconp.md)). Cada bloque encendido consume.
2. **Bajá la frecuencia:** correr a 100 MHz cuando alcanza con 12 MHz es gastar de más. Se puede
   ajustar el divisor de `CCLK` ([página 03](./03-arbol-de-clock-y-pll.md)); si bajás mucho, podés
   también relajar los wait states del flash ([página 04](./04-flash-accelerator.md)).
3. **Dormí entre tareas:** el patrón `__WFI()` del principio. Es lo de mayor impacto y lo más fácil.
4. **Elegí el modo más profundo posible** según qué necesites que siga vivo y qué te tenga que
   despertar.

## El trade-off

Más ahorro = más tiempo y complejidad para despertar y recuperar el estado:

- **Sleep** es casi gratis (una línea, despierta con cualquier interrupción) y ya ahorra bastante.
  Para la mayoría de los proyectos, **alcanza**.
- **Power-down / Deep Power-down** ahorran muchísimo pero despertar es más lento (hay que re-arrancar
  el oscilador, la PLL…) y se pierde más estado. Se usan en dispositivos que duermen mucho y despiertan
  poco (un sensor que mide una vez por hora).

> Regla práctica para la materia: empezá con el patrón `atender_eventos(); __WFI();` (Sleep). Es
> simple, no rompe nada, y ya es una mejora enorme respecto al `while(1)` que quema CPU. Los modos
> profundos, cuando el consumo sea crítico de verdad.

## Ejercicios
1. Tomá un proyecto con SysTick (módulo 6) y agregá `__WFI()` en el bucle principal. El SysTick lo
   despierta cada 1 ms; comprobá que sigue funcionando igual pero "duerme" entre ticks.
2. Hacé que un botón en EINT0 despierte al micro de Power-down (prendé un LED al despertar).
3. Investigá el consumo en cada modo en el datasheet del LPC1769 y armá una tabla comparativa.

## Cierre del módulo

Ya tenés las bases que faltaban para arrancar con periféricos:
- **PCONP** para encenderlos (y apagar lo que no usás),
- **PCLKSEL / el árbol de clock** para saber a qué frecuencia corren,
- el **flash accelerator** y la **PLL** para entender de dónde sale (y cómo se ajusta) esa frecuencia,
- y los **modos de bajo consumo** para gastar lo justo.

De acá en más, cada periférico va a seguir el mismo guion: *encenderlo (PCONP) → clockearlo (PCLKSEL)
→ conectar sus pines (PINSEL) → configurarlo → usarlo*. El próximo módulo es justamente **PINSEL**.

---

**Anterior:** [05 - Clock del USB (PLL1) y CLKOUT](./05-usb-clock-y-clkout.md) ·
**Módulo:** [Clock y Power](./README.md) ·
**Siguiente módulo:** [04 - PINSEL](../04_pinsel/)
