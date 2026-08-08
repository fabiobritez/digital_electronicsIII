# Redirigir printf a la UART (clase práctica)

> **Objetivo de la clase.** Entender cómo la librería estándar de C se conecta con el hardware, y
> usar ese conocimiento para que `printf("ADC=%d\r\n", v)` salga por el cable serie y te sirva como
> consola de depuración. Al final vas a tener un archivo `syscalls.c` propio que podés copiar a
> cualquier proyecto del LPC1769.

Esta es una de las prácticas más lindas del curso porque toca un punto que casi nunca se explica:
*¿cómo sabe `printf` a dónde mandar los caracteres?* Spoiler: no lo sabe. Lo decidís vos.

---

## 1. El problema

En la PC escribís `printf("Hola\n")` y aparece en la terminal. Eso funciona porque el sistema
operativo le da a tu programa una **salida estándar** (stdout): un "archivo" abierto en el
descriptor `1` que el SO conecta con la consola.

En el micro **no hay sistema operativo y no hay consola**. Nadie conectó stdout a ningún lado. Si
linkeás un `printf` "pelado" y lo corrés, los caracteres se van... a una función vacía que no hace
nada (o que se cuelga). El texto se pierde.

Pero el micro **sí** tiene una salida natural hacia la PC: la **UART** (módulo 9). Un cable serie
(o un conversor USB-serie) lleva los bytes a una terminal en tu compu (`minicom`, `screen`,
`PuTTY`, el monitor serie del IDE). La idea de esta clase es **enchufar la salida estándar de C a la
UART**, para poder escribir:

```c
printf("ADC=%d  estado=%d\r\n", valor, estado);
```

y verlo en la terminal. A esto se le llama **retargeting** (re-apuntar) de la librería estándar.

---

## 2. Cómo funciona la librería estándar (newlib)

El compilador `arm-none-eabi-gcc` trae como librería C la **newlib** (y su variante chica
**newlib-nano**). Cuando llamás `printf`, por dentro pasa esto:

1. `printf` **formatea** el string: interpreta `%d`, `%x`, `%s`, rellena con los argumentos y arma
   en memoria la cadena final de bytes.
2. Cuando ya tiene bytes listos para salir, **no los manda a ningún hardware directamente**. En
   cambio, llama a una función de bajo nivel:

   ```c
   int _write(int fd, const char *buf, int len);
   ```

   Le pasa el descriptor de archivo (`fd = 1` para stdout, `2` para stderr), un puntero al buffer de
   bytes y cuántos son.

`_write` es lo que se llama un **syscall stub** (talón de llamada al sistema). En una PC, ese stub
es parte del SO y termina escribiendo en la consola. En el micro, ese stub **lo tenés que escribir
vos**. Eso es el retargeting: **reescribir los stubs de bajo nivel de newlib para que hablen con tu
hardware** en lugar de con un SO que no existe.

```
  printf("ADC=%d\n", v)
        │   formatea: "ADC=42\n"  (esto lo hace newlib)
        ▼
  _write(1, "ADC=42\n", 7)    ←── ESTE STUB LO ESCRIBÍS VOS
        │
        ▼
  UART_SendByte(...)  →  THR  →  pin TXD0 (P0.2)  →  cable  →  terminal en la PC
```

Toda la cadena de `printf`, `puts`, `putchar`, `fprintf(stdout, ...)`, `fwrite`, etc. termina
pasando por `_write`. Si arreglás `_write`, **todas** esas funciones salen por la UART.

---

## 3. Los syscall stubs de newlib

newlib espera que el sistema le provea un conjunto de stubs. Si no los definís, el linker linkea las
versiones por defecto (las de `--specs=nosys.specs`, que devuelven error) o directamente falla. Los
principales:

