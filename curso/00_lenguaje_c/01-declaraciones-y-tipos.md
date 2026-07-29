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
| `float`        | Punto flotante, precisión estándar       |
| `double`       | Punto flotante, precisión doble          |
| `long double`  | Punto flotante, precisión extendida      |


#### Ejemplo:

```c
char letra;
float temperatura;

void copiar(int from, int to) {
    to = from;
}
```
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
* Las variables dentro de funciones son `auto` por defecto.
* Las variables fuera de funciones son `static` por defecto. 

---
 
 

| Especificador | Ubicación de almacenamiento             | Tiempo de vida                  | Alcance (scope)                                                         | Valor inicial              | Comentarios clave                               |
| ------------- | --------------------------------------- | ------------------------------- | ---------------------------------------------------------------------- | -------------------------- | ----------------------------------------------- |
| `auto`        | Memoria RAM (automática)                | Mientras la función esté activa | Local a la función                                                     | Indefinido                 | Valor no se conserva entre llamadas             |
| `static`      | Memoria RAM   | Todo el programa                | Local (si está dentro de función) o interna al archivo (si está fuera) | Cero (si no se inicializa) | Mantiene su valor entre llamadas                |
| `register`    | Registro de CPU (si está disponible)    | Mientras la función esté activa | Local a la función                                                     | Indefinido                 | Más rápido (teóricamente); no se puede usar `&` |
| `extern`      | Memoria RAM (definida en otro archivo)  | Todo el programa                | Global (visible en otros archivos)                                     | Cero (si no se inicializa) | Se usa para compartir variables entre archivos  |

---

 

