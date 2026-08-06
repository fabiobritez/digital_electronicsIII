# Funciones en C

Las funciones son **bloques de código reutilizables** que realizan una tarea específica. Son fundamentales para:

* Organizar y estructurar el código
* Evitar repetición (principio DRY: Don't Repeat Yourself)
* Facilitar el mantenimiento y depuración
* Crear abstracciones y modularizar el programa

---

## Anatomía de una función

```c
tipo_retorno nombre_funcion(tipo_param1 param1, tipo_param2 param2) {
    // cuerpo de la función
    return valor;  // opcional según el tipo de retorno
}
```

### Componentes:

1. **Tipo de retorno**: tipo de dato que devuelve la función (o `void` si no retorna nada)
2. **Nombre**: identificador de la función
3. **Parámetros**: lista de variables que recibe (pueden ser cero o más)
4. **Cuerpo**: código que se ejecuta cuando se llama la función
5. **`return`**: devuelve un valor al llamador (obligatorio si el tipo no es `void`)

---

## Ejemplo básico

```c
int sumar(int a, int b) {
    int resultado = a + b;
    return resultado;
}

int main(void) {
    int x = 5, y = 3;
    int suma = sumar(x, y);  // llamada a la función
    printf("Suma: %d\n", suma);  // imprime 8
    return 0;
}
```

---

## Declaración vs Definición

### Declaración (prototipo)

Indica al compilador que la función existe, pero no proporciona el código:

```c
int sumar(int a, int b);  // declaración/prototipo
```

### Definición

Proporciona el código real de la función:

```c
int sumar(int a, int b) {   // definición
    return a + b;
}
```

### ¿Por qué usar prototipos?

Permiten usar funciones antes de definirlas:

```c
// Prototipos al inicio
int sumar(int a, int b);
int restar(int a, int b);

int main(void) {
    int x = sumar(5, 3);
    int y = restar(10, 4);
    return 0;
}

// Definiciones después
int sumar(int a, int b) {
    return a + b;
}

int restar(int a, int b) {
    return a - b;
}
```

> En proyectos grandes, los prototipos van en archivos `.h` (headers) y las definiciones en `.c`.
> Por qué se separa así, qué va exactamente en cada archivo y qué significan los errores
> `undefined reference` y `multiple definition`, en
> [07 - El preprocesador](./07-preprocesador.md#por-qué-existen-los-h-cada-c-se-compila-solo).

### Por qué los prototipos importan de verdad (no es solo orden)

Un prototipo le dice al compilador **cuántos parámetros** lleva la función y **de qué tipo** son, además del tipo de retorno. Con esa información, el compilador puede:

- **Verificar que la llamés bien.** Si pasás un `float` donde se esperaba un `int*`, te lo marca como error en vez de generar código roto.
- **Convertir los argumentos correctamente.** Sin prototipo, no sabe a qué tipo convertir lo que le pasás.

¿Qué pasa si llamás una función **sin** haberla declarado antes? En C89 el compilador asumía un "prototipo implícito" que devolvía `int` y aceptaba cualquier cosa, una fuente enorme de bugs silenciosos. C99 eliminó esa regla del lenguaje. Ejemplo clásico que falla feo sin prototipo:

```c
// Sin incluir <math.h> ni declarar sqrt:
double r = sqrt(2.0);   // sin prototipo, el compilador podría asumir que sqrt
                        // devuelve int → te da basura, no 1.414...
```

> [!CAUTION]
> **Que el estándar lo prohíba no significa que el compilador te frene.** Por compatibilidad con
> código viejo, GCC lo sigue aceptando **con un simple warning**, incluso pidiendo `-std=c99`:
>
> ```console
> $ gcc -std=c99 -c prueba.c
> warning: implicit declaration of function 'sqrt' [-Wimplicit-function-declaration]
> ```
>
> Un warning que se pierde entre otros cincuenta es un bug que llega a la placa. (Recién GCC 14
> lo convirtió en error por defecto; con toolchains anteriores, que son los que vas a usar, es
> warning.)

**Por eso siempre incluís el header correspondiente** (`#include <math.h>`, `#include "sensor.h"`) antes de usar una función. Y compilá con `-Wall -Werror=implicit-function-declaration` para que esa llamada sin prototipo sea un **error** de compilación y no una sorpresa en runtime.

---

## Funciones sin retorno (`void`)

```c
void imprimir_mensaje(void) {
    printf("Hola mundo\n");
    // no hay return (o "return;" sin valor)
}

void saludar(char nombre[]) {
    printf("Hola, %s!\n", nombre);
}

int main(void) {
    imprimir_mensaje();
    saludar("Juan");
    return 0;
}
```

---

## Funciones sin parámetros

```c
int obtener_numero_aleatorio(void) {
    return 42;  // número "aleatorio" :)
}

int main(void) {
    int num = obtener_numero_aleatorio();
    printf("Número: %d\n", num);
    return 0;
}
```

> En C moderno se recomienda usar `void` explícitamente: `func(void)` en lugar de `func()`.

---

## Paso de parámetros

> [!NOTE]
> **De acá en adelante van a aparecer `*` y `&`.** Son los operadores de puntero, y el capítulo que
> los explica en serio es [08 - Punteros](./08-punteros.md), que viene después. Para seguir estas
> secciones alcanza con quedarse con tres ideas:
>
> * `&a` significa **"la dirección de `a`"**: en qué posición de memoria vive esa variable.
> * `int *p` declara una variable que **guarda una dirección** de un `int`, no un `int`.
> * `*p` significa **"el valor que hay en esa dirección"**.
>
> Con eso alcanza para entender por qué una función puede modificar una variable del que la llamó.
> El porqué completo, la aritmética de punteros y todos los casos borde están en el capítulo 08.

En C, los parámetros se pasan **por valor** por defecto. Esto significa que se hace una **copia** del argumento.

### Paso por valor

```c
void incrementar(int x) {
    x = x + 1;
    printf("Dentro: %d\n", x);  // 11
}

int main(void) {
    int a = 10;
    incrementar(a);
    printf("Fuera: %d\n", a);   // 10 (no cambió)
    return 0;
}
```

La función recibe una **copia** de `a`, por lo que modificar `x` no afecta a `a`.

---

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
[08 - Punteros](./08-punteros.md#const-y-punteros-const-correctness).

> Los parámetros que **no** son punteros no necesitan `const`: como se pasan por copia, modificarlos
> adentro no afecta a nadie. `void f(const int x)` es válido pero no aporta nada a quien llama.

---

## Retornar valores

### Retornar tipos básicos

```c
int maximo(int a, int b) {
    return (a > b) ? a : b;
}

float calcular_promedio(float a, float b) {
    return (a + b) / 2.0f;   // ojo el sufijo 'f': ver la nota de abajo
}
```

> **El sufijo `f` no es cosmético.** `2.0` sin sufijo es un `double`, así que `(a + b) / 2.0` hace la
> cuenta entera en 64 bits y recién después la trunca al `float` de retorno. En el Cortex-M3, que
> **no tiene FPU**, cada operación en `double` es una llamada a una rutina por software: pagás
> muchísimos ciclos por nada. Escribí `2.0f` y las constantes se quedan en 32 bits. El tema completo
> está en [15 - Punto fijo vs punto flotante](./15-punto-fijo-vs-flotante.md).

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

### Retornar estructuras

```c
typedef struct {
    int x;
    int y;
} Punto;

Punto crear_punto(int x, int y) {
    Punto p = {x, y};
    return p;  // se copia la estructura
}

int main(void) {
    Punto p1 = crear_punto(10, 20);
    printf("Punto: (%d, %d)\n", p1.x, p1.y);
    return 0;
}
```

---

### Todos los caminos tienen que retornar

Si una función declara que devuelve algo, **cada camino posible** tiene que terminar en un `return`
con valor. Si la ejecución llega al final sin uno, quien llamó se lleva **basura**: lo que haya
quedado en el registro `R0`. No es un valor indeterminado "cualquiera pero estable" — es
comportamiento indefinido, y cambia con el nivel de optimización.

```c
int clasificar(int t) {
    if (t > 100)  return 2;
    else if (t > 50) return 1;
    // ¿y si t <= 50? Cae al final SIN retornar nada
}
```

El compilador te avisa, pero solo si se lo pedís:

```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -c clasificar.c
warning: control reaches end of non-void function [-Wreturn-type]
```

Es uno de los warnings que **más conviene convertir en error** (`-Werror=return-type`): no hay
ningún caso legítimo en el que quieras que esto pase.

> El caso inverso también existe: una función `void` no puede hacer `return valor;`, y una función
> que no retorna nunca (un `panic()` que reinicia el micro) se marca con `noreturn` para que el
> compilador deje de pedirte un `return` que no tiene sentido. Está en la sección de atributos, más
> abajo.

---

## Funciones con arreglos

Un arreglo **nunca se copia** al pasarlo: lo que viaja es un puntero a su primer elemento. Por eso la
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

## Funciones recursivas

Una función puede llamarse a sí misma:

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;  // caso base
    }
    return n * factorial(n - 1);  // llamada recursiva
}

int main(void) {
    printf("5! = %d\n", factorial(5));  // 120
    return 0;
}
```

### El costo de la recursión en la pila (crítico en embebido)

Cada llamada a función reserva un **marco de pila (stack frame)**: espacio para sus parámetros, variables locales y la dirección de retorno. En recursión, esos marcos se **apilan**: `factorial(5)` tiene 5 marcos vivos al mismo tiempo.

En una PC con gigabytes de RAM eso no preocupa. En el **LPC1769 tus variables viven en 32 KB de SRAM principal** (hay 64 KB en total, pero los otros 32 son dos bloques separados en el bus AHB), y la pila es apenas una porción de eso. Si la recursión es profunda o el caso base falla, la pila **crece hasta pisar otras zonas de memoria** (stack overflow): el síntoma típico es que el micro se cuelga, se reinicia, o corrompe variables aparentemente al azar. Y no hay un sistema operativo que te avise: simplemente se rompe.

Por eso, en firmware:

- **Evitá la recursión** salvo que la profundidad esté acotada y sea chica y conocida.
- Cualquier recursión se puede reescribir como un **bucle** con una pila/array explícitos. El factorial, por ejemplo, es trivial de forma iterativa:

  ```c
  uint32_t factorial(uint32_t n) {
      uint32_t r = 1;
      for (uint32_t i = 2; i <= n; i++) {
          r *= i;
      }
      return r;
  }
  ```

  La versión iterativa usa **un solo** marco de pila sin importar `n`.

> **Ojo:** en sistemas embebidos, evitá recursión profunda: consume mucha pila y puede causar overflow. Cuando puedas, preferí la versión iterativa.

> **¿Qué es exactamente ese "marco de pila", y cómo sabés cuántos bytes usa cada función?** Tiene un
> capítulo entero: [10 - Dónde vive cada variable: stack, heap y
> estáticos](./10-donde-vive-cada-variable.md), donde se ve el prólogo y el epílogo en ensamblador
> real, el contrato AAPCS (qué argumentos viajan en registros y cuáles van a la pila) y cómo medir el
> stack con `-fstack-usage`.

---

## Valores de retorno y códigos de error

En C no hay excepciones como en otros lenguajes. La forma idiomática de reportar errores es **mediante el valor de retorno**. Hay dos convenciones muy comunes en firmware:

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
[04 - Control de flujo](./04-control-de-flujo.md#return) hace esto: devuelve el índice si lo encuentra, o `-1` si no:

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

## Funciones con número variable de argumentos

C permite funciones con argumentos variables usando `<stdarg.h>`:

```c
#include <stdarg.h>

int sumar_varios(int cantidad, ...) {
    va_list args;
    va_start(args, cantidad);

    int suma = 0;
    for (int i = 0; i < cantidad; i++) {
        suma += va_arg(args, int);
    }

    va_end(args);
    return suma;
}

int main(void) {
    printf("%d\n", sumar_varios(3, 10, 20, 30));  // 60
    printf("%d\n", sumar_varios(5, 1, 2, 3, 4, 5));  // 15
    return 0;
}
```

Ejemplo conocido: **`printf` es la función variádica por excelencia**. Por eso `printf("%d %s", n, txt)` puede tomar cantidades y tipos distintos de argumentos: el primer parámetro (la cadena de formato) le dice cuántos argumentos siguen y de qué tipo, y los va sacando con el mismo mecanismo `va_arg` que ves arriba.

> **Cuidado en embebido:** las funciones variádicas tienen costos ocultos. Los argumentos pasan por reglas especiales (los tipos chicos se promocionan: un `float` viaja como `double`, un `uint8_t` como `int`), y el compilador **no puede verificar los tipos** contra el formato. Un `%d` con un argumento que no es `int` es comportamiento indefinido. Además, `printf` completo es pesado en código y RAM; en micros chicos se usan versiones reducidas o se evita. Para tus propias APIs, casi siempre es mejor una función con parámetros fijos y bien tipados que una variádica.

---

## Punteros a funciones

El nombre de una función también es una dirección, así que podés guardarlo en una variable y llamar
a la función a través de ella. Eso es un **puntero a función**, y es la base de los *callbacks*, las
tablas de comandos y las máquinas de estado dirigidas por tabla: todo el patrón que usan los drivers
de verdad para avisarte de que llegó un dato.

Como necesita punteros, el tema completo (sintaxis, `typedef`, callbacks, tablas de dispatch y el uso
en ISRs) está en [09 - Punteros avanzados](./09-punteros-avanzado.md#punteros-a-función-y-callbacks).

---

## Funciones `inline`

Sugiere al compilador que inserte el código directamente (evita overhead de llamada):

```c
static inline int cuadrado(int x) {
    return x * x;
}

int main(void) {
    int y = cuadrado(5);  // puede ser reemplazado por: int y = 5 * 5;
    return 0;
}
```

> Útil para funciones pequeñas y muy usadas. En embebidos puede mejorar performance crítica.

`inline` es solo una **sugerencia**: el compilador puede ignorarla.

> [!WARNING]
> **Escribí `static inline`, no `inline` a secas.** Un `inline` sin `static` **no define la función**:
> aporta una "definición en línea" que el compilador puede usar si le conviene, pero si decide *no*
> expandirla (por ejemplo con `-O0`, que es como compilás en debug), la llamada queda buscando un
> símbolo que nadie definió:
>
> ```console
> $ gcc -O0 -std=c99 cuadrado.c -o cuadrado
> /usr/bin/ld: undefined reference to `cuadrado'
> collect2: error: ld returned 1 exit status
> ```
>
> Es un error desconcertante porque **compila bien y falla recién al linkear**, y solo con ciertos
> niveles de optimización. Con `static inline` el problema no existe. Las semánticas finas de
> `inline`, `static inline` y `extern inline` (y por qué en los headers de firmware siempre vas a ver
> la segunda) están en [`static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md).

---

## Funciones `static`

Una función `static` solo es **visible dentro del archivo** donde se define:

```c
// archivo: sensor.c

static int leer_registro(int addr) {
    // función privada, solo para este archivo
    return 0;
}

int leer_sensor(void) {
    // función pública
    return leer_registro(0x10);
}
```

Ventajas:

* Encapsulación: oculta detalles de implementación
* Evita conflictos de nombres entre archivos
* El compilador puede optimizar mejor

> En firmware bien organizado, **todo lo que no sea parte de la API pública de un módulo se declara `static`**. Esto se trata más a fondo, junto con `static` aplicado a variables y a la visibilidad entre archivos, en [`static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md).

---

## Convenciones en sistemas embebidos

### Inicialización de periféricos

```c
void GPIO_Init(void) {
    // configurar pines como entrada/salida
}

void UART_Init(uint32_t baudrate) {
    // configurar comunicación serial
}
```

---

### Lectura/escritura de hardware

```c
uint16_t ADC_Read(uint8_t canal) {
    uint16_t valor = 0;
    // leer valor del convertidor analógico-digital
    return valor;
}

void LED_Set(uint8_t pin, uint8_t estado) {
    // encender/apagar LED
}
```

> El retorno es `uint16_t` y no `uint8_t` porque **el ADC del LPC1769 es de 12 bits**: los valores van
> de 0 a 4095 y no entran en un byte. Elegir mal el tipo de retorno de una función de driver trunca
> los datos en silencio. Lo vemos en [10 - ADC y DAC](../10_adc_dac/).

---

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

## Buenas prácticas

### Usar nombres descriptivos

```c
// Malo
int f(int x) { ... }

// Bueno
int calcular_temperatura_celsius(int adc_value) { ... }
```

---

### Funciones pequeñas y enfocadas

Cada función debe hacer **una cosa bien**.

```c
// Función que hace demasiado
void procesar_sensor_y_actualizar_display_y_enviar_uart(void) {
    // ...
}

// Mejor: dividir en funciones más pequeñas
void leer_sensor(void) { ... }
void actualizar_display(int valor) { ... }
void enviar_uart(int valor) { ... }

void procesar_sensores(void) {
    int valor = leer_sensor();
    actualizar_display(valor);
    enviar_uart(valor);
}
```

---

### Validar parámetros

```c
int dividir(int a, int b) {
    if (b == 0) {
        return -1;  // indicar error
    }
    return a / b;
}
```

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

## Alcance (scope) de variables

### Variables locales

Existen solo dentro de la función:

```c
void funcion(void) {
    int x = 10;  // local
}  // x se destruye aquí

int main(void) {
    // printf("%d", x);  // ERROR: x no existe aquí
    return 0;
}
```

---

### Variables `static` locales

Conservan su valor entre llamadas:

```c
void contador(void) {
    static int count = 0;  // se inicializa solo una vez
    count++;
    printf("Llamada #%d\n", count);
}

int main(void) {
    contador();  // Llamada #1
    contador();  // Llamada #2
    contador();  // Llamada #3
    return 0;
}
```

---

### Variables globales

Accesibles desde cualquier función:

```c
int temperatura_actual = 0;  // global

void actualizar_temperatura(int nueva) {
    temperatura_actual = nueva;
}

int obtener_temperatura(void) {
    return temperatura_actual;
}
```

> **Ojo:** Minimizá el uso de globales: dificultan el testing y el entendimiento del código.

---

## Ejemplo completo: módulo de sensor

```c
// sensor.h
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void Sensor_Init(void);
uint16_t Sensor_Read(void);
float Sensor_GetTemperature(void);

#endif

// sensor.c
#include "sensor.h"

static uint16_t ultimo_valor = 0;  // variable privada

void Sensor_Init(void) {
    // Configurar ADC, pines, etc.
}

uint16_t Sensor_Read(void) {
    // Leer valor del ADC
    ultimo_valor = ADC_Read(0);
    return ultimo_valor;
}

float Sensor_GetTemperature(void) {
    // Convertir ADC a temperatura
    return (float)ultimo_valor * 0.0625f;  // ejemplo
}

// main.c
#include "sensor.h"
#include <stdio.h>

int main(void) {
    Sensor_Init();

    uint16_t raw = Sensor_Read();
    float temp = Sensor_GetTemperature();

    printf("Temperatura: %.2f°C\n", temp);

    return 0;
}
```

---

## Para los curiosos (avanzado)

> Esta sección es opcional. Si recién empezás, saltala sin culpa y volvé cuando quieras profundizar.

### Atributos de función de GCC

GCC permite "anotar" funciones con `__attribute__((...))` para darle información extra al compilador. Algunos muy usados en firmware:

```c
// Esta función nunca retorna (ej: un handler de error que reinicia el micro).
// El compilador deja de avisar "control reaches end of non-void function"
// y puede optimizar el código que viene después de la llamada.
__attribute__((noreturn)) void panic(void);

// No emitir warning si el parámetro/función no se usa (común en callbacks).
void Timer_Callback(void *ctx __attribute__((unused)));

// Handler de interrupción "naked": sin prólogo/epílogo automático.
__attribute__((naked)) void HardFault_Handler(void);

// Colocar la función/variable en una sección concreta del linker (ej: RAM).
__attribute__((section(".fast_code"))) void rutina_critica(void);

// Alinear, empaquetar, forzar que se mantenga aunque parezca no usada, etc.
__attribute__((weak)) void Default_Handler(void);   // símbolo "débil", redefinible
```

El `weak` es especialmente importante: los handlers de interrupción por defecto del LPC1769 se declaran `weak` para que vos puedas **redefinirlos** simplemente escribiendo una función con el mismo nombre, sin tocar el archivo de arranque. Lo vas a ver en el módulo de interrupciones y de build/linker.

### Tail-call (llamada de cola)

Si lo **último** que hace una función es llamar a otra y devolver su resultado (`return otra(x);`), el compilador puede reusar el marco de pila actual en vez de apilar uno nuevo: es la **optimización de llamada de cola**. Con `-O2`, GCC convierte una recursión "de cola" en un bucle, eliminando el riesgo de stack overflow. **Pero no te apoyes en esto** en firmware: no está garantizado por el estándar y depende del nivel de optimización. Si necesitás un bucle, escribí un bucle.

### `_Generic` (C11): "sobrecarga" según el tipo

C no tiene sobrecarga de funciones como C++, pero desde C11 `_Generic` permite elegir una expresión según el **tipo** de un argumento, en tiempo de compilación. Se usa para construir macros con apariencia de función genérica:

```c
#define abs_val(x) _Generic((x),        \
        int:    abs,                    \
        long:   labs,                   \
        float:  fabsf,                  \
        double: fabs                    \
    )(x)

int    a = abs_val(-5);     // usa abs
double b = abs_val(-5.0);   // usa fabs
```

Es una herramienta de bibliotecas, rara vez la vas a escribir vos, pero es útil reconocerla.

---

## Resumen

| Concepto | Descripción |
|----------|-------------|
| **Declaración** | Prototipo que indica existencia de la función |
| **Definición** | Código real de la función |
| **Paso por valor** | Se copia el argumento (no modifica original) |
| **Paso "por referencia"** | Se pasa un puntero por valor (puede modificar original) |
| **Parámetro `const`** | Promesa verificada de que la función no modifica lo apuntado |
| **`static inline`** | Sugiere insertar el código; `inline` a secas no linkea |
| **Reentrante** | Sobrevive a que la llamen de nuevo desde una ISR |
| **`static`** | Función visible solo en el archivo actual |
| **Recursión** | Función que se llama a sí misma |
| **Punteros a funciones** | Variables que apuntan a funciones (callbacks) |
| **Variables locales** | Existen solo dentro de la función |
| **Variables `static` locales** | Conservan valor entre llamadas |

---

**Reglas de oro:**

1. Una función, una responsabilidad
2. Nombres claros y descriptivos
3. Validar parámetros de entrada
4. Evitar efectos secundarios ocultos
5. Documentar funciones públicas
6. Minimizar uso de variables globales
7. En embebidos: cuidado con recursión profunda y uso de pila
8. `const` en todo parámetro puntero que no modifiques
9. Todos los caminos de una función con retorno tienen que retornar
10. Si una ISR la puede llamar, revisá que sea reentrante

**Los flags que atrapan los errores de este capítulo:**

```make
CFLAGS += -Wall -Werror=implicit-function-declaration -Werror=return-type
```

El primero convierte en error la llamada sin prototipo; el segundo, la función con retorno que se
olvida de retornar. `-Wall` ya trae `-Wsizeof-array-argument`, que atrapa el `sizeof` de un
parámetro arreglo.

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf).
  Cláusulas relevantes: 6.9.1 (definiciones de función), 6.7.6.3 (declaradores de función y el
  ajuste de parámetros arreglo a puntero), 6.5.2.2 (llamadas y conversión de argumentos), 6.7.4
  (`inline` y por qué una definición en línea no es una definición externa) y 7.16 (`<stdarg.h>`).
