# La consola por el cable del debugger (RTT)

> **Objetivo.** Usar `printf` y recibir comandos sin ocupar una UART ni pines de aplicación,
> mediante la misma sonda que se usa para grabar y depurar.

RTT es una opción práctica mientras el equipo permanece conectado a la PC. No reemplaza a la UART
en todos los casos: al final del capítulo comparamos sus costos y límites.

---

## 1. Por qué

Las siguientes mediciones corresponden al LPC1769 a 100 MHz, la implementación de este repositorio
y una línea de 48 caracteres
([procedimiento](../../curso/ejemplos/uart/MEDICIONES.md)):

| Salida | Recursos de la aplicación | Tiempo de CPU medido |
|---|---|---:|
| `_DBG` del Debug Framework | UART0 y TXD | 4005 µs |
| `_DBG` mejorado, UART con DMA | UART0, TXD, cola y un canal DMA | 5,90 µs |
| `printf` por UART, polling | UART0 y TXD | 4091 µs |
| `printf` por UART, DMA | UART0, TXD y un canal DMA | 36 µs |
| `printf` por RTT | unos 1,2 KiB de RAM y la interfaz de debug | 17 µs |

Estos números no describen a toda UART ni a toda implementación RTT. Muestran que las macros del
Debug Framework original no cambian el costo del transporte: `_DBG` y `printf` por polling esperan
a la misma UART. La [versión mejorada](../../curso/ejemplos/uart/debug_framework_mejorado/) conserva
las macros, pero cambia el backend. Encolar el texto libera antes al CPU; RTT, además, deja libre la
UART. El caudal hacia la PC depende del baudrate o de la sonda y el software del host.

## 2. Cómo funciona

```
   printf()  →  _write()  →  cola circular en RAM
                                   ↑
                          el debugger la lee por SWD
                          MIENTRAS el programa corre
```

El puerto de debug puede acceder a la memoria mientras el núcleo ejecuta. El firmware coloca los
datos en una cola circular y el host actualiza el índice de lectura. No hace falta detener el CPU,
aunque las lecturas de la sonda comparten buses y pueden introducir algo de contención.

El costo del lado del micro incluye el formato de `printf`, la copia a la cola y su sincronización.
No incluye el tiempo que el host tarda en retirar los datos. OpenOCD busca el bloque de control por
su identificador `"SEGGER RTT"` y expone el canal mediante un puerto TCP.

La estructura es compatible con el formato **SEGGER RTT**, que también pueden interpretar
herramientas como OpenOCD, J-Link y pyOCD. La implementación usada en el curso está en
[`rtt.c`](../../curso/ejemplos/uart/printf_rtt/rtt.c) y no depende de la biblioteca de SEGGER.

---

## 3. Preparar Ubuntu 24.04

Esta receta se verificó con Ubuntu 24.04.4 LTS y OpenOCD 0.12.0. En otra distribución, los paquetes
y su versión pueden cambiar.

```bash
sudo apt install openocd netcat-openbsd
openocd --version
```

La guía usa los comandos `rtt setup`, `rtt start` y `rtt server`. Antes de seguir, comprobá que
tu versión de OpenOCD los incluya.

### Permisos del USB

Sin una regla de permisos adecuada, el nodo USB de la sonda puede quedar inaccesible para tu
usuario:

```
Error: unable to find a matching CMSIS-DAP device
```

Ejecutar OpenOCD como `root` puede servir como diagnóstico, pero no como configuración permanente.
Instalá la regla udev incluida en el repositorio:

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Desenchufá y volvé a enchufar la placa. Comprobá que la ve sin `sudo`:

```bash
openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
```

Tenés que ver `Info : CMSIS-DAP: SWD supported`.

---

## 4. Usarlo: tres pasos

### Paso 1: agregá los dos archivos a tu proyecto

```bash
cd plantilla
cp ../curso/ejemplos/uart/printf_rtt/rtt.h src/
cp ../curso/ejemplos/uart/printf_rtt/rtt.c src/
```

