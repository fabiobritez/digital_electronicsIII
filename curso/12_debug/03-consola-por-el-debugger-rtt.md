# La consola por el cable del debugger (RTT)

> **Objetivo.** Tener `printf` y entrada de teclado **sin gastar la UART, sin ocupar pines y sin
> conversor USB-serie**, usando el mismo cable que ya usás para grabar y depurar. Al final de esta
> guía vas a poder escribir `printf("adc=%d\n", v)` y verlo en una terminal con un solo comando,
> mientras seguís poniendo breakpoints.

Esta es la forma más cómoda de depurar en el laboratorio, y la que conviene usar de acá en adelante
para el resto del curso.

---

## 1. Por qué

Las dos formas anteriores de imprimir tienen un costo que ya medimos
([módulo 0, capítulo 16 §6](../00_lenguaje_c/16-redirigir-printf-a-uart.md)):

| | Necesita | Gasta | CPU por línea de 48 caracteres |
|---|---|---|---|
| `printf` por UART, polling | conversor USB-serie | 1 pin + UART0 | 4091 µs |
| `printf` por UART, DMA | conversor USB-serie | 1 pin + UART0 + 1 canal DMA | 36 µs |
| **`printf` por RTT** | **el debugger que ya tenés** | **nada** | **17 µs** |

Y no es solo velocidad: **no gastás la UART**. En un trabajo práctico donde la UART es *el tema*
(RS-485, un módulo GPS, un módem), no podés usarla además como consola de depuración. Con RTT no
tenés que elegir.

