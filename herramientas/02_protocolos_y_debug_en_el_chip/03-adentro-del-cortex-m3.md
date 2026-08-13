# El debug adentro del Cortex-M3

Este es el capítulo central de la parte. La pregunta que responde:

> Si el micro está corriendo mi programa a 100 MHz, ¿cómo hace el depurador para frenarlo en
> la línea 42, mostrarme el valor de una variable, y hacerlo seguir como si nada?

La respuesta corta es que el firmware no tiene que implementar esa función. El Cortex-M3 y
la integración del LPC1769 incluyen bloques de depuración y traza de la arquitectura
**CoreSight**. Algunos recursos, como la cantidad de comparadores o la presencia de ETM,
dependen de cada implementación.

---

## El mapa

```
   sonda ──SWD/JTAG──►┌──────────┐
                      │  SWJ-DP  │   el que habla el protocolo
                      └────┬─────┘
                           │
                      ┌────▼─────┐
                      │  AHB-AP  │   el que traduce a accesos de bus
                      └────┬─────┘
                           │
    ═══════════════════════╪═══════════════ bus interno del chip
        │        │         │         │          │
   ┌────▼───┐┌───▼───┐┌────▼───┐┌────▼───┐ ┌────▼────┐
   │  CPU   ││ FLASH ││  SRAM  ││ perif. │ │ CoreSight│
   │(halt,  ││       ││        ││        │ │ FPB DWT  │
   │ regs)  ││       ││        ││        │ │ ITM TPIU │
   └────────┘└───────┘└────────┘└────────┘ └──────────┘
```

**La idea central, y si te llevás una sola cosa que sea esta:**

> A través del Access Port, el depurador puede actuar como **otro maestro del bus** y hacer
> accesos sin ejecutar instrucciones del firmware.

De ahí salen todas las propiedades raras que tiene un depurador:

- Puede funcionar con el CPU **frenado** mientras el dominio de depuración y el bus sigan
  alimentados.
- Puede leer parte de la memoria con el CPU **corriendo**, aunque esos accesos compiten por el
  bus y una lectura puede no representar un estado coherente si el programa modifica los
  datos al mismo tiempo.
- Puede conectarse con la **FLASH vacía**, porque no depende de una rutina de la aplicación.
- Controla la detención del núcleo mediante registros de depuración. Seguridad, bajo consumo,
  reset o falta de clock todavía pueden impedir el acceso.

---

## Las dos piezas del DAP

El **DAP** (*Debug Access Port*) es el conjunto de las dos primeras cajas del diagrama, y la
separación entre ellas es a propósito:

| Pieza | Qué hace |
|---|---|
| **DP** (*Debug Port*) | habla el protocolo del cable: SWD o JTAG. En Cortex-M3 es el **SWJ-DP**, que entiende los dos |
| **AP** (*Access Port*) | traduce los pedidos del DP en transacciones sobre el bus del chip. En Cortex-M3 es un **AHB-AP**, porque el bus se llama AHB |

La separación permite combinar distintos puertos externos y buses internos. En un SWJ-DP,
JTAG y SWD llegan al mismo sistema de depuración y ofrecen las mismas operaciones sobre el
DAP; las funciones de boundary scan siguen siendo propias de JTAG.

El AHB-AP tiene, en esencia, tres registros:

| Registro | Para qué |
|---|---|
| `CSW` | configuración: tamaño del acceso (8/16/32 bits), autoincremento |
| `TAR` | *Transfer Address Register*: la dirección a la que querés acceder |
| `DRW` | *Data Read/Write*: el dato |

Conceptualmente, una lectura consiste en escribir `TAR` y obtener el dato por `DRW`.
Con autoincremento, se fija la dirección inicial una vez y se transfieren varias palabras.
El protocolo usa lecturas diferidas y registros intermedios, pero el servidor de depuración
oculta ese detalle al usuario.

---

## Frenar el CPU: los registros de depuración

El CPU se controla desde cuatro registros en el mapa de memoria, en la zona `0xE000EDF0`
(dentro del *System Control Space*):

| Registro | Nombre | Qué hace |
|---|---|---|
| `DHCSR` | *Debug Halting Control and Status* | habilita la depuración (`C_DEBUGEN`), frena (`C_HALT`), ejecuta un paso (`C_STEP`), enmascara interrupciones (`C_MASKINTS`) |
| `DCRSR` | *Debug Core Register Selector* | qué registro del CPU (R0, PC, xPSR...) querés ver |
| `DCRDR` | *Debug Core Register Data* | y su valor |
| `DEMCR` | *Debug Exception and Monitor Control* | configura *vector catch*, Debug Monitor y la habilitación global de los bloques de traza |