| Stub | Para qué lo usa newlib | ¿Importa para solo-salida? |
|------|------------------------|----------------------------|
| `_write(fd, buf, len)` | mandar bytes de stdout/stderr | **SÍ. Es el central.** |
| `_sbrk(incr)` | pedir memoria al heap (malloc, y a veces el buffer interno de `printf`) | **Sí**, conviene tenerlo real |
| `_read(fd, buf, len)` | leer de stdin (`scanf`, `getchar`) | solo si vas a leer (ejercicio) |
| `_close(fd)` | cerrar un "archivo" | no, stub trivial |
| `_fstat(fd, st)` | preguntar el tipo de un fd (newlib decide buffering según esto) | stub que dice "es un terminal" |
| `_isatty(fd)` | ¿el fd es una terminal? | stub que devuelve 1 |
| `_lseek(fd, off, dir)` | mover el cursor de un archivo | no, stub trivial |
| `_exit` / `_kill` / `_getpid` | terminar el "proceso" | stubs triviales (no hay proceso) |

Para **solo imprimir** alcanza con que funcionen bien dos: `_write` (el que hace el trabajo) y
`_sbrk` (para que el heap no devuelva basura si `printf` o `malloc` piden memoria). El resto pueden
ser stubs mínimos que solo existen para que el linker quede contento.

> ¿Por qué `printf` toca el heap? Según la implementación y los flags, `printf` puede usar un buffer
> temporal en el heap para formatear (sobre todo con campos anchos o `%f`). Si tu `_sbrk` está roto,
> `malloc` devuelve un puntero inválido y el programa se cae justo cuando imprimís algo "grande".
> Por eso conviene que `_sbrk` sea real aunque vos no llames `malloc` a mano.

---

## 4. La implementación clave: `syscalls.c`

Acá está el archivo completo. Es el corazón de la práctica. Mandamos por **UART0** cada byte de
stdout/stderr usando el driver del módulo 9 (`UART_SendByte`).

El "truco del `\n`": las terminales serie esperan **retorno de carro + avance de línea** (`\r\n`)
para empezar renglón nuevo. Si solo mandás `\n`, el cursor baja pero no vuelve al margen izquierdo y
ves la típica "escalera". Para no tener que escribir `\r\n` a mano cada vez, `_write` **traduce cada
`\n` a `\r\n`** al vuelo.

```c
/* syscalls.c: retargeting de newlib a UART0 (LPC1769) */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include "lpc17xx_uart.h"

/* errno es provisto por newlib; algunas configs lo piden explícito */
#undef errno
extern int errno;

/* ------------------------------------------------------------------ */
/*  EL STUB CENTRAL: a dónde van los bytes de printf/puts/fwrite       */
/* ------------------------------------------------------------------ */
int _write(int fd, const char *buf, int len)
{
    if (fd == 1 || fd == 2) {            /* 1 = stdout, 2 = stderr */
        for (int i = 0; i < len; i++) {
            if (buf[i] == '\n')          /* traducir LF -> CR LF */
                UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)'\r');
            UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)buf[i]);
        }
        return len;                      /* le decimos a newlib: mandé todo */
    }
    errno = EBADF;                       /* cualquier otro fd: no existe */
    return -1;
}

/* ------------------------------------------------------------------ */
/*  _sbrk: el asignador de heap que usa malloc (y a veces printf)      */
/*  _end lo define el linker script: marca el final de .bss            */
/* ------------------------------------------------------------------ */
extern char _end;            /* símbolo del linker: fin de la RAM usada */
static char *heap_end;

void *_sbrk(int incr)
{
    char *prev;
    if (heap_end == 0)
        heap_end = &_end;
    prev = heap_end;
    /* (opcional) acá podrías chequear contra el tope del stack y fallar */
    heap_end += incr;
    return (void *)prev;
}

/* ------------------------------------------------------------------ */
/*  Stubs mínimos: existen solo para que el linker no se queje.        */
/* ------------------------------------------------------------------ */
int _read(int fd, char *buf, int len)   { (void)fd; (void)buf; (void)len; return 0; }
int _close(int fd)                      { (void)fd; return -1; }
int _lseek(int fd, int off, int dir)    { (void)fd; (void)off; (void)dir; return 0; }
int _isatty(int fd)                     { (void)fd; return 1; }   /* "sí, es terminal" */

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;   /* "character device": newlib no bufferiza por bloques */
    return 0;
}

int  _getpid(void)              { return 1; }
int  _kill(int pid, int sig)    { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _exit(int code)            { (void)code; while (1) { } }   /* no hay a dónde "salir" */
```

