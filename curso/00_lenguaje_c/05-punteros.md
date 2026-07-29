# Punteros


Los punteros son el tema que define si entendés C o no. Y en esta materia son ineludibles: como vas a
ver en el módulo 01, **un registro de hardware es literalmente una dirección fija de memoria a la que
se accede por puntero**, así que cada vez que configures un periférico vas a estar usando esto.

En este capítulo vemos qué es un puntero, los operadores `&` y `*`, la aritmética de punteros y cómo
se llevan con los distintos tipos de datos, marcando las buenas prácticas y los errores clásicos (que
en un micro no terminan en un mensaje de error, sino en un *HardFault* o en un bug silencioso). Lo que
sigue (arreglos y *decay*, cadenas, punteros a función) está en el
[capítulo 6](./06-punteros-avanzado.md).


## ¿Qué es un puntero?

Un puntero en C es, fundamentalmente, una variable que almacena la dirección de otra variable.  
Cada variable en un programa en ejecución se guarda en una ubicación específica de memoria. Esta ubicación tiene una dirección (en general expresada como un número hexadecimal). Un puntero guarda esa dirección.  Esto permite la manipulación de memoria a bajo nivel y más adelante veremos como nos permite un manejo eficiente de arreglos y cadenas de caracteres en C.

> **IMPORTANTE**
>
> - El tamaño de un puntero es fijo para cualquier tipo de dato, y depende de la arquitectura del procesador. En los ARM Cortex-M 3, los punteros son de 32 bits.

## Declaración e inicialización de punteros

Para usar punteros, primero debemos declararlos.
Una declaración de puntero especifica el tipo de dato al que apuntará, seguido de un asterisco `*` y el nombre del puntero.
La forma general es:

```c
data_type *nombre_puntero;
```


> **El tipo es importante:** A los punteros se les asigna un tipo de dato, que es el tipo de dato de la variable a la que apuntan. Esto es importante, porque el compilador sabe cuántos bytes debe leer o escribir en memoria cuando se desreferencia el puntero.

Un `int*` solo puede apuntar a un entero (o al primer elemento de un arreglo de enteros), un `char*` a un carácter, y así sucesivamente.


### Ejemplo:

Por ejemplo, si tenemos una variable entera `num` almacenada en la dirección de memoria `0x80000004`, un puntero podría contener el valor `0x80000004` para “apuntar” a `num`.  
```c
int num = 42;
int *ptr;
ptr = &num;

printf("El valor de num es: %d\n", num);
printf("La dirección de num es: %p\n", &num);
printf("El valor de ptr es: %p\n", ptr);
printf("El valor al que apunta ptr es: %d\n", *ptr);
````

Resultado:
```shell
El valor de num es: 42
La dirección de num es: 0x80000004
El valor de ptr es: 0x80000004
El valor al que apunta ptr es: 42
```

En este fragmento, `int *ptr;` declara `ptr` como un puntero a entero. 

La instrucción `ptr = &num;` asigna a `ptr` la dirección de `num` (usando el operador de dirección `&`). Ahora decimos que “`ptr` apunta a `num`”.


Luego imprimimos `ptr` (con el especificador de formato `%p` para direcciones).

 
### Punteros sin inicializar:

Cuando un puntero se declara, no apunta automáticamente a algo significativo. Si simplemente escribimos `int *ptr;` dentro de una función, `ptr` contendrá una dirección basura indefinida hasta que lo inicialicemos.


> **IMPORTANTE**
>
> Usar un puntero sin inicializar es un error grave: se le llama  *wild pointer*, porque apunta a una ubicación de memoria aleatoria.
>
> Desreferenciar un puntero sin inicializar (acceder a la memoria a la que apunta) puede bloquear el programa o corromper datos.
>
> **Por eso, siempre se debe inicializar un puntero antes de usarlo.**

--- 
### Operadores de puntero


Después de inicializar un puntero, este puede usarse para referirse a la variable o al bloque de memoria al que apunta.
En este momento, es importante entender dos operadores clave:

* **`&` (address-of):** colocado antes de una variable (ej., `&var`) devuelve la dirección de memoria de esa variable.
  Lo usamos para obtener direcciones que podamos asignar a punteros.

* **`*` (dereferencia):** colocado antes de un puntero (ej., `*ptr`) accede al valor almacenado en la dirección contenida en el puntero.

> **IMPORTANTE**
>
> El `*` debe estar entre el tipo de dato y el nombre del puntero. Puede estar pegado al tipo de dato o al nombre del puntero y tambien puede haber un espacio entre ambos.

Por ejemplo: 
```c
int* ptr;
int * ptr;
int *ptr;
```





---

## Desreferenciando punteros (accediendo a valores apuntados)

Una vez que un puntero tiene una dirección válida, podés usar el operador de desreferencia `*` para acceder o modificar el dato en esa dirección.
Desreferenciar significa “seguir” el puntero hasta su ubicación y tratarla como una variable.
Por ejemplo:

```c
int num = 10;
int *ptr = &num;
printf("%d\n", *ptr);  // imprime 10, el valor de num
*ptr = 20;             // modifica el valor en la dirección de ptr, es decir, cambia num a 20
printf("%d\n", num);   // imprime 20, confirmando que num cambió vía el puntero
```

Esto muestra cómo los punteros permiten leer y escribir variables de forma indirecta.
En este caso, tanto `num` como `*ptr` se refieren a la misma variable.


> **RESUMEN**
> 
> Los operadores `&` y `*` son inversos entre sí:  
> Si `ptr = &var;` ---> entonces `*ptr` es `var`.   

 
## Aritmética de punteros

Los punteros no se usan solo para variables individuales; también podés hacer operaciones aritméticas con ellos para moverte por la memoria. Esto es especialmente útil con arreglos (tema que veremos después).  

En C, la **aritmética de punteros** está definida como operaciones que mueven el puntero para apuntar a otras ubicaciones de memoria relativas a la actual.  
Sin embargo, no se comporta como la aritmética de enteros normal: **está escalada por el tamaño del tipo de dato al que apunta el puntero**.

Por ejemplo, supongamos que `ptr` es un `int*` que guarda la dirección `0x1000`.  
En un sistema de 32 bits, un `int` ocupa 4 bytes, así que:

- `ptr + 1` → dirección `0x1004` (siguiente entero en memoria)  
- `ptr + 2` → dirección `0x1008` (dos enteros adelante)  


<img src="./img/arithmetic_pointers.png" width="70%"/>

- En general, `ptr + n` mueve el puntero hacia adelante **n elementos** (no bytes) de su tipo.  
- Del mismo modo, `ptr - n` lo mueve hacia atrás **n elementos**.  
- El compilador hace este escalado automáticamente en función del tipo de puntero.

Ejemplos por tipo de dato:

- Si `ptr` es un `char*` (1 byte por elemento), `ptr + 1` avanza 1 byte.  
- Si `ptr` es un `double*` (8 bytes por elemento en la mayoría de sistemas), `ptr + 1` salta 8 bytes.

---

### Operaciones válidas con punteros

- **Incremento y decremento:**  
  `p++` o `++p` avanza al siguiente elemento,  
  `p--` retrocede al elemento anterior.

- **Suma y resta de un entero:**  
  `p + n` o `p - n` mueve el puntero **n elementos** hacia adelante o atrás (el desplazamiento se escala por el tamaño del tipo apuntado).

- **Resta entre punteros:**  
  La resta entre punteros es válida si ambos son del mismo tipo. 
  El resultado es el número de elementos entre los punteros (no es el número de bytes).

Por ejemplo: si tenemos dos punteros tipo `int*` ptr1 (dirección: 1000) y ptr2 (dirección: 1004) y los restamos, la diferencia entre direcciones es de 4 bytes. Como el tamaño de un int es de 4 bytes, la diferencia entre ptr1 y ptr2 viene dado es = 1.

 
---

### Operaciones no permitidas
No podés multiplicar, dividir o sumar dos punteros. Estas operaciones no tienen sentido conceptual para direcciones y C generará un error de compilación.

---

### Ejemplo

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;        // apunta a arr[0]
int *q = arr + 3;    // apunta a arr[3] (valor 40)

printf("%d\n", *p);        // imprime 10 (arr[0])
p++;                       // avanza al siguiente elemento
printf("%d\n", *p);        // imprime 20 (arr[1])
printf("%ld\n", q - p);    // imprime 2 (cant. de elementos entre arr[1] y arr[3])
```

 
La aritmética de punteros es muy usada en bucles para recorrer arreglos o buffers, a menudo combinada con desreferencia (`*(p + i)` es equivalente a `p[i]`).  

