# Máquinas de estado

El [capítulo anterior](./17-superloop-y-codigo-no-bloqueante.md) dejó una regla: ninguna tarea
bloquea. Pero apenas el comportamiento tiene **etapas** que se suceden según el tiempo o los eventos
(un semáforo, un menú, abrir un portón, un protocolo de comunicación), cumplir esa regla con banderas
y `if` anidados se vuelve un nido imposible de seguir.

La herramienta correcta es la **máquina de estados finitos** (FSM, *finite state machine*). Y en C se
escribe con dos cosas que ya tenés: un [`enum`](./05-estructuras-y-enums.md#enumeraciones-enum) y un
[`switch`](./04-control-de-flujo.md#2-estructura-switch).

---

## La idea

Una máquina de estados describe el comportamiento como:

- un conjunto de **estados**: en qué situación está el sistema *ahora*,
- **transiciones**: qué evento o condición hace pasar de un estado a otro,
- **acciones**: qué hacer al entrar a un estado, mientras se está en él, o al salir.

En cada momento el sistema está en **exactamente un** estado. Eso solo ya ordena la cabeza: en vez de
"¿el LED está prendido y pasaron 3 segundos y el botón...?", pensás "estoy en VERDE; cuando pasen
5 s, paso a AMARILLO".

Y trae una garantía que las banderas no dan: si tenés tres banderas booleanas, el sistema tiene 2³ = 8
combinaciones posibles, y probablemente **cuatro de ellas no significan nada** ("esperando_amarillo y
ya_paso_verde a la vez"). Con un `enum` de tres estados hay tres, y ninguna es imposible. La mitad de
los bugs de un proyecto integrador viven en esos estados imposibles.

---

## Ejemplo: un semáforo

Tres estados (VERDE → AMARILLO → ROJO → VERDE...), cada uno con su duración:

```
   ┌─────────────────────────────────────────────────────────┐
   ▼                                                         │
[VERDE] ──(5 s)──▶ [AMARILLO] ──(2 s)──▶ [ROJO] ──(5 s)──────┘
```

En C, el patrón es un `enum` de estados, una variable de estado y un `switch`, todo dentro de una
tarea **no bloqueante** del superloop:

```c
typedef enum { VERDE, AMARILLO, ROJO } estado_semaforo_t;

void tarea_semaforo(void) {
    static estado_semaforo_t estado = VERDE;
    static uint32_t t_estado = 0;              // cuándo entramos al estado actual
    uint32_t ahora = g_millis;

    switch (estado) {
    case VERDE:
        luz(VERDE_ON);
        if (ahora - t_estado >= 5000) {        // 5 s en verde
            estado = AMARILLO; t_estado = ahora;
        }
        break;

    case AMARILLO:
        luz(AMARILLO_ON);
        if (ahora - t_estado >= 2000) {        // 2 s en amarillo
            estado = ROJO; t_estado = ahora;
        }
        break;

    case ROJO:
        luz(ROJO_ON);
        if (ahora - t_estado >= 5000) {        // 5 s en rojo
            estado = VERDE; t_estado = ahora;
        }
        break;
    }
}
```

Fijate las propiedades buenas:

- **No bloquea:** entra, mira en qué estado está y si toca cambiar, y sale. El superloop sigue con
  otras tareas.
- **Es clarísimo:** cada estado es un `case`; las transiciones están a la vista.
- **Es extensible:** agregar un estado "INTERMITENTE de noche" es un `case` más, no reescribir todo.
- **Se mapea 1 a 1 con el dibujo:** cada círculo del diagrama es un `case`, cada flecha es un `if`.

### El `default` que te salva, y el warning que te lo tapa

Ese `switch` está incompleto a propósito: le falta el `default`. Y acá hay una sutileza que ya
apareció en el [capítulo 04](./04-control-de-flujo.md#2-estructura-switch) y que en una FSM es
crítica.

Querés **las dos** cosas a la vez:

1. Un `default` en ejecución, porque si la RAM se corrompe (o un puntero salvaje pisa la variable) el
   estado puede tomar un valor que no existe, y sin `default` la tarea deja de hacer **nada** para
   siempre, en silencio.
2. Un aviso al compilar si agregás un estado al `enum` y te olvidás de manejarlo.

El problema es que agregar el `default` **desactiva** `-Wswitch`. La solución es pedir
`-Wswitch-enum`, que cuenta los casos faltantes aunque haya `default`:

```c
    default:
        // estado imposible: registrar y volver a un estado seguro
        log_error("estado de semaforo invalido");
        estado = ROJO;                  // el estado seguro de un semáforo es ROJO, no VERDE
        t_estado = ahora;
        break;
    }
```

```make
CFLAGS += -Wall -Wextra -Wswitch-enum
```

> **El estado seguro no siempre es el primero.** En un semáforo es ROJO; en un horno es "calefactor
> apagado"; en un motor es "parado". Elegilo pensando qué pasa si el sistema se queda ahí.

---

## Eventos, no solo tiempo

Las transiciones no tienen que ser por tiempo: pueden ser por **eventos** (un botón, un dato que
llegó, un sensor que cruzó un umbral).

```c
typedef enum { INICIO, MENU, CONFIG } estado_ui_t;

void tarea_ui(void) {
    static estado_ui_t estado = INICIO;
    switch (estado) {
    case INICIO:
        mostrar("Bienvenido");
        if (boton_apretado())  estado = MENU;
        break;
    case MENU:
        mostrar("1.Medir 2.Config");
        if (boton_largo())     estado = CONFIG;
        else if (boton_apretado()) hacer_medicion();
        break;
    case CONFIG:
        mostrar("Configuracion");
        if (boton_largo())     estado = MENU;
        break;
    }
}
```

> [!CAUTION]
> **Cuidado con encadenar transiciones sin querer.** Si en un mismo `case` evaluás dos condiciones y
> las dos pueden ser verdad, el orden en que las escribiste decide el comportamiento (por eso arriba
> hay un `else if` y no dos `if` sueltos). Peor todavía: si una función como `boton_apretado()`
> **consume** el evento (devuelve 1 una sola vez), llamarla dos veces en la misma vuelta se come la
> segunda. Regla: **una transición por vuelta y por máquina.**

---

## Acciones de entrada y de salida

A veces querés hacer algo **una sola vez al entrar** a un estado (arrancar un timer, mandar un
mensaje, prender un buzzer) o **al salir** (apagar lo que prendiste). Meterlo dentro del `case` no
alcanza: ese código corre en **cada vuelta** del superloop mientras estés en ese estado.

El patrón limpio es no asignar el estado a mano, sino pasar por una función de transición:

```c
static estado_semaforo_t estado = VERDE;
static uint32_t t_estado = 0;

static void al_salir(estado_semaforo_t s) {
    switch (s) {
    case AMARILLO: buzzer_off(); break;
    default:       break;
    }
}

static void al_entrar(estado_semaforo_t s) {
    switch (s) {
    case AMARILLO: buzzer_on();  break;     // suena solo mientras dura el amarillo
    case ROJO:     contar_ciclo(); break;   // se cuenta UNA vez por ciclo, no 5000
    default:       break;
    }
}

static void ir_a(estado_semaforo_t nuevo) {
    al_salir(estado);
    estado   = nuevo;
    t_estado = g_millis;                    // el timestamp se actualiza en un solo lugar
    al_entrar(nuevo);
}
```

Y en el `switch` principal, cada transición pasa a ser una línea:

```c
    case VERDE:
        luz(VERDE_ON);
        if (ahora - t_estado >= 5000) ir_a(AMARILLO);
        break;
```

Tres cosas se arreglan de golpe: la acción de entrada corre exactamente una vez, el `t_estado` no se
puede olvidar nunca (era el error clásico de la versión a mano), y si mañana querés registrar todas
las transiciones para depurar, lo hacés en **una** línea dentro de `ir_a()`.

> Todas esas funciones son `static`: son internas de este `.c` y no parte de ninguna API. Es la regla
> del [capítulo 14](./14-static-const-inline-y-bitfields.md#static-los-dos-patrones-que-vas-a-escribir).

---

## Moore y Mealy, sin misticismo

Vas a ver estos dos nombres en la bibliografía y en la teoría de la materia:

- **Moore:** la salida depende **solo del estado**. El semáforo de arriba es Moore: estando en VERDE,
  la luz verde está prendida, y punto.
- **Mealy:** la salida depende del estado **y del evento** que provoca la transición. Por ejemplo,
  "al pasar de MENU a CONFIG, además emitir un beep".

En la práctica no elegís uno: escribís las salidas estables en el cuerpo del `case` (Moore) y las
salidas puntuales en las acciones de entrada o en la transición (Mealy). Los dos conviven en la misma
máquina y nadie se ofende. Lo que sí conviene es **saber cuál estás usando en cada salida**, porque
una salida Moore es más fácil de razonar y de probar.

---

## FSM dirigida por tabla

Cuando la máquina crece, el `switch` gigante empieza a molestar. Con **punteros a función**
([capítulo 09](./09-punteros-avanzado.md#punteros-a-función-y-callbacks)) podés poner cada estado en
su propia función y guardar la máquina en una tabla:

```c
typedef enum { ST_INIT, ST_IDLE, ST_ACTIVO, N_ESTADOS } estado_t;
typedef estado_t (*func_estado_t)(void);      // cada estado devuelve el próximo

static estado_t en_init(void)   { /* ... */ return ST_IDLE; }
static estado_t en_idle(void)   { /* ... */ return boton() ? ST_ACTIVO : ST_IDLE; }
static estado_t en_activo(void) { /* ... */ return ST_IDLE; }

static const func_estado_t maquina[N_ESTADOS] = { en_init, en_idle, en_activo };

void tarea_maquina(void) {
    static estado_t estado = ST_INIT;
    if (estado < N_ESTADOS && maquina[estado] != NULL) {   // validar SIEMPRE el índice
        estado = maquina[estado]();
    }
}
```

Ese `N_ESTADOS` al final del `enum` es un truco que vale oro: como los valores arrancan en 0 y
avanzan de a uno, la última constante **es** la cantidad de estados, y se actualiza sola cuando
agregás uno. El arreglo es `static const`, así que la tabla se va a Flash y no gasta RAM
([capítulo 01](./01-declaraciones-y-tipos.md#2-especificador-de-almacenamiento)).

| Preferí el `switch` | Preferí la tabla |
|---|---|
| Hasta unos 5 o 6 estados | Muchos estados, o estados con lógica larga |
| Querés ver toda la máquina de un vistazo | Querés que cada estado sea una función testeable aparte |
| Es lo que se lee más fácil, y es la forma que se pide en el parcial | El despacho es un salto indirecto, sin cadena de comparaciones |

> **El precio de la tabla:** perdés el chequeo de `-Wswitch-enum`. Si agregás un estado al `enum` y te
> olvidás de agregar su función, el arreglo queda con un `NULL` (los inicializadores faltantes se
> ponen en cero) y la máquina no hace nada. Por eso el `!= NULL` del ejemplo no es opcional, y por eso
> conviene un `_Static_assert(sizeof maquina / sizeof maquina[0] == N_ESTADOS, "falta un estado");`
> ([capítulo 07](./07-preprocesador.md)).

---

## Varias máquinas a la vez

Un proyecto real tiene **varias** FSM corriendo en paralelo en el superloop: una para el semáforo,
una para la interfaz, una para la comunicación. Cada una es una tarea no bloqueante:

```c
while (1) {
    tarea_semaforo();
    tarea_ui();
    tarea_comunicacion();
}
```

No se estorban porque ninguna bloquea y cada una tiene su propio estado en una `static` local. Si
necesitan hablar entre ellas, que sea por una interfaz explícita (una función, una cola), no por
banderas globales compartidas: ahí es donde se vuelven a mezclar los estados.

Cuando una máquina empieza a tener estados que se repiten con variantes ("ROJO", "ROJO_CON_PEATON",
"ROJO_DE_NOCHE"...), la señal es que te falta un nivel: eso son las **máquinas jerárquicas**
(*statecharts*), donde un estado puede contener sub-estados y heredar sus transiciones. Es el tema
del libro de Samek que está en las fuentes, y es más de lo que necesitás para la materia, pero saber
que existe te ahorra reinventarlo mal.

---

## Por qué esto importa tanto

Sin máquinas de estado, un comportamiento con etapas se escribe con un montón de banderas globales
(`ya_paso_verde`, `esperando_amarillo`, `boton_fue_apretado`...) y `if` anidados que se contradicen
entre sí. Es **la** fuente de bugs de los proyectos integradores: estados imposibles, transiciones que
faltan, comportamiento que depende del orden en que se revisan las banderas.

Con una FSM:

- El estado es **uno y explícito** (la variable `estado`), no disperso en diez banderas.
- Es fácil **dibujar** el diagrama y revisarlo con otra persona: estados en círculos, transiciones en
  flechas.
- Se **prueba**: podés forzar un estado, mandarle un evento y verificar a dónde fue, sin hardware.
- Se **depura**: un solo `printf` dentro de `ir_a()` te da la traza completa de la ejecución.

> **Consejo práctico: dibujá la máquina antes de codificar.** Estados, flechas, qué dispara cada
> flecha. El código sale solo del dibujo, y los errores se ven en el papel, que es donde son gratis.
> Y hacé la tabla de transiciones completa (una fila por estado, una columna por evento): las celdas
> que quedan vacías son los casos que te ibas a olvidar.

---

## Ejercicios

1. Modelá el semáforo de arriba **con botón de peatón**: un estado extra que adelanta el rojo.
   Dibujalo primero.
2. Una FSM de **antirrebote** de botón (estados: SUELTO, REBOTE, APRETADO) con tiempo. Compará el
   resultado con lo que hace el [módulo 5](../05_gpio/03-debounce-y-filtrado-de-entradas.md).
3. Una FSM que parsee comandos por UART: ESPERANDO_INICIO, LEYENDO_COMANDO, LEYENDO_DATOS,
   VERIFICANDO_CRC. ¿Qué pasa si el mensaje se corta a la mitad? Agregale un estado de timeout.
4. Tomá cualquiera de las tres y escribila en las **dos** formas (`switch` y tabla). Compará el
   tamaño del binario con `arm-none-eabi-size` y decidí cuál preferís y por qué.

---

## Fuentes y para seguir leyendo

**Libros**

- Miro Samek, *Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded Systems*,
  2.ª ed., Newnes, 2008. **La** referencia sobre máquinas de estado en C embebido. Arranca justo con
  el `switch` de este capítulo, muestra sus límites y construye desde ahí las máquinas jerárquicas.
  Los primeros capítulos se leen perfecto con lo que ya sabés.
- Elecia White, *Making Embedded Systems*, 2.ª ed., O'Reilly, 2024. El capítulo de gestión de tareas
  cubre las FSM dentro del superloop, con el mismo enfoque práctico que este capítulo.
- Michael Barr y Anthony Massa, *Programming Embedded Systems in C and C++*, 2.ª ed., O'Reilly, 2006.
- Kernighan y Ritchie, *The C Programming Language*, 2.ª ed., §3.4. El `switch`, de la fuente.

**Artículos y referencias**

- David Harel, *Statecharts: A Visual Formalism for Complex Systems*, Science of Computer Programming,
  vol. 8, 1987. El artículo que inventó las máquinas jerárquicas. Es legible y explica **por qué**
  hace falta el anidamiento; vale la pena aunque no implementes statecharts.
- [UML State Machine (OMG)](https://www.omg.org/spec/UML/). La notación estándar para dibujar estos
  diagramas, por si querés que tu dibujo lo entienda cualquiera.
- [QP/C framework](https://www.state-machine.com/). La implementación libre del libro de Samek, con
  port para Cortex-M. Para mirar cómo se hace en serio.

**Del curso**

- [04 - Control de flujo](./04-control-de-flujo.md#2-estructura-switch): el `switch`, el
  *fall-through* y la pelea entre `default` y `-Wswitch`.
- [05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md#uso-en-sistemas-embebidos): el `enum`
  de estados y por qué es mejor que tres `#define`.
- [09 - Punteros a función](./09-punteros-avanzado.md#máquina-de-estados-dirigida-por-tabla): la
  versión con tabla, desde el lado de los punteros.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [17 - El superloop y el código no bloqueante](./17-superloop-y-codigo-no-bloqueante.md) ·
**Siguiente:** [19 - Cuando el superloop no alcanza: intro a RTOS](./19-intro-a-rtos.md)
