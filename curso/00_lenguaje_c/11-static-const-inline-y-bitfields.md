# `static`, `const`, `inline` y campos de bits (C embebido fino)

Estas cuatro herramientas del lenguaje aparecen todo el tiempo en código embebido bien escrito (y en
CMSIS). No son "C avanzado": son lo que distingue un código prolijo y eficiente de uno que "anda pero
es un desastre". Vale la pena dominarlas.

## `static`: dos significados, un keyword

`static` hace cosas distintas según dónde lo pongas. Las dos son útiles en embebidos.

### 1. Variable local `static`: recuerda su valor entre llamadas

Una variable local normal nace y muere en cada llamada (vive en el stack). Una `static` **conserva su
valor** entre llamadas (vive en `.data`/`.bss`, módulo 16):

```c
void contar(void) {
    static uint32_t veces = 0;   // se inicializa UNA sola vez
    veces++;                     // recuerda cuántas veces se llamó
}
```

Es el patrón que usamos en las **tareas no bloqueantes** (módulo 17) para que cada tarea "recuerde"
cuándo actuó por última vez (`static uint32_t t_prev`). Sin `static`, ese valor se perdería en cada
vuelta del superloop.

### 2. Variable/función global `static`: privada de su archivo

Puesto sobre una variable global o una función, `static` la hace **invisible desde otros archivos**
(solo se usa dentro del `.c` donde está). Es el equivalente a "privado":

```c
static uint8_t buffer[256];        // solo este .c puede usar 'buffer'
static void rutina_interna(void) { ... }   // función auxiliar, no parte de la API pública
```

Lo viste en `mygpio.c` (módulo 2): la tabla `static MYGPIO_Port * const puertos[5]` es un detalle de
implementación que el usuario de la librería no ve. **Usar `static` para todo lo que no es API pública
es buena práctica**: evita choques de nombres y deja claro qué es interno.

### El concepto de fondo: *storage class* y *linkage*

Lo que hace `static` se entiende mejor con dos ideas que el estándar usa para clasificar cada
identificador:

- **Duración de almacenamiento (*storage duration*)**: cuánto vive la variable.
  - *automática*: locales normales; nacen y mueren con la función (viven en el stack).
  - *estática*: globales y `static` locales; viven **todo el programa** (en `.data`/`.bss`).
- **Enlace (*linkage*)**: si el nombre es visible desde otros archivos `.c`.
  - **externo**: visible desde todo el programa. Es lo que tienen por defecto las globales y
    funciones. Otro archivo puede usarlas declarándolas con `extern`.
  - **interno**: visible **solo dentro de su `.c`**. Es lo que les da `static` a nivel de archivo.
  - *sin enlace*: las locales (cada una es propia de su bloque).

Con esa terminología, los dos usos de `static` quedan claros:

| `static` en... | Cambia la... | Efecto |
|----------------|--------------|--------|
| variable **local** | duración (automática → estática) | recuerda su valor entre llamadas |
| variable/función **global** | enlace (externo → interno) | privada de su archivo |

Y el complemento de `static` a nivel global es **`extern`**: la declaración "esta variable/función
existe, pero está definida en otro archivo". Es lo que ponés en un header para exponer una global
del módulo:

```c
// en sensor.h:
extern volatile uint32_t sensor_ticks;   // declaración: "existe, vive en otro .c"
// en sensor.c:
volatile uint32_t sensor_ticks = 0;      // definición: acá vive de verdad (1 sola vez en todo el programa)
```

## `const`: más de lo que parece

Ya vimos `const` en el [capítulo 08](./08-tipos-de-ancho-fijo-y-volatile.md). Dos usos finos más:

### Datos `const` van a Flash (ahorran RAM)
Una tabla global `const` se ubica en **Flash** en vez de gastar RAM (la Flash sobra, la RAM es
escasa):

```c
const uint16_t tabla_seno[256] = { 512, 524, ... };   // vive en Flash, 0 bytes de RAM
```

### `const` con punteros: leer la posición
La posición del `const` cambia qué es constante. Leelo de derecha a izquierda:

```c
const uint8_t *p;          // puntero a uint8_t const: no podés cambiar *p, sí p
uint8_t * const p;         // puntero const a uint8_t: no podés cambiar p, sí *p
const uint8_t * const p;   // ambos const
```

Esto es justo lo que CMSIS usa con `__I` (`volatile const`): un registro de **solo lectura** (un
registro de estado) se declara así para que el compilador te frene si intentás escribirlo.

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
con el toolchain en el [Módulo 18](../18_toolchain_y_entorno/).

