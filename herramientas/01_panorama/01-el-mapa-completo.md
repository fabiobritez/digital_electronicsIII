# El mapa completo: de `main.c` a un LED parpadeando

Antes de instalar nada, conviene tener el mapa entero en la cabeza. Esta página presenta
las piezas principales que intervienen entre el archivo de texto que escribís y el
transistor que se enciende en la placa, y explica para qué sirve cada una.

Primero se presenta el esquema general, que se repite en muchas plataformas. Después se muestra
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
                        │ SWDIO, SWCLK, GND y, según la sonda, VTref y RESET
                        ▼
                  FLASH del micro
                        │
                 ┌──────▼──────┐
                 │  8. RESET   │  la boot ROM verifica y arranca
                 └──────┬──────┘
                        ▼
                  Reset_Handler ──► SystemInit() ──► main()
```

Ocho pasos. MCUXpresso los coordina cuando apretás un botón. Es cómodo, pero para interpretar
un error necesitás reconocer en cuál de esos pasos apareció.

## Las piezas, una por una

### 1 a 3. El compilador cruzado

**En general.** Tu PC tiene un procesador x86-64 y el micro un ARM. Un `gcc` normal genera
código para la máquina donde corre; acá hace falta un **compilador cruzado**: corre en
x86-64 pero produce instrucciones ARM. Por eso se llama `arm-none-eabi-gcc` y no `gcc`.

Ese nombre no es decorativo, describe el objetivo en tres partes:

| Parte | Significa |
|-------|-----------|
| `arm` | la arquitectura de destino |
| `none` | no apunta a un sistema operativo concreto: está pensado para *bare metal* |
| `eabi` | la ABI de ARM para sistemas embebidos: convenciones de llamada, formato de objetos, tamaños y alineaciones |

En un sistema *bare metal* no hay un sistema operativo que cargue el ejecutable, asigne
memoria o decida adónde escribe `printf`. Tampoco hay un proceso al cual volver cuando
`main()` termina. El firmware, o las bibliotecas que integres, debe resolver esas tareas;
de ahí salen varias de las piezas que siguen.

**En el LPC1769.** El núcleo es un **Cortex-M3**, que solo entiende el set de
instrucciones **Thumb-2**. De ahí los dos flags que aparecen en todos lados:

```
-mcpu=cortex-m3 -mthumb
```

Si faltan o no coinciden con el núcleo, el build puede fallar o generar instrucciones que el
micro no admite. Detalle completo en
[04 - ¿Qué es un toolchain?](../04_toolchains/01-que-es-un-toolchain.md).

### 4. El linker y el linker script

**En general.** Después de compilar tenés varios `.o` con código máquina, pero **sin
dirección asignada**. El linker los junta, resuelve las referencias entre ellos (la
llamada a `delay()` de un archivo tiene que apuntar a la función en el otro) y le asigna a
cada cosa una dirección concreta.

Para eso necesita saber qué memoria tiene el chip y dónde. En una PC intervienen el linker,
el formato del ejecutable y el cargador del sistema operativo. En un firmware *bare metal*,
ese mapa queda definido en el **linker script** (`.ld`).

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
[`plantilla/linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld). La explicación, en el
[05 - El linker script y el startup](../05_del_codigo_al_binario/02-linker-y-startup.md).

### 5. El startup

**En general.** Cuando el micro sale del reset **no salta a `main()`**. Primero ejecuta una
rutina de arranque que prepara el entorno que C necesita. En un proyecto típico hace, como
mínimo, estas tareas:

1. **Copiar `.data`** de FLASH a RAM, para que tus globales con inicializador valgan lo
   que dijiste.
2. **Poner `.bss` en cero**, para que tus globales sin inicializar valgan 0.
3. Llamar a la inicialización temprana del sistema, por ejemplo `SystemInit()` para el
   clock, y después a `main()`.

Los dos primeros puntos explican por qué una variable global sin inicializador comienza en
cero: C exige ese valor inicial, y la rutina de arranque materializa esa garantía antes de
entrar a `main()`.

El startup también define la **tabla de vectores**: un arreglo de punteros a función, uno
por cada interrupción, que tiene que estar al principio de la FLASH. Es lo que conecta
"se disparó el Timer 0" con "llamá a esta función mía".

**En el LPC1769.** La tabla tiene 16 entradas de excepciones del núcleo Cortex-M3 más 35
de periféricos (IRQ 0 a 34, de `WDT_IRQHandler` a `CANActivity_IRQHandler`). Las dos
primeras palabras son las que lee el núcleo al tomar el control de la aplicación:

| Dirección | Contenido |
|-----------|-----------|
| `0x00000000` | valor inicial del stack pointer (`0x10008000`, el tope de la SRAM) |
| `0x00000004` | dirección del `Reset_Handler` |

