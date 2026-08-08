# Cómo reproducir las mediciones

Todos los números que aparecen en [el capítulo 16](../../00_lenguaje_c/16-redirigir-printf-a-uart.md)
y en los README de esta carpeta salieron de correr los comandos de acá, en una **LPCXpresso LPC1769
(OM13085)** con su sonda CMSIS-DAP de a bordo y un conversor USB-serie CP2102 en P0.2/P0.3.

No hace falta creerle a la tabla: corré esto y comprobá.

> **Herramientas usadas:** `arm-none-eabi-gcc` 13.2, `openocd` 0.12.0, `minicom` 2.9, `python3`.
> Instalación en [el anexo B](../../anexos/B_toolchain_y_entorno/).

---

## 0. Antes de empezar: quién tiene el puerto

El error más común de todos, y no tiene nada que ver con el firmware. **Un puerto serie le entrega
cada byte a un solo lector.** Si dejás un `cat` colgado y después abrís minicom, se reparten el texto
y ves algo que parece corrupción pero no lo es.

```bash
ls -l /proc/*/fd/* 2>/dev/null | grep ttyUSB0     # ¿quién lo tiene abierto?
```

Si aparece algo, cerralo antes de seguir.

---

## 1. Compilar, grabar y mirar la salida

```bash
cd plantilla

# el ejemplo por polling
cp ../curso/ejemplos/uart/printf_retarget.c src/main.c
make USE_CMSIS=1 flash

# el ejemplo por DMA (son tres archivos)
cp ../curso/ejemplos/uart/printf_dma/dbg_uart.h src/
cp ../curso/ejemplos/uart/printf_dma/dbg_uart.c src/
cp ../curso/ejemplos/uart/printf_dma/main.c     src/
make USE_CMSIS=1 flash
```

`USE_CMSIS=1` no es opcional: sin `SystemInit()` el micro corre a 4 MHz y 115200 no se puede generar.

Para ver la salida:

```bash
minicom -D /dev/ttyUSB0 -b 115200            # salir: Ctrl-A luego X
```

Sin minicom, solo con herramientas base:

```bash
stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb raw -echo
cat /dev/ttyUSB0
```

Para mandarle datos a la placa desde otra terminal (prueba el camino RX):

```bash
printf 'hola' > /dev/ttyUSB0
```

---

## 2. Flash y RAM estática

```bash
cd plantilla
make USE_CMSIS=1
arm-none-eabi-size build/firmware.elf
```

`text` = código y constantes (Flash); `data` = globales con valor inicial (ocupa Flash **y** RAM);
`bss` = globales en cero (solo RAM).

Cuidado con dos cosas al leer esa salida:

- `size` mete en la columna `bss` **todas** las secciones sin contenido, incluidos los 2 KB que el
  linker reserva para el stack y la cola en AHB SRAM del ejemplo de DMA. Para ver el desglose real:

  ```bash
  arm-none-eabi-objdump -h build/firmware.elf | grep -E '\.text|\.data|\.bss|ahbram|heap'
  ```

- Para saber **dónde** quedó una variable (por ejemplo, comprobar que la cola del DMA cayó en la AHB
  SRAM en `0x2007C000` y no en la RAM principal):

  ```bash
  arm-none-eabi-nm -S build/firmware.elf | grep cola
  ```

Comparar dos configuraciones (la tabla de la sección 6.1 del capítulo 16):

```bash
make clean && make USE_CMSIS=1                                   # newlib-nano (por defecto)
make clean && make USE_CMSIS=1 EXTRA_LDFLAGS="-u _printf_float"  # + soporte de %f
```

Para la fila de newlib completa hay que cambiar `--specs=nano.specs` por `--specs=nosys.specs` en
el `LDFLAGS` del Makefile.

---

## 3. Cuánto stack usa de verdad una función

`size` no te lo dice: el stack se gasta en tiempo de ejecución. Se mide **pintando la pila**.

La idea: rellenar la memoria libre con un patrón conocido, correr lo que querés medir, y buscar
hasta dónde llegó a pisarse el patrón. Esa es la marca de agua.

```c
extern char end;                    /* fin de .bss, lo define el linker script */
#define ESTACK  0x10008000u         /* tope de la SRAM = SP inicial */
#define PATRON  0xAAu

static uint32_t base;               /* desde dónde pintar (ver la trampa de abajo) */

static void pintar(void)
{
    volatile uint32_t aqui;
    uint8_t *p    = (uint8_t *) base;
    uint8_t *tope = (uint8_t *) ((uint32_t) &aqui - 64u);   /* no pisar el frame actual */
    while (p < tope) { *p++ = PATRON; }
}

static uint32_t marca_de_agua(void)
{
    uint8_t *p = (uint8_t *) base;
    while (*p == PATRON) { p++; }
    return ESTACK - (uint32_t) p;   /* bytes tocados desde el tope */
}
```

