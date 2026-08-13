# Arreglos, strings, punteros avanzados y callbacks

Ya sabemos qué es un puntero y cómo usar los operadores `&` y `*`. Ahora podemos estudiar tres
aplicaciones que aparecen continuamente en código real:

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

### Índices contra aritmética de punteros: medir, no adivinar

C5 inicializó un buffer mediante índices. Ahora podemos escribir la alternativa y comparar el código
generado sin presentar los punteros antes de tiempo.

La misma idea con aritmética de punteros, que vas a ver escrita así en mucho código:

```c
uint8_t *ptr = buffer;
for (size_t i = 0; i < sizeof buffer; i++) {
    *ptr++ = 0;
}
```

> **Las dos versiones no se diferencian en velocidad.** Es común leer que la de punteros "es más
> eficiente"; era cierto con los compiladores de los años 80. Hoy GCC con `-O2` genera para las dos
> un lazo con una sola instrucción de escritura:
>
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O2 -S limpiar.c -o -
> con índice:   strb  r1, [r3, #1]!
> con puntero:  strb  r2, [r0], #1
> ```
>
> Elegí la que se lea mejor, que casi siempre es la del índice. Y para el caso puntual de llenar un
> buffer, lo más claro y lo más rápido es no escribir el bucle: `memset(buffer, 0, sizeof buffer);`
> de `<string.h>`, que el compilador reemplaza por una rutina optimizada.

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

Así se implementa el mismo contrato a mano. Usamos otro nombre porque redefinir una función de la
biblioteca estándar produce comportamiento indefinido:

```c
#include <stddef.h>

size_t mi_strlen(const char s[]) {
    size_t i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}
```

El retorno es `size_t` y el parámetro es `const` porque la función no modifica los caracteres. La
sintaxis `s[]` en un parámetro se ajusta a `const char *`, como muestra la sección de *decay*.

> **Ojo con el tipo de retorno:** `strlen` devuelve `size_t` (sin signo), **no `int`**. Si la declarás
> mal, el compilador te frena con *conflicting types*; y si comparás su resultado con un `int`
> negativo, te comés el bug de comparaciones mixtas de
> [C2 - Conversiones](./02-expresiones-operadores-conversiones.md#conversión-de-tipos).

---

### Cuidado con los literales de cadena

Si hacés:
```c
char *msg = "Hello";
```
`msg` apuntará a un **literal de cadena**, que está en una zona de memoria **de solo lectura**.
Intentar modificarlo (ej., `msg[0] = 'J';`) es **comportamiento indefinido**.

Compilá con `-Wwrite-strings` para que GCC diagnostique desde el principio las asignaciones de un
literal a un `char *` modificable.

Fijate que hay dos cosas en dos lugares distintos: **el puntero `msg` sí vive en RAM** (es una variable
como cualquier otra), pero **lo apuntado vive en `.rodata`, o sea en la Flash**. El detalle de qué
sección es cada cosa está en
[C10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md#recorrido-dónde-cae-cada-declaración).

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
> [C12 - Structs para hardware](./12-layout-alineacion-unions-y-bitfields.md).

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
[C2 - Operadores](./02-expresiones-operadores-conversiones.md#mini-tabla-de-precedencia-que-más-muerde-en-embebido).

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
   > [C10B - Asignación dinámica](./10b-asignacion-dinamica.md)) y una matriz estática contigua es más
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

## Contratos de error con parámetros de salida

Este desarrollo estaba en C4. Se ubica acá porque el patrón completo combina el valor de retorno con
un puntero donde la función deja el dato útil.

### Convención 1: la función devuelve un código de estado

La función retorna un `int` (o un `enum`) que indica éxito o el tipo de error, y los datos "útiles" salen por punteros de salida:

```c
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERR_TIMEOUT,
    SENSOR_ERR_CRC,
} sensor_status_t;

// El resultado sale por 'out'; el return informa si salió bien
sensor_status_t Sensor_Leer(uint16_t *out) {
    if (!dato_listo())      return SENSOR_ERR_TIMEOUT;
    uint16_t v = leer_raw();
    if (!crc_ok(v))         return SENSOR_ERR_CRC;
    *out = v;
    return SENSOR_OK;
}

