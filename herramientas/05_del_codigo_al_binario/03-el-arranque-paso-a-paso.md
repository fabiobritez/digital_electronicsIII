# El arranque paso a paso: de 0 V a `main()`

Cuando se energiza el LPC1769, el hardware estabiliza la alimentación y el clock, ejecuta la
boot ROM y decide si transfiere el control a la aplicación. En el caso normal, la secuencia
tarda del orden de cientos de microsegundos; si debe evaluar la entrada a ISP puede tardar
más.

Este capítulo recorre la secuencia en orden. Los tiempos de referencia salen de los capítulos
3, 4, 5 y 32 del UM10360. Los valores de registros indicados como mediciones corresponden a
la placa y al firmware concretos usados en esta guía.

## La línea de tiempo completa

```
   VDD
    ^                    ┌──────────────────────────────────────────
  3.3│              ┌────┘
    │           ┌──┘
    │        ┌─┘  ← umbral válido
  0 │──────┘
    └──────┬─────┬──────┬────────┬───────────┬──────────┬──────────> t
           │     │      │        │           │          │
           0     1      2        3           4          5
        el IRC  IRC   se libera  la boot   el Cortex   tu
        arranca estable el reset  ROM       toma SP/PC Reset_Handler
                                  decide
        │◄─60 µs─►│◄1µs►│         │◄────── 412 µs ──────►│
        │◄──── hasta 3 ms si hay que muestrear P2.10 ────►│
```

---

## Etapa 0: la alimentación sube, y el chip se niega a arrancar

Antes de nada, el chip se mantiene **en reset a propósito**. No es pasividad: hay dos
circuitos analógicos vigilando la alimentación.

**El POR (Power-On Reset)** detecta el encendido y mantiene el reset mientras VDD esté por
debajo de aproximadamente 1 V.

**El BOD (Brown-Out Detector)** vigila en dos niveles (capítulo 3.5 del manual):


| Nivel        | Valor típico | Qué hace                                                      |
| ------------ | ------------ | ------------------------------------------------------------- |
| Interrupción | 2.2 V        | Avisa al NVIC. Podés atenderlo y guardar datos antes de morir |
| Reset        | 1.85 V       | Fuerza el reset del chip                                      |


El reset por brown-out evita ejecutar el chip fuera del rango de alimentación garantizado,
situación en la que el comportamiento de la lógica y de la FLASH deja de ser confiable.

Además de POR y BOD, el reset puede venir del pin externo, del watchdog o de una solicitud del
sistema. El registro `RSID`, visto más adelante, ayuda a distinguir esas fuentes.

## Etapa 1: arranca el oscilador interno

Acá está la respuesta a una de las preguntas más frecuentes. El manual es tajante
(sección 4.3):

> *Since chip operation always begins using the Internal RC Oscillator, and the main
> oscillator may not be used at all in some applications, it will only be started by
> software request.*

**El cristal de 12 MHz de tu placa no participa del arranque.** Ni siquiera está encendido.
El chip arranca siempre con el **IRC**, un oscilador RC integrado en el silicio, a **4 MHz**.
El cristal recién se enciende cuando tu software lo pide, poniendo el bit `OSCEN` en el
registro `SCS`.

Por eso el blink bare metal de la plantilla, que no cambia el clock, corre a 4 MHz.
`openocd/lpc1769.cfg` usa ese valor para el estado posterior al reset. Si una herramienta se
conecta sin resetear un firmware que ya reconfiguró el clock, no debe asumir automáticamente
la misma frecuencia.

El IRC tarda **hasta 60 µs** en arrancar desde el encendido, más **1 µs** de conteo de
estabilidad. Recién cuando entrega un clock estable, la señal de reset se sincroniza con
él y empieza lo que sigue.

## Etapa 2: dos relojes de arena en paralelo

Con el reset ya sincronizado arrancan **dos temporizadores simultáneos** (capítulo 3.4):

1. **El temporizador del IRC**, de 2 bits. Cuando expira, **arranca el código de la boot
  ROM**.
2. **El temporizador de la Flash**, de 9 bits, que genera los **100 µs** que la Flash
  necesita para despertar. Al expirar, comienza la inicialización de la Flash, que lleva
   unos **250 ciclos** más.

Los dos corren a la vez porque no dependen uno del otro, y así el arranque es más corto. Si
la boot ROM necesita leer la Flash antes de que esté lista, el **Flash Accelerator** inserta
ciclos de espera automáticamente hasta que la lectura pueda completarse.

## Etapa 3: se libera el reset interno

El manual lo dice con una precisión que conviene leer literal:

> *When the internal Reset is removed, the processor begins executing at address 0, which
> is initially the Reset vector mapped from the Boot Block. At that point, all of the
> processor and peripheral registers have been initialized to predetermined values.*

