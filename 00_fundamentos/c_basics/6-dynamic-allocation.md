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
* **Fragmentación**: asignar y liberar bloques de distintos tamaños deja “huecos”. Con el tiempo, puedes tener RAM total suficiente pero **no contigua** para la siguiente asignación.
* **Fugas y dobles liberaciones**: un `free` olvidado o repetido no siempre se detecta; en micros sin MMU suele terminar en cuelgues silenciosos.
* **Colisión heap–stack**: al crecer pila y heap desde extremos opuestos, una racha de asignaciones puede chocar con la pila y corromper estado.
* **Sobrecoste de runtime**: el *allocator* añade metadatos y lógica extra; pagas RAM y CPU por cada asignación.
* **Concurrencia**: en RTOS, `malloc` suele estar protegido con locks → **bloqueos** y latencias; desde una ISR, generalmente **prohibido**.
* **Portabilidad y depuración**: sin herramientas de profiling/ASan, diagnosticar fallos del heap en campo es caro y lento.

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
* **Trazabilidad**: puedes auditar cuánta RAM se usa (`used`) y definir límites claros.
* **Simplicidad**: menos puntos de fallo, más fácil de testear.

---

## Si **sí o sí** necesito memoria dinámica…

Si no hay alternativa (p. ej., cargas desde SD de tamaño variable grande):

* **Hacer todas las asignaciones al inicio** (fase de *init*) y evitar `free` durante operación.
* **Usa bloques de tamaño fijo** (pool) con un *free-list*; así eliminas fragmentación externa.
* **Nunca desde ISR**, y con RTOS, piensa en la latencia de los locks.
* **Monitorear**: contador de fallos de asignación, watermark de heap y reinicios seguros.

---
