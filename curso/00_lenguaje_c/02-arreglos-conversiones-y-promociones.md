# Arreglos, conversiones y promociones enteras

Segunda mitad del capítulo de tipos. En [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md)
vimos **cómo se declara** una variable y **qué tipos** hay. Acá vemos qué pasa cuando esos tipos se
juntan: arreglos (varios datos del mismo tipo, uno al lado del otro) y las **conversiones** que el
compilador hace por su cuenta cuando mezclás tipos en una cuenta.

La segunda mitad de este capítulo (promociones enteras, `signed` vs `unsigned`) es la fuente de
errores sutiles más común cuando se programa un micro. No hace falta memorizarla la primera vez que
la leés, pero sí volver acá cuando un cálculo con `uint8_t` te dé un resultado que no cierra.

---

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
> **Un arreglo local SIN inicializar NO arranca en cero.** `int arr[5];` dentro de una función te da 5 valores *indeterminados*, y leerlos antes de escribirlos es **comportamiento indefinido**, no "te sale lo que había en el stack" (el porqué de esa distinción está en [01 - Declaraciones](./01-declaraciones-y-tipos.md#2-especificador-de-almacenamiento)). Solo los arreglos con duración estática (globales o `static`) arrancan en cero, porque el código de arranque limpia la sección `.bss`. Es un error clásico: funciona en el escritorio por casualidad y falla en el micro.

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
size_t tamano_bytes = sizeof(arr);      // 40 bytes (10 * 4)
size_t cantidad = sizeof(arr) / sizeof(arr[0]);  // 10 elementos
```

> Este truco solo funciona cuando el arreglo está en el mismo scope. Si pasás el arreglo a una función, `sizeof` devolverá el tamaño del puntero, no del arreglo.

---

### Arreglos multidimensionales

Igual que antes, las dos inicializaciones son **alternativas**, no dos líneas seguidas:

```c
// Matriz 3x3, por filas
int matriz[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

int valor = matriz[1][2];  // 6 (fila 1, columna 2)

// Inicialización lineal: exactamente la misma matriz
int matriz[3][3] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
```

> **Usá siempre la forma por filas.** La lineal no solo es más frágil (si te olvidás un elemento,
> corre todo lo que sigue una posición y la matriz queda distinta sin que nada falle), sino que el
> propio compilador la desaconseja: `-Wmissing-braces` está **dentro de `-Wall`**, así que con las
> flags que recomienda este curso la segunda forma no compila limpio.
>
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -c matriz.c
> warning: missing braces around initializer [-Wmissing-braces]
> ```

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

Como se verá en [08 - Punteros](./08-punteros.md) y [09 - Punteros avanzados](./09-punteros-avanzado.md), en **casi todos** los contextos el nombre de un arreglo se **convierte automáticamente** ("decae", *array decay*) en un puntero a su primer elemento:

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
> // 1) operando de sizeof
> sizeof(arr);        // 20 → el arreglo entero. sizeof(ptr) daría 4
>
> // 2) operando de & (y de _Alignof)
> &arr;               // tipo int(*)[5] → puntero a arreglo de 5. &ptr es int**
>
> // 3) literal de cadena que inicializa un arreglo
> char s[] = "Hola";  // copia los 5 bytes en s; NO apunta al literal
> ```
>
> Otra diferencia concreta: un puntero es una **variable** que ocupa sus 4 bytes en memoria y podés reasignar (`ptr = otro;`). El nombre del arreglo no es una variable reasignable: `arr = otro;` no compila. De ahí viene la analogía con "puntero constante", pero el arreglo además **no ocupa memoria propia** para guardar la dirección: la dirección *es* dónde está el arreglo.

---

### Uso en sistemas embebidos

> [!NOTE]
> **Acá aparecen `*` y `&` por primera vez.** Son los operadores de puntero, y el capítulo que los
> explica en serio es [08 - Punteros](./08-punteros.md). Para leer el ejemplo alcanza con dos ideas:
> `uint32_t *p` declara una variable que **guarda una dirección** de un `uint32_t` en vez de un
> número, y `*p` significa **"el valor que hay en esa dirección"**. Es decir que `*gpio_set[0] = ...`
> escribe *en el registro del micro* cuya dirección está guardada en `gpio_set[0]`. Si te resulta
> denso, saltá el bloque y volvé después del capítulo 08: no te perdés nada de arreglos.

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

Cuando pasás un arreglo a una función, en realidad se pasa un **puntero**:

```c
#include <stdio.h>
#include <stddef.h>

// `const int arr[]` acá es EXACTAMENTE lo mismo que `const int *arr`
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

> Por eso es necesario pasar el tamaño como parámetro separado: **dentro de `procesar()`, `sizeof(arr)` da 4** (el tamaño de un puntero en el M3), no 20. El `int arr[]` en la lista de parámetros es puramente decorativo; el compilador lo reescribe a `int *arr`. Incluso si escribís `int arr[5]`, **el 5 no forma parte del tipo** y podés pasarle un arreglo de otro tamaño.

> *Cuánto te ayuda el compilador acá depende de su versión.* Con el `arm-none-eabi-gcc` 9 del repo, pasarle un `int chico[2]` a un parámetro `int arr[5]` pasa **sin un solo warning**. Los GCC modernos (11 en adelante) sí lo miran: usan el tamaño escrito como una promesa y avisan si la llamada no la cumple.
>
> ```console
> $ gcc-13 -Wall -c parametros.c
> warning: 'g' accessing 20 bytes in a region of size 8 [-Wstringop-overflow=]
> ```
>
> No te confíes igual: el chequeo solo funciona cuando el compilador ve el tamaño del arreglo original en el mismo archivo, y desaparece por completo si escribís `int arr[]` sin número. **Pasar el tamaño como parámetro aparte sigue siendo la única forma robusta.**

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

### Posibles errores


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

Si esperabas el complemento del byte alto de un valor de 16 bits (`~0x12 = 0xED`)... acá tuviste suerte y dio lo mismo, pero el camino fue por 32 bits **con signo**. Y la suerte se acaba en cuanto usás el valor sin guardarlo primero en un `uint8_t`:

```c
if ((~valor >> 8) == 0xED)             // FALSO: comparás 0xFFFFFFED contra 0xED
if ((uint8_t)(~valor >> 8) == 0xED)    // verdadero
```

**Regla:** cuando uses `~` sobre tipos chicos, enmascará explícitamente el resultado al ancho que querés:

```c
reg = reg & (uint8_t)~mask;          // forzás 8 bits
byte_alto = (uint8_t)(~valor >> 8);  // el cast no cambia el número que se guarda,
                                     // pero deja el ancho escrito en el código
```

Ese último cast es la clave del asunto: **no arregla un valor mal calculado**, hace explícito el ancho al que querés truncar, y por eso sigue valiendo lo mismo si mañana movés la expresión a un `if` o a una comparación, que es justo donde el problema aparece.

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

## `signed` vs `unsigned`: bugs clásicos

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
>
> ```console
> $ gcc -Wall -c bucle.c          # silencio total
> $ gcc -Wall -Wextra -c bucle.c
> warning: comparison is always true due to limited range of data type [-Wtype-limits]
> ```
>
> Otra razón para no compilar nunca sin `-Wextra`.

### Comparaciones mixtas signed/unsigned

Si comparás un `signed` con un `unsigned`, C convierte **el operando con signo a sin signo** (regla de conversiones aritméticas usuales). Esto da resultados absurdos:

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


| Tipo       | Qué pasa al desbordar                                                                                                                                                        |
| ---------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `unsigned` | Da la vuelta de forma **definida**: aritmética módulo 2^N. `0xFF + 1 == 0x00` en `uint8_t`. Esto está **garantizado**.                                                       |
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
>
> - **Promoción entera (integer promotion):** todo tipo entero de rango menor que `int` se convierte a `int` (o a `unsigned int` si `int` no puede representar todos sus valores). En el M3, `uint16_t` cabe en `int`, así que se promociona a `int` (con signo), no a `unsigned`.
> - **Conversiones aritméticas usuales (usual arithmetic conversions):** cuando los dos operandos son de tipos distintos tras la promoción, se llevan a un "tipo común" siguiendo un ranking (`int` < `unsigned int` < `long` < ...). Si uno es `unsigned` y tiene rango mayor o igual, el otro se convierte a `unsigned`. De ahí sale el bug de comparar `int` con `unsigned int`.
> - **Truncamiento:** al convertir a un tipo entero **sin signo** más chico, se conservan los bits de menor orden (módulo 2^N) y está **garantizado**. Para destino **con signo** y un valor fuera de rango, en C99/C11/C17 el resultado es *definido por la implementación* (en GCC/ARM, complemento a dos sin sorpresas); **en C23 pasó a estar definido** como módulo 2^N, porque C23 obliga a que los enteros con signo sean complemento a dos y eliminó los formatos exóticos (complemento a uno, signo-magnitud).
> - El cast explícito **no** elimina estas reglas; solo te deja controlar **cuándo** ocurre la conversión (y le dice al compilador y al lector que la pérdida es intencional, lo que además calla el warning).
> - **`sizeof` no evalúa su operando.** `sizeof(i++)` no incrementa `i`: el compilador solo mira el *tipo*. Es un operador de tiempo de compilación (salvo con VLAs), y su resultado es de tipo `size_t`.

---

## Resumen de reglas para no equivocarse

Si te llevás solo una cosa de este capítulo, que sea esta lista:


| Regla                                                     | Por qué                                                                |
| --------------------------------------------------------- | ---------------------------------------------------------------------- |
| Pasá siempre el tamaño del arreglo como parámetro aparte  | Dentro de la función el arreglo ya es un puntero y `sizeof` da 4       |
| No asumas que un arreglo local arranca en cero            | Solo los estáticos se limpian (`.bss`); el local trae basura del stack |
| Cast explícito al ancho deseado en máscaras, shifts y `~` | Todo se calcula en `int` de 32 bits por promoción entera               |
| Desplazá a la derecha solo tipos `unsigned`               | `>>` de un negativo es definido por la implementación                  |
| No mezcles `signed` con `unsigned` en comparaciones       | El `signed` se convierte a `unsigned` y `-1 < 1u` es falso             |
| `unsigned` para contadores que dan la vuelta              | El wraparound `unsigned` está definido; el `signed` es UB              |
| Escribí el sufijo `f` en las constantes de `float`        | Sin él son `double` y arrastran toda la cuenta a 64 bits por software  |
| Compilá con `-Wall -Wextra`                               | Varios bugs de este capítulo **solo** los ve `-Wextra`                 |


---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.3.1.1 (promoción entera y conversiones aritméticas usuales), 6.3.1.3 (conversión entre enteros), 6.3.1.4 (de flotante a entero), 6.5.2.1 (indexado de arreglos), 6.7.9 (inicializadores).
- [cppreference: C language](https://en.cppreference.com/w/c/language). La referencia práctica más clara, con los cambios por versión del estándar.

**GCC y el toolchain**

- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). Confirma que `-Wsign-compare` está en `-Wextra` para C (y en `-Wall` solo para C++), y que `-Wtype-limits` está en `-Wextra`.
- [GCC: Implementation-defined behavior](https://gcc.gnu.org/onlinedocs/gcc/C-Implementation.html). Qué elige GCC donde el estándar deja libertad (`>>` de negativos, conversión a un entero con signo fuera de rango).

**ARM y el LPC1769**

- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf). Está en el repo. Capítulo 9 (GPIO), tabla 106, para las direcciones `FIOxSET` del arreglo de punteros del ejemplo.

**Sobre los temas puntuales**

- [Why Not Mix Signed and Unsigned Values in C/C++? (John Regehr)](https://blog.regehr.org/archives/268). Sobre el bug de comparaciones mixtas.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md) ·
**Siguiente:** [03 - Operadores](./03-operadores.md)