El archivo real: [`plantilla/startup/startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c).

### 6 y 7. El grabador y el debug probe

**En general.** Ya tenés los bytes; falta meterlos en la FLASH. La FLASH no se escribe como
la RAM: hay que **borrar por sectores** enteros primero y después **escribir por bloques**,
respetando tiempos. De eso se encargan dos piezas:

- El **debug probe**: el hardware que traduce entre el USB de tu PC y los pines de depuración
  del micro.
- El **grabador**: el programa de tu PC que le dice al probe qué escribir.

**En el LPC1769.** Las señales principales de depuración son **SWDIO** y **SWCLK**; además se
necesita una masa común y muchas sondas usan VTref y RESET. La placa de la cátedra trae el
probe soldado: un LPC11U35 corriendo CMSIS-DAP, que es un
estándar abierto de ARM. Por eso funciona con `openocd` y `pyocd` sin nada de NXP.

Qué es exactamente una sonda y cuáles existen: [parte 03](../03_debug_probes/). Cuál te
tocó a vos y qué hacer en cada caso: [guía por sonda](../07_lpc1769/probes/).

Un detalle interesante: para grabar este chip, la herramienta se apoya en rutinas **IAP**
de la boot ROM. Carga datos y un pequeño algoritmo en RAM; ese código llama a las rutinas
que controlan la FLASH.

### 8. El arranque

**En general.** Al resetear, el micro lee la tabla de vectores y salta al `Reset_Handler`.

**En el LPC1769 hay un paso previo.** Antes que tu código
corre la **boot ROM**, 8 KB que NXP grabó en fábrica en `0x1FFF0000` y que no se pueden
borrar. Hace dos verificaciones:

1. **¿El pin P2.10 está en bajo?** Si sí, entra en modo ISP y espera por la UART0 en vez
   de correr tu programa.
2. **¿Hay código válido?** Suma las primeras **8 palabras** de la tabla de vectores y exige
   que el resultado sea **cero**. Si no da cero, asume que la FLASH está vacía o corrupta
   y se queda en ISP.

Como las primeras siete palabras contienen el stack pointer y direcciones de handlers, la
plantilla pone en la octava (el **vector 7**, offset `0x1C`, que ARM declara
"reservado") el complemento a dos de las otras siete.

Si el checksum falta, la grabación puede verificarse correctamente y, aun así, la aplicación
no arrancar. Algunas herramientas lo corrigen durante el grabado y otras esperan recibir
una imagen ya válida. Para que el resultado no dependa del grabador, la
[plantilla](../../plantilla/) lo calcula durante el build:

```bash
make vectores      # muestra la tabla y verifica que la suma dé cero
```

Recién cuando la boot ROM se da por satisfecha, tu `Reset_Handler` toma el control.

Esto es un resumen de la etapa. La secuencia completa, desde que la alimentación empieza a
subir (el POR, el brown-out, el oscilador interno, los temporizadores de la Flash) hasta la
primera instrucción de tu `main()`, está en
[05 - El arranque paso a paso](../05_del_codigo_al_binario/03-el-arranque-paso-a-paso.md).

## La lista completa de lo que hace falta

| Pieza | Para qué | En el LPC1769 |
|-------|----------|---------------|
| Compilador cruzado | C → código máquina ARM | `arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb` |
| Biblioteca C | `memcpy`, `printf`... en versión chica | newlib-nano (`--specs=nano.specs`) |
| Syscalls | el piso que `printf` y `malloc` esperan | [`syscalls.c`](../../plantilla/src/syscalls.c) |
| Linker script | el mapa de memoria del chip | [`lpc1769.ld`](../../plantilla/linker/lpc1769.ld) |
| Startup | `.data`, `.bss`, clock, tabla de vectores | [`startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c) |
| Sistema de build | orquestar todo lo anterior | [`Makefile`](../../plantilla/Makefile) |
| Checksum | que la boot ROM acepte el firmware | [`lpc_checksum.py`](../../plantilla/tools/lpc_checksum.py) |
| Grabador | mover los bytes a la FLASH | openocd, pyocd, LinkServer, lpc21isp |
| Probe | traducir USB a SWD | CMSIS-DAP a bordo (OM13085) |
| Depurador | breakpoints y ver variables | gdb + un servidor gdb |
| Editor | escribir y navegar el código con comodidad | el que quieras |

No todos los proyectos usan cada pieza de la tabla: por ejemplo, podés prescindir de la
biblioteca C, de los syscalls o de un editor gráfico. Lo importante es reconocer qué problema
resuelve cada componente y cuál está participando en tu build.

Los archivos específicos del proyecto (`Makefile`, linker script, startup, syscalls y
checksum) están comentados en la [plantilla del repo](../../plantilla/).

## Por qué vale la pena

Un IDE que hace los ocho pasos con un botón es cómodo hasta que algo falla. Y cuando falla,
la diferencia entre saber y no saber esto es la diferencia entre:

> "no anda"

y

> "compila y linkea bien, el `.bin` tiene 608 bytes y el checksum correcto, openocd
> encuentra el probe pero falla al verificar el sector 0: debe ser un problema de
> velocidad del adaptador".

La segunda descripción no garantiza una solución inmediata, pero acota mucho la búsqueda:
ya separó el build, la imagen y la comunicación con la sonda.

Además, el esquema es transferible. Al cambiar de familia vas a reemplazar el mapa de
memoria, el startup, las opciones del compilador y el método de grabado, pero las funciones
de cada etapa siguen siendo reconocibles.

---

**Panorama:** [índice](./README.md) ·
**Siguiente:** [02 - Cómo se graba un micro](./02-como-se-graba-un-micro.md)
