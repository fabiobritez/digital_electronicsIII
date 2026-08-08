# `printf` por el cable del debugger (RTT)

> **La guía de uso está en [módulo 12, capítulo 3](../../../12_debug/03-consola-por-el-debugger-rtt.md)**:
> puesta a punto en Ubuntu 24, los tres pasos para usarlo, cómo depurar y ver los `printf` a la vez,
> y qué pasa si además usás MCUXpresso. Este README explica el ejemplo y el porqué del diseño.

Los dos ejemplos anteriores mandan el texto por la UART. Este no usa la UART **para nada**: el
programa escribe en una cola en RAM y **el debugger la lee por SWD mientras el micro corre**, sin
frenarlo. Ni un pin de aplicación, ni un periférico, ni conversor USB-serie, ni baudrate que
calcular.

| | CPU por línea de 48 caracteres |
|---|---:|
| `printf` por UART, polling | 4091 µs |
| `printf` por UART, DMA | 36 µs |
| **`printf` por RTT** | **17 µs** |

## ¿Y el SWO/ITM, que es lo que todo el mundo nombra?

Es la respuesta "de manual", y en esta placa **no se puede usar**. Vale la pena entender por qué,
porque es un caso lindo de "el chip puede, la herramienta no".

El LPC1769 **sí** tiene la salida de traza: el UM10360 §33.4 documenta el pin **SWO**, y lo mejor es
que comparte pin con **TDO**, o sea que es un pin del conector de debug y **no te cuesta ningún pin
de aplicación**. La unidad ITM del Cortex-M3 puede mandar caracteres por ahí.

El problema está del otro lado del cable. La sonda CMSIS-DAP de a bordo de la LPCXpresso tiene
firmware viejo y no sabe capturar SWO:

```console
$ openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

Fijate lo que **no** dice: `SWO-UART supported`. Si igual insistís:

```console
Error: SWO-trace is not supported by the device.
Error: Failed to start adapter's trace
```

Para usar SWO necesitás otra sonda: un J-Link, un ST-Link V2/V3, o un MCU-Link / LPC-Link2 con
firmware CMSIS-DAP v2. Con esas, el camino es `tpiu configure -protocol uart` en OpenOCD.

**RTT es la alternativa que funciona con la sonda que ya tenés**, y encima es más rápida, porque no
depende de un pin serie sino de lecturas de memoria por SWD.

## Cómo funciona

```
   printf()  →  _write()  →  cola circular en RAM
                                   ↑
                          el debugger la lee por SWD
                          MIENTRAS el programa corre
```

La clave es que la unidad de debug del Cortex-M3 puede leer memoria **en paralelo al CPU**, sin
frenarlo ni interrumpirlo. El micro no se entera. Por eso `_write` cuesta 17 µs: es un `memcpy` a
RAM y nada más.

El bloque de control tiene el formato de **SEGGER RTT**, que es el que entienden OpenOCD, J-Link y
pyOCD. No usamos el código de SEGGER: son cien líneas, están en [`rtt.c`](./rtt.c) y se leen enteras.

### Dos detalles que parecen manías y no lo son

**El identificador se arma carácter por carácter, y al final.** El host encuentra el bloque
**escaneando la RAM** en busca de la cadena `"SEGGER RTT"`. Si la pusiéramos como literal, el
compilador dejaría además una copia en `.rodata` y el host podría toparse con la equivocada.
Y escribirlo último evita que, si el host se conecta justo durante el arranque, encuentre un bloque
a medio construir.

**Hay una barrera de memoria antes de publicar el índice.** Primero los datos, después `WrOff`. Si
el host viera el índice nuevo con el buffer todavía sin escribir, leería basura.

**Se copia con `memcpy` en uno o dos tramos, no byte por byte.** El segundo tramo aparece cuando los
datos dan la vuelta al final del arreglo. Para una línea de 48 caracteres la diferencia medida fue
de 24 µs a 17 µs.

### Y es de ida y vuelta

El canal de bajada (PC → micro) hace que `getchar()` y `scanf()` funcionen, así que tenés una consola
de comandos sin gastar un solo pin:

```c
int k = rtt_getchar();          /* -1 si no hay nada, no bloquea */
if (k == 'l') { alternar_led(); }
```

Probado mandando texto desde la PC: el micro recibe cada tecla y contesta.

## Compilar y probar

```bash
cd plantilla
cp ../curso/ejemplos/uart/printf_rtt/rtt.h  src/
cp ../curso/ejemplos/uart/printf_rtt/rtt.c  src/
cp ../curso/ejemplos/uart/printf_rtt/main.c src/
make USE_CMSIS=1 flash
make rtt                 # levanta el servidor y se conecta, en un comando
```

`make rtt` hace esto por debajo, que conviene saber para el día que algo falle:

```bash
openocd -f openocd/lpc1769.cfg \
  -c "init" -c "reset run" \
  -c 'rtt setup 0x10000000 0x8000 "SEGGER RTT"' \
  -c "rtt start" \
  -c "rtt server start 9090 0"