### Paso 2: llamá a `rtt_init()` antes del primer `printf`

```c
#include <stdio.h>
#include "rtt.h"

int main(void)
{
    rtt_init();                 /* inicializa el canal RTT */

    printf("arranque ok\n");

    while (1) {
        printf("adc=%d\n", leer_adc());
        ...
    }
}
```

No hay que configurar pines, ni clock, ni baudrate, ni periférico. `rtt_init()` no toca ningún
registro del chip: solo arma una estructura en RAM.

### Paso 3: compilá, grabá y mirá

```bash
make USE_CMSIS=1 flash
make rtt
```

La salida esperada es:

```
  RTT     puerto 9090, SWD 4000 kHz, polleo 10 ms; Ctrl-C para salir

Info : rtt: Control block found at 0x10000244

=== consola por el debugger (RTT) ===
[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C
48 caracteres: 1726 ciclos (17 us de CPU)
tick 0 (descartados 0, pendientes 0)
tick 1 (descartados 0, pendientes 0)
```

Se sale con `Ctrl-C`.

### También podés escribirle

El canal es bidireccional: lo que escribís en la terminal llega a la cola de bajada.
`rtt_getchar()` devuelve un byte disponible sin bloquear:

```c
int k = rtt_getchar();          /* -1 si no hay nada */
if (k == 'r') { reiniciar(); }
```

La implementación también redirige las funciones de entrada de la libc. Como `_read()` es no
bloqueante, para una consola interactiva suele ser más claro consultar `rtt_getchar()` y armar el
comando de forma incremental.

---

## 5. Qué hace `make rtt` por debajo

Conviene saberlo, porque el día que algo falle vas a querer correrlo a mano:

```bash
# terminal 1: el servidor
openocd -f openocd/lpc1769.cfg \
  -c "adapter speed 4000" \
  -c "init" -c "reset run" \
  -c 'rtt setup 0x10000000 0x8000 "SEGGER RTT"' \
  -c "rtt polling_interval 10" \
  -c "rtt start" \
  -c "rtt server start 9090 0"

# terminal 2: el cliente
nc localhost 9090
```

Los argumentos de `rtt setup` son **dónde buscar** el bloque de control: dirección de inicio, cuántos
bytes barrer, y la marca. `0x10000000 0x8000` son los 32 KB de RAM del LPC1769: barre toda la RAM.

El intervalo de consulta y la velocidad SWD influyen en el caudal. Con la sonda y el host usados en
las pruebas se midieron unos 5,5 kB/s a 100 ms y 1 MHz, y 15,7 kB/s a 10 ms y 4 MHz. Son resultados
del montaje, no límites de RTT. La plantilla usa la segunda configuración; podés cambiarla con
`make rtt RTT_POLL=100 RTT_SPEED=1000`. El procedimiento completo está en el
[`README` del ejemplo](../../curso/ejemplos/uart/printf_rtt/README.md).

### Dos cosas que `make rtt` hace y conviene saber

- **Resetea el micro** (`reset run`), así que tu programa arranca de cero en cada `make rtt`. Es a
  propósito: si no, te perderías los primeros `printf`.
- **No graba.** Si editaste el código, `make rtt` recompila el `.elf` pero la placa sigue corriendo
  el firmware viejo. El hábito seguro es `make flash && make rtt`.

---

## 6. Depurar y ver los `printf` al mismo tiempo

Sí, y por el mismo cable. El servidor RTT y el `gdbserver` conviven en la misma instancia de OpenOCD:

```bash
make rttserver          # levanta RTT en el 9090 y gdb en el 3333
```

Desde otra terminal te conectás con gdb al 3333 como siempre, y desde una tercera hacés
`nc localhost 9090`. Comprobado parando y reanudando el micro con el canal abierto:

```
t= 2.3s  tick 367        ← corriendo
                         ← halt: el micro está frenado, no sale nada
t= 7.0s  tick 368        ← resume: sigue exactamente donde iba
```