Dos cosas importantes ahí:

- El procesador empieza en la dirección 0, pero **en la dirección 0 todavía no está tu
programa**: está el vector de reset del **Boot Block**, la ROM de NXP mapeada
temporalmente ahí.
- Todos los registros del procesador y de los periféricos ya tienen sus valores por
defecto. Ese es el estado "de fábrica" del que parte tu código.

> **Nota para quien venga de los LPC2000 (ARM7):** en aquellos había un registro `MEMMAP`
> para controlar este remapeo. **El LPC176x no lo tiene.** El Cortex-M3 resuelve el tema
> con `VTOR` (*Vector Table Offset Register*, en `0xE000ED08`), que después del reset vale
> 0 y por eso la tabla se lee desde el arranque de la Flash. Si buscás `MEMMAP` en el
> manual de este chip, no lo vas a encontrar.

## Etapa 4: la boot ROM decide si tu código merece correr

Ahora corre un programa de NXP de 8 KB que vive en `0x1FFF0000` y **no se puede borrar ni
modificar**. Es el mismo que implementa el bootloader ISP y las rutinas IAP.

Su trabajo es decidir entre dos destinos: tu programa, o el modo ISP. Las preguntas que se
hace, en orden (capítulo 32.4):

### 1. ¿Qué protección de lectura está configurada?

Lee la palabra de CRP en `0x000002FC`. Si contiene uno de los valores reconocidos, limita
el acceso de depuración y ciertas operaciones de ISP según el nivel. Con **CRP3** se deshabilitan
SWD/JTAG y la entrada a ISP mediante P2.10 cuando hay código de usuario válido. La recuperación
queda en manos del propio firmware, si este invoca IAP o ISP. Por eso `make preflight` comprueba
que la imagen no active CRP por accidente; el detalle está en
[07-06 - El primer grabado verificado](../07_lpc1769/06-primer-grabado-verificado.md).

### 2. ¿Se desbordó el watchdog?

Si el reset vino de un watchdog vencido, cambia el criterio: en ese caso **se ignora el
pedido de ISP por hardware**. La lógica es sensata, porque un watchdog que se desborda es
un programa colgado, no alguien queriendo reprogramar la placa.

### 3. ¿Alguien pide entrar a ISP por hardware?

Muestrea el pin **P2.10**. Si está en **bajo**, entra al modo ISP y se queda esperando por
la UART0 en lugar de correr tu programa.

Dos detalles que hacen perder tiempo:

- El muestreo puede tardar **hasta 3 ms** después del flanco de subida de `RESET`. Ese es
el motivo real de que el arranque total pueda ser bastante más largo que los 412 µs del
caso feliz.
- **P2.10 queda en alta impedancia después del reset.** El manual advierte explícitamente
que hay que ponerle un pull-up externo, porque si el pin queda flotando podés entrar a
ISP sin querer. Si tenés una placa propia que a veces arranca y a veces no, mirá ese pin
antes que cualquier otra cosa.

### 4. ¿El código de usuario es válido?

Esta es la famosa. Textual del manual (32.3.1.1):

> *The reserved Cortex-M3 exception vector location 7 (offset 0x001C in the vector table)
> should contain the 2's complement of the check-sum of table entries 0 through 6. This
> causes the checksum of the first 8 table entries to be 0.*

O sea: **suma las 8 primeras palabras de tu tabla de vectores y exige que den cero.** Si no
dan cero, concluye que la Flash está vacía o corrupta, y arranca la rutina de auto-baud
esperando un `?` (0x3F) por la UART0.

Este es un motivo frecuente del **"grabó y verificó, pero la aplicación no arranca"**. La plantilla lo resuelve inyectando el checksum en tiempo de compilación
(`tools/lpc_checksum.py`), y `make preflight` lo verifica antes de grabar.

### Cuánto tarda todo esto

El diagrama de tiempos del manual (figura 5 del capítulo 3) da estos valores de referencia
desde que el IRC está estable:


| Tramo                               | Tiempo     |
| ----------------------------------- | ---------- |
| Hasta que empieza a leer la Flash   | 7 µs       |
| Lectura de la Flash                 | 181 µs     |
| Resto de la ejecución del boot code | 224 µs     |
| **Total**                           | **412 µs** |


## Etapa 5: el Cortex-M3 toma tu programa

Superadas las verificaciones, la boot ROM transfiere el control y la FLASH de usuario queda
visible en la dirección 0. Para iniciar una excepción de reset, el núcleo toma dos valores de
la tabla:

```
    0x00000000  ──►  lo carga en el Stack Pointer (MSP)
    0x00000004  ──►  lo carga en el Program Counter
```