// Uso
uint16_t valor;
if (Sensor_Leer(&valor) != SENSOR_OK) {
    // manejar el error
}
```

Usar `0` para "éxito" es la convención más extendida (permite escribir `if (funcion() != 0)` para "hubo error").

### Convención 2: valor válido + un valor "centinela" para el error

Cuando todos los valores válidos dejan libre alguno (típicamente negativos), se devuelve ese valor centinela para señalar error. La función `encontrar` de
[C3 - Control de flujo](./03-control-de-flujo.md#return) hace esto: devuelve el índice si lo encuentra, o `-1` si no:

```c
int encontrar(const int arr[], size_t cantidad, int valor) {
    for (size_t i = 0; i < cantidad; i++)
        if (arr[i] == valor) return (int) i;
    return -1;   // centinela: "no encontrado"
}
```

Es compacto, pero solo sirve si tenés un valor que **nunca** es un resultado legítimo. Si todos los valores posibles son válidos, usá la Convención 1.

> **Regla:** elegí una convención por módulo y mantenela. Y **siempre chequeá** el código de retorno: ignorar el error de una función de hardware es una de las causas más comunes de bugs intermitentes en firmware.

---

## Funciones que reciben y devuelven punteros

C4 estableció que C pasa todos los argumentos por valor. Ahora que ya conocemos direcciones, esa
regla se puede completar sin atajos: una función también recibe **una copia del puntero**, pero a
través de ella puede leer o modificar el objeto original. Cada interfaz debe declarar además si el
puntero puede ser `NULL`, qué tamaño tiene el objeto apuntado y quién conserva su propiedad.

### Paso por referencia (simulado con punteros)

Para modificar el valor original, se pasa un **puntero**:

```c
void incrementar(int *x) {
    *x = *x + 1;
    printf("Dentro: %d\n", *x);  // 11
}

int main(void) {
    int a = 10;
    incrementar(&a);  // pasar dirección de a
    printf("Fuera: %d\n", a);    // 11 (cambió!)
    return 0;
}
```

Ahora la función puede modificar el valor original a través del puntero.

> **En C no existe el "paso por referencia" de verdad.** Lo que ves arriba sigue siendo paso por valor: lo que se copia es el **puntero** (la dirección). La función recibe una copia de esa dirección y, a través de ella, llega a la variable original. Es paso por valor de un puntero. La diferencia con C++ (que sí tiene referencias `&`) es importante: en C, si querés modificar algo del llamador, **siempre** es vía puntero explícito (`&` al pasar, `*` para acceder).

---

### Parámetros `const`: decir qué vas a tocar y qué no

Cuando una función recibe un puntero, quien la llama se queda con una duda razonable: **¿me va a
modificar el dato?** El `const` en el parámetro responde eso, y el compilador lo hace cumplir.

```c
// Promete NO modificar el buffer: solo lo lee
void uart_enviar(const uint8_t *buf, size_t len);

// Sí lo modifica: acá se escribe lo recibido
void uart_recibir(uint8_t *buf, size_t len);
```

Las dos firmas se leen distinto de un vistazo, y no es solo documentación: si dentro de
`uart_enviar` alguien escribe `buf[0] = 0;`, **no compila**. Es una promesa verificada.

La regla práctica es simple: **todo parámetro puntero que la función no modifique va `const`**. Es
gratis, documenta la intención y permite que el compilador optimice mejor. La mecánica completa de
`const` con punteros (la diferencia entre `const uint8_t *p` y `uint8_t * const p`) está en
[C8 - Punteros](./08-punteros.md#const-y-punteros-const-correctness).

> Los parámetros que **no** son punteros no necesitan `const`: como se pasan por copia, modificarlos
> adentro no afecta a nadie. `void f(const int x)` es válido pero no aporta nada a quien llama.

---

### Retornar punteros

```c
char *obtener_saludo(void) {
    static char mensaje[] = "Hola!";  // IMPORTANTE: static
    return mensaje;
}

