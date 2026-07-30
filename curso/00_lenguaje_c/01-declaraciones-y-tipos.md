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

### 1. **Tipos de datos básicos**

| Tipo           | Descripción                              |
|----------------|------------------------------------------|
| `void`         | Sin tipo, se usa para indicar ausencia de tipo |
| `char`         | Un byte, representa un carácter          |
| `int`          | Entero, tamaño natural de la máquina     |
| `long long`    | Entero de al menos 64 bits (desde C99)   |
| `float`        | Punto flotante, precisión simple         |
| `double`       | Punto flotante, precisión doble          |
| `long double`  | Punto flotante, precisión **al menos** igual a `double` |
| `_Bool`        | Booleano, `0` o `1` (desde C99; con `<stdbool.h>` se escribe `bool`) |

> *Ojo con `long double`:* el estándar solo pide que **no sea más chico** que `double`. En x86 con GCC son 80 bits (10 bytes, alineados a 16); en `arm-none-eabi-gcc` para el LPC1769 `long double` es **exactamente igual que `double`** (64 bits). "Precisión extendida" no es algo que tengas garantizado.

#### Ejemplo:

```c
char letra;
float temperatura;

void copiar(int from, int to) {
    to = from;   // ¡no hace nada útil!
}
```

> El ejemplo de `copiar()` es a propósito un **contraejemplo** clásico (está en el K&R): en C los argumentos se pasan **por valor**, así que `from` y `to` son copias locales. Modificar `to` no cambia nada afuera de la función. Para eso hacen falta punteros, que se ven en [el capítulo de punteros](05-punteros.md).

---

### 2. **Especificador de almacenamiento** 
  

La **clase de almacenamiento** de una variable define:

* Su **alcance (scope)**, su **tiempo de vida (lifetime)** y **ubicación de almacenamiento**
 
Pueden ser:

* `auto`: se destruye cuando la función termina.
* `static`: se inicializa solo una vez y luego conserva su valor entre llamadas.
* `extern`: se define en otro archivo y se puede usar en el archivo actual.
* `register`: se intenta usar un registro de la CPU, pero no es garantizado.

--- 
Si no se declara ningún especificador:
* Las variables **dentro** de funciones son `auto` por defecto: viven en el stack y mueren al salir.
* Las variables **fuera** de funciones ya tienen *tiempo de vida* estático (duran todo el programa) sin escribir nada, pero su **enlace (linkage) es externo**: otros archivos las pueden ver con `extern`.

> [!IMPORTANT]
> Una variable global **no** es `static` por defecto. `static` a nivel de archivo no cambia el tiempo de vida (ya era todo el programa), lo que hace es **ocultarla**: pasa a tener enlace *interno* y ningún otro archivo puede referenciarla. Son dos cosas distintas que se llaman igual:
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
 
 

| Especificador | Ubicación de almacenamiento             | Tiempo de vida                  | Alcance (scope)                                                         | Valor inicial              | Comentarios clave                               |
| ------------- | --------------------------------------- | ------------------------------- | ---------------------------------------------------------------------- | -------------------------- | ----------------------------------------------- |
| `auto`        | Stack (RAM)                | Mientras la función esté activa | Local a la función                                                     | Indefinido (¡basura!)      | Valor no se conserva entre llamadas             |
| `static`      | RAM (`.data`/`.bss`), o Flash si además es `const`   | Todo el programa                | Local (si está dentro de función) o interna al archivo (si está fuera) | Cero (si no se inicializa) | Mantiene su valor entre llamadas                |
| `register`    | Registro de CPU (si está disponible)    | Mientras la función esté activa | Local a la función                                                     | Indefinido                 | Más rápido (teóricamente); no se puede usar `&` |
| `extern`      | Donde la haya definido el otro archivo  | Todo el programa                | Global (visible en otros archivos)                                     | No aplica: `extern` no crea la variable | Se usa para compartir variables entre archivos  |

Dos aclaraciones sobre la tabla:

* **`extern` no reserva memoria.** Es solo una *declaración*: le avisa al compilador "esta variable existe en algún lado, confiá y que el linker la encuentre". La *definición* (la que sí reserva memoria y se inicializa en cero) está en exactamente uno de los `.c`. Lo típico es poner el `extern` en el `.h` y la definición en el `.c`.
* **`static const` no gasta RAM.** En el LPC1769, una tabla `static const uint16_t tabla[256]` la pone el linker en `.rodata`, que vive en **Flash** (512 KB) y no en los 64 KB de RAM. Es la forma de guardar tablas de lookup grandes sin comerte la RAM.
* **`register` hoy no sirve de mucho.** GCC con `-O2` ya asigna registros mejor que vos; lo único que `register` sigue garantizando es que **no podés tomarle la dirección** con `&`.

---

 

