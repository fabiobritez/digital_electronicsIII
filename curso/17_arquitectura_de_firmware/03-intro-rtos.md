# Cuando el superloop no alcanza: intro a RTOS

El superloop con tareas no bloqueantes y máquinas de estado resuelve la mayoría de los proyectos. Pero
tiene un límite. Esta página explica **cuándo** se queda corto y qué es la alternativa: un **RTOS**
(Real-Time Operating System). Es una intro conceptual (no vas a configurar FreeRTOS acá), para que
sepas que existe y cuándo conviene.

## Dónde se queda corto el superloop

Imaginá que una tarea del superloop, por una sola vez, **tarda** (procesa un buffer grande, hace una
cuenta pesada). Mientras está en eso, **ninguna otra tarea corre**: el LED deja de parpadear preciso,
la respuesta al botón se demora. El superloop reparte el CPU "por buena voluntad": cada tarea **tiene
que** ceder rápido. Si una se porta mal, arrastra a todas.

Problemas típicos cuando el proyecto crece:
- Una tarea ocasionalmente larga **desincroniza** a las demás.
- Necesitás que cierta tarea corra **sí o sí** cada 1 ms, pase lo que pase (control de un motor).
- Tenés tareas con **prioridades** muy distintas y el reparto manual se vuelve un rompecabezas.
- El código de coordinación (banderas, tiempos) crece más que la lógica útil.

## Qué es un RTOS

Un RTOS es una pequeña pieza de software que te deja escribir tu programa como **varias "tareas"
independientes**, cada una con su propio `while(1)`, **como si cada una tuviera su propio CPU**. El
RTOS se encarga de **repartir** el CPU real entre ellas, muchas veces por segundo.

```c
// Con RTOS, cada tarea es un bucle propio, escrito de forma simple y hasta "bloqueante":
void tarea_led(void *p) {
    while (1) {
        LED_toggle();
        vTaskDelay(500);   // "dormir" 500 ms: el RTOS le da el CPU a OTRA tarea mientras tanto
    }
}
void tarea_sensor(void *p) {
    while (1) {
        leer_sensor();
        vTaskDelay(100);
    }
}
```

La diferencia importante: ese `vTaskDelay(500)` **no bloquea el sistema** como el `delay_ms` del superloop. Le dice al
RTOS "no me necesitás por 500 ms, dale el CPU a otra tarea". El RTOS **cambia de tarea** (eso se llama
*context switch*) y vuelve a `tarea_led` cuando corresponde. Vos escribís cada tarea de forma simple y
secuencial; el RTOS crea la ilusión de paralelismo.

## El planificador (scheduler) y las prioridades

El corazón del RTOS es el **scheduler**: decide qué tarea corre en cada momento, normalmente por
**prioridad** (la tarea lista de mayor prioridad corre) y repartiendo el tiempo entre las de igual
prioridad. Una tarea de **control de motor** de alta prioridad va a interrumpir a una de **actualizar
display** de baja prioridad, automáticamente. Eso, a mano en un superloop, es difícil.

El cambio de tareas se apoya en el **SysTick** (módulo 6): el RTOS usa su interrupción periódica (el
*tick*, típicamente cada 1 ms) para decidir si toca cambiar de tarea.

## Lo que el RTOS te da (y lo que cuesta)

**Te da:**
- Tareas independientes, cada una escrita simple y secuencial.
- Prioridades reales y *timing* más garantizado.
- Herramientas para comunicar tareas con seguridad: **colas** (pasar datos), **semáforos** y
  **mutex** (sincronizar, proteger recursos compartidos: el problema de las
  [secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md), resuelto de forma
  ordenada).

**Cuesta:**
- **RAM y Flash:** cada tarea necesita su propio stack; el RTOS ocupa memoria (FreeRTOS es chico, pero
  no gratis). En un micro muy chico no entra.
- **Complejidad:** aparecen problemas nuevos (race conditions entre tareas, deadlocks, inversión de
  prioridad). Hay que entender sincronización.
- **Curva de aprendizaje.**

## ¿Cuándo conviene? (regla práctica)

| Usá superloop + FSM si… | Pensá en un RTOS si… |
|--------------------------|----------------------|
| pocas tareas, todas cortas | muchas tareas, algunas largas o complejas |
| los tiempos no son críticos al ms | necesitás *timing* garantizado y prioridades reales |
| el micro es chico (poca RAM) | tenés RAM de sobra (el LPC1769, con 64 KB, da para FreeRTOS) |
| querés simplicidad y control total | la coordinación manual ya es más código que la lógica |

> **Para la materia:** la enorme mayoría de los proyectos se hacen perfecto con **superloop + tareas
> no bloqueantes + máquinas de estado**. El RTOS es bueno conocerlo como horizonte, y el LPC1769 lo
> soporta de sobra, pero no lo necesitás para empezar. Aprendé primero a no bloquear; el RTOS tiene
> mucho más sentido una vez que entendés *por qué* no hay que bloquear.

## FreeRTOS, el más común

Si querés explorarlo, **FreeRTOS** es el RTOS gratuito y abierto más usado en Cortex-M, con port
oficial para el LPC1769. Conceptos que verías: `xTaskCreate` (crear tareas), `vTaskDelay` (dormir),
**colas** (`xQueueSend`/`xQueueReceive`), **semáforos** y **mutex**. Pero todo eso se apoya en lo que
ya sabés: interrupciones, SysTick, y la idea de no bloquear.

## Cierre del módulo

La progresión completa de cómo estructurar firmware:

1. **Superloop + interrupciones**: el patrón base.
2. **Código no bloqueante**: nunca esperar; tareas cortitas y tiempo con `millis`.
3. **Máquinas de estado**: modelar comportamiento con etapas, claro y extensible.
4. **RTOS**: cuando hacen falta tareas reales con prioridades, sin coordinar a mano.

Con esto cerrás el salto de "sé usar periféricos" a "sé diseñar firmware". Es la base sobre la que se
apoyan todos los proyectos integradores.

---

**Anterior:** [02 - Máquinas de estado](./02-maquinas-de-estado.md) ·
**Volver al** [índice del curso](../README.md)
