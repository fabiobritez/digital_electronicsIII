
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

---

## 3. `#undef`: Elimina una macro

```c
#include <driver_x.h> // incluye una librería que define una macro MAX
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b)) // reemplaza la macro MAX por nuestra propia macro
```

* Útil si incluiste algo que define una macro que quieres reemplazar. Por ejemplo, en un programa que usa una librería que define una macro `MAX` y queremos reemplazarla por nuestra propia macro.

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

 