int main(void) {
    char *msg = obtener_saludo();
    printf("%s\n", msg);
    return 0;
}
```

> **NUNCA** retornes un puntero a una variable local (no estática):

```c
// INCORRECTO
char *obtener_saludo_malo(void) {
    char mensaje[] = "Hola!";  // local (se destruye al salir)
    return mensaje;  // ¡PELIGRO! puntero a memoria no válida
}
```

---

## Diseño de funciones con buffers

Esta sección aplica el mecanismo anterior al contrato de una función. Un arreglo **nunca se copia** al pasarlo: lo que viaja es un puntero a su primer elemento. Por eso la
función trabaja sobre el arreglo original del llamador (sigue siendo paso por valor, pero de una
dirección: lo mismo que vimos recién con `int *`).

```c
void imprimir_arreglo(const int arr[], size_t cantidad) {
    for (size_t i = 0; i < cantidad; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void llenar_con_ceros(int arr[], size_t cantidad) {
    for (size_t i = 0; i < cantidad; i++) {
        arr[i] = 0;  // modifica el arreglo original
    }
}

int main(void) {
    int numeros[5] = {1, 2, 3, 4, 5};
    imprimir_arreglo(numeros, 5);

    llenar_con_ceros(numeros, 5);
    imprimir_arreglo(numeros, 5);  // 0 0 0 0 0
    return 0;
}
```

> Siempre se debe pasar la cantidad como parámetro separado, ya que la función no puede determinarla.
> Y fijate el `const` en `imprimir_arreglo`: declara que esa función **no toca** el arreglo, mientras
> que `llenar_con_ceros` sí. Es una diferencia que el compilador verifica; se desarrolla en
> [Parámetros `const`](#parámetros-const-decir-qué-vas-a-tocar-y-qué-no).

### Por qué el arreglo "decae" a puntero

Cuando pasás un arreglo a una función, **no se copia el arreglo entero**: lo que se pasa es la dirección de su primer elemento. Se dice que el arreglo **decae (decays) a un puntero**. Por eso estos tres prototipos son **idénticos** para el compilador:

```c
void f(int arr[10]);   // el "10" se ignora por completo
void f(int arr[]);     // exactamente lo mismo
void f(int *arr);      // ...y esto también
```

Consecuencias prácticas que tenés que tener clarísimas en embebido:

1. **Dentro de la función, `sizeof(arr)` te da el tamaño de un puntero (4 bytes en el M3), NO el del arreglo.** El truco `sizeof(arr)/sizeof(arr[0])` para contar elementos **solo funciona en el scope donde se declaró el arreglo**, nunca dentro de una función que lo recibió. Por eso se pasa el tamaño aparte.

   ```c
   void procesar(uint8_t buf[]) {
       size_t n = sizeof(buf);   // ¡4, no el tamaño del buffer! BUG clásico
   }
   ```

   La buena noticia es que este no te lo tenés que acordar: **`-Wall` lo detecta**.

   ```console
   $ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -c procesar.c
   warning: 'sizeof' on array function parameter 'buf' will return size of 'uint8_t *'
            [-Wsizeof-array-argument]
   ```

2. Como la función recibe la dirección real, **puede modificar el contenido del arreglo del llamador** (no una copia). Eso es lo que aprovecha `llenar_con_ceros` más arriba.

---

### Documentar funciones

```c
/**
 * @brief Calcula el promedio de un arreglo de enteros
 * @param arr      Puntero al arreglo (la función no lo modifica)
 * @param cantidad Número de elementos
 * @return Promedio como float, o 0.0f si el arreglo está vacío
 */
float calcular_promedio(const int arr[], size_t cantidad) {
    if (cantidad == 0) return 0.0f;

    int suma = 0;
    for (size_t i = 0; i < cantidad; i++) {
        suma += arr[i];
    }
    return (float)suma / (float)cantidad;
}
```

---

### Evitar efectos secundarios ocultos

```c
// Efecto secundario oculto
int contador_global = 0;
int obtener_siguiente(void) {
    contador_global++;  // modifica estado global
    return contador_global;
}

// Mejor: explícito
int obtener_siguiente(int *contador) {
    (*contador)++;
    return *contador;
}
```

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

Combinando enums ([C6 - Estructuras y enumeraciones](./06-estructuras-y-enums.md)) con punteros a función podés escribir una máquina de estados sin un `switch` enorme: cada estado es una función que devuelve el próximo estado.

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
> [C6 - `enum`, `switch` y estado](./06-estructuras-y-enums.md#uso-en-sistemas-embebidos-máquina-de-estados),
> escrita como tabla. Las dos formas se comparan en
> [Máquinas de estado](./arquitectura/18-maquinas-de-estado.md).

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
> [C11 - `volatile` y tipos para hardware](./11-c-para-hardware.md). Además, muchos
> drivers reales reciben también un `void *contexto` que te devuelven en el callback, para que no
> tengas que usar variables globales (ahí entra el `void *` del
> [capítulo anterior](./08-punteros.md#punteros-void--y-casts)).

> **Para los curiosos (avanzado): la tabla de vectores**
>
> El propio Cortex-M3 usa esta idea a nivel hardware. Al principio de la Flash hay una **tabla de vectores de interrupción**: un arreglo de punteros a función. Cuando ocurre una interrupción, el procesador toma el puntero correspondiente de esa tabla y salta a tu *handler*. O sea: registrar una ISR es, literalmente, poner un puntero a función en una tabla. Por eso `UART0_IRQHandler` tiene que llamarse exactamente así: el *startup* lo coloca en la posición correcta del vector. Se ve completo en [07 - NVIC y vectores](../07_interrupciones/01-nvic-y-vectores.md).

---

## Callbacks, interrupciones y reentrancia

### Callbacks de interrupciones

```c
void UART_RxCallback(uint8_t dato) {
    // se llama cuando llega un byte por UART
}

void Timer_Callback(void) {
    // se llama cada vez que el timer expira
}
```

---

### Funciones reentrantes: el problema que aparece con las interrupciones

Una interrupción puede caer **en cualquier instrucción**, incluso en el medio de una de tus
funciones. Si la ISR llama a esa misma función, hay dos ejecuciones vivas a la vez. Una función que
soporta eso sin romperse se llama **reentrante**.

Lo que rompe la reentrancia es **el estado compartido entre llamadas**: variables globales y
`static` locales. Las variables locales comunes no dan problema, porque cada llamada tiene su propio
marco de pila.

```c
// NO reentrante: las dos ejecuciones se pisan el mismo contador
static int llamadas = 0;
int siguiente_id(void) {
    llamadas++;          // si la ISR entra justo acá, se pierde una cuenta
    return llamadas;
}

// Reentrante: todo el estado es local o del llamador
int siguiente_id_r(int *contador) {
    return ++(*contador);
}
```

Dos consecuencias concretas en firmware:

- **La función que devuelve un puntero a un `static`** (como `obtener_saludo` más arriba) no es
  reentrante: dos llamadas devuelven **la misma dirección**, así que la segunda pisa el resultado de
  la primera.
- **Varias funciones de la biblioteca estándar tampoco lo son.** El caso clásico es `strtok`, que
  guarda estado interno entre llamadas. Tampoco conviene llamar a `printf` desde una ISR: además de
  no ser reentrante en todas las implementaciones, se lleva cientos de bytes de pila.

El tema completo —qué pasa cuando la ISR y el `main` comparten una variable, por qué hace falta
`volatile` y cuándo hay que deshabilitar interrupciones— está en
[07 - Secciones críticas y atomicidad](../07_interrupciones/03-secciones-criticas-y-atomicidad.md).

---

## Patrones embebidos y casos límite

La mecánica básica ya quedó establecida. Estas son las diferencias que importan al diseñar una API o
leer código de firmware real.

### Un arreglo no es un puntero

> [!IMPORTANT]
> **Un arreglo NO es un puntero.** Es una simplificación que se dice mucho pero que induce a error. `arr` es de tipo `int[5]`; lo que pasa es que en la mayoría de las expresiones se convierte a `int *`. Hay **tres excepciones** donde el arreglo *no* decae y se ve la diferencia:
>
> ```c
> int arr[5];
> int *ptr = arr;
>
> // 1) operando de sizeof
> sizeof(arr);        // 20 → el arreglo entero. sizeof(ptr) daría 4
>
> // 2) operando de & (y de _Alignof)
> &arr;               // tipo int(*)[5] → puntero a arreglo de 5. &ptr es int**
>
> // 3) literal de cadena que inicializa un arreglo
> char s[] = "Hola";  // copia los 5 bytes en s; NO apunta al literal
> ```
>
> Otra diferencia concreta: un puntero es una **variable** que ocupa sus 4 bytes en memoria y podés reasignar (`ptr = otro;`). El nombre del arreglo no es una variable reasignable: `arr = otro;` no compila. De ahí viene la analogía con "puntero constante", pero el arreglo además **no ocupa memoria propia** para guardar la dirección: la dirección *es* dónde está el arreglo.

---

### Uso en sistemas embebidos

El ejemplo combina arreglos con `static`, `const` y `volatile` para representar tablas y registros.
La sintaxis de punteros ya se desarrolló en C8; C11 precisará las garantías de `volatile` para MMIO.

```c
// Buffer para UART (en RAM, sin inicializar → va a .bss, arranca en cero)
static uint8_t rx_buffer[256];

// Tabla de lookup: al ser static const va a .rodata, o sea Flash. No gasta RAM.
static const uint16_t adc_to_temp[256] = { /* ... */ };

// Arreglo de punteros a los registros FIOxSET de los 5 puertos del LPC1769
// (UM10360 tabla 106: FIO0SET..FIO4SET, de 0x2009C018 a 0x2009C098, cada
//  puerto separado 0x20)
static volatile uint32_t * const gpio_set[5] = {
    (volatile uint32_t *) 0x2009C018,   // P0
    (volatile uint32_t *) 0x2009C038,   // P1
    (volatile uint32_t *) 0x2009C058,   // P2
    (volatile uint32_t *) 0x2009C078,   // P3
    (volatile uint32_t *) 0x2009C098    // P4
};

// Prender el pin `pin` del puerto `puerto`
void set_pin(uint8_t puerto, uint8_t pin) {
    *gpio_set[puerto] = (1u << pin);
}
```

> [!NOTE]
> Fijate en el tipo del último arreglo: `volatile uint32_t * const gpio_set[5]` es un **arreglo de punteros constantes a `uint32_t` volátiles**. El `volatile` va sobre el *registro apuntado* (que cambia por hardware), no sobre el arreglo. Declarar `volatile uint32_t gpio_ports[4] = {0x...}` sería un arreglo de *números* volátil, que no es lo que querés: son direcciones fijas, no datos que cambien. Y al ser `const`, el arreglo de punteros también se va a Flash.

---

### Limitaciones importantes

1. **Tamaño fijo**: una vez declarado, no se puede cambiar el tamaño
2. **No se puede asignar directamente**: `arr1 = arr2;` es **inválido**. Para copiar, `memcpy(arr1, arr2, sizeof arr1);` (de `<string.h>`)
3. **No se puede retornar un arreglo desde una función**: el lenguaje no admite tipos de retorno de arreglo, ni siquiera se puede escribir. Y si devolvés un *puntero* a un arreglo local, el arreglo ya murió al salir de la función: es **comportamiento indefinido** (GCC avisa con `-Wreturn-local-addr`). Las salidas son pasar un buffer del llamador, o usar `static`
4. **Sin verificación de límites**: accesos fuera de rango no generan error
5. **No se comparan con `==`**: `if (arr1 == arr2)` compara *direcciones*, no contenido. Para contenido, `memcmp()`

---

### Cuánto verifica el compilador en un parámetro arreglo

> *Cuánto te ayuda el compilador acá depende de su versión.* Con el `arm-none-eabi-gcc` 9 del repo, pasarle un `int chico[2]` a un parámetro `int arr[5]` pasa **sin un solo warning**. Los GCC modernos (11 en adelante) sí lo miran: usan el tamaño escrito como una promesa y avisan si la llamada no la cumple.
>
> ```console
> $ gcc-13 -Wall -c parametros.c
> warning: 'g' accessing 20 bytes in a region of size 8 [-Wstringop-overflow=]
> ```
>
> No te confíes igual: el chequeo solo funciona cuando el compilador ve el tamaño del arreglo original en el mismo archivo, y desaparece por completo si escribís `int arr[]` sin número. **Pasar el tamaño como parámetro aparte sigue siendo la única forma robusta.**

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

- [C10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md). Por qué un literal de cadena está
  en Flash y una copia local en RAM.
- [C12 - Structs para hardware](./12-layout-alineacion-unions-y-bitfields.md). Los punteros a `struct` de este
  capítulo, aplicados a mapear los registros del micro.
- [Máquinas de estado](./arquitectura/18-maquinas-de-estado.md). La tabla de punteros
  a función llevada a la arquitectura de un firmware completo.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [C8 - Punteros](./08-punteros.md) ·
**Siguiente:** [C10A - Dónde vive cada variable](./10-donde-vive-cada-variable.md)
