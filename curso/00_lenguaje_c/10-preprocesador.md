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

Una función `inline` (cap. 11) no tiene este problema: evalúa cada argumento una sola vez. Por eso,
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

`##` es el motor que hace posibles las **X-macros**, que vemos en seguida.

### Include guards vs `#pragma once`

Ya viste el **include guard** (sección 4): el par `#ifndef`/`#define`/`#endif` que evita que un
header se incluya dos veces (lo que daría errores de "redefinición"). Hay una alternativa más corta:

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

## X-macros: generar tablas y enums desde una sola lista

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
 
## Utilidad del preprocesador en embebidos

* **Configuración por hardware:** se define qué pines, módulos, o versiones se usan.
* **Compilar para distintos dispositivos sin cambiar el código base.**
* **Control preciso del código incluido**, esencial para ahorrar memoria y ciclos.
* **Facilita reutilizar drivers entre proyectos**.

---

**Anterior:** [09 - Asignación dinámica](./09-asignacion-dinamica.md) ·
**Siguiente:** [11 - static, const, inline y campos de bits](./11-static-const-inline-y-bitfields.md)