- [cppreference: Functions](https://en.cppreference.com/w/c/language/functions). Incluye la tabla de
  qué implica `inline`, `static inline` y `extern inline`.
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., capítulo 4 (*Functions and Program
  Structure*).

**GCC y el toolchain**

- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). `-Wreturn-type`,
  `-Wimplicit-function-declaration` y `-Wsizeof-array-argument`, los tres citados acá.
- [GCC: Common Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html).
  `noreturn`, `weak`, `unused`, `naked` y `section`, los de la sección para curiosos.
- [GCC: An Inline Function is As Fast As a Macro](https://gcc.gnu.org/onlinedocs/gcc/Inline.html).
  La explicación oficial de por qué `inline` a secas puede dejarte sin símbolo al linkear.

**Sobre los temas puntuales**

- [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md). El marco de pila de cada
  llamada, el contrato AAPCS y cómo medir cuánto stack usa cada función.
- [14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md). Las
  semánticas finas de `static` e `inline`.
- [09 - Punteros avanzados](./09-punteros-avanzado.md#punteros-a-función-y-callbacks). Punteros a
  función, callbacks y tablas de dispatch.

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.5.2.2 (llamadas a función), 6.7.6.3 (declaradores de función y por qué `int arr[5]` como parámetro es `int *`), 6.9.1 (definiciones de función), 7.16 (`<stdarg.h>`, funciones variádicas).
- [cppreference: functions](https://en.cppreference.com/w/c/language/functions). Referencia práctica, con las reglas de `inline` que se profundizan en el capítulo 14.

**GCC y el toolchain**

- [GCC: Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html). Los `__attribute__` del bloque "para los curiosos": `noreturn`, `weak`, `naked`, `section`, `unused`.
- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). `-Werror-implicit-function-declaration` y `-Wreturn-local-addr`, los dos que atrapan los errores de este capítulo.

**ARM y el LPC1769**

- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). Define el contrato de una llamada: los cuatro primeros argumentos van en `r0`-`r3`, el resto por stack, y el valor de retorno en `r0`. El detalle, con el desensamblado al lado, está en [10 - El contrato entre funciones: AAPCS](./10-donde-vive-cada-variable.md#el-contrato-entre-funciones-aapcs).

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md) ·
**Siguiente:** [07 - El preprocesador](./07-preprocesador.md)