> [!NOTE]
> [What are storage class specifiers in C?](https://how.dev/answers/what-are-storage-class-specifiers-in-c)

---

### 3. **Calificador de tipo**

Afectan cómo el compilador **trata el contenido de la variable**.

| Calificador | ¿Qué hace?                                               | Ejemplo embebido             |
| ----------- | -------------------------------------------------------- | ---------------------------- |
| `const`     | El valor no se puede modificar **a través de ese nombre** | `const float PI = 3.14f;`     |
| `volatile`  | Puede cambiar fuera del programa (por hardware)          | `volatile uint32_t *port;` |
| `restrict`  | Promesa de **no aliasing**: a ese dato se accede *solo* por este puntero (C99) | `void f(int * restrict a);`  |

> En sistemas embebidos, `volatile` es **crítico** para registros de periféricos.

**Sobre `restrict`:** no es "la única forma de acceder es un puntero". Es una **promesa que le hacés al compilador**: durante la vida de ese puntero, al objeto apuntado se accede únicamente a través de él (o de punteros derivados de él). Con esa garantía el compilador puede mantener valores en registros en vez de releer memoria. Si le mentís (si dos punteros `restrict` apuntan a lo mismo y escribís por ambos), es **comportamiento indefinido**. Por eso `memcpy()` declara sus dos punteros `restrict` (las regiones no se pueden solapar) y `memmove()` **no** (sí se pueden solapar).

> [!WARNING]
> **`const` en C no es una constante de compilación.** A diferencia de C++, en C un `const int N = 8;` **no** sirve para tamaños de arreglo a nivel de archivo, ni para `case`, ni para `#if`. Para eso usás `#define` o `enum`:
> ```c
> const int  N1 = 8;
> int buf1[N1];        // ERROR a nivel de archivo (dentro de una función sería un VLA)
>
> #define    N2   8
> int buf2[N2];        // OK
>
> enum { N3 = 8 };
> int buf3[N3];        // OK, y además con chequeo de tipo
> ```
> Además, `const` significa "prometo no escribirlo por este nombre", **no** "está en memoria de solo lectura". Si otro alias sin `const` lo modifica, es UB.

* **Cual es la diferencia entre especificar `volatile` y no?**
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

> `volatile` resuelve la **visibilidad** de la variable, no la **atomicidad**. Si la ISR y el `main` hacen lectura-modificación-escritura sobre la misma variable (por ejemplo `contador++`), `volatile` no te salva: sigue habiendo una condición de carrera. Eso se ve en [el capítulo de tipos de ancho fijo y `volatile`](08-tipos-de-ancho-fijo-y-volatile.md).

---


### 4. **Modificadores del tipo**

Modifican el tipo base en cuanto a **tamaño o signo**:

| Modificador                | Qué afecta                     |
| -------------------------- | ------------------------------ |
| `short`, `long`, `long long`| Cambia el tamaño del entero    |
| `signed`, `unsigned`       | Permite o no números negativos |

> Combinables: `unsigned long int`, `short int`, etc. El orden no importa: `unsigned long int` y `int long unsigned` son el mismo tipo.

#### Ejemplo:

```c
unsigned char      codigo;     // 0 a 255
signed short int   temp;       // -32 768 a 32 767
unsigned long long acumulador; // 64 bits sin signo en el M3
long double        resultado;  // en el LPC1769, lo mismo que un double
```

> Todos los tipos enteros son `signed` por defecto **excepto `char`**, cuyo signo lo elige el compilador (ver más abajo). O sea: `short` ≡ `signed short`, pero `char` **no** es necesariamente `signed char`.



---

### 5. **Nombre de variables**

- Compuestos por letras, dígitos y `_`.
- El primer carácter **debe ser una letra** (o `_`, aunque no se recomienda).
- **Mayúsculas y minúsculas son distintas** (`a` ≠ `A`).
- Convención:
  - minúsculas para variables
  - MAYÚSCULAS para constantes simbólicas
- Evitar nombres que empiecen con `_`: **están reservados para la implementación** (el compilador y la librería estándar). En concreto, `_` seguido de mayúscula o de otro `_` (`_Bool`, `__attribute__`) está reservado *siempre*, y `_` seguido de minúscula está reservado a nivel de archivo. Si los usás, podés chocar con nombres internos de la libc.
- Cuántos caracteres se garantizan como **significativos** (o sea, hasta dónde el compilador se compromete a distinguir dos nombres parecidos):

  | Estándar | Identificadores internos | Identificadores externos |
  |----------|--------------------------|--------------------------|
  | C89/C90  | 31                       | 6 (¡seis!)               |
  | C99/C11/C17 | 63                    | 31                       |

  *Internos* = locales, `static`, macros. *Externos* = los que ve el linker (funciones y globales no `static`). En la práctica GCC no tiene ningún límite, pero si te importa la portabilidad no hagas nombres exportados que solo se diferencien después del carácter 31.
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

Un puntero constante a un registro de un puerto del LPC1769. No puede cambiar de dirección, pero sí puede cambiar su valor por hardware.

Usamos `FIO0PIN` (`0x2009C014`), el registro que refleja el estado de los pines del puerto 0. La dirección está tomada de la tabla 103 del [UM10360](../../UM10360.pdf), capítulo 9 (GPIO).

```c
#include <stdint.h>

static const volatile uint32_t * const FIO0PIN =
        (const volatile uint32_t *) 0x2009C014;

void leer_puerto(void) {
    uint32_t estado = *FIO0PIN;   // Leer el estado de los pines de P0
    (void) estado;
}
```
 Desglose de la declaración anterior:

| Parte          | Función                                                            |
| -------------- | ------------------------------------------------------------------ |
| `static`       | Solo visible en este archivo                                       |
| `const` (el primero) | Lo **apuntado** no se puede escribir desde el código (registro de solo lectura) |
| `volatile`     | Lo apuntado puede cambiar por hardware: el compilador no puede cachear la lectura |
| `uint32_t`     | Tipo base: los registros del LPC1769 son de 32 bits                |
| `* const`      | El **puntero** es constante: no puede apuntar a otra dirección     |
| `FIO0PIN`      | Nombre de la variable                                              |
| `= ...`        | Se inicializa apuntando a una dirección fija de memoria            |

Tres detalles que importan:

1. **El `const` de la izquierda y el `const` de la derecha son distintos.** El de antes del `*` califica **el dato apuntado**; el de después del `*` califica **el puntero**. Se lee de derecha a izquierda: *"`FIO0PIN` es un puntero `const` a un `uint32_t` `const volatile`"*.
2. **Para un registro que sí escribís** (como `FIO0SET`, `0x2009C018`) hay que sacar el `const` del dato, dejando solo `volatile`:
   ```c
   static volatile uint32_t * const FIO0SET = (volatile uint32_t *) 0x2009C018;
   *FIO0SET = (1u << 22);   // prender P0.22
   ```
3. **La inicialización tiene que ir a nivel de archivo, pero la lectura no.** `uint32_t estado = *FIO0PIN;` **fuera** de una función no compila: los inicializadores de variables con duración estática deben ser expresiones constantes, y desreferenciar un puntero no lo es. Por eso la lectura va dentro de `leer_puerto()`.

> En la práctica no escribís estos punteros a mano: los define el header del fabricante (`LPC17xx.h`), que agrupa los registros en un `struct` (`LPC_GPIO0->FIOSET = ...`). La técnica se ve en [el capítulo de estructuras](07-estructuras-uniones-enums.md).
 

## Tamaños de los tipos

- Los tamaños exactos de los tipos **dependen del compilador y hardware**.
- Garantías mínimas del lenguaje:
  - `char`: al menos 8 bits
  - `short` y `int`: al menos 16 bits
  - `long`: al menos 32 bits
  - `long long`: al menos 64 bits (C99)
  - `char <= short <= int <= long <= long long`

Relaciones entre los tipos:
```c 
sizeof(char)      = 1              // siempre 1 por definición
sizeof(short)    <= sizeof(int)
sizeof(int)      <= sizeof(long)
sizeof(long)     <= sizeof(long long)
sizeof(float)    <= sizeof(double)
sizeof(double)   <= sizeof(long double)
```

> **`sizeof(char) == 1` siempre, pero eso no significa "8 bits".** `sizeof` mide en unidades de `char`, y `char` es *por definición* la unidad de 1. Cuántos bits tiene realmente lo dice `CHAR_BIT` en `<limits.h>`, y el estándar solo exige `CHAR_BIT >= 8`. En el LPC1769 (y en cualquier máquina que vas a usar en la práctica) `CHAR_BIT == 8`, así que "byte" y "8 bits" coinciden. Existen DSPs donde `CHAR_BIT` es 16 y ahí `sizeof(int)` puede dar 1.

### Tamaño REAL de los tipos en el Cortex-M3 (LPC1769)

El estándar solo da garantías mínimas, pero a vos te interesa qué pasa **en tu micro**. Con `arm-none-eabi-gcc` sobre el LPC1769 (ABI estándar de ARM, AAPCS) los tamaños son:

| Tipo          | Tamaño en el M3 | Rango                                   |
|---------------|-----------------|------------------------------------------|
| `char`        | 8 bits          | 0 a 255 (es `unsigned`, ver abajo)      |
| `short`       | 16 bits         | -32 768 a 32 767                         |
| `int`         | **32 bits**     | -2 147 483 648 a 2 147 483 647           |
| `long`        | 32 bits         | igual que `int`                          |
| `long long`   | 64 bits         | ±9,2 · 10^18                             |
| `float`       | 32 bits         | precisión simple (IEEE-754)              |
| `double`      | 64 bits         | precisión doble (IEEE-754)               |
| `long double` | 64 bits         | **igual que `double`** en el ABI de ARM de 32 bits |
| `_Bool`       | 8 bits          | `0` o `1`                                |
| puntero (`*`) | 32 bits         | el M3 tiene un espacio de direcciones de 32 bits |

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

1. En el Cortex-M3 `int` es de **32 bits**, igual que `long` y que un puntero. El M3 es una máquina de 32 bits, así que operar con `int` es lo más natural y rápido para la ALU. Operar con `uint8_t` o `uint16_t` a veces obliga al compilador a **enmascarar** después de cada cuenta para que el resultado "entre" en 8 o 16 bits.
2. El M3 **no tiene unidad de punto flotante** (el LPC1769 es Cortex-M3, no M4F). Cada `float`/`double` se calcula por **software**, lo cual es lento. Evitá flotantes en código crítico; usá aritmética entera o de punto fijo cuando puedas.

> **`char` puede ser con o sin signo.** El estándar deja librado al compilador si `char` "pelado" es `signed` o `unsigned`. En ARM el ABI define `char` como **unsigned**, y `arm-none-eabi-gcc` lo cumple: define `__CHAR_UNSIGNED__` y `char` va de 0 a 255. En x86 con GCC, en cambio, `char` es **signed** (-128 a 127).
>
> Esto importa y muerde de verdad:
> ```c
> char c = 0x80;
> if (c < 0) { /* en tu PC: SÍ entra. En el LPC1769: NO entra. */ }
> ```
> El mismo código fuente, dos comportamientos. Verificalo:
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -dM -E - < /dev/null | grep CHAR_UNSIGNED
> #define __CHAR_UNSIGNED__ 1
> ```
> (Se puede forzar con `-fsigned-char` / `-funsigned-char`, pero no lo hagas: escribí código que no dependa de eso.)
>
> **Moraleja:** usá `char` **solo para texto**. Para un byte de datos usá `uint8_t`, y si necesitás un byte con signo pedí `int8_t` o `signed char` explícitamente. Nunca dejes que el signo de `char` te importe.

### Conclusión práctica: usá `stdint.h`

Por todo esto, en embebido **declarás los enteros con los tipos de `<stdint.h>`** (`uint8_t`, `int32_t`, etc.) en vez de `int`/`short`/`long` pelados. Decís exactamente cuántos bits querés y el código se comporta igual en cualquier compilador. Lo vemos en detalle más abajo y en [el capítulo de tipos de ancho fijo y `volatile`](08-tipos-de-ancho-fijo-y-volatile.md).

---

###  Headers útiles

- `<limits.h>` → define los límites de tipos enteros (`INT_MAX`, etc.).
- `<float.h>` → define propiedades de tipos flotantes (`FLT_MAX`, etc.).


### Solución: tipos con tamaño fijo (`stdint.h`)

Para evitar ambigüedades sobre cuántos bits tiene un `int`, `short`, etc., C ofrece una forma **explícita y portable** de declarar tipos enteros con tamaño exacto a través del header:

```c
#include <stdint.h>
```

Esto define tipos estándar como:

| Tipo         | Tamaño exacto | Descripción   |
|--------------|---------------|---------------|
| `int8_t`     | 8 bits        | Con signo     |
| `uint8_t`    | 8 bits        | Sin signo     |
| `int16_t`    | 16 bits       | Con signo     |
| `uint16_t`   | 16 bits       | Sin signo     |
| `int32_t`    | 32 bits       | Con signo     |
| `uint32_t`   | 32 bits       | Sin signo     |
| `int64_t`    | 64 bits       | Con signo     |
| `uint64_t`   | 64 bits       | Sin signo     |

Ejemplo:

```c
#include <stdint.h>

uint8_t edad = 25;
int16_t temperatura = -120;
uint32_t contador = 100000;
```
Esto trae varias ventajas:
- Claridad total sobre el tamaño de cada tipo
- Portable entre distintas arquitecturas
- Más seguro

> Es fundamental usarlos en sistemas embebidos.

**Detalles que conviene saber:**

* Los tipos de tamaño **exacto** (`intN_t`) son técnicamente **opcionales** en el estándar: solo existen si la máquina tiene un tipo de exactamente ese ancho y sin bits de relleno. En el Cortex-M3 están los cuatro (8/16/32/64), así que en este curso podés usarlos sin miedo. Los que están **garantizados siempre** son los de ancho *mínimo*, `int_leastN_t`.
* `<stdint.h>` también trae `int_fastN_t` ("el más rápido de al menos N bits"), `intptr_t` (un entero donde cabe un puntero) e `intmax_t`.
* Para los **límites** tenés macros propias: `UINT8_MAX`, `INT32_MIN`, `INT32_MAX`, etc.
* Para **escribir literales** de un ancho dado están las macros `UINT32_C(x)` / `INT64_C(x)`: `UINT32_C(0xFFFFFFFF)`.
* Para **imprimir** estos tipos con `printf` no sirve `%d` a ciegas: `<inttypes.h>` define las macros correctas, `printf("%" PRIu32 "\n", contador);`. En el M3 `%lu` suele funcionar para `uint32_t` porque `long` también es de 32 bits, pero es suerte, no portabilidad.
* `size_t` (de `<stddef.h>`) es el tipo para **tamaños y cantidades**; es sin signo y en el M3 son 32 bits. Usalo para índices de arreglos y resultados de `sizeof`, no `int`.

## Formas de escribir enteros en C (octales, hexadecimales, binarios)

En C, los **números enteros** pueden escribirse en **diferentes bases** usando prefijos:

| Forma         | Ejemplo       | Base | Prefijo   | Notas                           |
|---------------|---------------|------|-----------|---------------------------------|
| Decimal       | `123`         | 10   | (ninguno) | La forma más común              |
| Octal         | `0123`        | 8    | `0`       | Solo dígitos `0` a `7`          |
| Hexadecimal   | `0x7B`        | 16   | `0x`/`0X` | Dígitos `0-9`, letras `a-f`     |
| Binario       | `0b01111011`  | 2    | `0b`/`0B` | Extensión de GCC/Clang desde 2008; **estándar desde C23** |

Los cuatro literales de la tabla valen exactamente lo mismo: 123.

---

### Sufijos para modificar el tipo

Podés agregar sufijos para cambiar el tipo del literal:

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

> Preferí `l` en mayúscula (`123L`): la `l` minúscula se confunde con el dígito `1` en muchas tipografías.

**En embebido el sufijo `U` no es un detalle cosmético.** Cuando armás máscaras de bits, `1 << 31` es un desplazamiento sobre un `int` con signo, y desbordar un `signed` es **comportamiento indefinido**:

```c
uint32_t mask_mal  = 1 << 31;    // UB: 1 es int, el bit 31 es el bit de signo
uint32_t mask_bien = 1u << 31;   // correcto: unsigned, wraparound definido
```

Por eso en código de registros vas a ver siempre `(1u << n)` o `(1UL << n)`.

---

### Notas importantes

- El prefijo `0` en un número **lo convierte en octal**: `012` es 10, no 12. Es una fuente clásica de bugs cuando alineás números en columnas con ceros adelante (`{ 007, 008 }` ni compila, porque `8` no es un dígito octal).
- `0` solo es, técnicamente, un literal octal. No cambia nada, pero explica por qué no existe un prefijo decimal.
- Los literales **binarios** (`0b...`) son estándar desde **C23**; con `-std=gnu17` (el default de `arm-none-eabi-gcc`) funcionan como extensión de GCC. Si compilás con `-std=c17 -pedantic` te va a tirar warning.
- C23 agrega además el **separador de dígitos** `'`: `0b1111'0000`, `1'000'000`.

# Constantes en C (también llamadas literales)

### Constantes enteras (integer literals)

- Por defecto, un número como `1234` es `int`.
- Se puede agregar un sufijo:
  - `L` o `l`: long → `123456789L`
  - `UL` o `ul`: unsigned long
- Si el número **no cabe** en `int`, el compilador va probando tipos cada vez más grandes hasta que entre. Para un literal **decimal sin sufijo** la lista es `int` → `long` → `long long`; para uno **octal o hexadecimal** también se consideran los tipos sin signo: `int` → `unsigned int` → `long` → `unsigned long` → `long long` → `unsigned long long`.
- Por eso, en el LPC1769 (donde `int` es de 32 bits), `0xFFFFFFFF` termina siendo `unsigned int`, mientras que `4294967295` (el mismo valor en decimal) termina siendo `long long`. Un motivo más para escribir las máscaras en hexa y con sufijo `U`.

### Constantes octales y hexadecimales (octal and hexadecimal literals)

- Octal: comienza con `0` → `037` (equivale a 31 decimal).
- Hexadecimal: comienza con `0x` o `0X` → `0x1F`
- Se puede agregar sufijo `L` o `U` para indicar tipo (`0xFFUL` → unsigned long).


### Constantes de punto flotante (floating point literals)

- Incluyen punto decimal o exponente:
  - `123.4`, `1e-2`
- Tipos por sufijo:
  - Sin sufijo: `double`
  - `f` o `F`: `float`
  - `l` o `L`: `long double`



###  Constantes de carácter (character literals)

- Se escriben entre comillas simples: `'x'`
- En C, una constante de carácter tiene tipo **`int`**, no `char`. Representa el valor numérico del carácter (por ejemplo, `'0'` es 48 en ASCII). Por eso `sizeof('x')` da **4** en el M3, aunque `sizeof(char)` dé 1. (En C++ sí es `char` y daría 1.)
- Participan en operaciones como cualquier `int`. El truco clásico: `digito = c - '0';` convierte el carácter `'7'` en el número 7.
- No confundir `'x'` (carácter) con `"x"` (cadena), pues el último es un arreglo de caracteres (se agrega `'\0'` al final). `sizeof("x")` es **2**.
- `'ab'` (más de un carácter) compila en GCC pero su valor es *definido por la implementación*. No lo uses.


### Constantes de cadena (string literals)

* Secuencia entre comillas dobles: `"Hola mundo"`
* `"Hola," "mundo"` se concatena automáticamente como `"Hola, mundo"`
* Siempre terminan con `'\0'` (carácter nulo). Por eso `"Hola"` ocupa **5** bytes, no 4.
* Técnicamente, son **arreglos de caracteres** de tipo `char[N]`.
* La función `strlen()` (de `<string.h>`) devuelve la longitud (sin contar el `'\0'`).

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

* Lista de identificadores con valores enteros constantes:

```c
enum boolean { NO, YES };      // NO = 0, YES = 1
enum months { ENE = 1, FEB, MAR };  // FEB = 2, MAR = 3
enum escapes { BELL = '\a', BACKSPACE = '\b', TAB = '\t' , NEWLINE = '\n', RETURN = '\r'};
```

* Si no se da valor explícito, continúan desde el anterior (el primero arranca en 0).
* Se usan como alternativa a `#define`.
* Los `enum` hacen que el compilador pueda verificar su uso, lo cual es más seguro. Además el depurador te muestra el **nombre** (`YES`) en vez del número, algo que con `#define` perdés.
* Los valores pueden repetirse: `enum { A = 1, B = 1 };` es válido.
* **Cada constante de enumeración tiene tipo `int`** (en C17 y anteriores). O sea que `NO` y `YES` son `int`, y `sizeof(YES)` da 4 en el M3.
* El *tipo enumerado* en sí (`enum boolean`) es otra cosa: el compilador elige por vos un tipo entero compatible, y esa elección es **definida por la implementación**.

> [!IMPORTANT]
> **En el LPC1769 el `sizeof` de un `enum` no es 4.** El ABI de ARM manda usar el tipo más chico que alcance, y `arm-none-eabi-gcc` viene con `-fshort-enums` **activado por defecto**. Comprobado con el toolchain del repo:
>
> ```c
> enum big { X = 300 };   // sizeof(enum big) == 2   (¡dos bytes!)
> enum sm  { Y = 1   };   // sizeof(enum sm)  == 1   (¡un byte!)
> ```
>
> El mismo código en tu PC (x86) da **4 en los dos casos**. Dos consecuencias:
> 1. **Nunca asumas el tamaño de un `enum`** en un `struct` que mapea un registro, una trama de protocolo o algo que se guarde en memoria. Si necesitás un ancho exacto, poné `uint8_t`/`uint32_t` en el campo y usá las constantes del `enum` aparte.
> 2. Si alguna vez linkeás una librería precompilada con la opción contraria, los tamaños no coinciden y tenés corrupción silenciosa. El linker de GNU suele avisar con un warning sobre atributos `Tag_ABI_enum_size`.

> En **C23** el tipo subyacente se puede fijar explícitamente y el problema desaparece: `enum estado : uint8_t { OFF, ON };`

## Secuencias de escape

- Se utilizan cuando se escriben caracteres especiales dentro de `'` o `"`:

| Escape | Significado           |
|--------|------------------------|
| `\a`   | campana (alerta)       |
| `\b`   | retroceso              |
| `\f`   | avance de hoja         |
| `\n`   | nueva línea            |
| `\r`   | retorno de carro       |
| `\t`   | tabulador horizontal   |
| `\v`   | tabulador vertical     |
| `\\`   | barra invertida        |
| `\'`   | apóstrofe              |
| `\"`   | comillas               |
| `\?`   | signo de pregunta      |
| `\0`   | carácter nulo (es el caso `\ooo` con valor 0) |
| `\ooo` | valor octal (1 a 3 dígitos octales) |
| `\xhh` | valor hexadecimal (1 o más dígitos hexa) |

Dos trampas con estos:

* **`\ooo` toma como máximo 3 dígitos, pero `\xhh` toma *todos* los dígitos hexa que encuentre.** Así que `"\x41B"` no es `"AB"`: el compilador intenta leer `0x41B`, que no cabe en un `char`, y avisa `warning: hex escape sequence out of range`. Si necesitás un hexa seguido de una letra hexa, cortá la cadena en dos aprovechando la concatenación automática: `"\x41" "B"`.
* `'\0'` (el carácter nulo, valor 0) no es lo mismo que `'0'` (el dígito cero, valor 48), y ninguno de los dos es `NULL` (que es un puntero). Son tres cosas distintas que se escriben parecido.



## Arreglos (Arrays)

Un arreglo es una **colección de elementos del mismo tipo** almacenados en posiciones **contiguas de memoria**.

### Declaración

```c
tipo nombre[tamaño];
```

Ejemplo:

```c
int numeros[5];           // arreglo de 5 enteros
float temperaturas[10];   // arreglo de 10 floats
char mensaje[100];        // arreglo de 100 caracteres
```

---

### Inicialización

Estas son **cuatro alternativas**, no cuatro líneas seguidas (no podés declarar `arr` cuatro veces en el mismo scope):

```c
// Inicialización completa
int arr[5] = {1, 2, 3, 4, 5};

// Inicialización parcial (resto se inicializa en 0)
int arr[5] = {1, 2};  // {1, 2, 0, 0, 0}

// Tamaño inferido
int arr[] = {1, 2, 3, 4, 5};  // tamaño automático = 5

// Todos en cero
int arr[5] = {0};  // {0, 0, 0, 0, 0}
```

> [!CAUTION]
> **Un arreglo local SIN inicializar contiene basura**, no ceros. `int arr[5];` dentro de una función te da 5 valores impredecibles (lo que hubiera quedado en el stack). Solo los arreglos con duración estática (globales o `static`) arrancan en cero, porque el código de arranque limpia la sección `.bss`. Es un error clásico: funciona en el escritorio por casualidad y falla en el micro.

También podés inicializar posiciones sueltas con **inicializadores designados** (C99), muy útiles para tablas dispersas:

```c
uint8_t tabla[256] = { [10] = 0xFF, [200] = 0x0A };  // el resto queda en 0
```

---

### Acceso a elementos

Los índices **empiezan en 0**:

```c
int numeros[3] = {10, 20, 30};

int primero = numeros[0];   // 10
int segundo = numeros[1];   // 20
int tercero = numeros[2];   // 30

numeros[0] = 100;  // modificar elemento
```

> **IMPORTANTE**: C **no verifica límites**. Acceder a `numeros[10]` cuando solo hay 3 elementos es **comportamiento indefinido** (puede corromper memoria o causar crashes).

---

### Tamaño de un arreglo

```c
int arr[10];
size_t tamaño_bytes = sizeof(arr);      // 40 bytes (10 * 4)
size_t cantidad = sizeof(arr) / sizeof(arr[0]);  // 10 elementos
```

> Este truco solo funciona cuando el arreglo está en el mismo scope. Si pasas el arreglo a una función, `sizeof` devolverá el tamaño del puntero, no del arreglo.

---

### Arreglos multidimensionales

```c
// Matriz 3x3
int matriz[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

int valor = matriz[1][2];  // 6 (fila 1, columna 2)

// Inicialización lineal (equivalente)
int matriz[3][3] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
```

---

### Arreglos de caracteres (cadenas)

```c
char nombre[20] = "Hola";  // {'H', 'o', 'l', 'a', '\0', 0, 0, ...}

// Forma explícita
char saludo[] = {'H', 'o', 'l', 'a', '\0'};

// Tamaño automático con string literal
char mensaje[] = "Hola mundo";  // tamaño = 11 (incluye '\0')
```

> Las cadenas **siempre** terminan con `'\0'` (carácter nulo).

---

### Relación con punteros

Como se verá en las secciones de punteros, en **casi todos** los contextos el nombre de un arreglo se **convierte automáticamente** ("decae", *array decay*) en un puntero a su primer elemento:

```c
int arr[5] = {1, 2, 3, 4, 5};
int *ptr = arr;  // arr decae a &arr[0]

// Estas expresiones son equivalentes:
arr[2]  ==  *(arr + 2)  ==  ptr[2]  ==  *(ptr + 2)
```

> [!IMPORTANT]
> **Un arreglo NO es un puntero.** Es una simplificación que se dice mucho pero que induce a error. `arr` es de tipo `int[5]`; lo que pasa es que en la mayoría de las expresiones se convierte a `int *`. Hay **tres excepciones** donde el arreglo *no* decae y se ve la diferencia:
>
> ```c
> int arr[5];
> int *ptr = arr;
>
> sizeof(arr);   // 20 → el arreglo entero. sizeof(ptr) daría 4
> &arr;          // tipo int(*)[5] → puntero a arreglo de 5. &ptr es int**
> // 3) inicializar un char[] con un literal: char s[] = "Hola";  (copia, no apunta)
> ```
>
> Otra diferencia concreta: un puntero es una **variable** que ocupa sus 4 bytes en memoria y podés reasignar (`ptr = otro;`). El nombre del arreglo no es una variable reasignable: `arr = otro;` no compila. De ahí viene la analogía con "puntero constante", pero el arreglo además **no ocupa memoria propia** para guardar la dirección: la dirección *es* dónde está el arreglo.

---

### Uso en sistemas embebidos

```c
// Buffer para UART (en RAM, sin inicializar → va a .bss, arranca en cero)
static uint8_t rx_buffer[256];

// Tabla de lookup: al ser static const va a .rodata, o sea Flash. No gasta RAM.
static const uint16_t adc_to_temp[256] = { /* ... */ };

// Arreglo de punteros a los registros FIOxSET de los 5 puertos del LPC1769
// (UM10360 tabla 106: FIO0SET..FIO4SET, de 0x2009C018 a 0x2009C098, cada
//  puerto separado 0x20)
static volatile uint32_t * const gpio_set[5] = {
    (volatile uint32_t *) 0x2009C018,   // P0
    (volatile uint32_t *) 0x2009C038,   // P1
    (volatile uint32_t *) 0x2009C058,   // P2
    (volatile uint32_t *) 0x2009C078,   // P3
    (volatile uint32_t *) 0x2009C098    // P4
};

// Prender el pin `pin` del puerto `puerto`
void set_pin(uint8_t puerto, uint8_t pin) {
    *gpio_set[puerto] = (1u << pin);
}
```

> [!NOTE]
> Fijate en el tipo del último arreglo: `volatile uint32_t * const gpio_set[5]` es un **arreglo de punteros constantes a `uint32_t` volátiles**. El `volatile` va sobre el *registro apuntado* (que cambia por hardware), no sobre el arreglo. Declarar `volatile uint32_t gpio_ports[4] = {0x...}` sería un arreglo de *números* volátil, que no es lo que querés: son direcciones fijas, no datos que cambien. Y al ser `const`, el arreglo de punteros también se va a Flash.

---

### Limitaciones importantes

1. **Tamaño fijo**: una vez declarado, no se puede cambiar el tamaño
2. **No se puede asignar directamente**: `arr1 = arr2;` es **inválido**. Para copiar, `memcpy(arr1, arr2, sizeof arr1);` (de `<string.h>`)
3. **No se puede retornar un arreglo desde una función**: el lenguaje no admite tipos de retorno de arreglo, ni siquiera se puede escribir. Y si devolvés un *puntero* a un arreglo local, el arreglo ya murió al salir de la función: es **comportamiento indefinido** (GCC avisa con `-Wreturn-local-addr`). Las salidas son pasar un buffer del llamador, o usar `static`
4. **Sin verificación de límites**: accesos fuera de rango no generan error
5. **No se comparan con `==`**: `if (arr1 == arr2)` compara *direcciones*, no contenido. Para contenido, `memcmp()`

---

### Pasar arreglos a funciones

Cuando pasas un arreglo a una función, en realidad se pasa un **puntero**:

```c
#include <stdio.h>
#include <stddef.h>

// `int arr[]` acá es EXACTAMENTE lo mismo que `int *arr`
void procesar(const int arr[], size_t tamanio) {
    for (size_t i = 0; i < tamanio; i++) {
        printf("%d ", arr[i]);
    }
}

int main(void) {
    int datos[5] = {1, 2, 3, 4, 5};
    procesar(datos, sizeof datos / sizeof datos[0]);
    return 0;
}
```

> Por eso es necesario pasar el tamaño como parámetro separado: **dentro de `procesar()`, `sizeof(arr)` da 4** (el tamaño de un puntero en el M3), no 20. El `int arr[]` en la lista de parámetros es puramente decorativo; el compilador lo reescribe a `int *arr`. Incluso si escribís `int arr[5]`, el 5 se ignora y podés pasarle un arreglo de otro tamaño sin ningún warning.

> Nota: `int main(void)` con `void` explícito. En C, `int main()` significa "no digo nada sobre los parámetros" y no es lo mismo que "sin parámetros" (en C++ sí). Y ojo que en un programa para el LPC1769 `main()` no debe retornar: es un lazo infinito, porque no hay sistema operativo al que volver.

---

## Conversión de tipos

- Se puede convertir un tipo a otro usando **operadores de conversión** o **funciones de conversión**. Se puede hacer de forma implícita (automática) o explícita(manual).
- Ejemplo:
  ```c
  int   x = 10;
  float y = 3.14f;

  int z = x + y;    // DOS conversiones: x pasa a float (13.14f), y el
                    // resultado se trunca al asignarlo a int → z == 13
  int w = (int) y;  // conversión explícita: trunca hacia cero → w == 3
  ```

> Notá que en `float y = 3.14f;` el sufijo `f` importa: `3.14` sin sufijo es un **`double`**, que después se convierte a `float` al asignarlo. En el LPC1769, que no tiene FPU, dejar constantes `double` sueltas puede arrastrar toda la aritmética a 64 bits por software sin que te des cuenta. Escribí siempre el `f` en constantes de `float`.

Reglas generales: 
- C promociona tipos más pequeños a más grandes. Ej: Los `int` se convierten a `float` si hay un `float` en la operación.
- Los `char` y `short` se convierten a `int` antes de operar.
- Se puede convertir un tipo a uno más pequeño manualmente, pero se puede perder información (truncamiento).
- **La conversión de flotante a entero trunca hacia cero, no redondea:** `(int)3.9` da 3 y `(int)-3.9` da **-3** (no -4). Para redondear usá `roundf()` de `<math.h>`, o el truco entero `(int)(x + 0.5f)` si `x` es positivo.
- **Convertir un flotante a entero cuando el valor no cabe en el destino es comportamiento indefinido**, no un truncamiento prolijo: `(uint8_t)300.0f` no te garantiza 44. La regla del módulo 2^N solo vale entre tipos **enteros**.


Ejemplo en un sistema embebido:

```c
uint16_t valor = (uint16_t)(sensor_raw & 0xFFFF);
uint8_t dato = (uint8_t)(ADC_Read() >> 2);
```

### Posibles errores:
| Problema                              | Ejemplo                                                      |
| ------------------------------------- | ------------------------------------------------------------ |
| **Pérdida de datos**                  | `(uint8_t)300 → 44`                                          |
| **Truncamiento**                      | `(int)3.9 → 3`                                               |
| **Conversión entre signo/sin signo**  | `int a = -1; uint32_t b = a;` → `b` es enorme                |
| **Alineación incorrecta en punteros** | `(uint32_t *)ptr_byte` sin verificar alineación puede fallar |

---

## Promociones enteras: el bug silencioso de los tipos chicos

Esta es **la** fuente de errores sutiles más común cuando se programa un micro. Leela con atención.

En C, **antes de operar, todo tipo entero más chico que `int` se convierte (promociona) a `int`.** Esto incluye `char`, `signed char`, `unsigned char`, `short`, `uint8_t`, `uint16_t`, etc. Como en el Cortex-M3 `int` es de **32 bits**, cuando vos escribís una cuenta entre `uint8_t`, internamente se calcula con 32 bits.

Casi siempre eso es inofensivo. Pero a veces cambia el resultado de formas inesperadas:

### Trampa 1: el complemento (`~`) de un tipo chico

```c
uint8_t  reg  = 0x0F;
uint8_t  mask = 0x01;

// Intención: apagar el bit 0 de reg
reg = reg & ~mask;
```

Acá `mask` (un `uint8_t` con valor `0x01`) se promociona a `int`, queda `0x00000001`. Al aplicar `~` obtenés `0xFFFFFFFE` (32 bits, **no** `0xFE`). Como después hacés `& reg` y `reg` solo tiene 8 bits útiles, el resultado en este caso sale bien (`0x0E`). **Pero** mirá este otro:

```c
uint16_t valor = 0x1234;
uint8_t  byte_alto = ~valor >> 8;   // ¿qué da?
```

`valor` se promociona a `int`: `0x00001234`. `~` da `0xFFFFEDCB`, que como `int` es un número **negativo**. Y acá aparece la segunda trampa escondida: el `>> 8` de un `int` negativo es un **desplazamiento aritmético**, que replica el bit de signo. Así que el resultado es `0xFFFFFFED`, **no** `0x00FFFFED`. Al asignarlo a `uint8_t byte_alto` se trunca a `0xED`.

Comprobalo:

```c
uint16_t valor = 0x1234;
printf("%08X %08X\n", (unsigned)~valor, (unsigned)(~valor >> 8));
// imprime: FFFFEDCB FFFFFFED
```

Si esperabas el complemento del byte alto de un valor de 16 bits (`~0x12 = 0xED`)... acá tuviste suerte y dio lo mismo, pero el camino fue por 32 bits **con signo**. Cambiá el tipo o las máscaras y el resultado se te escapa. **Regla:** cuando uses `~` sobre tipos chicos, enmascará explícitamente el resultado al ancho que querés:

```c
reg = reg & (uint8_t)~mask;          // forzás 8 bits
byte_alto = (uint8_t)(~valor >> 8);  // queda claro y correcto
```

> **Lección extra:** para desplazar a la derecha, trabajá siempre con tipos **sin signo**. `>>` sobre un valor negativo es un desplazamiento aritmético en GCC/ARM, pero el estándar lo declara *definido por la implementación*. Con `unsigned` siempre es un desplazamiento lógico (rellena con ceros) y está garantizado. Otro motivo para usar `uint32_t` en manipulación de bits.

### Trampa 2: máscara de registro que se "desborda" hacia arriba

```c
uint8_t flags = 0xF0;
uint8_t resultado = (flags << 4);   // ¿0x00?
```

Uno esperaría que correr `0xF0` cuatro lugares a la izquierda en 8 bits "tire" los unos y quede `0x00`. Pero `flags` se promociona a `int`, el `<< 4` produce `0x00000F00`, **no se pierde nada en el cálculo**, y recién al asignar a `uint8_t` se trunca a `0x00`. El resultado final coincide acá, pero si en el medio comparás o usás el valor intermedio, vas a ver `0xF00`, no `0x00`. Por eso, en manipulación de registros, conviene **operar en el ancho del registro** (típicamente `uint32_t` en el LPC1769) y enmascarar al final.

### Trampa 3: la comparación que nunca se cumple

```c
uint8_t a = 200;
uint8_t b = 100;
if (a + b > 255) {        // a+b se calcula en int: 300 > 255 → ¡verdadero!
    // entra acá
}
```

Como `a + b` se hace en `int` (300, no 44), la comparación da verdadero aunque "en 8 bits" la suma se hubiera desbordado a 44. No está mal, pero hay que **saberlo**: la suma no se desborda durante el cálculo, solo cuando la guardás de vuelta en un `uint8_t`.

> **Conclusión:** los tipos `uint8_t`/`uint16_t` son geniales para **almacenar**, pero recordá que **se calculan en `int` (32 bits)**. El truncamiento ocurre al **asignar** de vuelta a un tipo chico, no durante la cuenta. Cuando el ancho importa (máscaras, shifts, complementos), poné un cast explícito al ancho deseado.

---

## `signed` vs `unsigned`: bichos clásicos

### El bucle que nunca termina

```c
// ¡BUG! Bucle infinito
for (uint8_t i = 9; i >= 0; i--) {
    procesar(i);
}
```

Un `unsigned` **nunca** es negativo. Cuando `i` vale 0 y hacés `i--`, da la vuelta a 255 (en `uint8_t`) o a 4 294 967 295 (en `uint32_t`): jamás se cumple `i < 0`, así que `i >= 0` es **siempre verdadero**. Soluciones:

```c
// Opción A: usar un tipo con signo
for (int i = 9; i >= 0; i--) { ... }

// Opción B: condición con "mayor que" y otra forma de contar
for (uint8_t i = 10; i-- > 0; ) { ... }   // truco: post-decremento

// Opción C: contar al revés
for (uint8_t i = 0; i < 10; i++) {
    uint8_t j = 9 - i;
    ...
}
```

La opción B es el idiom estándar y vale la pena entenderla: `i-- > 0` primero **compara** `i` con 0 y después lo decrementa. Con `i = 10` entra al cuerpo con `i == 9`; en la última vuelta compara `1 > 0` (verdadero) y entra con `i == 0`; después compara `0 > 0` (falso) y sale. Recorre 9, 8, ..., 1, 0 y nunca decrementa por debajo de cero.

> **Este bug también lo detecta el compilador, pero solo con `-Wextra`.** Con `-Wall` solo, GCC no dice nada:
> ```console
> $ gcc -Wall -c bucle.c          # silencio total
> $ gcc -Wall -Wextra -c bucle.c
> warning: comparison is always true due to limited range of data type [-Wtype-limits]
> ```
> Otra razón para no compilar nunca sin `-Wextra`.

### Comparaciones mixtas signed/unsigned

Si comparás un `signed` con un `unsigned`, C convierte **el con signo a sin signo** (regla de conversiones aritméticas usuales). Esto da resultados absurdos:

```c
int a = -1;
unsigned int b = 1;
if (a < b) {
    // NO entra: -1 se convierte a 0xFFFFFFFF (4294967295), que NO es < 1
}
```

Compilá con `-Wsign-compare` y el compilador te avisa de estas comparaciones. **Cuidado con un detalle:** en **C** ese warning **no** viene con `-Wall`, viene con **`-Wextra`** (en C++ sí está en `-Wall`, de ahí la confusión). O sea que compilar solo con `-Wall` te deja pasar este bug en silencio. Usá siempre las dos:

```make
CFLAGS += -Wall -Wextra
```

**Regla práctica:** no mezcles signo en comparaciones; elegí un signo y mantenelo.

> Ojo que el problema aparece cuando el `unsigned` tiene rango **mayor o igual** al del `signed`. Comparar `int` con `uint8_t` **no** tiene este problema: el `uint8_t` se promociona a `int` y la comparación se hace con signo, como esperás. El bug vive cuando comparás `int` contra `unsigned int`, `uint32_t` o `size_t`. Y `size_t` está por todas partes (`strlen()`, `sizeof`), así que este caso es el más frecuente en la práctica:
>
> ```c
> for (int i = 0; i < strlen(s); i++)   // -Wextra avisa: int vs size_t
> for (size_t i = 0; i < strlen(s); i++) // así está bien
> ```

### Overflow: `unsigned` da la vuelta, `signed` es comportamiento indefinido

| Tipo       | Qué pasa al desbordar                                          |
|------------|---------------------------------------------------------------|
| `unsigned` | Da la vuelta de forma **definida**: aritmética módulo 2^N. `0xFF + 1 == 0x00` en `uint8_t`. Esto está **garantizado**. |
| `signed`   | Es **comportamiento indefinido (UB)**. `INT_MAX + 1` no "da la vuelta a INT_MIN"; el compilador puede asumir que nunca pasa y optimizar de formas que te rompen el programa. |

Por eso, para **contadores que dan la vuelta** (timestamps, índices circulares, CRC, hashes) usá siempre tipos `unsigned`. El cálculo de diferencias de tiempo con `uint32_t` que dan la vuelta funciona justamente porque el overflow unsigned está definido:

```c
uint32_t t0 = millis();
// ... pasa el tiempo, incluso si el contador da la vuelta ...
uint32_t transcurrido = millis() - t0;   // correcto aun con wraparound
```

> ### Para los curiosos (avanzado): reglas exactas de conversión
>
> Las reglas que usé arriba tienen nombre formal en el estándar:
> - **Promoción entera (integer promotion):** todo tipo entero de rango menor que `int` se convierte a `int` (o a `unsigned int` si `int` no puede representar todos sus valores). En el M3, `uint16_t` cabe en `int`, así que se promociona a `int` (con signo), no a `unsigned`.
> - **Conversiones aritméticas usuales (usual arithmetic conversions):** cuando los dos operandos son de tipos distintos tras la promoción, se llevan a un "tipo común" siguiendo un ranking (`int` < `unsigned int` < `long` < ...). Si uno es `unsigned` y tiene rango mayor o igual, el otro se convierte a `unsigned`. De ahí sale el bug de comparar `int` con `unsigned int`.
> - **Truncamiento:** al convertir a un tipo entero **sin signo** más chico, se conservan los bits de menor orden (módulo 2^N) y está **garantizado**. Para destino **con signo** y un valor fuera de rango, en C99/C11/C17 el resultado es *definido por la implementación* (en GCC/ARM, complemento a dos sin sorpresas); **en C23 pasó a estar definido** como módulo 2^N, porque C23 obliga a que los enteros con signo sean complemento a dos y eliminó los formatos exóticos (complemento a uno, signo-magnitud).
> - El cast explícito **no** elimina estas reglas; solo te deja controlar **cuándo** ocurre la conversión (y le dice al compilador y al lector que la pérdida es intencional, lo que además calla el warning).
> - **`sizeof` no evalúa su operando.** `sizeof(i++)` no incrementa `i`: el compilador solo mira el *tipo*. Es un operador de tiempo de compilación (salvo con VLAs), y su resultado es de tipo `size_t`.

---

## Resumen de reglas para no equivocarse

Si te llevás solo una cosa de este capítulo, que sea esta lista:

| Regla | Por qué |
|-------|---------|
| Declará enteros con `<stdint.h>` (`uint32_t`, `int8_t`...) | El ancho es explícito y portable |
| `char` solo para texto; `uint8_t` para bytes | En ARM `char` es `unsigned`, en x86 es `signed` |
| `volatile` en toda variable compartida con una ISR o un periférico | Si no, el compilador cachea la lectura y con `-O2` te rompe el programa |
| `u`/`U` en las máscaras de bits: `(1u << 31)` | `1 << 31` desborda un `int` con signo → UB |
| No mezcles `signed` con `unsigned` en comparaciones | El `signed` se convierte a `unsigned` y `-1 < 1u` es falso |
| `unsigned` para contadores que dan la vuelta | El wraparound `unsigned` está definido; el `signed` es UB |
| Cast explícito al ancho deseado en máscaras, shifts y `~` | Todo se calcula en `int` de 32 bits por promoción entera |
| Desplazá a la derecha solo tipos `unsigned` | `>>` de un negativo es definido por la implementación |
| `static const` para tablas grandes | Van a Flash (`.rodata`), no gastan RAM |
| Compilá con `-Wall -Wextra` | Varios bugs de este capítulo **solo** los ve `-Wextra` |
| Evitá `float`/`double` en código crítico | El Cortex-M3 del LPC1769 no tiene FPU: todo por software |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**
- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes: 6.2.4 (duración de almacenamiento), 6.2.5 (tipos), 6.3.1.1 (promoción entera y conversiones aritméticas usuales), 6.3.1.3 (conversión entre enteros), 6.4.4 (constantes), 6.7.1 (especificadores de almacenamiento), 6.7.3 (calificadores).
- [cppreference: C language](https://en.cppreference.com/w/c/language). La referencia práctica más clara, con los cambios por versión del estándar.
- [Límites de identificadores (5.2.4.1)](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/declarations-and-initialization-dcl/dcl23-c/). CERT DCL23-C, sobre los 63/31 caracteres significativos.

**GCC y el toolchain**
- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). Confirma que `-Wsign-compare` está en `-Wextra` para C (y en `-Wall` solo para C++), y que `-Wtype-limits` está en `-Wextra`.
- [GCC: Implementation-defined behavior](https://gcc.gnu.org/onlinedocs/gcc/C-Implementation.html). Qué elige GCC donde el estándar deja libertad (signo de `char`, `>>` de negativos, tipo de los `enum`).
- Los tamaños y el signo de `char` de este capítulo se pueden verificar con el toolchain del repo:
  ```console
  $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -dM -E - < /dev/null | grep -E 'SIZEOF|CHAR_UNSIGNED'
  ```

**ARM y el LPC1769**
- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf). Está en el repo. Capítulo 9 (GPIO) para las direcciones de `FIOxDIR`/`FIOxSET`/`FIOxPIN` usadas en los ejemplos.
- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). Define que `char` es `unsigned`, que `long double` es igual a `double` y el tamaño de los `enum` en ARM de 32 bits.

**Sobre los temas puntuales**
- [What are storage class specifiers in C?](https://how.dev/answers/what-are-storage-class-specifiers-in-c)
- [Why Not Mix Signed and Unsigned Values in C/C++? (John Regehr)](https://blog.regehr.org/archives/268). Sobre el bug de comparaciones mixtas.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Siguiente:** [02 - Operadores](./02-operadores.md)
