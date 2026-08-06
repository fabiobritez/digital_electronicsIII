# Operadores

Los operadores son los verbos del lenguaje: con ellos se calcula, se compara y se decide. La mayoría
te va a resultar familiar de la matemática, pero prestá especial atención a los **operadores bitwise**
(`&`, `|`, `^`, `~`, `<<`, `>>`): en esta materia son los que más vas a usar, porque configurar un
periférico es, en el fondo, prender y apagar bits de un registro.

## Operadores aritméticos


| Operador | Descripción    | Ejemplo |
| -------- | -------------- | ------- |
| `+`      | Suma           | `x + y` |
| `-`      | Resta          | `x - y` |
| `*`      | Multiplicación | `x * y` |
| `/`      | División       | `x / y` |
| `%`      | Resto (módulo) | `x % y` |

Dos detalles que muerden: si ambos operandos son enteros, `/` es **división entera** y trunca hacia
cero (`7 / 2` da `3`, no `3.5`); y `%` solo se aplica a enteros (no existe para `float` ni `double`).


## Operadores de relación


| Operador | Descripción       | Ejemplo  |
| -------- | ----------------- | -------- |
| `==`     | Igual a           | `x == y` |
| `!=`     | Distinto de       | `x != y` |
| `>`      | Mayor que         | `x > y`  |
| `<`      | Menor que         | `x < y`  |
| `>=`     | Mayor o igual que | `x >= y` |
| `<=`     | Menor o igual que | `x <= y` |


Devuelven un valor de verdad: `1` si se cumple la condición y `0` si no.

## Operadores lógicos


| Operador | Descripción | Ejemplo    |
| -------- | ----------- | ---------- |
| `&&`     | Y lógico    | `x && y`   |
| `\|\|`   | O lógico    | `x \|\| y` |
| `!`      | No lógico   | `!x`       |

Trabajan con valores de verdad (todo lo distinto de cero es verdadero) y devuelven `1` o `0`. Además,
`&&` y `||` evalúan **en cortocircuito**: si el lado izquierdo ya decide el resultado, el derecho no
se evalúa.


## Operadores de asignación

El operador de asignación básico es `=`, que asigna el valor de la derecha a la variable de la izquierda.

```c
int x = 10;  // asigna 10 a x
```

### Operadores de asignación compuesta

C permite combinar operaciones aritméticas y bitwise con la asignación:


| Operador | Descripción                   | Equivalente a       |
| -------- | ----------------------------- | ------------------- |
| `+=`     | Suma y asigna                 | `x = x + y`         |
| `-=`     | Resta y asigna                | `x = x - y`         |
| `*=`     | Multiplica y asigna           | `x = x * y`         |
| `/=`     | Divide y asigna               | `x = x / y`         |
| `%=`     | Módulo y asigna               | `x = x % y`         |
| `&=`     | AND bitwise y asigna          | `x = x & y`         |
| `\|=`    | OR bitwise y asigna           | `x = x \| y`        |
| `^=`     | XOR bitwise y asigna          | `x = x ^ y`         |
| `<<=`    | Desplaza a izquierda y asigna | `x = x << y`        |
| `>>=`    | Desplaza a derecha y asigna   | `x = x >> y`        |


### Ejemplos

```c
int contador = 0;
contador += 5;    // contador = 5
contador *= 2;    // contador = 10

uint8_t flags = 0b00001111;
flags &= 0xF0;    // flags = 0b00000000 (borra los 4 bits inferiores)
flags |= 0x01;    // flags = 0b00000001 (activa el bit 0)
```

### Ventajas

