# ¿Por qué existe la asignación dinámica de memoria?

Cuando estamos programando, no siempre sabemos de antemano cuánta memoria vamos a necesitar.

* Un string leído por UART podría tener 10 o 100 caracteres.
* Un archivo en una microSD puede variar de unos pocos bytes a varios kilobytes.
* Una estructura para manejar datos de sensores podría crecer si se conectan más dispositivos.

En sistemas como PC o servidores, esto se resuelve fácilmente con **asignación dinámica** usando funciones como `malloc`, `calloc` o `realloc`.
Estas funciones permiten pedir al sistema operativo exactamente la memoria necesaria **en tiempo de ejecución**, adaptándose a la cantidad de datos reales, en lugar de reservar siempre el máximo posible desde el inicio.

La asignación dinámica resuelve problemas como:

* Evitar desperdicio de RAM al no reservar de más.
* Manejar datos cuyo tamaño no se conoce hasta que el programa está corriendo.
* Crear estructuras que crecen o decrecen mientras el programa se ejecuta.

Sin embargo, en sistemas embebidos, esta técnica trae riesgos y limitaciones importantes…

---

## ¿Por qué **no** usar memoria dinámica (`malloc`) en sistemas embebidos?

En embebidos, “funciona” no siempre significa “predecible”. La memoria dinámica introduce incertidumbre difícil de controlar cuando los recursos son escasos y el tiempo real importa.

### Problemas típicos de `malloc`/`free` en embebidos

* **No determinismo en tiempo**: el tiempo de `malloc` y `free` varía según el estado del heap → mala noticia para tareas de tiempo real, ISRs o lazo de control.
* **Fragmentación**: asignar y liberar bloques de distintos tamaños deja “huecos”. Con el tiempo, podés tener RAM total suficiente pero **no contigua** para la siguiente asignación.
* **Fugas y dobles liberaciones**: un `free` olvidado o repetido no siempre se detecta; en micros sin MMU suele terminar en cuelgues silenciosos.
* **Colisión heap–stack**: al crecer pila y heap desde extremos opuestos, una racha de asignaciones puede chocar con la pila y corromper estado.
* **Sobrecoste de runtime**: el *allocator* añade metadatos y lógica extra; pagas RAM y CPU por cada asignación.
* **Concurrencia**: en RTOS, `malloc` suele estar protegido con locks → **bloqueos** y latencias; desde una ISR, generalmente **prohibido**.
* **Portabilidad y depuración**: sin herramientas de profiling/ASan, diagnosticar fallos del heap en campo es caro y lento.

### ¿De dónde sale el heap, en realidad?

En una PC, `malloc` le pide RAM al sistema operativo. En el LPC1769 **no hay sistema operativo**:
el heap es simplemente una **región de RAM que el linker reservó**, entre el final de los datos
(`.bss`) y el inicio del stack. La librería C (newlib) llega a esa región a través de una función
llamada `_sbrk`, que vos (o el startup) tenés que proveer y que "empuja" el tope del heap.

```
RAM:  [ .data | .bss | --- heap crece hacia arriba ---> ...  <--- crece el stack | tope RAM ]
```

Si el heap y el stack se cruzan, no hay MMU que lo detecte: simplemente se corrompen mutuamente y el
sistema falla de forma silenciosa y difícil de reproducir. **`malloc` sin `_sbrk` bien hecho ni
siquiera enlaza** (o enlaza con un `_sbrk` por defecto que devuelve error). Todo esto lo ves en
detalle en [Módulo 16 - Linker y startup](../16_build_linker_startup/02-linker-y-startup.md): el
heap es una decisión del *linker script*, no algo "que viene gratis".

> Conclusión: en embebidos, `malloc` no viene gratis con el sistema: es código y RAM que **vos** pagás,
> con todos los riesgos de arriba. Por eso la regla de oro es evitarlo.

---

## Alternativas a `malloc` (las tres que vas a usar)

### 1. Asignación estática

Lo más simple y predecible: declarás de antemano todo lo que vas a necesitar, como variable global
o `static`. El linker reserva esa RAM en `.bss`/`.data` y **nunca** hay riesgo de fallo de asignación
en tiempo de ejecución.

```c
static uint8_t rx_buffer[256];     // buffer de recepción UART, tamaño máximo conocido
static sensor_t sensores[8];       // hasta 8 sensores, fijo
```

