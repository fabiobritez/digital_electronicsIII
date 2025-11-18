# Control de Flujo en C

El control de flujo permite que un programa tome decisiones y repita tareas. En C tenemos:

* **Estructuras de decisión**: `if`, `else`, `else if`, `switch`
* **Bucles**: `while`, `for`, `do-while`
* **Saltos**: `break`, `continue`, `return`, `goto`

---

## 1. Estructuras de decisión: `if`, `else`, `else if`

### Sintaxis básica

```c
if (condición) {
    // se ejecuta si condición es verdadera (≠ 0)
}
```

Si la condición es verdadera (cualquier valor distinto de cero), se ejecuta el bloque.

### `if-else`

```c
if (condición) {
    // se ejecuta si condición es verdadera
} else {
    // se ejecuta si condición es falsa
}
```

### `if-else if-else`

```c
if (condición1) {
    // bloque 1
} else if (condición2) {
    // bloque 2
} else {
    // bloque por defecto
}
```

---

### Ejemplos

```c
int temperatura = 85;

if (temperatura > 100) {
    printf("Sobrecalentado\n");
} else if (temperatura > 75) {
    printf("Temperatura alta\n");
} else {
    printf("Temperatura normal\n");
}
```

En sistemas embebidos:

```c
uint8_t sensor_value = ADC_Read();

if (sensor_value > THRESHOLD_HIGH) {
    LED_On();
    Alarm_Trigger();
} else if (sensor_value < THRESHOLD_LOW) {
    LED_Blink();
} else {
    LED_Off();
}
```

---

### Operador ternario (`? :`)

Una forma compacta de `if-else` para asignaciones simples:

```c
resultado = (condición) ? valor_si_verdadero : valor_si_falso;
```

Ejemplo:

```c
int max = (a > b) ? a : b;
```

Equivale a:

```c
int max;
if (a > b) {
    max = a;
} else {
    max = b;
}
```

> El operador ternario es útil para código conciso, pero **no abuses**: puede reducir legibilidad si se anida.

---

## 2. Estructura `switch`

Permite elegir entre múltiples casos basados en el valor de una expresión entera (o carácter).

### Sintaxis

```c
switch (expresión) {
    case valor1:
        // código para valor1
        break;
    case valor2:
        // código para valor2
        break;
    default:
        // código si ningún caso coincide
        break;
}
```

---

### Características importantes

* La expresión debe ser de tipo **entero** o **carácter** (int, char, enum).
* Cada `case` debe terminar con un valor constante.
* **`break`** es necesario para evitar que se ejecuten los siguientes casos (*fall-through*).
* **`default`** es opcional, pero recomendado para manejar valores inesperados.

---

### Ejemplo básico

```c
int opcion = 2;

switch (opcion) {
    case 1:
        printf("Opción 1 seleccionada\n");
        break;
    case 2:
        printf("Opción 2 seleccionada\n");
        break;
    case 3:
        printf("Opción 3 seleccionada\n");
        break;
    default:
        printf("Opción no válida\n");
        break;
}
```

---

### Fall-through intencional

A veces queremos que varios casos ejecuten el mismo código:

```c
char c = 'a';

switch (c) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
        printf("Es una vocal\n");
        break;
    default:
        printf("Es una consonante\n");
        break;
}
```

---

### Uso en sistemas embebidos: máquina de estados

```c
enum State { IDLE, RUNNING, PAUSED, ERROR };
enum State estado = IDLE;

switch (estado) {
    case IDLE:
        // inicializar sistema
        if (start_button_pressed()) {
            estado = RUNNING;
        }
        break;

    case RUNNING:
        // ejecutar tarea principal
        if (pause_button_pressed()) {
            estado = PAUSED;
        } else if (error_detected()) {
            estado = ERROR;
        }
        break;

    case PAUSED:
        // esperar
        if (resume_button_pressed()) {
            estado = RUNNING;
        }
        break;

    case ERROR:
        // manejar error
        LED_Error();
        estado = IDLE;
        break;

    default:
        // estado no reconocido → reiniciar
        estado = IDLE;
        break;
}
```

> `switch` es ideal para **máquinas de estados**, muy comunes en firmware embebido.

---

## 3. Bucle `while`

Ejecuta un bloque mientras la condición sea verdadera.

### Sintaxis