Ahora se puede leer al derecho la secuencia de un `make debug`:

1. La sonda escribe `C_DEBUGEN=1` en `DHCSR`. La depuración queda habilitada.
2. Escribe `C_HALT=1`. **El CPU se frena.**
3. Con el núcleo detenido, para mostrar `PC` selecciona ese registro en `DCRSR` y lee
   `DCRDR`.
4. Para mostrarte una variable, lee su dirección de RAM por el AHB-AP.
5. Para el "paso a paso", escribe `C_STEP=1`, que ejecuta **una** instrucción y vuelve a
   frenar.
6. Para continuar, escribe `C_HALT=0`.

Para ejecutar `break main`, GDB obtiene la dirección desde el ELF y el servidor instala un
breakpoint adecuado. Si `main` está en FLASH, normalmente usa un comparador del FPB; si
estuviera en memoria escribible también podría insertar una instrucción `BKPT`.

> **`C_MASKINTS` merece un párrafo.** Es el bit que decide si, mientras vas paso a paso,
> las interrupciones se atienden o se quedan pendientes. Sin él, un `SysTick` cada
> milisegundo te tiraría adentro de su handler cada vez que apretás "next", y sería
> imposible seguir el hilo de `main`. Con él, el `main` avanza limpio pero las ISR no corren
> mientras depurás. Ninguna de las dos opciones es "la correcta": son dos vistas distintas
> de tu programa, y conviene saber en cuál estás parado.

### El otro modo: Debug Monitor

Frenar el CPU no siempre se puede. En un control de motor o en una fuente conmutada, si el
procesador se para el hardware sigue girando y algo se rompe.

Para eso el Cortex-M3 ofrece **Debug Monitor**. Cuando está habilitado y el modo de halting
debug no toma el evento, determinados eventos de depuración generan la excepción `DebugMon`
en lugar de detener todo el núcleo. El firmware debe implementar su handler y asignarle una
prioridad compatible con el resto del sistema.

No lo vas a usar en la materia, pero es la respuesta a la pregunta "¿y si no puedo frenar el
micro?".

---

## FPB: los breakpoints de hardware

El **FPB** (*Flash Patch and Breakpoint unit*) es un bloque con **comparadores de dirección**.
Le cargás una dirección y, cuando el CPU va a buscar la instrucción que está ahí, el FPB
dispara una parada.

En el LPC1769 (UM10360 §33.1):

| Recurso | Cantidad |
|---|---|
| Comparadores de **instrucción** | **6** |
| Comparadores de **literales** (datos constantes) | 2 |

Por eso, al conectar OpenOCD, aparece esta línea:

```
Info : [lpc17xx.cpu] target has 6 breakpoints, 4 watchpoints
```

Para código en FLASH, esos seis comparadores fijan el límite práctico de breakpoints
simultáneos. El IDE puede reservar alguno o usar otro mecanismo en memoria escribible, por
eso el número visible puede variar.

> **¿Y por qué "Flash Patch"?** Porque el mismo hardware sirve para otra cosa: en vez de
> frenar, puede **redirigir** una búsqueda de instrucción a otra dirección. Así se parcha un
> bug en un firmware grabado en FLASH sin regrabarlo: se pone el parche en RAM y se redirige.
> Es un recurso de producto terminado, no de laboratorio, pero explica el nombre.

### Breakpoints de hardware y de software

Existen dos clases, y conviene distinguirlas:

| | De hardware (FPB) | De software |
|---|---|---|
| Cómo funciona | un comparador de dirección | el depurador **reemplaza** la instrucción por `BKPT` |
| Cuántos | 6 comparadores de instrucción en este chip | sin límite de comparadores; dependen de memoria escribible |
| ¿En FLASH? | **sí** | en general no, porque exigiría reprogramarla |
| ¿En RAM? | sí | sí |

Como el código de un firmware suele ejecutarse desde FLASH, normalmente se usan breakpoints
de hardware y aparece el límite del FPB. En RAM, GDB puede insertar `BKPT` y restaurar la
instrucción al quitar el breakpoint.

---

## DWT: watchpoints y mediciones

El **DWT** (*Data Watchpoint and Trace*) combina comparadores de datos, contadores y
generación de eventos de traza. En este Cortex-M3 dispone de **cuatro comparadores**, cuyo uso
depende de la función configurada.

### 1. Watchpoints

Frenar cuando el programa **lee o escribe** una dirección. Es la herramienta correcta para el
bug clásico de embebidos:

> "Hay una variable que se está pisando y no sé quién la pisa."

Ponés un watchpoint de escritura sobre esa variable, corrés, y el depurador te frena
exactamente en la instrucción culpable. Encontrar eso a mano puede llevar días.

```gdb
(gdb) watch mi_variable          # frena cuando alguien la escribe
(gdb) rwatch mi_variable         # cuando alguien la lee
```

### 2. Contadores de rendimiento

El DWT incluye contadores de hardware útiles para medir sin reservar un timer de la
aplicación:

| Registro | Qué cuenta |
|---|---|
| `CYCCNT` | **ciclos de reloj**. Un contador de 32 bits que corre libre |
| `CPICNT` | ciclos extra gastados por instrucciones de más de un ciclo |
| `EXCCNT` | ciclos gastados en entrar y salir de excepciones |
| `SLEEPCNT` | ciclos dormido |
| `LSUCNT` | ciclos extra de accesos a memoria |
| `FOLDCNT` | instrucciones que salieron gratis |

`CYCCNT` permite medir intervalos con resolución de un ciclo (10 ns a 100 MHz) y sin
reservar un timer:

```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* habilitar el DWT */
DWT->CYCCNT = 0;
DWT->CTRL  |= 1;                                   /* arrancar el contador */

hacer_la_cosa();

uint32_t ciclos = DWT->CYCCNT;
```

Es un contador de 32 bits: a 100 MHz desborda aproximadamente cada 42,9 segundos. Para
intervalos cortos, la resta unsigned entre la lectura final y la inicial maneja correctamente
un desborde.

Es exactamente lo que se usa en este repo para las mediciones de
[`ejemplos/uart/MEDICIONES.md`](../../curso/ejemplos/uart/MEDICIONES.md).

### 3. Muestreo del PC

El DWT puede emitir, cada tantos ciclos, **dónde está parado el programa**. Si el host junta
esas muestras y las cuenta, sale un *profiler* estadístico: qué función se lleva el tiempo,
sin instrumentar una sola línea de código. Necesita una vía de salida de traza, o sea SWO.

---

## ITM: que el programa mande texto

El **ITM** (*Instrumentation Trace Macrocell*) permite que el firmware genere eventos y
texto sin usar una UART.

Expone hasta **32 puertos de estímulo** en el mapa de memoria. Después de habilitar el ITM,
el puerto elegido y la salida de traza, el programa puede escribir un byte:

```c
ITM->PORT[0].u8 = 'H';    /* si el puerto está habilitado y listo */
```

La serialización de un carácter de 10 bits a 115200 bit/s tarda unos 87 µs. Una escritura
no bloqueante al ITM puede costar mucho menos CPU, aunque debe decidir qué hacer si el puerto
no está listo: esperar o descartar el dato.

Además del texto, el ITM se encarga de **sellar el tiempo** de los paquetes, para que del
otro lado se pueda reconstruir cuándo pasó cada cosa.

**La trampa, otra vez:** los bytes del ITM salen por SWO, y **la sonda tiene que poder
capturarlo**. La de la cátedra no puede, y por eso el curso usa RTT, que consigue algo
parecido sin depender de eso. Los dos caminos están comparados en el
[capítulo siguiente](./04-vias-de-salida-de-datos.md).

---

## TPIU y ETM: la traza pesada

- El **TPIU** (*Trace Port Interface Unit*) adapta y formatea los flujos de traza para la
  salida implementada por el chip. ITM y DWT pueden salir por **SWO**; la traza de mayor
  caudal usa el puerto paralelo (`TRACECLK` + `TRACEDATA[3:0]`).
- El **ETM** (*Embedded Trace Macrocell*) es la traza de instrucciones: reconstruye
  **cada instrucción que ejecutó el procesador**, comprimida. Con eso se puede rebobinar un
  programa y ver qué pasó *antes* del cuelgue, que es lo único que sirve para bugs que
  aparecen una vez cada dos horas.

El LPC1769 tiene ETM y saca los cinco pines de traza paralela (UM10360 §33.4). Pero esos
cinco pines son pines de aplicación, hace falta una sonda de traza (bastante más cara que una
sonda de depuración normal), y la placa de la cátedra no los lleva a ningún conector. En la
práctica, en esta materia el ETM no se usa.

---

## La tabla ROM: cómo se descubren los componentes

