# Control de Flujo en C

El control de flujo permite que un programa tome decisiones y repita tareas. En C tenemos:

* **Estructuras de decisión**: `if`, `else`, `else if`, `switch`
* **Bucles**: `while`, `for`, `do-while`
* **Saltos**: `break`, `continue`, `return`, `goto`

---

## 1. Estructuras de decisión: `if`, `else`, `else if`

### Sintaxis básica

```c
if (condición) {
    // se ejecuta si condición es verdadera (≠ 0)
}
```

Si la condición es verdadera (cualquier valor distinto de cero), se ejecuta el bloque.

### `if-else`

```c
if (condición) {
    // se ejecuta si condición es verdadera
} else {
    // se ejecuta si condición es falsa
}
```

### `if-else if-else`

```c
if (condición1) {
    // bloque 1
} else if (condición2) {
    // bloque 2
} else {
    // bloque por defecto
}
```

---

### Ejemplos

```c
int temperatura = 85;

if (temperatura > 100) {
    printf("Sobrecalentado\n");
} else if (temperatura > 75) {
    printf("Temperatura alta\n");
} else {
    printf("Temperatura normal\n");
}
```

En sistemas embebidos:

```c
uint8_t sensor_value = ADC_Read();

if (sensor_value > THRESHOLD_HIGH) {
    LED_On();
    Alarm_Trigger();
} else if (sensor_value < THRESHOLD_LOW) {
    LED_Blink();
} else {
    LED_Off();
}
```

---

### Las dos trampas clásicas del `if`

Estas dos son responsables de una cantidad desproporcionada de bugs, y las dos vienen del mismo lugar:
en C, `if` acepta **cualquier expresión** y **una sola sentencia**.

#### Trampa 1: `=` en lugar de `==`

```c
if (estado = LISTO) {    // ¡asigna, no compara!
    ...
}
```

Como `=` es un operador que **devuelve el valor asignado**, esto guarda `LISTO` en `estado` y después
pregunta si ese valor es distinto de cero. Si `LISTO` no es 0, el `if` entra **siempre**, y encima te
pisó la variable. Compila sin errores porque es una expresión perfectamente válida.

Por suerte acá el compilador sí ayuda, y con `-Wall` alcanza:

```console
$ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -c estado.c
warning: suggest parentheses around assignment used as truth value [-Wparentheses]
```

Si la asignación es a propósito (pasa, por ejemplo con `while ((c = leer()) != EOF)`), poné
paréntesis dobles o una comparación explícita, y el warning se calla porque le mostraste que era
intencional.

#### Trampa 2: el `else` colgante (*dangling else*)

Sin llaves, un `else` se pega **al `if` más cercano**, no al que sugiere la indentación:

```c
if (hay_energia)
    if (boton_apretado())
        arrancar();
else                        // la indentación miente:
    apagar_todo();          // este else es del if INTERNO, no del externo
```

O sea que `apagar_todo()` corre cuando hay energía y **no** se apretó el botón, que es probablemente
lo contrario de lo que querías. GCC lo detecta con `-Wdangling-else`, incluido en `-Wall`:

```console
warning: suggest explicit braces to avoid ambiguous 'else' [-Wdangling-else]
```

