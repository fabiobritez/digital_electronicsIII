# El camino completo: de `main.c` a un LED parpadeando

Antes de instalar nada, conviene tener el mapa entero en la cabeza. Esta página nombra
**todas** las piezas que intervienen entre el archivo de texto que escribís y el
transistor que se enciende en la placa, y explica para qué está cada una.

Primero en general, porque el camino es el mismo en cualquier microcontrolador. Después,
qué le corresponde a cada pieza en el LPC1769.

## El mapa

```
   main.c                          lo que escribís
     │
     │  ┌──────────────────────────────────────────────────┐
     │  │  1. PREPROCESADOR   resuelve #include y #define   │
     │  │  2. COMPILADOR      C -> assembler de ARM         │  arm-none-eabi-gcc
     │  │  3. ENSAMBLADOR     assembler -> código máquina   │
     │  └──────────────────────────────────────────────────┘
     ▼
   main.o          startup.o          drivers.o        (código máquina, sin ubicar)
     └──────────────────┬──────────────────┘
                        │
                 ┌──────▼──────┐
                 │ 4. LINKER   │ ◄──── linker script (.ld): el mapa de memoria del chip
                 └──────┬──────┘
                        ▼
                 firmware.elf     código máquina UBICADO + símbolos + info de debug
                        │
                 ┌──────▼──────┐
                 │ 5. OBJCOPY  │  saca los metadatos, deja los bytes pelados
                 └──────┬──────┘
                        ▼
              firmware.bin / .hex
                        │
                 ┌──────▼──────────┐
                 │ 6. GRABADOR     │  openocd / pyocd / LinkServer / lpc21isp
                 └──────┬──────────┘
                        │ USB
                 ┌──────▼──────┐
                 │  7. PROBE   │  el debug probe: traduce USB a SWD
                 └──────┬──────┘
                        │ 2 cables (SWDIO, SWCLK)
                        ▼
                  FLASH del micro
                        │
                 ┌──────▼──────┐
                 │  8. RESET   │  la boot ROM verifica y arranca
                 └──────┬──────┘
                        ▼
                  Reset_Handler ──► SystemInit() ──► main()
```

Ocho pasos. MCUXpresso los hace todos cuando apretás un botón, y ese es exactamente el
problema: cuando alguno falla, no tenés idea de cuál fue.

## Las piezas, una por una

### 1 a 3. El compilador cruzado

**En general.** Tu PC tiene un procesador x86-64 y el micro un ARM. Un `gcc` normal genera
código para la máquina donde corre; acá hace falta un **compilador cruzado**: corre en
x86-64 pero produce instrucciones ARM. Por eso se llama `arm-none-eabi-gcc` y no `gcc`.

Ese nombre no es decorativo, describe el objetivo en tres partes:

| Parte | Significa |
|-------|-----------|
| `arm` | la arquitectura de destino |
| `none` | **no hay sistema operativo**. Es la parte importante |
| `eabi` | la convención de llamadas que se usa (qué registro lleva cada argumento, cómo se devuelven los valores) |

El `none` es la diferencia de fondo con programar para una PC. Sin sistema operativo no
hay quien cargue tu programa en memoria, no hay `printf` que sepa a dónde escribir, no hay
memoria virtual y no hay nadie a quien devolverle el control cuando `main()` termina.
Todo eso lo tenés que proveer vos, y de ahí salen las piezas que siguen.

**En el LPC1769.** El núcleo es un **Cortex-M3**, que solo entiende el set de
instrucciones **Thumb-2**. De ahí los dos flags que aparecen en todos lados:

```
-mcpu=cortex-m3 -mthumb
```

Si te los olvidás, el compilador genera código para otro ARM y el micro se cuelga apenas
arranca, sin ningún mensaje de error. Detalle completo en
[01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md).

### 4. El linker y el linker script

**En general.** Después de compilar tenés varios `.o` con código máquina, pero **sin
dirección asignada**. El linker los junta, resuelve las referencias entre ellos (la
llamada a `delay()` de un archivo tiene que apuntar a la función en el otro) y le asigna a
cada cosa una dirección concreta.

