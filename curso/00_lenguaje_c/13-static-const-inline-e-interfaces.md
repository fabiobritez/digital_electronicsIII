# `static`, `const`, `inline` y diseño de interfaces

`static`, `const` e `inline` aparecen todo el tiempo en los drivers y en CMSIS. No son adornos:
determinan quién puede modificar un dato, qué nombres quedan visibles fuera de un archivo y cómo se
publica una función pequeña sin llenar la interfaz de detalles internos.

Vamos a usarlos juntos para separar con claridad el `.h` público de la implementación privada en
el `.c`.

---

## `const` como parte del contrato y del almacenamiento

En una interfaz, `const` comunica qué puede modificar una función y permite aceptar tanto datos
mutables como datos de solo lectura. En una definición a nivel de archivo, además ayuda a que tablas
y configuración terminen en `.rodata` (Flash) en vez de consumir RAM.

```c
// sensor.h: el llamador conserva la propiedad; la función solo lee
sensor_status_t sensor_configure(const sensor_config_t *cfg);

// sensor.c: detalle privado y de solo lectura
static const uint16_t linearizacion[256] = { /* ... */ };
```

No pongas la **definición** de una tabla global común en un header: cada unidad de traducción puede
obtener su propia copia o producir símbolos duplicados, según cómo la declares. Si el dato debe ser
único, el header publica `extern const T nombre[];` y un solo `.c` aporta la definición. Si cada
unidad necesita un helper pequeño, ahí sí suele corresponder `static inline`.

### `restrict`: una promesa de no aliasing

`restrict` no significa “la única forma de acceder es un puntero”. Es una promesa hecha al
compilador: durante la vida de ese puntero, el objeto se accede únicamente a través de él o de
punteros derivados. Eso permite mantener valores en registros y vectorizar o reordenar operaciones.

```c
void copiar(size_t n,
            uint8_t * restrict destino,
            const uint8_t * restrict origen);
```

Si `destino` y `origen` se solapan y el código escribe y lee por ambos, se viola el contrato y el
comportamiento es indefinido. Por eso `memcpy` declara punteros `restrict`, mientras que `memmove`
acepta solapamiento y no puede hacer esa promesa. Es una herramienta de optimización y diseño de
API; no una forma de corregir aliasing a fuerza de agregar una palabra.

---

## `static`: los dos patrones que vas a escribir

`static` ya lo conocés del capítulo 01. Lo que importa acá es que en firmware se usa para dos cosas
muy concretas, y conviene reconocerlas de una mirada.

**1. Estado que sobrevive entre llamadas.** Es la base de las tareas no bloqueantes: cada tarea
necesita recordar cuándo actuó por última vez, y sin `static` ese dato se perdería en cada vuelta del
superloop.

```c
void tarea_led(void) {
    static uint32_t t_prev = 0;      // se inicializa UNA sola vez, en el arranque
    if (millis() - t_prev >= 500) {
        t_prev = millis();
        led_toggle();
    }
}
```

Es la alternativa a una variable global: el dato vive todo el programa, pero **solo esta función puede
tocarlo**. Se ve en detalle en
[Superloop no bloqueante](./arquitectura/17-superloop-y-codigo-no-bloqueante.md).

**2. Todo lo que no es API pública.** Sobre una global o una función a nivel de archivo, `static` la
hace invisible para el resto del programa:

```c
static uint8_t buffer[256];                 // solo este .c puede usarlo
static void rutina_interna(void) { ... }    // auxiliar, no parte de la API
```

La regla práctica: **en un `.c`, todo lo que no está declarado en su `.h` debería ser `static`.** Evita
choques de nombres, le dice al lector qué es interno, y encima le da al compilador libertad para
optimizar más agresivamente, porque sabe que nadie de afuera lo usa. Es lo que hace `mygpio.c` en el
módulo 2 con su tabla `static MYGPIO_Port * const puertos[5]`.

## `inline` y `static inline`: velocidad sin perder claridad

Una función chiquita (ej. "poné el pin alto") tiene un costo: llamarla (guardar registros, saltar,
volver). Para funciones muy usadas en código crítico, ese costo importa. `inline` le sugiere al
compilador que **pegue el cuerpo de la función en el lugar de la llamada**, evitando el salto:

```c
static inline void led_on(void)  { LPC_GPIO0->FIOSET = (1u << 22); }
static inline void led_off(void) { LPC_GPIO0->FIOCLR = (1u << 22); }
```

Con `static inline` en un **header**, tenés lo mejor de dos mundos: código **legible** (llamás
`led_on()`) que el compilador convierte en **una sola instrucción**, sin el costo de una llamada.
CMSIS hace esto muchísimo: `NVIC_EnableIRQ`, `__WFI`, los accesos del núcleo, son `static inline`.

