# Punteros avanzados: arreglos, cadenas y punteros a función

En [08 - Punteros](./08-punteros.md) viste qué es un puntero, los operadores `&` y `*` y la aritmética
de punteros. Este capítulo usa todo eso para las tres cosas en las que más se apoya el código real:

1. **Arreglos**, y la relación con punteros que hace que pasar un arreglo a una función no copie nada.
2. **Cadenas**, que en C no son un tipo sino un arreglo de `char` con una convención.
3. **Punteros a función**, la base de los *callbacks* de cualquier driver y de la tabla de vectores
   del propio Cortex-M3.

---

## Punteros y arreglos

En C, los arreglos y los punteros están estrechamente relacionados.
En muchas expresiones, **el nombre de un arreglo se convierte automáticamente** (o *"decay"*) **en un puntero a su primer elemento**.

Por ejemplo, si tienes:
```c
int arr[10];
```
entonces, en la mayoría de los contextos, `arr` puede usarse como si fuera de tipo `int*` apuntando a `arr[0]`.
De hecho, `arr` y `&arr[0]` devuelven la **misma dirección**.
Podrías hacer:
```c
int *p = arr;
```
y entonces `p` apuntará al inicio del arreglo. Después de esa asignación:

- `p[0]` es igual a `arr[0]`
- `*(p + 1)` es igual a `arr[1]`
- y así sucesivamente.

Esto es muy útil para **iterar sobre arreglos** con aritmética de punteros o **pasar arreglos a funciones** (ya que los parámetros de tipo arreglo en realidad reciben punteros).

> **IMPORTANTE**
>
> Aunque `arr` (en una expresión) actúa como un puntero al primer elemento, hay una distinción clave:
>
> - `arr` **no** es un *lvalue* modificable → no podés hacer `arr = otroArray;`
>   (en su declaración, `arr` es una referencia constante a un bloque fijo de memoria).
> - En cambio, un puntero como `p` **sí** puede reasignarse para apuntar a otro lado.

En resumen: los arreglos "se comportan como" punteros en expresiones, pero **no son completamente intercambiables**.
Aun así, entender que `arr` puede tratarse como un puntero a su primer elemento es fundamental.

---

### Ejemplo

```c
int numbers[3] = {5, 10, 15};
int *ptr = numbers;            // ptr apunta a numbers[0]

printf("%d\n", ptr[2]);        // imprime 15 (ptr[2] == *(ptr + 2))
ptr[1] = 20;                   // modifica numbers[1] a 20
printf("%d\n", numbers[1]);    // imprime 20, reflejando el cambio
printf("%d\n", *numbers);      // imprime 5 (numbers[0]), desreferencia del arreglo
```

Aquí, `ptr[2]` y `numbers[2]` se refieren al **mismo elemento**.
El estándar de C define que `ptr[i]` es equivalente a `*(ptr + i)`.

Incluso se puede hacer algo como `2[numbers]` porque es igual a `*(2 + numbers)` → mismo que `numbers[2]`.
Pero **no lo hagas nunca en código real**; solo sirve para ilustrar que el índice `x[y]` se interpreta como `*(x + y)`.

---

### Patrón común de iteración con punteros

```c
#define N 8
int arr[N];
int *end = arr + N;   // apunta a una posición más allá del último elemento

for (int *p = arr; p < end; ++p) {
    printf("%d\n", *p);
}
```

Este bucle recorre el arreglo desde el inicio (`arr`) hasta `end` (sin incluirlo), desreferenciando en cada paso para imprimir el valor.
Evita usar una variable de índice y puede ser muy eficiente.

> El puntero "uno más allá del último elemento" (`arr + N`) es un caso especial que el estándar
> **permite calcular y comparar**, aunque no se puede desreferenciar. Es lo que hace que este patrón
> sea legal. Un `arr + N + 1` ya sería comportamiento indefinido, aunque nunca lo desreferencies.

