# Arquitectura de firmware

Este trayecto aplica el lenguaje C a la organización temporal de un firmware completo: primero el
superloop, después las máquinas de estado y, por último, una introducción a los RTOS.

## Prerrequisitos

Antes de trabajarlo, conviene haber completado:

- [C6 - `struct`, `enum` y estado](../06-estructuras-y-enums.md);
- [C9 - callbacks y tablas](../09-punteros-avanzado.md);
- [C10 - memoria y stack](../10-donde-vive-cada-variable.md);
- [GPIO](../../05_gpio/), [SysTick](../../06_systick/) e
  [interrupciones](../../07_interrupciones/).

## Recorrido

1. [El superloop y el código no bloqueante](./17-superloop-y-codigo-no-bloqueante.md): tareas
   cooperativas, tiempos con wraparound, presupuesto de ejecución y eliminación de esperas activas.
2. [Máquinas de estado](./18-maquinas-de-estado.md): estado explícito, transiciones, acciones de
   entrada/salida y FSM dirigida por tabla.
3. [Cuando el superloop no alcanza](./19-intro-a-rtos.md): cambio de contexto, stacks por tarea,
   prioridades, sincronización, costos y criterios para introducir un RTOS.

---

**Módulo:** [Lenguaje C](../README.md) ·
**Aplicación previa recomendada:** [Interrupciones](../../07_interrupciones/)