> Cuándo usarlo: funciones **muy chicas** y **muy llamadas** (accesos a registros, helpers). Para
> funciones grandes no tiene sentido (inflaría el código). El compilador igual puede ignorar la
> sugerencia si no le conviene.

### ¿Por qué `static inline` y no solo `inline` en un header?

Esto confunde a casi todos. La clave: si ponés una función `inline` "pelada" en un header y el
compilador **decide no expandirla** (por ejemplo en `-O0`, o si la función crece), necesita emitir
una copia "real" de la función. Pero como el header se incluye en varios `.c`, terminarías con
**varias definiciones** del mismo símbolo de enlace externo → el linker se queja de "definición
múltiple". (Las reglas de `inline` solo en C son sutiles y poco intuitivas justo por esto.)

`static inline` lo resuelve de raíz: el `static` le da **enlace interno**, así que cada `.c` que
incluye el header tiene su propia copia privada y no hay choque de símbolos. Si el compilador la
expande, no queda ninguna copia; si no, queda una copia local e inofensiva por archivo. **Por eso
los helpers de un header siempre se escriben `static inline`**, y por eso CMSIS lo hace en todo
(`NVIC_EnableIRQ`, `__WFI`, etc.).

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
> `inline`, `static inline` y `extern inline` explican por qué en los headers de firmware casi
> siempre vas a ver la segunda.

### Efecto en tamaño vs velocidad, y LTO

Inline es un **canje**: ganás velocidad (sacás el costo de la llamada) a cambio de **tamaño** (el
cuerpo se duplica en cada lugar donde la llamás). Para una función de 1-2 instrucciones, el cuerpo
es más chico que la propia secuencia de llamada/retorno, así que ganás en las dos. Para funciones
grandes llamadas en muchos lugares, inflás la Flash sin beneficio: ahí conviene **no** inline.

Una herramienta relacionada es **LTO** (*Link-Time Optimization*, flag `-flto`): permite al compilador
optimizar **a través de archivos** en el momento del enlace. Con LTO, el compilador puede hacer inline
de funciones que están en **otro** `.c` (algo imposible en la compilación normal, archivo por archivo)
y eliminar código muerto entre módulos. En embebidos suele reducir el binario de forma notable; lo ves
con el toolchain en la [unidad de herramientas](../../herramientas/04_toolchains/).

## Ejemplo completo: interfaz de un módulo de sensor



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

## Atributos de función y extensiones del compilador

Además de las palabras definidas por C, GCC permite agregar información sobre una función o un
símbolo. Son extensiones del compilador: resultan útiles en firmware, pero hacen que el código dependa
de esa familia de herramientas.

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

## Para los curiosos (avanzado)

Nada de esto es necesario para aprobar la materia, pero aparece en código embebido profesional y en
las entrañas de CMSIS/startup. Te lo dejamos para cuando tengas curiosidad.

### Atributos de GCC: `__attribute__((...))`

GCC (y el `arm-none-eabi-gcc` del curso) permite anotar variables y funciones con *atributos* que
controlan cómo se generan. Los que más vas a ver en embebidos:

```c
// aligned: forzar alineación (ej. un buffer de DMA que debe arrancar en múltiplo de 4)
static uint8_t buffer_dma[64] __attribute__((aligned(4)));

// packed: sin padding (struct que mapea un protocolo o registro exacto; ver C12)
typedef struct __attribute__((packed)) { uint8_t cmd; uint32_t val; } trama_t;

// section: ubicar el símbolo en una sección concreta del linker (ej. la tabla de vectores)
__attribute__((section(".isr_vector"), used))
const void *vectores[] = { /* ... */ };

// weak: símbolo "débil", que otro .c puede sobrescribir sin error de doble definición
__attribute__((weak)) void SysTick_Handler(void) { /* handler por defecto, vacío */ }

// used: "no lo borres aunque parezca que nadie lo usa" (típico junto a section)
```

`section`, `used` y `weak` son la base de la **tabla de vectores y el startup** ([herramientas 05](../../herramientas/05_del_codigo_al_binario/)): los
handlers se declaran `weak` con un cuerpo por defecto, y cuando vos escribís tu `SysTick_Handler`
real, el tuyo "gana". `aligned` y `packed` son los que más vas a tocar en datos.

### Barreras de memoria: `__DMB()` / `__DSB()` y la barrera del compilador

Como vimos en C11, `volatile` **no** es una barrera de memoria. Hay dos tipos de barrera:

- **Barrera del compilador**: impide que el compilador *reordene* instrucciones a través de ella.
  En GCC: `__asm volatile ("" ::: "memory");`. No genera ninguna instrucción.
- **Barrera de hardware**: instrucciones reales del Cortex-M que ordenan los accesos a memoria a
  nivel del bus. CMSIS las expone como `__DMB()` (Data Memory Barrier), `__DSB()` (Data
  Synchronization Barrier) e `__ISB()` (Instruction Sync Barrier).

```c
LPC_SC->PCONP |= (1u << 15);   // habilito el reloj de un periférico
__DSB();                        // me aseguro de que la escritura "se asentó" antes de seguir
// ...ahora sí configuro el periférico...
```

En la práctica del LPC1769 las necesitás poco (sobre todo después de habilitar relojes o tras escribir
registros de control de sistema). Tenelas en el radar para cuando un periférico "no arranca" pese a
que la configuración parece correcta.

### Una pizca de *inline assembly*

A veces necesitás una instrucción que C no expresa. GCC permite incrustar ensamblador:

```c
static inline void no_op(void) {
    __asm volatile ("nop");          // una instrucción NOP exacta
}
static inline void esperar_irq(void) {
    __asm volatile ("wfi");          // Wait For Interrupt: el CPU duerme hasta la próxima IRQ
}
```

CMSIS ya te envuelve las más útiles (`__NOP()`, `__WFI()`, `__disable_irq()`), así que rara vez
escribís asm a mano. Pero saber que se puede ayuda a leer los intrínsecos del núcleo.

### `volatile` no alcanza para multinúcleo (pero el M3 es single-core)

Un punto fino: `volatile` ordena los accesos *de un mismo hilo de ejecución* respecto a la memoria,
pero **no** establece un orden visible entre varios núcleos. En un multinúcleo (o con caches y buffers
de escritura entre CPUs) hacen falta barreras (`DMB`) y operaciones atómicas reales para que un núcleo
vea los datos de otro de forma coherente: `volatile` solo no garantiza nada de eso. Por suerte para
nosotros, **el Cortex-M3 del LPC1769 es single-core y sin cache de datos**, así que esta clase de
problemas no aparece: nuestra única fuente de concurrencia son las **interrupciones**, y para eso
alcanza con `volatile` + secciones críticas (Módulo 07). Pero si algún día saltás a un Cortex-A o a
un sistema con SMP, acordate de esto.

## Resumen

| Herramienta | Para qué |
|-------------|----------|
| `static` (local) | estado que sobrevive entre llamadas: la base de las tareas no bloqueantes |
| `static` (global/función) | todo lo que no está en el `.h` debería serlo: privado y más optimizable |
| `static inline` | helpers chicos rápidos y legibles en headers (como CMSIS), sin choque de símbolos |
| `inline` a secas en un header | **evitalo**: si el compilador no lo expande, el linker se queja de doble definición |
| `-flto` | dejar que el compilador haga inline y borre código muerto **entre** archivos |
| `__attribute__`, `__DMB`/`__DSB`, asm (curiosos) | control fino de memoria y código; startup, DMA, lazos calientes |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.7.4 (especificadores de función, o sea las reglas de `inline` y por qué en un header hace falta `static`), 6.2.2 (enlace interno y externo) y 6.7.3 (`const`).
- [cppreference: inline](https://en.cppreference.com/w/c/language/inline). La explicación más clara del modelo de `inline` en C, que no es el de C++.

**GCC y el toolchain**

- [GCC: An Inline Function is As Fast As a Macro](https://gcc.gnu.org/onlinedocs/gcc/Inline.html). Qué hace GCC con `inline`, `static inline` y `extern inline` en cada nivel de optimización.
- [GCC: Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html). `-flto` y qué habilita a través de archivos.
- [GCC: Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html) y [Variable Attributes](https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html). Los `__attribute__` del bloque avanzado.

**ARM y el LPC1769**

- [CMSIS-Core (Cortex-M)](https://arm-software.github.io/CMSIS_5/Core/html/index.html). `NVIC_EnableIRQ`, `__WFI`, `__DMB`/`__DSB`: todos son `static inline` en `core_cm3.h`, que está en el repo (`library/CMSISv2p00_LPC17xx/inc/`) y podés abrir para verlo.
- [ARMv7-M Architecture Reference Manual](https://developer.arm.com/documentation/ddi0403/latest/). Qué garantizan de verdad `DMB`, `DSB` e `ISB`.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [C12 - Layout, alineación, uniones y bitfields](./12-layout-alineacion-unions-y-bitfields.md) ·
**Siguiente:** [C14 - Punto fijo y punto flotante](./14-punto-fijo-vs-flotante.md)
