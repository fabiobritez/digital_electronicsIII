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

> ⚠️ **NUNCA** retornes un puntero a una variable local (no estática):

```c
// ❌ INCORRECTO
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

> ⚠️ En sistemas embebidos, evita recursión profunda: consume mucha pila y puede causar overflow.

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

Ejemplo conocido: `printf` usa argumentos variables.

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

### ✅ Usar nombres descriptivos

```c
// ❌ Malo
int f(int x) { ... }

// ✅ Bueno
int calcular_temperatura_celsius(int adc_value) { ... }
```

---

### ✅ Funciones pequeñas y enfocadas

Cada función debe hacer **una cosa bien**.

```c
// ❌ Función que hace demasiado
void procesar_sensor_y_actualizar_display_y_enviar_uart(void) {
    // ...
}

// ✅ Mejor: dividir en funciones más pequeñas
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

### ✅ Validar parámetros

```c
int dividir(int a, int b) {
    if (b == 0) {
        return -1;  // indicar error
    }
    return a / b;
}
```

---

### ✅ Documentar funciones

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

### ✅ Evitar efectos secundarios ocultos

```c
// ❌ Efecto secundario oculto
int contador_global = 0;
int obtener_siguiente(void) {
    contador_global++;  // modifica estado global
    return contador_global;
}

// ✅ Mejor: explícito
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

> ⚠️ Minimiza el uso de globales: dificultan el testing y el entendimiento del código.

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