Eso es todo el "arranque" del Cortex-M3. Las dos palabras son las dos primeras entradas de
la tabla de vectores que arma
[`startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c).

El procesador queda en **modo Thread, privilegiado, usando el MSP**. Y hay un detalle que
rompe programas cuando se arma la tabla a mano:

**El vector 1 tiene que ser impar.** El Cortex-M3 solo ejecuta Thumb-2, y lo señaliza con
el bit 0 de la dirección de salto en 1. Por eso en nuestro binario el vector 1 vale
`0x000001B1` y no `0x000001B0`: el `1` final no es parte de la dirección, es el bit de modo.
Si queda par, el núcleo detecta un estado de instrucción inválido; según la configuración,
el UsageFault puede escalar a HardFault antes de ejecutar el handler. El linker conserva
correctamente el bit en una dirección de función y `make preflight` lo verifica.

## Etapa 6: `Reset_Handler`, o cómo C se vuelve cierto

Todavía no estás en `main()`. Falta que alguien cumpla las promesas que hace el lenguaje C,
porque hasta acá la RAM tiene basura. Eso es
[`Reset_Handler`](../../plantilla/startup/startup_lpc1769.c), y hace cuatro cosas:

**1. Copia `.data` de la Flash a la RAM.** Es lo que hace que `int contador = 42;` valga 42
en el primer ciclo de `main`. El 42 estaba guardado en la Flash (tiene que sobrevivir al
apagado) y se copia a la RAM (tiene que poder cambiar).

**2. Pone `.bss` en cero.** C exige que `static int estado;` valga 0 al entrar a `main`.
Este bucle cumple esa garantía. Si se elimina, el programa deja de respetar el entorno
requerido por el lenguaje.

**3. Llama a `SystemInit()`.** Acá se configura el clock: encender el cristal, enganchar la
PLL, subir a 100 MHz y ajustar los wait states de la Flash. Todo el detalle está en el
[módulo 3](../../curso/03_clock_y_power/03-arbol-de-clock-y-pll.md).

> **La trampa de la PLL.** El manual avisa (sección 4.5.1.1) que si el chip pasó por modo
> ISP, **el boot code deja la PLL configurada con el IRC**. O sea que no podés asumir que
> la PLL está apagada cuando arranca tu código, sobre todo al abrir una sesión de debug.
> Por eso el código de arranque tiene que **desconectar la PLL** antes de reconfigurarla, y
> no simplemente escribirle valores nuevos. Es la causa de un buen porcentaje de los "la
> PLL no engancha".
>
> Compilando sin CMSIS (`USE_CMSIS=0`) esto no te toca: `SystemInit()` queda vacía y el
> chip se queda con el IRC a 4 MHz. Es el arranque más difícil de arruinar, y por eso es el
> modo por defecto de la plantilla.

**4. Corre `__libc_init_array()`**, que ejecuta los constructores. En C puro casi nunca hay
(solo con `__attribute__((constructor))`); en C++ son los objetos globales.

Y recién ahí:

```c
main();
```

## El estado en el que quedás, medido en la placa

Esto no es teoría: son los registros leídos con el debugger sobre la LPC1769, con el blink
bare metal corriendo.


| Registro    | Dirección    | Valor        | Qué significa                                |
| ----------- | ------------ | ------------ | -------------------------------------------- |
| `SCS`       | `0x400FC1A0` | `0x00000000` | `OSCEN`=0: **el cristal está apagado**       |
| `CLKSRCSEL` | `0x400FC10C` | `0x00000000` | fuente de clock = IRC                        |
| `PLL0STAT`  | `0x400FC088` | `0x00000000` | PLL0 sin habilitar ni conectar               |
| `CCLKCFG`   | `0x400FC104` | `0x00000000` | divisor 1, o sea **CCLK = 4 MHz**            |
| `FLASHCFG`  | `0x400FC000` | `0x0000303A` | `FLASHTIM`=3, o sea 4 ciclos por acceso      |
| `VTOR`      | `0xE000ED08` | `0x00000000` | tabla de vectores en el arranque de la Flash |


Estas filas confirman el estado del firmware bare metal medido: seguía usando el IRC, con el
oscilador principal y la PLL deshabilitados. No describen el estado de cualquier programa
después de que `SystemInit()` reconfigure el clock.

## Cómo saber por qué se reseteó: `RSID`

Cuando una placa se reinicia sola, la pregunta es cuál de las cuatro fuentes fue. El chip
lo guarda en el registro `RSID` (`0x400FC180`):


| Bit | Símbolo    | Fuente                                                                                                            |
| --- | ---------- | ----------------------------------------------------------------------------------------------------------------- |
| 0   | `POR`      | Power-On Reset: se le dio energía                                                                                 |
| 1   | `EXTR`     | el pin `RESET`                                                                                                    |
| 2   | `WDTR`     | el watchdog se desbordó                                                                                           |
| 3   | `BODR`     | brown-out: la alimentación se cayó                                                                                |
| 4   | `SYSRESET` | pedido por software (`SYSRESETREQ`, lo que usa el debugger)                                                       |
| 5   | `LOCKUP`   | el núcleo entró en estado de *lockup* (el manual aclara que el lockup provoca reset del chip en los LPC178x/177x) |


En nuestra placa leímos **`0x00000013`**, o sea `POR` + `EXTR` + `SYSRESET`. Los tres a la
vez, y tiene sentido: hubo un power-on al enchufar el USB, un reset por pin, y varios
resets pedidos por OpenOCD al grabar.

Varios bits pueden quedar indicados a la vez. Para atribuir el próximo reset, leé el registro
al inicio y limpiá por escritura de uno los flags documentados:

```c
uint32_t causa = LPC_SC->RSID;   /* leer primero */
LPC_SC->RSID = 0x3F;             /* y limpiar, escribiendo 1 en cada bit */
```

Si no lo limpiás, la próxima vez no vas a poder distinguir el reset nuevo del viejo.

## Mirar el arranque vos mismo

Con la placa conectada, desde `plantilla/`:

```bash
openocd -f openocd/lpc1769.cfg -c "init; halt; exit"
```

Fijate en el PC:


| PC                                 | Dónde está parado                                        |
| ---------------------------------- | -------------------------------------------------------- |
| `0x1fff0xxx` | en la **boot ROM**; puede estar entrando a ISP, validando la imagen o atendiendo una rutina ROM |
| una dirección de FLASH de usuario | en **tu programa** |


Ese contraste indica qué región se está ejecutando. Para saber por qué quedó en ROM, además
hay que revisar P2.10, checksum, CRP y estado de reset. En la sesión documentada en el
[07-06](../07_lpc1769/06-primer-grabado-verificado.md), antes de grabar el
PC estaba en `0x1fff0080` y después en `0x00000164`: se ve exactamente el momento en que la
boot ROM entrega el control.

Y para ver los registros de arranque:

```bash
openocd -f openocd/lpc1769.cfg -c "init" \
  -c "mdw 0x400FC180" -c "mdw 0x400FC1A0" -c "mdw 0x400FC10C" \
  -c "mdw 0x400FC088" -c "mdw 0x400FC104" -c "mdw 0xE000ED08" -c "exit"
```

## Dónde se rompe cada etapa


| Síntoma                                   | Etapa | Causa                                               |
| ----------------------------------------- | ----- | --------------------------------------------------- |
| Ni la sonda logra conectarse | 0 o acceso de debug | alimentación, reset, cableado, velocidad o protección |
| Arranca de forma intermitente | 4 | revisar P2.10 y su pull-up, además de alimentación y reset |
| Graba y verifica, pero no ejecuta | 4 | comprobar checksum y vectores con `make preflight` |
| El debug quedó bloqueado y P2.10 no fuerza ISP | 4 | revisar si se programó CRP3 en `0x2FC` |
| Fault antes del handler | 5 | vector 1 par, SP inválido o tabla corrupta |
| Se detiene antes de `main` | 5 o 6 | inspeccionar PC, SP, copia de `.data` e inicialización |
| Las globales arrancan con basura          | 6     | falta la copia de `.data` o el borrado de `.bss`    |
| La PLL no engancha                        | 6     | no se desconectó antes de reconfigurarla            |
| Todo va 25 veces más lento de lo esperado | 6     | `SystemInit()` vacía: seguís con el IRC a 4 MHz     |
| Se reinicia solo cada tanto               | -     | leé `RSID`: te dice si fue watchdog o brown-out     |


## Lo que te llevás

- El LPC1769 inicia la secuencia con el IRC de 4 MHz; la aplicación puede habilitar después
  el oscilador principal y la PLL.
- Antes de la aplicación corre la boot ROM de NXP, que evalúa ISP, protección y validez de la
  imagen.
- La suma de las 8 primeras palabras de la tabla de vectores tiene que dar cero. No es una
convención: es una condición para arrancar.
- "Grabado correctamente" y "ejecutándose" son cosas distintas, y se verifican distinto.
- Si una global no tiene el valor inicial exigido por C, verificá primero la copia de
  `.data`, la limpieza de `.bss` y los símbolos del linker.

---

**Del código al binario:** [índice](./README.md) ·
**Anterior:** [02 - El linker script y el startup](./02-linker-y-startup.md) ·
**Siguiente parte:** [06 - Depurar en serio](../06_depurar_en_serio/)