---

## Arreglos como parámetros: el *decay* en acción

Cuando pasás un arreglo a una función, **no se copia el arreglo**: se copia solo la dirección de su primer elemento (el *decay* que vimos arriba). Por eso estas tres firmas son **exactamente equivalentes**:

```c
void procesar(int datos[10]);   // el "10" lo IGNORA el compilador
void procesar(int datos[]);     // igual que la anterior
void procesar(int *datos);      // igual: lo que llega es un int*
```

Esto tiene una consecuencia que sorprende a todos al principio: **dentro de la función, `sizeof(datos)` NO te da el tamaño del arreglo**, sino el tamaño de un puntero (4 bytes en Cortex-M3).

```c
void procesar(int datos[]) {
    // sizeof(datos) == 4 (¡es un puntero!), NO el tamaño del arreglo original
}

int main(void) {
    int v[10];
    // aquí sí: sizeof(v) == 40 (10 ints * 4 bytes)
}
```

> **IMPORTANTE**
>
> Como la función no sabe cuántos elementos hay, **siempre tenés que pasar la longitud por separado**. Es el patrón universal en C embebido (lo vas a ver en cada driver):
> ```c
> void uart_send(const uint8_t *buf, uint32_t len);
> ```
> El `const` ([capítulo anterior](./08-punteros.md#const-y-punteros-const-correctness)) documenta que la función no modifica el buffer; `len` le dice cuántos bytes hay.

---

## Arreglos multidimensionales

Un arreglo 2D en C se almacena en memoria **fila por fila** (*row-major*), de forma contigua:

```c
int m[2][3] = {
    {1, 2, 3},
    {4, 5, 6}
};
// En memoria: 1 2 3 4 5 6 (seis ints seguidos)
```

`m[i][j]` es, internamente, `*(&m[0][0] + i*3 + j)`. El número de columnas (3) es parte del tipo: el compilador lo necesita para saltar de fila en fila. Por eso, al pasar una matriz a una función, **podés omitir la primera dimensión pero no las demás**:

```c
void imprimir(int filas, int mat[][3]) {   // la cantidad de columnas es obligatoria
    for (int i = 0; i < filas; i++)
        for (int j = 0; j < 3; j++)
            printf("%d ", mat[i][j]);
}
```

> En microcontroladores las matrices estáticas de tamaño fijo (como esta) son comunes y eficientes. Las matrices dinámicas con `int **` (punteros a punteros, más abajo) son raras en embebido porque fragmentan la memoria y agregan saltos extra; se prefiere un único arreglo contiguo.

---

## Punteros y cadenas

En C, las **cadenas** se representan típicamente como **arreglos de `char` terminados en el carácter nulo** (`'\0'`).
Dado que los arreglos y punteros están relacionados, una cadena puede manipularse fácilmente con un `char*`.

Ejemplo:
```c
char str[] = "Hello";  // arreglo de chars (H, e, l, l, o, '\0')
char *p = str;         // p apunta a 'H'

while (*p) {           // recorre hasta encontrar '\0'
    printf("%c\n", *p);
    p++;
}
```

Muchas funciones estándar (como `strlen`, `strcpy`, etc.) usan `char*` para procesar cadenas.
Por ejemplo:
```c
size_t strlen(const char *s);
```
recibe un `const char*` apuntando a la cadena y recorre la memoria hasta encontrar `'\0'`.

> **Ojo con el tipo de retorno:** `strlen` devuelve `size_t` (sin signo), **no `int`**. Si la declarás
> mal, el compilador te frena con *conflicting types*; y si comparás su resultado con un `int`
> negativo, te comés el bug de comparaciones mixtas de
> [02 - Conversiones](./02-arreglos-conversiones-y-promociones.md#conversión-de-tipos).

---

### Cuidado con los literales de cadena

Si hacés:
```c
char *msg = "Hello";
```
`msg` apuntará a un **literal de cadena**, que está en una zona de memoria **de solo lectura**.
Intentar modificarlo (ej., `msg[0] = 'J';`) es **comportamiento indefinido**.

Fijate que hay dos cosas en dos lugares distintos: **el puntero `msg` sí vive en RAM** (es una variable
como cualquier otra), pero **lo apuntado vive en `.rodata`, o sea en la Flash**. El detalle de qué
sección es cada cosa está en
[10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md#recorrido-dónde-cae-cada-declaración).

Si necesitás modificar la cadena, se copia primero a un arreglo (esto sí genera una copia en la RAM):
```c
char msg[] = "Hello";  // ahora se puede modificar
msg[0] = 'J';          // válido
```

Es la misma diferencia que en la tabla del capítulo 10: `char *msg = "..."` es un puntero en RAM a
Flash; `char msg[] = "..."` es un arreglo en RAM que el startup rellena copiando desde Flash.

**Ventaja de usar punteros con cadenas:**
Permite manejo dinámico, paso eficiente de parámetros a funciones y manipulación directa de la memoria subyacente.
La iteración puede hacerse con índices (`str[i]`) o aritmética de punteros (`*(str + i)`), según la preferencia y el caso de uso.

---

## Funciones de cadena de la biblioteca estándar

Como las cadenas son arreglos de `char` terminados en `'\0'`, la biblioteca `<string.h>` ofrece funciones para manipularlas. Las más usadas:

| Función | Qué hace |
|---------|----------|
| `strlen(s)` | Cantidad de caracteres antes del `'\0'` (no cuenta el `'\0'`) |
| `strcpy(dst, src)` | Copia `src` en `dst` (incluido el `'\0'`) |
| `strncpy(dst, src, n)` | Copia **exactamente** `n` bytes; leé la advertencia de abajo |
| `strcmp(a, b)` | Compara: 0 si son iguales |
| `strcat(dst, src)` | Concatena `src` al final de `dst` |
| `memcpy(dst, src, n)` | Copia `n` bytes crudos (no mira `'\0'`) |
| `memset(dst, val, n)` | Pone `n` bytes con el valor `val` |

```c
char destino[20];
strcpy(destino, "LPC1769");
printf("Largo: %u\n", (unsigned)strlen(destino));  // 7
```

> **PRECAUCIÓN: desbordes de buffer**
>
> `strcpy` y `strcat` **no chequean el tamaño del destino**. Si la cadena origen es más larga que el buffer, escribís fuera de él y corrompés memoria (la causa número uno de bugs y vulnerabilidades en C). En sistemas embebidos, donde no hay protección de memoria que te avise, no las uses con datos que vengan de afuera.

> [!WARNING]
> **`strncpy` no es "el `strcpy` seguro".** Es una función vieja, pensada para otra cosa, y tiene dos
> comportamientos que sorprenden:
>
> - **Si el origen no entra en `n` bytes, el destino queda SIN el `'\0'` final.** Lo que te queda no
>   es una cadena, y el próximo `strlen` o `printf("%s")` se va de largo leyendo memoria ajena.
> - **Si el origen es más corto que `n`, rellena todo el resto con ceros**, así que copiar 4 letras
>   en un buffer de 256 escribe los 256 bytes.
>
> ```c
> char dst[8];
> strncpy(dst, "1234567890", sizeof dst);   // dst = "12345678", ¡sin '\0'!
> printf("%s\n", dst);                      // lee más allá del buffer
> ```
>
> GCC con `-Wall` te lo marca, y vale la pena hacerle caso:
>
> ```console
> warning: 'strncpy' output truncated copying 8 bytes from a string of length 10
>          [-Wstringop-truncation]
> ```
>
> Si la usás, terminá vos la cadena a mano (`dst[sizeof dst - 1] = '\0';`). En la práctica, para
> armar texto conviene **`snprintf`**, que siempre termina en `'\0'` y te dice cuánto habría
> necesitado.

---

## Punteros y estructuras

> Acá va la mecánica: cómo se declara un puntero a `struct` y cómo se accede a sus campos. El uso que
> le da el firmware (mapear registros del micro, controlar el *padding*, uniones) está en
> [13 - Structs para hardware](./13-structs-para-hardware.md).

Los punteros también pueden apuntar a tipos estructurados (`struct` o `union`).
Un puntero a una estructura permite:

- **Pasar estructuras grandes de forma eficiente** (se pasa solo la dirección en lugar de copiar toda la estructura).
- **Crear estructuras enlazadas** (como listas, árboles, etc., donde cada nodo contiene un puntero al siguiente).

Ejemplo de declaración:
```c
struct Point {
    int x;
    int y;
};

struct Point p1;            // declaración de una estructura tipo Point

struct Point *pPtr = &p1;   // declaración de un puntero a la estructura p1
```

Aquí, `pPtr` es un puntero a `struct Point`.

---

### Acceso a miembros de estructura con punteros

En C, para acceder a los miembros de una estructura a través de un puntero se usa el **operador flecha `->`**.

* `pPtr->x` accede al campo `x` de la estructura a la que `pPtr` apunta.
* `pPtr->x` es equivalente a `(*pPtr).x` (se desreferencia el puntero y luego se accede al campo).

El operador `->` hace el código más legible. Y los paréntesis de `(*pPtr).x` **no son opcionales**:
como `.` tiene más precedencia que `*`, escribir `*pPtr.x` significa `*(pPtr.x)`, que ni siquiera
compila. Es la trampa de precedencia de
[03 - Operadores](./03-operadores.md#mini-tabla-de-precedencia-que-más-muerde-en-embebido).

Ejemplo:

```c
struct Point p1 = {2, 3};
p1.x = 10; // se puede modificar el valor de x e y directamente
p1.y = 20;

struct Point *pPtr = &p1;

printf("%d, %d\n", pPtr->x, pPtr->y);  // imprime "10, 20"

pPtr->x = 11;                          // también se puede modificar mediante el puntero

printf("%d\n", p1.x);                  // imprime "11", reflejando el cambio
```

> El operador `->` funciona tanto para punteros a estructuras como a *unions*.

---

### Uso común en estructuras enlazadas

Los punteros a estructuras se usan intensamente en estructuras de datos dinámicas:
por ejemplo, en una lista enlazada cada nodo contiene un puntero al siguiente.

También son útiles al pasar estructuras a funciones:
en lugar de pasar la estructura por valor (lo que copia todo su contenido), se pasa un puntero para ganar eficiencia.

---

## Punteros a punteros (multinivel)

Así como podemos tener un puntero a un `int` o a un `char`, podemos tener un puntero **a otro puntero**.

Un puntero a puntero (o doble puntero) se declara con un `*` adicional:

```c
int **pp;   // pp es un puntero a un int*
```

---

### Desglose con ejemplo

```c
int x = 5;        // x es un int
int *p = &x;      // p es un puntero a int (contiene la dirección de x)
int **pp = &p;    // pp es un puntero a puntero a int (contiene la dirección de p)
```

En este escenario:

* `*pp` es de tipo `int*` → es el puntero `p`.
* `**pp` es de tipo `int` → es el valor al que apunta `p` (o sea, `x`).

```c
printf("%d\n", **pp); // imprime 5
```

---

### ¿Para qué se usan los punteros a punteros?

1. **Arreglos 2D dinámicos:**
   Para reservar memoria manualmente para una matriz bidimensional:

   ```c
   int **matrix;
   matrix = malloc(rows * sizeof(int*));
   for (int i = 0; i < rows; i++) {
       matrix[i] = malloc(cols * sizeof(int));
   }
   ```

   > En firmware esto casi no se usa: `malloc` se evita (ver
   > [11 - Asignación dinámica](./11-asignacion-dinamica.md)) y una matriz estática contigua es más
   > rápida y predecible. Lo incluimos porque lo vas a ver en código de PC.

2. **Pasar punteros a funciones para modificarlos:**
   Si querés que una función asigne memoria y devuelva el puntero a través de un parámetro:

   ```c
   void allocateArray(int **p, int size) {
       *p = malloc(size * sizeof(int));
   }

   int *arr;
   allocateArray(&arr, 10); // arr queda inicializado en main
   ```

   Es el mismo motivo por el que se pasa `int *` cuando se quiere modificar un `int`: **para
   modificar algo desde una función hace falta su dirección**, y si ese algo ya es un puntero, su
   dirección es un puntero a puntero.

3. **Argumentos de línea de comandos (`argv`):**
   En `main(int argc, char **argv)`, `argv` es un puntero a puntero a `char` (arreglo de cadenas).
   En un micro sin sistema operativo `main` no recibe argumentos, así que este caso no aparece.

---

### Ejemplo práctico

```c
int a = 100;
int *p = &a;
int **pp = &p;

printf("%d\n", **pp);  // imprime 100

*pp = NULL;            // cambia p a NULL
```

Aquí, `*pp = NULL;` modifica el puntero `p` (ya que `pp` apunta a `p`).
Después de esa línea, `p` es `NULL`.

Esto demuestra que un puntero a puntero permite **manipular el puntero original** (no solo el valor al que apunta) desde otra función o contexto.

Podés tener más niveles (`***` para triple puntero, etc.), pero rara vez se necesitan a menos que trabajes con datos muy complejos o arreglos multidimensionales.
El caso más común en C es el **doble puntero**.

---

## Punteros a función y *callbacks*

Esta sección es **clave para programar microcontroladores**. Hasta ahora los punteros apuntaban a *datos*. Pero en C también podés tener un puntero que apunte a **código**: a una función. Esto es la base de los *callbacks*, las tablas de comandos y las máquinas de estado dirigidas por tablas.

### Sintaxis

Una función tiene un *tipo* determinado por lo que devuelve y por sus parámetros. Un puntero a función reproduce esa firma:

```c
// puntero a "función que recibe (int, int) y devuelve int"
int (*p_op)(int, int);
```

Los paréntesis alrededor de `(*p_op)` son **obligatorios**: sin ellos, `int *p_op(int,int)` sería "función que devuelve `int *`", que es otra cosa completamente distinta.

Para asignarlo, simplemente usás el nombre de la función (que, igual que un arreglo, *decae* a un puntero):

```c
int sumar(int a, int b) { return a + b; }

int (*p_op)(int, int) = sumar;   // p_op apunta a sumar
int r = p_op(3, 4);              // llamar a través del puntero: r == 7
// también vale (*p_op)(3, 4); ambas formas son equivalentes
```

Como las firmas se vuelven ilegibles rápido, casi siempre se usa `typedef`:

```c
typedef int (*Operacion)(int, int);   // Operacion = "puntero a función (int,int)->int"

Operacion op = sumar;
int r = op(5, 6);                     // r == 11
```

### Pasar un puntero a función a otra función: el *callback*

Un **callback** es una función que vos le pasás a otra para que la llame "cuando corresponda". Es la forma de **inyectar comportamiento** sin que la función receptora sepa de antemano qué vas a hacer.

```c
// Recorre el arreglo y aplica 'accion' a cada elemento
void para_cada(int *arr, int n, void (*accion)(int)) {
    for (int i = 0; i < n; i++) {
        accion(arr[i]);          // llamamos al callback
    }
}

void imprimir(int x) { printf("%d\n", x); }

int main(void) {
    int v[3] = {10, 20, 30};
    para_cada(v, 3, imprimir);   // pasamos la función como argumento
}
```

### Tabla de punteros a función: *dispatch* de comandos

En vez de un `switch` gigante, podés tener un **arreglo de punteros a función** indexado por un comando. Es más compacto, más rápido de extender y muy usado para parsear protocolos.

```c
typedef void (*CmdHandler)(void);

void cmd_led_on(void)  { /* prender LED */ }
void cmd_led_off(void) { /* apagar LED */ }
void cmd_reset(void)   { /* reiniciar  */ }

// La posición en la tabla ES el código de comando
CmdHandler tabla[] = {
    cmd_led_on,    // comando 0
    cmd_led_off,   // comando 1
    cmd_reset      // comando 2
};
#define N_CMDS (sizeof(tabla) / sizeof(tabla[0]))

void ejecutar(uint8_t cmd) {
    if (cmd < N_CMDS && tabla[cmd] != NULL) {   // ¡validar siempre el índice!
        tabla[cmd]();                           // dispatch
    }
}
```

> **PRECAUCIÓN**
>
> Antes de llamar a través de un puntero a función **siempre** verificá que (1) el índice esté dentro de la tabla y (2) el puntero no sea `NULL`. Llamar a un puntero a función inválido salta a una dirección arbitraria: en Cortex-M3 eso dispara un *HardFault* y reinicia (o cuelga) el micro.

### Máquina de estados dirigida por tabla

Combinando enums ([05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md)) con punteros a función podés escribir una máquina de estados sin un `switch` enorme: cada estado es una función que devuelve el próximo estado.

```c
typedef enum { ST_INIT, ST_IDLE, ST_ACTIVE, N_ESTADOS } Estado;
typedef Estado (*FuncEstado)(void);

Estado en_init(void)   { /* ... */ return ST_IDLE; }
Estado en_idle(void)   { /* ... */ return ST_ACTIVE; }
Estado en_active(void) { /* ... */ return ST_IDLE; }

FuncEstado maquina[N_ESTADOS] = { en_init, en_idle, en_active };

int main(void) {
    Estado estado = ST_INIT;
    while (1) {
        estado = maquina[estado]();   // ejecuta el estado y obtiene el siguiente
    }
}
```

> Es la misma máquina de estados del `switch` de
> [04 - Control de flujo](./04-control-de-flujo.md#uso-en-sistemas-embebidos-máquina-de-estados),
> escrita como tabla. Las dos formas se comparan en
> [Máquinas de estado](./18-maquinas-de-estado.md).

### Callbacks en drivers e ISRs (el caso real del embebido)

Acá está el motivo por el que todo esto importa. Un driver bien hecho **no sabe** qué querés hacer cuando llega un dato o se cumple un timer: te deja **registrar tu propia función**. Cuando ocurre el evento (normalmente dentro de una **interrupción**, ISR), el driver llama tu callback.

```c
// --- En el driver ---
typedef void (*RxCallback)(uint8_t dato);

static volatile RxCallback rx_cb = NULL;   // arranca sin callback

void uart_on_receive(RxCallback cb) {
    rx_cb = cb;                     // el usuario registra su handler
}

// La ISR de UART corre cuando llega un byte por hardware
void UART0_IRQHandler(void) {
    uint8_t b = UART_LeerByte();
    RxCallback cb = rx_cb;          // copia local: evita que cambie entre el chequeo y la llamada
    if (cb != NULL) {               // siempre chequear antes de llamar
        cb(b);                      // avisamos al usuario
    }
}

// --- En el código de aplicación ---
void mi_handler(uint8_t dato) {
    /* procesar el byte recibido */
}

int main(void) {
    uart_on_receive(mi_handler);    // registramos nuestro callback
    // ... el resto sigue; mi_handler se llama solo cuando llega un byte
}
```

> **IMPORTANTE**
>
> El puntero al callback (`rx_cb` arriba) lo comparten la ISR y el código principal, y por eso va
> `volatile`: sin él, el compilador puede leerlo una sola vez y no enterarse de que la aplicación lo
> cambió. La copia local dentro de la ISR es el otro lado del mismo problema: si leyeras `rx_cb` dos
> veces (una para el `!= NULL` y otra para llamar), podría cambiar entre medio. El detalle de
> `volatile` y la concurrencia con interrupciones está en
> [12 - `volatile` y tipos para hardware](./12-volatile-y-tipos-para-hardware.md). Además, muchos
> drivers reales reciben también un `void *contexto` que te devuelven en el callback, para que no
> tengas que usar variables globales (ahí entra el `void *` del
> [capítulo anterior](./08-punteros.md#punteros-void--y-casts)).

> **Para los curiosos (avanzado): la tabla de vectores**
>
> El propio Cortex-M3 usa esta idea a nivel hardware. Al principio de la Flash hay una **tabla de vectores de interrupción**: un arreglo de punteros a función. Cuando ocurre una interrupción, el procesador toma el puntero correspondiente de esa tabla y salta a tu *handler*. O sea: registrar una ISR es, literalmente, poner un puntero a función en una tabla. Por eso `UART0_IRQHandler` tiene que llamarse exactamente así: el *startup* lo coloca en la posición correcta del vector. Se ve completo en [07 - NVIC y vectores](../07_interrupciones/01-nvic-y-vectores.md).

---

## Resumen

| Si escribís… | Lo que realmente tenés |
|---|---|
| `int arr[10];` y después `arr` en una expresión | un `int *` al primer elemento (*decay*) |
| `void f(int a[10])` | exactamente `void f(int *a)`: el 10 se ignora y `sizeof(a)` es 4 |
| `char *msg = "Hola";` | puntero en RAM a una cadena en Flash: **no la modifiques** |
| `char msg[] = "Hola";` | copia propia en RAM: se puede modificar |
| `int m[2][3];` | 6 `int` contiguos; el 3 es parte del tipo |
| `int **pp;` | puntero a puntero: sirve para modificar un puntero desde otra función |
| `int (*p)(int, int);` | puntero a función; sin los paréntesis sería otra cosa |

**Las reglas para no equivocarse:**

1. Un arreglo pasado a una función **pierde su tamaño**: pasá la longitud siempre.
2. Un literal de cadena vive en Flash. `char *` a un literal es **de solo lectura**.
3. `strcpy`/`strcat` no miran el tamaño del destino, y `strncpy` no siempre termina en `'\0'`:
   para armar texto usá `snprintf`.
4. Antes de llamar a un puntero a función, chequeá el índice **y** el `NULL`.
5. Un puntero a callback compartido con una ISR va `volatile`, y se copia a una local antes de usarlo.

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf).
  Cláusulas relevantes: 6.3.2.1 (conversión de arreglo a puntero, el *decay*), 6.5.2.1 (indexado, que
  define `a[i]` como `*(a+i)`), 6.7.6.3 (los parámetros de tipo arreglo se ajustan a puntero),
  6.4.5 (literales de cadena y por qué modificarlos es UB) y 7.24 (`<string.h>`).
- [cppreference: Array to pointer conversion](https://en.cppreference.com/w/c/language/array).
- [cppreference: `strncpy`](https://en.cppreference.com/w/c/string/byte/strncpy). Documenta los dos
  comportamientos del recuadro de advertencia: el relleno con ceros y la falta de terminador.
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., capítulo 5 completo (*Pointers and
  Arrays*). §5.11 es la introducción clásica a los punteros a función.

**Sobre los temas puntuales**

- [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md). Por qué un literal de cadena está
  en Flash y una copia local en RAM.
- [13 - Structs para hardware](./13-structs-para-hardware.md). Los punteros a `struct` de este
  capítulo, aplicados a mapear los registros del micro.
- [Máquinas de estado](./18-maquinas-de-estado.md). La tabla de punteros
  a función llevada a la arquitectura de un firmware completo.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [08 - Punteros](./08-punteros.md) ·
**Siguiente:** [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md)
