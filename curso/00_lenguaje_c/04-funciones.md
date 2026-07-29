# Funciones en C

Las funciones son **bloques de código reutilizables** que realizan una tarea específica. Son fundamentales para:

* Organizar y estructurar el código
* Evitar repetición (principio DRY: Don't Repeat Yourself)
* Facilitar el mantenimiento y depuración
* Crear abstracciones y modularizar el programa

---

## Anatomía de una función

```c
tipo_retorno nombre_funcion(tipo_param1 param1, tipo_param2 param2) {
    // cuerpo de la función
    return valor;  // opcional según el tipo de retorno
}
```

### Componentes:

1. **Tipo de retorno**: tipo de dato que devuelve la función (o `void` si no retorna nada)
2. **Nombre**: identificador de la función
3. **Parámetros**: lista de variables que recibe (pueden ser cero o más)
4. **Cuerpo**: código que se ejecuta cuando se llama la función
5. **`return`**: devuelve un valor al llamador (obligatorio si el tipo no es `void`)

---

## Ejemplo básico

```c
int sumar(int a, int b) {
    int resultado = a + b;
    return resultado;
}

int main() {
    int x = 5, y = 3;
    int suma = sumar(x, y);  // llamada a la función
    printf("Suma: %d\n", suma);  // imprime 8
    return 0;
}
```

---

## Declaración vs Definición

### Declaración (prototipo)

Indica al compilador que la función existe, pero no proporciona el código:

```c
int sumar(int a, int b);  // declaración/prototipo
```

### Definición

Proporciona el código real de la función:

```c
int sumar(int a, int b) {   // definición
    return a + b;
}
```

### ¿Por qué usar prototipos?

Permiten usar funciones antes de definirlas:

```c
// Prototipos al inicio
int sumar(int a, int b);
int restar(int a, int b);

int main() {
    int x = sumar(5, 3);
    int y = restar(10, 4);
    return 0;
}

// Definiciones después
int sumar(int a, int b) {
    return a + b;
}

int restar(int a, int b) {
    return a - b;
}
```

> En proyectos grandes, los prototipos van en archivos `.h` (headers) y las definiciones en `.c`.

### Por qué los prototipos importan de verdad (no es solo orden)

Un prototipo le dice al compilador **cuántos parámetros** lleva la función y **de qué tipo** son, además del tipo de retorno. Con esa información, el compilador puede:

- **Verificar que la llamés bien.** Si pasás un `float` donde se esperaba un `int*`, te lo marca como error en vez de generar código roto.
- **Convertir los argumentos correctamente.** Sin prototipo, no sabe a qué tipo convertir lo que le pasás.

¿Qué pasa si llamás una función **sin** haberla declarado antes? En C89 el compilador asumía un "prototipo implícito" que devolvía `int` y aceptaba cualquier cosa, una fuente enorme de bugs silenciosos. En C99 en adelante esto es directamente un **error**. Ejemplo clásico que falla feo sin prototipo:

```c
// Sin incluir <math.h> ni declarar sqrt:
double r = sqrt(2.0);   // sin prototipo, el compilador podría asumir que sqrt
                        // devuelve int → te da basura, no 1.414...
```

**Por eso siempre incluís el header correspondiente** (`#include <math.h>`, `#include "sensor.h"`) antes de usar una función. Compilá con `-Wall -Werror-implicit-function-declaration` (o simplemente `-Werror`) para que cualquier llamada sin prototipo sea un error y no una sorpresa en runtime.

---

## Funciones sin retorno (`void`)

```c
void imprimir_mensaje(void) {
    printf("Hola mundo\n");
    // no hay return (o "return;" sin valor)
}

void saludar(char nombre[]) {
    printf("Hola, %s!\n", nombre);
}

int main() {
    imprimir_mensaje();
    saludar("Juan");
    return 0;
}
```

---

## Funciones sin parámetros

```c
int obtener_numero_aleatorio(void) {
    return 42;  // número "aleatorio" :)
}

int main() {
    int num = obtener_numero_aleatorio();
    printf("Número: %d\n", num);
    return 0;
}
```

> En C moderno se recomienda usar `void` explícitamente: `func(void)` en lugar de `func()`.

---

## Paso de parámetros

En C, los parámetros se pasan **por valor** por defecto. Esto significa que se hace una **copia** del argumento.

### Paso por valor

```c
void incrementar(int x) {
    x = x + 1;
    printf("Dentro: %d\n", x);  // 11
}

int main() {
    int a = 10;
    incrementar(a);
    printf("Fuera: %d\n", a);   // 10 (no cambió)
    return 0;
}
```

La función recibe una **copia** de `a`, por lo que modificar `x` no afecta a `a`.

---

### Paso por referencia (simulado con punteros)

Para modificar el valor original, se pasa un **puntero**:

```c
void incrementar(int *x) {
    *x = *x + 1;
    printf("Dentro: %d\n", *x);  // 11
}

int main() {
    int a = 10;
    incrementar(&a);  // pasar dirección de a
    printf("Fuera: %d\n", a);    // 11 (cambió!)
    return 0;
}
```

Ahora la función puede modificar el valor original a través del puntero.

> **En C no existe el "paso por referencia" de verdad.** Lo que ves arriba sigue siendo paso por valor: lo que se copia es el **puntero** (la dirección). La función recibe una copia de esa dirección y, a través de ella, llega a la variable original. Es paso por valor de un puntero. La diferencia con C++ (que sí tiene referencias `&`) es importante: en C, si querés modificar algo del llamador, **siempre** es vía puntero explícito (`&` al pasar, `*` para acceder).

---

## Retornar valores

### Retornar tipos básicos

```c
int maximo(int a, int b) {
    return (a > b) ? a : b;
}

float calcular_promedio(float a, float b) {
    return (a + b) / 2.0;
}
```

---

### Retornar punteros

```c
char *obtener_saludo(void) {
    static char mensaje[] = "Hola!";  // IMPORTANTE: static
    return mensaje;
}

int main() {
    char *msg = obtener_saludo();
    printf("%s\n", msg);
    return 0;
}
```

> **NUNCA** retornes un puntero a una variable local (no estática):

```c
// INCORRECTO
char *obtener_saludo_malo(void) {
    char mensaje[] = "Hola!";  // local (se destruye al salir)
    return mensaje;  // ¡PELIGRO! puntero a memoria no válida
}
```

---

### Retornar estructuras

```c
typedef struct {
    int x;
    int y;
} Punto;

Punto crear_punto(int x, int y) {
    Punto p = {x, y};
    return p;  // se copia la estructura
}

int main() {
    Punto p1 = crear_punto(10, 20);
    printf("Punto: (%d, %d)\n", p1.x, p1.y);
    return 0;
}
```

---

## Funciones con arreglos

Los arreglos se pasan **por referencia** (en realidad, se pasa un puntero al primer elemento):

```c
void imprimir_arreglo(int arr[], int tamaño) {
    for (int i = 0; i < tamaño; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void llenar_con_ceros(int arr[], int tamaño) {
    for (int i = 0; i < tamaño; i++) {
        arr[i] = 0;  // modifica el arreglo original
    }
}

int main() {
    int numeros[5] = {1, 2, 3, 4, 5};
    imprimir_arreglo(numeros, 5);

    llenar_con_ceros(numeros, 5);
    imprimir_arreglo(numeros, 5);  // 0 0 0 0 0
    return 0;
}
```

> Siempre se debe pasar el tamaño como parámetro separado, ya que la función no puede determinarlo.

### Por qué el arreglo "decae" a puntero

Cuando pasás un arreglo a una función, **no se copia el arreglo entero**: lo que se pasa es la dirección de su primer elemento. Se dice que el arreglo **decae (decays) a un puntero**. Por eso estos tres prototipos son **idénticos** para el compilador:

```c
void f(int arr[10]);   // el "10" se ignora por completo
void f(int arr[]);     // exactamente lo mismo
void f(int *arr);      // ...y esto también
```

Consecuencias prácticas que tenés que tener clarísimas en embebido:

1. **Dentro de la función, `sizeof(arr)` te da el tamaño de un puntero (4 bytes en el M3), NO el del arreglo.** El truco `sizeof(arr)/sizeof(arr[0])` para contar elementos **solo funciona en el scope donde se declaró el arreglo**, nunca dentro de una función que lo recibió. Por eso se pasa el tamaño aparte.

   ```c
   void procesar(uint8_t buf[]) {
       size_t n = sizeof(buf);   // ¡4, no el tamaño del buffer! BUG clásico
   }
   ```

2. Como la función recibe la dirección real, **puede modificar el contenido del arreglo del llamador** (no una copia). Eso es lo que aprovecha `llenar_con_ceros` más arriba.

---

## Funciones recursivas

Una función puede llamarse a sí misma:

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;  // caso base
    }
    return n * factorial(n - 1);  // llamada recursiva
}

