# Cuando el superloop no alcanza: intro a RTOS

El superloop con tareas no bloqueantes y máquinas de estado resuelve la enorme mayoría de los
proyectos de la materia. Pero tiene un límite, y conviene saber cuál es y qué hay del otro lado.

Este capítulo cierra el módulo. Es una intro **conceptual**: no vas a configurar FreeRTOS acá. La idea
es que sepas qué es un RTOS, qué problema resuelve, qué cuesta, y sobre todo **cuándo no lo
necesitás**, que es casi siempre al principio.

---

## Dónde se queda corto el superloop

Imaginá que una tarea, por una sola vez, **tarda**: procesa un buffer grande, hace una cuenta pesada,
espera a un sensor lento. Mientras está en eso, **ninguna otra tarea corre**. El LED deja de parpadear
preciso, la respuesta al botón se demora, el buffer de la UART se desborda.

El superloop reparte el CPU **por buena voluntad**: es *cooperativo*, y cada tarea **tiene que** ceder
rápido. Si una se porta mal, arrastra a todas. Ese es el defecto de fondo, y no se arregla escribiendo
mejor: se arregla cambiando de arquitectura.

Los síntomas de que el proyecto ya te queda grande:

- Una tarea ocasionalmente larga **desincroniza** a las demás.
- Necesitás que cierta tarea corra **sí o sí** cada 1 ms, pase lo que pase (control de un motor, un
  lazo PID, generación de una señal).
- Tenés tareas con **prioridades** muy distintas y el reparto manual se volvió un rompecabezas.
- El código de coordinación (banderas, tiempos, quién avisa a quién) creció más que la lógica útil.
- Partir una tarea larga en una máquina de estados te dejó una FSM de veinte estados que existe solo
  para no bloquear, no porque el problema tenga veinte etapas.

Ese último es la señal más clara. Cuando la máquina de estados **no modela el problema sino la
necesidad de ceder el CPU**, estás escribiendo a mano lo que un RTOS hace solo.

---

## Qué es un RTOS

Un RTOS es una pieza chica de software que te deja escribir tu programa como **varias tareas
independientes**, cada una con su propio `while(1)`, **como si cada una tuviera su propio CPU**. El
RTOS se encarga de repartir el CPU real entre ellas.