> **PRECAUCIÓN**
>
> Si te salís de los límites del arreglo (por ejemplo, avanzas más allá de su rango válido), el puntero ya no apuntará a memoria legítima y desreferenciarlo será **comportamiento indefinido** . Genera los problemas de *buffer overflow*.
>
> C **no** hace verificación de límites en el compilador ni en tiempo de ejecución.

---

## `NULL`: el puntero que no apunta a nada

A veces necesitamos un puntero que explícitamente **no apunte a ningún lado**. Para eso existe `NULL`, una constante (definida en `<stddef.h>`, `<stdio.h>` y otros headers) que vale `0` y representa "puntero inválido / vacío".

```c
int *p = NULL;   // p no apunta a nada todavía
```

La gran ventaja es que `NULL` se puede **comprobar**. En vez de tener un puntero basura (*wild pointer*) que no sabemos a dónde apunta, un puntero en `NULL` se puede chequear antes de usarlo:

```c
if (p != NULL) {
    *p = 10;     // solo desreferenciamos si p es válido
}
```

> **PRECAUCIÓN**
>
> Desreferenciar un puntero `NULL` (`*p` cuando `p == NULL`) es **comportamiento indefinido**. En una PC normalmente provoca un *segmentation fault*. En el LPC1769 la dirección `0x00000000` es el **inicio de la Flash** (ahí vive el vector de reset), así que escribir en `NULL` puede no fallar de inmediato pero corrompe la lógica de tu programa de forma muy difícil de depurar. **Siempre chequeá los punteros que pueden venir en `NULL`.**

Un patrón muy común en funciones que reciben punteros (por ejemplo, drivers) es validar la entrada:

```c
void uart_send(const uint8_t *data, uint32_t len) {
    if (data == NULL || len == 0) {
        return;          // no hay nada que enviar; evitamos el crash
    }
    // ... enviar data ...
}
```

---

## `const` y punteros (const-correctness)

Esta es una de las partes más importantes de los punteros para programar microcontroladores, y casi siempre se explica mal. La clave: **`const` puede aplicarse al dato apuntado, al puntero en sí, o a ambos**, y son cosas distintas.

La regla para leer una declaración es **de derecha a izquierda**, empezando por el nombre de la variable:

```c
const uint32_t *p;          // p: puntero a (uint32_t const) -> el DATO es const
uint32_t * const p;         // p: const puntero a uint32_t   -> el PUNTERO es const
const uint32_t * const p;   // p: const puntero a (uint32_t const) -> ambos const
```

Veámoslos uno por uno.

**1. Puntero a dato constante: `const uint32_t *p`**

Podés mover el puntero, pero **no podés modificar el valor apuntado a través de él**.

```c
uint32_t x = 5, y = 9;
const uint32_t *p = &x;

p = &y;        // OK: el puntero se puede reasignar
*p = 100;      // ERROR de compilación: el dato es de solo lectura vía p
```

Esto es lo que ves en la firma de `strlen(const char *s)`: la función promete que **no va a tocar** la cadena que le pasás. Es una garantía documentada y verificada por el compilador.

**2. Puntero constante: `uint32_t * const p`**

El puntero **siempre apunta al mismo lado** (no se puede reasignar), pero sí podés modificar el dato apuntado.

```c
uint32_t x = 5, y = 9;
uint32_t * const p = &x;

*p = 100;      // OK: modificamos x a través de p
p = &y;        // ERROR de compilación: p no se puede reasignar
```

**3. Ambos constantes: `const uint32_t * const p`**

Ni se reasigna el puntero ni se modifica el dato a través de él.

```c
const uint32_t * const p = &x;
*p = 100;      // ERROR
p = &y;        // ERROR
```