int main() {
    printf("5! = %d\n", factorial(5));  // 120
    return 0;
}
```

### El costo de la recursión en la pila (crítico en embebido)

Cada llamada a función reserva un **marco de pila (stack frame)**: espacio para sus parámetros, variables locales y la dirección de retorno. En recursión, esos marcos se **apilan**: `factorial(5)` tiene 5 marcos vivos al mismo tiempo.

En una PC con gigabytes de RAM eso no preocupa. En el **LPC1769 tenés 64 KB de RAM en total**, y la pila es apenas una porción de eso (a veces unos pocos KB, definidos en el linker script). Si la recursión es profunda o el caso base falla, la pila **crece hasta pisar otras zonas de memoria** (stack overflow): el síntoma típico es que el micro se cuelga, se reinicia, o corrompe variables aparentemente al azar. Y no hay un sistema operativo que te avise: simplemente se rompe.

Por eso, en firmware:

- **Evitá la recursión** salvo que la profundidad esté acotada y sea chica y conocida.
- Cualquier recursión se puede reescribir como un **bucle** con una pila/array explícitos. El factorial, por ejemplo, es trivial de forma iterativa:

  ```c
  uint32_t factorial(uint32_t n) {
      uint32_t r = 1;
      for (uint32_t i = 2; i <= n; i++) {
          r *= i;
      }
      return r;
  }
  ```

  La versión iterativa usa **un solo** marco de pila sin importar `n`.

> **Ojo:** en sistemas embebidos, evitá recursión profunda: consume mucha pila y puede causar overflow. Cuando puedas, preferí la versión iterativa.

---

## Valores de retorno y códigos de error

En C no hay excepciones como en otros lenguajes. La forma idiomática de reportar errores es **mediante el valor de retorno**. Hay dos convenciones muy comunes en firmware:

### Convención 1: la función devuelve un código de estado

La función retorna un `int` (o un `enum`) que indica éxito o el tipo de error, y los datos "útiles" salen por punteros de salida:

```c
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR_TIMEOUT,
    SENSOR_ERR_CRC,
} sensor_status_t;