Los mensajes se cortan mientras está frenado (lógico, no está ejecutando) y siguen al reanudar, sin
reconfigurar nada.

> Antes de un reset o dentro de un handler de fault, `rtt_flush()` da al host una oportunidad de
> consumir lo pendiente. Tiene un límite de espera; aun así, no conviene depender del texto como
> único registro de un fallo crítico.

---

## 7. ¿Y si uso MCUXpresso?

El código RTT no configura periféricos: reserva dos colas en RAM y define los enganches de entrada y
salida de la libc. Por eso puede incorporarse a un proyecto de MCUXpresso, pero no es totalmente
transparente: consume memoria, el formateo usa CPU y el acceso de la sonda agrega tráfico al bus de
debug.

### Biblioteca C

La implementación define los símbolos de newlib (`_write` y `_read`) y de Redlib
(`__sys_write`, `__sys_write0` y `__sys_readc`). Se verificó con Redlib de MCUXpresso 11.10:
cuando el proyecto aporta esos símbolos, el linker no extrae las versiones de `libcr_nohost.a`.

Si cambiás de biblioteca o de versión, comprobá el mapa de enlace y los símbolos:

```bash
arm-none-eabi-nm build/firmware.elf | grep -E '_write|_read'
```

Si el proyecto usaba semihosting, sus mensajes dejarán de aparecer en la consola correspondiente
porque ahora esas funciones escriben en RTT. Para volver al comportamiento anterior, quitá
`rtt.c` o seleccioná de forma explícita otra retargetización.

### Servidor y sonda

En general, una sola aplicación puede controlar la sonda a la vez. Mientras una sesión de
MCUXpresso usa LinkServer, OpenOCD no puede abrir el mismo dispositivo, y viceversa.

El flujo verificado para esta guía usa OpenOCD como servidor RTT y GDB. MCUXpresso 11.10 no ofrecía
un visor RTT integrado con LinkServer; una versión posterior podría cambiarlo. Si el IDE que usás
incorpora soporte RTT, seguí su documentación. De lo contrario, cerrá su sesión antes de ejecutar
`make rtt`, o depurá mediante la instancia de OpenOCD iniciada con `make rttserver`.

## 8. ¿Entonces conviene RTT o la UART?

Son herramientas distintas. RTT encola datos en RAM y espera que el host los retire por el puerto de
debug. La UART entrega bytes a un periférico que los transmite con su propio reloj.

### Resultados del banco de pruebas

| Salida | Tiempo hasta volver, 48 B | Caudal de la configuración | Recursos principales |
|---|---:|---:|---|
| Debug Framework original, 115200 | 4005 µs | 11 513 B/s | UART y TXD |
| `printf` por UART y polling, 115200 | 4091 µs | cerca de 11 520 B/s | UART, TXD y libc |
| `printf` por UART y DMA, 115200 | 36 µs | cerca de 11 520 B/s | UART, TXD, RAM y DMA |
| Debug Framework mejorado, DMA a 115200 | **5,90 µs** | 11 513 B/s | UART, TXD, 2 KiB y DMA |
| Debug Framework mejorado, DMA a 921600 | **5,84 µs** | **92 104 B/s** | UART, TXD, 2 KiB y DMA |
| `printf` por RTT | 17 µs | 15 660 B/s | RAM e interfaz de debug |

Los tiempos y los caudales indicados se obtuvieron con el LPC1769 y el código del repositorio. En el
caso del Debug Framework original, 48 000 bytes enviados en forma consecutiva ocuparon 4 168 979
µs, lo que da 11 513 B/s. La versión mejorada a 921600 tardó 521 149 µs y alcanzó 92 104 B/s sin
descartar datos en la prueba controlada. El caudal útil también depende del receptor y del software
del host.