> **¿Por qué importa tanto en embebido?**
>
> Por los **registros de hardware**. Un registro de **solo lectura** (por ejemplo, un registro de estado o un buffer de recepción) se modela como puntero a `volatile` **y** `const`:
> ```c
> // Registro de estado de solo lectura en 0x40000000
> volatile const uint32_t * const STATUS = (volatile const uint32_t *)0x40000000;
> uint32_t s = *STATUS;   // OK: leer
> *STATUS = 0;            // ERROR de compilación: es de solo lectura, te frena el compilador
> ```
> El `const` (sobre el dato) hace que el compilador **te avise si intentás escribir** un registro que el hardware no deja escribir; el `volatile` hace que **siempre lo lea de memoria** y no lo cachee en un registro de la CPU. Esta combinación `volatile const` la vas a ver mucho en headers de CMSIS. Lo desarrollamos en el módulo [08 - Tipos de ancho fijo y volatile](./08-tipos-de-ancho-fijo-y-volatile.md).

> **Para los curiosos (avanzado)**
>
> El orden importa: `const` aplica al tipo que tiene **a su izquierda**, salvo que sea lo primero, en cuyo caso aplica al de la derecha. Por eso `const uint32_t *p` y `uint32_t const *p` son **idénticos** (las dos formas son "puntero a uint32_t const"). En cambio, el `const` que está **después del `*`** siempre se refiere al puntero. Una forma infalible de no marearte: leé siempre de derecha a izquierda partiendo del nombre, y tratá al `*` como la palabra "puntero a".

---

## Punteros `void *` y casts

Un puntero `void *` es un **puntero genérico**: puede guardar la dirección de cualquier tipo, pero **no se puede desreferenciar directamente** (el compilador no sabe cuántos bytes leer ni cómo interpretarlos). Antes de usarlo hay que convertirlo (*cast*) al tipo concreto.

```c
int x = 42;
void *vp = &x;        // OK: void* acepta cualquier dirección

// *vp = 1;           // ERROR: no se puede desreferenciar un void*
int *ip = (int *)vp;  // cast al tipo correcto
printf("%d\n", *ip);  // OK: imprime 42
```

¿Para qué sirve? Para escribir funciones **genéricas** que trabajan con memoria sin importar el tipo. El ejemplo clásico es `memcpy`, que copia bytes crudos:

```c
void *memcpy(void *dest, const void *src, size_t n);
```

En embebido aparece muchísimo en los **callbacks**: un driver te deja registrar un puntero `void *` con "contexto del usuario" que te devuelve cuando llama tu función, sin que el driver tenga que saber qué tipo es (lo vemos en el capítulo de punteros a función).

> **PRECAUCIÓN**
>
> Convertir una dirección a un tipo con requisitos de **alineación** más estrictos es peligroso en Cortex-M3. Por ejemplo, castear un `uint8_t *` que apunta a una dirección impar hacia un `uint32_t *` y desreferenciarlo puede provocar un fallo o lectura incorrecta. Tratamos el acceso desalineado en detalle en el capítulo de estructuras.

---

## Punteros colgantes (*dangling pointers*)

Un **puntero colgante** es un puntero que apunta a memoria que **ya no es válida**. El caso más peligroso y frecuente en C: **devolver la dirección de una variable local**.

```c
int *funcion_rota(void) {
    int local = 42;
    return &local;     // MAL: local deja de existir al volver de la función
}                      // la dirección devuelta apunta a basura (stack reutilizado)
```

Cuando la función retorna, su *stack frame* se libera y `local` deja de existir. El puntero devuelto sigue teniendo la dirección, pero esa memoria se reutiliza para la siguiente llamada, así que leerla o escribirla es **comportamiento indefinido**.

Soluciones correctas:

```c
// Opción A: el llamador provee el espacio (patrón muy usado en embebido)
void funcion_ok(int *out) {
    *out = 42;         // escribimos en memoria del llamador
}

// Opción B: usar una variable static (vive todo el programa) -- ojo, no es reentrante
int *funcion_static(void) {
    static int valor = 42;
    return &valor;     // OK: 'static' no se destruye al volver
}
```

> **IMPORTANTE**
>
> Otras formas de quedar con un puntero colgante: usar memoria liberada con `free()` (lo vemos en el módulo [09 - Asignación dinámica](./09-asignacion-dinamica.md)), o guardar un puntero a un elemento de un buffer que después se reutiliza. La regla de oro: **un puntero nunca debe sobrevivir al dato al que apunta**.

---

**Anterior:** [04 - Funciones](./04-funciones.md) ·
**Siguiente:** [06 - Punteros avanzados](./06-punteros-avanzado.md)
