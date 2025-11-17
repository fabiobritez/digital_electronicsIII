## Operadores aritméticos

| Operador | Descripción | Ejemplo |
|----------|-------------|---------|
| +        | Suma | `x + y` |
| -        | Resta | `x - y` |
| *        | Multiplicación | `x * y` |
| /        | División | `x / y` |
| %        | Módulo | `x % y` |

## Operadores de relación

| Operador | Descripción | Ejemplo |
|----------|-------------|---------|
| ==       | Igual a | `x == y` |
| !=       | Distinto de | `x != y` |
| >        | Mayor que | `x > y` |
| <        | Menor que | `x < y` |
| >=       | Mayor o igual que | `x >= y` |
| <=       | Menor o igual que | `x <= y` |

Devuelven un valor de verdad (1 o 0), si se cumple la condición.

## Operadores lógicos

| Operador | Descripción | Ejemplo |
|----------|-------------|---------|
| &&       | Y lógico | `x && y` |
| ||       | O lógico | `x || y` |
| !        | No lógico | `!x` |

## Operadores de asignación

El operador de asignación básico es `=`, que asigna el valor de la derecha a la variable de la izquierda.

```c
int x = 10;  // asigna 10 a x
```

### Operadores de asignación compuesta

C permite combinar operaciones aritméticas y bitwise con la asignación:

| Operador | Descripción | Equivalente a |
|----------|-------------|---------------|
| `+=`     | Suma y asigna | `x = x + y` |
| `-=`     | Resta y asigna | `x = x - y` |
| `*=`     | Multiplica y asigna | `x = x * y` |
| `/=`     | Divide y asigna | `x = x / y` |
| `%=`     | Módulo y asigna | `x = x % y` |
| `&=`     | AND bitwise y asigna | `x = x & y` |
| `\|=`    | OR bitwise y asigna | `x = x \| y` |
| `^=`     | XOR y asigna | `x = x ^ y` |
| `<<=`    | Desplaza a izquierda y asigna | `x = x << y` |
| `>>=`    | Desplaza a derecha y asigna | `x = x >> y` |

### Ejemplos

```c
int contador = 0;
contador += 5;    // contador = 5
contador *= 2;    // contador = 10

uint8_t flags = 0b00001111;
flags &= 0xF0;    // flags = 0b00000000 (enmascara los 4 bits inferiores)
flags |= 0x01;    // flags = 0b00000001 (activa el bit 0)
```

### Ventajas

* **Más conciso**: escribir `x += 5` es más corto que `x = x + 5`
* **Más eficiente**: en algunos casos el compilador puede generar código más optimizado
* **Más claro**: muestra la intención de modificar la variable existente

> En sistemas embebidos, estos operadores son muy comunes al manipular registros de hardware y banderas.

## Operadores de incremento y decremento
Hay dos tipos de operadores de incremento y decremento:

- Pre-incremento: `++x`
- Post-incremento: `x++`


El pre-incremento y el pre-decremento incrementan y decrementan la variable antes de usar su valor, mientras que el post-incremento y el post-decremento incrementan y decrementan la variable después de usar su valor. Esto es importante para el orden de evaluación de las expresiones.

Por ejemplo:

```c
int x = 10;
int y = x++; // y = 10, x = 11
int z = ++x; // z = 12, x = 12
``` 

## Operadores de bit
 
 | Operador | Descripción | Ejemplo |
 |----------|-------------|---------|
 | &        | AND | `x & y` |
 | \|        | OR | `x \| y` |
 | ^        | XOR | `x ^ y` |
 | ~        | NOT | `~x` |
 | <<       | Desplazamiento a izquierda | `x << y` |
 | >>       | Desplazamiento a derecha | `x >> y` |
 

Estos no se pueden aplicar a valores de tipo `float` o `double`.

 
### Operador `&` (AND)


El operador bitwise AND `&` se usa a menudo para enmascarar un conjunto de bits; por ejemplo:

```c
n = 0b11001100; // n = 1100 1100
c = n & 0x0F ;  // c = n & 0000 1111 = 0000 1100 
```

Este ejemplo establece en cero todos los bits excepto los 4 bits menos significativos de la variable n.