```c
while (condición) {
    // código a repetir
}
```

La condición se evalúa **antes** de cada iteración. Si es falsa desde el principio, el bloque nunca se ejecuta.

---

### Ejemplo

```c
int i = 0;
while (i < 5) {
    printf("%d\n", i);
    i++;
}
```

Salida:
```
0
1
2
3
4
```

---

### Ejemplo embebido: esperar evento

```c
while (!UART_DataReady()) {
    // espera activa (polling) hasta que llegue un dato
}
char dato = UART_Read();
```

> En embebidos, ten cuidado con bucles infinitos que bloquean la CPU. Considera usar interrupciones o timeouts.

---

## 4. Bucle `do-while`

Similar a `while`, pero la condición se evalúa **después** de cada iteración. Esto garantiza que el bloque se ejecute **al menos una vez**.

### Sintaxis

```c
do {
    // código a repetir
} while (condición);
```

---

### Ejemplo

```c
int i = 0;
do {
    printf("%d\n", i);
    i++;
} while (i < 5);
```

Salida (igual que `while`):
```
0
1
2
3
4
```

---

### Diferencia clave con `while`

```c
int x = 10;

// Con while: NO se ejecuta
while (x < 5) {
    printf("while: %d\n", x);
}

// Con do-while: se ejecuta UNA VEZ
do {
    printf("do-while: %d\n", x);
} while (x < 5);
```

Salida:
```
do-while: 10
```

---

### Uso común: validación de entrada

```c
int entrada;
do {
    printf("Ingrese un número entre 1 y 10: ");
    scanf("%d", &entrada);
} while (entrada < 1 || entrada > 10);
```

Esto asegura pedir entrada al menos una vez.

---

## 5. Bucle `for`

Es la forma más compacta de iterar un número determinado de veces.

### Sintaxis

```c
for (inicialización; condición; actualización) {
    // código a repetir
}
```

---

### Componentes

1. **Inicialización**: se ejecuta una sola vez al inicio
2. **Condición**: se evalúa antes de cada iteración
3. **Actualización**: se ejecuta al final de cada iteración

---

### Ejemplo clásico

```c
for (int i = 0; i < 10; i++) {
    printf("%d\n", i);
}
```

Equivale a:

```c
int i = 0;
while (i < 10) {
    printf("%d\n", i);
    i++;
}
```

---

### Recorrer un arreglo

```c
int numeros[5] = {10, 20, 30, 40, 50};

for (int i = 0; i < 5; i++) {
    printf("numeros[%d] = %d\n", i, numeros[i]);
}
```

---

### Variantes del `for`

Puedes omitir cualquier parte (pero los `;` deben estar):

```c
int i = 0;
for (; i < 10; ) {   // sin inicialización ni actualización
    printf("%d\n", i);
    i += 2;
}
```

Bucle infinito:

```c
for (;;) {
    // equivale a while(1)
}
```

---

### Ejemplo embebido: inicializar buffer

```c
uint8_t buffer[256];
for (int i = 0; i < 256; i++) {
    buffer[i] = 0;
}
```

O más eficientemente con aritmética de punteros:

```c
uint8_t *ptr = buffer;
for (int i = 0; i < 256; i++) {
    *ptr++ = 0;
}
```

---

## 6. Instrucciones de salto

### `break`

Sale **inmediatamente** del bucle o `switch` más interno.

```c
for (int i = 0; i < 10; i++) {
    if (i == 5) {
        break;  // sale del for cuando i == 5
    }
    printf("%d\n", i);
}
// imprime 0, 1, 2, 3, 4
```

Uso en embebidos:

```c
while (1) {
    if (UART_DataReady()) {
        char c = UART_Read();
        if (c == '\n') {
            break;  // termina al recibir nueva línea
        }
        buffer[index++] = c;
    }
}
```

---

### `continue`

Salta el resto de la iteración actual y pasa a la siguiente.

```c
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // salta los pares
    }
    printf("%d\n", i);
}
// imprime 1, 3, 5, 7, 9
```

---

### Diferencia entre `break` y `continue`

```c
for (int i = 0; i < 5; i++) {
    if (i == 2) break;
    printf("%d ", i);
}
// Salida: 0 1

for (int i = 0; i < 5; i++) {
    if (i == 2) continue;
    printf("%d ", i);
}
// Salida: 0 1 3 4
```