> [!NOTE]
> [What are storage class specifiers in C?](https://how.dev/answers/what-are-storage-class-specifiers-in-c)

---

### 3. **Calificador de tipo**

Afectan cómo el compilador **trata el contenido de la variable**.

| Calificador | ¿Qué hace?                                               | Ejemplo embebido             |
| ----------- | -------------------------------------------------------- | ---------------------------- |
| `const`     | El valor no se puede modificar desde el código           | `const float PI = 3.14;`     |
| `volatile`  | Puede cambiar fuera del programa (por hardware)          | `volatile uint8_t *PORTA;` |
| `restrict`  | La única forma de acceder a ese dato es mediante un puntero (C99) | `void f(int * restrict a);`  |

> En sistemas embebidos, `volatile` es **crítico** para registros de periféricos.

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


---


### 4. **Modificadores del tipo**

Modifican el tipo base en cuanto a **tamaño o signo**:

| Modificador          | Qué afecta                     |
| -------------------- | ------------------------------ |
| `short`, `long`      | Cambia el tamaño del entero    |
| `signed`, `unsigned` | Permite o no números negativos |

> Combinables: `unsigned long int`, `short int`, etc.

#### Ejemplo:

```c
unsigned char codigo; // 0-255
signed short int codigo; // -32768-32767
long double resultado; // double extendido
```



---

### 5. **Nombre de variables**

- Compuestos por letras, dígitos y `_`.
- El primer carácter **debe ser una letra** (o `_`, aunque no se recomienda).
- **Mayúsculas y minúsculas son distintas** (`a` ≠ `A`).
- Convención:
  - minúsculas para variables
  - MAYÚSCULAS para constantes simbólicas
- Evitar nombres confusos o con `_` inicial (estos están reservados a librerías).
- Mínimo: los primeros 31 caracteres deben ser significativos (al menos), es decir, tener alguna diferencia con otros nombres.   
- Las **palabras clave** (`if`, `int`, `float`, etc.) están reservadas y **no se pueden usar como nombres**.

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

Un puntero constante a un registro de un puerto de un microcontrolador. No puede cambiar de dirección, pero si puede cambiar su valor por hardware.

```c
static const volatile unsigned int * const GPIO_PORT = (unsigned int *) 0x40021000;

uint32_t estado = *GPIO_PORT;  // Leer el valor del registro GPIO_PORT

```
 Desglose de la declaración anterior:

| Parte          | Función                                                            |
| -------------- | ------------------------------------------------------------------ |
| `static`       | Solo visible en este archivo                                       |
| `const`        | No se puede cambiar desde el código                                |
| `volatile`     | El valor puede cambiar por hardware                                |
| `unsigned int` | Tipo base, entero sin signo                                        |
| `* const`      | Puntero constante (no puede cambiar de dirección)                  |
| `GPIO_PORT`    | Nombre de la variable                                              |
| `= ...`        | Se inicializa apuntando a una dirección fija de memoria |
 

## Tamaños de los tipos

- Los tamaños exactos de los tipos **dependen del compilador y hardware**.
- Garantías mínimas del lenguaje:
  - `short` y `int`: al menos 16 bits
  - `long`: al menos 32 bits
  - `short <= int <= long`

Relaciones entre los tipos:
```c 
sizeof(char)    = 1              // siempre 1 byte
sizeof(short)  <= sizeof(int)
sizeof(int)    <= sizeof(long)
sizeof(float)  <= sizeof(double)
sizeof(double) <= sizeof(long double)
```

### Tamaño REAL de los tipos en el Cortex-M3 (LPC1769)

El estándar solo da garantías mínimas, pero a vos te interesa qué pasa **en tu micro**. Con `arm-none-eabi-gcc` sobre el LPC1769 (ABI estándar de ARM, AAPCS) los tamaños son:

| Tipo          | Tamaño en el M3 | Rango                                   |
|---------------|-----------------|------------------------------------------|
| `char`        | 8 bits          | depende del signo (ver abajo)           |
| `short`       | 16 bits         | -32 768 a 32 767                         |
| `int`         | **32 bits**     | -2 147 483 648 a 2 147 483 647           |
| `long`        | 32 bits         | igual que `int`                          |
| `long long`   | 64 bits         | enorme                                   |
| `float`       | 32 bits         | precisión simple (IEEE-754)              |
| `double`      | 64 bits         | precisión doble                          |
| puntero (`*`) | 32 bits         | el M3 tiene un espacio de direcciones de 32 bits |

Dos cosas importantes:

1. En el Cortex-M3 `int` es de **32 bits**, igual que `long` y que un puntero. El M3 es una máquina de 32 bits, así que operar con `int` es lo más natural y rápido para la ALU. Operar con `uint8_t` o `uint16_t` a veces obliga al compilador a **enmascarar** después de cada cuenta para que el resultado "entre" en 8 o 16 bits.
2. El M3 **no tiene unidad de punto flotante** (el LPC1769 es Cortex-M3, no M4F). Cada `float`/`double` se calcula por **software**, lo cual es lento. Evitá flotantes en código crítico; usá aritmética entera o de punto fijo cuando puedas.

> **`char` puede ser con o sin signo.** El estándar deja librado al compilador si `char` "pelado" es `signed` o `unsigned`. En `arm-none-eabi-gcc` el `char` por defecto es **unsigned** (0 a 255). Esto importa: `char c = 0x80; if (c < 0)` puede dar distinto según el compilador. **Moraleja:** nunca uses `char` para hacer aritmética con signo; usá `signed char` o `int8_t` si querés signo, y `uint8_t` si querés un byte sin signo.

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

## Formas de escribir enteros en C (octales, hexadecimales, binarios)

En C, los **números enteros** pueden escribirse en **diferentes bases** usando prefijos:

| Forma         | Ejemplo       | Base | Prefijo   | Notas                           |
|---------------|---------------|------|-----------|---------------------------------|
| Decimal       | `123`         | 10   | (ninguno) | La forma más común              |
| Octal         | `0123`        | 8    | `0`       | Solo dígitos `0` a `7`          |
| Hexadecimal   | `0x7B`        | 16   | `0x`/`0X` | Dígitos `0-9`, letras `a-f`     |
| Binario*      | `0b01111011`  | 2    | `0b`/`0B` | *No es estándar en C99, pero lo aceptan GCC/Clang*

---

### Sufijos para modificar el tipo

Podés agregar sufijos para cambiar el tipo del literal:

- `U` o `u`: unsigned
- `L` o `l`: long
- `UL`, `LU`, etc.: combinaciones válidas

**Ejemplo:**
```c
0xFF       // int
0xFFU      // unsigned int
0123L      // long (octal)
123UL      // unsigned long
```

---

### Notas importantes

- El prefijo `0` en un número **lo convierte en octal**, no es lo mismo `012` que `12`
- **Binarios (`0b...`) no son estándar**, pero podés usarlos con GCC.

# Constantes en C (también llamadas literales)

### Constantes enteras (integer literals)

- Por defecto, un número como `1234` es `int`.
- Se puede agregar un sufijo:
  - `L` o `l`: long → `123456789L`
  - `UL` o `ul`: unsigned long
- Si el número es muy grande para `int`, se considera automáticamente `long`.

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
- Son enteros que representan el valor numérico del carácter (por ejemplo, `'0'` es 48 en ASCII).
- Participan en operaciones como cualquier `int`.
- No confundir `'x'` (carácter) con `"x"` (cadena), pues el ultimo es un array de caracteres (se agregan `\0` al final).


### Constantes de cadena (string literals)

* Secuencia entre comillas dobles: `"Hola mundo"`
* `"Hola," "mundo"` se concatena automáticamente como `"Hola, mundo"`
* Siempre terminan con `'\0'` (carácter nulo).
* Técnicamente, son **arreglos de caracteres**.
* La función `strlen()` devuelve la longitud (sin contar el `'\0'`).

```c
int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0')
        i++;
    return i;
}
```

---

### Constantes de enumeración (`enum`)

* Lista de identificadores con valores enteros constantes:

```c
enum boolean { NO, YES };      // NO = 0, YES = 1
enum months { ENE = 1, FEB, MAR };  // FEB = 2, MAR = 3
enum escapes { BELL = '\a', BACKSPACE = '\b', TAB = '\t' , NEWLINE = '\n', RETURN = '\r'};
```

* Si no se da valor explícito, continúan desde el anterior.
* Se usan como alternativa a `#define`.
* Los `enum` hacen que el compilador pueda verificar su uso, lo cual es más seguro.
* Solo pueden ser enteros de tipo `int` (o sus variantes como `char`, `short`, `long`, etc.).

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
| `\ooo` | valor octal            |
| `\xhh` | valor hexadecimal      | 



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

Como se verá en las secciones de punteros, **el nombre de un arreglo se comporta como un puntero constante** a su primer elemento:

```c
int arr[5] = {1, 2, 3, 4, 5};
int *ptr = arr;  // equivalente a &arr[0]

// Estas expresiones son equivalentes:
arr[2]  ==  *(arr + 2)  ==  ptr[2]  ==  *(ptr + 2)
```

---

### Uso en sistemas embebidos

```c
// Buffer para UART
uint8_t rx_buffer[256];

// Tabla de lookup para conversión
const uint16_t adc_to_temp[256] = { /* ... */ };

// Array de registros
volatile uint32_t gpio_ports[4] = {
    0x40020000,
    0x40020400,
    0x40020800,
    0x40020C00
};
```

---

### Limitaciones importantes

1. **Tamaño fijo**: una vez declarado, no se puede cambiar el tamaño
2. **No se puede asignar directamente**: `arr1 = arr2;` es **inválido**
3. **No se puede retornar desde función**: retornar un arreglo local es **comportamiento indefinido**
4. **Sin verificación de límites**: accesos fuera de rango no generan error

---

### Pasar arreglos a funciones

Cuando pasas un arreglo a una función, en realidad se pasa un **puntero**:

```c
void procesar(int arr[], int tamaño) {
    for (int i = 0; i < tamaño; i++) {
        printf("%d ", arr[i]);
    }
}

int main() {
    int datos[5] = {1, 2, 3, 4, 5};
    procesar(datos, 5);
    return 0;
}
```

> Por eso es necesario pasar el tamaño como parámetro separado.

---

## Conversión de tipos

- Se puede convertir un tipo a otro usando **operadores de conversión** o **funciones de conversión**. Se puede hacer de forma implícita (automática) o explícita(manual).
- Ejemplo:
  ```c
  int x = 10; 
  float y = 3.14;
  int z = x + y;  // conversión implícita a float
  int w = (int) y;  // conversión explícita a int
  ```

Reglas generales: 
- C promociona tipos más pequeños a más grandes. Ej: Los int se convierten a float si hay un float en la operación.
- Los char y short se convierten a int antes de operar.
- Se puede convertir un tipo a uno más pequeño manualmente, pero se puede perder información (truncamiento).  


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

`valor` se promociona a `int`: `0x00001234`. `~` da `0xFFFFEDCB`. El `>> 8` da `0x00FFFFED`, y al asignarlo a `uint8_t byte_alto` se trunca a `0xED`. Si esperabas el complemento del byte alto de un valor de 16 bits (`~0x12 = 0xED`)... acá tuviste suerte y dio lo mismo, pero el camino fue por 32 bits. Cambiá el tipo o las máscaras y el resultado se te escapa. **Regla:** cuando uses `~` sobre tipos chicos, enmascará explícitamente el resultado al ancho que querés:

```c
reg = reg & (uint8_t)~mask;          // forzás 8 bits
byte_alto = (uint8_t)(~valor >> 8);  // queda claro y correcto
```

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

### Comparaciones mixtas signed/unsigned

Si comparás un `signed` con un `unsigned`, C convierte **el con signo a sin signo** (regla de conversiones aritméticas usuales). Esto da resultados absurdos:

```c
int a = -1;
unsigned int b = 1;
if (a < b) {
    // NO entra: -1 se convierte a 0xFFFFFFFF (4294967295), que NO es < 1
}
```

Compilá con `-Wsign-compare` (incluido en `-Wall`) y el compilador te avisa de estas comparaciones. **Regla práctica:** no mezcles signo en comparaciones; elegí un signo y mantenelo.

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
> - **Truncamiento:** al convertir a un tipo entero más chico, se conservan los bits de menor orden (módulo 2^N). Para destino `signed` con un valor fuera de rango, el resultado es definido por la implementación (en GCC/ARM, complemento a dos sin sorpresas).
> - El cast explícito **no** elimina estas reglas; solo te deja controlar **cuándo** ocurre la conversión.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Siguiente:** [02 - Operadores](./02-operadores.md)