Para eso necesita saber qué memoria tiene el chip y dónde. En una PC eso lo maneja el
sistema operativo; acá se lo tenés que decir vos, y el archivo donde se lo decís es el
**linker script** (`.ld`).

También es acá donde se resuelve el asunto de las variables globales: `int x = 5;` tiene
que vivir en RAM (porque puede cambiar) pero su valor inicial tiene que estar guardado en
FLASH (para sobrevivir al apagado). El linker le asigna las dos direcciones.

**En el LPC1769.** El mapa que va en el script:

| Memoria | Dirección | Tamaño | Para qué |
|---------|-----------|--------|----------|
| FLASH | `0x00000000` | 512 KB | el código y las constantes |
| SRAM principal | `0x10000000` | 32 KB | variables y stack |
| SRAM AHB 0 | `0x2007C000` | 16 KB | buffers de DMA y USB |
| SRAM AHB 1 | `0x20080000` | 16 KB | buffers de Ethernet |

Son 64 KB de RAM en total, pero **no contiguos**: hay un agujero enorme entre la SRAM
principal y las de AHB. Por eso en el linker script son cuatro regiones separadas y no
una sola.

El archivo real y comentado:
[`plantilla/linker/lpc1769.ld`](../../../plantilla/linker/lpc1769.ld). La explicación, en el
[anexo A](../A_build_linker_startup/02-linker-y-startup.md).

### 5. El startup

**En general.** Cuando el micro sale del reset **no salta a `main()`**: no sabe que
existe. Salta a una función que el estándar de C da por sentada pero que alguien tiene que
escribir, y que hace tres cosas antes de llamar a tu programa:

1. **Copiar `.data`** de FLASH a RAM, para que tus globales con inicializador valgan lo
   que dijiste.
2. **Poner `.bss` en cero**, para que tus globales sin inicializar valgan 0.
3. **Configurar el clock** y llamar a `main()`.

Los dos primeros puntos son la respuesta a una pregunta que casi nadie se hace: *¿por qué
una variable global sin inicializar arranca en cero?* No lo hace el lenguaje. Lo hace este
código, que corre antes que el tuyo.

El startup también define la **tabla de vectores**: un arreglo de punteros a función, uno
por cada interrupción, que tiene que estar al principio de la FLASH. Es lo que conecta
"se disparó el Timer 0" con "llamá a esta función mía".

**En el LPC1769.** La tabla tiene 16 entradas de excepciones del núcleo Cortex-M3 más 35
de periféricos (IRQ 0 a 34, de `WDT_IRQHandler` a `CANActivity_IRQHandler`). Las dos
primeras son las que lee el hardware al resetear:

| Dirección | Contenido |
|-----------|-----------|
| `0x00000000` | valor inicial del stack pointer (`0x10008000`, el tope de la SRAM) |
| `0x00000004` | dirección del `Reset_Handler` |

El archivo real: [`plantilla/startup/startup_lpc1769.c`](../../../plantilla/startup/startup_lpc1769.c).

### 6 y 7. El grabador y el debug probe

**En general.** Ya tenés los bytes; falta meterlos en la FLASH. La FLASH no se escribe como
la RAM: hay que **borrar por sectores** enteros primero y después **escribir por bloques**,
respetando tiempos. De eso se encargan dos piezas:

- El **debug probe**: el hardware que traduce entre el USB de tu PC y los pines de depuración
  del micro.
- El **grabador**: el programa de tu PC que le dice al probe qué escribir.

**En el LPC1769.** Los pines de depuración son **SWDIO** y **SWCLK** (SWD, dos cables). La
placa de la cátedra trae el probe soldado: un LPC11U35 corriendo CMSIS-DAP, que es un
estándar abierto de ARM. Por eso funciona con `openocd` y `pyocd` sin nada de NXP.

Cuál te tocó a vos y qué hacer en cada caso: [guía de probes](./probes/).

Un detalle interesante: hasta el probe termina apoyándose en el chip. Copia los datos a la
RAM del LPC1769 y llama a las rutinas **IAP** que están en la boot ROM, que son las que de
verdad escriben la FLASH.

### 8. El arranque

**En general.** Al resetear, el micro lee la tabla de vectores y salta al `Reset_Handler`.