// El resultado sale por 'out'; el return informa si salió bien
sensor_status_t Sensor_Leer(uint16_t *out) {
    if (!dato_listo())      return SENSOR_ERR_TIMEOUT;
    uint16_t v = leer_raw();
    if (!crc_ok(v))         return SENSOR_ERR_CRC;
    *out = v;
    return SENSOR_OK;
}

// Uso
uint16_t valor;
if (Sensor_Leer(&valor) != SENSOR_OK) {
    // manejar el error
}
```

Usar `0` para "éxito" es la convención más extendida (permite escribir `if (funcion() != 0)` para "hubo error").

### Convención 2: valor válido + un valor "centinela" para el error

Cuando todos los valores válidos dejan libre alguno (típicamente negativos), se devuelve ese valor centinela para señalar error. La función `encontrar` del capítulo de control de flujo hace esto: devuelve el índice si lo encuentra, o `-1` si no:

```c
int encontrar(int arr[], int size, int valor) {
    for (int i = 0; i < size; i++)
        if (arr[i] == valor) return i;
    return -1;   // centinela: "no encontrado"
}
```

Es compacto, pero solo sirve si tenés un valor que **nunca** es un resultado legítimo. Si todos los valores posibles son válidos, usá la Convención 1.

> **Regla:** elegí una convención por módulo y mantenela. Y **siempre chequeá** el código de retorno: ignorar el error de una función de hardware es una de las causas más comunes de bugs intermitentes en firmware.

---

## Funciones con número variable de argumentos

C permite funciones con argumentos variables usando `<stdarg.h>`:

```c
#include <stdarg.h>

