# Expresiones, operadores, promociones y conversiones

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
> int y = i++ + i;    // comportamiento INDEFINIDO: modifica y lee i sin orden
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
> [C7 - El preprocesador](./07-preprocesador.md).

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
> [Promociones enteras](#trampa-1-el-complemento--de-un-tipo-chico):
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
size_t bytes_int = sizeof(int);       // 4 en el ABI del Cortex-M3
size_t bytes_double = sizeof(double); // 8 en este toolchain
```

La aplicación de `sizeof` para calcular el tamaño y la cantidad de elementos de un arreglo se hace
en C5, una vez definido ese tipo compuesto.

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

Los operadores `.` y `->` también pertenecen a este nivel de precedencia, pero se presentan junto
con los tipos sobre los que trabajan: `.` en C6 (`struct` y `union`) y `->` en C9 (puntero a
estructura).

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


### Reglas de conversión que conviene conservar

- Los tipos enteros menores que `int` se promocionan antes de operar.
- No mezcles signo en comparaciones sin justificar el tipo común.
- El overflow `unsigned` es módulo 2^N; el overflow `signed` es comportamiento indefinido.
- Un cast hace explícito cuándo convertís, pero no vuelve segura una operación que ya era inválida.
- Usá `-Wall -Wextra`: `-Wsign-compare` y `-Wtype-limits` son parte de `-Wextra` en C.

Fuentes específicas: [C17 N2176](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf),
cláusulas 6.3.1.1, 6.3.1.3 y 6.3.1.4; y
[GCC Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html).

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
**Anterior:** [C1 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md) ·
**Siguiente:** [C3 - Control de flujo](./03-control-de-flujo.md)