```

```
Info : rtt: Searching for control block 'SEGGER RTT'
Info : rtt: Control block found at 0x10000244
Info : Listening on port 9090 for rtt connections
```

Lector (otra terminal):

```bash
nc localhost 9090
```

```
=== printf por SWD (RTT): sin UART ni pines ===
[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C
48 caracteres: 1726 ciclos (17 us de CPU)
tick 0 (descartados 0)
tick 1 (descartados 0)
```

## No rompe el debugger

Es la pregunta obvia, y la respuesta es que **no**: el mismo OpenOCD sigue sirviendo gdb en el 3333
mientras el canal RTT está abierto. Comprobado parando y reanudando el micro por el canal de control
(puerto 4444) con el RTT conectado:

```
t= 1.9s  tick 365        ← corriendo
t= 2.1s  tick 366
t= 2.3s  tick 367
                         ← halt: el micro está frenado, no sale nada
t= 7.0s  tick 368        ← resume: sigue exactamente donde iba
t= 7.2s  tick 369
```

Los mensajes se cortan mientras el micro está frenado (lógico: no está ejecutando) y siguen al
reanudar, sin reconfigurar nada. Se puede depurar y mirar los `printf` al mismo tiempo, por el mismo
cable.

## Limitaciones, que son reales

- **Necesita el debugger conectado y OpenOCD corriendo.** En un equipo en el campo no tenés nada.
  Para producción, la UART sigue siendo la opción universal.
- **Si nadie lee el canal, la cola se llena y se descarta.** Igual que en la versión por DMA: el
  hardware no inventa ancho de banda. `rtt_perdidos()` lleva la cuenta.
- **El caudal depende de cada cuánto pollea el host**, no de un reloj fijo. No sirve para medir
  tiempos con precisión del lado de la PC.
- **No sirve para depurar el arranque temprano** si el bloque de control todavía no se inicializó.
- **No es reentrante.** Imprimir desde una ISR y desde el `main` a la vez corrompe la cola. Hay una
  red de seguridad opcional (`-DRTT_SEGURO_ISR=1`), pero protege *la cola*, no `printf`, que sigue
  sin ser reentrante.

## Lo que cuesta

| | |
|---|---|
| RAM | 1024 B de cola + ~64 B del bloque de control + 128 B de buffer de stdout |
| Flash | menos que la versión por UART: no hay que configurar ningún periférico |
| Configuración | `RTT_UP_SIZE`, `RTT_DOWN_SIZE`, `RTT_BLOQUEANTE`, `RTT_SEGURO_ISR` (se pisan con `-D`) |
| Pines | **ninguno** |
| Periféricos | **ninguno** |

## Ver también

- [`../MEDICIONES.md`](../MEDICIONES.md) §7 y §8 — los comandos para comprobar todo esto
- [`../printf_dma/`](../printf_dma/) — la versión por UART sin bloquear, para cuando no hay debugger
- [Módulo 12 - Debug](../../../12_debug/)