**Las dos trampas desaparecen con la misma regla: poné siempre las llaves**, incluso para una sola
línea. Es la recomendación de [la sección 8](#usar-llaves--siempre) y esta es la razón.

---

### Operador ternario (`? :`): cuándo usarlo en lugar de `if-else`

La sintaxis y la precedencia del ternario ya las vimos en
[03 - Operadores](./03-operadores.md#operador-ternario-condicional). Acá interesa la pregunta de
control de flujo: **cuándo conviene y cuándo no.**

La diferencia de fondo es que `if-else` es una **sentencia** y `? :` es una **expresión**: produce un
valor. Por eso el ternario sirve donde una sentencia no entra:

```c
// 1) Inicializar una variable const, que solo se puede escribir una vez
const uint32_t divisor = usar_alta_velocidad ? DIV_RAPIDO : DIV_LENTO;

// 2) Dentro de una llamada, sin variable temporal
UART_Enviar(exito ? "OK\r\n" : "ERROR\r\n");

// 3) En un inicializador de arreglo o struct, donde no podés poner un if
uint8_t config[2] = { modo_pwm ? 0x0F : 0x00, 0x20 };
```

> Ojo con el caso 3: eso vale porque `config` es una variable **local**. Si le agregaras `static` (o
> fuera global), el inicializador tendría que ser una **expresión constante**.

En el caso 1, con `if-else` tendrías que sacarle el `const` a la variable para poder asignarla
después, y perdés la garantía. Ese es el argumento fuerte a favor del ternario.

**Cuándo NO usarlo:** si las dos ramas son *acciones* en vez de *valores*, usá `if-else`. Esto es
válido en C pero es difícil de leer y no aporta nada:

```c
(temperatura > LIMITE) ? Alarma_On() : Alarma_Off();   // preferí un if-else
```

Y no lo anides: `a ? b : c ? d : e` es legal (el `? :` asocia de derecha a izquierda, así que es
`a ? b : (c ? d : e)`), pero a la tercera condición ya nadie lo entiende. Ahí va un `if-else if` o un
`switch`.

> [!IMPORTANT]
> **Las dos ramas tienen que dar tipos compatibles, y el tipo del resultado sale de las dos.** Igual
> que en cualquier operación, se aplican las conversiones aritméticas usuales de
> [02 - Conversiones](./02-arreglos-conversiones-y-promociones.md#conversión-de-tipos). Esto sorprende:
>
> ```c
> uint8_t a = 1;
> int32_t b = -1;
> printf("%d\n", (cond) ? a : b);   // el tipo de TODA la expresión es int32_t,
>                                   // aunque se elija la rama del uint8_t
> ```
>
> Y si mezclás `unsigned` con `signed` acá, te comés el mismo bug de comparaciones mixtas del
> capítulo 02, pero sin que ninguna comparación esté a la vista.

---

## 2. Estructura `switch`

Permite elegir entre múltiples casos basados en el valor de una expresión entera (o carácter).

### Sintaxis

```c
switch (expresión) {
    case valor1:
        // código para valor1
        break;
    case valor2:
        // código para valor2
        break;
    default:
        // código si ningún caso coincide
        break;
}
```

---

### Características importantes

* La expresión debe ser de tipo **entero** (`int`, `char`, `enum`...). No sirven `float`, `double`, punteros ni cadenas.
* Cada `case` lleva una **expresión constante entera**, evaluable en tiempo de compilación: un literal, un `#define` o una constante de `enum`.
* **`break`** es necesario para evitar que se ejecuten los siguientes casos (*fall-through*).
* **`default`** es opcional, pero recomendado para manejar valores inesperados.
* Los valores de los `case` no se pueden repetir dentro del mismo `switch`, y el orden en que los escribís no importa.

> [!CAUTION]
> **Una variable `const` NO sirve como etiqueta de `case`.** En C, `const int N = 8;` declara una
> *variable* que prometés no modificar, no una constante de compilación, y el compilador necesita el
> valor antes de que el programa exista:
>
> ```c
> const int N = 8;
> switch (x) {
>     case N: break;   // error: case label does not reduce to an integer constant
> }
> ```
>
> Para etiquetas de `case` usá `#define` o, mejor, `enum`. El tema completo está en
> [01 - Declaraciones y tipos](./01-declaraciones-y-tipos.md#3-calificador-de-tipo).

---

### Ejemplo básico

```c
int opcion = 2;

switch (opcion) {
    case 1:
        printf("Opción 1 seleccionada\n");
        break;
    case 2:
        printf("Opción 2 seleccionada\n");
        break;
    case 3:
        printf("Opción 3 seleccionada\n");
        break;
    default:
        printf("Opción no válida\n");
        break;
}
```

---

### Fall-through intencional

A veces queremos que varios casos ejecuten el mismo código:

```c
char c = 'a';

switch (c) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
        printf("Es una vocal\n");
        break;
    default:
        printf("Es una consonante\n");
        break;
}
```

Apilar varios `case` sin código entre ellos (como `'a'`...`'u'`) **es** fall-through intencional y es perfectamente válido: todos caen en el mismo bloque.

### Fall-through accidental: el `break` olvidado

El problema es cuando el fall-through **no** era tu intención. Si te olvidás un `break`, la ejecución "se cae" al siguiente `case`:

```c
switch (comando) {
    case CMD_ENCENDER:
        encender_motor();
        // ¡falta el break!
    case CMD_APAGAR:
        apagar_motor();   // se ejecuta SIEMPRE que llegue CMD_ENCENDER
        break;
}
```

Acá, al recibir `CMD_ENCENDER`, primero encendés el motor y a continuación lo apagás, porque sin `break` la ejecución sigue al `case` de abajo. Bugs de este tipo son difíciles de ver.

Cuando el fall-through es **a propósito**, dejalo documentado con un comentario para que se note que no es un olvido:

```c
case CMD_ENCENDER:
    encender_motor();
    /* fall-through */     // intencional: también queremos resetear contadores
case CMD_RESET:
    resetear_contadores();
    break;
```

Compilá con `-Wimplicit-fallthrough` (lo incluye `-Wextra`) y GCC te avisa de cada fall-through que no esté marcado con ese comentario.

### ¿Por qué conviene siempre poner `default`?

En firmware, una variable de estado o un comando pueden tomar un valor inesperado (memoria corrupta, un bit que se voló, un bug en otra parte). El `default` es tu red de seguridad: te deja **detectar y recuperarte** de lo imprevisto en vez de no hacer nada en silencio.

```c
switch (estado) {
    case IDLE:    ... break;
    case RUNNING: ... break;
    default:
        // valor imposible: registramos y reiniciamos a un estado seguro
        log_error("estado invalido");
        estado = IDLE;
        break;
}
```

---

### Uso en sistemas embebidos: máquina de estados

```c
enum State { IDLE, RUNNING, PAUSED, ERROR };
enum State estado = IDLE;

switch (estado) {
    case IDLE:
        // inicializar sistema
        if (start_button_pressed()) {
            estado = RUNNING;
        }
        break;

    case RUNNING:
        // ejecutar tarea principal
        if (pause_button_pressed()) {
            estado = PAUSED;
        } else if (error_detected()) {
            estado = ERROR;
        }
        break;

    case PAUSED:
        // esperar
        if (resume_button_pressed()) {
            estado = RUNNING;
        }
        break;

    case ERROR:
        // manejar error
        LED_Error();
        estado = IDLE;
        break;

    default:
        // estado no reconocido → reiniciar
        estado = IDLE;
        break;
}
```

> `switch` es ideal para **máquinas de estados**, muy comunes en firmware embebido.

El patrón es siempre el mismo: un `enum` con los estados posibles, una variable que guarda el estado actual, y un `switch` que en cada vuelta del bucle principal decide qué hacer y a qué estado pasar. Las ventajas de usar un `enum` (en vez de números mágicos `0`, `1`, `2`) son que el código se lee solo y que el compilador puede avisarte si te olvidás de manejar algún estado. Este patrón se profundiza en [Máquinas de estado](./18-maquinas-de-estado.md), donde vas a ver cómo estructurar firmware entero alrededor de esta idea.

> [!WARNING]
> **Ojo: `default` y `-Wswitch` se pelean, y nadie te lo dice.**
>
> `-Wswitch` (incluido en `-Wall`) avisa cuando a un `switch` sobre un `enum` le falta manejar algún
> valor. Es la red de seguridad que hace que agregar un estado nuevo al `enum` no pase inadvertido.
> El problema es que **agregar un `default` la desactiva por completo**: para GCC, un `default` ya
> cubre "todo lo demás", así que deja de contar los casos que faltan.
>
> ```c
> enum State { IDLE, RUNNING, PAUSED, ERR };
>
> void a(enum State s) { switch (s) { case IDLE: break; case RUNNING: break; } }
> void b(enum State s) { switch (s) { case IDLE: break; case RUNNING: break;
>                                     default: break; } }
> ```
> ```console
> $ arm-none-eabi-gcc -mcpu=cortex-m3 -Wall -c estados.c
> warning: enumeration value 'PAUSED' not handled in switch [-Wswitch]
> warning: enumeration value 'ERR' not handled in switch [-Wswitch]
> ```
>
> Los dos warnings son de `a()`. Sobre `b()`, que tiene el mismo agujero, **GCC no dice nada**.
>
> Y las dos cosas las querés: el `default` para sobrevivir a un valor corrupto en ejecución, y el
> aviso para no olvidarte de un estado al compilar. La forma de tener las dos es pedir
> **`-Wswitch-enum`**, que cuenta los valores faltantes *aunque haya `default`*. Con esa flag el
> ejemplo de arriba tira los cuatro warnings, dos por cada función.
>
> ```make
> CFLAGS += -Wall -Wextra -Wswitch-enum
> ```

---

## 3. Bucle `while`

Ejecuta un bloque mientras la condición sea verdadera.

### Sintaxis

```c
while (condición) {
    // código a repetir
}
```

La condición se evalúa **antes** de cada iteración. Si es falsa desde el principio, el bloque nunca se ejecuta.

---

### Ejemplo

```c
int i = 0;
while (i < 5) {
    printf("%d\n", i);
    i++;
}
```

Salida:
```
0
1
2
3
4
```

---

### Ejemplo embebido: esperar evento

```c
while (!UART_DataReady()) {
    // espera activa (polling) hasta que llegue un dato
}
char dato = UART_Read();
```

> [!CAUTION]
> **Si el bucle espera una *variable* en lugar de una función, hace falta `volatile`.** Este es el
> lugar donde más muerde:
>
> ```c
> volatile uint8_t dato_listo = 0;   // sin volatile, este bucle no termina nunca
>
> while (!dato_listo) { }            // la ISR pone dato_listo = 1
> ```
>
> Sin `volatile`, el compilador ve que nadie modifica `dato_listo` dentro del bucle, lee la variable
> una sola vez, la deja en un registro y arma un bucle infinito. Lo peor: con `-O0` funciona igual, y
> se rompe recién cuando compilás la versión final con `-O2`. El detalle completo está en
> [01 - Declaraciones y tipos](./01-declaraciones-y-tipos.md#3-calificador-de-tipo).

**Toda espera activa necesita un timeout.** Un `while` que espera a un periférico que no responde
cuelga el micro para siempre, y sin sistema operativo no hay nadie que lo rescate. En firmware de
verdad la espera se escribe así:

```c
uint32_t inicio = millis();

while (!UART_DataReady()) {
    if (millis() - inicio > TIMEOUT_MS) {
        return ERROR_TIMEOUT;      // el periférico no contestó: avisá, no te cuelgues
    }
}
```

> La resta `millis() - inicio` funciona incluso cuando el contador da la vuelta, porque el
> desbordamiento de un `unsigned` está definido. El porqué está en
> [02 - Overflow](./02-arreglos-conversiones-y-promociones.md#overflow-unsigned-da-la-vuelta-signed-es-comportamiento-indefinido).

> Mejor todavía que esperar con timeout es **no esperar**: dejar que una interrupción avise cuando el
> dato llegó y usar el tiempo de CPU en otra cosa. Eso se ve en
> [07 - Interrupciones](../07_interrupciones/) y en
> [Superloop no bloqueante](./17-superloop-y-codigo-no-bloqueante.md).

---

## 4. Bucle `do-while`

Similar a `while`, pero la condición se evalúa **después** de cada iteración. Esto garantiza que el bloque se ejecute **al menos una vez**.

### Sintaxis

```c
do {
    // código a repetir
} while (condición);
```

---

### Ejemplo

```c
int i = 0;
do {
    printf("%d\n", i);
    i++;
} while (i < 5);
```

Salida (igual que `while`):
```
0
1
2
3
4
```

---

### Diferencia clave con `while`

```c
int x = 10;

// Con while: NO se ejecuta
while (x < 5) {
    printf("while: %d\n", x);
}

// Con do-while: se ejecuta UNA VEZ
do {
    printf("do-while: %d\n", x);
} while (x < 5);
```

Salida:
```
do-while: 10
```

---

### Uso común: validación de entrada

```c
int entrada;
do {
    printf("Ingrese un número entre 1 y 10: ");
    scanf("%d", &entrada);
} while (entrada < 1 || entrada > 10);
```

Esto asegura pedir entrada al menos una vez.

---

### El idioma `do { ... } while(0)` (anticipo de macros)

Hay un uso muy particular de `do-while` que **no es un bucle**: corre exactamente una vez. Parece inútil, pero es la forma estándar de escribir **macros de varias sentencias** que se comporten como una sola instrucción. Lo vas a ver mucho en headers de firmware. Esto se desarrolla en [07 - El preprocesador](./07-preprocesador.md); acá te dejo la intuición.

Si definís una macro con varias líneas así:

```c
#define APAGAR_LED()   LED_OFF(); contador = 0   // ¡macro problemática!

if (error)
    APAGAR_LED();      // solo LED_OFF() queda dentro del if; contador=0 corre SIEMPRE
else
    seguir();          // error de compilación: 'else' sin 'if'
```

Envolviéndola en `do { ... } while(0)` resolvés ambos problemas de golpe: las sentencias quedan agrupadas en un solo bloque, y el `;` final del uso queda en su lugar natural:

```c
#define APAGAR_LED()   do { LED_OFF(); contador = 0; } while (0)

if (error)
    APAGAR_LED();      // ahora sí: las dos sentencias quedan dentro del if
else
    seguir();          // compila sin problemas
```

> No te preocupes si todavía no entendés del todo las macros: la idea es que reconozcas el patrón `do { ... } while(0)` cuando lo veas y sepas que es una macro, no un bucle de verdad.

---

## 5. Bucle `for`

Es la forma más compacta de iterar un número determinado de veces.

### Sintaxis

```c
for (inicialización; condición; actualización) {
    // código a repetir
}
```

---

### Componentes

1. **Inicialización**: se ejecuta una sola vez al inicio
2. **Condición**: se evalúa antes de cada iteración
3. **Actualización**: se ejecuta al final de cada iteración

---

### Ejemplo clásico

```c
for (int i = 0; i < 10; i++) {
    printf("%d\n", i);
}
```

Equivale a:

```c
int i = 0;
while (i < 10) {
    printf("%d\n", i);
    i++;
}
```

---

### Recorrer un arreglo

```c
int numeros[5] = {10, 20, 30, 40, 50};

for (int i = 0; i < 5; i++) {
    printf("numeros[%d] = %d\n", i, numeros[i]);
}
```

---

### Variantes del `for`

Podés omitir cualquier parte (pero los `;` deben estar):

```c
int i = 0;
for (; i < 10; ) {   // sin inicialización ni actualización
    printf("%d\n", i);
    i += 2;
}
```

Bucle infinito:

```c
for (;;) {
    // equivale a while(1)
}
```

---

### Ejemplo embebido: inicializar buffer

```c
uint8_t buffer[256];
for (size_t i = 0; i < sizeof buffer; i++) {
    buffer[i] = 0;
}
```

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

> Fijate el `sizeof buffer` en vez del `256` repetido: si mañana cambiás el tamaño del arreglo, el
> bucle se ajusta solo. Un `256` escrito a mano en dos lugares es un desbordamiento esperando a pasar.

---

## 6. Instrucciones de salto

### `break`

Sale **inmediatamente** del bucle o `switch` más interno.

```c
for (int i = 0; i < 10; i++) {
    if (i == 5) {
        break;  // sale del for cuando i == 5
    }
    printf("%d\n", i);
}
// imprime 0, 1, 2, 3, 4
```

Uso en embebidos:

```c
char buffer[64];
size_t index = 0;

while (1) {
    if (UART_DataReady()) {
        char c = UART_Read();
        if (c == '\n' || index == sizeof buffer - 1) {
            break;  // termina al recibir nueva línea, o si se llenó el buffer
        }
        buffer[index++] = c;
    }
}
buffer[index] = '\0';
```

> Fijate la segunda condición del `break`: sin ella, una línea más larga que el buffer lo desborda y
> te pisa memoria vecina. En firmware, **todo bucle que escribe en un arreglo tiene que tener un
> límite además de su condición "natural"**.

---

### `continue`

Salta el resto de la iteración actual y pasa a la siguiente.

```c
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // salta los pares
    }
    printf("%d\n", i);
}
// imprime 1, 3, 5, 7, 9
```

---

### Diferencia entre `break` y `continue`

```c
for (int i = 0; i < 5; i++) {
    if (i == 2) break;
    printf("%d ", i);
}
// Salida: 0 1

for (int i = 0; i < 5; i++) {
    if (i == 2) continue;
    printf("%d ", i);
}
// Salida: 0 1 3 4
```

---

### Dos sutilezas de `break` y `continue`

#### `break` dentro de un `switch` que está dentro de un bucle

`break` sale de **la estructura más interna que lo contenga**, y un `switch` cuenta como tal. Así que
este `break` corta el `switch`, **no** el `while`:

```c
while (1) {
    switch (comando) {
        case CMD_SALIR:
            break;      // corta el switch y sigue el while: ¡el bucle NO termina!
    }
    // la ejecución llega acá y vuelve a dar la vuelta
}
```

Es un bug silencioso porque *parece* que corta el bucle. Las salidas son usar una bandera, un `goto`,
o directamente un `return` si el bucle está solo en su función:

```c
int terminar = 0;

while (1) {
    switch (comando) {
        case CMD_SALIR:
            terminar = 1;
            break;              // corta el switch, como corresponde
    }
    if (terminar) {
        break;                  // este sí corta el while
    }
}
```

#### `continue` dentro de un `do-while` no saltea la condición

En un `for` o un `while`, `continue` va a evaluar la condición de nuevo. En un `do-while` pasa lo
mismo, y por eso sorprende: **no** sale del bucle, salta a la comprobación del final.

```c
int i = 0;
do {
    i++;
    if (i == 2) continue;   // salta el printf, pero NO sale
    printf("i=%d\n", i);
} while (i < 4);
// imprime: i=1, i=3, i=4
```

Si la variable de control se actualiza *después* del `continue`, el bucle no avanza nunca y te queda
colgado. En un `for` no pasa, porque la actualización va en el encabezado y `continue` la ejecuta igual.

---

### `return`

Sale de la función actual y opcionalmente devuelve un valor.

```c
#include <stddef.h>

// devuelve el índice, o -1 si no está
int encontrar(const int arr[], size_t cantidad, int valor) {
    for (size_t i = 0; i < cantidad; i++) {
        if (arr[i] == valor) {
            return (int) i;  // devuelve el índice y sale
        }
    }
    return -1;  // no encontrado
}
```

> El arreglo va `const` porque la función no lo modifica, y la cantidad va en `size_t`, que es el
> tipo para tamaños: las dos convenciones vienen de
> [02 - Arreglos](./02-arreglos-conversiones-y-promociones.md#pasar-arreglos-a-funciones). El cast en
> el `return` es porque la función devuelve `int` para poder usar el -1 como "no encontrado".

---

### `goto`: usar con extrema precaución

Salta a una etiqueta en el código.

```c
int main(void) {
    int i = 0;
inicio:
    printf("%d\n", i);
    i++;
    if (i < 5) {
        goto inicio;
    }
    return 0;
}
```

> Este ejemplo es solo para mostrar la mecánica: un `while` hace lo mismo y se lee mejor. El `goto`
> se justifica en los dos casos de abajo, no acá.

---

### ¿Cuándo usar `goto`?

**Casi nunca**. Hace el código difícil de seguir ("código espagueti").

**Casos legítimos**:

1. **Limpieza de recursos en funciones con múltiples puntos de salida**

La idea es tener **un solo camino de salida** que deshaga lo que se hizo, en orden inverso, sin repetir
la limpieza en cada `return`. Fijate que cada etiqueta libera solo lo que ya se había conseguido:

```c
int procesar_archivo(const char *path) {
    int resultado = -1;                 // pesimista: solo pasa a 0 si todo salió bien

    FILE *f = fopen(path, "r");
    if (!f) {
        return resultado;               // nada que limpiar todavía
    }

    char *buffer = malloc(1024);
    if (!buffer) {
        goto cerrar_archivo;
    }

    int *data = malloc(sizeof(int) * 100);
    if (!data) {
        goto liberar_buffer;
    }

    // procesar...
    resultado = 0;                      // éxito

    free(data);
liberar_buffer:
    free(buffer);
cerrar_archivo:
    fclose(f);
    return resultado;
}
```

> **La clave está en la variable `resultado`.** Arranca en error y solo se pone en 0 cuando todo
> funcionó. Si en cambio hacés que la función termine con `return 0;` fijo, los caminos de error
> devuelven "éxito" y el llamador nunca se entera del problema: es el bug más común al copiar este
> patrón.
>
> En firmware sin sistema operativo casi no vas a usar `malloc` (ver
> [11 - Asignación dinámica](./11-asignacion-dinamica.md)), pero el patrón es idéntico cuando lo que
> hay que deshacer es apagar un periférico, liberar un pin o volver a habilitar interrupciones.

2. **Salir de bucles anidados**

`break` sale de **un solo** nivel, así que para cortar dos `for` de una vez hace falta una bandera y
dos `break`, o un `goto`:

```c
int encontrado = 0;

for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
        if (matriz[i][j] == buscado) {
            encontrado = 1;
            goto salir;      // sale de los DOS bucles de una
        }
    }
}
salir:
    if (!encontrado) {
        manejar_no_encontrado();
    }
```

> [!WARNING]
> **Una etiqueta no detiene la ejecución.** Si los bucles terminan sin saltar, el programa **sigue
> igual** hacia la etiqueta y ejecuta lo que haya debajo. Una etiqueta llamada `error:` con el manejo
> de error abajo y nada que la saltee corre también en el camino feliz, que es exactamente el bug que
> se quería evitar. Por eso el ejemplo de arriba usa una bandera que distingue los dos casos; la
> alternativa es poner un `return` o un `goto fin` justo antes de la etiqueta.

> En la mayoría de casos, `break`, `continue` o reestructurar el código (por ejemplo, sacando los
> bucles anidados a una función aparte y usando `return`) es mejor que `goto`.

---

## 7. Bucles infinitos en sistemas embebidos

En firmware embebido, el bucle principal suele ser infinito:

```c
int main(void) {
    // inicialización
    System_Init();
    Peripherals_Init();

    // bucle principal
    while (1) {
        // tarea 1
        Leer_Sensores();

        // tarea 2
        Actualizar_Display();

        // tarea 3
        Procesar_Comandos();
    }

    return 0;  // nunca se alcanza
}
```

También se puede escribir como:

```c
for (;;) {
    // bucle infinito
}
```

---

## 8. Consejos y buenas prácticas

### Usar llaves `{}` siempre

Aunque no sean necesarias para una sola instrucción:

```c
// Evitar (propenso a errores)
if (x > 0)
    y = 1;

// Mejor
if (x > 0) {
    y = 1;
}
```

Esta es **la** regla de estilo que más bugs evita, y no es cuestión de gusto: sin llaves aparecen el
`else` colgante y el clásico de agregar una segunda línea al `if` que queda afuera sin que nada falle.
Las dos trampas están explicadas en
[Las dos trampas clásicas del `if`](#las-dos-trampas-clásicas-del-if).

```c
if (error)
    apagar_motor();
    encender_alarma();   // ¡NO está dentro del if! Corre siempre.
```

### Evitar condiciones complejas

```c
#include <stdbool.h>

// Difícil de leer
if ((x > 0 && y < 10) || (z == 5 && w != 3)) {
    // ...
}

// Mejor: cada condición con un nombre que diga qué significa
bool en_rango       = (x > 0 && y < 10);
bool modo_valido    = (z == 5 && w != 3);
if (en_rango || modo_valido) {
    // ...
}
```

> `bool` necesita `#include <stdbool.h>` (desde C99); sin ese header el tipo se llama `_Bool`. En el
> M3 ocupa 1 byte. Y nombrá las variables por lo que **significan**, no `condicion1` y `condicion2`:
> si el nombre no te dice nada, no ganaste legibilidad, solo moviste el problema de lugar.

### Inicializar variables de bucle

```c
// El índice sobrevive al bucle: sigue visible (y con el valor final) después del for
int i;
for (i = 0; i < 10; i++) { ... }

// Mejor (C99+): i solo existe dentro del bucle, y no podés reusarla por accidente
for (int i = 0; i < 10; i++) { ... }
```

La excepción es cuando *necesitás* el valor del índice después del bucle (por ejemplo, para saber
dónde cortó un `break`): ahí la variable tiene que declararse afuera.

### Evitar comparaciones con punto flotante

Un flotante casi nunca vale exactamente lo que parece, y el error se **acumula** al sumar. En un
capítulo de control de flujo esto importa por una razón concreta: un bucle que termina comparando
flotantes con `==` o `!=` **puede no terminar nunca**.

```c
// ¡BUG! Este for no termina: x nunca vale exactamente 1.0f
for (float x = 0.0f; x != 1.0f; x += 0.1f) {
    ...
}
```

El motivo se ve sumando a mano:

```c
float suma = 0.0f;
for (int i = 0; i < 10; i++) {
    suma += 0.1f;       // diez veces 0.1f
}
// suma vale 1.000000119, NO 1.0f  →  (suma == 1.0f) es falso
```

Las dos salidas: **contá con un entero**, o compará con una tolerancia.

```c
#include <math.h>

// Opción A (preferida): el contador es entero y el flotante se deriva de él
for (int i = 0; i < 10; i++) {
    float x = i * 0.1f;
    ...
}

// Opción B: comparar con tolerancia en lugar de con ==
#define EPSILON 0.0001f
if (fabsf(suma - 1.0f) < EPSILON) { ... }
```

> [!NOTE]
> **Cuidado con el ejemplo famoso.** Seguro viste que `0.1 + 0.2 != 0.3`. Eso es cierto en `double`,
> pero **en `float` esa suma puntual redondea justo y sí da igual**: `0.1f + 0.2f == 0.3f` es
> verdadero. No te confíes de la intuición en un caso suelto; el problema real aparece al **acumular**,
> que es lo que hacen los bucles. La regla práctica no cambia: no uses `==` ni `!=` con flotantes.

> Fijate el sufijo `f` en todas las constantes y `fabsf()` en lugar de `fabs()`. Sin eso, cada
> constante es un `double` y arrastra la cuenta entera a 64 bits: en el Cortex-M3, que **no tiene
> FPU**, eso significa llamar a rutinas de punto flotante por software y pagar una barbaridad de
> ciclos. Es el tema de [15 - Punto fijo vs punto flotante](./15-punto-fijo-vs-flotante.md), y el
> motivo por el que en firmware conviene directamente **evitar los flotantes**: si podés, trabajá con
> enteros o punto fijo y el problema no existe.

### Usar `switch` para múltiples comparaciones con el mismo valor

```c
// Repetitivo
if (estado == 1) { ... }
else if (estado == 2) { ... }
else if (estado == 3) { ... }

// Más claro
switch (estado) {
    case 1: ... break;
    case 2: ... break;
    case 3: ... break;
    default: ... break;   // no te olvides del default (ver sección 2)
}
```

---

## 9. Resumen

| Estructura | Uso | Ejemplo |
|------------|-----|---------|
| `if-else` | Decisión binaria | `if (x > 0) { ... } else { ... }` |
| `switch` | Múltiples opciones | `switch (opcion) { case 1: ... }` |
| `while` | Repetir mientras condición | `while (i < 10) { ... }` |
| `do-while` | Ejecutar al menos una vez | `do { ... } while (i < 10);` |
| `for` | Repetir N veces | `for (int i = 0; i < 10; i++) { ... }` |
| `break` | Salir de bucle/switch | `if (done) break;` |
| `continue` | Saltar a siguiente iteración | `if (skip) continue;` |
| `return` | Salir de función | `return resultado;` |
| `goto` | Salto incondicional (evitar) | `goto error;` |

### Reglas para no equivocarse

| Regla | Por qué |
|-------|---------|
| Llaves `{}` siempre, aunque sea una sola línea | Evita el `else` colgante y la segunda línea que queda afuera del `if` |
| Nunca `=` donde va `==` | `if (x = 5)` asigna y entra siempre; `-Wall` lo avisa con `-Wparentheses` |
| `default` en todo `switch` sobre un estado | Un valor corrupto en ejecución no puede quedar sin manejar |
| Compilá con `-Wswitch-enum`, no solo `-Wall` | Poner `default` **desactiva** el aviso de `-Wswitch` por casos faltantes |
| Marcá el *fall-through* a propósito con `/* fall-through */` | `-Wextra` te avisa de cada `break` olvidado, y no del intencional |
| `break` dentro de un `switch` corta el `switch`, no el bucle | Para salir del bucle hace falta una bandera, un `goto` o un `return` |
| Toda espera activa lleva timeout | Un periférico que no contesta cuelga el micro para siempre |
| `volatile` en la variable que espera un bucle de *polling* | Sin él, el compilador la lee una vez y el bucle no termina nunca |
| `sizeof buffer` en vez del tamaño escrito a mano | Si cambia el arreglo, el bucle se ajusta solo |
| No compares flotantes con `==` | Y en el M3, sin FPU, mejor ni uses flotantes |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.8.4 (sentencias de selección, `if` y `switch`), 6.8.5 (sentencias de iteración), 6.8.6 (sentencias de salto: `goto`, `continue`, `break`, `return`).
- [cppreference: C language statements](https://en.cppreference.com/w/c/language/statements). Referencia práctica de cada estructura, con las reglas exactas de `switch` y etiquetas.

**GCC y el toolchain**

- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). Documenta `-Wparentheses` y `-Wdangling-else` (en `-Wall`), `-Wimplicit-fallthrough` (en `-Wextra`) y la diferencia entre `-Wswitch`, `-Wswitch-enum` y `-Wswitch-default`.

**Sobre los temas puntuales**

- [Máquinas de estado](./18-maquinas-de-estado.md). El patrón `enum` + `switch` de este capítulo, llevado a la arquitectura de un firmware completo.
- [Superloop no bloqueante](./17-superloop-y-codigo-no-bloqueante.md). Cómo reemplazar las esperas activas de este capítulo por un bucle principal que no bloquea.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [03 - Operadores](./03-operadores.md) ·
**Siguiente:** [05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md)