Si tu problema tiene una cota conocida (y casi todos en embebidos la tienen), esta es la respuesta.

### 2. Buffer de tamaño fijo con asignador lineal (bump allocator)

Es la del ejemplo de más abajo: un área estática y un puntero que avanza. Útil para acumular datos
de tamaño variable pero con un **total** acotado. La limitación: no podés liberar bloques sueltos,
solo "resetear" todo de una vez (poniendo `used = 0`).

### 3. Memory pool / object pool (bloques de tamaño fijo)

Cuando sí necesitás pedir **y devolver** objetos individualmente (ej.: nodos de una lista de eventos),
la solución determinista es un **pool de objetos del mismo tamaño**. Como todos los bloques son
iguales, **no hay fragmentación externa**: un bloque liberado siempre sirve para el próximo pedido.

```c
#define POOL_N 8

typedef struct objeto {
    int            dato;
    struct objeto *next;     // se reusa como puntero del "free-list" cuando está libre
} objeto_t;

static objeto_t  pool[POOL_N];   // los 8 objetos viven en .bss (RAM estática)
static objeto_t *libre;          // cabeza de la lista de bloques libres

void pool_init(void) {           // encadena todos los bloques en la free-list
    for (int i = 0; i < POOL_N - 1; i++) pool[i].next = &pool[i + 1];
    pool[POOL_N - 1].next = NULL;
    libre = &pool[0];
}

objeto_t *pool_alloc(void) {     // O(1): saca el primero de la free-list
    if (libre == NULL) return NULL;          // pool agotado -> fallo controlado
    objeto_t *o = libre;
    libre = libre->next;
    return o;
}

void pool_free(objeto_t *o) {    // O(1): lo devuelve al frente de la free-list
    o->next = libre;
    libre = o;
}
```

Ventajas frente a `malloc`: tiempo **constante** (O(1)), **cero fragmentación**, RAM acotada y
auditable, y el fallo es explícito (`NULL`) en vez de un cuelgue. Es el patrón que usan los RTOS
(los "memory pools" de CMSIS-RTOS o las *block pools* de FreeRTOS) y los stacks de red.

> **Truco del free-list:** cuando un bloque está libre no guarda datos útiles, así que reutilizamos
> su propia memoria para guardar el puntero `next`. El pool no gasta RAM extra en metadatos.

---

## Recomendación práctica: **buffer estático** administrado por la aplicación

Cuando el tamaño máximo es acotado y conocido (muy común en embebidos), un **área estática** con un **asignador lineal** (bump allocator) es simple, rápido y 100% determinista.

```c
#define BUFFER_SIZE 512
char storage[BUFFER_SIZE];
size_t used = 0;

char *add_string(const char *s) {
    size_t len = strlen(s) + 1;
    if (used + len > BUFFER_SIZE) return NULL; // no hay espacio
    char *ptr = &storage[used];
    strcpy(ptr, s);
    used += len;
    return ptr;
}
```

### ¿Cómo funciona?

* `storage` es el **pool fijo**.
* `used` marca el **próximo offset libre**.
* `add_string` copia la cadena y devuelve un puntero **estable** dentro del pool.
* Si no hay espacio suficiente, devuelve `NULL` sin tocar memoria → **fallo controlado**.

### Ventajas

* **Tiempo O(1)** y constante: copiar y avanzar un puntero. Ideal para lazos críticos.
* **Sin fragmentación**: el crecimiento es lineal.
* **Trazabilidad**: podés auditar cuánta RAM se usa (`used`) y definir límites claros.
* **Simplicidad**: menos puntos de fallo, más fácil de testear.

---

## Si **sí o sí** necesito memoria dinámica…

Si no hay alternativa (p. ej., cargas desde SD de tamaño variable grande):

* **Hacer todas las asignaciones al inicio** (fase de *init*) y evitar `free` durante operación.
* **Usa bloques de tamaño fijo** (pool) con un *free-list*; así eliminas fragmentación externa.
* **Nunca desde ISR**, y con RTOS, piensa en la latencia de los locks.
* **Monitorear**: contador de fallos de asignación, watermark de heap y reinicios seguros.

---

---

**Anterior:** [08 - Tipos de ancho fijo, volatile y const](./08-tipos-de-ancho-fijo-y-volatile.md) ·
**Siguiente:** [10 - Preprocesador](./10-preprocesador.md)