- **Más conciso**: escribir `x += 5` es más corto que `x = x + 5`
- **Más claro**: muestra la intención de modificar la variable existente
- **Menos propenso a errores**: al nombrar la variable una sola vez, no podés equivocarte de nombre
del lado derecho.

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
> func(i++, i++);     // comportamiento INDEFINIDO: dos modificaciones de i sin
>                     // orden entre ellas (el orden de evaluación de los
>                     // argumentos, además, no está especificado)
> ```
>
> son **comportamiento indefinido (UB)** o quedan sin orden definido. El estándar define ciertos **puntos de secuencia** (sequence points): el `;` al final de una sentencia, el `&&`, `||`, `?:` y la coma `,`, y la entrada a una función. Entre dos puntos de secuencia, el compilador puede evaluar y aplicar los efectos secundarios (como `i++`) **en el orden que quiera**. Si dependés de un orden que no existe, el resultado cambia entre compiladores y niveles de optimización. **Regla:** un `++`/`--` por variable y por sentencia; si dudás, partilo en dos líneas. Compilá con `-Wall` para que GCC te avise (`-Wsequence-point`).

## Operadores de bit


| Operador | Descripción                | Ejemplo   |
| -------- | -------------------------- | --------- |
| `&`      | AND                        | `x & y`   |
| `\|`     | OR                         | `x \| y`  |
| `^`      | XOR                        | `x ^ y`   |
| `~`      | NOT (complemento a uno)    | `~x`      |
| `<<`     | Desplazamiento a izquierda | `x << y`  |
| `>>`     | Desplazamiento a derecha   | `x >> y`  |


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
> Se debe distinguir cuidadosamente los operadores bitwise (`&` y `|`) de los conectivos lógicos (`&&` y `||`), que implican una evaluación de izquierda a derecha de un valor de verdad. Por ejemplo, si `x = 1` e `y = 2`, entonces `x & y = 0`, mientras que `x && y = 1`. (¿Por qué? En C, `&&` evalúa que ambos operandos sean distintos de cero. En este caso, el valor de verdad de `x` es verdadero y el de `y` también, por lo tanto el resultado es verdadero, o sea `1`).
>
> Hay una segunda diferencia, tanto o más importante en embebido: **`&&` y `||` hacen cortocircuito y
> los operadores de bits no.** En `if (p != NULL && p->campo)` el lado derecho no se evalúa si el
> izquierdo es falso; si escribieras `&` en lugar de `&&`, se evaluarían los dos y el programa se
> caería. Los operadores de bits siempre evalúan ambos lados.

### Operador `|` (OR)

El operador `|` (OR) se utiliza para activar bits:

```c
#define MASK 0x10   // MASK = 0001 0000, o sea el bit 4
x = 0b10000111;     // x = 1000 0111

// es equivalente a escribir x |= MASK;
x = x | MASK;       // x = 1000 0111 | 0001 0000 = 1001 0111
```

Este código establece en uno en x los bits que están en uno en MASK, es decir el bit 4 (los bits se
numeran desde 0, así que `0x10` es el bit 4, no el 5).

> [!WARNING]
> **Un `#define` no lleva punto y coma al final.** El preprocesador reemplaza texto literal, así que
> `#define MASK 0x10;` hace que `MASK` valga `0x10;` **con el punto y coma incluido**. En una
> asignación suelta puede pasar desapercibido, pero en cualquier expresión revienta:
>
> ```c
> #define MASK 0x10;
> if (x & MASK) { }        // se expande a: if (x & 0x10;) { }  → error de sintaxis
> y = (x | MASK) >> 2;     // se expande a: (x | 0x10;) >> 2    → error de sintaxis
> ```
>
> Es un error clásico de quien viene de lenguajes donde toda línea termina en `;`. Más sobre esto en
> [07 - El preprocesador](./07-preprocesador.md).

### Operador `^` (XOR)

El operador `^` es el operador de OR exclusivo, que produce un `1` en cada posición donde sus operandos difieren:

```c
#define MASK 0xF0   // MASK = 1111 0000
x = 0b10000111;     // x = 1000 0111

// es equivalente a escribir x ^= MASK;
x = x ^ MASK;     // x = 1000 0111 ^ 1111 0000 = 0111 0111
```

Este código invierte los bits en x que están en uno en MASK, es decir, los bits 4, 5, 6 y 7.

### Operadores de Desplazamiento

Los operadores de desplazamiento `<<` y `>>` realizan desplazamientos a la izquierda y a la derecha de su operando izquierdo por el número de posiciones de bits dado por el operando derecho.

