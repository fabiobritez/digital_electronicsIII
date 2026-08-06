# El preprocesador de C

## ¿Qué es el preprocesador en C?

Antes de que el código en C se compile, **pasa por una etapa llamada "preprocesamiento"**.
Esta etapa maneja las **instrucciones que empiezan con `#`**, llamadas **directivas del preprocesador**.

### El preprocesador:

* No entiende C como tal (no analiza variables ni tipos).
* Solo **hace sustituciones y control de texto**.
* El resultado es un nuevo archivo `.c` con el código modificado (ya con includes, defines, etc. resueltos).

---

## Directivas del preprocesador más comunes

| Directiva  | ¿Qué hace?                                 |
| ---------- | ------------------------------------------ |
| `#include` | Inserta código de otro archivo             |
| `#define`  | Define macros o constantes                 |
| `#undef`   | Elimina una definición previa              |
| `#ifdef`   | Compila si la macro está definida          |
| `#ifndef`  | Compila si la macro NO está definida       |
| `#if`      | Compila si la condición se cumple          |
| `#elif`    | "else if" para `#if`                       |
| `#else`    | Alternativa si no se cumple la condición   |
| `#endif`   | Marca el final de una condición            |
| `#error`   | Muestra un mensaje de error de compilación |
| `#pragma`  | Proporciona instrucciones específicas al compilador  |

---

## 1. `#include`: Incluir archivos

### Ejemplo:

```c
#include <stdio.h>   // bibliotecas del sistema
#include "mi_sensor.h"  // archivos locales
```

* En sistemas embebidos, se usa para:

  * Incluir controladores (`lpc1769_gpio.h`, `lpc1769_uart.h`)
  * Incluir registros (`lpc1769.h`, etc.)
  * Separar código por módulos (main, periféricos, etc.)

### Qué hace `#include` en realidad

Nada mágico: **copia y pega el archivo completo en ese punto**. Si `mi_sensor.h` tiene 40 líneas, el
compilador recibe tu `.c` con esas 40 líneas insertadas donde estaba el `#include`. Nunca "importa"
ni "enlaza" nada.

La única diferencia entre las dos formas es **dónde busca el archivo**:

| Forma | Dónde busca |
|---|---|
| `#include <stdio.h>` | solo en los directorios del sistema y en los que pases con `-I` |
| `#include "mi_sensor.h"` | primero al lado del archivo actual; si no lo encuentra, busca como `<>` |

Regla práctica: `<>` para headers del compilador y de librerías (`<stdint.h>`, `"LPC17xx.h"` según
cómo esté instalado), comillas para los tuyos.

#### ¿Y dónde están, físicamente?

Los headers del sistema no son abstractos: son archivos que podés abrir y leer. Para el LPC1769 vienen
de **tres lugares distintos**:

| Familia | Qué trae | Dónde |
|---|---|---|
| **Del compilador (GCC)** | `stdint.h`, `stdbool.h`, `stddef.h`, `limits.h`, `float.h`, `stdarg.h` | dentro del toolchain, en `lib/gcc/arm-none-eabi/<version>/include/` |
| **De la librería estándar (newlib)** | `stdio.h`, `string.h`, `stdlib.h`, `math.h`, `inttypes.h`, `time.h`... | dentro del toolchain, en `arm-none-eabi/include/` |
| **Del fabricante (CMSIS/NXP)** | `LPC17xx.h`, `core_cm3.h` | en el repo: `library/CMSISv2p00_LPC17xx/inc/` |

La división no es caprichosa: **los que definen tipos y límites los tiene que dar el compilador**,
porque solo él sabe cuántos bits mide un `int` en este target. Los que dan funciones los da la libc.
Por eso `stdint.h` viene de GCC y `inttypes.h` de newlib, aunque parezcan hermanos.

> [!NOTE]
> **La carpeta del toolchain no está en el repositorio** (pesa cientos de MB y está en `.gitignore`).
> La crea `bash tools/install_toolchain.sh`, que la deja en `tools/toolchain/`. Si esa carpeta no
> existe todavía, correr eso primero. Por lo mismo, **no memorices la ruta**: el número de versión de
> GCC cambia cuando el curso actualiza el toolchain.

En vez de anotar rutas, **preguntale al compilador**. Estos comandos andan en cualquier máquina y con
cualquier versión:

```console
$ arm-none-eabi-gcc -print-file-name=include        # dónde están los headers de GCC
$ arm-none-eabi-gcc -print-sysroot                  # ahí adentro, include/ son los de newlib

$ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -E -Wp,-v - < /dev/null
#include <...> search starts here:
 .../lib/gcc/arm-none-eabi/13.2.1/include
 .../lib/gcc/arm-none-eabi/13.2.1/include-fixed
 .../arm-none-eabi/include
End of search list.
```

Y para saber **qué archivo terminó usando** un `#include` concreto:

```console
$ echo '#include <limits.h>' | arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -E - | grep limits.h
```

Leerlos vale la pena: abrí `limits.h` y vas a encontrar los valores reales de **esta** placa. Por
ejemplo, ahí se ve que `CHAR_BIT` es 8 y que `CHAR_MIN` es **0**, no `-128`, que es la comprobación
de que en ARM el `char` es `unsigned` (ver [01 - Declaraciones y tipos](./01-declaraciones-y-tipos.md)).

> Qué hay en cada carpeta del toolchain, por qué pesa 710 MB y qué es el *multilib*, en
> [18 - Adentro de la carpeta del toolchain](../anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md).

### Por qué existen los `.h`: cada `.c` se compila solo

Esta es la idea que hace que todo lo demás tenga sentido. Cuando tu proyecto tiene varios archivos:

```
main.c     sensor.c     uart.c
```

el compilador los procesa **uno por uno, por separado**, y produce un `.o` de cada uno. Mientras
compila `main.c`, **no ve `sensor.c` en absoluto**. No sabe que existe. Recién al final el *linker*
junta los `.o` y resuelve quién llama a quién.

```
main.c   ──[compilador]──▶ main.o   ─┐
sensor.c ──[compilador]──▶ sensor.o ─┼─[linker]─▶ firmware.elf
uart.c   ──[compilador]──▶ uart.o   ─┘
   ▲
   └── cada uno se compila a ciegas, sin ver a los otros
```

Entonces, si `main.c` quiere llamar a `sensor_leer()`, tiene un problema: el compilador necesita
saber **cuántos parámetros lleva y qué devuelve** para generar la llamada correcta, y esa información
está en `sensor.c`, que no puede ver.

La solución es el **header**: un archivo con las *declaraciones* (las promesas), que `main.c`
incluye. Ahí está el contrato; el código de verdad sigue en `sensor.c`.

### Qué va en el `.h` y qué en el `.c`

La regla es una sola: **en el `.h` va lo que otros archivos necesitan conocer; en el `.c`, cómo está
hecho.**

```c
// ───── sensor.h : LO QUE OTROS NECESITAN SABER ─────
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

#define SENSOR_MAX_MUESTRAS  16        // constantes de la API

typedef struct {                        // tipos que la API usa
    uint16_t crudo;
    int16_t  celsius;
} sensor_dato_t;

void     sensor_init(void);             // PROTOTIPOS: qué existe, no cómo
uint16_t sensor_leer(void);

extern uint32_t sensor_errores;         // "esta global existe en algún .c"

#endif
```

```c
// ───── sensor.c : CÓMO ESTÁ HECHO ─────
#include "sensor.h"                     // el .c incluye su propio .h

uint32_t sensor_errores = 0;            // LA DEFINICIÓN (acá se reserva la memoria)

static uint16_t ultima_lectura;         // 'static' = privado, nadie más lo ve

static void calibrar(void) { ... }      // función interna: NO va en el .h

void sensor_init(void) { ... }          // las implementaciones de verdad
uint16_t sensor_leer(void) { ... }
```

| Va en el `.h` | Va en el `.c` |
|---|---|
| Prototipos de las funciones públicas | El **cuerpo** de todas las funciones |
| `typedef`, `struct`, `enum` que usa la API | Variables `static` (privadas del archivo) |
| `#define` de constantes públicas | Funciones `static` (auxiliares internas) |
| `extern` de las globales compartidas | La **definición** de esas globales (una sola vez) |