> **Nota importante:**
> 
> Se debe distinguir cuidadosamente los operadores bitwise (& y |) de los conectivos lógicos (&& y ||), que implican una evaluación de izquierda a derecha de un valor de verdad. Por ejemplo, si `x = 1` e `y = 2`, entonces `x & y = 0`, mientras que `x && y = 1`. (¿Por qué? En C , && evalua que ambos operandos sean distintos de cero. En este caso, el valor de verdad de `x` es `true` y el valor de verdad de `y` es `true`, por lo tanto, `true && true = true`).


### Operador `|` (OR)

 El operador `OR` (|) se utiliza para activar bits:

```c
#define MASK 0x10; // MASK = 0001 0000
x = 0b10000111; // x = 1000 0111

// es equivalente a escribir x |= MASK;
x = x | MASK;   // x = 1000 0111 | 0001 0000 = 1001 0111
```
Este código establece en uno en x los bits que están en uno en MASK, es decir el bit 5.



### Operador `^` (XOR)

El operador `^` es el operador de OR exclusivo, que produce un `1` en cada posición donde sus operandos difieren:

```c
#define MASK 0xF0;  // MASK = 1111 0000
x = 0b10000111;    // x = 1000 0111

// es equivalente a escribir x ^= MASK;
x = x ^ MASK;     // x = 1000 0111 ^ 1111 0000 = 0111 0111
```

Este código invierte los bits en x que están en uno en MASK, es decir, los bits 4, 5, 6 y 7.


### Operadores de Desplazamiento

Los operadores de desplazamiento `<<` y `>>` realizan desplazamientos a la izquierda y a la derecha de su operando izquierdo por el número de posiciones de bits dado por el operando derecho.

 Así, `x << 2` desplaza `x` a la izquierda dos posiciones, llenando los bits restantes con `0`; esto tambien equivalente a multiplicar por 4. 

Por ejemplo:

```c
x = 0b00000011; // x = 0000 0011, en decimal x = 3
y = x << 2;     // y = 0000 1100, en decimal y = 12
```

Por otro lado, `x >> 2` desplaza `x` a la derecha dos posiciones, llenando los bits restantes con `0`; esto tambien equivalente a dividir por 4.

Por ejemplo:

```c
x = 0b00011000; // x = 0001 1000, en decimal x = 24
y = x >> 2;     // y = 0000 0110, en decimal y = 6
```
 


### Operador `~` (NOT)

El operador unario `~` produce el complemento a uno de un entero; es decir, convierte cada bit `1` en un bit `0` y viceversa. Este operador se utiliza típicamente en expresiones como

```c
x &  ~077
```

que enmascara los últimos 6 bits de x a 0 (asumiendo que 077 es octal).

Ejemplo:

```c
uint8_t x = 0b11111111;  // x = 255
uint8_t y = ~x;          // y = 0b00000000 = 0
```

---

## Operador ternario (condicional)

El operador ternario `? :` permite escribir expresiones condicionales de forma compacta:

```c
condición ? expresión_si_verdadero : expresión_si_falso
```

Ejemplo:

```c
int a = 10, b = 20;
int max = (a > b) ? a : b;  // max = 20
```

Es equivalente a:

```c
int max;
if (a > b) {
    max = a;
} else {
    max = b;
}
```

Útil en sistemas embebidos para asignaciones condicionales compactas:

```c
uint8_t modo = (boton_presionado()) ? MODO_ACTIVO : MODO_IDLE;
LED_Set((sensor_value > THRESHOLD) ? LED_ON : LED_OFF);
```

---

## Operador sizeof

El operador `sizeof` devuelve el tamaño en bytes de un tipo o variable:

```c
sizeof(tipo)
sizeof(expresión)
```

Ejemplos:

```c
size_t tamaño_int = sizeof(int);           // típicamente 4
size_t tamaño_char = sizeof(char);         // siempre 1
size_t tamaño_array = sizeof(int[10]);     // 40 (10 * 4)

int arr[5];
size_t elementos = sizeof(arr) / sizeof(arr[0]);  // 5
```

> `sizeof` es un operador, no una función. Se evalúa en tiempo de compilación.

---

## Precedencia y asociatividad de operadores

La precedencia determina qué operadores se evalúan primero en expresiones complejas:

```c
int resultado = 2 + 3 * 4;  // resultado = 14 (no 20)
```

Aquí, `*` tiene mayor precedencia que `+`, por lo que se evalúa primero.

### Tabla de precedencia (de mayor a menor)