Así, `x << 2` desplaza `x` a la izquierda dos posiciones, llenando los bits que entran con `0`; esto también es equivalente a multiplicar por 4, mientras no se te vaya nada por arriba.

Por ejemplo:

```c
x = 0b00000011; // x = 0000 0011, en decimal x = 3
y = x << 2;     // y = 0000 1100, en decimal y = 12
```

Por otro lado, `x >> 2` desplaza `x` a la derecha dos posiciones; **si `x` es `unsigned`**, los bits que entran por arriba son `0` y equivale a dividir por 4. Si `x` tiene signo y es negativo, lo que entra por arriba lo decide el compilador (punto 3 de abajo).

Por ejemplo:

```c
uint8_t x = 0b00011000; // x = 0001 1000, en decimal x = 24
uint8_t y = x >> 2;     // y = 0000 0110, en decimal y = 6
```

#### Cuidados con los desplazamientos (importante en embebido)

Los shifts son omnipresentes al manipular registros, pero tienen tres trampas que producen comportamiento indefinido (UB) o resultados portables solo "de casualidad":

1. **Desplazar igual o más que el ancho del tipo es UB.** En el M3 un `uint32_t` tiene 32 bits, así que `x << 32` o `x >> 32` son **comportamiento indefinido**: el resultado **no es 0**, es impredecible, y lo peor es que *cada máquina se equivoca distinto*. En x86 la instrucción de corrimiento enmascara la cuenta a 5 bits, así que `x << 32` te devuelve `x` sin tocar:
  ```console
   $ ./prueba          # compilado en la PC, con el corrimiento en una variable
   x = 0xDEADBEEF, x << 32 = 0xDEADBEEF
  ```
   El barrel shifter de ARM usa los 8 bits bajos de la cuenta y con 32 o más da 0, o sea justo lo
   contrario. Y si el corrimiento es una constante, el compilador puede plegar la expresión en tiempo
   de compilación y darte un tercer resultado distinto. Nunca dependas de ninguno de los tres.
2. **Cuidado con el tipo del literal.** `1 << 31` usa `1`, que es un `int` **con signo**. Correr un 1 al bit de signo de un `int` es UB (técnicamente). Para máscaras de registros usá siempre el sufijo `u`: `1u << 31` (o, mejor todavía, `(uint32_t)1 << 31`). Esto evita sorpresas y deja la intención clara.
3. **Shift a la derecha de un valor con signo negativo está definido por la implementación.** Para `int x = -8; x >> 1;` el estándar permite que el bit de signo se replique (desplazamiento aritmético) o no. En GCC/ARM se replica, pero **no te apoyes en eso**: si vas a manipular bits, usá tipos `unsigned`, donde `>>` siempre rellena con ceros (desplazamiento lógico) de forma garantizada.

> **Regla de oro para registros:** trabajá siempre con tipos `unsigned` de ancho conocido (`uint32_t`) y literales con sufijo `u`. Así `<<`, `>>`, `~` y las máscaras se comportan de forma predecible.

### Operador `~` (NOT)

El operador unario `~` produce el complemento a uno de un entero; es decir, convierte cada bit `1` en un bit `0` y viceversa. Este operador se utiliza típicamente en expresiones como

```c
x & ~077
```

que pone en 0 los 6 bits menos significativos de `x` y deja el resto como estaba (`077` es un literal **octal**, o sea 63, o sea `0b111111`).

Ejemplo:

```c
uint8_t x = 0b11111111;  // x = 255
uint8_t y = ~x;          // y = 0b00000000 = 0
```