int sumar_varios(int cantidad, ...) {
    va_list args;
    va_start(args, cantidad);

    int suma = 0;
    for (int i = 0; i < cantidad; i++) {
        suma += va_arg(args, int);
    }

    va_end(args);
    return suma;
}

int main() {
    printf("%d\n", sumar_varios(3, 10, 20, 30));  // 60
    printf("%d\n", sumar_varios(5, 1, 2, 3, 4, 5));  // 15
    return 0;
}
```

Ejemplo conocido: **`printf` es la función variádica por excelencia**. Por eso `printf("%d %s", n, txt)` puede tomar cantidades y tipos distintos de argumentos: el primer parámetro (la cadena de formato) le dice cuántos argumentos siguen y de qué tipo, y los va sacando con el mismo mecanismo `va_arg` que ves arriba.

> **Cuidado en embebido:** las funciones variádicas tienen costos ocultos. Los argumentos pasan por reglas especiales (los tipos chicos se promocionan: un `float` viaja como `double`, un `uint8_t` como `int`), y el compilador **no puede verificar los tipos** contra el formato. Un `%d` con un argumento que no es `int` es comportamiento indefinido. Además, `printf` completo es pesado en código y RAM; en micros chicos se usan versiones reducidas o se evita. Para tus propias APIs, casi siempre es mejor una función con parámetros fijos y bien tipados que una variádica.

---

## Punteros a funciones

Los punteros también pueden apuntar a funciones:

```c
int sumar(int a, int b) {
    return a + b;
}

int restar(int a, int b) {
    return a - b;
}

int main() {
    // Declarar puntero a función
    int (*operacion)(int, int);

    operacion = sumar;
    printf("Suma: %d\n", operacion(5, 3));  // 8

    operacion = restar;
    printf("Resta: %d\n", operacion(5, 3));  // 2

    return 0;
}
```

---

### Uso: callbacks

```c
void aplicar_operacion(int arr[], int tamaño, int (*func)(int)) {
    for (int i = 0; i < tamaño; i++) {
        arr[i] = func(arr[i]);
    }
}

int duplicar(int x) {
    return x * 2;
}

int cuadrado(int x) {
    return x * x;
}

int main() {
    int numeros[3] = {1, 2, 3};

    aplicar_operacion(numeros, 3, duplicar);
    // numeros = {2, 4, 6}

    aplicar_operacion(numeros, 3, cuadrado);
    // numeros = {4, 16, 36}

    return 0;
}
```

> Esto es solo una introducción. Los punteros a función se profundizan en [Punteros (avanzado)](06-punteros-avanzado.md), donde vas a ver su uso real en firmware: **tablas de vectores de interrupción**, **tablas de salto (jump tables)** para máquinas de estado, y **callbacks** registrables que pasás a un driver para que te avise cuando llega un dato.

---

## Funciones `inline`

Sugiere al compilador que inserte el código directamente (evita overhead de llamada):

```c
inline int cuadrado(int x) {
    return x * x;
}

int main() {
    int y = cuadrado(5);  // puede ser reemplazado por: int y = 5 * 5;
    return 0;
}
```

> Útil para funciones pequeñas y muy usadas. En embebidos puede mejorar performance crítica.

`inline` es solo una **sugerencia**: el compilador puede ignorarla. Las semánticas finas de `inline`, `static inline` (la forma que más se usa en headers de firmware) y `const` se ven en detalle en [`static`, `const`, `inline` y bitfields](11-static-const-inline-y-bitfields.md).

---

## Funciones `static`

Una función `static` solo es **visible dentro del archivo** donde se define:

```c
// archivo: sensor.c

static int leer_registro(int addr) {
    // función privada, solo para este archivo
}

