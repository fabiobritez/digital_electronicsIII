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