> **La distinción clave es declarar vs definir.** *Declarar* es prometer que algo existe (no reserva
> memoria, se puede repetir en muchos archivos): eso va en el `.h`. *Definir* es crearlo de verdad
> (reserva memoria o genera código, y tiene que pasar **exactamente una vez** en todo el programa):
> eso va en el `.c`. Es la misma idea de
> [prototipo vs cuerpo de función](./06-funciones.md#declaración-vs-definición), ahora repartida
> entre dos archivos.

### Los dos errores que vas a ver, y qué significan

Entender el modelo de arriba convierte dos mensajes crípticos en algo obvio:

- **`undefined reference to 'sensor_leer'`** → lo prometiste (está el prototipo en el `.h`) pero
  nadie lo cumplió: falta el cuerpo, o te olvidaste de compilar `sensor.c` y sumarlo al linkeo. Es un
  error **del linker**, no del compilador: por eso aparece al final, cuando ya "compiló todo bien".

- **`multiple definition of 'sensor_errores'`** → lo definiste más de una vez. La causa típica es
  haber puesto `uint32_t sensor_errores;` (sin `extern`) **en el `.h`**: como el header se copia en
  cada `.c` que lo incluye, terminás con una definición por archivo. En el `.h` va el `extern`; la
  definición, en un solo `.c`.

Y ahora sí, con esto en la cabeza, el **include guard** que vas a ver en la sección 4 deja de ser una receta: como
`#include` es copiar y pegar, si dos headers incluyen a un tercero, ese tercero se pega **dos veces**
en el mismo `.c` y todos sus `typedef` quedan duplicados. El guard hace que la segunda copia quede
vacía.

---

## 2. `#define`: Constantes y macros

### Constantes:

```c
#define LED_PIN 13
```

### Macros con argumentos:

```c
#define MAX(a,b) ((a) > (b) ? (a) : (b)) // macro que devuelve el mayor de dos valores
```



* Muy usado en embebidos para definir pines, tamaños, direcciones de memoria, etc.

  ```c
  #define UART0_BASE 0x4000C000
  ```

### Las dos trampas de las macros con argumentos

Una macro **no es una función**: es sustitución de texto antes de compilar. Eso trae dos errores
clásicos que tenés que conocer sí o sí.

**Trampa 1: Faltan paréntesis.** Como es texto, la precedencia de operadores te puede traicionar:

```c
#define CUADRADO(x) x * x        // MAL

int r = CUADRADO(2 + 3);         // se expande a:  2 + 3 * 2 + 3  = 11  (esperabas 25)
```

Por eso la regla es **encerrar cada argumento y el resultado completo entre paréntesis**:

```c
#define CUADRADO(x) ((x) * (x))  // BIEN

int r = CUADRADO(2 + 3);         // ((2 + 3) * (2 + 3)) = 25
```

**Trampa 2: Doble evaluación.** Como el argumento se pega tal cual, si lo usás dos veces en la
macro, **se ejecuta dos veces**. Con efectos secundarios esto es un bug feo:

```c
#define MAX(a,b) ((a) > (b) ? (a) : (b))

int m = MAX(leer_adc(), 100);    // ¡leer_adc() se llama DOS veces! (una en la comparación, otra en el resultado)
int n = MAX(i++, 10);            // i se incrementa una o dos veces, según cuál rama gane
```

Una función `inline` (cap. 14) no tiene este problema: evalúa cada argumento una sola vez. Por eso,
para algo que no necesita ser macro, **preferí `static inline`**.

### El idiom `do { ... } while(0)`

¿Cómo escribís una macro de **varias sentencias** que se comporte como una sola instrucción? El
intento ingenuo se rompe con `if`/`else`:

```c
#define LED_ON()  LPC_GPIO0->FIODIR |= M;  LPC_GPIO0->FIOSET = M   // MAL

if (cond) LED_ON();   // solo la primera línea queda dentro del if; la segunda corre SIEMPRE
```

La solución estándar es envolver el cuerpo en `do { ... } while(0)`. Es un único bloque que se ejecuta
una vez, y admite el `;` final sin romper el `if`/`else`:

```c
#define LED_ON()  do {                       \
        LPC_GPIO0->FIODIR |= M;               \
        LPC_GPIO0->FIOSET  = M;               \
    } while (0)

if (cond) LED_ON();   // ahora SÍ: ambas sentencias quedan dentro del if
else      otra_cosa();
```

Es el patrón que vas a ver en casi todas las macros multi-línea de drivers y de CMSIS.

### Stringify `#` y token paste `##`

Dos operadores del preprocesador que parecen oscuros pero son utilísimos:

- **`#` (stringify)**: convierte un argumento de macro en una **cadena de texto**.
- **`##` (token paste)**: **pega** dos tokens para formar un identificador nuevo.

```c
#define IMPRIMIR_VAR(x)   printf(#x " = %d\r\n", (x))   //  #x  ->  "x"  (el nombre como texto)

int temperatura = 25;
IMPRIMIR_VAR(temperatura);   // imprime:  temperatura = 25     (útil para depurar)
```

```c
#define REG_PUERTO(n)   LPC_GPIO##n           //  ##  pega "LPC_GPIO" + n
REG_PUERTO(0)->FIOSET = M;                    //  ->  LPC_GPIO0->FIOSET = M
```

`##` es el motor que hace posibles las **X-macros**, que vemos al final del capítulo.

## 3. `#undef`: Elimina una macro

```c
#include <driver_x.h> // incluye una librería que define una macro MAX
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b)) // reemplaza la macro MAX por nuestra propia macro
```

* Útil si incluiste algo que define una macro que querés reemplazar. Por ejemplo, en un programa que usa una librería que define una macro `MAX` y queremos reemplazarla por nuestra propia macro.

---

## 4. Condicionales: `#if`, `#ifdef`, `#ifndef`, `#else`, `#endif`

### a) `#ifdef` / `#ifndef`

```c
#ifdef DEBUG
  // Solo se compila si DEBUG está definido
#endif
```

```c
#ifndef SENSOR_H
#define SENSOR_H
// código del archivo .h
#endif
```

→ Este último ejemplo es un **include guard**: evita múltiples inclusiones de un mismo archivo.

---

### b) `#if`, `#elif`, `#else`, `#endif`

```c
#define BOARD_VERSION 2

#if BOARD_VERSION == 1
  #define LED_PIN 13
#elif BOARD_VERSION == 2
  #define LED_PIN 2
#else
  #error "Versión de placa no soportada"
#endif
```

→ Muy útil para compilar el mismo código para varias plataformas o versiones de hardware.

---

### c) Include guards vs `#pragma once`

El **include guard** que acaba de aparecer en el punto a) es el uso más frecuente de `#ifndef`: el par
`#ifndef`/`#define`/`#endif` que evita que un header se incluya dos veces (lo que daría errores de
"redefinición"). Hay una alternativa más corta:

```c
// estilo include guard (portable, estándar):
#ifndef MI_SENSOR_H
#define MI_SENSOR_H
// ... contenido ...
#endif

// estilo pragma once (más corto, no estándar pero soportado por GCC/Clang/ARM):
#pragma once
// ... contenido ...
```

`#pragma once` es una línea contra tres y no podés equivocarte con el nombre del guard. Su contra:
no es parte del estándar C (aunque en la práctica todos los compiladores que vas a usar lo soportan).
**Recomendación para el curso: usá include guards** (son universales y los ves en todo CMSIS); pero
sabé que `#pragma once` existe y significa lo mismo.

---

## 5. `#error` y `#pragma`

### `#error`

```c
#ifndef CLOCK_FREQ
  #error "CLOCK_FREQ no está definido"
#endif
```

→ Detiene la compilación con un mensaje útil.

### `#pragma`

Su nombre viene de "pragmatic information". Cada compilador tiene sus propios #pragma, pero hay algunos comunes. En embebidos se usa para:

* Controlar alineación de estructuras que van a memoria o hardware.
* Desactivar warnings, habilitar extensiones del compilador.

  **No afecta la lógica del programa directamente**, sino **cómo el compilador trata ciertas partes**.

---

## Uso típico en embebidos

1. **Controlar alineación de estructuras (`#pragma pack`)**
2. **Desactivar advertencias (`#pragma warning`)**
3. **Ubicar código o datos en regiones de memoria específicas**
4. **Evitar padding innecesario en estructuras**
5. **Definir secciones para el linker (en microcontroladores)**

---

### Ejemplo 1: `#pragma pack` 
Con el objetivo de evitar relleno ("padding") automático que el compilador pone en estructuras para alinearlas.