int leer_sensor(void) {
    // función pública
    return leer_registro(0x10);
}
```

Ventajas:

* Encapsulación: oculta detalles de implementación
* Evita conflictos de nombres entre archivos
* El compilador puede optimizar mejor

> En firmware bien organizado, **todo lo que no sea parte de la API pública de un módulo se declara `static`**. Esto se trata más a fondo, junto con `static` aplicado a variables y a la visibilidad entre archivos, en [`static`, `const`, `inline` y bitfields](11-static-const-inline-y-bitfields.md).

---

## Convenciones en sistemas embebidos

### Inicialización de periféricos

```c
void GPIO_Init(void) {
    // configurar pines como entrada/salida
}

void UART_Init(uint32_t baudrate) {
    // configurar comunicación serial
}
```

---

### Lectura/escritura de hardware

```c
uint8_t ADC_Read(uint8_t canal) {
    // leer valor del convertidor analógico-digital
    return valor;
}

void LED_Set(uint8_t pin, uint8_t estado) {
    // encender/apagar LED
}
```

---

### Callbacks de interrupciones

```c
void UART_RxCallback(uint8_t dato) {
    // se llama cuando llega un byte por UART
}

void Timer_Callback(void) {
    // se llama cada vez que el timer expira
}
```

---

## Buenas prácticas

### Usar nombres descriptivos

```c
// Malo
int f(int x) { ... }

// Bueno
int calcular_temperatura_celsius(int adc_value) { ... }
```

---

### Funciones pequeñas y enfocadas

Cada función debe hacer **una cosa bien**.

```c
// Función que hace demasiado
void procesar_sensor_y_actualizar_display_y_enviar_uart(void) {
    // ...
}

// Mejor: dividir en funciones más pequeñas
void leer_sensor(void) { ... }
void actualizar_display(int valor) { ... }
void enviar_uart(int valor) { ... }

void procesar_sensores(void) {
    int valor = leer_sensor();
    actualizar_display(valor);
    enviar_uart(valor);
}
```

---

### Validar parámetros

```c
int dividir(int a, int b) {
    if (b == 0) {
        return -1;  // indicar error
    }
    return a / b;
}
```

---

### Documentar funciones

```c
/**
 * @brief Calcula el promedio de un arreglo de enteros
 * @param arr Puntero al arreglo
 * @param tamaño Número de elementos
 * @return Promedio como float
 */
float calcular_promedio(int arr[], int tamaño) {
    if (tamaño == 0) return 0.0;

    int suma = 0;
    for (int i = 0; i < tamaño; i++) {
        suma += arr[i];
    }
    return (float)suma / tamaño;
}
```

---

### Evitar efectos secundarios ocultos

```c
// Efecto secundario oculto
int contador_global = 0;
int obtener_siguiente(void) {
    contador_global++;  // modifica estado global
    return contador_global;
}

// Mejor: explícito
int obtener_siguiente(int *contador) {
    (*contador)++;
    return *contador;
}
```

---

## Alcance (scope) de variables

### Variables locales

Existen solo dentro de la función:

```c
void funcion(void) {
    int x = 10;  // local
}  // x se destruye aquí

int main() {
    // printf("%d", x);  // ERROR: x no existe aquí
    return 0;
}
```

---

### Variables `static` locales

Conservan su valor entre llamadas:

```c
void contador(void) {
    static int count = 0;  // se inicializa solo una vez
    count++;
    printf("Llamada #%d\n", count);
}

int main() {
    contador();  // Llamada #1
    contador();  // Llamada #2
    contador();  // Llamada #3
    return 0;
}
```

---

### Variables globales

Accesibles desde cualquier función:

```c
int temperatura_actual = 0;  // global

void actualizar_temperatura(int nueva) {
    temperatura_actual = nueva;
}

int obtener_temperatura(void) {
    return temperatura_actual;
}
```

> **Ojo:** Minimiza el uso de globales: dificultan el testing y el entendimiento del código.

---

## Ejemplo completo: módulo de sensor

```c
// sensor.h
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void Sensor_Init(void);
uint16_t Sensor_Read(void);
float Sensor_GetTemperature(void);

#endif

// sensor.c
#include "sensor.h"

static uint16_t ultimo_valor = 0;  // variable privada

void Sensor_Init(void) {
    // Configurar ADC, pines, etc.
}

uint16_t Sensor_Read(void) {
    // Leer valor del ADC
    ultimo_valor = ADC_Read(0);
    return ultimo_valor;
}