| Precedencia | Operadores | Descripción | Asociatividad |
|-------------|------------|-------------|---------------|
| 1 | `()` `[]` `->` `.` | Llamadas, acceso | Izq → Der |
| 2 | `!` `~` `++` `--` `+` `-` `*` `&` `sizeof` | Unarios | Der → Izq |
| 3 | `*` `/` `%` | Multiplicación, división, módulo | Izq → Der |
| 4 | `+` `-` | Suma, resta | Izq → Der |
| 5 | `<<` `>>` | Desplazamiento | Izq → Der |
| 6 | `<` `<=` `>` `>=` | Relacionales | Izq → Der |
| 7 | `==` `!=` | Igualdad | Izq → Der |
| 8 | `&` | AND bitwise | Izq → Der |
| 9 | `^` | XOR bitwise | Izq → Der |
| 10 | `\|` | OR bitwise | Izq → Der |
| 11 | `&&` | AND lógico | Izq → Der |
| 12 | `\|\|` | OR lógico | Izq → Der |
| 13 | `? :` | Condicional ternario | Der → Izq |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `^=` `\|=` `<<=` `>>=` | Asignación | Der → Izq |
| 15 | `,` | Coma (secuencia) | Izq → Der |

### Asociatividad

- **Izq → Der**: se evalúa de izquierda a derecha
  ```c
  a + b + c  →  (a + b) + c
  ```

- **Der → Izq**: se evalúa de derecha a izquierda
  ```c
  a = b = c  →  a = (b = c)
  ```

### Ejemplos

```c
int a = 5, b = 10, c = 15;

// Precedencia de operadores
int x = a + b * c;           // x = 5 + (10 * 15) = 155

// Uso de paréntesis para cambiar precedencia
int y = (a + b) * c;         // y = (5 + 10) * 15 = 225

// Combinación de operadores bitwise y lógicos
if ((flags & 0x01) && (status == OK)) {  // correcto
    // ...
}

// ⚠️ Cuidado con la precedencia
if (flags & 0x01 && status == OK) {  // INCORRECTO
    // se interpreta como: flags & (0x01 && status) == OK
}
```

### Recomendaciones

1. **Usa paréntesis** cuando hay duda: mejora legibilidad y evita errores
2. **No confíes en la precedencia** para expresiones complejas
3. **Separa operaciones** en múltiples líneas si es necesario

```c
// ❌ Difícil de leer
int resultado = a + b << 2 & 0xFF | c;

// ✅ Mejor
int temp = (a + b) << 2;
temp &= 0xFF;
int resultado = temp | c;
```

---

## Operadores de acceso

### Operador `.` (punto)

Accede a miembros de estructuras y uniones:

```c
struct Punto {
    int x;
    int y;
};

struct Punto p = {10, 20};
int valor_x = p.x;  // acceso con punto
```

### Operador `->` (flecha)

Accede a miembros a través de punteros:

```c
struct Punto *ptr = &p;
int valor_y = ptr->y;  // equivale a (*ptr).y
```

### Operador `,` (coma)

Evalúa expresiones de izquierda a derecha y devuelve el valor de la última:

```c
int x = (5, 10, 15);  // x = 15
```

Útil en bucles `for`:

```c
for (i = 0, j = 10; i < j; i++, j--) {
    // ...
}
```

---

## Resumen

C ofrece un conjunto rico de operadores que permiten:

* **Aritmética básica**: `+`, `-`, `*`, `/`, `%`
* **Comparaciones**: `==`, `!=`, `<`, `>`, `<=`, `>=`
* **Lógica booleana**: `&&`, `||`, `!`
* **Manipulación de bits**: `&`, `|`, `^`, `~`, `<<`, `>>`
* **Asignación compuesta**: `+=`, `-=`, `*=`, `/=`, etc.
* **Incremento/decremento**: `++`, `--`
* **Otros**: `? :`, `sizeof`, `,`, `.`, `->`

**Buenas prácticas:**

1. Conoce la precedencia, pero usa paréntesis para claridad
2. Prefiere operadores compuestos (`+=`) sobre versión larga
3. Ten cuidado con `++`/`--` en expresiones complejas
4. Usa operadores bitwise en sistemas embebidos para eficiencia
5. No confundas `&`/`|` (bitwise) con `&&`/`||` (lógicos)
6. Recuerda que `sizeof` se evalúa en tiempo de compilación

---