> [!IMPORTANT]
> Ese `y == 0` sale bien, pero **no por lo que parece**. `~` no operó sobre 8 bits: `x` se promocionó
> a `int`, así que la cuenta real fue `~0x000000FF == 0xFFFFFF00`, y recién al guardar en `y` se
> truncó a `0x00`. Con otras máscaras el resultado intermedio de 32 bits se te escapa, sobre todo si
> lo comparás en vez de guardarlo. Es la trampa que se explica en
> [02 - Promociones enteras](./02-arreglos-conversiones-y-promociones.md#trampa-1-el-complemento--de-un-tipo-chico):
> cuando uses `~` sobre tipos chicos, escribí el ancho que querés con un cast.

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

> `sizeof` es un operador, no una función. Se resuelve en tiempo de compilación y **no evalúa su
> operando**: `sizeof(i++)` no incrementa `i`, porque al compilador solo le interesa el *tipo* de la
> expresión. La única excepción son los arreglos de tamaño variable (VLA), donde el tamaño no se
> conoce hasta ejecutar. Su resultado es de tipo `size_t`.

---

## Precedencia y asociatividad de operadores

La precedencia determina qué operadores se evalúan primero en expresiones complejas:

```c
int resultado = 2 + 3 * 4;  // resultado = 14 (no 20)
```

Aquí, `*` tiene mayor precedencia que `+`, por lo que se evalúa primero.

### Tabla de precedencia (de mayor a menor)


| Precedencia | Operadores                                 | Descripción                      | Asociatividad |
| ----------- | ------------------------------------------ | -------------------------------- | ------------- |
| 1           | `()` `[]` `->` `.`                         | Llamadas, acceso                 | Izq → Der     |
| 2           | `!` `~` `++` `--` `+` `-` `*` `&` `sizeof` | Unarios                          | Der → Izq     |
| 3           | `*` `/` `%`                                | Multiplicación, división, módulo | Izq → Der     |
| 4           | `+` `-`                                    | Suma, resta                      | Izq → Der     |
| 5           | `<<` `>>`                                  | Desplazamiento                   | Izq → Der     |
| 6           | `<` `<=` `>` `>=`                          | Relacionales                     | Izq → Der     |
| 7           | `==` `!=`                                  | Igualdad                         | Izq → Der     |
| 8           | `&`                                        | AND bitwise                      | Izq → Der     |
| 9           | `^`                                        | XOR bitwise                      | Izq → Der     |
| 10          | `\|`                                       | OR bitwise                       | Izq → Der     |
| 11          | `&&`                                       | AND lógico                       | Izq → Der     |
| 12          | `\|\|`                                     | OR lógico                        | Izq → Der     |
| 13          | `? :`                                      | Condicional ternario             | Der → Izq     |
| 14          | `=` `+=` `-=` `*=` `/=` `%=` `&=` `^=` `\|=` `<<=` `>>=` | Asignación         | Der → Izq     |
| 15          | `,`                                        | Coma (secuencia)                 | Izq → Der     |


Dos precisiones sobre la tabla:

- **`++` y `--` aparecen en dos niveles.** En la forma *sufija* (`x++`) son del nivel 1, junto con `[]`
y `()`; en la forma *prefija* (`++x`) son del nivel 2, con el resto de los unarios. Por eso `*p++`
es `*(p++)`: el `++` sufijo gana.
- **El cast `(tipo)` también es un unario de nivel 2**, con asociatividad de derecha a izquierda. De ahí
que `(uint8_t)~mask` aplique el `~` primero y después el cast, que es justo lo que querés.

> **La precedencia no es orden de evaluación.** Que `*` se evalúe "antes" que `+` en `a + b * c`
> significa que `b * c` es un operando de la suma, no que el procesador calcule `b * c` primero. En
> `f() + g()`, la precedencia no dice nada sobre cuál de las dos funciones se llama antes: eso queda
> **sin especificar** y el compilador elige. Es el mismo tema del recuadro de puntos de secuencia.

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
if ((flags & 0x01) && (status == OK)) {  // correcto y explícito
    // ...
}

// Esta línea significa EXACTAMENTE lo mismo que la de arriba: tanto `&` como
// `==` se evalúan antes que `&&`, así que los paréntesis son opcionales acá.
// Ponelos igual, por legibilidad.
if (flags & 0x01 && status == OK) {
    // ...
}
```

> **Ojo con cuál es la combinación peligrosa.** Mezclar bits con `&&`/`||` es inofensivo, porque los
> operadores de bits y los de comparación se evalúan **antes** que los conectivos lógicos. La que
> muerde es mezclar bits con **comparación**, y esa va en la sección siguiente.

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


| Más fuerte que...          | Ejemplo de la trampa          | Lo que querías                            |
| -------------------------- | ----------------------------- | ----------------------------------------- |
| `==` antes que `&`         | `x & M == 0` → `x & (M==0)`   | `(x & M) == 0`                            |
| `+` antes que `<<`         | `a + b << 2` → `(a+b) << 2`   | a veces `a + (b<<2)`                      |
| `++` sufijo antes que `*`  | `*p++` → `*(p++)`             | (casi siempre lo que querés, pero sabelo) |
| `.` antes que `*` (deref)  | `*ptr.campo` → `*(ptr.campo)` | `(*ptr).campo` o `ptr->campo`             |


> **No memorices la tabla completa.** Memorizá una sola cosa: **ante la duda, poné paréntesis.** Son gratis y hacen el código legible.

### Recomendaciones

1. **Usá paréntesis** cuando hay duda: mejora la legibilidad y evita errores
2. **No confíes en la precedencia** para expresiones complejas
3. **Separá las operaciones** en varias líneas si hace falta

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

Accede a miembros de estructuras y uniones. Todavía no vimos qué es una estructura (viene en
[05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md)), pero conviene que el operador
aparezca acá, junto con el resto:

```c
struct Punto {
    int x;
    int y;
};

struct Punto p = {10, 20};
int valor_x = p.x;  // acceso con punto
```

> Existe un segundo operador de acceso, la flecha `->`, para llegar a los miembros de una estructura
> **a través de un puntero**. Como necesita punteros, se ve en
> [09 - Punteros avanzados](./09-punteros-avanzado.md#acceso-a-miembros-de-estructura-con-punteros).

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

- **Aritmética básica**: `+`, `-`, `*`, `/`, `%`
- **Comparaciones**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Lógica booleana**: `&&`, `||`, `!`
- **Manipulación de bits**: `&`, `|`, `^`, `~`, `<<`, `>>`
- **Asignación compuesta**: `+=`, `-=`, `*=`, `/=`, etc.
- **Incremento/decremento**: `++`, `--`
- **Otros**: `? :`, `sizeof`, `,`, `.`, `->`

**Buenas prácticas:**

1. Conocé la precedencia, pero poné paréntesis igual.
2. Preferí los operadores compuestos (`+=`, `|=`) sobre la versión larga, por claridad
3. Tené cuidado con `++`/`--` en expresiones complejas: uno por sentencia
4. Trabajá los registros con `uint32_t` y literales con sufijo `u`
5. No confundas `&`/`|` (bits) con `&&`/`||` (lógicos, con cortocircuito)
6. Acordate de que `sizeof` se resuelve en compilación y no evalúa su operando
7. Encerrá entre paréntesis toda operación de bits que compares con `==` o `!=`

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 5.1.2.3 (ejecución del programa y puntos de secuencia), 6.5 (expresiones y precedencia), 6.5.7 (operadores de desplazamiento), 6.5.16 (asignación).
- [cppreference: C operator precedence](https://en.cppreference.com/w/c/language/operator_precedence). La tabla de precedencia completa, con las formas prefija y sufija bien separadas.
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., §2.9. De ahí salen los ejemplos clásicos de `&`, `|` y `x & ~077`.

**GCC y el toolchain**

- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). `-Wparentheses` y `-Wsequence-point` vienen incluidos en `-Wall`.
- [GCC: Integers implementation](https://gcc.gnu.org/onlinedocs/gcc/Integers-implementation.html). Documenta que en GCC el `>>` de un valor con signo negativo es aritmético, aunque el estándar lo deje librado a la implementación.

**ARM y el LPC1769**

- [ARMv7-M Architecture Reference Manual](https://developer.arm.com/documentation/ddi0403/latest/). El pseudocódigo de `LSL`/`LSR` define que la cuenta de corrimiento sale de los 8 bits bajos del registro, de donde viene la diferencia con x86 que se menciona arriba.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [02 - Arreglos, conversiones y promociones](./02-arreglos-conversiones-y-promociones.md) ·
**Siguiente:** [04 - Control de flujo](./04-control-de-flujo.md)