El techo de 15,7 kB/s dejó de mejorar al subir SWD por encima de 4 MHz. Eso indica que, en este
montaje, el límite estaba en la sonda o en las transacciones con el host, no en la cola del
LPC1769. Otra sonda, otro transporte USB o una implementación distinta pueden dar otro resultado.

### Qué cambia en la práctica

| Aspecto | UART | RTT |
|---|---|---|
| Momento de transmisión | regido por el baudrate | regido por el sondeo del host |
| Pines | al menos TXD; RXD si es bidireccional | no usa pines de aplicación |
| Funcionamiento sin lector | el periférico transmite igual | la cola se llena y descarta o bloquea |
| Arranque temprano | disponible después de inicializar la UART | disponible después de `rtt_init()` |
| Riesgo principal | bloquear o saturar la salida serie | perder mensajes o depender de la sonda |
| Uso típico | salida independiente, flujo sostenido o interfaz del producto | diagnóstico durante el desarrollo |

Elegí RTT si necesitás liberar la UART y querés una consola sencilla durante el desarrollo. Elegí
UART con DMA si necesitás un canal independiente del debugger, un ritmo definido por hardware o
mayor caudal. Si ya tenés ejemplos con `_DBG`, la versión mejorada permite conservar esas macros;
el framework original sigue siendo apropiado solo para mensajes esporádicos sin requisitos de
tiempo. Para medir tiempos, marcá cada dato en el micro: la hora de llegada a la terminal no
representa necesariamente el instante del evento.

Ambas vías pueden convivir. Por ejemplo, RTT puede llevar mensajes de estado y la UART un flujo de
muestras. En los dos casos calculá el caudal requerido y definí qué hacer cuando el consumidor no
alcanza.

## 9. Cuando algo no anda

| Síntoma | Causa |
|---|---|
| `Error: unable to find a matching CMSIS-DAP device` | reglas udev sin instalar (§3), otra herramienta usando la sonda, o **la sonda quedó sin driver** (ver abajo) |
| `rtt: No control block found` | el firmware no llama a `rtt_init()`, o `rtt.c` no está en el proyecto |
| Conecta pero no sale nada | ¿estás seguro de que el programa llega a los `printf`? Mirá el LED, o poné un `printf` como primera línea de `main` |
| `descartados` crece sin parar | nadie está leyendo el canal, o imprimís más rápido de lo que el host pollea. Imprimí menos seguido o agrandá `RTT_UP_SIZE` |
| Falta la última línea | estaba en el buffer de `stdio` o en la cola al detener o resetear; usá `fflush(stdout)` y, si corresponde, `rtt_flush()` |
| Anda solo con `sudo` | reglas udev (§3). No lo resuelvas con `sudo` |

### La sonda aparece en USB, pero OpenOCD no la abre

Primero distinguí enumeración de permisos y de ocupación:

```bash
lsusb
lsusb -t
ls -l /dev/hidraw*
```

Si aparece en `lsusb` pero no existe el nodo esperado o no tiene un driver asociado, desenchufá y
volvé a enchufar la sonda. Si el problema empezó después de cerrar OpenOCD de forma forzada o de
suspender la PC, la reenumeración suele restaurar el estado del driver.

Si el nodo existe, revisá la regla udev y comprobá que ninguna otra sesión de OpenOCD, LinkServer o
IDE tenga abierta la sonda. Cerrá los servidores de forma normal con `Ctrl-C`; evitá corregir el
problema ejecutando todo como `root`.

## 10. Cuándo **no** usar RTT

Es cómodo, pero no es universal:

- **Para ver la salida necesita una sonda y un lector como OpenOCD o pyOCD.** El firmware puede
  seguir ejecutándose sin ellos, pero la cola se llena y aplica la política configurada.
- **Si nadie lee, se descarta.** Es a propósito: en depuración es preferible perder texto a frenar
  el programa que estás tratando de observar. `rtt_perdidos()` lleva la cuenta. Si preferís lo
  contrario, compilá con `-DRTT_BLOQUEANTE=1`, sabiendo que sin nadie leyendo el programa se cuelga.