**La trampa:** si pintás desde `&end`, estás pintando **encima del heap**. `printf` con `%f` llama a
`malloc`, así que le corrompés los datos y la medición da cualquier cosa (la primera vez dio 31928
bytes, o sea "toda la RAM"). Hay que empezar a pintar por arriba del heap:

```c
base = ((uint32_t) _sbrk(0) + 7u) & ~7u;
```

Y antes de pintar, un "calentamiento" (`printf` una vez) para que toda la inicialización perezosa de
la libc y sus `malloc` ya hayan ocurrido.

---

## 4. Cuánto heap usa `printf`

El heap tampoco aparece en `make size`. Se lee del propio asignador:

```c
extern char end;
extern void *_sbrk(ptrdiff_t);

uint32_t heap_usado(void) { return (uint32_t) _sbrk(0) - (uint32_t) &end; }
```

Imprimí eso antes y después del primer `printf`, con y sin `setvbuf`. Con buffering por defecto vas
a ver el salto de **1032 bytes** que newlib reserva para el buffer de stdout.

---

## 5. Cuánto tiempo tarda: el contador de ciclos

El Cortex-M3 tiene un contador de ciclos por hardware (DWT). A 100 MHz, un ciclo son 10 ns.

```c
#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

DEMCR |= (1u << 24);        /* TRCENA: habilita el bloque de traza */
DWT_CYCCNT = 0;
DWT_CTRL  |= 1u;            /* arrancar el contador */

uint32_t t0 = DWT_CYCCNT;
printf("lo que quiero medir\n");
uint32_t ciclos = DWT_CYCCNT - t0;
```

**Dos precauciones que cambian el resultado:**

1. **Drená la UART antes de cada medida** (`while (!(LPC_UART0->LSR & (1u<<6))) { }`, esperando
   `TEMT`). Si no, la medición arrastra el carácter anterior que todavía se está transmitiendo, y te
   da ~87 µs de más a 115200.
2. **Usá una línea de largo realista** (40-60 caracteres). Con textos muy cortos la FIFO de 16 bytes
   de la UART se traga casi todo y la versión por polling parece más rápida de lo que es.

Para separar el costo de *formatear* del costo de *transmitir*, medí un `sprintf` a un buffer (que
no toca la UART) contra el `printf` completo. Esa comparación es la que muestra que el 98% del
tiempo se va en el cable.

---

## 6. Subir el baudrate

Los divisores no se adivinan: se calculan. Este script hace el mismo barrido que `UART_Init` del
driver de NXP, respetando las restricciones del manual (`DIVADDVAL < MULVAL`, y si el fraccional
está activo con `DLM = 0` entonces `DLL > 2`):

```python
def mejor(pclk, baud):
    best = None
    for dl in range(1, 81):
        for mul in range(1, 16):
            for add in range(0, mul):
                if add > 0 and dl <= 2: continue
                real = pclk / (16 * dl * (1 + add/mul))
                err = abs(real - baud) / baud
                if best is None or err < best[0]:
                    best = (err, dl, mul, add, real)
    return best

for b in (115200, 230400, 460800, 921600, 1000000):
    print(b, mejor(100_000_000, b))
```

Para pasar `PCLK_UART0` de 25 MHz a 100 MHz (necesario por encima de 460800):

```c
LPC_SC->PCLKSEL0 &= ~(0x3u << 6);
LPC_SC->PCLKSEL0 |=  (0x1u << 6);   /* 01 = CCLK/1 */
```

Y del lado de la PC:

```bash
stty -F /dev/ttyUSB0 921600 cs8 -cstopb -parenb raw -echo
cat /dev/ttyUSB0
```

**Verificá la integridad, no te conformes con que "se lee".** Hacé que el firmware repita una línea
conocida y contá cuántas llegan mal:

```bash
timeout 5 cat /dev/ttyUSB0 > salida.log
grep -ac "^tick " salida.log
grep -a "^tick " salida.log | grep -avc "^tick [0-9]* 0123456789abcdefghijklmnopqrstuvwxyz$"
```

(El `-a` es necesario: si hay corrupción, el archivo tiene bytes no imprimibles y `grep` lo trata
como binario y no cuenta nada.)

Resultados en este hardware: **921600 → 1850 líneas, 0 corruptas. 1000000 → 520 líneas, 183
corruptas (35%)**, aun con divisor exacto. El límite no está en el LPC.

---

## 7. Qué sabe hacer tu sonda

Antes de perder una tarde intentando usar SWO, preguntale a la sonda qué soporta:

```bash
openocd -f interface/cmsis-dap.cfg -c "transport select swd" -c init -c exit
```

La sonda de la LPCXpresso responde solo:

```
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: FW Version = 1.0
```

