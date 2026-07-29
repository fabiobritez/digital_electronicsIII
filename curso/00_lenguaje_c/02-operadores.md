# Operadores

Los operadores son los verbos del lenguaje: con ellos se calcula, se compara y se decide. La mayoría
te va a resultar familiar de la matemática, pero prestá especial atención a los **operadores bitwise**
(`&`, `|`, `^`, `~`, `<<`, `>>`): en esta materia son los que más vas a usar, porque configurar un
periférico es, en el fondo, prender y apagar bits de un registro.

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

> ### Para los curiosos (avanzado): puntos de secuencia y UB
>
> No modifiques la misma variable dos veces "en el mismo paso" de una expresión, ni la leas y la modifiques sin un orden definido. Expresiones como:
>
> ```c
> i = i++ + 1;        // comportamiento INDEFINIDO
> arr[i] = i++;       // comportamiento INDEFINIDO
> func(i++, i++);     // orden de evaluación no especificado
> ```
>
> son **comportamiento indefinido (UB)** o quedan sin orden definido. El estándar define ciertos **puntos de secuencia** (sequence points): el `;` al final de una sentencia, el `&&`, `||`, `?:` y la coma `,`, y la entrada a una función. Entre dos puntos de secuencia, el compilador puede evaluar y aplicar los efectos secundarios (como `i++`) **en el orden que quiera**. Si dependés de un orden que no existe, el resultado cambia entre compiladores y niveles de optimización. **Regla:** un `++`/`--` por variable y por sentencia; si dudás, partilo en dos líneas. Compilá con `-Wall` para que GCC te avise (`-Wsequence-point`).

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

#### Cuidados con los desplazamientos (importante en embebido)

Los shifts son omnipresentes al manipular registros, pero tienen tres trampas que producen comportamiento indefinido (UB) o resultados portables solo "de casualidad":

1. **Desplazar igual o más que el ancho del tipo es UB.** En el M3 un `uint32_t` tiene 32 bits, así que `x << 32` o `x >> 32` son **comportamiento indefinido**: el resultado no es 0, es impredecible (en ARM el hardware suele tomar solo los 5 bits bajos del corrimiento, dando `<< 0`). Nunca dependas de eso.

   ```c
   uint32_t mascara = 1u << 31;   // OK: bit 31, el máximo válido en 32 bits
   uint32_t mal     = 1u << 32;   // UB: NO hagas esto
   ```

2. **Cuidado con el tipo del literal.** `1 << 31` usa `1`, que es un `int` **con signo**. Correr un 1 al bit de signo de un `int` es UB (técnicamente). Para máscaras de registros usá siempre el sufijo `u`: `1u << 31` (o, mejor todavía, `(uint32_t)1 << 31`). Esto evita sorpresas y deja la intención clara.

3. **Shift a la derecha de un valor con signo negativo está definido por la implementación.** Para `int x = -8; x >> 1;` el estándar permite que el bit de signo se replique (desplazamiento aritmético) o no. En GCC/ARM se replica, pero **no te apoyes en eso**: si vas a manipular bits, usá tipos `unsigned`, donde `>>` siempre rellena con ceros (desplazamiento lógico) de forma garantizada.

> **Regla de oro para registros:** trabajá siempre con tipos `unsigned` de ancho conocido (`uint32_t`) y literales con sufijo `u`. Así `<<`, `>>`, `~` y las máscaras se comportan de forma predecible.

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

// **Ojo:** Cuidado con la precedencia
if (flags & 0x01 && status == OK) {  // INCORRECTO
    // se interpreta como: flags & (0x01 && status) == OK
}
```

### La trampa N.º 1 de embebidos: `&` vs `==`

Mirá esta línea, típica al testear un bit de un registro:

```c
if (REG & MASK == 0) {   // ¡casi seguro NO hace lo que pensás!
    ...
}
```

En la tabla de arriba, `==` (nivel 7) tiene **mayor precedencia** que `&` (nivel 8). Así que el compilador lo lee como:

```c
if (REG & (MASK == 0)) { ... }
```

Es decir: primero evalúa `MASK == 0` (que da `0` o `1`), y recién después hace el AND. Si `MASK` es `0x10`, entonces `MASK == 0` es `0`, y `REG & 0` es siempre `0`: **el `if` nunca entra**. Un bug que cuesta horas encontrar porque "se ve bien".

La forma correcta es poner paréntesis explícitos alrededor del AND:

```c
if ((REG & MASK) == 0) { ... }   // correcto: primero el AND, después comparar
```

Lo mismo aplica a `|`, `^` y los shifts cuando se combinan con `==`, `!=`, `<`, `>`. **Regla práctica para registros:** cada vez que mezclés un operador de bits con uno de comparación, encerrá la operación de bits entre paréntesis. Activá `-Wparentheses` (viene con `-Wall`) y el compilador te marca varios de estos casos.

### Mini-tabla de precedencia que más muerde en embebido

De mayor a menor "fuerza" (los de arriba se evalúan primero):

| Más fuerte que... | Ejemplo de la trampa                | Lo que querías        |
|-------------------|-------------------------------------|-----------------------|
| `==` antes que `&`| `x & M == 0`  → `x & (M==0)`         | `(x & M) == 0`        |
| `+` antes que `<<`| `a + b << 2`  → `(a+b) << 2`         | a veces `a + (b<<2)`  |
| `*` (deref) antes... | `*p++`     → `*(p++)`               | (casi siempre lo que querés, pero sabelo) |
| `.` antes que `*` | `*ptr.campo` → `*(ptr.campo)`        | `(*ptr).campo` o `ptr->campo` |

> **No memorices la tabla completa.** Memorizá una sola cosa: **ante la duda, poné paréntesis.** Son gratis y hacen el código legible.

### Recomendaciones

1. **Usa paréntesis** cuando hay duda: mejora legibilidad y evita errores
2. **No confíes en la precedencia** para expresiones complejas
3. **Separa operaciones** en múltiples líneas si es necesario

```c
// Difícil de leer
int resultado = a + b << 2 & 0xFF | c;

// Mejor
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

---

**Anterior:** [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md) ·
**Siguiente:** [03 - Control de flujo](./03-control-de-flujo.md)
