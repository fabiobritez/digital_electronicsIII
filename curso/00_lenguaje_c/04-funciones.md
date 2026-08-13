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
> [C7 - El preprocesador](./07-preprocesador.md#por-qué-existen-los-h-cada-c-se-compila-solo).

### Por qué los prototipos importan de verdad (no es solo orden)

Un prototipo le dice al compilador **cuántos parámetros** lleva la función y **de qué tipo** son, además del tipo de retorno. Con esa información, el compilador puede:

- **Verificar que la llamés bien.** Si pasás un `float` donde se esperaba un `int`, te lo marca antes de generar código.
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

void saludar(void) {
    printf("Hola desde una función!\n");
}

int main(void) {
    imprimir_mensaje();
    saludar();
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

La función recibe una **copia** de `a`, por lo que modificar `x` no afecta a `a`. C siempre pasa los
argumentos por valor. Más adelante veremos que también se puede pasar la dirección de una variable;
en ese caso, lo que se copia es el puntero.

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
> está en [C14 - Punto fijo vs punto flotante](./14-punto-fijo-vs-flotante.md).

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

## `return` como salida anticipada

`return` también permite terminar apenas el contrato ya está resuelto, sin agregar niveles de
`else`. En este punto usamos solo parámetros por valor:

```c
int dividir_si_es_posible(int dividendo, int divisor) {
    if (divisor == 0) {
        return 0;               // centinela: operación inválida
    }
    return dividendo / divisor;
}
```

Cuando C9 introduzca arreglos como parámetros, esta misma salida anticipada se aplicará a búsquedas
y validación de buffers sin ocultar el tamaño ni la mutabilidad.

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

Una función puede devolver un `int` que indique éxito o el tipo de error. C6 presenta `enum` como
forma más expresiva de nombrar esos códigos sin números mágicos:

```c
int Sensor_Verificar(void) {
    if (!dato_listo()) return -1;  // timeout
    if (!crc_ok())     return -2;  // error de CRC
    return 0;                      // éxito
}

if (Sensor_Verificar() != 0) {
    // manejar el error
}
```

Usar cero para éxito es la convención más extendida. Cuando la función también debe entregar un
dato, C9 muestra el patrón de **código de estado + parámetro de salida**.

### Convención 2: un valor centinela

Si existe un valor que nunca puede ser un resultado legítimo, se lo puede reservar para error:

```c
int leer_temperatura(void) {
    if (!sensor_responde()) return -1000;
    return convertir_miligrados();
}
```

El centinela simplifica la interfaz, pero obliga a documentar su rango. Si todos los valores posibles
son válidos, usá la convención anterior y, después de C8, un parámetro de salida.

> **Regla:** elegí una convención por módulo, documentala y comprobá siempre el resultado.
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

### Documentar contratos

La documentación de una función pública debe indicar qué recibe, qué devuelve, qué errores puede
producir y qué efectos secundarios tiene. En C9 se amplía este contrato para buffers y punteros,
incluidas sus capacidades, nulabilidad y mutabilidad.

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

## Resumen de C4

| Concepto | Descripción |
|---|---|
| Declaración o prototipo | Contrato de tipos conocido antes de llamar |
| Definición | Cuerpo que implementa ese contrato |
| Paso por valor | Cada parámetro recibe una copia |
| `return` | Entrega un valor compatible con el tipo declarado |
| Código de estado | Hace explícito el éxito o el motivo del error |
| Variable local `static` | Estado que conserva su valor entre llamadas |
| Recursión | Consume un nuevo marco de stack por llamada |
| Función variádica | Cantidad variable de argumentos, reservada para casos justificados |

---

**Reglas de oro:**

1. Una función, una responsabilidad y un contrato verificable.
2. Declarar el prototipo antes de la primera llamada.
3. Todos los caminos de una función no `void` deben retornar.
4. Validar parámetros y comprobar los códigos de error.
5. Minimizar estado global y efectos secundarios.
6. Evitar recursión no acotada en firmware.
7. Después de C8, completar el contrato con nulabilidad, mutabilidad y tamaño de los objetos apuntados.

**Los flags que atrapan los errores de este capítulo:**

```make
CFLAGS += -Wall -Werror=implicit-function-declaration -Werror=return-type
```

El primero convierte en error la llamada sin prototipo; el segundo, la función con retorno que se
olvida de retornar. Al llegar a C9 se agrega `-Wsizeof-array-argument`, que detecta el `sizeof` aplicado por error a un parámetro arreglo.

---

## Fuentes y para seguir leyendo

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf).
  Cláusulas 6.9.1 (definiciones), 6.5.2.2 (llamadas y argumentos) y 7.16 (`<stdarg.h>`).
- [cppreference: Functions](https://en.cppreference.com/w/c/language/functions).
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., capítulo 4 (*Functions and Program
  Structure*).
- [GCC: Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html). `-Wreturn-type`,
  `-Wimplicit-function-declaration` y `-Werror=return-type`.
- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst).
  Define el paso de argumentos y valores de retorno del Cortex-M3.
- [C10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md). El marco de pila de cada
  llamada, el contrato AAPCS y cómo medir cuánto stack usa cada función.
- [C13 - `static`, `const`, `inline` e interfaces](./13-static-const-inline-e-interfaces.md).
  Privacidad de símbolos, helpers en headers y atributos de función.
- [C9 - Arreglos, punteros y callbacks](./09-punteros-avanzado.md). Parámetros y retornos puntero,
  arreglos como parámetros, callbacks y tablas de dispatch.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [C3 - Control de flujo](./03-control-de-flujo.md) ·
**Siguiente:** [C5 - Arreglos y strings](./05-arreglos-y-strings.md)
