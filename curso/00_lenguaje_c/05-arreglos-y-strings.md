# Arreglos y strings

Hasta ahora trabajamos con variables individuales. Un arreglo permite reunir varios datos del mismo
tipo y recorrerlos con un índice. Los *strings* se construyen de la misma manera: son arreglos de
caracteres que terminan con un byte nulo (`'\0'`).

En los primeros ejemplos vamos a usar índices. La relación entre arreglos y punteros se estudia más
adelante, cuando ya tengamos claro qué es una dirección y cómo se desreferencia.

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
> **Un arreglo local SIN inicializar NO arranca en cero.** `int arr[5];` dentro de una función te da 5 valores *indeterminados*, y leerlos antes de escribirlos es **comportamiento indefinido**, no "te sale lo que había en el stack" (el porqué de esa distinción está en [C1 - Declaraciones](./01-declaraciones-y-tipos.md#2-especificador-de-almacenamiento)). Solo los arreglos con duración estática (globales o `static`) arrancan en cero, porque el código de arranque limpia la sección `.bss`. Es un error clásico: funciona en el escritorio por casualidad y falla en el micro.

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

Sobre un arreglo completo, `sizeof` permite calcular tanto los bytes ocupados como la cantidad de
elementos:

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

## Strings recorridos por índice

Una cadena de C es un arreglo de `char` cuyo contenido útil termina en el primer `'\0'`. Antes de
usar funciones de biblioteca o punteros conviene poder recorrer esa representación de forma
explícita:

```c
char mensaje[] = "LPC1769";
size_t largo = 0;

while (mensaje[largo] != '\0') {
    largo++;
}
```

El límite físico y el final lógico no son lo mismo. En `char mensaje[20] = "Hola";`, el arreglo
tiene 20 bytes, pero la cadena tiene 4 caracteres útiles y un terminador. Los otros 15 bytes
quedaron en cero por la inicialización.

Para copiar por índice, el contrato debe incluir la capacidad del destino y siempre reservar un byte
para el terminador:

```c
char origen[] = "ADC";
char destino[8];
size_t i = 0;

while (origen[i] != '\0' && i + 1 < sizeof destino) {
    destino[i] = origen[i];
    i++;
}
destino[i] = '\0';
```

> [!CAUTION]
> Un arreglo de caracteres no es necesariamente una cadena. `char datos[3] = {'A', 'D', 'C'};`
> ocupa tres bytes pero no tiene terminador; pasarlo a una operación que espera un string hará que
> siga leyendo fuera del arreglo. C no guarda la longitud ni comprueba el límite por vos.

Las funciones de `<string.h>`, sus contratos y sus trampas se estudian en C9, cuando ya se puede
explicar por qué reciben punteros y por qué necesitan límites explícitos.

---

## Bucles aplicados a arreglos y buffers

Ahora que C3 ya definió `for`, `while`, `break` y `continue`, podemos aplicarlos a objetos cuyo
tamaño y terminador conocemos.

### Recorrer un arreglo

```c
int numeros[5] = {10, 20, 30, 40, 50};

for (int i = 0; i < 5; i++) {
    printf("numeros[%d] = %d\n", i, numeros[i]);
}
```

---

### Ejemplo embebido: inicializar buffer

```c
uint8_t buffer[256];
for (size_t i = 0; i < sizeof buffer; i++) {
    buffer[i] = 0;
}
```

> Fijate el `sizeof buffer` en vez del `256` repetido: si mañana cambiás el tamaño del arreglo, el
> bucle se ajusta solo. Un `256` escrito a mano en dos lugares es un desbordamiento esperando a pasar.

---

### Cortar una recepción sin desbordar el buffer

Uso en embebidos:

```c
char buffer[64];
size_t index = 0;

while (1) {
    if (UART_DataReady()) {
        char c = UART_Read();
        if (c == '\n' || index == sizeof buffer - 1) {
            break;  // termina al recibir nueva línea, o si se llenó el buffer
        }
        buffer[index++] = c;
    }
}
buffer[index] = '\0';
```

> Fijate la segunda condición del `break`: sin ella, una línea más larga que el buffer lo desborda y
> te pisa memoria vecina. En firmware, **todo bucle que escribe en un arreglo tiene que tener un
> límite además de su condición "natural"**.

### Expresiones condicionales en inicializadores

Como el operador ternario produce un valor, puede usarse dentro de un inicializador:

```c
uint8_t config[2] = { modo_pwm ? 0x0F : 0x00, 0x20 };
```

Esto vale para una variable local. Si el arreglo fuera `static` o global, el inicializador tendría
que estar formado por expresiones constantes.

---

## Resumen y reglas de C5

| Regla | Motivo |
|---|---|
| El último índice de `T v[N]` es `N - 1` | C no verifica límites |
| Un arreglo local sin inicializar tiene valores indeterminados | Solo los objetos estáticos arrancan en cero |
| `sizeof v / sizeof v[0]` se usa donde `v` todavía es un arreglo | En C9 se explica por qué deja de servir como parámetro |
| Una matriz se inicializa por filas | Expresa el layout y permite que `-Wmissing-braces` ayude |
| Todo string válido termina en `'\0'` | Las operaciones de texto recorren hasta ese byte |
| Al copiar texto se reserva un byte para `'\0'` | Evita leer o escribir fuera del buffer |

---

## Fuentes y para seguir leyendo

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf).
  Cláusulas 6.5.2.1 (indexado), 6.7.6.2 (declaradores de arreglo), 6.7.9
  (inicialización) y 7.1.1 (definición de string).
- [cppreference: arrays](https://en.cppreference.com/w/c/language/array) y
  [null-terminated byte strings](https://en.cppreference.com/w/c/string/byte).
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., capítulos 1 y 5.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [C4 - Funciones](./04-funciones.md) ·
**Siguiente:** [C6 - Estructuras, enumeraciones y estado](./06-estructuras-y-enums.md)