## Campos de bits (*bitfields*): registros sin máscaras

Una alternativa a las máscaras (`REG |= (1u << 3)`) es describir un registro como una `struct` con
**campos de bits**, donde cada campo ocupa la cantidad de bits que le declarás:

```c
typedef struct {
    uint32_t ENABLE   : 1;   // bit 0
    uint32_t TICKINT  : 1;   // bit 1
    uint32_t CLKSOURCE: 1;   // bit 2
    uint32_t reserved : 13;
    uint32_t COUNTFLAG: 1;   // bit 16
} SysTick_CTRL_bits;
```

Y se usa como campos normales, sin máscaras:

```c
ctrl.ENABLE = 1;       // en vez de  REG |= (1u << 0)
ctrl.CLKSOURCE = 1;    // en vez de  REG |= (1u << 2)
```

Se lee precioso. **Pero ojo con dos trampas** (por eso CMSIS prefiere las máscaras):

1. **El orden de los bits no está garantizado por el estándar.** Que `ENABLE:1` sea el bit 0 depende
   del compilador (la mayoría en ARM lo pone como esperás, pero el estándar no obliga). Para un
   registro de hardware, el bit exacto **sí importa**, así que es un riesgo de portabilidad.
2. **El compilador puede generar accesos raros** (leer-modificar-escribir múltiples), no siempre la
   instrucción mínima.

> Conclusión práctica: los *bitfields* son cómodos y legibles para **tus propias** estructuras de
> datos. Para **registros de hardware**, las **máscaras** (`|=`, `&= ~`, `<<`) son más seguras y
> predecibles: por eso son las que enseñamos en todo el curso y las que usa CMSIS. Conocé los dos.

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

// packed: sin padding (struct que mapea un protocolo o un registro exacto; ver cap. 10)
typedef struct __attribute__((packed)) { uint8_t cmd; uint32_t val; } trama_t;

// section: ubicar el símbolo en una sección concreta del linker (ej. la tabla de vectores)
__attribute__((section(".isr_vector"), used))
const void *vectores[] = { /* ... */ };

// weak: símbolo "débil", que otro .c puede sobrescribir sin error de doble definición
__attribute__((weak)) void SysTick_Handler(void) { /* handler por defecto, vacío */ }

// used: "no lo borres aunque parezca que nadie lo usa" (típico junto a section)
```

`section`, `used` y `weak` son la base de la **tabla de vectores y el startup** (Módulo 16): los
handlers se declaran `weak` con un cuerpo por defecto, y cuando vos escribís tu `SysTick_Handler`
real, el tuyo "gana". `aligned` y `packed` son los que más vas a tocar en datos.

### `restrict`: "este puntero es el único que apunta acá"

`restrict` (C99) es una promesa tuya al compilador: "a esta zona de memoria no se llega por ningún
otro puntero". Eso le permite optimizar (no recargar valores por miedo a *aliasing*). Útil en
funciones de copia/proceso de buffers en lazos calientes:

```c
void mezclar(int32_t *restrict dst, const int32_t *restrict a,
             const int32_t *restrict b, size_t n) {
    for (size_t i = 0; i < n; i++) dst[i] = a[i] + b[i];   // el compilador asume que no se solapan
}
```

Si rompés la promesa (pasás punteros que sí se solapan), el comportamiento es indefinido. Úsalo solo
cuando estés seguro.

### Barreras de memoria: `__DMB()` / `__DSB()` y la barrera del compilador

Como vimos en el cap. 08, `volatile` **no** es una barrera de memoria. Hay dos tipos de barrera:

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
| `static` (local) | que una variable recuerde su valor entre llamadas (tareas no bloqueantes) |
| `static` (global/función) | hacer privado lo que no es API pública (enlace interno) |
| `extern` | declarar una global/función que vive en otro `.c` |
| `const` | solo-lectura + mandar datos a Flash; con punteros, marcar qué es constante |
| `static inline` | helpers chicos rápidos y legibles en headers (como CMSIS), sin choque de símbolos |
| bitfields | structs propias legibles; para registros, mejor máscaras |
| `__attribute__`, `restrict`, `__DMB`/`__DSB` (curiosos) | control fino de memoria y código; startup, DMA, lazos calientes |

---

**Anterior:** [10 - Preprocesador](./10-preprocesador.md) ·
**Siguiente:** [12 - Punto fijo vs punto flotante](./12-punto-fijo-vs-flotante.md)
