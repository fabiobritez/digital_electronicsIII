# Asignación dinámica de memoria (y por qué casi no vas a usarla)

En este capítulo se junta todo lo anterior: punteros, dónde vive cada variable y qué pasa cuando la RAM
se acaba. La conclusión va adelante, porque es la parte importante:

> **En firmware para un micro como el LPC1769, la respuesta por defecto es no usar `malloc`.** No es una
> manía de puristas: es lo que exigen las normas de las industrias donde un cuelgue cuesta caro. En
> este capítulo vas a ver por qué, y cada afirmación viene con el comando para que la compruebes vos
> con el toolchain que ya tenés.

Ahora bien, "no lo uses" sin entender qué es sirve de poco. Así que primero vemos qué problema resuelve,
después qué cuesta de verdad en un micro, y al final las tres alternativas que sí se usan.

---

## 1. Qué problema resuelve

Cuando escribís un programa, muchas veces no sabés de antemano cuánta memoria vas a necesitar:

- Una línea de texto que llega por UART puede tener 10 o 200 caracteres.
- Un archivo de una microSD puede pesar 100 bytes o 20 KB.
- Una lista de dispositivos conectados crece y se achica mientras el sistema corre.

La **asignación estática** obliga a decidir el tamaño en tiempo de compilación: reservás el máximo
posible y listo. La **asignación dinámica** te deja pedir memoria mientras el programa corre, con la
cantidad exacta que hace falta en ese momento.

```c
#include <stdlib.h>

char *buf = malloc(n);      // pedir n bytes; devuelve NULL si no hay
if (buf == NULL) {
    // manejar el fallo
}
// ... usar buf ...
free(buf);                  // devolverlos
```


| Función             | Qué hace                                                                                            |
| ------------------- | --------------------------------------------------------------------------------------------------- |
| `malloc(n)`         | Reserva `n` bytes **sin inicializar**. Devuelve `void `*, o `NULL` si falla                         |
| `calloc(cant, tam)` | Reserva `cant * tam` bytes y los **pone en cero**. Además detecta el desborde de esa multiplicación |
| `realloc(p, n)`     | Cambia el tamaño del bloque `p` a `n` bytes, copiando el contenido si tiene que moverlo             |
| `free(p)`           | Devuelve el bloque. `free(NULL)` es válido y no hace nada                                           |


En una PC esto funciona bien porque hay un sistema operativo con MMU, gigabytes de RAM y un asignador
muy trabajado. En un micro no hay nada de eso, y ahí empiezan los problemas.

---

## 2. En el LPC1769 no hay sistema operativo: ¿de dónde sale el heap?

En Linux, `malloc` le pide páginas al kernel. En el LPC1769 **no hay a quién pedirle**. El heap es
simplemente una región de RAM que el *linker script* dejó libre, y a la que newlib llega a través de
una función que **vos tenés que escribir**, llamada `_sbrk`, que corre el "tope" del heap hacia arriba.

```
RAM (64 KB en el LPC1769)
0x10000000                                                        0x10008000
    |                                                                  |
    v                                                                  v
    +--------+--------+------------------------+---------+-------------+
    | .data  |  .bss  |  heap  --->            |  libre  |   <--- stack|
    +--------+--------+------------------------+---------+-------------+
                      ^                                  ^
                    _end                          tope de la RAM
              (símbolo del linker)          (donde arranca el stack)
```

El heap crece hacia arriba y el stack hacia abajo, **desde extremos opuestos del mismo espacio libre**.
Nadie vigila el medio: el Cortex-M3 no tiene MMU. Si se cruzan, la siguiente escritura del stack pisa
un bloque del heap, o al revés, y el sistema falla de una manera que no se parece en nada a la causa.