> El *padding*, por qué existe, qué cuesta sacarlo en un Cortex-M3 y cuándo conviene de verdad se
> ven en [13 - Structs para hardware](./13-structs-para-hardware.md#padding-y-alineación). Acá solo
> nos interesa la directiva.

#### Sin `#pragma`:

```c
struct SensorData {
  uint8_t id;     // 1 byte
  uint32_t value; // 4 bytes
};
```

Muchos compiladores **insertan 3 bytes vacíos** después de `id`, para que `value` empiece en una dirección múltiplo de 4. Por lo que la estructura **ocupa 8 bytes**, no 5.

#### Con `#pragma pack(1)`:

```c
#pragma pack(1)
struct SensorData {
  uint8_t id;
  uint32_t value;
};
#pragma pack()
```

Esto **desactiva el relleno automático**, y ahora la estructura ocupa **solo 5 bytes**.
 
---

### Ejemplo 2: Desactivar advertencias
 
En GCC:

```c
#pragma GCC diagnostic ignored "-Wunused-variable"
```

---

### Ejemplo 3: Controlar sección de memoria  

En el caso de que una función se ubique en una región de memoria específica (útil en **bootloaders**, por ejemplo):

```c
#pragma location = 0x0800F800
const uint8_t firmware_version[] = { 1, 0, 3 };
```

Este `#pragma` (dependiendo del compilador, por ejemplo IAR) indica que `firmware_version` debe ir **exactamente** en esa dirección de memoria.

---
 
## X-macros: generar tablas y enums desde una sola lista

> Las dos secciones que siguen son **opcionales**: ya viste todas las directivas. Son las dos
> técnicas del preprocesador que más rinden en firmware, pero podés dejarlas para una segunda vuelta.

Esta es la técnica "avanzada" más útil del preprocesador para embebidos. La idea: definís **una sola
lista** de tus datos (pines, registros, comandos, estados) y la reusás para generar automáticamente
un `enum`, una tabla, nombres en texto, etc. Si agregás un elemento, **todo se actualiza solo** y nunca
quedan desincronizados.

Ejemplo concreto: una tabla de pines de la placa.

```c
// 1) UNA sola lista: nombre, puerto, bit
#define LISTA_PINES(X)        \
    X(LED,    0, 22)          \
    X(BOTON,  2, 10)          \
    X(BUZZER, 1,  5)

// 2) Generamos un enum con un índice por pin (+ un PIN_COUNT al final, gratis)
#define ENUM_PIN(nombre, puerto, bit)  PIN_##nombre,
typedef enum { LISTA_PINES(ENUM_PIN) PIN_COUNT } pin_id_t;

// 3) Generamos la tabla de definiciones (puerto, bit) de cada pin
typedef struct { uint8_t puerto; uint8_t bit; } pin_def_t;
#define TABLA_PIN(nombre, puerto, bit)  { (puerto), (bit) },
static const pin_def_t pines[PIN_COUNT] = { LISTA_PINES(TABLA_PIN) };
```

Ahora `pines[PIN_LED].bit` vale 22, `PIN_COUNT` vale 3, y la tabla vive en Flash (es `const`). Para
agregar un pin, tocás **una sola línea** de `LISTA_PINES` y el enum, el conteo y la tabla se
regeneran juntos. Lo mismo sirve para tablas de comandos UART, máquinas de estado con nombres, o
mapas de registros.

> Las X-macros se apoyan en `##` (para `PIN_##nombre`) y en que la lista recibe como argumento la
> "operación" `X` a aplicar a cada fila. Cuesta un poco leerlas la primera vez; el beneficio es que
> eliminan toda una clase de bugs por tablas desincronizadas.

---

## Compilación condicional para DEBUG, `assert` y `static_assert`

### Bloques de depuración con `#ifdef`

Un patrón muy común: código de depuración (logs por UART) que **solo** se compila en la build de
debug, para que la build de producción salga chica y rápida.

```c
#ifdef DEBUG
    #define LOG(...)  printf(__VA_ARGS__)   // VA_ARGS: argumentos variables, como printf
#else
    #define LOG(...)  ((void)0)             // en release no genera NADA de código
#endif

LOG("ADC = %u\r\n", cuentas);   // se imprime en debug, desaparece en release
```

Se activa pasándole `-DDEBUG` al compilador. `((void)0)` es una "sentencia que no hace nada", para
que la línea con `;` siga siendo válida.

### `assert`: chequeos en tiempo de EJECUCIÓN

`assert(cond)` (de `<assert.h>`) aborta el programa si `cond` es falsa: sirve para atrapar
"esto nunca debería pasar" durante el desarrollo. En release se desactiva definiendo `NDEBUG`. Ojo:
en un micro, `assert` necesita una implementación de qué hacer al fallar (típicamente quedar en un
`while(1)` con un breakpoint), y **gasta** Flash con los strings. Úsalo en desarrollo, no lo dejes en
producción.

### `_Static_assert`: chequeos en tiempo de COMPILACIÓN

Mucho más valioso en embebidos: `_Static_assert(condicion, "mensaje")` (C11; también `static_assert`
con `<assert.h>`) verifica una condición **al compilar**. Si es falsa, **no compila** y te muestra el
mensaje. Costo en el binario: **cero**. El uso estrella es verificar el tamaño de una struct que va a
hardware o a un protocolo, donde un byte de padding inesperado sería un desastre:

```c
typedef struct __attribute__((packed)) {
    uint8_t  comando;
    uint32_t valor;
} trama_t;

_Static_assert(sizeof(trama_t) == 5, "trama_t debe medir 5 bytes (revisar padding/packed)");
```

Si alguien saca el `packed` o cambia un campo y la struct pasa a medir 8, el compilador frena al
instante con el mensaje, en vez de mandar tramas corruptas que descubrís recién en el osciloscopio.
También sirve para chequear supuestos de plataforma:

```c
_Static_assert(sizeof(int) == 4, "este codigo asume int de 32 bits (Cortex-M3)");
```

---

## Utilidad del preprocesador en embebidos

* **Configuración por hardware:** se define qué pines, módulos, o versiones se usan.
* **Compilar para distintos dispositivos sin cambiar el código base.**
* **Control preciso del código incluido**, esencial para ahorrar memoria y ciclos.
* **Facilita reutilizar drivers entre proyectos**.

---

## Resumen de reglas para no equivocarse

| Regla | Por qué |
| ----- | ------- |
| Un `#define` **no lleva** `;` al final | El preprocesador pega texto literal: el `;` viaja con la macro y rompe cualquier expresión |
| Encerrá entre paréntesis **cada argumento y el resultado** de una macro | `#define CUADRADO(x) x*x` con `CUADRADO(2+3)` da 11, no 25 |
| Una macro evalúa sus argumentos **tantas veces como los escriba** | `MAX(leer_adc(), 100)` llama al ADC dos veces; si no necesita ser macro, usá `static inline` |
| Macros de varias sentencias: siempre `do { ... } while (0)` | Es lo único que se comporta como una sentencia única dentro de un `if`/`else` |
| Include guard en **todo** `.h` | `#include` es copiar y pegar: sin guard, un header incluido dos veces duplica sus `typedef` |
| En el `.h` va el `extern`; la definición, en **un solo** `.c` | Si no, `multiple definition` en el linker |
| `_Static_assert` para todo layout que tenga que medir algo exacto | Cuesta cero bytes y frena la compilación en vez de mandar tramas corruptas |
| No dejes `assert` en la build de producción | Gasta Flash con los strings; se apaga definiendo `NDEBUG` |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.10 (todas las directivas), 6.10.3 (macros con argumentos, `#` y `##`), 6.10.3.5 ¶2 (por qué una macro no se expande recursivamente), 7.2 (`<assert.h>`), 6.7.10 (`_Static_assert`).
- [cppreference: Preprocessor](https://en.cppreference.com/w/c/preprocessor). Referencia completa, con las macros predefinidas (`__FILE__`, `__LINE__`, `__DATE__`).

**GCC y el toolchain**

- [The C Preprocessor (manual de GCC)](https://gcc.gnu.org/onlinedocs/cpp/). El manual entero del `cpp`, con un capítulo dedicado a las trampas de las macros y otro a `#pragma once`.
- [GCC: Pragmas](https://gcc.gnu.org/onlinedocs/gcc/Pragmas.html). `#pragma GCC diagnostic` y `#pragma pack`.
- Para ver **qué queda** después del preprocesador, sin compilar nada:
  ```console
  $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -E mi_archivo.c | less
  ```
  Es la mejor forma de depurar una macro que no hace lo que creías.

**Sobre los temas puntuales**

- [X-Macros (Randy Meyers, Dr. Dobb's)](https://www.drdobbs.com/the-new-c-x-macros/184401387). El artículo que popularizó la técnica.
- [Dónde vive cada header del toolchain](../anexos/B_toolchain_y_entorno/04-adentro-del-toolchain.md). Qué es cada carpeta del compilador y por qué `stdint.h` lo da GCC y `stdio.h` lo da newlib.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [06 - Funciones](./06-funciones.md) ·
**Siguiente:** [08 - Punteros](./08-punteros.md)