```c
// Con RTOS, cada tarea es un bucle propio, escrito de forma simple y hasta "bloqueante":
void tarea_led(void *p) {
    while (1) {
        LED_toggle();
        vTaskDelay(pdMS_TO_TICKS(500));   // dormir 500 ms
    }
}

void tarea_sensor(void *p) {
    while (1) {
        leer_sensor();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

La diferencia importante está en ese `vTaskDelay(500)`: **no bloquea el sistema** como el `delay_ms`
del [capítulo 17](./17-superloop-y-codigo-no-bloqueante.md#el-enemigo-delay-bloqueante). Le dice al
RTOS "no me necesitás por 500 ms, dale el CPU a otra tarea". El RTOS **cambia de tarea** y vuelve a
`tarea_led` cuando corresponde.

Ese es el canje central: **volvés a poder escribir código secuencial y bloqueante**, porque bloquear
una tarea ya no bloquea el sistema. La complejidad no desaparece, se muda: del código de tus tareas al
kernel y a la sincronización entre ellas.

---

## Qué es, físicamente, un cambio de tarea

Vale la pena bajarlo a tierra, porque con el [capítulo 10](./10-donde-vive-cada-variable.md) ya tenés
todas las piezas.

Una tarea "que corre" es, en el fondo, **un stack y un juego de registros**. El estado completo de una
función a medio ejecutar está en los registros del CPU (`r0`-`r12`, `sp`, `lr`, `pc`) y en su stack
frame. Cambiar de tarea es literalmente:

1. Guardar los registros de la tarea que sale, en **su** stack.
2. Cambiar el stack pointer al stack de la tarea que entra.
3. Restaurar los registros de esa tarea desde su stack.
4. Saltar: la tarea nueva sigue exactamente donde había quedado, sin enterarse de nada.

A eso se le llama **cambio de contexto** (*context switch*). En Cortex-M el hardware ayuda de dos
maneras que ya viste de refilón:

- **Hay dos stack pointers**, `MSP` y `PSP`
  ([capítulo 10, "para los curiosos"](./10-donde-vive-cada-variable.md#para-los-curiosos-avanzado)).
  El RTOS corre las tareas sobre `PSP` y deja `MSP` para el kernel y las interrupciones. Así cada
  tarea tiene su propio stack sin pisar al de nadie.
- **La excepción `PendSV`** existe justamente para esto: es de prioridad mínima y se dispara "cuando
  no haya nada más urgente", que es el momento exacto en que conviene cambiar de tarea. El RTOS la usa
  para no interrumpir una ISR a la mitad.

El **tick** del planificador lo da el **SysTick** (módulo 6): la misma interrupción periódica que
usabas para `g_millis`, ahora usada por el kernel para decidir si toca cambiar de tarea.

> **Consecuencia directa en RAM:** cada tarea necesita **su propio stack**, dimensionado para su peor
> caso. Diez tareas con 512 bytes cada una son 5 KB de los 32 KB de SRAM principal del LPC1769, más lo
> que ocupen el kernel y las colas. Todo lo del
> [capítulo 10](./10-donde-vive-cada-variable.md#cuánto-stack-usa-tu-programa-y-cómo-saberlo) sobre
> medir el stack deja de ser una curiosidad y pasa a ser obligatorio: ahora tenés que medirlo N veces.

---

## El planificador y las prioridades

El corazón del RTOS es el **planificador** (*scheduler*): decide qué tarea corre en cada momento.
FreeRTOS y la mayoría usan **prioridad fija con desalojo** (*preemptive*): corre siempre la tarea
lista de mayor prioridad, y si aparece una de prioridad más alta, **desaloja** a la que estaba, sin
pedirle permiso.

Esa es la diferencia de fondo con el superloop:

| | Superloop (cooperativo) | RTOS (con desalojo) |
|---|---|---|
| Quién decide cuándo cambiar | la tarea, al retornar | el kernel, en cualquier momento |
| Si una tarea se cuelga | se cuelga todo | las de mayor prioridad siguen corriendo |
| Latencia de la tarea urgente | la vuelta más larga del bucle | unos pocos microsegundos |
| ¿Puedo asumir que nadie me interrumpe? | **sí**, entre llamada y llamada | **no**, nunca |

Esa última fila es la que más cuesta. En un superloop, una tarea sabe que mientras corre nadie más va
a tocar sus datos (salvo las ISRs). Con desalojo, **cualquier línea puede ser interrumpida por otra
tarea**, así que todo dato compartido necesita protección explícita. Es el problema de las
[secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md), ahora entre tareas y
no solo contra interrupciones.

---

## Lo que el RTOS te da, y lo que cuesta

**Te da:**

- Tareas independientes, cada una escrita de forma simple y secuencial.
- Prioridades reales y tiempos mucho más garantizados.
- Herramientas para comunicar tareas con seguridad: **colas** (pasar datos de una tarea a otra sin
  variables globales), **semáforos** (señalizar "ya pasó algo") y **mutex** (proteger un recurso
  compartido, por ejemplo la UART, para que dos tareas no escriban mezclado).

**Cuesta:**

- **RAM y Flash.** Cada tarea lleva su stack; el kernel ocupa unos pocos KB de Flash y una tabla por
  tarea. FreeRTOS es chico, pero no es gratis.
- **Complejidad nueva.** Aparecen problemas que en el superloop no existían: condiciones de carrera
  entre tareas, *deadlocks* (dos tareas esperándose mutuamente para siempre) e **inversión de
  prioridad**.
- **Depuración más difícil.** Un bug que depende del momento exacto en que el planificador cambió de
  tarea no se reproduce apretando F5.

> **La inversión de prioridad, en una línea:** una tarea de baja prioridad toma un mutex, una de alta
> prioridad se queda esperando ese mutex, y en el medio corre una de prioridad media que no necesita
> nada de eso. Resultado: la de prioridad media le gana a la de prioridad alta, que es exactamente lo
> contrario de lo que pediste. Le pasó a la Mars Pathfinder en 1997, en Marte, y se arregló habilitando
> **herencia de prioridad** en el mutex por una actualización remota. La crónica de Glenn Reeves está
> en las fuentes y es de lectura obligatoria: es el mejor ejemplo de por qué esto importa.

---

## ¿Cuándo conviene? La regla práctica

| Usá superloop + FSM si... | Pensá en un RTOS si... |
|---|---|
| pocas tareas, todas cortas | muchas tareas, algunas largas o complejas |
| los tiempos no son críticos al milisegundo | necesitás tiempos garantizados y prioridades reales |
| el micro es chico o la RAM está justa | tenés RAM de sobra (el LPC1769, con 64 KB, da de sobra) |
| querés simplicidad y control total | la coordinación manual ya es más código que la lógica |
| el equipo es de una o dos personas | varias personas escriben tareas independientes |

> **Para la materia:** los proyectos se hacen perfecto con **superloop + tareas no bloqueantes +
> máquinas de estado**. El RTOS es bueno conocerlo como horizonte, y el LPC1769 lo soporta de sobra,
> pero no lo necesitás para empezar. Aprendé primero a no bloquear: el RTOS tiene mucho más sentido
> una vez que entendés **por qué** no hay que bloquear.

Y hay un aviso que la experiencia repite: meter un RTOS **no arregla** un diseño con tareas mal
separadas ni un código que no entendés. Le agrega una capa de indeterminismo encima. Si el superloop
te falla, primero preguntate si el problema es la arquitectura o una sola tarea que hace demasiado.

---

## El escalón intermedio

Entre "superloop puro" y "RTOS completo" hay opciones que a veces alcanzan y cuestan mucho menos:

- **Planificador cooperativo con tabla de tareas.** Es el del
  [capítulo 17](./17-superloop-y-codigo-no-bloqueante.md#subir-un-escalón-la-tabla-de-tareas): te da
  períodos declarativos y un lugar único para instrumentar, sin cambio de contexto ni stacks
  múltiples.
- **Prioridades con las interrupciones que ya tenés.** El NVIC del Cortex-M3 tiene prioridades
  configurables (módulo 7). Poner la tarea crítica de 1 ms en una ISR de alta prioridad y dejar el
  resto en el superloop resuelve muchísimos casos, sin kernel.
- **Protothreads.** Una técnica que simula tareas bloqueantes con macros y un solo stack: te deja
  escribir código secuencial dentro de una tarea del superloop. Muy liviana, con una limitación fuerte
  (las variables locales no sobreviven al "bloqueo", tienen que ser `static`). Está en las fuentes.

---

## FreeRTOS, el más común

Si querés explorarlo, **FreeRTOS** es el RTOS abierto más usado en Cortex-M, con port oficial para el
LPC1769. Los conceptos que verías, todos apoyados en lo que ya sabés:

| Concepto de FreeRTOS | Lo que ya viste que lo explica |
|---|---|
| `xTaskCreate` y el stack por tarea | [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md) |
| `vTaskDelay` contra `delay_ms` | [17 - El superloop](./17-superloop-y-codigo-no-bloqueante.md) |
| El tick del planificador | [SysTick](../06_systick/) |
| `PendSV` y el cambio de contexto | [Interrupciones y NVIC](../07_interrupciones/) |
| Colas, semáforos y mutex | [secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md) |
| `configTOTAL_HEAP_SIZE` y los esquemas `heap_1`..`heap_5` | [11 - Asignación dinámica](./11-asignacion-dinamica.md) |

Esa última fila es linda: FreeRTOS trae **cinco** implementaciones distintas de asignador, y elegir
entre ellas es exactamente la discusión del capítulo 11 (`heap_1` no libera nunca, `heap_4` fusiona
bloques libres, `heap_5` maneja regiones separadas). Que un kernel de tiempo real dedique cinco
archivos a eso te dice cuánto importa el tema.

---

## Cierre del módulo

La progresión completa de cómo estructurar firmware, que es lo que cierra estos tres capítulos:

1. **Superloop + interrupciones:** el patrón base. La ISR hace lo mínimo, el bucle hace el trabajo.
2. **Código no bloqueante:** nunca esperar. Tareas cortitas, tiempo con `millis` y resta `unsigned`.
3. **Máquinas de estado:** modelar comportamiento con etapas, de forma clara, extensible y testeable.
4. **RTOS:** cuando hacen falta tareas reales con prioridades y no querés coordinar a mano.

Y con eso cierra el módulo 0 entero. Empezaste declarando un `int` y terminás sabiendo cómo se ordena
un programa que atiende cinco cosas a la vez sobre un solo CPU. Lo que sigue es el
[módulo 01](../01_arquitectura_y_acceso_a_registros/), donde ese lenguaje se aplica a lo único que
importa en un micro: **una dirección de memoria que es un registro de hardware**.

---

## Fuentes y para seguir leyendo

**Libros**

- Richard Barry, *Mastering the FreeRTOS Real Time Kernel: A Hands-On Tutorial Guide*. **PDF gratuito
  y oficial** en [freertos.org](https://www.freertos.org/Documentation/RTOS_book.html). Escrito por el
  autor del kernel; los primeros capítulos explican tareas, colas y prioridades mejor que cualquier
  tutorial suelto.
- Joseph Yiu, *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors*, 3.ª ed., Newnes, 2014.
  **El** libro del núcleo que tenés en la placa. Los capítulos de excepciones, `MSP`/`PSP` y `PendSV`
  son los que explican, a nivel hardware, cómo es posible un cambio de contexto.
- Elecia White, *Making Embedded Systems*, 2.ª ed., O'Reilly, 2024. El capítulo de gestión de tareas
  compara con honestidad superloop, planificador cooperativo y RTOS, sin venderte ninguno.
- Jean J. Labrosse, *µC/OS-III: The Real-Time Kernel*. El otro RTOS clásico, con un libro que explica
  el kernel línea por línea. Útil si querés **entender** un RTOS por dentro y no solo usarlo.
- Qing Li y Caroline Yao, *Real-Time Concepts for Embedded Systems*, CMP Books, 2003. La teoría
  (planificación, sincronización, inversión de prioridad) sin atarse a un kernel concreto.

**Artículos**

- C. L. Liu y James Layland, *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time
  Environment*, Journal of the ACM, vol. 20 n.º 1, 1973. El artículo fundacional: de acá salen el
  planificador **rate monotonic** y la prueba de si un conjunto de tareas periódicas es planificable.
- Lui Sha, Ragunathan Rajkumar y John Lehoczky, *Priority Inheritance Protocols: An Approach to
  Real-Time Synchronization*, IEEE Transactions on Computers, vol. 39 n.º 9, 1990. La solución formal
  a la inversión de prioridad.
- [Glenn Reeves: What really happened on Mars](http://www.cs.cmu.edu/afs/cs/user/raj/www/mars.html).
  El relato en primera persona del jefe del equipo de software de la Mars Pathfinder sobre la
  inversión de prioridad que reiniciaba la sonda. Corto y buenísimo.
- Adam Dunkels et al., *Protothreads: Simplifying Event-Driven Programming of Memory-Constrained
  Embedded Systems*, SenSys, 2006. La alternativa liviana que se menciona más arriba, con el código
  disponible.

**Del curso**

- [10 - Dónde vive cada variable](./10-donde-vive-cada-variable.md): el stack, los dos stack pointers
  y cómo medir cuánto usa cada tarea. Es el capítulo que hay que tener fresco antes de tocar un RTOS.
- [Interrupciones](../07_interrupciones/): el NVIC, las prioridades y las secciones críticas.
- [SysTick](../06_systick/): el tick sobre el que corre cualquier planificador.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [18 - Máquinas de estado](./18-maquinas-de-estado.md) ·
**Siguiente módulo:** [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/)
