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

Es más corto, pero **depende** de que tu `--specs` provea ese `_write` intermediario (el material
del módulo 9 lo menciona como "a veces es `_write()`"). No es universal. Si no funciona, caés al
`_write` completo de la sección 4, que **siempre** anda.

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
| `_write` propio (sección 4) | querés `printf` completo, con todos sus `%`. El más portable. |
| `__io_putchar` | tu specs lo soporta y querés menos código que escribir |
| `uart_puts` / `uart_put_int` propias | querés el binario **mínimo**, no necesitás formato complejo, o todavía no querés depender de la libc |

---

## 6. El costo de printf

`printf` es **caro en tamaño de código**: tiene que interpretar el string de formato, soportar
decenas de especificadores, anchos, banderas, etc. Eso se nota, y mucho, en un micro con 512 KB de
flash pero proyectos que a veces quieren entrar en mucho menos.

Medido en este mismo toolchain (Cortex-M3, el mismo binario de prueba de la sección 4):

| Configuración | `text` (flash) |
|---------------|----------------|
| `printf("%d")` con newlib completa | ~38.8 KB |
| `printf("%d")` con **newlib-nano** (`--specs=nano.specs`) | ~5.9 KB |
| nano + `printf("%f")` **sin** flag de float | ~5.5 KB (imprime mal el float) |
| nano + `printf("%f")` **con** `-u _printf_float` | ~17.4 KB |

Conclusiones:

- **Usá newlib-nano**: agregá `--specs=nano.specs` al linkeo. Es un `printf` reducido pero más que
  suficiente para depurar, y achica el binario casi 7×.
- **El soporte de `%f` (float) es pesado y, por defecto, está apagado en nano.** Si imprimís un
  `float` sin habilitarlo, sale basura. Para activarlo tenés que pasarle al linker
  `-u _printf_float`, y eso **infla el binario de ~5.9 KB a ~17.4 KB** (¡+11 KB solo por imprimir
  comas decimales!). En el Cortex-M3 **no hay FPU**, así que todo el float es por software: lento y
  grande.
- **Recomendación: evitá `%f` al depurar.** Imprimí en enteros o en **punto fijo** (milivoltios,
  centésimas, etc.). En vez de `printf("%f V\n", 3.3f * adc / 4096)` hacé las cuentas en `int` y
  mandá `printf("%d.%03d V\n", mv/1000, mv%1000)`. Esto está desarrollado en
  [15 - Punto fijo vs flotante](./15-punto-fijo-vs-flotante.md).

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

---

## 8. Cuidados

- **NO uses `printf` dentro de una ISR.** Dos razones: (1) `printf` **no es reentrante** (usa estado
  global y, según el caso, el heap); si una interrupción lo llama mientras el `main` también lo está
  usando, corrompés ese estado. (2) Es **lento**: formatear y mandar 20 bytes por UART a 115200
  tarda ~2 ms, una eternidad dentro de una interrupción. Regla del [módulo 12](../12_debug/): la ISR
  **levanta una bandera**, y el `main` imprime.
- **El `_write` por polling bloquea.** `UART_SendByte` espera con `while` a que `THRE` esté libre
  (módulo 9). Mientras imprime, tu programa no hace nada más. A 9600 baud eso es lentísimo; a 115200
  es tolerable para depurar pero igual frena. Si necesitás imprimir sin bloquear, hay que mandar por
  interrupción/DMA con una cola, lo cual es bastante más complejo.
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

> **Verificación del código.** El `syscalls.c` de la sección 4 y el `main` de prueba se compilaron y
> linkearon con el toolchain del curso (`arm-none-eabi-gcc` 13.3.1, xPack) con
> `-mcpu=cortex-m3 -mthumb`, tanto con newlib completa como con `--specs=nano.specs`. Los tamaños de
> la tabla de la sección 6 salen de ese mismo experimento.

---

**Anterior:** [15 - Punto fijo vs flotante](./15-punto-fijo-vs-flotante.md) ·
**Módulo:** [Lenguaje C](./README.md)

**Ver también:** [Módulo 09 - UART](../09_uart/) · [Módulo 12 - Debug](../12_debug/)

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