**En el LPC1769 hay un paso previo que sorprende a todo el mundo.** Antes que tu código
corre la **boot ROM**, 8 KB que NXP grabó en fábrica en `0x1FFF0000` y que no se pueden
borrar. Hace dos verificaciones:

1. **¿El pin P2.10 está en bajo?** Si sí, entra en modo ISP y espera por la UART0 en vez
   de correr tu programa.
2. **¿Hay código válido?** Suma las primeras **8 palabras** de la tabla de vectores y exige
   que el resultado sea **cero**. Si no da cero, asume que la FLASH está vacía o corrupta
   y se queda en ISP.

Como las primeras 7 palabras son el stack pointer y los handlers, la única forma de que la
suma dé cero es poner en la octava (el **vector 7**, offset `0x1C`, que ARM declara
"reservado") el complemento a dos de las otras siete.

Esta es la causa número uno del clásico **"grabé la placa, dice que grabó bien, y no hace
nada"**. Y es especialmente traicionera porque cada herramienta lo maneja distinto:
`openocd` y `lpc21isp` lo parchean solos, **`pyocd` no**. Por eso la
[plantilla](../../../plantilla/) lo inyecta en tiempo de compilación, y así los cuatro
caminos funcionan igual:

```bash
make vectores      # muestra la tabla y verifica que la suma dé cero
```

Recién cuando la boot ROM se da por satisfecha, tu `Reset_Handler` toma el control.

Esto es un resumen de la etapa. La secuencia completa, desde que la alimentación empieza a
subir (el POR, el brown-out, el oscilador interno, los temporizadores de la Flash) hasta la
primera instrucción de tu `main()`, está en
[16 - El arranque paso a paso](../A_build_linker_startup/03-el-arranque-paso-a-paso.md).

## La lista completa de lo que hace falta

| Pieza | Para qué | En el LPC1769 |
|-------|----------|---------------|
| Compilador cruzado | C → código máquina ARM | `arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb` |
| Biblioteca C | `memcpy`, `printf`... en versión chica | newlib-nano (`--specs=nano.specs`) |
| Syscalls | el piso que `printf` y `malloc` esperan | [`syscalls.c`](../../../plantilla/src/syscalls.c) |
| Linker script | el mapa de memoria del chip | [`lpc1769.ld`](../../../plantilla/linker/lpc1769.ld) |
| Startup | `.data`, `.bss`, clock, tabla de vectores | [`startup_lpc1769.c`](../../../plantilla/startup/startup_lpc1769.c) |
| Sistema de build | orquestar todo lo anterior | [`Makefile`](../../../plantilla/Makefile) |
| Checksum | que la boot ROM acepte el firmware | [`lpc_checksum.py`](../../../plantilla/tools/lpc_checksum.py) |
| Grabador | mover los bytes a la FLASH | openocd, pyocd, LinkServer, lpc21isp |
| Probe | traducir USB a SWD | CMSIS-DAP a bordo (OM13085) |
| Depurador | breakpoints y ver variables | gdb + un servidor gdb |
| Editor | escribir cómodo | el que quieras |

Son once piezas. **Ninguna es opcional y ninguna es magia**: cada una resuelve un problema
concreto que en una PC te resuelve el sistema operativo.

Las siete primeras están, escritas y comentadas, en la
[plantilla del repo](../../../plantilla/). Podés leerlas enteras en una tarde, y ese es el
objetivo de este módulo.

## Por qué vale la pena

Un IDE que hace los ocho pasos con un botón es cómodo hasta que algo falla. Y cuando falla,
la diferencia entre saber y no saber esto es la diferencia entre:

> "no anda"

y

> "compila y linkea bien, el `.bin` tiene 608 bytes y el checksum correcto, openocd
> encuentra el probe pero falla al verificar el sector 0: debe ser un problema de
> velocidad del adaptador".

El segundo problema se resuelve en dos minutos. El primero puede llevarte una semana.

Además, lo que aprendas acá es transferible: cambiá el linker script y el startup, y el
mismo esquema te sirve para un STM32, un ESP32 o cualquier otro Cortex-M.

---

**Módulo:** [18 - Toolchain y entorno](./README.md) ·
**Siguiente:** [01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md)
