# Máquinas de estado

Cuando un comportamiento tiene **etapas** que se suceden según el tiempo o los eventos (un semáforo,
un menú, abrir/cerrar un portón, un protocolo de comunicación) modelarlo con banderas y `if` anidados
se vuelve un nido imposible de seguir. La herramienta correcta es la **máquina de estados finitos**
(FSM, *finite state machine*).

## La idea

Una máquina de estados describe el comportamiento como:

- un conjunto de **estados** (en qué situación está el sistema *ahora*),
- **transiciones** entre estados (qué evento o condición hace pasar de uno a otro),
- **acciones** asociadas (qué hacer al entrar a un estado o en cada estado).

En cada momento el sistema está en **exactamente un** estado. Eso solo ya ordena la cabeza: en vez de
"¿el LED está prendido y pasaron 3 segundos y el botón…?", pensás "estoy en VERDE; cuando pasen 5 s,
paso a AMARILLO".

## Ejemplo: un semáforo

Tres estados (VERDE → AMARILLO → ROJO → VERDE…), cada uno con su duración. El diagrama:

```
   ┌──────────────────────────────────────────────┐
   ▼                                               │
[VERDE] ──(5 s)──▶ [AMARILLO] ──(2 s)──▶ [ROJO] ──(5 s)──┘
```

En C, el patrón es un `enum` de estados + una variable de estado + un `switch`, todo dentro de una
tarea **no bloqueante** del superloop:

```c
typedef enum { VERDE, AMARILLO, ROJO } EstadoSemaforo;

void tarea_semaforo(void) {
    static EstadoSemaforo estado = VERDE;
    static uint32_t t_estado = 0;          // cuándo entramos al estado actual
    uint32_t ahora = millis;

    switch (estado) {
    case VERDE:
        luz(VERDE_ON);
        if (ahora - t_estado >= 5000) {     // 5 s en verde
            estado = AMARILLO; t_estado = ahora;
        }
        break;
    case AMARILLO:
        luz(AMARILLO_ON);
        if (ahora - t_estado >= 2000) {     // 2 s en amarillo
            estado = ROJO; t_estado = ahora;
        }
        break;
    case ROJO:
        luz(ROJO_ON);
        if (ahora - t_estado >= 5000) {     // 5 s en rojo
            estado = VERDE; t_estado = ahora;
        }
        break;
    }
}
```

Fijate las propiedades buenas:
- **No bloquea:** entra, mira en qué estado está y si toca cambiar, y sale. El superloop sigue con
  otras tareas (`tarea_boton()`, etc.).
- **Es clarísimo:** cada estado es un `case`; las transiciones (las condiciones de tiempo) están a la
  vista.
- **Es extensible:** agregar un estado "INTERMITENTE de noche" es un `case` más, no reescribir todo.

## Eventos, no solo tiempo

Las transiciones no tienen que ser por tiempo; pueden ser por **eventos** (un botón, un dato que
llegó, un sensor que cruzó un umbral). Ejemplo: un menú que avanza con un botón.

```c
typedef enum { INICIO, MENU, CONFIG } EstadoUI;

void tarea_ui(void) {
    static EstadoUI estado = INICIO;
    switch (estado) {
    case INICIO:
        mostrar("Bienvenido");
        if (boton_apretado())  estado = MENU;
        break;
    case MENU:
        mostrar("1.Medir 2.Config");
        if (boton_largo())     estado = CONFIG;
        if (boton_apretado())  hacer_medicion();
        break;
    case CONFIG:
        mostrar("Configuracion");
        if (boton_largo())     estado = MENU;
        break;
    }
}
```

## Acciones de entrada (entry actions)

A veces querés hacer algo **una sola vez al entrar** a un estado (ej. arrancar un timer, mandar un
mensaje). El patrón es comparar con el estado anterior o usar una bandera de "recién entré":

```c
if (estado != estado_prev) {
    // ENTRY: corre una vez al entrar al estado nuevo
    switch (estado) {
        case ROJO: encender_buzzer_breve(); break;
        default: break;
    }
    estado_prev = estado;
}
```

## Por qué esto importa tanto

Sin máquinas de estado, un comportamiento con etapas se escribe con un montón de banderas globales
(`ya_paso_verde`, `esperando_amarillo`, `boton_fue_apretado`…) y `if` anidados que se contradicen
entre sí. Es **la** fuente de bugs de los proyectos integradores: estados imposibles, transiciones que
faltan, comportamiento que depende del orden en que se revisan las banderas.

Con una FSM:
- El estado es **uno y explícito** (la variable `estado`), no disperso en diez banderas.
- Es fácil **dibujar** el diagrama y revisarlo con otro: estados (círculos) y transiciones (flechas).
- Se mapea casi 1 a 1 del diagrama al `switch`.

> Consejo práctico: **dibujá la máquina de estados antes de codificar.** Estados, flechas, qué
> dispara cada flecha. El código sale solo del dibujo, y los errores se ven en el papel.

## Varias máquinas a la vez

Un proyecto real tiene **varias** FSM corriendo en paralelo en el superloop: una para el semáforo, una
para la UI, una para la comunicación. Cada una es una tarea no bloqueante:

```c
while (1) {
    tarea_semaforo();
    tarea_ui();
    tarea_comunicacion();
}
```

Cuando son muchas, o algunas necesitan correr en tiempos muy distintos y garantizados, el superloop
empieza a quedar chico. Ese es el momento de mirar un **RTOS**, en la
[próxima página](./03-intro-rtos.md).

## Ejercicios
1. Modelá el semáforo de arriba **con botón de peatón**: un estado extra que adelanta el rojo.
2. Una FSM para un **antirrebote** de botón (estados: SUELTO, REBOTE, APRETADO) con tiempo.
3. Una FSM que parsee comandos por UART: estados ESPERANDO, LEYENDO_COMANDO, EJECUTANDO.
4. Dibujá primero el diagrama de cada una; después codificá. Compará el dibujo con el `switch`.

---

**Anterior:** [01 - Superloop y no bloqueante](./01-superloop-no-bloqueante.md) ·
**Siguiente:** [03 - Intro a RTOS](./03-intro-rtos.md)