> **Ojo con esa tabla: mide un solo eje.** RTT gana en *costo de CPU*, que es lo que importa para no
> perturbar al programa. Pero en **caudal** pierde contra una UART a 921600, y por bastante. No son
> intercambiables: cada uno es mejor para algo distinto, y está desarrollado en la
> [sección 8](#8-entonces-conviene-rtt-o-la-uart). Si vas a elegir uno, leé eso primero.

## 2. Cómo funciona

```
   printf()  →  _write()  →  cola circular en RAM
                                   ↑
                          el debugger la lee por SWD
                          MIENTRAS el programa corre
```

La clave está en el Cortex-M3: la **unidad de debug es hardware separado del CPU** y puede leer
memoria sin frenar la ejecución. El micro no se entera de que le están mirando la RAM.

Por eso `printf` cuesta lo que cuesta un `memcpy` y nada más: no hay periférico, no hay espera, no
hay interrupción. Del otro lado, OpenOCD encuentra el bloque de control **escaneando la RAM** en
busca de la marca `"SEGGER RTT"`, y a partir de ahí lee la cola y la publica en un puerto TCP.

El formato es el de **SEGGER RTT** porque es el que ya entienden OpenOCD, J-Link y pyOCD. El código
del curso no usa la librería de SEGGER: son cien líneas y están en
[`ejemplos/uart/printf_rtt/rtt.c`](../ejemplos/uart/printf_rtt/rtt.c) para que las leas enteras.

---

## 3. Preparar Ubuntu 24.04

Verificado en Ubuntu 24.04.4 LTS con OpenOCD 0.12.0, que es el que viene en los repositorios.

```bash
sudo apt install openocd netcat-openbsd
openocd --version        # tiene que decir 0.12.0 o mayor
```

> **Por qué 0.12.0 o mayor:** los comandos `rtt setup` / `rtt server` aparecieron en OpenOCD 0.11.
> Con la versión de Ubuntu 24 andan sin tocar nada.

### Permisos del USB (esto es lo que más tarda a la gente)

Recién enchufada, la sonda pertenece a `root` y OpenOCD no la puede abrir:

```
Error: unable to find a matching CMSIS-DAP device
```

La reacción natural es `sudo openocd`, y **es la solución equivocada**: después no anda desde el
editor y te quedan archivos de root en el proyecto. La solución correcta son las reglas udev que ya
están en el repo:

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
    rtt_init();                 /* eso es todo lo que hay que configurar */

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

Y ya:

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

El canal es de ida y vuelta: lo que tecleás en esa misma terminal le llega al micro. `getchar()` y
`scanf()` funcionan, y `rtt_getchar()` te da un byte sin bloquear si preferís no usar la libc:

```c
int k = rtt_getchar();          /* -1 si no hay nada */
if (k == 'r') { reiniciar(); }
```

Es una consola de comandos gratis para tus prácticas.

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

Las otras dos son las que deciden el caudal, y los valores por defecto de OpenOCD son malos: con
`polling_interval` en 100 ms se sacan ~5.5 KB/s, **menos que una UART a 115200**. Con 10 ms y el SWD
a 4 MHz se llega a ~15.7 KB/s, que es el techo que impone la sonda. La plantilla ya usa los buenos;
se cambian con `make rtt RTT_POLL=100 RTT_SPEED=1000`. Está medido en
[`ejemplos/uart/printf_rtt/README.md`](../ejemplos/uart/printf_rtt/README.md).

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

Los mensajes se cortan mientras está frenado —lógico, no está ejecutando— y siguen al reanudar, sin
reconfigurar nada.

> **Truco para hard faults.** Poné `rtt_flush()` como primera línea de tu `HardFault_Handler`. Sin
> eso, el último `printf` antes del cuelgue todavía está en la cola y no lo ves nunca. `rtt_flush()`
> tiene un tope de espera, así que no se cuelga si no hay nadie leyendo.

---

## 7. ¿Y si uso MCUXpresso?

Es la pregunta correcta, porque mucha gente de la materia usa el IDE con sus breakpoints. Hay que
separar dos cosas.

### El código no rompe nada. Nunca.

`rtt.c` **no toca un solo registro del chip**: ni PCONP, ni PCLKSEL, ni PINSEL, ni el NVIC, ni un
timer. Es un arreglo en RAM y dos índices. Por lo tanto:

- **Los breakpoints siguen funcionando igual.** No hay forma de que los afecte.
- **El paso a paso, los watchpoints y la vista de registros, igual.**
- No consume interrupciones, así que no cambia latencias ni prioridades.
- No consume periféricos, así que no choca con nada de tu práctica.

Lo único que gasta es RAM: 1 KB de cola + ~64 bytes de bloque de control + 128 del buffer de stdout.

### Compila y linkea sin conflicto (verificado)

MCUXpresso usa **Redlib** por defecto, no newlib. Redlib no engancha `printf` por `_write` sino por
`__sys_write`. Por eso `rtt.c` define **los dos juegos de enganches**, el de newlib y el de Redlib:
en cada proyecto se usa uno y el linker descarta el otro con `--gc-sections`. No hay que configurar
nada.

¿No choca con el `__sys_write` que ya trae Redlib? **No**, y está verificado linkeando contra la
Redlib de MCUXpresso 11.10: dentro de `libcr_nohost.a` cada función vive en su propio objeto
(`__sys_write.o`), así que cuando tu código ya define el símbolo, el linker simplemente no saca ese
objeto del archivo. Linkeo limpio, gana tu versión.

```console
$ arm-none-eabi-nm t.elf | grep __sys_
00008014 T __sys_readc
00008010 T __sys_write
```

### Lo que sí tenés que saber

**1. Dos herramientas no pueden usar la sonda a la vez.** Mientras MCUXpresso tiene una sesión de
debug abierta, su LinkServer se queda con el probe y OpenOCD no puede entrar (y al revés). No es un
problema de RTT: es que hay un solo cable. Cerrá una sesión antes de abrir la otra.

**2. MCUXpresso no trae visor de RTT.** LinkServer no lo soporta. Con el IDE, entonces, tenés dos
opciones:

- Depurar en el IDE (breakpoints, paso a paso) y, cuando quieras la consola, cerrar la sesión y
  levantar `make rtt` desde la terminal.
- Mirar la cola a mano desde la vista **Memory** del IDE, apuntando al símbolo `up_buf`. Feo pero
  sirve para una mirada rápida.

**3. Si tu proyecto de MCUXpresso está en "Redlib (semihost)", los `printf` van a dejar de aparecer
en la consola del IDE.** No es un error: tu `__sys_write` reemplazó al de semihosting, así que el
texto ahora va a la cola RTT en vez de a la consola. Es exactamente lo que pediste, pero conviene
saberlo antes de pensar que se rompió algo. Si querés volver atrás, sacá `rtt.c` del proyecto.

**Recomendación práctica para la materia:** usá la plantilla y `make rtt` para el día a día, y
MCUXpresso cuando necesites sus breakpoints gráficos. Son dos herramientas para la misma placa, no
rivales.

---

## 8. ¿Entonces conviene RTT o la UART?

Es la pregunta que aparece apenas ves las dos tablas de números, porque parecen contradecirse:

| | CPU por línea de 48 caracteres |
|---|---:|
| UART por polling | 4091 µs |
| UART por DMA | 36 µs |
| **RTT** | **17 µs** ← gana RTT |

| | Caudal sostenido |
|---|---:|
| RTT afinado | 15 660 B/s |
| **UART a 921600** | **92 160 B/s** ← gana la UART |

No se contradicen: **están midiendo dos cosas distintas**, y la confusión es culpa de haber
presentado RTT como "la mejor" a secas cuando en realidad gana en un eje y pierde en otro.

### La diferencia de fondo: uno tiene motor, el otro no

Esto es lo que hay que entender, y todo lo demás sale de acá.

**La UART tiene un transmisor de hardware.** Vos la configurás una vez y a partir de ahí el
periférico saca bits solo, a un ritmo fijo dado por un reloj, sin que nadie se lo pida. Le cargás un
byte y se va. El caudal es una **garantía de hardware**: a 921600 baudios salen 92 160 bytes por
segundo, siempre, pase lo que pase en la PC.

**RTT no transmite nada.** Ahí no hay ningún periférico. El micro escribe en un arreglo en RAM y
listo — hasta ahí llegó su trabajo. Los bytes llegan a la PC **porque la PC va a buscarlos**:
OpenOCD, cada tanto, manda transacciones de lectura de memoria por el cable SWD y se trae lo que
haya. Si OpenOCD no pregunta, no llega nada.

> **La UART es una cinta transportadora andando sola a velocidad constante.**
> **RTT es alguien que pasa cada tanto con un balde.**

Por eso RTT le sale casi gratis al micro (escribir en RAM es un `memcpy`) y por eso su caudal
depende de con qué frecuencia y con qué balde pasa el host — no de un reloj.

### Por qué el caudal de RTT se planta en 15.7 KB/s

Ese techo **no lo pone el LPC1769 ni el cable SWD**. La prueba: subir el reloj del SWD de 4 a 15 MHz
no cambió el resultado ni un byte. Si el cuello fuera el SWD, habría mejorado.

Lo pone **la sonda**. La CMSIS-DAP de a bordo habla con la PC por **USB HID**, que en un puerto
full-speed mueve como mucho 64 bytes por milisegundo, y encima cada lectura de memoria necesita ida
y vuelta (pedido y respuesta). Los ~15.7 KB/s medidos son del orden de lo que da esa cuenta.

**Esto es importante para no sacar la conclusión equivocada: RTT no es lento *por ser RTT*.** Es
lento *con esta sonda*. Con una sonda que hable USB bulk a alta velocidad —un J-Link, o una
CMSIS-DAP v2— el techo es muchísimo más alto. Nuestro número es el de esta placa, no el de la
técnica.

### La comparación completa

| | UART por polling | UART por DMA | RTT |
|---|---|---|---|
| **CPU por línea (48 car.)** | 4091 µs | 36 µs | **17 µs** |
| **Caudal** | el del baudrate | **92 KB/s a 921600** | 15.7 KB/s (esta sonda) |
| **Latencia** | inmediata y **determinista** | inmediata | 185 ms ida y vuelta (polleo de 100 ms) |
| **Pines** | 1 (TXD) | 1 (TXD) | **ninguno** |
| **Periféricos** | UART0 | UART0 + 1 canal DMA | **ninguno** |
| **Hace falta** | conversor USB-serie (~$) | conversor USB-serie | **el debugger + OpenOCD corriendo** |
| **¿Anda sin PC?** | **sí** | **sí** | no |
| **¿Anda sin debugger?** | **sí** | **sí** | no |

### Para qué es mejor cada uno

**Usá RTT cuando:**

- Estás en el laboratorio con el debugger ya enchufado. Que es casi siempre, en esta materia.
- **No querés perturbar los tiempos.** 17 µs es lo mínimo que vas a conseguir. Si estás persiguiendo
  un problema de temporización, cualquier otra cosa te lo cambia.
- **Necesitás la UART para otra cosa.** Un TP de RS-485, un GPS, un módem: no podés gastarla además
  como consola. RTT no te obliga a elegir.
- Te faltan pines, o no tenés a mano un conversor USB-serie.
- Querés una consola interactiva (comandos por teclado) sin cablear nada más.

**Usá la UART cuando:**

- **Tenés que mover volumen de datos.** Subís a 921600 y tenés 92 KB/s garantizados: seis veces más
  que RTT con esta sonda.
- **Te importa *cuándo* llegó cada cosa.** La UART sale por un reloj de hardware; RTT depende de que
  el sistema operativo de tu PC decida correr el polleo. Si el dato tiene que estar sellado en el
  tiempo, la UART es más confiable.
- **El equipo va a funcionar sin PC ni debugger.** Un prototipo instalado en algún lado, una demo,
  una defensa de TP donde no querés depender de que OpenOCD arranque. Un conversor de dos dólares y
  cualquier terminal.
- Querés que un compañero lea la salida en su máquina sin instalar nada.
- Estás depurando el arranque muy temprano, antes de que `rtt_init()` haya corrido.

### La regla corta

> **RTT es la mejor consola. La UART es el mejor caño.**

RTT gana cuando lo que te importa es **no molestar** al programa. La UART gana cuando lo que te
importa es **sacar datos**. Y las dos pueden convivir en el mismo firmware: nada impide dejar RTT
para los mensajes de estado y la UART para volcar un buffer de muestras.

Y para los dos vale la misma advertencia, que es la de
[capítulo 16 §10](../00_lenguaje_c/16-redirigir-printf-a-uart.md): antes de llenar el código de
`printf`, hacé las dos cuentas. Ninguno de los dos caminos te salva de pedirle al enlace más de lo
que puede dar.

---

## 9. Cuando algo no anda

| Síntoma | Causa |
|---|---|
| `Error: unable to find a matching CMSIS-DAP device` | reglas udev sin instalar (§3), otra herramienta usando la sonda, o **la sonda quedó sin driver** (ver abajo) |
| `rtt: No control block found` | el firmware no llama a `rtt_init()`, o `rtt.c` no está en el proyecto |
| Conecta pero no sale nada | ¿estás seguro de que el programa llega a los `printf`? Mirá el LED, o poné un `printf` como primera línea de `main` |
| `descartados` crece sin parar | nadie está leyendo el canal, o imprimís más rápido de lo que el host pollea. Imprimí menos seguido o agrandá `RTT_UP_SIZE` |
| Se ve la mitad de la última línea | falta `rtt_flush()` antes de frenar o resetear |
| Anda solo con `sudo` | reglas udev (§3). No lo resuelvas con `sudo` |

### El caso raro: "no encuentro la sonda" y sin embargo está enchufada

Si `lsusb` la muestra pero OpenOCD dice que no la encuentra, mirá si la interfaz USB quedó sin
dueño:

```bash
cat /sys/bus/usb/devices/1-3/1-3:1.0/uevent | grep DRIVER
```

(el `1-3` puede ser otro; sale de `lsusb -t`). Si no dice `DRIVER=usbhid`, ese es el problema.

**Por qué pasa.** La sonda CMSIS-DAP es un dispositivo USB **HID**, y al enchufarla el kernel le ata
su driver `usbhid`. OpenOCD necesita hablarle en crudo, así que le pide al kernel que **suelte** esa
interfaz, y se la devuelve al cerrarse. Si OpenOCD muere antes de ese último paso —lo mataste con
`kill -9`, se colgó, o suspendiste la máquina con una sesión abierta— la interfaz queda huérfana:
enumerada pero sin driver. Y sin driver no hay nodo `/dev/hidraw*`, que es justo por donde OpenOCD
la busca. De ahí el mensaje engañoso: la ve, pero no la puede abrir.

**Cómo se arregla.** Desenchufar y volver a enchufar el USB del debugger. Fuerza una re-enumeración
completa y el kernel le ata `usbhid` de nuevo. Sin desenchufar, también sirve:

```bash
sudo sh -c 'echo -n 1-3:1.0 > /sys/bus/usb/drivers/usbhid/bind'
```

**Cómo evitarlo.** Salí de `make rtt` con `Ctrl-C`, que deja a OpenOCD cerrarse ordenadamente. Nada
de `kill -9` ni `pkill`.

**No te preocupes por el hardware:** no se daña nada. Es puro estado del lado de la PC —qué driver
es dueño de una interfaz USB—, no toca ni el firmware de la sonda ni el LPC1769 ni lo que tenés
grabado. La falla ocurre *antes* de conectarse al chip, así que ni siquiera llega a escribir.

---

## 10. Cuándo **no** usar RTT

Es cómodo, pero no es universal:

- **Necesita el debugger conectado y OpenOCD corriendo.** En un equipo instalado en el campo no
  tenés nada. Para eso está la UART, que anda con un conversor de pocos pesos y sin PC de por medio.
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

## 11. Sobre el SWO, que es lo que todo el mundo nombra primero

El camino "de manual" para imprimir por el debugger es **ITM/SWO**: el Cortex-M3 tiene una unidad de
trazado que saca caracteres por el pin SWO, que en el LPC1769 comparte pin con TDO (UM10360 §33.4),
o sea que tampoco cuesta pines.

**Pero la sonda de a bordo de la LPCXpresso no lo soporta.** Su firmware CMSIS-DAP es viejo:

```console
$ openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

Fijate lo que **no** dice: `SWO-UART supported`. Si insistís, OpenOCD contesta
`Error: SWO-trace is not supported by the device`.

Para usar SWO hace falta otra sonda (J-Link, ST-Link V2/V3, o MCU-Link / LPC-Link2 con firmware
CMSIS-DAP v2). Es un caso claro de **el chip puede, la herramienta no**, y por eso el curso usa RTT:
funciona con la sonda que ya tenés y encima es más rápido.

---

## Ejercicios

1. **Consola de comandos.** Usando `rtt_getchar()`, hacé que tu programa responda a teclas: `l`
   prende y apaga el LED, `a` imprime la lectura del ADC, `r` reinicia un contador. Sin gastar un
   solo pin.
2. **El costo real.** Medí con `DWT->CYCCNT` cuánto tarda un `printf` por RTT en tu placa
   (los comandos están en [`ejemplos/uart/MEDICIONES.md`](../ejemplos/uart/MEDICIONES.md) §5) y
   comparalo con los 4091 µs de la versión por UART. ¿Cuánto de lo que queda es formatear?
3. **Saturación.** Imprimí en un lazo cerrado, sin delay, y mirá cómo crece `rtt_perdidos()`.
   Después agrandá la cola con `-DRTT_UP_SIZE=4096` y volvé a probar. ¿Se arregla o solo se
   posterga?
4. **Sobrevivir al cuelgue.** Poné un `printf("justo antes\n")` seguido de una división por cero
   (que dispara el HardFault). Comprobá que el mensaje **no** aparece. Después agregá `rtt_flush()`
   como primera línea del `HardFault_Handler` y verificá que sí.

---

**Módulo:** [Debug](./README.md) ·
**Anterior:** [02 - El debugger y un método](./02-debugger-y-metodo.md)

**Ver también:** [Ejemplo completo](../ejemplos/uart/printf_rtt/) ·
[Cómo reproducir las mediciones](../ejemplos/uart/MEDICIONES.md) ·
[Módulo 0, capítulo 16 - printf a la UART](../00_lenguaje_c/16-redirigir-printf-a-uart.md)
