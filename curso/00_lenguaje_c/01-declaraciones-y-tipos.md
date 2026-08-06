# Declaraciones, tipos y constantes

## Declaraciones de variables

Indican qué variables vamos a usar, qué tipo tienen y (opcionalmente) su valor inicial.

Tienen el siguiente formato:

```c
[especificador] [calificador] [modificador]  [tipo] [nombre] = [valor inicial];
```

> *Nota:* No todos los elementos aparecen siempre. En una declaración, son obligatorios solo el **tipo** y el **nombre** de la variable.

Ejemplo simple:

```c
int x = 10; // tipo int, nombre x, valor inicial 10
```

Las secciones que siguen recorren esas seis partes, pero **no en el orden en que se escriben**: arrancamos por el tipo, que es lo único obligatorio junto con el nombre, y de ahí vamos hacia afuera. Este es el mapa:


| Parte de la línea | Sección                                                     |
| ----------------- | ----------------------------------------------------------- |
| `[tipo]`          | [1. Tipos de datos](#1-tipos-de-datos)                      |
| `[especificador]` | [2. Especificador de almacenamiento](#2-especificador-de-almacenamiento) |
| `[calificador]`   | [3. Calificador de tipo](#3-calificador-de-tipo)            |
| `[modificador]`   | [4. Modificadores del tipo](#4-modificadores-del-tipo)      |
| `[nombre]`        | [5. Nombre de variables](#5-nombre-de-variables)            |
| `= [valor inicial]` | [6. Inicialización](#6-inicialización-opcional)           |

### 1. **Tipos de datos**


| Tipo            | Descripción                                                                                                                                                            |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void`          | Ausencia de tipo. No podés declarar una variable `void`: se usa para funciones que no devuelven nada, para "no recibe parámetros" y para punteros genéricos (`void *`) |
| `char`          | Un byte (al menos 8 bits). Pensado para caracteres                                                                                                                     |
| `short int`     | Entero de al menos 16 bits                                                                                                                                             |
| `int`           | Entero, del tamaño natural de la máquina (al menos 16 bits)                                                                                                            |
| `long int`      | Entero de al menos 32 bits                                                                                                                                             |
| `long long int` | Entero de al menos 64 bits (desde C99)                                                                                                                                 |
| `float`         | Punto flotante, precisión simple                                                                                                                                       |
| `double`        | Punto flotante, precisión doble                                                                                                                                        |
| `long double`   | Punto flotante, precisión **al menos** igual a `double`                                                                                                                |
| `_Bool`         | Booleano, `0` o `1` (desde C99; con `<stdbool.h>` se escribe `bool`)                                                                                                   |


Cada tipo entero tiene además su versión **sin signo** (`unsigned int`, `unsigned long`...), y las palabras `short`/`long` se pueden escribir solas: `short` es lo mismo que `short int`. Eso se ve en [4. Modificadores del tipo](#4-modificadores-del-tipo).

> *Ojo con `long double`:* el estándar solo pide que **no sea más chico** que `double`. En x86 con GCC el formato es el extendido de 80 bits de la FPU, aunque `sizeof` dé más por el relleno de alineación (16 en x86-64, 12 en x86 de 32 bits); en `arm-none-eabi-gcc` para el LPC1769 `long double` es **exactamente igual que `double`** (64 bits). "Precisión extendida" no es algo que tengas garantizado.

> *Detalle de precisión:* el estándar llama **tipos básicos** a `char`, a los tipos enteros con y sin signo y a los de punto flotante (§6.2.5 ¶14). `void` **no** es uno: es un tipo *incompleto*. Está en la tabla porque es una palabra clave de tipo que vas a escribir todo el tiempo.

---

### 2. **Especificador de almacenamiento**

La **clase de almacenamiento** de una variable define:

- Su **alcance (scope)**, su **tiempo de vida (lifetime)** y **ubicación de almacenamiento**

Pueden ser:

- `auto`: se destruye cuando la función termina.
- `static`: se inicializa solo una vez y luego conserva su valor entre llamadas.
- `extern`: se define en otro archivo y se puede usar en el archivo actual.
- `register`: se intenta usar un registro de la CPU, pero no es garantizado.

---

Si no se declara ningún especificador:

- Las variables **dentro** de funciones son `auto` por defecto: viven en el stack y mueren al salir.
- Las variables **fuera** de funciones ya tienen *tiempo de vida* estático (duran todo el programa) sin escribir nada, pero su **enlace (linkage) es externo**: otros archivos las pueden ver con `extern`.

> [!IMPORTANT]
> Una variable global **no** es `static` por defecto. `static` a nivel de archivo no cambia el tiempo de vida (ya era todo el programa), lo que hace es **ocultarla**: pasa a tener enlace *interno* y ningún otro archivo puede referenciarla. Son dos cosas distintas que se llaman igual:
>
> - **duración estática** → cuánto vive (todo el programa).
> - **palabra `static`** → quién la ve (solo este archivo, o solo esta función).

```c
int contador = 0;         // global: duración estática + enlace EXTERNO (otros .c la ven)
static int privada = 0;   // global: duración estática + enlace INTERNO (solo este .c)

void f(void) {
    int a = 0;            // auto: se reinicia en cada llamada
    static int b = 0;     // duración estática, pero visible solo dentro de f()
    b++;                  // conserva su valor entre llamadas
}
```

> En C, `auto` es puramente decorativo: nunca hace falta escribirlo. (En **C23** la palabra `auto` se reutilizó para inferencia de tipos, como en C++; con el `-std=gnu17` que usa `arm-none-eabi-gcc` por defecto eso todavía no aplica.)

---


| Especificador | Ubicación de almacenamiento                        | Tiempo de vida                  | Alcance (scope)                                                        | Valor inicial                           | Comentarios clave                               |
| ------------- | -------------------------------------------------- | ------------------------------- | ---------------------------------------------------------------------- | --------------------------------------- | ----------------------------------------------- |
| `auto`        | Stack (RAM)                                        | Mientras la función esté activa | Local a la función                                                     | **Indeterminado** (ver nota)            | Valor no se conserva entre llamadas             |
| `static`      | RAM (`.data`/`.bss`), o Flash si además es `const` | Todo el programa                | Local (si está dentro de función) o interna al archivo (si está fuera) | Cero (si no se inicializa)              | Mantiene su valor entre llamadas                |
| `register`    | Registro de CPU (si está disponible)               | Mientras la función esté activa | Local a la función                                                     | Indefinido                              | Más rápido (teóricamente); no se puede usar `&` |
| `extern`      | Donde la haya definido el otro archivo             | Todo el programa                | Global (visible en otros archivos)                                     | No aplica: `extern` no crea la variable | Se usa para compartir variables entre archivos  |


Tres aclaraciones sobre la tabla:

- **`extern` no reserva memoria.** Es solo una *declaración*: le avisa al compilador "esta variable existe en algún lado, confiá y que el linker la encuentre". La *definición* (la que sí reserva memoria y se inicializa en cero) está en exactamente uno de los `.c`. Lo típico es poner el `extern` en el `.h` y la definición en el `.c`.
- **`static const` no gasta RAM.** En el LPC1769, una tabla `static const uint16_t tabla[256]` la pone el linker en `.rodata`, que vive en **Flash** (512 KB) y no en los 64 KB de RAM. Es la forma de guardar tablas de lookup grandes sin comerte la RAM.
- **`register` hoy no sirve de mucho.** GCC con `-O2` ya asigna registros mejor que vos; lo único que `register` sigue garantizando es que **no podés tomarle la dirección** con `&`.
- **"Indeterminado" no es lo mismo que "basura".** El estándar dice que una `auto` sin inicializar tiene un valor *indeterminado*, y **leerlo es comportamiento indefinido**, no "te da un número cualquiera". El compilador puede asumir que eso nunca pasa y optimizar en consecuencia, así que el programa puede hacer cosas que no se explican con "quedó lo que había antes en el stack". Inicializá siempre.

---

> [!NOTE]
> [What are storage class specifiers in C?](https://how.dev/answers/what-are-storage-class-specifiers-in-c)

---

### 3. **Calificador de tipo**

Afectan cómo el compilador **trata el contenido de la variable**.


| Calificador | ¿Qué hace?                                                                     | Ejemplo embebido            |
| ----------- | ------------------------------------------------------------------------------ | --------------------------- |
| `const`     | El valor no se puede modificar **a través de ese nombre**                      | `const float PI = 3.14f;`   |
| `volatile`  | Puede cambiar fuera del programa (por hardware)                                | `volatile uint32_t *port;`  |
| `restrict`  | Promesa de **no aliasing**: a ese dato se accede *solo* por este puntero (C99) | `void f(int * restrict a);` |


> En sistemas embebidos, `volatile` es **crítico** para registros de periféricos.

**Sobre `restrict`:** no es "la única forma de acceder es un puntero". Es una **promesa que le hacés al compilador**: durante la vida de ese puntero, al objeto apuntado se accede únicamente a través de él (o de punteros derivados de él). Con esa garantía el compilador puede mantener valores en registros en vez de releer memoria. Si le mentís (si dos punteros `restrict` apuntan a lo mismo y escribís por ambos), es **comportamiento indefinido** (*undefined behavior*, **UB**: el estándar no define qué pasa, así que el compilador puede hacer cualquier cosa). Por eso `memcpy()` declara sus dos punteros `restrict` (las regiones no se pueden solapar) y `memmove()` **no** (sí se pueden solapar).

> [!WARNING]
> **`const` en C no es una constante de compilación.** `const int N = 8;` no crea "el número 8": crea una **variable** que vale 8 y que prometés no modificar. El compilador la trata como variable, y hay tres lugares donde eso importa porque el compilador necesita el valor **antes** de que el programa exista: el tamaño de un arreglo, las etiquetas `case` y las directivas `#if`.
>
> Lo que pasa en cada caso, en concreto:
>
> ```c
> const int N = 8;
>
> int buf[N];              // (1) a nivel de archivo: NO COMPILA
>                          //     error: variably modified 'buf' at file scope
>
> void f(int x) {
>     int buf[N];          // (2) dentro de una función: COMPILA, pero es un VLA
>     switch (x) {
>         case N: break;   // (3) NO COMPILA
>     }                    //     error: case label does not reduce to an integer constant
> }
>
> #if N == 8               // (4) COMPILA, pero está MAL: el preprocesador no conoce
>     ...                  //     las variables de C, así que 'N' vale 0 y esto es falso.
> #endif                   //     Se ve con -Wundef: warning: "N" is not defined, evaluates to 0
> ```
>
> El caso (2) es el que más confunde, porque **no da error**: al estar dentro de una función, C99 lo acepta como **VLA** (*variable length array*, arreglo de tamaño variable). Un VLA es un arreglo cuyo tamaño se calcula **al ejecutar**, y se reserva en el stack en ese momento. Para el compilador es lo mismo que `int buf[x];` con `x` viniendo de un parámetro: no sabe cuánto va a medir. Anda, pero en firmware **conviene evitarlos**: si el tamaño llega a ser grande te comés el stack y el micro se cuelga, y no hay forma de saber de antemano cuánta RAM necesita tu programa (ver [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md)). Compilá con `-Wvla` y el compilador te avisa cuando aparece uno sin querer.
>
> **Entonces, para tamaños de arreglo, `case` y `#if`, usá `#define` o `enum`:**
>
> ```c
> #define    N2   8
> int buf2[N2];        // OK: el preprocesador reemplaza por 8 antes de compilar
>
> enum { N3 = 8 };
> int buf3[N3];        // OK, y además es un símbolo real (lo ve el debugger, tiene tipo)
> ```
>
> Un último detalle: `const` significa "prometo no escribirlo **por este nombre**", **no** "está en memoria de solo lectura". Si otro alias sin `const` lo modifica, es comportamiento indefinido. Y ojo que ese alias (`*(uint32_t *)&CLOCK_HZ = ...`) compila **sin un solo warning** con `-Wall -Wextra`: hace falta agregar `-Wcast-qual` para que GCC avise `cast discards 'const' qualifier`, y aun así es warning, no error.

#### ¿Cuál es la diferencia entre poner `volatile` y no ponerlo?

Basicamente, le decimos al compilador que no optimice la lectura/escritura de esta variable, porque puede cambiar sin que el código que ve la modifique.

Supongamos que declaramos una variable que es modificada por una interrupción de hardware, no especificamos `volatile`.

```c
#include <stdint.h>

uint8_t flag = 0;  // Esta la cambia una ISR

void ISR_timer(void) {
    flag = 1;      // Se llama cuando pasa un tiempo
}

int main(void) {
    while (1) {
        if (flag == 1) {
            // Aquí llega cuando flag == 1
        }
    }
    return 0;
}
```

El compilador, al ver que flag nunca cambia dentro de `main`, puede guardar el valor en un registro de CPU y no volver a leerlo de memoria.
Si el hardware cambia flag a 1, el programa nunca se entera porque está mirando una copia desactualizada. Resultado: Nunca ingresa al `if`.

La corrección es una sola palabra:

```c
volatile uint8_t flag = 0;   // ahora el compilador relee la memoria en cada iteración
```

> [!CAUTION]
> Este bug es **traicionero** porque **depende del nivel de optimización**. Compilado con `-O0` (sin optimizar) el programa "funciona", porque GCC releé la variable de memoria de todas formas. Recién cuando pasás a `-O1`/`-O2`/`-Os`, o sea cuando compilás la versión final, el compilador saca la lectura del lazo y el programa se cuelga. Poné `volatile` desde el principio en toda variable compartida con una ISR o con un periférico, no cuando "aparezca" el problema.

> `volatile` resuelve la **visibilidad** de la variable, no la **atomicidad**. Si la ISR y el `main` hacen lectura-modificación-escritura sobre la misma variable (por ejemplo `contador++`), `volatile` no te salva: sigue habiendo una condición de carrera. Eso se ve en [12 - `volatile` y tipos para hardware](./12-volatile-y-tipos-para-hardware.md).

---

### 4. **Modificadores del tipo**

Los modificadores no crean tipos nuevos de la nada: **ajustan el tamaño o el signo** de un tipo base.


| Modificador                  | Qué afecta                     | Se aplica a   |
| ---------------------------- | ------------------------------ | ------------- |
| `short`, `long`, `long long` | Cambia el tamaño del entero    | `int`         |
| `long`                       | Pide más precisión             | `double`      |
| `signed`, `unsigned`         | Permite o no números negativos | tipos enteros |


> Combinables: `unsigned long int`, `short int`, etc. El orden no importa: `unsigned long int` y `int long unsigned` son el mismo tipo.

**Un detalle que confunde:** `int` se puede omitir cuando hay algún modificador. `short`, `long`, `unsigned` y `long long` a secas significan `short int`, `long int`, `unsigned int` y `long long int`. Son la misma cosa escrita más corto, no tipos distintos.

#### Ejemplo:

```c
unsigned char      codigo;     // 0 a 255
signed short int   temp;       // -32 768 a 32 767
unsigned long long acumulador; // 64 bits sin signo en el M3
long double        resultado;  // en el LPC1769, lo mismo que un double
```

> Todos los tipos enteros son `signed` por defecto **excepto `char`**, cuyo signo lo elige el compilador (ver más abajo). O sea: `short` ≡ `signed short`, pero `char` **no** es necesariamente `signed char`: son **tres** tipos distintos (`char`, `signed char` y `unsigned char`), aunque `char` se comporte igual que uno de los otros dos.

---

### 5. **Nombre de variables**

- Compuestos por letras, dígitos y `_`.
- El primer carácter **debe ser una letra** (o `_`, aunque no se recomienda).
- **Mayúsculas y minúsculas son distintas** (`a` ≠ `A`).
- Convención:
  - minúsculas para variables
  - MAYÚSCULAS para constantes simbólicas
- Evitar nombres que empiecen con `_`: **están reservados para la implementación** (el compilador y la librería estándar). En concreto, `_` seguido de mayúscula o de otro `_` (`_Bool`, `__attribute__`) está reservado *siempre*, y `_` seguido de minúscula está reservado a nivel de archivo. Si los usás, podés chocar con nombres internos de la libc.
- **El largo del nombre no es un problema:** el toolchain distingue los nombres completos, sin truncar. Usá los caracteres que necesites para que se entienda.
- Las **palabras clave** (`if`, `int`, `float`, etc.) están reservadas y **no se pueden usar como nombres**.
- Tampoco uses nombres de la librería estándar (`strlen`, `malloc`, `index`...): redefinirlos es UB, aunque compile.

> Buenas prácticas: usa nombres claros, cortos pero significativos.

---

### 6. **Inicialización (opcional)**

Se puede o no dar un valor de un literal, otra variable o una expresión:

```c
int led_pin = 13;
int led2_pin = led_pin + 1;
```

---

### Ejemplo completo

Juntemos todo en declaraciones reales de firmware. Estas dos líneas aparecen en casi cualquier proyecto del LPC1769:

```c
#include <stdint.h>

static volatile uint32_t ticks_ms = 0;           // la incrementa la interrupción del SysTick
static const    uint32_t CLOCK_HZ = 100000000u;  // frecuencia del núcleo: 100 MHz, fija
```

Desglose de la primera:


| Parte      | Qué es         | Función                                                                            |
| ---------- | -------------- | ---------------------------------------------------------------------------------- |
| `static`   | especificador  | Vive durante todo el programa y solo la ve este archivo                            |
| `volatile` | calificador    | La modifica una interrupción: el compilador tiene que releerla de memoria cada vez |
| `uint32_t` | tipo           | Entero sin signo de 32 bits                                                        |
| `ticks_ms` | nombre         | Cómo la llamamos                                                                   |
| `= 0`      | inicialización | Valor de arranque                                                                  |


La segunda cambia **una sola palabra** y con eso cambia todo: `const` en lugar de `volatile` significa que prometemos no modificarla nunca. Además, al ser `static const`, el linker la manda a `.rodata`, o sea a **Flash**: no gasta ni un byte de los 64 KB de RAM.

**Todo junto, la primera se lee así:** *"`ticks_ms` es una variable privada de este archivo (`static`), que existe durante todo el programa, guarda un entero de 32 bits sin signo (`uint32_t`), arranca valiendo 0, y puede cambiar en cualquier momento por fuera del flujo normal del programa (`volatile`), así que hay que ir a buscarla a memoria cada vez que se la lee."*

Fijate que los dos calificadores responden preguntas **distintas e independientes**: `const` dice *qué puede escribir tu código*; `volatile` dice *si se puede confiar en un valor ya leído*. Tanto es así que existe `const volatile`, y es exactamente lo que se usa para un **registro de hardware de solo lectura**: no lo podés escribir, y cambia solo. Ese caso necesita punteros, así que lo vemos en [12 - `volatile`, `const` y tipos propios](./12-volatile-y-tipos-para-hardware.md#const-volatile-registros-de-solo-lectura).

## Tamaños de los tipos

Los tamaños exactos **dependen del compilador y del hardware**. El estándar solo fija mínimos, que son los de la tabla de [1. Tipos de datos](#1-tipos-de-datos): 8 bits para `char`, 16 para `short` e `int`, 32 para `long` y 64 para `long long`.

Lo que sí garantiza siempre el estándar es el **orden relativo**:

```c
char <= short <= int <= long <= long long
```

Escrito con `sizeof`:

```c
sizeof(char)      = 1              // siempre 1 por definición
sizeof(short)    <= sizeof(int)
sizeof(int)      <= sizeof(long)
sizeof(long)     <= sizeof(long long)
```

> *Para los flotantes la garantía es otra:* el estándar no habla de `sizeof`, sino de **conjuntos de valores** (§6.2.5 ¶10): los valores de `float` son un subconjunto de los de `double`, y los de `double` un subconjunto de los de `long double`. En la práctica eso viene acompañado de `sizeof(float) <= sizeof(double) <= sizeof(long double)`, pero es una consecuencia de cómo lo implementan los compiladores reales, no algo que el estándar exija.

> **`sizeof(char) == 1` siempre, pero eso no significa "8 bits".** `sizeof` mide en unidades de `char`, y `char` es *por definición* la unidad de 1. Cuántos bits tiene realmente lo dice `CHAR_BIT` en `<limits.h>`, y el estándar solo exige `CHAR_BIT >= 8`. En el LPC1769 (y en cualquier máquina que vas a usar en la práctica) `CHAR_BIT == 8`, así que "byte" y "8 bits" coinciden. Existen DSPs donde `CHAR_BIT` es 16 y ahí `sizeof(int)` puede dar 1.

### Tamaño REAL de los tipos en el Cortex-M3 (LPC1769)

El estándar solo da garantías mínimas, pero a vos te interesa qué pasa **en tu micro**. Con `arm-none-eabi-gcc` sobre el LPC1769 (ABI estándar de ARM, AAPCS) los tamaños son:


| Tipo          | Tamaño en el M3 | Rango                                              |
| ------------- | --------------- | -------------------------------------------------- |
| `char`        | 8 bits          | 0 a 255 (es `unsigned`, ver abajo)                 |
| `short`       | 16 bits         | -32 768 a 32 767                                   |
| `int`         | **32 bits**     | -2 147 483 648 a 2 147 483 647                     |
| `long`        | 32 bits         | igual que `int`                                    |
| `long long`   | 64 bits         | ±9,2 · 10^18                                       |
| `float`       | 32 bits         | precisión simple (IEEE-754)                        |
| `double`      | 64 bits         | precisión doble (IEEE-754)                         |
| `long double` | 64 bits         | **igual que `double`** en el ABI de ARM de 32 bits |
| `_Bool`       | 8 bits          | `0` o `1`                                          |
| puntero (`*`) | 32 bits         | el M3 tiene un espacio de direcciones de 32 bits   |


Estos valores no son de memoria: los podés confirmar vos mismo con el toolchain del repo.

```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -dM -E - < /dev/null | grep SIZEOF
#define __SIZEOF_INT__ 4
#define __SIZEOF_LONG__ 4
#define __SIZEOF_LONG_LONG__ 8
#define __SIZEOF_SHORT__ 2
#define __SIZEOF_DOUBLE__ 8
#define __SIZEOF_LONG_DOUBLE__ 8
#define __SIZEOF_POINTER__ 4
```

Dos cosas importantes:

1. En el Cortex-M3 `int` es de **32 bits**, igual que `long` y que un puntero. El M3 es una máquina de 32 bits, así que operar con `int` es lo más natural y rápido para la ALU. Operar con `uint8_t` o `uint16_t` a veces obliga al compilador a **enmascarar** para que el resultado "entre" en 8 o 16 bits. Lo hace con las instrucciones `uxtb`/`uxth`, y el costo aparece cuando el valor angosto tiene que materializarse en cada paso. Un acumulador `uint8_t` dentro de un lazo, compilado con `-O2`, queda así:
  ```
   ldrb.w  r2, [r3, #1]!
   add     r0, r2
   cmp     r3, r1
   uxtb    r0, r0        <- enmascara en CADA vuelta
   bne.n   ...
  ```
   Con un acumulador `uint32_t` esa instrucción no está: el lazo tiene una instrucción menos por iteración. En cambio, si las cuentas son seguidas y nadie mira los valores intermedios, GCC opera en 32 bits y enmascara una sola vez al final. Por eso conviene **almacenar** en tipos angostos y **calcular** en `uint32_t`.
2. El M3 **no tiene unidad de punto flotante** (el LPC1769 es Cortex-M3, no M4F). Cada `float`/`double` se calcula por **software**, lo cual es lento. Evitá flotantes en código crítico; usá aritmética entera o de punto fijo cuando puedas.

> **`char` puede ser con o sin signo.** El estándar deja librado al compilador si `char` "pelado" es `signed` o `unsigned`. En ARM el ABI define `char` como **unsigned**, y `arm-none-eabi-gcc` lo cumple: define `__CHAR_UNSIGNED__` y `char` va de 0 a 255. En x86 con GCC, en cambio, `char` es **signed** (-128 a 127).
>
> Esto importa y muerde de verdad:
>
> ```c
> char c = 0x80;
> if (c < 0) { /* en tu PC: SÍ entra. En el LPC1769: NO entra. */ }
> ```
>
> El mismo código fuente, dos comportamientos. Acá GCC sí te ayuda, siempre que compiles con `-Wextra`:
>
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -Wextra -c signo.c
> warning: comparison is always false due to limited range of data type [-Wtype-limits]
> ```
>
> Verificá también el signo que eligió el compilador:
>
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -dM -E - < /dev/null | grep CHAR_UNSIGNED
> #define __CHAR_UNSIGNED__ 1
> ```
>
> (Se puede forzar con `-fsigned-char` / `-funsigned-char`, pero no lo hagas: escribí código que no dependa de eso.)
>
> **Moraleja:** usá `char` **solo para texto**. Para un byte de datos usá `uint8_t`, y si necesitás un byte con signo pedí `int8_t` o `signed char` explícitamente. Nunca dejes que el signo de `char` te importe.

### Conclusión práctica: tipos de ancho fijo (`stdint.h`)

Por todo esto, en embebido **declarás los enteros con los tipos de `<stdint.h>`** (`uint8_t`, `int32_t`, etc.) en vez de `int`/`short`/`long` pelados. Decís exactamente cuántos bits querés y el código se comporta igual en cualquier compilador:

```c
#include <stdint.h>
```

El header define tipos de tamaño exacto:


| Tipo       | Tamaño exacto | Descripción |
| ---------- | ------------- | ----------- |
| `int8_t`   | 8 bits        | Con signo   |
| `uint8_t`  | 8 bits        | Sin signo   |
| `int16_t`  | 16 bits       | Con signo   |
| `uint16_t` | 16 bits       | Sin signo   |
| `int32_t`  | 32 bits       | Con signo   |
| `uint32_t` | 32 bits       | Sin signo   |
| `int64_t`  | 64 bits       | Con signo   |
| `uint64_t` | 64 bits       | Sin signo   |


Ejemplo:

```c
#include <stdint.h>

uint8_t edad = 25;
int16_t temperatura = -120;
uint32_t contador = 100000;
```

Las ventajas son el ancho explícito, la portabilidad entre arquitecturas y que el lector del código no tiene que adivinar nada. **Es fundamental usarlos en sistemas embebidos**: de acá en adelante los usamos en todo el curso, y en [12 - `volatile`, `const` y tipos propios](./12-volatile-y-tipos-para-hardware.md) vas a ver cómo se combinan con `volatile` para llegar a un registro del micro.

**Detalles que conviene saber:**

- Los tipos de tamaño **exacto** (`intN_t`) son técnicamente **opcionales** en el estándar: solo existen si la máquina tiene un tipo de exactamente ese ancho y sin bits de relleno. En el Cortex-M3 están los cuatro (8/16/32/64), así que en este curso podés usarlos sin miedo. Los que están **garantizados siempre** son los de ancho *mínimo*, `int_leastN_t`.
- `<stdint.h>` también trae `int_fastN_t` ("el más rápido de al menos N bits"), `intptr_t` (un entero donde cabe un puntero) e `intmax_t`.
- Para los **límites** tenés macros propias: `UINT8_MAX`, `INT32_MIN`, `INT32_MAX`, etc.
- Para **escribir literales** de un ancho dado están las macros `UINT32_C(x)` / `INT64_C(x)`: `UINT32_C(0xFFFFFFFF)`.
- `size_t` (de `<stddef.h>`) es el tipo para **tamaños y cantidades**; es sin signo y en el M3 son 32 bits. Usalo para índices de arreglos y resultados de `sizeof`, no `int`.

> **Ojo al imprimir con `printf`.** Estos nombres son `typedef`, no tipos nuevos: cada uno es un alias de algún tipo entero del compilador, y **cuál** depende de la plataforma. En `arm-none-eabi-gcc`, `uint32_t` es un `unsigned long`, no un `unsigned int`:
>
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -dM -E - < /dev/null | grep UINT32_TYPE
> #define __UINT32_TYPE__ long unsigned int
> ```
>
> O sea que acá el especificador correcto para un `uint32_t` es `%lu`, y **`%u` es el que está mal**, aunque los dos midan 32 bits. En un x86 de 64 bits es exactamente al revés. Como el ancho coincide, el error no se nota casi nunca, pero es UB igual.
>
> La solución portable es no adivinar: `<inttypes.h>` define una macro por tipo, que expande al especificador que corresponda en cada plataforma.
>
> ```c
> #include <inttypes.h>
> printf("ticks = %" PRIu32 "\n", contador);   // en el M3, PRIu32 expande a "lu"
> ```

---

### Headers útiles

- `<limits.h>` → define los límites de tipos enteros (`INT_MAX`, etc.).
- `<float.h>` → define propiedades de tipos flotantes (`FLT_MAX`, etc.).
- `<stdint.h>` e `<inttypes.h>` → los tipos de ancho fijo que acabamos de ver y sus macros de `printf`.

> Estos archivos existen de verdad y los podés abrir: los provee el compilador, y la ruta te la dice
> él mismo con `arm-none-eabi-gcc -print-file-name=include`. Dónde vive cada header y por qué unos los
> da GCC y otros newlib, en [07 - El preprocesador](./07-preprocesador.md#y-dónde-están-físicamente).

## Constantes en C (también llamadas literales)

Una **constante** o **literal** es un valor escrito directamente en el código: `123`, `0x7B`, `3.14f`,
`'x'`, `"hola"`. No es "solo un número": cada literal **tiene un tipo**, y ese tipo es el que manda en
las cuentas donde aparece. La mitad de los bugs de este capítulo salen de ahí.

### Constantes enteras: las bases (decimal, octal, hexadecimal, binario)

En C, los **números enteros** pueden escribirse en **diferentes bases** usando prefijos:


| Forma       | Ejemplo      | Base | Prefijo   | Notas                                                     |
| ----------- | ------------ | ---- | --------- | --------------------------------------------------------- |
| Decimal     | `123`        | 10   | (ninguno) | La forma más común                                        |
| Octal       | `0123`       | 8    | `0`       | Solo dígitos `0` a `7`                                    |
| Hexadecimal | `0x7B`       | 16   | `0x`/`0X` | Dígitos `0-9`, letras `a-f`                               |
| Binario     | `0b01111011` | 2    | `0b`/`0B` | Extensión de GCC/Clang desde 2008; **estándar desde C23** |


Los cuatro literales de la tabla valen exactamente lo mismo: 123.

**Cuidado con estos detalles:**

- El prefijo `0` en un número **lo convierte en octal**: `012` es 10, no 12. Es una fuente clásica de bugs cuando alineás números en columnas con ceros adelante (`{ 007, 008 }` ni compila, porque `8` no es un dígito octal).
- `0` solo es, técnicamente, un literal octal. No cambia nada, pero explica por qué no existe un prefijo decimal.
- Los literales **binarios** (`0b...`) son estándar desde **C23**; con `-std=gnu17` (el default de `arm-none-eabi-gcc`) funcionan como extensión de GCC. Si compilás con `-std=c17 -pedantic` te va a tirar warning.
- C23 agrega además el **separador de dígitos** `'`: `0b1111'0000`, `1'000'000`.

---

### Sufijos y tipo de un literal entero

Un literal entero **ya tiene un tipo antes de que vos digas nada**: por defecto, un número como `1234` es `int`. Con un sufijo lo cambiás:

- `U` o `u`: unsigned
- `L` o `l`: long
- `LL` o `ll`: long long (C99)
- `UL`, `ULL`, `LU`, etc.: combinaciones válidas

**Ejemplo:**

```c
0xFF       // int
0xFFU      // unsigned int
0123L      // long (octal, vale 83)
123UL      // unsigned long
1ULL << 40 // unsigned long long: sin el sufijo esto se desborda
```

**¿Y si el número no entra en un `int`?** El compilador va probando tipos cada vez más grandes hasta que entre, y la lista que recorre **depende de la base en la que lo escribiste**:

- Literal **decimal** sin sufijo: `int` → `long` → `long long`.
- Literal **octal o hexadecimal** sin sufijo: se consideran además los tipos sin signo, `int` → `unsigned int` → `long` → `unsigned long` → `long long` → `unsigned long long`.

Esa asimetría muerde. En el LPC1769 (donde `int` es de 32 bits), `0xFFFFFFFF` termina siendo `unsigned int`, mientras que `4294967295`, **el mismo valor escrito en decimal**, termina siendo `long long`. Un motivo más para escribir las máscaras en hexa y con sufijo `U`.

> Preferí `l` en mayúscula (`123L`): la `l` minúscula se confunde con el dígito `1` en muchas tipografías.

**En embebido el sufijo `U` no es un detalle cosmético.** Cuando armás máscaras de bits, `1 << 31` es un desplazamiento sobre un `int` con signo, y desbordar un `signed` es **comportamiento indefinido**:

```c
uint32_t mask_mal  = 1 << 31;    // UB: 1 es int, el bit 31 es el bit de signo
uint32_t mask_bien = 1u << 31;   // correcto: unsigned, wraparound definido
```

> *Detalle de versión:* eso es UB en **C17 y anteriores**, que son los que vas a usar acá (`arm-none-eabi-gcc` compila con `-std=gnu17` por defecto). **C23** definió el corrimiento a la izquierda en términos del patrón de bits y el caso dejó de ser UB. No cambia la recomendación: escribí el sufijo igual, porque el código embebido se compila con toolchains anteriores a C23.

Por eso en código de registros vas a ver siempre `(1u << n)` o `(1UL << n)`.

Y acá el compilador **no te ayuda**: `1 << 31` compila sin decirte nada. Hace falta pedirlo con la flag especifica para que detecte:

```console
$ arm-none-eabi-gcc -Wall -Wextra -Wshift-overflow=2 -c mascaras.c
warning: result of '1 << 31' requires 33 bits to represent, but 'int' only has 32 bits
```

Es una razón más para escribir el sufijo siempre, en vez de confiar en que algo te avise.

---

### Constantes de punto flotante (floating point literals)

- Incluyen punto decimal o exponente:
  - `123.4`, `1e-2`
- Tipos por sufijo:
  - Sin sufijo: `double`
  - `f` o `F`: `float`
  - `l` o `L`: `long double`

### Constantes de carácter (character literals)

- Se escriben entre comillas simples: `'x'`
- En C, una constante de carácter tiene tipo **`int`**, no `char`. Representa el valor numérico del carácter (por ejemplo, `'0'` es 48 en ASCII). Por eso `sizeof('x')` da **4** en el M3, aunque `sizeof(char)` dé 1. (En C++ sí es `char` y daría 1.)
- Participan en operaciones como cualquier `int`. El truco clásico: `digito = c - '0';` convierte el carácter `'7'` en el número 7.
- No confundir `'x'` (carácter) con `"x"` (cadena), pues el último es un arreglo de caracteres (se agrega `'\0'` al final). `sizeof("x")` es **2**.
- `'ab'` (más de un carácter) compila en GCC pero su valor es *definido por la implementación*. No lo uses.

### Constantes de cadena (string literals)

- Secuencia entre comillas dobles: `"Hola mundo"`
- `"Hola," "mundo"` se concatena automáticamente como `"Hola, mundo"`
- Siempre terminan con `'\0'` (carácter nulo). Por eso `"Hola"` ocupa **5** bytes, no 4.
- Técnicamente, son **arreglos de caracteres** de tipo `char[N]` (los arreglos se ven en [02 - Arreglos](./02-arreglos-conversiones-y-promociones.md#arreglos-arrays)).
- La función `strlen()` (de `<string.h>`) devuelve la longitud (sin contar el `'\0'`).

Así se implementaría a mano. Le ponemos otro nombre porque redefinir una función de la librería estándar es comportamiento indefinido:

```c
#include <stddef.h>

size_t mi_strlen(const char s[]) {
    size_t i = 0;
    while (s[i] != '\0')
        i++;
    return i;
}
```

> Fijate en los tipos: la real devuelve `size_t` (sin signo) y toma `const char *`, porque no modifica la cadena. El `const` permite pasarle también literales.

> [!WARNING]
> **Un literal de cadena no se puede modificar.** `char *p = "Hola"; p[0] = 'h';` es **comportamiento indefinido**: los literales viven en `.rodata`, que en el LPC1769 está en **Flash**, así que la escritura simplemente no tiene efecto (o falla). Si necesitás modificarla, copiala a un arreglo: `char buf[] = "Hola";`, que sí es un arreglo propio en RAM. Compilá con `-Wwrite-strings` para que el compilador te avise.

---

### Constantes de enumeración (`enum`)

- Lista de identificadores con valores enteros constantes:

```c
enum boolean { NO, YES };      // NO = 0, YES = 1
enum months { ENE = 1, FEB, MAR };  // FEB = 2, MAR = 3
enum escapes { BELL = '\a', BACKSPACE = '\b', TAB = '\t' , NEWLINE = '\n', RETURN = '\r'};
```

- Si no se da valor explícito, continúan desde el anterior (el primero arranca en 0).
- Se usan como alternativa a `#define`.
- Los `enum` hacen que el compilador pueda verificar su uso, lo cual es más seguro. Además el depurador te muestra el **nombre** (`YES`) en vez del número, algo que con `#define` perdés.
- Los valores pueden repetirse: `enum { A = 1, B = 1 };` es válido.
- **Cada constante de enumeración tiene tipo `int`** (en C17 y anteriores). O sea que `NO` y `YES` son `int`, y `sizeof(YES)` da 4 en el M3.
- El *tipo enumerado* en sí (`enum boolean`) es otra cosa: el compilador elige por vos un tipo entero compatible, y esa elección es **definida por la implementación**.

> [!IMPORTANT]
> **En el LPC1769 el `sizeof` de un `enum` no es 4.** El ABI de ARM manda usar el tipo más chico que alcance, y `arm-none-eabi-gcc` viene con `-fshort-enums` **activado por defecto**. Comprobado con el toolchain del repo:
>
> ```c
> enum big { X = 300 };   // sizeof(enum big) == 2   (¡dos bytes!)
> enum sm  { Y = 1   };   // sizeof(enum sm)  == 1   (¡un byte!)
> ```
>
> El mismo código en tu PC (x86) da **4 en los dos casos**. 
>
> **Nunca asumas el tamaño de un `enum`** en un `struct` que mapea un registro, una trama de protocolo o algo que se guarde en memoria. Si necesitás un ancho exacto, poné `uint8_t`/`uint32_t` en el campo y usá las constantes del `enum` aparte.

> En **C23** el tipo subyacente se puede fijar explícitamente y el problema desaparece: `enum estado : uint8_t { OFF, ON };`

### Secuencias de escape

Sirven para escribir caracteres especiales dentro de `'` o `"`, o sea dentro de los dos tipos de constante que acabamos de ver.

| Escape | Significado                                                               |
| ------ | ------------------------------------------------------------------------- |
| `\n`   | nueva línea                                                               |
| `\r`   | retorno de carro                                                          |
| `\t`   | tabulador horizontal                                                      |
| `\0`   | carácter nulo: el que termina toda cadena (es el caso `\ooo` con valor 0) |
| `\ooo` | valor octal (1 a 3 dígitos octales)                                       |
| `\xhh` | valor hexadecimal (1 o más dígitos hexa)                                  |

**La que más vas a usar es `\r\n`, y siempre las dos juntas.** Una terminal serie espera *retorno de carro* **y** *avance de línea* para empezar un renglón nuevo. Si mandás solo `\n`, el texto sale en escalera: cada línea arranca donde terminó la anterior. Es el primer tropiezo de cualquiera que conecta el micro a la PC, y se resuelve en [16 - Redirigir `printf` a la UART](./16-redirigir-printf-a-uart.md).

Dos trampas con estas secuencias:

- **`\ooo` toma como máximo 3 dígitos, pero `\xhh` toma *todos* los dígitos hexa que encuentre.** Así que `"\x41B"` no es `"AB"`: el compilador intenta leer `0x41B`, que no cabe en un `char`, y avisa `warning: hex escape sequence out of range`. Si necesitás un hexa seguido de una letra hexa, cortá la cadena en dos aprovechando la concatenación automática: `"\x41" "B"`.
- `'\0'` (el carácter nulo, valor 0) no es lo mismo que `'0'` (el dígito cero, valor 48), y ninguno de los dos es `NULL` (que es un puntero). Son tres cosas distintas que se escriben parecido.

---

## Resumen de reglas para no equivocarse


| Regla                                                                     | Por qué                                                                 |
| ------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| Declará enteros con `<stdint.h>` (`uint32_t`, `int8_t`...)                | El ancho es explícito y portable                                        |
| `char` solo para texto; `uint8_t` para bytes                              | En ARM `char` es `unsigned`.                                            |
| `volatile` en toda variable compartida con una ISR o un periférico        | Si no, el compilador cachea la lectura y con `-O2` te rompe el programa |
| `u`/`U` en las máscaras de bits: `(1u << 31)`                             | `1 << 31` desborda un `int` con signo → UB                              |
| `#define` o `enum` para tamaños de arreglo, `case` y `#if`, nunca `const` | En C un `const` es una variable, no una constante de compilación        |
| `static const` para tablas grandes                                        | Van a Flash (`.rodata`), no gastan RAM                                  |
| Nunca asumas el `sizeof` de un `enum`                                     | En ARM vale 1 o 2 bytes (`-fshort-enums`)                               |
| Evitá `float`/`double` en código crítico                                  | El Cortex-M3 del LPC1769 no tiene FPU: todo por software                |
| Compilá con `-Wall -Wextra`                                               | Varios bugs de este capítulo el compilador los ve antes que vos         |


---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.2.4 (duración de almacenamiento), 6.2.5 (tipos), 6.4.4 (constantes), 6.7.1 (especificadores de almacenamiento), 6.7.3 (calificadores).
- [cppreference: C language](https://en.cppreference.com/w/c/language). La referencia práctica más clara, con los cambios por versión del estándar.

**GCC y el toolchain**

- [GCC: Implementation-defined behavior](https://gcc.gnu.org/onlinedocs/gcc/C-Implementation.html). Qué elige GCC donde el estándar deja libertad (signo de `char`, tipo de los `enum`).
- Los tamaños y el signo de `char` de este capítulo se pueden verificar con el toolchain del repo:
  ```console
  $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -dM -E - < /dev/null | grep -E 'SIZEOF|CHAR_UNSIGNED'
  ```

**ARM y el LPC1769**

- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf). Está en el repo. Capítulo 9 (GPIO) para las direcciones de `FIOxDIR`/`FIOxSET`/`FIOxPIN` usadas en los ejemplos.
- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). Define que `char` es `unsigned`, que `long double` es igual a `double` y el tamaño de los `enum` en ARM de 32 bits.

**Sobre los temas puntuales**

- [What are storage class specifiers in C?](https://how.dev/answers/what-are-storage-class-specifiers-in-c)

---

**Módulo:** [Lenguaje C](./README.md) ·
**Siguiente:** [02 - Arreglos, conversiones y promociones](./02-arreglos-conversiones-y-promociones.md)