- **El caudal depende de cada cuánto pollea el host**, no de un reloj fijo. No sirve para medir
  tiempos con precisión desde la PC.
- **No sirve para el arranque muy temprano**, antes de `rtt_init()`.
- **No es reentrante.** Imprimir desde una ISR y desde el `main` a la vez corrompe la cola. Hay una
  red de seguridad (`-DRTT_SEGURO_ISR=1`, que protege con una sección crítica), pero ojo: eso hace
  segura *la cola*, no `printf`, que sigue sin ser reentrante. La regla del módulo sigue siendo la
  misma: **la ISR levanta una bandera y el `main` imprime.**

## 11. SWO: la alternativa más conocida

El camino "de manual" para imprimir por el debugger es **ITM/SWO**: el Cortex-M3 tiene una unidad de
trazado que saca caracteres por el pin SWO, que en el LPC1769 comparte pin con TDO (UM10360 §33.4),
o sea que tampoco cuesta pines. Cómo funciona por dentro está en
[02-03](../02_protocolos_y_debug_en_el_chip/03-adentro-del-cortex-m3.md), y la comparación con las
otras vías en [02-04](../02_protocolos_y_debug_en_el_chip/04-vias-de-salida-de-datos.md).

**La sonda de a bordo de la placa probada no captura SWO.** OpenOCD informó estas capacidades:

```console
$ openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

Fijate lo que **no** dice: `SWO-UART supported`. Si insistís, OpenOCD contesta
`Error: SWO-trace is not supported by the device`.

Para usar SWO hace falta una sonda, un conector y un firmware que declaren explícitamente esa
capacidad. Que una sonda use CMSIS-DAP v2 no la garantiza: SWO es opcional. Este caso muestra por qué
hay que comprobar por separado las capacidades del chip, de la placa y de la sonda. Con el hardware
del curso, RTT permite reutilizar el acceso SWD disponible.

---

## Ejercicios

1. **Consola de comandos.** Usando `rtt_getchar()`, hacé que tu programa responda a teclas: `l`
   prende y apaga el LED, `a` imprime la lectura del ADC, `r` reinicia un contador. Sin gastar un
   solo pin.
2. **El costo real.** Medí con `DWT->CYCCNT` cuánto tarda un `printf` por RTT en tu placa
   (los comandos están en [`ejemplos/uart/MEDICIONES.md`](../../curso/ejemplos/uart/MEDICIONES.md) §5) y
   comparalo con los 4091 µs de la versión por UART. ¿Cuánto de lo que queda es formatear?
3. **Saturación.** Imprimí en un lazo cerrado, sin delay, y mirá cómo crece `rtt_perdidos()`.
   Después agrandá la cola con `-DRTT_UP_SIZE=4096` y volvé a probar. ¿Se arregla o solo se
   posterga?
4. **Conservar el último mensaje.** Generá un fault controlado. Por ejemplo, habilitá
   `DIV_0_TRP` y dividí dos variables `volatile` con divisor cero. Compará qué llega al host con
   y sin `fflush(stdout)` y `rtt_flush()`. No asumas el resultado: depende de cuándo el host leyó
   la cola.

---

**Depurar en serio:** [índice](./README.md) ·
**Anterior:** [03 - Hard faults](./03-hard-faults.md) ·
**Siguiente parte:** [07 - LPC1769](../07_lpc1769/)

**Ver también:** [Ejemplo completo](../../curso/ejemplos/uart/printf_rtt/) ·
[Medición del Debug Framework](../../curso/ejemplos/uart/debug_framework/) ·
[Debug Framework mejorado](../../curso/ejemplos/uart/debug_framework_mejorado/) ·
[Cómo reproducir las mediciones](../../curso/ejemplos/uart/MEDICIONES.md) ·
[05 - Redirigir `printf` a la UART](./05-redirigir-printf-a-uart.md)