Una vez que la herramienta conoce el tipo de target y logró acceder al DAP, todavía necesita
ubicar los componentes CoreSight presentes. En `0xE00FF000` hay una **tabla ROM** con
entradas que apuntan a esos bloques. Cada componente incluye registros de identificación
(`CIDR`, `PIDR`) con su clase y versión.

La tabla permite descubrir la topología de depuración; no reemplaza la configuración
específica del micro, que sigue siendo necesaria para conocer su FLASH, resets y
periféricos.

El mapa completo de la zona (el *Private Peripheral Bus*):

| Dirección | Bloque |
|---|---|
| `0xE0000000` | ITM |
| `0xE0001000` | DWT |
| `0xE0002000` | FPB |
| `0xE000E000` | SCS: NVIC, SysTick, SCB y los registros de depuración |
| `0xE0040000` | TPIU |
| `0xE0041000` | ETM |
| `0xE00FF000` | tabla ROM |

Notá algo: **son direcciones de memoria comunes.** Tu propio programa las puede leer y
escribir, y eso es lo que hace el `DWT->CYCCNT` de más arriba. La única diferencia con la
sonda es por dónde entra.

---

## Lo que se frena y lo que no

Cuando el depurador detiene el CPU, no todos los periféricos se detienen con él. Cuáles
siguen funcionando depende del chip y de sus opciones de congelamiento. En el LPC1769 esto
produce varias sorpresas:

Del UM10360 §33.5:

| Qué | Con el CPU frenado |
|---|---|
| **SysTick** | se detiene automáticamente |
| **RIT** (*Repetitive Interrupt Timer*) | se detiene automáticamente |
| **Timers 0 a 3** | **siguen contando** |
| **UART** | **sigue recibiendo**, y se le desborda el FIFO |
| **ADC, DMA, PWM, CAN** | en general siguen, según su estado y fuente de clock |
| El mundo exterior (un motor, un sensor) | ni se entera |

Las consecuencias prácticas:

- **Ir paso a paso no conserva los tiempos.** Si tu código depende de que dos cosas pasen
  cerca, paso a paso no vas a ver el bug o vas a ver uno que no existe.
- **Una UART recibiendo se desborda** mientras estás parado, y aparecen errores de overrun
  que no tienen nada que ver con tu bug.
- **Si controlás algo físico, frenar puede romperlo.** Ahí es donde entra el Debug Monitor.

Y tres advertencias más del mismo capítulo del manual, que ahorran horas:

1. **No uses Deep Sleep ni Power-down mientras depurás.** Por una limitación de la
   integración del Cortex-M3 en este chip, el micro no despierta de esos modos como debería.
2. **No midas consumo mientras depurás.** La lógica de depuración activa cambia el manejo de
   los modos de bajo consumo, y las mediciones te van a dar más altas que en operación real.
3. **El CRP restringe o deshabilita la depuración según el nivel elegido.** Es una medida de
   seguridad para impedir la lectura del firmware. Algunos niveles se revierten con un
   borrado completo; el nivel más restrictivo puede cerrar las vías de recuperación. Antes
   de grabarlo, verificá el valor y sus consecuencias
   ([ver 07-06](../07_lpc1769/06-primer-grabado-verificado.md)).

---

## Lo que hay que recordar

- El Access Port puede actuar como **otro maestro del bus**. Por eso puede acceder a memoria
  sin ejecutar firmware y, en condiciones normales, sin detener el núcleo.
- Frenar, ir paso a paso y leer registros del CPU es escribir bits en `DHCSR` y `DCRSR`.
- El FPB aporta **6 comparadores de instrucción** para breakpoints sobre código en FLASH.
- Los watchpoints son **4** (DWT), y son la mejor herramienta que existe para "alguien me
  está pisando esta variable".
- El **`CYCCNT` del DWT** mide ciclos sin reservar un timer, pero es de 32 bits y
  desborda.
- El **ITM** permite emitir eventos y texto con bajo costo, pero requiere configuración,
  ancho de banda y una sonda que capture SWO.
- Al frenar el CPU, algunos bloques se detienen y otros siguen. En el LPC1769, SysTick y RIT
  se congelan automáticamente, mientras varios periféricos continúan.

---

**Protocolos:** [índice](./README.md) ·
**Anterior:** [02 - SWD](./02-swd.md) ·
**Siguiente:** [04 - Las vías de salida de datos](./04-vias-de-salida-de-datos.md)

**Manual:** capítulo [33](../../manual/ch33_jtag-serial-wire-debug-and-trace.pdf) (JTAG, SWD
y traza) y capítulo [34](../../manual/ch34_appendix-cortex-m3-user-guide.pdf) (guía de usuario
del Cortex-M3).
