# Módulo 17: Arquitectura de firmware

Hasta acá aprendiste a manejar cada periférico. Pero un proyecto real usa **varios a la vez** (leer un
sensor, parpadear un LED, responder a botones, mandar datos por UART) y todos tienen que convivir sin
estorbarse. Acá está la pregunta que el resto del curso no respondió:

> **¿Cómo organizo el programa para que haga muchas cosas "al mismo tiempo" sin volverse un caos?**

Este es, probablemente, el salto más grande entre "hacer andar un ejemplo" y "hacer un producto". No
es sobre un periférico nuevo: es sobre **cómo se estructura el código**.

## Recorrido

1. [01 - El superloop y el código no bloqueante](./01-superloop-no-bloqueante.md)
   El patrón base de casi todo firmware, y por qué los `delay()` bloqueantes son el enemigo.
2. [02 - Máquinas de estado](./02-maquinas-de-estado.md)
   La herramienta para modelar comportamiento (un semáforo, un menú, un protocolo) de forma clara y
   sin `if` anidados infinitos.
3. [03 - Cuando el superloop no alcanza: intro a RTOS](./03-intro-rtos.md)
   Qué es un sistema operativo de tiempo real, qué es una "tarea", y cuándo conviene (y cuándo no).

## Cuándo leerlo
Después de tener varios periféricos vistos (al menos GPIO, SysTick/timers e interrupciones). Es el
módulo que **junta todo** y prepara para los proyectos integradores.

## La idea central
Un micro tiene **un solo** CPU: no hace dos cosas literalmente a la vez. La "simultaneidad" se logra
**no bloqueando nunca**: cada tarea hace un pedacito y devuelve el control, muy rápido, en un bucle.
Las interrupciones se ocupan de lo urgente. Cuando eso ya no alcanza, aparece el RTOS.

---

**Anterior:** [16 - Build, linker y startup](../16_build_linker_startup/) ·
**Volver al** [índice del curso](../README.md)
