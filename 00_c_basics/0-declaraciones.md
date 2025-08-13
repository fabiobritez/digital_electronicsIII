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

 