Sin `SWO-UART supported` ni `SWO-Manchester supported`. Si igual insistís:

```
Error: SWO-trace is not supported by the device.
```

---

## 8. `printf` por el debugger (RTT), sin UART

Funciona con esta misma sonda, porque no usa hardware de traza: OpenOCD lee un buffer de RAM por
SWD **mientras el programa corre**. Ver [`printf_rtt/`](./printf_rtt/).

Con la plantilla es un solo comando:

```bash
make rtt
```

Y esto es lo que hace por debajo, que conviene saber para el día que falle:

```bash
# 1) servidor: encuentra el bloque de control y publica el canal en el 9090
openocd -f openocd/lpc1769.cfg \
  -c "init" -c "reset run" \
  -c 'rtt setup 0x10000000 0x8000 "SEGGER RTT"' \
  -c "rtt start" \
  -c "rtt server start 9090 0"
```

Deberías ver:

```
Info : rtt: Searching for control block 'SEGGER RTT'
Info : rtt: Control block found at 0x10000244
Info : Listening on port 9090 for rtt connections
```

```bash
# 2) en otra terminal, leer el canal
nc localhost 9090
```

**Y no rompe el debugger**: el mismo OpenOCD sigue sirviendo gdb en el 3333. Se puede comprobar
parando y reanudando el micro por el canal de control (puerto 4444) mientras el RTT sigue abierto:

```bash
nc localhost 4444
> halt
> reg pc
> resume
```

Los mensajes se cortan mientras el micro está frenado y siguen exactamente donde iban al reanudar,
sin reconfigurar nada.

---

## 9. Medir el rendimiento de RTT

Hay un banco de pruebas completo: [`printf_rtt/bench.c`](./printf_rtt/bench.c) del lado del micro y
[`printf_rtt/medir_rtt.py`](./printf_rtt/medir_rtt.py) del lado de la PC.

```bash
cd plantilla
cp ../curso/ejemplos/uart/printf_rtt/rtt.h    src/
cp ../curso/ejemplos/uart/printf_rtt/rtt.c    src/
cp ../curso/ejemplos/uart/printf_rtt/bench.c  src/main.c
make USE_CMSIS=1 flash
```

**Informe completo automático** (levanta OpenOCD, mide y cierra todo):

```bash
python3 ../curso/ejemplos/uart/printf_rtt/medir_rtt.py
```

**A mano, interactivo:**

```bash
make rtt
```

y ahí tecleás: `1` costo de CPU, `2` caudal sostenido, `3` ráfaga a fondo, `0` quieto.

**Barrido de las dos perillas del host** (tarda unos minutos, levanta y baja OpenOCD para cada
combinación):

```bash
python3 ../curso/ejemplos/uart/printf_rtt/medir_rtt.py --barrido
```

### Qué mira cada prueba

- **Costo de CPU**: mide `_write()` directo (solo la cola) contra `printf` completo, para varios
  largos. Es lo que separa el costo de la cola del costo de la libc.
- **Caudal sostenido**: el micro **se autolimita** esperando lugar en la cola antes de cada línea,
  así produce exactamente al ritmo al que el host consume. El número que sale es el caudal real del
  enlace. Además se verifica la continuidad de los números de secuencia, así que si se perdiera algo
  se vería.
- **Ráfaga**: sin autolimitarse. Muestra que la cola absorbe picos pero no ensancha el caño.
- **Latencia**: ida y vuelta PC → micro → PC, que son dos intervalos de polleo.

### Las dos perillas

```bash
make rtt RTT_POLL=100 RTT_SPEED=1000    # los valores por defecto de OpenOCD
make rtt                                # los afinados (10 ms, 4 MHz)
```

O a mano:

```bash
openocd -f openocd/lpc1769.cfg \
  -c "adapter speed 4000" \
  -c "init" -c "reset run" \
  -c 'rtt setup 0x10000000 0x8000 "SEGGER RTT"' \
  -c "rtt polling_interval 10" \
  -c "rtt start" -c "rtt server start 9090 0"
```

Los resultados medidos están en [`printf_rtt/README.md`](./printf_rtt/README.md). El resumen: con
los valores por defecto RTT rinde **menos** que una UART a 115200; afinado llega a ~15.7 KB/s, y ese
techo lo pone la sonda.

---

## Ver también

- [Capítulo 16 §6](../../00_lenguaje_c/16-redirigir-printf-a-uart.md) — la tabla completa de costos
- [`printf_retarget.c`](./printf_retarget.c) · [`printf_dma/`](./printf_dma/) ·
  [`printf_rtt/`](./printf_rtt/)
- [Módulo 12, capítulo 3 - La consola por el cable del debugger](../../12_debug/03-consola-por-el-debugger-rtt.md)
  — la guía de uso de RTT, con la puesta a punto en Ubuntu 24 y la parte de MCUXpresso