> Un `_sbrk` correcto **tiene que comprobar el puntero de pila** antes de entregar memoria, y devolver
> `(void *) -1` si el heap alcanzaría al stack. Muchos `_sbrk` de ejemplo que circulan por internet no
> lo hacen, y en ese caso `malloc` te devuelve alegremente un puntero a memoria que el stack va a pisar.
> Cómo se escribe uno bien, y de dónde salen `_end` y los demás símbolos, está en
> [16 - Linker y startup](../anexos/A_build_linker_startup/02-linker-y-startup.md#el-linker-script-ld-el-mapa-de-memoria).
> El panorama de las cuatro zonas de memoria está en
> [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md#zona-3-el-heap).

**Conclusión de esta sección:** en embebido el heap no "viene con el sistema". Es RAM que vos separaste,
gestionada por código que vos enlazaste, con una función de crecimiento que vos escribiste. Todo lo que
salga mal ahí es tuyo.

---

## 3. Qué cuesta, medido

Esto no hay que creerlo: se puede medir, y el experimento es corto. Escribí dos programas que hagan
exactamente lo mismo, guardar un byte en 64 bytes de memoria, pero consiguiendo esa memoria de las dos
maneras.

`sin.c`, con memoria estática:

```c
#include <stdint.h>

volatile uint8_t sink;      /* para que el compilador no borre el buffer */

int main(void) {
    static uint8_t buf[64];
    buf[0] = 1;
    sink = buf[0];
    for (;;) { }
}
```

`con.c`, con memoria dinámica:

```c
#include <stdint.h>
#include <stdlib.h>

volatile uint8_t sink;      /* para que el compilador no borre el buffer */

int main(void) {
    uint8_t *buf = malloc(64);
    if (buf != NULL) {
        buf[0] = 1;
        sink = buf[0];
        free(buf);
    }
    for (;;) { }
}
```

> Fijate el `sink`. Si no leyeras el buffer nunca, el compilador con `-O2` se daría cuenta de que
> escribirlo no sirve para nada y lo borraría entero: medirías cero y sacarías la conclusión
> equivocada. Es un detalle a tener siempre en cuenta al medir código optimizado, y de paso es
> `volatile` haciendo exactamente su trabajo, que es el tema del
> [capítulo 12](./12-volatile-y-tipos-para-hardware.md).

Compilalos los dos igual (mismo `-O2`, misma eliminación de secciones muertas) y comparalos con
`arm-none-eabi-size`, que te dice cuánto ocupa cada sección del binario:

```console
$ arm-none-eabi-size sin.elf con.elf
   text    data     bss     dec     hex  filename
     12       0       4      16      10  sin.elf     <- buffer estático
   2212    2112     100    4424    1148  con.elf     <- malloc + free
```

Leé la diferencia con calma, porque es más grande de lo que cualquiera espera: **una sola llamada a
`malloc` arrastró 2212 bytes de Flash y 2212 bytes de RAM**. Y ojo que esa RAM no es el buffer que
pediste: son las estructuras internas del asignador, que están ahí ocupando lugar desde antes de que
tu programa arranque.

¿Querés ver de dónde salen? `arm-none-eabi-nm` lista los símbolos del binario, y con `--size-sort` los
ordena por tamaño:

```console
$ arm-none-eabi-nm --size-sort -S con.elf | tail -4
00008690 0000020c T _free_r               <-  524 B de código, solo para liberar
000188a8 00000408 D __malloc_av_          <- 1032 B: los "bins" de bloques libres
00018cc0 00000428 d impure_data           <- 1064 B: estado reentrante de newlib
00008058 0000055c T _malloc_r             <- 1372 B de código
```

La segunda columna es el tamaño en hexa y la primera la dirección, así que se lee de abajo hacia
arriba: los dos símbolos más grandes, `impure_data` y `__malloc_av_`, suman 2096 bytes y **son datos,
no código**. Ahí se fue casi toda la RAM.

Ahora probá de nuevo agregando `--specs=nano.specs`, que le pide al linker la versión reducida de la
librería C (newlib-nano, que es la que conviene en un micro). El asignador es mucho más chico, pero
seguí mirando: tampoco es gratis.

```console
$ arm-none-eabi-size con.elf con_nano.elf
   text    data     bss     dec     hex  filename
   2212    2112     100    4424    1148  con.elf
    480     100      32     612     264  con_nano.elf
```

De 2212 bytes de RAM a 100: veinte veces menos. Vale la pena saber que esa opción existe, aunque la
conclusión del capítulo no cambie.

**Y además hay un costo por cada bloque.** El asignador tiene que recordar de qué tamaño es cada
bloque para que `free` sepa cuánto devolver, y esa metadata va pegada a tu memoria. En newlib-nano son
**8 bytes de encabezado por asignación**, con alineación a 8 y un chunk mínimo de 12 bytes. O sea que
`malloc(1)` no te cuesta 1 byte: te cuesta 12. Cien objetos chiquitos son 800 bytes de puro
encabezado, sobre 64 KB de RAM total.

---

## 4. Los cuatro problemas serios

Que sea caro es lo de menos. Lo grave es que introduce modos de falla que no podés acotar.

### 4.1 Fragmentación: te queda RAM, pero no te sirve

Pedir y liberar bloques de distintos tamaños deja el heap lleno de agujeros. Podés tener 2 KB libres
en total y aun así fallar al pedir 1 KB, porque no hay 1 KB **contiguo**.

```
Heap después de un rato de uso:

[ usado ][libre][ usado ][libre][ usado ][libre][ usado ][libre]
          64 B            64 B            64 B            64 B

Total libre: 2048 B.   malloc(1024) -> NULL.
```

Esto es lo que hace a la fragmentación tan traicionera: **el fallo depende de la historia completa** de
asignaciones y liberaciones desde que el equipo arrancó. Un sistema que se probó una semana en el
laboratorio puede fallar a los cuatro meses en el campo, y no vas a poder reproducirlo.

No hay forma de "arreglarlo" con un allocator mejor. Un asignador con *coalescing* (que junta huecos
contiguos, como `heap_4` de FreeRTOS) reduce el problema pero no lo elimina, porque no puede mover
bloques que ya entregaste: alguien tiene punteros apuntando ahí.

### 4.2 No es determinista

`malloc` recorre listas de bloques libres. Cuánto tarda depende de cuántos bloques hay y de cómo
quedaron. No es una operación de tiempo acotado, y en un lazo de control o una rutina de interrupción
eso es inaceptable: no podés calcular el peor caso de tu sistema si una de las piezas no tiene peor
caso conocido.

### 4.3 El fallo no tiene salida

En una PC, si `malloc` devuelve `NULL` el programa aborta y el usuario lo vuelve a abrir. En un
controlador de motor no hay usuario ni forma de "volver a abrir". Y como el fallo aparece recién cuando
el heap está fragmentado, aparece **tarde**, en operación, no en la prueba.

Esto además genera un patrón de código que casi nadie escribe bien:

```c
char *buf = malloc(n);
buf[0] = 'x';        // ¡BUG! Si malloc falló, esto escribe en la dirección 0
```

Chequear el retorno de `malloc` es obligatorio, y después hay que decidir **qué hacer** cuando falla,
que es la pregunta difícil y la que no tiene una buena respuesta en un sistema empotrado.

### 4.4 No es reentrante, y en bare metal ni siquiera está protegido

Este es el que más sorprende de los cuatro, y también se puede comprobar sin creerle a nadie: la
librería está en tu disco y se puede abrir.

La documentación dice que newlib protege el heap con dos ganchos, `__malloc_lock` y `__malloc_unlock`.
Suena tranquilizador. Vayamos a ver qué hacen realmente. Primero hay que sacar el módulo de la
librería con `ar`, y después desensamblarlo con `objdump`:

```console
$ arm-none-eabi-ar x libc.a lib_a-mlock.o
$ arm-none-eabi-objdump -d lib_a-mlock.o
00000000 <__malloc_lock>:
   0:   4801        ldr r0, [pc, #4]
   2:   f7ff bffe   b.w 0 <__retarget_lock_acquire_recursive>
```

O sea que el candado no está acá: `__malloc_lock` se limita a llamar a otra función,
`__retarget_lock_acquire_recursive`. Sigamos el hilo un paso más y miremos esa:

```console
$ arm-none-eabi-ar x libc.a lib_a-lock.o
$ arm-none-eabi-objdump -d lib_a-lock.o
00000000 <__retarget_lock_acquire_recursive>:
   0:   4770        bx  lr
```

Ahí terminó el camino, y el candado resultó ser **una sola instrucción: `bx lr`**, que es como el
Cortex-M3 escribe "volvé de la función". Dicho de otro modo: **no hace absolutamente nada**. Es una
función vacía, y lo mismo pasa en newlib-nano.

El nombre lo dice, si uno lo lee con atención: *retarget* significa "reapuntar". Son ganchos pensados
para que **vos** los reemplaces con una implementación de verdad si tu sistema la necesita. Mientras
no lo hagas, el heap queda sin protección alguna.

La consecuencia es concreta: si tu `main` está en la mitad de un `malloc`, con las listas de bloques
libres a medio actualizar, y entra una interrupción que llama a `malloc` (o a `printf`, que llama a
`malloc`), **el heap queda corrupto**. No hay ningún candado que lo impida. Y la corrupción no se nota
en ese momento: se nota mucho después, en un `free` cualquiera que sigue un puntero basura.

> **Por eso la regla "nunca llames a `malloc` desde una ISR" no es una recomendación de estilo: es
> obligatoria.** Y lo mismo vale para todo lo que asigne por debajo, empezando por `printf`.
> Si usás un RTOS, tenés que proveer vos las implementaciones de esos ganchos (o usar el asignador del
> RTOS, que es lo habitual).

---

## 5. Qué dicen las normas serias

Las industrias que no pueden permitirse un cuelgue llegaron todas a la  misma conclusión.

**MISRA C:2012, Regla 21.3** (categoría *Required*, la más estricta):

> *The memory allocation and deallocation functions of `<stdlib.h>` shall not be used.*

Prohíbe `malloc`, `calloc`, `realloc` y `free`, sin excepciones, en código automotriz. La razón que da
la norma es que su uso conduce a comportamiento indefinido: liberar memoria no asignada dinámicamente,
usar punteros a memoria ya liberada, y demás.

**"The Power of 10", de la NASA/JPL** (Holzmann, *IEEE Computer* 39(6), 2006), diez reglas para código
crítico de vuelo. La **Regla 3** es:

> *Do not use dynamic memory allocation after initialization.*

Es decir: si necesitás asignar, hacelo todo durante el arranque y no liberes nunca más. Un satélite que
tiene que funcionar años sin reiniciar no puede convivir con fragmentación ni con fugas.

**SEI CERT C** no lo prohíbe, pero le dedica una familia entera de reglas al tema, lo cual dice bastante
sobre cuántas maneras hay de equivocarse: `MEM31-C` (liberar la memoria cuando ya no se usa),
`MEM34-C` (liberar solo memoria asignada dinámicamente), `MEM35-C` (reservar memoria suficiente para el
objeto), entre otras.

**FreeRTOS**, que sí tiene que ofrecer asignación dinámica, lo resuelve **no usando la del sistema**:
trae cinco asignadores propios (`heap_1` a `heap_5`) para que elijas el que se banque tu caso.
`heap_1` ni siquiera implementa liberar, y es el recomendado para los sistemas más críticos justamente
por eso. `heap_4`, el de uso general, agrega fusión de bloques adyacentes para mitigar la fragmentación.
Que un RTOS maduro considere razonable un asignador **que no puede liberar** te dice todo sobre cómo se
piensa este problema en embebido.

---

## 6. Las tres alternativas que sí se usan

### 6.1 Asignación estática: la respuesta correcta el 90 % de las veces

Declarás de antemano lo que vas a necesitar. El linker reserva esa RAM en `.bss` y **el programa no
puede quedarse sin memoria en ejecución**, porque no hay nada que pedir: si entra en la RAM, entra
siempre; y si no entra, no enlaza. El fallo se mueve de las tres de la mañana en el campo a tu pantalla
en tiempo de compilación, que es exactamente adonde lo querés.

```c
static uint8_t   rx_buffer[256];    // buffer de UART: tamaño máximo conocido
static sensor_t  sensores[8];       // hasta 8 sensores, y no más
```

La objeción típica es "pero desperdicio RAM si uso menos". Es cierto, y casi siempre no importa: en un
sistema empotrado te interesa mucho más saber que **nunca** vas a quedarte sin memoria que exprimir los
últimos kilobytes. Además, si dimensionaste para el peor caso, la memoria "desperdiciada" es
exactamente la que ibas a necesitar el día que llegue el peor caso.

**Casi todo problema de embebido tiene una cota conocida.** Si no la encontrás, suele ser señal de que
el requerimiento está mal definido, no de que necesites un heap.

### 6.2 Arena (*bump allocator*): tamaño variable, total acotado

Un área estática y un puntero que avanza. Sirve cuando necesitás piezas de tamaño variable pero el
**total** está acotado, y podés liberar todo junto (por ejemplo, al terminar de procesar un mensaje).

```c
#include <stdint.h>
#include <stddef.h>

#define ARENA_BYTES 512

static uint8_t arena[ARENA_BYTES] __attribute__((aligned(8)));
static size_t  arena_usado = 0;
static size_t  arena_pico  = 0;     // marca de agua, para dimensionar de verdad

void *arena_reservar(size_t bytes, size_t alineacion) {
    // redondear el offset hacia arriba al múltiplo de 'alineacion'
    size_t inicio = (arena_usado + (alineacion - 1u)) & ~(alineacion - 1u);

    if (bytes > ARENA_BYTES - inicio) {   // así, la suma no puede desbordar
        return NULL;                      // fallo explícito y controlado
    }

    arena_usado = inicio + bytes;
    if (arena_usado > arena_pico) {
        arena_pico = arena_usado;
    }
    return &arena[inicio];
}

void   arena_reset(void)     { arena_usado = 0; }   // libera TODO de una vez
size_t arena_pico_uso(void)  { return arena_pico; }
```

> [!IMPORTANT]
> **El parámetro `alineacion` no es un adorno.** Un asignador lineal ingenuo que hace
> `return &storage[used];` devuelve punteros con alineación arbitraria. Si el que llama guarda ahí un
> `uint32_t`, estás construyendo un puntero desalineado, y desreferenciarlo es **comportamiento
> indefinido** en C, independientemente de que el Cortex-M3 tolere accesos desalineados en algunas
> instrucciones. El compilador asume que los punteros están alineados y optimiza en consecuencia.
>
> ```c
> arena_reservar(1, 1);                     // deja el offset en 1
> uint32_t *p = arena_reservar(4, 1);       // p queda en offset 1: DESALINEADO
> ```
>
> Probalo vos: imprimí `(uintptr_t)p % 4` en las dos versiones del asignador. Con el redondeo del
> parámetro `alineacion`, ese mismo pedido devuelve el offset 4 y el resto da 0, que es lo que
> querías.

La **marca de agua** (`arena_pico`) es la parte que más se olvida y la más útil: te deja medir cuánto
llegaste a usar de verdad y dimensionar la arena con datos en vez de con intuición. Exponela por UART o
por el debugger y miralo después de una sesión larga.

### 6.3 Pool de bloques fijos: pedir y devolver, sin fragmentación

Cuando sí necesitás reservar y liberar objetos **individualmente** (nodos de una cola de eventos,
descriptores de conexión), la solución determinista es un pool de bloques **todos del mismo tamaño**.
Como son iguales, un bloque liberado sirve siempre para el próximo pedido: **la fragmentación externa
desaparece por construcción**.

```c
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define POOL_N 8

typedef struct objeto {
    int            dato;
    struct objeto *next;    // reutilizado como enlace de la free-list cuando está libre
} objeto_t;

static objeto_t  pool[POOL_N];      // los 8 objetos viven en .bss
static objeto_t *libre;             // cabeza de la lista de bloques libres
static uint8_t   en_uso[POOL_N];    // para detectar doble free
static size_t    pool_vivos = 0, pool_pico = 0;

void pool_init(void) {
    for (size_t i = 0; i < POOL_N - 1u; i++) {
        pool[i].next = &pool[i + 1];
    }
    pool[POOL_N - 1u].next = NULL;
    libre = &pool[0];
    memset(en_uso, 0, sizeof en_uso);
    pool_vivos = 0;
    pool_pico  = 0;
}

objeto_t *pool_alloc(void) {        // O(1)
    if (libre == NULL) {
        return NULL;                // pool agotado: fallo explícito, no cuelgue
    }
    objeto_t *o = libre;
    libre = libre->next;
    o->next = NULL;
    en_uso[o - pool] = 1;
    pool_vivos++;
    if (pool_vivos > pool_pico) {
        pool_pico = pool_vivos;
    }
    return o;
}

void pool_free(objeto_t *o) {       // O(1)
    if (o == NULL) {
        return;                              // como free(NULL): no hacer nada
    }
    assert(o >= pool && o < pool + POOL_N);  // ¿es un puntero de este pool?
    assert(en_uso[o - pool] == 1);           // ¿ya estaba liberado? (doble free)
    en_uso[o - pool] = 0;
    o->next = libre;
    libre = o;
    pool_vivos--;
}
```

Frente a `malloc`, el pool gana en todo lo que importa acá: **tiempo constante**, **cero fragmentación
externa**, **RAM acotada y auditable**, y el fallo es un `NULL` explícito en lugar de un cuelgue. Es el
patrón que usan los RTOS (los *memory pools* de CMSIS-RTOS, las *block pools* de FreeRTOS) y las pilas
de red embebidas.

> **El truco de la free-list:** un bloque libre no guarda datos útiles, así que reutilizamos su propia
> memoria para el puntero `next`. El pool no gasta ni un byte extra en metadata de enlace.

> **Los dos `assert`** convierten dos bugs silenciosos (liberar un puntero ajeno, liberar dos veces) en
> una parada inmediata y localizada durante el desarrollo. En la compilación de producción se
> desactivan definiendo `NDEBUG`, así que no cuestan nada en el binario final. Es lo que `free` **no**
> puede hacer por vos.

### Cuál elegir


| Necesidad                                                         | Herramienta                                 |
| ----------------------------------------------------------------- | ------------------------------------------- |
| Tamaño y cantidad conocidos                                       | **Estática** (`static uint8_t buf[N]`)      |
| Piezas de tamaño variable, total acotado, se liberan todas juntas | **Arena**                                   |
| Objetos iguales, se piden y devuelven de a uno                    | **Pool**                                    |
| Nada de lo anterior alcanza                                       | Revisá el requerimiento antes de ir al heap |


---

## 7. Si de verdad no hay alternativa

Existen casos legítimos: cargar un archivo de la SD cuyo tamaño se conoce recién al abrirlo, o
integrar una librería de terceros que asigna internamente. Si llegaste ahí:

1. **Asigná todo en el arranque y no liberes nunca.** Es la Regla 3 del Power of 10. Sin `free` no hay
  fragmentación, ni doble liberación, ni uso después de liberar. El heap se comporta como asignación
   estática decidida en ejecución.
2. **Chequeá siempre el retorno**, y definí de antemano qué hace el sistema cuando falla (reiniciar de
  forma controlada suele ser mejor que seguir con un puntero inválido).
3. **Nunca desde una ISR**, por la sección 4.4. Tampoco `printf` desde una ISR, por lo mismo.
4. **Usá un `_sbrk` que compare contra el puntero de pila** y devuelva `(void *) -1` si el heap
  alcanzaría al stack.
5. **Preferí `calloc(cant, tam)` a `malloc(cant * tam)`**: `calloc` detecta el desborde de la
  multiplicación, que es una vulnerabilidad clásica.
6. **Cuidado con `realloc`**: si falla devuelve `NULL` **sin liberar el bloque original**. Escribir
  `p = realloc(p, n);` es una fuga garantizada el día que falle. Usá un temporal:
7. **Poné `p = NULL` después de cada `free`.** No arregla todo, pero convierte un *use after free* en
  una desreferencia de `NULL`, que falla de inmediato y cerca del error.
8. **Medí la marca de agua del heap** y monitoreá los fallos de asignación como una métrica del sistema.

---

## 8. Cómo demostrar que tu firmware no usa heap

"Creo que no uso `malloc`" no es lo mismo que saberlo: se cuela por debajo, sobre todo a través de
`printf` y de librerías de terceros. Acá van tres formas de salir de la duda, de menor a mayor
compromiso, todas con el toolchain que ya tenés instalado.

**a) Buscar los símbolos en el binario final.** Lo más simple:

```console
$ arm-none-eabi-nm firmware.elf | grep -E ' (t|T) (malloc|_malloc_r|_sbrk)'
00008024 T malloc
00008044 T _malloc_r
0000801c T _sbrk
```

Si no sale nada, no hay asignador enlazado. Si sale, alguien lo está usando; el archivo `.map` del
linker te dice quién.

**b) Hacer que el enlace falle si alguien llama a `malloc`.** Con `--wrap`, el linker redirige las
llamadas a `__wrap_malloc`, que no existe:

```console
$ arm-none-eabi-gcc ... -Wl,--wrap=malloc -o firmware.elf
ld: main.o: in function `main':
main.c:(.text): undefined reference to `__wrap_malloc'
```

**c) Envenenar el símbolo directamente**, que da un mensaje más explícito:

```console
$ arm-none-eabi-gcc ... -Wl,--defsym=malloc=__PROHIBIDO_USAR_MALLOC -o firmware.elf
ld: --defsym:1: undefined symbol `__PROHIBIDO_USAR_MALLOC' referenced in expression
```

Cualquiera de las dos últimas, puesta en el `Makefile`, convierte la política de "no usamos heap" en
algo que el build **hace cumplir**, en lugar de un comentario que alguien va a ignorar en seis meses.

---

## 9. Resumen de reglas


| Regla                                                 | Por qué                                                         |
| ----------------------------------------------------- | --------------------------------------------------------------- |
| Por defecto, memoria estática                         | El fallo pasa a ser de compilación, no de ejecución en el campo |
| Si necesitás tamaño variable, usá arena o pool        | Deterministas, sin fragmentación, RAM auditable                 |
| Nunca `malloc` (ni `printf`) desde una ISR            | El candado de newlib es un `bx lr`: no protege nada             |
| Si usás heap, asigná todo en el arranque y no liberes | Power of 10, Regla 3: sin `free` no hay fragmentación           |
| Chequeá siempre el retorno de `malloc`/`calloc`       | Escribir en `NULL` es el bug que sigue al fallo                 |
| `realloc` a un temporal, nunca sobre el mismo puntero | Si falla, perdés la única referencia al bloque original         |
| Alineá los punteros que devuelve tu asignador         | Un puntero desalineado es UB, lo tolere o no el hardware        |
| Medí la marca de agua (arena, pool y heap)            | Dimensionar con datos, no con intuición                         |
| Hacé que el build falle si aparece `malloc`           | Una política que no se verifica no existe                       |


---

## Fuentes y para seguir leyendo

**Normas y estándares de codificación**

- **MISRA C:2012, Regla 21.3** (*Required*): "The memory allocation and deallocation functions of `<stdlib.h>` shall not be used". Descripción y ejemplos en la [referencia de MathWorks Polyspace](https://www.mathworks.com/help/bugfinder/ref/misrac2012rule21.3.html).
- **G. J. Holzmann, "The Power of 10: Rules for Developing Safety-Critical Code"**, *IEEE Computer*, vol. 39, n.º 6, pp. 95-99, junio de 2006. La Regla 3 es la que aplica acá. [PDF](https://spinroot.com/gerard/pdf/P10.pdf) · [resumen en Wikipedia](https://en.wikipedia.org/wiki/The_Power_of_10:_Rules_for_Developing_Safety-Critical_Code).
- **SEI CERT C Coding Standard**, sección [Memory Management (MEM)](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/). En particular `MEM31-C`, `MEM34-C` y `MEM35-C`.
- **Barr Group, [Embedded C Coding Standard (BARR-C:2018)](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf)**. Estándar de estilo y seguridad para C embebido, de lectura recomendada completa.

**Implementación real**

- **[FreeRTOS: Heap memory management](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/09-Memory-management/01-Memory-management)** y el [capítulo 3 del libro del kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch03.md). Los cinco asignadores `heap_1` a `heap_5` y cuándo usar cada uno.
- **[newlib: `nano-mallocr.c`](https://github.com/eblot/newlib/blob/master/newlib/libc/stdlib/nano-mallocr.c)**. El código del asignador que enlaza este toolchain con `--specs=nano.specs`. De ahí salen los 8 bytes de encabezado por bloque y el chunk mínimo de 12.
- **[newlib y FreeRTOS, de Dave Nadler](https://nadler.com/embedded/newlibAndFreeRTOS.html)**. Discusión detallada de `__malloc_lock`, `_sbrk` y los problemas de integración reales.

**Para comprobarlo vos mismo**

Ningún número de este capítulo hay que tomarlo por fe: todos salen de cuatro comandos que ya tenés
instalados. Si te queda tiempo, corré el experimento completo, que no lleva más de diez minutos y se
entiende mucho mejor haciéndolo que leyéndolo.

```console
$ arm-none-eabi-size sin.elf con.elf                  # cuánto cuesta en Flash y RAM
$ arm-none-eabi-nm --size-sort -S con.elf             # qué símbolos son los culpables
$ arm-none-eabi-ar x libc.a lib_a-lock.o              # sacar el "candado" de la librería
$ arm-none-eabi-objdump -d lib_a-lock.o               # y ver que está vacío
```

Si alguno te da un número distinto al del capítulo, no es un error tuyo: fijate qué versión de
toolchain estás usando y si estás enlazando con `--specs=nano.specs`. Esos dos detalles cambian los
resultados, y entender **por qué** los cambian es la mitad del aprendizaje.

**En este curso**

- [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md#zona-3-el-heap): las cuatro zonas de memoria y el choque stack/heap.
- [16 - Linker y startup](../anexos/A_build_linker_startup/02-linker-y-startup.md): de dónde sale el heap en el `.ld` y cómo se escribe `_sbrk`.
- [Superloop no bloqueante](./17-superloop-y-codigo-no-bloqueante.md): cómo estructurar el firmware con buffers estáticos y sin asignación en operación.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md) ·
**Siguiente:** [13 - Structs para hardware](./13-structs-para-hardware.md)