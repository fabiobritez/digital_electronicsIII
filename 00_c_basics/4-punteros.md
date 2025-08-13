# Punteros


Primero veremos qué son los punteros y cómo usarlos. Luego profundizaremos en los operadores de puntero (como `&` y `*`), la aritmética de punteros y cómo interactúan con distintos tipos de datos, arreglos y estructuras. También abordaremos temas avanzados como punteros a punteros, punteros a `void`, punteros a funciones y asignación dinámica de memoria. En el camino, se indican buenas prácticas y errores comunes. Al final, se debería tener una comprensión completa de los punteros en C y de cómo utilizarlos de forma efectiva.


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


> ⚠️ **IMPORTANTE**
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

Una vez que un puntero tiene una dirección válida, puedes usar el operador de desreferencia `*` para acceder o modificar el dato en esa dirección.
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

Los punteros no se usan solo para variables individuales; también puedes hacer operaciones aritméticas con ellos para moverte por la memoria. Esto es especialmente útil con arreglos (tema que veremos después).  

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
No puedes multiplicar, dividir o sumar dos punteros. Estas operaciones no tienen sentido conceptual para direcciones y C generará un error de compilación.

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