> **Si usás la plantilla del repo, no copies este archivo tal cual.** `plantilla/src/syscalls.c` ya
> trae un `_write` (y todos los demás stubs) listo para usar. Está declarado `weak`, así que podés
> pisarlo con el tuyo, pero el camino corto y recomendado ahí es el gancho `__io_putchar` de la
> [sección 5a](#a-el-gancho-__io_putchar): son cinco líneas y no tenés que escribir ningún stub.
> El `_write` completo de acá es para cuando armás el proyecto desde cero, sin esa plantilla.

Y el `main` de prueba. Asumimos que `uart0_init()` es la inicialización del módulo 9 (driver,
115200 8N1, pines P0.2/P0.3):

```c
/* main.c */
#include <stdio.h>
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void uart0_init(void);   /* la del módulo 9 (driver), 115200 8N1 */

int main(void)
{
    uart0_init();

    /* salida inmediata: que no espere a llenar un buffer (ver sección 7) */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Sistema iniciado\r\n");

    int v = 0;
    while (1) {
        /* v simula una lectura de ADC (módulo 10) */
        printf("ADC=%d\r\n", v);     /* ¡esto sale por la UART! */
        v = (v + 1) & 0x3FF;
        for (volatile int d = 0; d < 1000000; d++) { }   /* delay burdo */
    }
}
```

Compilás y linkeás `syscalls.c` **junto con** tu `main.c`, el startup, el driver de UART y el linker
script (anexo A). Como definiste `_write` propio, el linker usa **el tuyo** en vez del de
`nosys.specs`. Abrís la terminal a 115200 y ves `ADC=0`, `ADC=1`, ... saliendo solos.

> **Antes de buscar el bug en otro lado: mirá el clock.** Los 115200 de este capítulo (y los del
> módulo 9) suponen `PCLK_UART0 = 25 MHz`, que sale de `CCLK = 100 MHz` dividido 4. Pero después de
> un reset el LPC1769 corre con el **RC interno a 4 MHz**, así que `PCLK_UART0` vale 1 MHz y el
> baudrate más alto que el hardware puede generar es `1e6 / 16 = 62500`. **A 4 MHz, 115200 no es un
> error de redondeo: es inalcanzable**, y lo único que vas a ver en la terminal es basura. Hay que
> subir el clock antes de inicializar la UART: llamando a `SystemInit()` de CMSIS (en la plantilla,
> `make USE_CMSIS=1`) o configurando la PLL a mano ([módulo 3](../03_clock_y_power/)). Si por lo que
> sea tenés que quedarte a 4 MHz, usá un baudrate bajo: 4800 sale con `DL = 13` y 0.16% de error.

> Coherencia con el módulo 9: notá el caste `(LPC_UART_TypeDef *)LPC_UART0`. UART0 tiene su propio
> tipo `LPC_UART0_TypeDef`, idéntico en layout a `LPC_UART_TypeDef`; el caste solo calla el warning.
> Está explicado en [módulo 09 - UART con driver](../09_uart/02-uart-con-driver.md).

---

## 5. Alternativa liviana: sin meterte con los stubs

Reescribir `_write` es el camino "estándar" y portable. Pero hay dos atajos.

### a) El gancho `__io_putchar`

Algunas configuraciones de newlib traen un `_write` por defecto que llama, byte por byte, a una
función `int __io_putchar(int ch)`. Si ese es tu caso, alcanza con definir:

```c
int __io_putchar(int ch)
{
    if (ch == '\n')
        UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)'\r');
    UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)ch);
    return ch;
}
```

Es más corto, pero **depende de que alguien haya escrito ese `_write` intermediario**. No es algo que
`newlib` te dé: es una convención (viene de los proyectos generados por STM32CubeMX y se copió a
medio mundo). O sea que `__io_putchar` funciona si tu proyecto ya trae un `_write` que la llame.

**En la plantilla del repo, ese `_write` está.** `plantilla/src/syscalls.c` lo define y llama a
`__io_putchar` byte por byte, con una versión `weak` de `__io_putchar` que tira todo a la basura. Al
definir la tuya (fuerte, en cualquier archivo del proyecto) gana la tuya sin configurar nada más. Con
esta plantilla, entonces, **el orden de preferencia se da vuelta respecto de lo que uno esperaría**:

| | Con la plantilla del repo | En un proyecto armado desde cero |
|---|---|---|
| `__io_putchar` | **el camino recomendado**: 5 líneas, ya está todo enganchado | solo si tu proyecto trae un `_write` que la llame |
| `_write` propio (sección 4) | opcional: el de `syscalls.c` es `weak`, el tuyo lo pisa | **el camino que siempre anda** |

> Hasta hace poco el `_write` de la plantilla **no** era `weak`, y definir el tuyo cortaba el linkeo
> con `multiple definition of '_write'`. Si te topás con ese error en una copia vieja de la
> plantilla, la solución es agregarle `__attribute__((weak))` al `_write` de `syscalls.c`, o
> simplemente usar `__io_putchar`.

### b) Tu propia `uart_printf` / `putchar`, sin la libc

Si lo único que querés es imprimir números y mensajes, podés **evitar `printf` por completo** y
escribir tus propias funciones contra la UART:

```c
void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)'\r');
        UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)*s++);
    }
}

/* imprime un entero con signo en decimal, sin tocar la libc */
void uart_put_int(int32_t n)
{
    char buf[12];               /* -2147483648 + '\0' entra holgado */
    int i = 0;
    uint32_t u = (n < 0) ? (uart_puts("-"), (uint32_t)(-(int64_t)n)) : (uint32_t)n;
    if (u == 0) { UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, '0'); return; }
    while (u) { buf[i++] = '0' + (u % 10); u /= 10; }
    while (i--) UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)buf[i]);
}
```

(El debug framework de NXP del [módulo 12](../12_debug/) ya hace exactamente esto con macros como
`_DBG`/`_DBD32`; mirálo como referencia.)

### ¿Cuándo conviene cada camino?

| Camino | Conviene cuando |
|--------|-----------------|
| `__io_putchar` | **estás usando la plantilla del repo** (ya trae el `_write` que la llama) y querés escribir lo mínimo |
| `_write` propio (sección 4) | armaste el proyecto desde cero, o querés mandar el bloque entero de una en vez de byte por byte. El más portable. |
| `uart_puts` / `uart_put_int` propias | querés el binario **mínimo**, no necesitás formato complejo, o todavía no querés depender de la libc |

---

## 6. El costo de printf

Todo el mundo repite que "printf es caro". Vale la pena preguntarse **caro en qué**, porque la
respuesta no es la que uno espera.

Los números de esta sección están **medidos en una LPCXpresso LPC1769**, no estimados: los de Flash
con `arm-none-eabi-size`; los de stack pintando la pila con un patrón y buscando la marca de agua;
los de heap leyendo `_sbrk(0)`; los de tiempo con el contador de ciclos `DWT->CYCCNT` del Cortex-M3
a 100 MHz. Toolchain: `arm-none-eabi-gcc` 13.2, `-Og`, `--gc-sections`, `--specs=nano.specs`.

### 6.1 Flash: barato

| Configuración | `text+data` | % de los 512 KB |
|---|---:|---:|
| Piso: `main` vacío (startup + vectores) | 764 B | 0.15% |
| + UART y salida propia (`uart_puts`) | 1048 B | 0.20% |
| + `printf` de enteros, newlib-nano | 7664 B | 1.46% |
| + `printf` con `%f` (`-u _printf_float`) | 20936 B | 3.99% |

Aislando **solo lo que agrega `printf`** —contra hacer lo mismo a mano, porque la UART la vas a
necesitar igual—: **6616 B para enteros, o sea 1.26% de la Flash.** Con `%f`, 19888 B (3.79%).

### 6.2 RAM: hay que sumar tres cosas, no una

`make size` te muestra la memoria estática y nada más. El stack y el heap no aparecen ahí, y son
más de la mitad del gasto:

| | estática | stack | heap | total | % de 32 KB |
|---|---:|---:|---:|---:|---:|
| Rutinas propias | 40 B | 84 B | 0 | 124 B | 0.4% |
| `printf` enteros + `setvbuf(_IONBF)` | 464 B | 376 B | 0 | **840 B** | 2.6% |
| `printf` enteros **sin** `setvbuf` | 464 B | 376 B | **1032 B** | 1872 B | 5.7% |
| `printf` con `%f` | 832 B | 584 B | 232 B | 1648 B | 5.0% |

Dos cosas que conviene saber antes de que te muerdan:

- **Sin `setvbuf`, el primer `printf` reserva 1032 bytes de heap** para el buffer de stdout. Es el
  gasto más grande de la tabla y el más invisible: no figura en `make size` porque ocurre en tiempo
  de ejecución. La línea de la sección 7 que parecía una comodidad vale 3% de tu RAM.
- **376 bytes de stack son el 18% de los 2 KB** que el linker script reserva. Si encima llamás
  `printf` desde una ISR anidada sobre una cadena de llamadas profunda, ahí es donde se desborda,
  y un desborde de stack no avisa: corrompe variables y el programa falla en otro lado.

### 6.3 Tiempo: acá está el costo de verdad

Imprimiendo `"x1234567\n"` (10 caracteres) a 115200 baud, con el core a 100 MHz:

| | ciclos | tiempo |
|---|---:|---:|
| Solo formatear `%lu` (`sprintf`, sin tocar la UART) | 1557 | 15.6 µs |
| Solo formatear `%f` | 7371 | 73.7 µs |
| **`printf` completo, `%lu`** | **87396** | **874 µs** |
| `printf` completo, `%f` | 96367 | 964 µs |
| **Las mismas rutinas propias, sin libc** | **86812** | **868 µs** |

Comparen las dos filas en negrita: **escribir tus propias rutinas para "evitar lo caro de printf" no
ahorra tiempo.** 874 µs contra 868 µs, 0.7% de diferencia.

La razón está en la primera fila. Formatear cuesta 15.6 µs; mandar 10 caracteres por la línea cuesta
868 µs (`10 caracteres × 10 bits / 115131 baud`, que es exactamente lo que dio la medición). El
formateo es el 1.8% del total y encima se esconde adentro de la espera del `THRE`.

> **La conclusión que importa: el costo de `printf` no es `printf`, es el cable.** El 98% del tiempo
> el CPU está parado en el `while (!(LSR & THRE))` esperando a que la UART termine de sacar un bit
> por vez. A 100 MHz, esos 868 µs son **86.800 ciclos** tirados.

### 6.4 ¿Conviene o no?

**Para depurar, sí, sin discusión.** 1.26% de Flash y 2.6% de RAM en un chip con 512 KB y 32 KB es
regalado, y te ahorra horas.

**Dentro de un lazo de control, no.** Y no por la memoria: por los 86.800 ciclos bloqueados. Si tu
superloop corre a 1 kHz (1 ms de período), **un solo `printf` por vuelta se come el 87% del
presupuesto de tiempo**. El firmware empieza a perder plazos y el bug que estabas buscando se
convierte en otro distinto: el clásico "cuando le pongo un printf anda, cuando lo saco falla".

### 6.5 Qué hacer, ordenado por lo que realmente ataca

Si el problema es **tiempo** (el caso normal en este chip):

1. **Imprimí menos seguido**, no distinto: una de cada N vueltas, o solo cuando algo cambia. Cuesta
   una línea y ataca directamente el 98% del costo.
2. **Subí el baudrate.** El bloqueo es inversamente proporcional: a 921600 baja 8×, a ~109 µs.
3. **Mandá por interrupción o DMA con una cola circular.** `printf` deja los bytes en la cola y
   vuelve en microsegundos; el tiempo de línea sigue existiendo pero ya no lo paga el CPU. Es la
   única solución de fondo. **Está implementado y medido** en
   [`ejemplos/uart/printf_dma/`](../ejemplos/uart/printf_dma/): los 4091 µs de CPU bloqueado de una
   línea de 48 caracteres bajan a 36 µs, a cambio de 360 bytes de Flash y un canal de GPDMA. Ahí
   vas a ver además que, con DMA, `setvbuf(_IONBF)` pasa a ser **contraproducente** — justo al revés
   que en la sección 7.

Si el problema es **espacio**:

4. **Evitá `%f`.** Es lo único con un costo desproporcionado: +13 KB de Flash, +208 B de stack y
   +232 B de heap contra hacer la cuenta en punto fijo. En el Cortex-M3 no hay FPU: todo el float es
   por software. En vez de `printf("%f V\n", 3.3f*adc/4096)`, calculá en `int` y mandá
   `printf("%d.%03d V\n", mv/1000, mv%1000)`. Desarrollado en
   [15 - Punto fijo vs flotante](./15-punto-fijo-vs-flotante.md).
5. **Usá newlib-nano** (`--specs=nano.specs`, la plantilla ya lo trae): la newlib completa se lleva
   4× más Flash para el mismo `printf`.
6. **`setvbuf(stdout, NULL, _IONBF, 0)`**: una línea, 1032 bytes de heap.
7. **Rutinas propias**: ahorran 6.6 KB de Flash y 292 B de stack, y perdés `%08X`, los anchos de
   campo y el formato de `%s`. Elegí a sabiendas de que **no vas a ganar ni un microsegundo**.

Y si te aprieta la RAM antes de tener que sacar `printf`: el LPC1769 tiene **32 KB de AHB SRAM sin
usar** (dos bancos de 16 KB que el linker script declara pero no ocupa).

---

## 7. Buffering

newlib decide si bufferiza stdout según el `_fstat`/`_isatty`. Por defecto, stdout puede ser
**bufferizado por línea o por bloque**: los bytes se acumulan y recién salen cuando aparece un `\n`,
se llena el buffer, o hacés `fflush`. Depurando, eso es traicionero: ponés un `printf` antes de un
cuelgue, el programa se cuelga **antes** de que el buffer se vacíe, y nunca ves el mensaje.

Solución: poner stdout en **sin buffer** apenas arranca el programa, así cada byte sale al instante.

```c
setvbuf(stdout, NULL, _IONBF, 0);   /* _IONBF = sin buffer: salida inmediata */
```

Alternativa puntual: `fflush(stdout);` después de un `printf` crítico, para forzar el vaciado en ese
punto sin desactivar el buffering en general.

Para depurar, la salida inmediata casi siempre vale la pena (perdés un poco de eficiencia, ganás que
**lo último que ves es lo último que pasó**).

> **Esto vale mientras la salida sea por polling.** Si algún día pasás a mandar por DMA
> ([`ejemplos/uart/printf_dma/`](../ejemplos/uart/printf_dma/)), la recomendación **se da vuelta**:
> sin buffer, newlib llama a `_write` una vez por carácter y cada llamada arranca su propia
> transferencia con su propia interrupción. Medido, eso triplica el costo de un `printf`. Ahí
> conviene buffering de línea, dándole vos el buffer para que no lo pida al heap:
> `setvbuf(stdout, mi_buffer, _IOLBF, sizeof mi_buffer)`.

---

## 8. Cuidados

- **NO uses `printf` dentro de una ISR.** Tres razones, las tres medidas en la sección 6:
  (1) **no es reentrante** (usa estado global y, según el caso, el heap); si una interrupción lo
  llama mientras el `main` también lo está usando, corrompés ese estado. (2) Es **lentísimo para una
  ISR**: 874 µs para diez caracteres, o sea 86.800 ciclos con el CPU parado. (3) Se come **376 bytes
  de stack**, el 18% de lo que el linker reserva, encima de lo que ya venía usando la cadena de
  llamadas interrumpida. Regla del [módulo 12](../12_debug/): la ISR **levanta una bandera**, y el
  `main` imprime.
- **El `_write` por polling bloquea, y ese es el costo dominante.** `UART_SendByte` espera con
  `while` a que `THRE` esté libre (módulo 9), un bit por vez. El 98% del tiempo de un `printf` se va
  ahí, no en formatear. A 9600 baud es doce veces peor todavía. Si necesitás imprimir sin bloquear,
  hay que mandar por interrupción/DMA con una cola: más complejo, pero es lo único que ataca el
  problema real ([módulo 11](../11_dma/)).
- **Reentrancia y RTOS.** Si en el futuro usás un RTOS con varias tareas, dos tareas llamando
  `printf` a la vez chocan. Ahí se usa newlib con soporte de reentrancia (`_REENT`) o se protege con
  un mutex. Para programas bare-metal de un solo hilo (como los del curso) no es problema.

---

## 9. Otras formas de "imprimir" (solo para que sepas que existen)

La UART no es la única salida de depuración. Sin desarrollarlas, dos alternativas que vas a ver
nombradas:

- **Semihosting** (`--specs=rdimon.specs`): el `printf` sale por el **debugger** (JTAG/SWD) a la
  consola del IDE, sin usar UART. Cómodo porque no gastás un periférico, pero es **lento** y
  **requiere un debugger conectado y corriendo**: si lo desconectás, el programa se cuelga en el
  próximo `printf`. No sirve para un equipo en producción.
- **ITM / SWO**: el Cortex-M3 tiene una unidad de trazado (ITM) que puede sacar caracteres por el
  pin **SWO**, y el debugger los muestra. Muy rápido y no bloquea como la UART por polling, pero
  también depende de tener el debugger y la herramienta configurada.

Para el curso, la **UART es la opción más universal**: funciona con un simple conversor USB-serie de
pocos pesos, sin debugger.

---

## Ejercicios

1. **Lectura por la UART (`scanf`).** Hacé que `_read(fd, buf, len)` lea bytes de UART0 (con
   `UART_ReceiveByte`) cuando `fd == 0` (stdin), y probá `int x; scanf("%d", &x);`. Cuidado con el
   eco y con el `\r` que manda la terminal al apretar Enter. ¿Tenés que bloquear hasta recibir un
   `\n`?
2. **Medir el costo.** Compilá tu proyecto con y sin `--specs=nano.specs` y comparálos con
   `arm-none-eabi-size app.elf`. Después agregá un `printf("%f", ...)`, sumá `-u _printf_float` y
   volvé a medir. Anotá los tres tamaños de `text` y comparalos con la tabla de la sección 6.
3. **Sin la libc.** Reemplazá todos los `printf` de tu programa por `uart_puts` / `uart_put_int`
   propias (sección 5b) y medí cuánto baja el binario. ¿Vale la pena? ¿Qué perdés?
4. **El bug del buffer.** Sacá el `setvbuf(..., _IONBF, ...)`, poné un `printf("antes del cuelgue\n")`
   (con `\n`, sin `fflush`) seguido de un `while(1){}`, y comprobá que el mensaje **no aparece**.
   Después agregá `fflush(stdout)` y verificá que sí aparece. Eso es la sección 7 en acción.
5. **Los 1032 bytes invisibles.** Declarando `extern char end;` y `extern void *_sbrk(ptrdiff_t);`,
   imprimí `(uint32_t)_sbrk(0) - (uint32_t)&end` (el heap usado) antes y después del primer `printf`,
   con y sin `setvbuf(_IONBF)`. Vas a reproducir la fila más cara de la tabla de la sección 6.2.
   ¿Por qué `make size` no te muestra ese gasto?
6. **Medir el bloqueo.** Habilitá el contador de ciclos del Cortex-M3
   (`DEMCR |= 1<<24; DWT_CYCCNT = 0; DWT_CTRL |= 1;`) y medí cuántos ciclos tarda un `printf` de diez
   caracteres. Después medí solo el formateo, con `sprintf` a un buffer. Compará las dos cifras con
   el tiempo teórico de la línea (`10 caracteres × 10 bits / baudrate`) y decidí vos dónde se va el
   tiempo. Repetilo a 9600 baud: ¿cuál de los dos números cambia?

> **Verificación del código.** El camino de `__io_putchar` de la sección 5a está **probado en placa**:
> [`curso/ejemplos/uart/printf_retarget.c`](../ejemplos/uart/printf_retarget.c) se compiló sobre la
> plantilla con `make USE_CMSIS=1`, se grabó en una LPCXpresso LPC1769 por CMSIS-DAP y su salida se
> leyó a 115200 8N1 desde un conversor USB-serie en P0.2/P0.3, en los dos sentidos.
>
> Todos los números de la sección 6 son **mediciones sobre esa misma placa**, no estimaciones: Flash
> con `arm-none-eabi-size`; stack pintando la pila con un patrón `0xAA` y buscando la marca de agua;
> heap leyendo `_sbrk(0)`; tiempo con `DWT->CYCCNT` a 100 MHz. Los ejercicios 5 y 6 son exactamente
> esos experimentos, para que los reproduzcas.

---

**Anterior:** [15 - Punto fijo vs flotante](./15-punto-fijo-vs-flotante.md) ·
**Módulo:** [Lenguaje C](./README.md)

**Ver también:** [Módulo 09 - UART](../09_uart/) · [Módulo 12 - Debug](../12_debug/) ·
[Ejemplo probado en placa: `ejemplos/uart/printf_retarget.c`](../ejemplos/uart/printf_retarget.c)

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes: 7.21 (`<stdio.h>`: `printf`, los flujos y el *buffering*), 7.21.5.6 (`setvbuf`).
- [cppreference: printf](https://en.cppreference.com/w/c/io/fprintf). La tabla completa de especificadores de formato.

**La librería estándar**

- [Newlib: documentación oficial](https://sourceware.org/newlib/libc.html). La lista completa de los *syscall stubs* que espera (`_write`, `_read`, `_sbrk`, `_close`, `_fstat`, `_isatty`, `_lseek`).
- [Newlib-nano](https://sourceware.org/newlib/README). La variante reducida que usa `--specs=nano.specs`, y qué recorta respecto de la completa.
- El `printf` que efectivamente se linkeó se puede pesar sin placa:
  ```console
  $ arm-none-eabi-size firmware.elf
  $ arm-none-eabi-nm --size-sort -S firmware.elf | tail -20
  ```

**ARM y el LPC1769**

- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf), Capítulo 14 (UART0/2/3). Los registros `THR`, `LSR` y el divisor de baudios que usa `UART_SendByte`.
- [Semihosting (Arm)](https://developer.arm.com/documentation/dui0471/latest/what-is-semihosting-). La alternativa que se menciona al final: imprimir a través del debugger, sin cable serie, a costa de que el micro se frene en cada carácter.
- De dónde sale el heap que necesita `_sbrk`, en [Build, linker y startup](../anexos/A_build_linker_startup/02-linker-y-startup.md).

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [15 - Punto fijo vs punto flotante](./15-punto-fijo-vs-flotante.md) ·
**Siguiente:** [17 - El superloop y el código no bloqueante](./17-superloop-y-codigo-no-bloqueante.md)