float Sensor_GetTemperature(void) {
    // Convertir ADC a temperatura
    return (float)ultimo_valor * 0.0625;  // ejemplo
}

// main.c
#include "sensor.h"
#include <stdio.h>

int main(void) {
    Sensor_Init();

    uint16_t raw = Sensor_Read();
    float temp = Sensor_GetTemperature();

    printf("Temperatura: %.2f°C\n", temp);

    return 0;
}
```

---

## Para los curiosos (avanzado)

> Esta sección es opcional. Si recién empezás, saltala sin culpa y volvé cuando quieras profundizar.

### Atributos de función de GCC

GCC permite "anotar" funciones con `__attribute__((...))` para darle información extra al compilador. Algunos muy usados en firmware:

```c
// Esta función nunca retorna (ej: un manejador de error que reinicia el micro).
// El compilador deja de avisar "control reaches end of non-void function"
// y puede optimizar el código que viene después de la llamada.
__attribute__((noreturn)) void panic(void);

// No emitir warning si el parámetro/función no se usa (común en callbacks).
void Timer_Callback(void *ctx __attribute__((unused)));

// Manejador de interrupción "desnudo": sin prólogo/epílogo automático.
__attribute__((naked)) void HardFault_Handler(void);

// Colocar la función/variable en una sección concreta del linker (ej: RAM).
__attribute__((section(".fast_code"))) void rutina_critica(void);

// Alinear, empaquetar, forzar que se mantenga aunque parezca no usada, etc.
__attribute__((weak)) void Default_Handler(void);   // símbolo "débil", redefinible
```

El `weak` es especialmente importante: los handlers de interrupción por defecto del LPC1769 se declaran `weak` para que vos puedas **redefinirlos** simplemente escribiendo una función con el mismo nombre, sin tocar el archivo de arranque. Lo vas a ver en el módulo de interrupciones y de build/linker.

### Tail-call (llamada de cola)

Si lo **último** que hace una función es llamar a otra y devolver su resultado (`return otra(x);`), el compilador puede reusar el marco de pila actual en vez de apilar uno nuevo: es la **optimización de llamada de cola**. Con `-O2`, GCC convierte una recursión "de cola" en un bucle, eliminando el riesgo de stack overflow. **Pero no te apoyes en esto** en firmware: no está garantizado por el estándar y depende del nivel de optimización. Si necesitás un bucle, escribí un bucle.

### `_Generic` (C11): "sobrecarga" según el tipo

C no tiene sobrecarga de funciones como C++, pero desde C11 `_Generic` permite elegir una expresión según el **tipo** de un argumento, en tiempo de compilación. Se usa para construir macros con apariencia de función genérica:

```c
#define abs_val(x) _Generic((x),        \
        int:    abs,                    \
        long:   labs,                   \
        float:  fabsf,                  \
        double: fabs                    \
    )(x)

int    a = abs_val(-5);     // usa abs
double b = abs_val(-5.0);   // usa fabs
```

Es una herramienta de bibliotecas, rara vez la vas a escribir vos, pero es útil reconocerla.

---

## Resumen

| Concepto | Descripción |
|----------|-------------|
| **Declaración** | Prototipo que indica existencia de la función |
| **Definición** | Código real de la función |
| **Paso por valor** | Se copia el argumento (no modifica original) |
| **Paso por referencia** | Se pasa puntero (puede modificar original) |
| **`inline`** | Sugiere insertar código directamente |
| **`static`** | Función visible solo en el archivo actual |
| **Recursión** | Función que se llama a sí misma |
| **Punteros a funciones** | Variables que apuntan a funciones (callbacks) |
| **Variables locales** | Existen solo dentro de la función |
| **Variables `static` locales** | Conservan valor entre llamadas |

---

**Reglas de oro:**

1. Una función, una responsabilidad
2. Nombres claros y descriptivos
3. Validar parámetros de entrada
4. Evitar efectos secundarios ocultos
5. Documentar funciones públicas
6. Minimizar uso de variables globales
7. En embebidos: cuidado con recursión profunda y uso de pila

---

---

**Anterior:** [03 - Control de flujo](./03-control-de-flujo.md) ·
**Siguiente:** [05 - Punteros](./05-punteros.md)