---

### `return`

Sale de la función actual y opcionalmente devuelve un valor.

```c
int encontrar(int arr[], int size, int valor) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == valor) {
            return i;  // devuelve el índice y sale
        }
    }
    return -1;  // no encontrado
}
```

---

### `goto` (⚠️ usar con extrema precaución)

Salta a una etiqueta en el código.

```c
int main() {
    int i = 0;
inicio:
    printf("%d\n", i);
    i++;
    if (i < 5) {
        goto inicio;
    }
    return 0;
}
```

---

### ¿Cuándo usar `goto`?

**Casi nunca**. Hace el código difícil de seguir ("código espagueti").

**Casos legítimos**:

1. **Limpieza de recursos en funciones con múltiples puntos de salida**

```c
int procesar_archivo(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char *buffer = malloc(1024);
    if (!buffer) goto cleanup_file;

    int *data = malloc(sizeof(int) * 100);
    if (!data) goto cleanup_buffer;

    // procesar...

    free(data);
cleanup_buffer:
    free(buffer);
cleanup_file:
    fclose(f);
    return 0;
}
```

2. **Salir de bucles anidados**

```c
for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
        if (condicion_error) {
            goto error;  // sale de ambos bucles
        }
    }
}
error:
    // manejo de error
```

> En la mayoría de casos, `break`, `continue` o reestructurar el código es mejor que `goto`.

---

## 7. Bucles infinitos en sistemas embebidos

En firmware embebido, el bucle principal suele ser infinito:

```c
int main(void) {
    // inicialización
    System_Init();
    Peripherals_Init();

    // bucle principal
    while (1) {
        // tarea 1
        Leer_Sensores();

        // tarea 2
        Actualizar_Display();

        // tarea 3
        Procesar_Comandos();
    }

    return 0;  // nunca se alcanza
}
```

También se puede escribir como:

```c
for (;;) {
    // bucle infinito
}
```

---

## 8. Consejos y buenas prácticas

### ✅ Usar llaves `{}` siempre

Aunque no sean necesarias para una sola instrucción:

```c
// ❌ Evitar (propenso a errores)
if (x > 0)
    y = 1;

// ✅ Mejor
if (x > 0) {
    y = 1;
}
```

### ✅ Evitar condiciones complejas

```c
// ❌ Difícil de leer
if ((x > 0 && y < 10) || (z == 5 && w != 3)) {
    // ...
}

// ✅ Mejor
bool condicion1 = (x > 0 && y < 10);
bool condicion2 = (z == 5 && w != 3);
if (condicion1 || condicion2) {
    // ...
}
```

### ✅ Inicializar variables de bucle

```c
// ❌ No confiable
int i;
for (i = 0; i < 10; i++) { ... }

// ✅ Mejor (C99+)
for (int i = 0; i < 10; i++) { ... }
```

### ✅ Evitar comparaciones con punto flotante

```c
// ❌ Puede fallar por errores de precisión
float x = 0.1 + 0.2;
if (x == 0.3) { ... }

// ✅ Usar tolerancia
#define EPSILON 0.0001
if (fabs(x - 0.3) < EPSILON) { ... }
```

### ✅ Usar `switch` para múltiples comparaciones con el mismo valor

```c
// ❌ Repetitivo
if (estado == 1) { ... }
else if (estado == 2) { ... }
else if (estado == 3) { ... }

// ✅ Más claro
switch (estado) {
    case 1: ... break;
    case 2: ... break;
    case 3: ... break;
}
```

---

## 9. Resumen

| Estructura | Uso | Ejemplo |
|------------|-----|---------|
| `if-else` | Decisión binaria | `if (x > 0) { ... } else { ... }` |
| `switch` | Múltiples opciones | `switch (opcion) { case 1: ... }` |
| `while` | Repetir mientras condición | `while (i < 10) { ... }` |
| `do-while` | Ejecutar al menos una vez | `do { ... } while (i < 10);` |
| `for` | Repetir N veces | `for (int i = 0; i < 10; i++) { ... }` |
| `break` | Salir de bucle/switch | `if (done) break;` |
| `continue` | Saltar a siguiente iteración | `if (skip) continue;` |
| `return` | Salir de función | `return resultado;` |
| `goto` | Salto incondicional (evitar) | `goto error;` |

---
