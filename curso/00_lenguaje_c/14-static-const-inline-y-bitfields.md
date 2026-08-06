# `static`, `inline` y campos de bits (C embebido fino)

Estas herramientas aparecen todo el tiempo en código embebido bien escrito (y en CMSIS). No son "C
avanzado": son lo que distingue un código prolijo y eficiente de uno que "anda pero es un desastre".
Vale la pena dominarlas.

> [!NOTE]
> **Lo que este capítulo da por sabido.** Qué hace `static` (duración contra enlace), qué es `extern`
> y por qué una tabla `const` termina en Flash está en
> [01 - Declaraciones y tipos](./01-declaraciones-y-tipos.md#2-especificador-de-almacenamiento).
> Cómo se lee la posición del `const` en un puntero está en
> [08 - Punteros](./08-punteros.md#const-y-punteros-const-correctness), y `restrict` en
> [01](./01-declaraciones-y-tipos.md#3-calificador-de-tipo). Acá no repetimos nada de eso: vamos
> directo a lo que todavía no viste.

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
[Superloop no bloqueante](./17-superloop-y-codigo-no-bloqueante.md).

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

### Efecto en tamaño vs velocidad, y LTO

Inline es un **canje**: ganás velocidad (sacás el costo de la llamada) a cambio de **tamaño** (el
cuerpo se duplica en cada lugar donde la llamás). Para una función de 1-2 instrucciones, el cuerpo
es más chico que la propia secuencia de llamada/retorno, así que ganás en las dos. Para funciones
grandes llamadas en muchos lugares, inflás la Flash sin beneficio: ahí conviene **no** inline.

Una herramienta relacionada es **LTO** (*Link-Time Optimization*, flag `-flto`): permite al compilador
optimizar **a través de archivos** en el momento del enlace. Con LTO, el compilador puede hacer inline
de funciones que están en **otro** `.c` (algo imposible en la compilación normal, archivo por archivo)
y eliminar código muerto entre módulos. En embebidos suele reducir el binario de forma notable; lo ves
con el toolchain en el [Anexo B](../anexos/B_toolchain_y_entorno/).

## Bitfields contra máscaras: por qué CMSIS eligió máscaras

Los campos de bits ya los viste en
[13 - Structs para hardware](./13-structs-para-hardware.md#estructuras-bit-field), con la advertencia
de que **el orden de los bits no está garantizado por el estándar**. Esa sola razón ya alcanzaría para
descartarlos en registros de hardware, pero hay una segunda que no se suele mencionar y que se ve
mejor con el desensamblado al lado.

Tomemos el registro de control del SysTick, y prendamos dos bits de las dos maneras:

```c
void con_bitfield(void) { R->ENABLE = 1; R->CLKSOURCE = 1; }
void con_mascara(void)  { *M |= (1u << 0) | (1u << 2); }
```

Compilá con `-O2` y mirá lo que sale:

```asm
con_bitfield:              con_mascara:
    ldr  r3, [r3]              ldr  r3, [r2]
    ldr  r2, [r3]      <-      orr  r3, r3, #5
    orr  r2, r2, #1            str  r3, [r2]
    str  r2, [r3]      <-
    ldr  r2, [r3]      <-
    orr  r2, r2, #4
    str  r2, [r3]      <-
```

La versión con máscara hace **un** ciclo leer-modificar-escribir. La de bitfields hace **dos**, uno
por campo, porque cada asignación es una sentencia independiente y el compilador no las puede fusionar.

En una `struct` tuya en RAM eso sería solo un poco más lento. En un **registro de hardware** puede
romperte el programa, y por dos motivos:

- Si el registro tiene bits que se limpian al leerlos (como `U0LSR`, que viste en el
  [capítulo 12](./12-volatile-y-tipos-para-hardware.md#const-volatile-registros-de-solo-lectura)),
  leerlo dos veces en lugar de una **pierde información**.
- Entre la primera escritura y la segunda hay una ventana en la que el registro quedó con un valor
  intermedio que vos nunca quisiste. Si en el medio entra una interrupción, o si el periférico
  reacciona al primer cambio, el resultado no es el que pediste.

> **Conclusión práctica:** los *bitfields* son cómodos y legibles para **tus propias** estructuras de
> datos, donde el orden de los bits no le importa a nadie más que a vos. Para **registros de
> hardware**, usá **máscaras** (`|=`, `&= ~`, `<<`): son portables, generan el acceso mínimo y te
> dejan controlar exactamente cuántas escrituras ocurren. Por eso son las que usa CMSIS y las que
> enseñamos en todo el curso. Conocé las dos, y sabé por qué elegís cada una.

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

// packed: sin padding (struct que mapea un protocolo o un registro exacto; ver cap. 13)
typedef struct __attribute__((packed)) { uint8_t cmd; uint32_t val; } trama_t;

// section: ubicar el símbolo en una sección concreta del linker (ej. la tabla de vectores)
__attribute__((section(".isr_vector"), used))
const void *vectores[] = { /* ... */ };

// weak: símbolo "débil", que otro .c puede sobrescribir sin error de doble definición
__attribute__((weak)) void SysTick_Handler(void) { /* handler por defecto, vacío */ }

// used: "no lo borres aunque parezca que nadie lo usa" (típico junto a section)
```

`section`, `used` y `weak` son la base de la **tabla de vectores y el startup** (Anexo A): los
handlers se declaran `weak` con un cuerpo por defecto, y cuando vos escribís tu `SysTick_Handler`
real, el tuyo "gana". `aligned` y `packed` son los que más vas a tocar en datos.

### Barreras de memoria: `__DMB()` / `__DSB()` y la barrera del compilador

Como vimos en el cap. 12, `volatile` **no** es una barrera de memoria. Hay dos tipos de barrera:

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
| bitfields | structs propias legibles; para registros, máscaras (orden de bits + doble RMW) |
| `-flto` | dejar que el compilador haga inline y borre código muerto **entre** archivos |
| `__attribute__`, `__DMB`/`__DSB`, asm (curiosos) | control fino de memoria y código; startup, DMA, lazos calientes |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.7.4 (especificadores de función, o sea las reglas de `inline` y por qué en un header hace falta `static`), 6.2.2 (enlace interno y externo), 6.7.2.1 ¶11 (campos de bits).
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
**Anterior:** [13 - Structs para hardware](./13-structs-para-hardware.md) ·
**Siguiente:** [15 - Punto fijo vs punto flotante](./15-punto-fijo-vs-flotante.md)
