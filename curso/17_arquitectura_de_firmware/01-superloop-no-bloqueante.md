# El superloop y el código no bloqueante

## El patrón base: superloop + interrupciones

Casi todo firmware sin sistema operativo tiene esta forma:

```c
int main(void) {
    init_todo();              // configurar clock, pines, periféricos (una vez)
    while (1) {               // el "superloop": se repite para siempre
        tarea_a();
        tarea_b();
        tarea_c();
    }
}
```

El `while(1)` (el **superloop** o *bucle principal*) corre una y otra vez, llamando a cada tarea por
turno. Como el CPU es rapidísimo, si **cada tarea es cortita**, la vuelta entera dura microsegundos y
da la **ilusión** de que todo pasa al mismo tiempo. Lo urgente (un flanco, un byte que llega) no
espera al bucle: lo atienden las **interrupciones**.

```
        ┌─────────────── superloop ───────────────┐
        │  tarea_a → tarea_b → tarea_c → (repetir) │
        └──────────────────────────────────────────┘
              ▲                          ▲
        interrupción                interrupción   ← caen cuando hace falta, cortan el bucle,
        (timer, botón)              (UART, etc.)      atienden lo urgente y devuelven el control
```

Esto ya lo venías usando sin nombrarlo. La clave que falta es una regla:

> **Ninguna tarea puede "colgarse" esperando. Nunca bloquees el superloop.**

## El enemigo: `delay()` bloqueante

Mirá este "parpadear un LED y leer un botón":

```c
while (1) {
    LED_on();
    delay_ms(500);     // <-- el CPU se queda 500 ms acá, sin hacer NADA más
    LED_off();
    delay_ms(500);     // <-- otros 500 ms congelado
    if (boton()) { ... }   // el botón solo se revisa cada 1 segundo: se siente trabado
}
```

El `delay_ms` bloqueante **detiene todo** durante medio segundo. Si en ese rato el usuario aprieta el
botón, no te enterás hasta que el `delay` termina. Con un solo LED no se nota; con cinco tareas, el
sistema se siente lento y "tragado". **El `delay()` bloqueante no escala.**

## La solución: tiempo no bloqueante

En vez de "esperar 500 ms", preguntás "¿ya pasaron 500 ms?" y, si no, seguís de largo. Usás la base
de tiempo del [SysTick](../06_systick/) (el contador `millis` que incrementa en la interrupción):

```c
extern volatile uint32_t millis;   // lo incrementa el SysTick_Handler cada 1 ms

void tarea_led(void) {
    static uint32_t t_prev = 0;        // 'static': recuerda su valor entre llamadas
    if (millis - t_prev >= 500) {      // ¿pasaron 500 ms desde la última vez?
        t_prev = millis;
        LED_toggle();                  // sí: cambiar el LED y registrar el momento
    }
    // si no pasaron, NO espera: devuelve el control enseguida
}
```

Ahora `tarea_led` **no bloquea**: entra, mira el reloj, y sale en microsegundos. El superloop puede
llamar a otras tareas entre parpadeo y parpadeo:

```c
int main(void) {
    init_todo();
    while (1) {
        tarea_led();      // parpadea cada 500 ms, sin bloquear
        tarea_boton();    // se revisa el botón en CADA vuelta: responde al instante
        tarea_sensor();   // lee el ADC cada 100 ms, sin bloquear
        tarea_uart();     // procesa lo que llegó, sin bloquear
    }
}
```

Las cuatro "conviven". Ninguna espera; cada una hace su pedacito cuando le toca. Eso es un firmware
no bloqueante.

> El truco de `static uint32_t t_prev` es clave: una variable `static` dentro de una función
> **conserva su valor entre llamadas** (vive en `.data`/`.bss`, no en el stack). Así cada tarea
> "recuerda" cuándo actuó por última vez. (Repaso del concepto de `static`, sugerido en
> `_SUGERENCIAS.md` B1.)

## El patrón "tarea periódica"

Generalizando, casi toda tarea temporizada tiene esta forma:

```c
void tarea_periodica(void) {
    static uint32_t t_prev = 0;
    if (millis - t_prev >= PERIODO_MS) {
        t_prev = millis;
        // ... hacer el trabajo (cortito) ...
    }
}
```

Y la otra forma común es la **bandera + procesamiento**: una interrupción levanta una `volatile` flag,
y una tarea del superloop la atiende:

```c
volatile uint8_t hay_dato = 0;     // la ISR la pone en 1
void tarea_procesar(void) {
    if (hay_dato) {
        hay_dato = 0;
        procesar();                 // el trabajo pesado, fuera de la ISR
    }
}
```

(Cuidado con compartir datos con la ISR: ver [secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md).)

## Reparto de responsabilidades

Una arquitectura sana separa así:

| Va en… | Qué |
|--------|-----|
| **Interrupciones** | lo **urgente** y cortito: capturar un flanco, guardar un byte que llegó, contar el tiempo. Levantan banderas. |
| **Superloop** | el **trabajo**: procesar datos, decidir, actualizar salidas. Mira banderas y relojes, nunca bloquea. |

> Regla de oro (ya la vimos en interrupciones, ahora encaja en el todo): **la ISR hace lo mínimo, el
> superloop hace el trabajo.**

## Lo que viene

El superloop no bloqueante alcanza para la enorme mayoría de los proyectos de la materia. Pero cuando
una tarea tiene **muchos pasos y estados** (un semáforo, un menú, un protocolo de comunicación), el
`if (millis - t_prev...)` se queda corto y el código se enreda. Ahí entra la herramienta de la
[próxima página](./02-maquinas-de-estado.md): las **máquinas de estado**.

---

**Módulo:** [Arquitectura de firmware](./README.md) ·
**Siguiente:** [02 - Máquinas de estado](./02-maquinas-de-estado.md)
