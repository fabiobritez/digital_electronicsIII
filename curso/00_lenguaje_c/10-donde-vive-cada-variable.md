# Dónde vive cada variable: stack, heap y estáticos

Hasta acá declaraste variables sin preguntarte dónde terminan. En una PC eso casi nunca importa: hay
gigabytes y un sistema operativo que te avisa cuando te pasás. En el LPC1769 tenés **32 KB de SRAM
principal** y **nadie te avisa nada**: si te pasás, el programa no falla con un mensaje, falla
*raro* —se cuelga, se reinicia, o una variable cambia sola.

Este capítulo responde una sola pregunta, pero a fondo:

> Cuando escribís `uint32_t x;`, **¿en qué parte de la memoria termina esa variable, quién elige esa
> dirección, y hasta cuándo vive?**

Es el capítulo que le da sentido a tres cosas que ya viste: por qué una local
"arranca con basura" y una global arranca en cero ([01 - Declaraciones](./01-declaraciones-y-tipos.md)),
por qué la recursión profunda cuelga el micro ([06 - Funciones](./06-funciones.md#el-costo-de-la-recursión-en-la-pila-crítico-en-embebido)),
y por qué devolver un puntero a una local es un bug ([08 - Punteros](./08-punteros.md)).

---

## Las dos memorias del micro

Antes de hablar de variables, el escenario. El LPC1769 tiene dos tipos de memoria bien distintos:

| | **Flash** | **SRAM** |
|---|---|---|
| Tamaño | 512 KB | 32 KB (+ 2×16 KB en el bus AHB) |
| ¿Se borra al apagar? | **No** (no volátil) | **Sí** (volátil) |
| ¿Se puede escribir en ejecución? | Prácticamente no (solo vía la boot ROM) | Sí, todo el tiempo |
| Dirección base | `0x0000_0000` | `0x1000_0000` |
| Qué guarda | el **programa** y las **constantes** | las **variables** |

Dos detalles que van a importar:

- **El programa se ejecuta desde la Flash.** No se copia a RAM como en una PC. Por eso la Flash
  "grande" (512 KB) no te salva si te quedás sin RAM: son recursos separados y no intercambiables.
- **Los 64 KB de RAM no son contiguos.** Hay 32 KB en `0x1000_0000` (SRAM local, pegada al núcleo)
  y dos bloques de 16 KB en `0x2007_C000` y `0x2008_0000` (SRAM del bus AHB, pensada para DMA,
  Ethernet y USB). Entre medio hay un agujero. Por eso el linker las declara como **cuatro regiones
  separadas** y tus variables normales entran solo en los primeros 32 KB. Podés verlo tal cual en
  [`plantilla/linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld).

> Todo esto sale del mapa de memoria del chip, que vemos en detalle en
> [Módulo 1 - El mapa de memoria del LPC1769](../01_arquitectura_y_acceso_a_registros/01-mapa-de-memoria.md).

---

## Las cuatro zonas de un programa corriendo

Tu firmware, mientras corre, tiene la memoria repartida así:

```
FLASH  0x00000000  ┌────────────────────┐
                   │  .isr_vector       │  tabla de vectores (SP inicial + handlers)
                   │  .text             │  ← EL CÓDIGO: tus funciones
                   │  .rodata           │  ← CONSTANTES: literales, tablas const
                   │  (copia inicial    │
                   │   de .data)        │
       0x0007FFFF  └────────────────────┘

RAM    0x10008000  ┌────────────────────┐  ← tope de la RAM (_estack)
                   │      STACK         │  ← ZONA 4: locales, parámetros, direcciones de retorno
                   │        ↓ crece     │
                   │                    │
                   │   ...libre...      │  ← acá se juegan la vida stack y heap
                   │                    │
                   │        ↑ crece     │
                   │      HEAP          │  ← ZONA 3: malloc() (en embebidos, casi siempre vacío)
                   ├────────────────────┤  ← símbolo 'end' que define el linker
                   │      .bss          │  ← ZONA 2: globales/static SIN valor inicial (en 0)
                   │      .data         │  ← ZONA 1: globales/static CON valor inicial
       0x10000000  └────────────────────┘  ← base de la RAM
```

La idea central del capítulo, y la que hay que llevarse:

> **Tres de las cuatro zonas tienen tamaño fijo y conocido en tiempo de compilación.** El linker las
> calcula, te las puede mostrar, y si no entran te da un error. **La cuarta —el stack— solo existe
> mientras el programa corre, cambia de tamaño en cada llamada a función, y nadie la mide por vos.**

Por eso el stack es la que da problemas, y por eso le dedicamos la mitad del capítulo.

---

## Recorrido: dónde cae cada declaración

Un programa chiquito con una de cada una:

```c
#include <stdint.h>

const char mensaje[] = "Temperatura: ";   // (A)  .rodata → FLASH
uint32_t contador_global = 100;           // (B)  .data   → RAM (valor inicial en Flash)
uint32_t errores;                         // (C)  .bss    → RAM (arranca en 0)
static uint8_t buffer_rx[256];            // (D)  .bss    → RAM (arranca en 0)

uint32_t leer_y_promediar(uint32_t n)
{
    uint32_t acumulador = 0;              // (E)  STACK: nace acá, muere al salir
    uint32_t muestras[8];                 // (F)  STACK: 32 bytes de una
    static uint32_t llamadas = 0;         // (G)  .bss  → RAM: NO está en el stack

    llamadas++;
    for (uint32_t i = 0; i < n && i < 8; i++) {   // (H) STACK (o un registro)
        muestras[i] = i;
        acumulador += muestras[i];
    }
    return acumulador;
}
```

| | Declaración | ¿Dónde vive? | ¿Cuánto vive? | Valor inicial |
|---|---|---|---|---|
| A | `const char mensaje[]` | **Flash** (`.rodata`) | todo el programa | el que escribiste |
| B | `uint32_t contador_global = 100` | **RAM** (`.data`) | todo el programa | 100, lo copia el startup |
| C | `uint32_t errores` | **RAM** (`.bss`) | todo el programa | **0**, lo pone el startup |
| D | `static uint8_t buffer_rx[256]` | **RAM** (`.bss`) | todo el programa | **0** |
| E | `uint32_t acumulador = 0` | **Stack** | mientras corre la función | 0 porque lo escribiste vos |
| F | `uint32_t muestras[8]` | **Stack** | mientras corre la función | **basura** |
| G | `static uint32_t llamadas` | **RAM** (`.bss`) | todo el programa | **0** |
| H | `uint32_t i` del `for` | **Stack** o un registro | mientras dura el `for` | el que le des |

Fijate en el par (D) y (G): **las dos dicen `static` y las dos viven en `.bss`**. La palabra `static`
no decide *dónde* vive la variable, decide **quién la ve**. Esa distinción está desarrollada en
[14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md#static-los-dos-patrones-que-vas-a-escribir).

Y fijate en (F): `muestras[8]` **son 32 bytes que aparecen y desaparecen** cada vez que entrás y
salís de la función. Si esa función se llamara desde dentro de otra que ya tenía su propio buffer, y
esa desde una tercera, los 32 bytes se suman. Ese apilado es exactamente el tema que sigue.

---

## ¿Quién decide la dirección, y cuándo?

La tabla de arriba dice en qué **zona** cae cada declaración. Falta la otra mitad de la pregunta:
quién elige el **número exacto** de la dirección, y en qué momento. La respuesta no es "al compilar"
ni "al ejecutar": son **tres actores** y cada uno decide una parte.

| Actor | Qué decide | ¿Cuándo? |
|---|---|---|
| **El compilador** | la sección (`.data`, `.bss`, `.rodata`) y el *offset* dentro de ella. Para una local: el desplazamiento respecto del `SP` —o **ningún lugar en memoria**, si la deja en un registro | al compilar cada `.c` por separado |
| **El linker** | la dirección **absoluta** de todo lo estático, siguiendo el linker script | al juntar todos los `.o` |
| **El hardware y el runtime** | dónde caen las locales (según cuánto valga el `SP` en ese instante) y qué devuelve `malloc()` | mientras el programa corre |

El punto que más sorprende: **el compilador nunca sabe la dirección final de una global.** Deja un
hueco simbólico ("acá va `contador_global`") y anota que alguien lo tiene que rellenar. Ese alguien
es el linker. Por eso podés compilar un `.c` suelto sin tener el resto del proyecto, y por eso
**cambiar el `.ld` mueve todas las direcciones sin recompilar una sola línea de C**.

Aplicado a las declaraciones del ejemplo anterior:

| Si escribís… | ¿Quién elige la dirección? | ¿Cuándo? | ¿Cambia? |
|---|---|---|---|
| global o `static` (A, B, C, D, G) | el **linker**, según el `.ld` | al **enlazar** | nunca, fija de por vida |
| local que vive en memoria (E, F) | el compilador el offset, el `SP` el resto | **en cada llamada** | **sí, cada vez** |
| local que vive en un registro (H) | el compilador | al compilar | **no tiene dirección** |
| `malloc(n)` | el asignador (`_sbrk` + newlib) | en ejecución | sí |
| un registro de periférico | el fabricante, en el silicio | — | nunca |

### Verlo con las dos manos

Este se corre en la PC, que es más rápido que grabar la placa y el mecanismo es idéntico:

```c
#include <stdio.h>
#include <stdint.h>

uint32_t contador_global = 100;                 /* .data */

static void nivel(int n)
{
    uint32_t acumulador = n;                    /* stack */
    printf("nivel %d -> &acumulador = %p\n", n, (void *)&acumulador);
    if (n < 3) nivel(n + 1);
}

int main(void)
{
    printf("&contador_global = %p\n", (void *)&contador_global);
    nivel(1);
    return 0;
}
```

```console
$ gcc -O0 quien.c -o quien && ./quien
&contador_global = 0x5d2a14d79010
nivel 1 -> &acumulador = 0x7ffe0350b064
nivel 2 -> &acumulador = 0x7ffe0350b034
nivel 3 -> &acumulador = 0x7ffe0350b004

$ ./quien                          # la MISMA corrida, otra vez
&contador_global = 0x613ae6cf4010
nivel 1 -> &acumulador = 0x7fff5af0f1e4

$ nm --defined-only quien | grep contador_global
0000000000004010 D contador_global
```

Tres cosas para leer ahí:

- **La misma variable local, tres direcciones distintas**, separadas por `0x30`: el tamaño del frame
  de `nivel()`. `acumulador` no "está" en ningún lado fijo; está *a tal distancia del `SP`*, y el `SP`
  depende de quién te llamó. Es la misma idea del diagrama de `main → A → B` de más abajo.
- **El linker dijo `0x4010`, a secas.** No una dirección: un offset. En las dos corridas la dirección
  impresa termina en `010`, porque es ese mismo `0x4010` más una base.
- **Esa base cambió entre corridas.** En la PC hay un cuarto actor, el **loader** del sistema
  operativo, que elige dónde montar el programa y encima lo aleatoriza (ASLR) como defensa contra
  exploits.

> **En el micro no hay ninguna de esas dos capas.** Sin sistema operativo, sin loader y sin MMU en
> uso, lo que dice el linker script **es** la dirección física que va a ver el bus, corrida tras
> corrida y placa tras placa. Por eso en bare metal podés abrir el `.map` y saber de antemano la
> dirección exacta de cualquier global —algo impensable en una PC—, y por eso el `.ld` es un archivo
> que sí vas a tener que leer. El recorrido completo `.c → .o → .elf`, con el linker script del
> LPC1769 comentado línea por línea, está en
> [Módulo 16 - De código a binario](../anexos/A_build_linker_startup/01-de-codigo-a-binario.md) y
> [Módulo 16 - El linker script y el startup](../anexos/A_build_linker_startup/02-linker-y-startup.md).

### Dos consecuencias prácticas

**1. Muchas variables no tienen dirección en absoluto.** Es la fila (H) de la tabla anterior: el `i`
del `for` normalmente vive en un registro y nunca toca la RAM. Lo que **obliga** al compilador a
darle un lugar en memoria es que le tomes la dirección con `&` (o que la declares `volatile`, o que
no entre en registros, como los arrays). Cuando en
[El optimizador cambia el resultado](#el-optimizador-cambia-el-resultado-mucho) veas una función
pasar de 24 bytes de stack a 0, es exactamente esto: sus variables dejaron de tener dirección.

**2. El orden en memoria no es el orden del código fuente.** El compilador y el linker agrupan por
sección, reordenan y meten relleno por alineación:

```c
uint8_t  a;      // no asumas que 'b' está justo después de 'a'
uint32_t b;      // ni que 'c' está en .bss al lado de las otras dos
uint8_t  c;
```

El estándar no garantiza nada sobre las posiciones relativas de dos variables declaradas por
separado, y en la práctica no se cumple casi nunca. La autoridad es el `.map`, no el `.c`. Esto
importa cuando un desborde te pisa una variable y querés saber **cuál**: la respuesta está en el
mapa, y suele no ser la vecina en el código.

---

## Zona 1 y 2: los estáticos (`.data`, `.bss`, `.rodata`)

Son las tres fáciles: **existen desde antes de `main()` hasta que apagás el micro**, siempre en la
misma dirección, y su tamaño se conoce al compilar.

- **`.rodata` (Flash).** Datos de solo lectura: literales de cadena y todo lo que declares
  `const` (y no modifiques por otro alias). Como viven en Flash, **no gastan RAM**. Es la razón por
  la que una tabla de lookup grande se declara `static const`: 512 KB de Flash contra 32 KB de RAM.

- **`.data` (RAM, con copia en Flash).** Globales y `static` **con** valor inicial distinto de cero.
  Ocupan **dos veces**: los bytes en RAM donde van a vivir, más los bytes en Flash donde se guarda el
  valor inicial.

- **`.bss` (RAM, sin copia).** Globales y `static` **sin** valor inicial, o inicializadas en cero. No
  ocupan Flash: no hace falta guardar 256 ceros, alcanza con anotar "poné 256 bytes en cero".

### Por qué las globales "ya vienen inicializadas"

No es magia del lenguaje ni del hardware: es **código que corre antes que tu `main`**. El
`Reset_Handler` del startup hace exactamente dos cosas antes de llamarte:

```c
/* 1) copiar .data de Flash a RAM (así tus 'int x = 5;' valen 5) */
uint32_t *src = &_etext, *dst = &_sdata;
while (dst < &_edata) *dst++ = *src++;

/* 2) poner .bss en cero (así tus globales sin inicializar valen 0) */
for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
```

Ese código lo desarmamos línea por línea en
[Anexo A - El linker script y el startup](../anexos/A_build_linker_startup/02-linker-y-startup.md).
La conclusión práctica es esta: **el startup inicializa `.data` y `.bss`, pero no toca el stack.**
Por eso una global sin inicializar vale 0 garantizado, y una local sin inicializar vale lo que haya
quedado ahí de antes.

---

## Zona 4: el stack, en serio

### Qué es, conceptualmente

Una **pila**: una estructura donde solo podés poner arriba (*push*) y sacar de arriba (*pop*). No
podés sacar algo del medio. Y hay un único puntero que dice dónde está el tope: el **stack pointer**
(`SP`, que en ARM es el registro `R13`).

¿Por qué una pila y no otra cosa? Porque las llamadas a función **anidan perfectamente**: si `main`
llama a `A` y `A` llama a `B`, entonces `B` termina antes que `A`, y `A` antes que `main`. Último en
entrar, primero en salir. Una pila es la estructura exacta para eso, y por eso el hardware la trae
integrada.

### Cómo lo hace el Cortex-M3

El manual del núcleo lo define en una frase (§34.3.1.2 del [UM10360](../../manual/ch34_appendix-cortex-m3-user-guide.pdf)):

> *"The processor uses a full descending stack. This means the stack pointer indicates the last
> stacked item on the stack memory. When the processor pushes a new item onto the stack, it
> decrements the stack pointer and then writes the item to the new memory location."*

Traducido, dos propiedades que explican todos los diagramas de este curso:

- **Full**: el `SP` apunta al **último dato guardado** (no al primer lugar libre).
- **Descending**: el stack **crece hacia direcciones más bajas**. Empieza arriba de todo
  (`_estack = 0x10008000`, el tope de la SRAM) y baja hacia donde están tus globales.

Y arranca solo: al salir del reset, el núcleo **carga el `SP` leyendo la palabra que está en la
dirección `0x00000000`** —la primera entrada de la tabla de vectores— antes de ejecutar una sola
instrucción tuya. Es literalmente lo primero que hace el chip. Lo ves en
[07 - NVIC y vectores](../07_interrupciones/01-nvic-y-vectores.md) y en
[16 - Linker y startup](../anexos/A_build_linker_startup/02-linker-y-startup.md).

### El *stack frame*: qué guarda cada llamada

Cada función activa tiene su propia porción de stack, su **marco** o *stack frame*. Adentro va:

1. La **dirección de retorno** (a dónde volver cuando termine).
2. Los **registros que la función va a usar** y que le prometió a su llamador no romper.
3. Sus **variables locales** que no entraron en registros (arrays y structs, siempre).
4. Los **argumentos** que no entraron en los registros `R0`–`R3`.

Con `main → A → B` corriendo, la RAM se ve así:

```
0x10008000  ┌──────────────────┐  ← _estack
            │ frame de main    │
            ├──────────────────┤
            │ frame de A       │
            ├──────────────────┤
            │ frame de B       │  ← SP apunta acá (el tope actual)
            │                  │
            │   ...libre...    │
```

Cuando `B` retorna, **su frame no se borra: simplemente el `SP` sube**. Los bytes siguen ahí, con los
valores viejos, hasta que la próxima llamada los pise. Esta única frase explica dos comportamientos
que confunden a todo el mundo:

- **Por qué una local sin inicializar tiene "basura":** esos bytes son las sobras de la última
  función que ocupó ese lugar.
- **Por qué a veces un puntero colgante "parece andar":** los datos todavía no fueron pisados. Y
  después falla, cuando sí lo fueron. Es el peor tipo de bug: el que no es determinista.

### Prólogo y epílogo, con los fierros de verdad

Esto no hay que creerlo: se mira. Tomemos la función más tonta posible:

```c
uint32_t promedio(uint32_t a, uint32_t b)
{
    uint32_t suma = a + b;
    return suma / 2;
}
```

Compilada **sin optimizar** (`-O0`) y desensamblada con el toolchain del repo:

```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -c d2.c -o d2.o
$ arm-none-eabi-objdump -d d2.o
```

```asm
00000000 <promedio>:
   0:   b480        push    {r7}        ; PRÓLOGO: guarda r7 (lo va a usar de frame pointer)
   2:   b085        sub     sp, #20     ;          reserva 20 bytes para las locales
   4:   af00        add     r7, sp, #0  ;          r7 = base del frame
   6:   6078        str     r0, [r7, #4] ;         guarda el argumento 'a' en el frame
   8:   6039        str     r1, [r7, #0] ;         guarda el argumento 'b' en el frame
   a:   687a        ldr     r2, [r7, #4] ; ─┐
   c:   683b        ldr     r3, [r7, #0] ;  │ CUERPO: todo pasa por memoria,
   e:   4413        add     r3, r2       ;  │ porque -O0 no usa registros para nada
  10:   60fb        str     r3, [r7, #12];  │
  12:   68fb        ldr     r3, [r7, #12];  │
  14:   085b        lsrs    r3, r3, #1   ; ─┘ /2 es un shift a la derecha
  16:   4618        mov     r0, r3      ;          valor de retorno en r0
  18:   3714        adds    r7, #20     ; EPÍLOGO: devuelve los 20 bytes
  1a:   46bd        mov     sp, r7      ;
  1c:   bc80        pop     {r7}        ;          restaura r7
  1e:   4770        bx      lr          ;          vuelve a la dirección guardada en LR
```

Ahí está todo el mecanismo en 16 instrucciones:

- **`sub sp, #20`** *es* la reserva del frame. Restar del `SP` = crecer la pila (porque es
  *descending*). Sumarle = liberarla.
- **`push {r7}` / `pop {r7}`**: la función usa `r7` y se lo devuelve intacto al llamador.
- **`bx lr`**: `LR` (*link register*, `R14`) trae la dirección de retorno. La instrucción `bl` que
  llamó a `promedio` la dejó ahí automáticamente.

En total: 4 bytes del `push` + 20 del `sub` = **24 bytes de stack** por cada llamada a esta función
de dos líneas.

### El optimizador cambia el resultado (mucho)

La misma función, con `-O2`:

```asm
00000000 <promedio>:
   0:   4408        add     r0, r1      ; a + b
   2:   0840        lsrs    r0, r0, #1  ; / 2
   4:   4770        bx      lr          ; listo
```

**Cero bytes de stack.** Todo pasó por registros y no hubo frame: es una función *hoja* (no llama a
nadie) y sus dos variables entraron en `R0` y `R1`. De 24 bytes a 0.

Y hay un caso todavía más llamativo. El `factorial` recursivo de
[06 - Funciones](./06-funciones.md#funciones-recursivas):

```c
uint32_t factorial(uint32_t n)
{
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

- con `-O0` usa **16 bytes por llamada**, y `factorial(20)` consume 320 bytes de stack;
- con `-O2`, GCC lo **convierte en un bucle** y usa **0 bytes**, sin importar `n`.

> **Consecuencia práctica, y es una trampa real:** si medís el stack en modo debug (`-O0`) y grabás
> en release (`-O2`), los números no tienen nada que ver. **Medí siempre con los flags con los que
> vas a grabar.** Y no confíes en que el optimizador te salve la recursión: acá lo hizo, con una
> función un poco más compleja no lo hace.

### El contrato entre funciones: AAPCS

Para que una función compilada hoy pueda llamar a otra de una librería compilada hace años, ARM
define un contrato: el **AAPCS** (*Procedure Call Standard for the Arm Architecture*). Lo que te
conviene saber de memoria:

| Registro | Rol | ¿Quién lo preserva? |
|---|---|---|
| `R0`–`R3` | primeros 4 argumentos; `R0` además es el valor de retorno | **nadie**: el llamador asume que se rompen |
| `R4`–`R11` | variables locales | **la función llamada**: si los usa, los guarda y los restaura |
| `R12` | scratch temporal | nadie |
| `R13` (`SP`) | stack pointer | siempre válido |
| `R14` (`LR`) | dirección de retorno | la función llamada, si va a llamar a otra |
| `R15` (`PC`) | program counter | — |

Dos cosas que se deducen de la tabla:

- **Los primeros 4 argumentos viajan por registros, del quinto en adelante van al stack.** Una
  función con muchos parámetros cuesta más stack. Si tenés que pasar 8 cosas, pasá un puntero a una
  `struct`.
- **Una función que llama a otra tiene que guardar `LR`** (si no, pierde su propia dirección de
  retorno). Por eso las funciones hoja suelen ser gratis y las intermedias no. Se ve directo en el
  prólogo: `push {r4, r5, lr}` contra `push {r7}`.

El AAPCS además exige que **el `SP` esté alineado a 8 bytes** en toda llamada pública. Por eso a
veces ves un `sub sp, #68` para un buffer de 64: los 4 de más son relleno de alineación.

### Tres consecuencias que ya viste (y ahora se explican)

**1. La local con basura.** Ya está: son las sobras del frame anterior.

```c
void f(void) {
    uint32_t x;          // NO vale 0: vale lo que dejó la función anterior
    if (x > 10) { ... }  // comportamiento indefinido
}
```

**2. El puntero colgante.** De [08 - Punteros](./08-punteros.md):

```c
int *mal(void) {
    int local = 42;
    return &local;   // el frame se libera al retornar: esa dirección ya no es tuya
}
```

La dirección sigue siendo válida como número, pero el `SP` ya subió por encima: la próxima llamada
va a escribir justo ahí.

**3. La recursión.** Cada nivel apila un frame entero. `factorial(1000)` con 16 bytes por nivel son
16 KB: **la mitad de toda la SRAM del micro**, por una sola función.

### El stack y las interrupciones

Hay una consecuencia que solo se ve en embebidos y vale oro:

> **Las variables locales son seguras frente a interrupciones. Las globales no.**

Cuando llega una IRQ, el núcleo apila **8 palabras (32 bytes)** en el stack actual (`R0`–`R3`, `R12`,
`LR`, el PC de retorno y el `xPSR`) y salta al handler (§34.3.3.7.1). El handler corre **arriba** del
frame de la función interrumpida, con su propio espacio. Nadie pisa las locales de nadie.

Las globales, en cambio, son una sola dirección compartida entre el `main` y la ISR: ahí sí hay
carrera, y para eso están `volatile` y las secciones críticas
([07 - Secciones críticas y atomicidad](../07_interrupciones/03-secciones-criticas-y-atomicidad.md)).

El precio de esa comodidad: **cada interrupción anidada cuesta 32 bytes de stack**, más lo que use el
handler. Los detalles del *stacking* automático están en
[07 - NVIC y vectores](../07_interrupciones/01-nvic-y-vectores.md#el-stacking-automático-por-qué-un-isr-es-una-función-c-normal).

---

## Zona 3: el heap

Es la zona de `malloc()`. La versión corta, porque tiene un capítulo entero:

- En una PC, `malloc` le pide memoria al sistema operativo. **En el LPC1769 no hay sistema
  operativo.** El heap es apenas **una región de RAM que el linker dejó libre** después de `.bss`.
- La librería C (newlib) llega ahí a través de una función que **vos** tenés que proveer: `_sbrk()`.
  El linker define el símbolo `end` justo al final de `.bss`, y el heap arranca ahí y crece hacia
  arriba, de frente al stack. Podés leer la implementación real, comentada, en
  [`plantilla/src/syscalls.c`](../../plantilla/src/syscalls.c):

  ```c
  void *_sbrk(ptrdiff_t incr)
  {
      extern char end;              /* fin de .bss, lo pone el linker script */
      static char *heap_actual = NULL;

      if (heap_actual == NULL) heap_actual = &end;

      char *tope_stack = (char *) __builtin_frame_address(0);
      char *anterior   = heap_actual;

      if (heap_actual + incr > tope_stack) {   /* ¿me estoy comiendo el stack? */
          errno = ENOMEM;
          return (void *) -1;                  /* malloc() devuelve NULL */
      }
      ...
  }
  ```

  Ese `if` es lo único que separa un `malloc` que devuelve `NULL` honestamente de un `malloc` que te
  entrega memoria que el stack va a pisar diez minutos después.

- **En firmware, la regla es no usarlo**: es no determinista, fragmenta, y no hay MMU que detecte el
  choque. El *por qué* completo y las tres alternativas (estático, bump allocator, memory pool)
  están en [11 - Asignación dinámica](./11-asignacion-dinamica.md).

---

## Las cuatro zonas, lado a lado

| | `.rodata` | `.data` / `.bss` | **Stack** | **Heap** |
|---|---|---|---|---|
| Memoria física | Flash | RAM | RAM | RAM |
| Qué guarda | `const`, literales | globales y `static` | locales, parámetros, retornos | `malloc()` |
| Tamaño decidido | al compilar | al compilar | **en ejecución** | en ejecución |
| Quién lo administra | el linker | el linker + el startup | **el hardware** (`SP`) y el compilador | vos, vía `_sbrk` |
| Tiempo de vida | todo el programa | todo el programa | mientras dure la función | hasta el `free()` |
| Costo de asignar | 0 | 0 | **1 instrucción** (`sub sp, #N`) | cientos de ciclos, variable |
| ¿Se puede quedar sin? | error de linkeo | error de linkeo | **falla silenciosa** | `NULL` (si `_sbrk` chequea) |
| Recomendación en firmware | usalo mucho | usalo | usalo, pero medilo | evitalo |

La fila que importa es la anteúltima: **de las cuatro zonas, el stack es la única que puede fallar
sin avisar.** Y la de "quién lo administra" es la que ya viste en detalle en
[¿Quién decide la dirección, y cuándo?](#quién-decide-la-dirección-y-cuándo): el linker fija las dos
primeras columnas de una vez y para siempre, las otras dos se resuelven recién en ejecución.

---

## Cuando el stack y el heap se encuentran: *stack overflow*

El stack baja. El heap (y `.bss`, que está fijo abajo) sube. En el medio hay un espacio libre que
**nadie vigila**.

```
        │      STACK       │  ← si crece de más...
        │        ↓         │
        │  ...libre...     │
        │        ↑         │
        │      .bss        │  ← ...empieza a escribir ACÁ ENCIMA
```

Un *stack overflow* en el LPC1769 **no genera una excepción por sí mismo**. Simplemente el `SP`
sigue bajando y las escrituras del próximo frame caen sobre tus variables globales. No hay MMU
configurada por defecto, no hay guardia, no hay mensaje.

Los síntomas típicos, y por qué son tan difíciles de diagnosticar:

- Una variable global **cambia sola**, sin que nadie la escriba.
- El micro se cuelga o se resetea **en un lugar distinto cada vez**.
- Funciona en debug (`-O0`, frames grandes... o al revés) y falla en release.
- Cae en `HardFault_Handler`: cuando la corrupción llega a una **dirección de retorno**, el `bx lr`
  salta a una dirección inventada. Ese es el camino más común de "stack overflow" a "hard fault",
  y está desarrollado en [12 - El debugger y el método](../12_debug/02-debugger-y-metodo.md).

Causas concretas, en orden de frecuencia:

1. **Un array local grande.** `uint8_t buffer[1024];` dentro de una función son 1 KB de stack de una.
   Si el buffer es grande, hacelo `static` (pasa a `.bss`) o global.
2. **Recursión** sin profundidad acotada.
3. **Cadenas de llamadas profundas**, sobre todo si en el medio hay `printf` (newlib usa cientos de
   bytes de stack; con `%f`, más).
4. **Interrupciones anidadas**: cada nivel son 32 bytes del hardware más el frame del handler.
5. **Reservar poco stack en el linker script** para lo que el programa realmente hace.

---

## Cuánto stack usa tu programa (y cómo saberlo)

Esta es la parte que casi nunca se enseña y es la que separa "me anda" de "sé que me anda".

### a) `-fstack-usage`: cuánto usa cada función

GCC te dice, función por función, cuántos bytes de frame reserva. Genera un archivo `.su` al lado del
`.o`:

```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O2 -c d3.c -o d3.o -fstack-usage
$ cat d3.su
d3.c:5:10:muestrear      80      static
d3.c:15:10:factorial      0      static
```

Se lee: `archivo:línea:columna:función  bytes  tipo`. `muestrear` (que tiene un `uint32_t
buffer[16]`) reserva 80 bytes: 64 del array, 12 del `push {r4, r5, lr}` y 4 de alineación.

El `tipo` puede ser:

- **`static`**: el tamaño es fijo y conocido. Es lo que querés ver siempre.
- **`dynamic`**: usa `alloca()` o un array de tamaño variable (VLA) → **el tamaño depende de datos en
  ejecución y no se puede acotar**. En firmware esto es una bandera roja.
- **`bounded`**: dinámico pero con cota conocida.

> **Ojo con dos cosas.** Primero, el `.su` toma el nombre del **archivo de salida**: con
> `-o d3_O0.o` el archivo es `d3_O0.su`, no `d3.su`. Segundo, y más importante: **esto es por
> función, no el total.** Nadie suma la cadena de llamadas por vos.

### b) El peor caso es una suma que tenés que hacer vos

El stack máximo que va a necesitar tu firmware es:

```
   stack de la cadena de llamadas más profunda del main
 + stack del handler de interrupción más caro
 + 32 bytes por cada nivel de anidamiento de interrupciones
 + un margen (30 % es razonable)
```

`-fstack-usage` te da los sumandos; el árbol de llamadas lo armás vos (o con
[`-fcallgraph-info`](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html), o con herramientas
como `puncover`). Y ojo con los **punteros a función** y los **callbacks**: ahí el análisis estático
se corta, porque nadie sabe a quién vas a llamar.

### c) Reservarlo en el linker script, y que el linkeo falle si no entra

En [`plantilla/linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld) el stack no es una esperanza,
es un requisito verificado:

```ld
_Min_Stack_Size = 2K;   /* espacio que le exigimos al stack */
_Min_Heap_Size  = 0;    /* 0 = no usamos malloc */

/* ... al final del archivo ... */
ASSERT(_ebss + _Min_Heap_Size + _Min_Stack_Size <= ORIGIN(RAM) + LENGTH(RAM),
       "ERROR: no entra en la RAM. Las variables globales dejaron menos de
        _Min_Stack_Size libre para el stack.")
```

El truco es sencillo y potentísimo: si tus globales crecieron tanto que ya no quedan 2 KB libres,
**falla el linkeo**, en tu máquina, ahora. La alternativa es descubrirlo en la placa, en la demo, a
las 3 de la mañana.

### d) Leer lo que ya te está diciendo el build

Compilando la plantilla del repo tal cual:

```console
$ cd plantilla && make
Memory region         Used Size  Region Size  %age Used
           FLASH:         608 B       512 KB      0.12%
             RAM:        2080 B        32 KB      6.35%

   text    data     bss     dec     hex filename
    600       8    2080    2688     a80 build/firmware.elf
```

¿2080 bytes de `.bss` en un programa que hace parpadear un LED y no declara ni una global grande?
Mirando el archivo `.map` se entiende:

```
._user_heap_stack
                0x1000001c      0x804
                0x10000020        PROVIDE (end = .)
                0x10000820        . = (. + _Min_Stack_Size)
```

`0x800` = 2048 de esos bytes **son la reserva del stack**, no variables. Las globales reales son unos
28 bytes. Es decir: **la herramienta ya te está contando el stack reservado**, si sabés leerlo. Sin
esa sección en el `.ld`, `size` te diría "28 bytes de bss" y te dejaría creyendo que tenés 32 KB
libres.

Más sobre cómo leer `size` y el `.map`, en
[16 - De código a binario](../anexos/A_build_linker_startup/01-de-codigo-a-binario.md).

### e) Pintar el stack: la medición empírica (*watermark*)

Todo lo anterior es análisis estático. Esta técnica mide **lo que realmente pasó** en la placa, y es
la que usan los RTOS para reportar el "stack libre" de cada tarea.

La idea: al arrancar, llenás toda la RAM libre con un patrón reconocible. Después de correr un rato,
mirás **hasta dónde sobrevivió el patrón**. Todo lo que fue pisado es stack que efectivamente se usó.

```c
#include <stdint.h>

extern char end;                 /* fin de .bss: lo define el linker script */
#define PATRON 0xC0DEBEEFu

/* Llamar al PRINCIPIO de main(), antes de cualquier otra cosa. */
void stack_pintar(void)
{
    uint32_t *p    = (uint32_t *) &end;
    uint32_t *tope = (uint32_t *) __builtin_frame_address(0);

    while (p < tope - 16) {      /* -16 palabras de margen: no pisar este mismo frame */
        *p++ = PATRON;
    }
}

/* Llamar cuando quieras: devuelve los bytes que NUNCA se usaron. */
uint32_t stack_libre_minimo(void)
{
    const uint32_t *p = (const uint32_t *) &end;
    uint32_t libres = 0;

    while (*p == PATRON) {
        p++;
        libres += 4;
    }
    return libres;
}
```

Si después de horas de funcionamiento `stack_libre_minimo()` devuelve 1500, te sobraron 1500 bytes en
el peor momento observado. Si devuelve 40, estás a punto de romper todo.

> **La limitación, y es importante:** esto mide **lo que pasó**, no **lo que puede pasar**. Si esa
> corrida no ejecutó el camino más profundo (el manejo de un error raro, la ISR que casi nunca
> dispara), el número te va a mentir por optimista. Por eso las dos técnicas son complementarias:
> `-fstack-usage` acota el peor caso teórico, el *watermark* confirma el caso real.

### f) El debugger

Frenado en cualquier punto, el debugger te muestra el **call stack** (la cadena de frames vivos) y el
valor del `SP`. Comparar `SP` contra `_estack` te dice cuánto stack se consumió hasta ese instante.
Es la forma más rápida de contestar "¿qué me trajo hasta acá?" cuando caés en un hard fault
([12 - Debug](../12_debug/02-debugger-y-metodo.md)).

---

## Reglas prácticas

| Regla | Por qué |
|---|---|
| Buffers grandes: `static` o globales, **no** locales | 1 KB local es 1 KB de stack de golpe; como `.bss` el linker lo cuenta y te avisa |
| Tablas de solo lectura: `static const` | van a `.rodata` en Flash, no gastan RAM |
| Evitá la recursión sin profundidad acotada | cada nivel es un frame; no hay red de seguridad |
| Nunca devuelvas un puntero a una local | el frame se libera al retornar |
| Locales para lo compartido con ISRs; globales `volatile` solo cuando hace falta | cada llamada tiene su frame, las globales son una sola dirección |
| Pasá `struct *` en vez de 8 parámetros sueltos | del 5.º argumento en adelante, todo va al stack |
| Reservá el stack en el `.ld` y poné el `ASSERT` | convierte una falla de campo en un error de linkeo |
| Medí con los flags de release, no de debug | `-O0` y `-O2` dan números completamente distintos |
| Cuidado con `printf` en funciones profundas | newlib se lleva cientos de bytes de stack |
| Si ves `dynamic` en un `.su`, sacá el VLA | tamaño de stack no acotable |
| No supongas que dos variables declaradas seguidas quedan pegadas en memoria | el orden y el relleno los eligen el compilador y el linker; la autoridad es el `.map` |

---

## Para los curiosos (avanzado)

### Hay dos stack pointers, no uno

El Cortex-M3 implementa **dos** stacks con punteros independientes (§34.3.1.2):

- **MSP** (*Main Stack Pointer*): el que se usa por defecto. Es el que se carga desde `0x00000000` al
  resetear, y el que usan **siempre** los handlers de excepción (en Handler mode no hay opción).
- **PSP** (*Process Stack Pointer*): opcional. En Thread mode, el bit `SPSEL` del registro `CONTROL`
  elige cuál se usa.

¿Para qué sirve tener dos? Para un **RTOS**: cada tarea tiene su propio stack sobre el `PSP`, mientras
el kernel y las interrupciones corren sobre el `MSP`. Así el desborde de una tarea no se lleva puesto
al kernel, y el cambio de contexto es cambiar un puntero. En el superloop de este curso solo usás el
`MSP`. Lo retomamos en [17 - Intro a RTOS](./19-intro-a-rtos.md).

### La alineación a 8 bytes en las excepciones

El AAPCS exige `SP` alineado a 8 en las llamadas públicas, pero una interrupción puede caer en
cualquier instrucción, incluso con el `SP` alineado solo a 4. Para eso el núcleo tiene el bit
**`STKALIGN`** del registro `CCR`: si vale 1, al entrar a la excepción el hardware **inserta un
relleno** para dejar el frame alineado a 8, y anota lo que hizo en el bit 9 del `xPSR` apilado para
poder deshacerlo al salir. Es automático, pero explica por qué a veces el stacking consume 36 bytes y
no 32.

### VLA y `alloca()`: por qué no

```c
void f(uint32_t n) {
    uint32_t buf[n];   // VLA: el tamaño se decide en ejecución
}
```

Compila (C99 lo permite), pero el frame pasa a depender de un dato de runtime: **no hay forma de
acotar el stack**. Si `n` viene de un paquete recibido por UART, un valor grande es un stack overflow
directo, y encima controlable desde afuera. Por eso MISRA C lo prohíbe y por eso el kernel de Linux
los eliminó. Compilá con **`-Wvla`** para que el compilador te frene, y con
**`-Wstack-usage=512`** para que avise cuando alguna función se pase de ese frame.

### ¿Y no hay una red de seguridad por hardware?

Algo hay, pero no viene gratis. El LPC1769 **sí incluye una MPU** (*Memory Protection Unit*) de 8
regiones (capítulo 1 del [UM10360](../../manual/ch01_introductory-information.pdf)). Se la puede
configurar para marcar una franja justo debajo del stack como "sin permiso de escritura": el desborde
pasa a generar un **MemManage fault** inmediato y determinista en vez de corromper `.bss` en silencio.
Es exactamente lo que hacen los RTOS con protección de tareas. Configurarla es trabajo, y por defecto
está apagada: de fábrica, el desborde es silencioso.

---

## Resumen

| Si escribís… | Vive en… | Arranca valiendo… | Muere… |
|---|---|---|---|
| `const int tabla[] = {...}` (global) | Flash, `.rodata` | lo que escribiste | nunca |
| `int x = 5;` (global) | RAM, `.data` | 5 (lo copia el startup) | nunca |
| `int x;` (global) | RAM, `.bss` | 0 (lo pone el startup) | nunca |
| `static int x;` (dentro de una función) | RAM, `.bss` | 0 | nunca |
| `int x;` (dentro de una función) | **Stack** | **basura** | al salir de la función |
| `int buf[256];` (dentro de una función) | **Stack** (1 KB de una) | basura | al salir de la función |
| `malloc(n)` | Heap | basura | en el `free()` (si llega) |

Y quién elige cada dirección:

| Si escribís… | La dirección la decide… | Y queda fijada… |
|---|---|---|
| cualquier global o `static` | el **linker**, según el `.ld` | al **enlazar**, para siempre |
| una local | el compilador (offset) + el `SP` (base) | **en cada llamada**, distinta cada vez |
| `malloc(n)` | el asignador | en ejecución |

Y las cuatro frases del capítulo:

1. **El stack crece hacia abajo desde el tope de la RAM, y `.bss` crece hacia arriba desde la base.
   En el medio no hay nadie vigilando.**
2. **Las locales viven en el stack y su memoria se reutiliza**: de ahí la basura inicial, los punteros
   colgantes y el costo de la recursión.
3. **El stack es la única zona cuyo tamaño nadie calcula por vos.** Medilo con `-fstack-usage`,
   reservalo con el `ASSERT` del linker, confirmalo pintando la RAM.
4. **El compilador no elige direcciones: elige secciones y offsets.** La dirección absoluta de una
   global la pone el linker, y la de una local recién existe cuando la función se está ejecutando.

---

## Fuentes y para seguir leyendo

**El manual del chip (está en el repo)**
- [UM10360, Capítulo 34 - Cortex-M3 User Guide](../../manual/ch34_appendix-cortex-m3-user-guide.pdf).
  §34.3.1.2 *Stacks* (full descending, MSP y PSP), §34.3.1.3.2 (el `SP` se carga desde `0x00000000`
  al resetear), §34.3.3.7.1 *Exception entry* (el stack frame de 8 palabras y la alineación a
  doble palabra), y el registro `CCR` con el bit `STKALIGN`.
- [UM10360, Capítulo 2 - Memory map](../../manual/ch02_memory-map.pdf). Las direcciones y tamaños de
  la Flash y de las tres SRAM.
- [UM10360, Capítulo 1 - Introductory information](../../manual/ch01_introductory-information.pdf).
  Confirma que el LPC1769 incluye MPU de 8 regiones.

**Los estándares**
- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf).
  §6.2.4 *Storage durations of objects* define las duraciones **estática** y **automática** que son la
  base de este capítulo. Ojo: el estándar **no menciona la palabra "stack"** en ningún lado; habla de
  tiempos de vida, y el stack es cómo la implementación los realiza.
- [Procedure Call Standard for the Arm Architecture (AAPCS32)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst).
  El contrato de registros de la tabla de arriba, y la regla de alineación del `SP` a 8 bytes.
- [ARMv7-M Architecture Reference Manual](https://developer.arm.com/documentation/ddi0403/latest/).
  La definición formal de `PUSH`/`POP`, `SP`, `LR` y el modelo de excepciones.

**GCC y las herramientas**
- [GCC: Developer Options - `-fstack-usage`](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html).
  El formato del archivo `.su` y el significado de `static` / `dynamic` / `bounded`.
- [GCC: Warning Options - `-Wstack-usage=`, `-Wvla`, `-Walloca`](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html).
- [GNU ld: Scripts](https://sourceware.org/binutils/docs/ld/Scripts.html). `MEMORY`, `SECTIONS`,
  `PROVIDE` y `ASSERT`, que son las piezas del `.ld` que se citan acá.
- [GNU binutils: `nm`](https://sourceware.org/binutils/docs/binutils/nm.html). Las letras de la
  columna del medio (`D` = `.data`, `B` = `.bss`, `R` = `.rodata`, `T` = `.text`) son la forma más
  rápida de confirmar en qué sección terminó cada símbolo.
- [newlib: `_sbrk` y los syscalls que hay que proveer](https://sourceware.org/newlib/libc.html#Syscalls).

**Los archivos de este repo que muestran todo esto funcionando**
- [`plantilla/linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld): `_Min_Stack_Size`, `_estack`,
  la sección `._user_heap_stack` y el `ASSERT` que hace fallar el linkeo.
- [`plantilla/startup/startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c): la copia de
  `.data` y el borrado de `.bss` antes de `main`.
- [`plantilla/src/syscalls.c`](../../plantilla/src/syscalls.c): el `_sbrk()` con el chequeo contra el
  stack pointer.

**Todo lo de este capítulo se puede reproducir con el toolchain del repo:**
```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O2 -c archivo.c -o archivo.o -fstack-usage
$ cat archivo.su                       # cuánto stack usa cada función
$ arm-none-eabi-objdump -d archivo.o   # el prólogo y el epílogo, en fierros
$ arm-none-eabi-nm --size-sort -S firmware.elf   # qué símbolo se comió la RAM
```

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [09 - Punteros avanzados](./09-punteros-avanzado.md) ·
**Siguiente:** [11 - Asignación dinámica](./11-asignacion-dinamica.md)
