# El superloop y el código no bloqueante

Hasta acá el módulo te dio **el lenguaje**: tipos, punteros, dónde vive cada variable, `volatile`,
structs de hardware. Los tres capítulos que cierran el módulo responden la pregunta que sigue, y que
no es sobre C sino sobre **cómo se ordena un programa entero**:

> **¿Cómo hago que el micro atienda muchas cosas "al mismo tiempo" sin que se vuelva un caos?**

Es, probablemente, el salto más grande entre "hacer andar un ejemplo" y "hacer un producto". Y es el
lugar donde todo lo del módulo se junta: `static` para el estado que sobrevive entre llamadas,
`volatile` para lo que comparte con una interrupción, `enum` + `switch` para modelar comportamiento,
punteros a función para las tablas de tareas.

> [!NOTE]
> **Cuándo leer estos tres capítulos.** Se entienden mejor si ya viste **GPIO** (módulo 5),
> **SysTick** (módulo 6) e **interrupciones** (módulo 7): los ejemplos usan un contador de
> milisegundos y hablan de ISRs. Si venís derecho desde el capítulo 16, leelos igual para tener el
> mapa, y volvé después de los periféricos: van a cerrar del todo.

---

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
        ┌─────────────── superloop ────────────────┐
        │  tarea_a → tarea_b → tarea_c → (repetir) │
        └──────────────────────────────────────────┘
              ▲                          ▲
        interrupción                interrupción   ← caen cuando hace falta, cortan el bucle,
        (timer, botón)              (UART, etc.)      atienden lo urgente y devuelven el control
```

Fijate que `main()` **nunca retorna**: en un micro no hay sistema operativo al que volver. Es la
misma observación del [capítulo 02](./02-arreglos-conversiones-y-promociones.md#pasar-arreglos-a-funciones),
ahora con su razón arquitectónica.

Esto ya lo venías usando sin nombrarlo. La clave que falta es una regla, y es toda la clase:

> **Ninguna tarea puede colgarse esperando. Nunca bloquees el superloop.**

---

## El enemigo: `delay()` bloqueante

Mirá este "parpadear un LED y leer un botón":

```c
while (1) {
    LED_on();
    delay_ms(500);         // <-- el CPU se queda 500 ms acá, sin hacer NADA más
    LED_off();
    delay_ms(500);         // <-- otros 500 ms congelado
    if (boton()) { ... }   // el botón solo se revisa cada 1 segundo: se siente trabado
}
```

El `delay_ms` bloqueante **detiene todo** durante medio segundo. Si en ese rato el usuario aprieta el
botón, no te enterás hasta que el `delay` termina. Con un solo LED no se nota; con cinco tareas, el
sistema se siente lento y tragado.

Y no es solo cuestión de comodidad. Poné números:

| Con `delay_ms` de 500 ms | Consecuencia medible |
|---|---|
| El botón se muestrea 1 vez por segundo | Una pulsación de 200 ms **se pierde entera** |
| La UART recibe sin que nadie lea | El buffer de recepción se desborda y perdés bytes |
| Un sensor que hay que leer cada 100 ms | Se lee cada 1000 ms: los datos son basura |

**El `delay()` bloqueante no escala.** Y ojo con la versión casera, que además de bloquear es
mentirosa:

```c
void delay_malo(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) { }   // "delay por conteo"
}
```

Cuánto tarde eso depende del nivel de optimización, de la frecuencia del clock y de si la Flash está
acelerada. Cambiás `-O0` por `-O2` y el tiempo cambia. Si además te olvidás el `volatile`, el
compilador borra el lazo entero y el delay dura **cero**. Para esperas cortas y precisas hay timers;
para todo lo demás, lo que sigue.

---

## La solución: tiempo no bloqueante

En vez de "esperar 500 ms", preguntás "¿ya pasaron 500 ms?" y, si no, seguís de largo. La base de
tiempo la da el [SysTick](../06_systick/): una interrupción cada 1 ms que incrementa un contador.

```c
volatile uint32_t g_millis = 0;          // la incrementa la ISR: por eso es volatile

void SysTick_Handler(void) {
    g_millis++;
}

void tarea_led(void) {
    static uint32_t t_prev = 0;          // 'static': recuerda su valor entre llamadas
    if (g_millis - t_prev >= 500) {      // ¿pasaron 500 ms desde la última vez?
        t_prev = g_millis;
        LED_toggle();                    // sí: cambiar el LED y anotar el momento
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

Las cuatro conviven. Ninguna espera; cada una hace su pedacito cuando le toca. Eso es un firmware no
bloqueante.

### Tres detalles de C que hacen que esto funcione

Esas nueve líneas apoyan en tres cosas del módulo, y vale la pena verlas explícitas:

**1. `static` es lo que le da memoria a la tarea.** `t_prev` vive en `.bss` durante todo el programa,
pero solo esta función la puede tocar ([capítulo 14](./14-static-const-inline-y-bitfields.md#static-los-dos-patrones-que-vas-a-escribir)).
Sin `static` sería una local que muere en cada llamada y la tarea no recordaría nada. Con una global
funcionaría, pero cualquiera podría pisarla.

**2. `volatile` en `g_millis` no es opcional.** La modifica una ISR, así que sin `volatile` el
compilador puede leerla una sola vez y cachearla en un registro: el `if` nunca se cumple y el LED no
parpadea nunca. Y es el caso traicionero de siempre: **con `-O0` anda y con `-O2` se rompe**
([capítulo 12](./12-volatile-y-tipos-para-hardware.md)).

**3. La resta está escrita así a propósito.** `g_millis - t_prev >= 500` y **no**
`g_millis >= t_prev + 500`. La primera sobrevive al desbordamiento del contador; la segunda no.

```c
// A los 49,7 días, g_millis pasa de 0xFFFFFFFF a 0x00000000.
// Supongamos t_prev = 0xFFFFFF00 y g_millis = 0x00000100 (pasaron 512 ms de verdad).

g_millis - t_prev            // 0x00000100 - 0xFFFFFF00 = 512   CORRECTO
g_millis >= t_prev + 500     // 0x100 >= 0xFFFFF0F4  ->  falso: la tarea se cuelga 49 días
```

El desbordamiento de un `unsigned` está **definido** como aritmética módulo 2³², y por eso la resta da
el intervalo real aunque el contador haya dado la vuelta en el medio. Es exactamente el motivo por el
que el [capítulo 02](./02-arreglos-conversiones-y-promociones.md#overflow-unsigned-da-la-vuelta-signed-es-comportamiento-indefinido)
insiste en usar `unsigned` para contadores que dan la vuelta. Con `int32_t` esto sería
**comportamiento indefinido** y el compilador podría hacer cualquier cosa.

> **Consecuencia práctica:** el intervalo máximo que podés medir así es la mitad del rango, unos 24,8
> días con `uint32_t` y milisegundos. Más que suficiente. Con un contador `uint16_t` serían 32
> segundos, que sí se te puede quedar corto.

---

## Período contra fase: el error de un `+=`

Hay dos formas de anotar "cuándo actué por última vez", y **no son equivalentes**:

```c
t_prev = g_millis;        // (A) desde AHORA
t_prev += PERIODO_MS;     // (B) desde cuando TENDRÍA que haber actuado
```

La (A) es la que viste arriba. Es robusta: si una vuelta del superloop se atrasa, la tarea
simplemente corre un poco después y sigue. Pero **acumula deriva**: cada retraso se suma al
siguiente, así que un LED que parpadea a 500 ms termina parpadeando a 503 ms de promedio.

La (B) no deriva: la fase queda anclada a la grilla original, y un atraso ocasional se compensa
solo en la vuelta siguiente. Su riesgo es el opuesto: si la tarea se atrasa **más de un período
entero**, `t_prev` queda en el pasado y la tarea se dispara varias veces seguidas para "ponerse al
día", cosa que a veces querés y a veces es un desastre.

| Usá `t_prev = ahora` (A) | Usá `t_prev += PERIODO` (B) |
|---|---|
| Parpadeos, refresco de display, sondeos | Muestreo a frecuencia fija, generación de señal, relojes |
| No importa un par de ms de deriva | Importa que N muestras cubran exactamente N períodos |
| Preferila por defecto: es más difícil de romper | Solo si además protegés el caso "me atrasé mucho" |

Para el caso de un reloj, que es donde la deriva se ve, la (B) con protección se escribe así:

```c
void tarea_reloj(void) {
    static uint32_t t_prev = 0;
    while (g_millis - t_prev >= 1000) {   // 'while', no 'if': recupera atrasos
        t_prev += 1000;                   // avanza la grilla, no la reengancha a "ahora"
        segundos++;
    }
}
```

---

## Los dos patrones que vas a escribir siempre

**Patrón 1: la tarea periódica.**

```c
void tarea_periodica(void) {
    static uint32_t t_prev = 0;
    if (g_millis - t_prev >= PERIODO_MS) {
        t_prev = g_millis;
        // ... hacer el trabajo (cortito) ...
    }
}
```

**Patrón 2: bandera + procesamiento.** Una interrupción levanta una bandera y una tarea del superloop
hace el trabajo pesado afuera de la ISR:

```c
volatile uint8_t hay_dato = 0;      // la ISR la pone en 1

void UART0_IRQHandler(void) {
    buffer[i++] = leer_byte();      // lo mínimo indispensable
    hay_dato = 1;
}

void tarea_procesar(void) {
    if (hay_dato) {
        hay_dato = 0;
        procesar(buffer);           // el trabajo pesado, ya fuera de la ISR
    }
}
```

> [!WARNING]
> **Ese `hay_dato = 0` tiene una carrera escondida.** Entre el `if` que la lee y la asignación que la
> baja, puede entrar la ISR y ponerla en 1 de nuevo: esa segunda notificación se pierde. Con una
> bandera de "hay algo que mirar" no suele importar; con un **contador** de eventos
> (`eventos++` en la ISR, `eventos--` en la tarea) sí importa, porque `eventos--` no es atómico.
> `volatile` resuelve la **visibilidad**, nunca la **atomicidad**: eso son las
> [secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md), y el
> [capítulo 12](./12-volatile-y-tipos-para-hardware.md#qué-garantiza-volatile-y-qué-no) lo dice con
> todas las letras.

---

## Reparto de responsabilidades

Una arquitectura sana separa así:

| Va en… | Qué | Cuánto puede durar |
|--------|-----|---|
| **Interrupciones** | lo **urgente** y cortito: capturar un flanco, guardar un byte que llegó, contar el tiempo. Levantan banderas. | microsegundos |
| **Superloop** | el **trabajo**: procesar datos, decidir, actualizar salidas. Mira banderas y relojes, nunca bloquea. | lo que haga falta, en pedacitos |

> **Regla de oro** (ya apareció en interrupciones, ahora encaja en el todo): **la ISR hace lo mínimo,
> el superloop hace el trabajo.**

Lo que **nunca** va en una ISR: `printf` (lento, y encima no es reentrante), `malloc`
([capítulo 11](./11-asignacion-dinamica.md)), esperas activas, cuentas con `float`
([capítulo 15](./15-punto-fijo-vs-flotante.md): sin FPU, cada operación es una llamada a software).

---

## Cuánto tarda tu vuelta, y por qué te tiene que importar

El superloop tiene un número que lo define: el **tiempo de la vuelta más larga**. Ese número es la
peor demora con la que tu programa puede reaccionar a algo que no sea una interrupción. Si tu vuelta
tarda 20 ms en el peor caso, un botón puede tardar 20 ms en responder (imperceptible) pero un
protocolo que exige contestar en 5 ms **no se cumple nunca**.

Medirlo es fácil y vale mucho más que estimarlo:

```c
// Prendé un pin al entrar a la vuelta y apagalo al salir: el osciloscopio te da el tiempo exacto,
// el duty y, sobre todo, el peor caso (con persistencia o disparo por ancho de pulso).
while (1) {
    LPC_GPIO0->FIOSET = (1u << 22);
    tarea_a(); tarea_b(); tarea_c();
    LPC_GPIO0->FIOCLR = (1u << 22);
}
```

Es la técnica más barata de instrumentación que existe en firmware, cuesta dos instrucciones y no
depende de ningún debugger. Los instrumentos están en el
[módulo 17 - Hardware y placa](../17_hardware_y_placa/03-instrumentos-de-medicion.md).

Si una tarea puntual es larga y no la podés partir, la salida es **partirla vos**: convertirla en una
máquina de estados que haga un pedacito por vuelta. Ese es justo el tema del
[capítulo siguiente](./18-maquinas-de-estado.md).

---

## Subir un escalón: la tabla de tareas

Cuando las tareas son muchas, llamarlas a mano en el `while(1)` se vuelve incómodo: los períodos
quedan desparramados dentro de cada función y no hay un lugar donde ver el sistema entero. Con lo que
ya sabés de **punteros a función** ([capítulo 09](./09-punteros-avanzado.md#punteros-a-función-y-callbacks))
podés escribir un planificador cooperativo de quince líneas:

```c
typedef struct {
    void     (*func)(void);   // qué hacer
    uint32_t period_ms;       // cada cuánto
    uint32_t t_prev;          // cuándo fue la última vez
} tarea_t;

static tarea_t tareas[] = {
    { tarea_led,    500, 0 },
    { tarea_sensor, 100, 0 },
    { tarea_uart,     0, 0 },   // período 0 = en cada vuelta, lo más rápido posible
};
#define N_TAREAS  (sizeof tareas / sizeof tareas[0])

static void planificador(void) {
    for (size_t i = 0; i < N_TAREAS; i++) {
        if (g_millis - tareas[i].t_prev >= tareas[i].period_ms) {
            tareas[i].t_prev = g_millis;
            tareas[i].func();
        }
    }
}

int main(void) {
    init_todo();
    while (1) {
        planificador();
    }
}
```

Lo que ganás no es velocidad, es **poder ver el sistema**: los períodos de todas las tareas están en
una tabla, agregar una es una línea, y el planificador es un solo lugar donde instrumentar,
cronometrar o depurar. La tabla es candidata natural a `static const` en la parte que no cambia, así
que se va a Flash ([capítulo 01](./01-declaraciones-y-tipos.md#2-especificador-de-almacenamiento)).

Sigue siendo **cooperativo**: si una tarea se cuelga, se cuelgan todas. Ese es exactamente el límite
que empuja hacia un RTOS, y es el tema del [capítulo 19](./19-intro-a-rtos.md).

---

## Resumen de reglas para no equivocarse

| Regla | Por qué |
| ----- | ------- |
| Ninguna tarea bloquea: entra, hace un pedacito y sale | Es la única forma de que N tareas convivan sobre un solo CPU |
| Nada de `delay_ms()` bloqueante en el superloop | Se come el tiempo de todas las demás tareas |
| El estado de cada tarea va en una `static` local, no en una global | Vive todo el programa pero solo esa función lo toca |
| `volatile` en toda variable que comparta con una ISR | Sin él, el compilador la cachea y con `-O2` el programa se rompe |
| Compará tiempos con `ahora - t_prev >= T`, nunca con `ahora >= t_prev + T` | La resta `unsigned` sobrevive al desbordamiento del contador |
| `volatile` da visibilidad, no atomicidad | Para leer-modificar-escribir compartido hacen falta secciones críticas |
| La ISR levanta banderas; el trabajo lo hace el superloop | Una ISR larga retrasa a todas las demás interrupciones |
| Nada de `printf`, `malloc` ni `float` dentro de una ISR | Lentos, no reentrantes, y en el M3 el `float` es software |
| Medí el tiempo de tu vuelta con un pin y el osciloscopio | Es tu peor latencia de respuesta, y adivinarla no sirve |

---

## Fuentes y para seguir leyendo

**Libros**

- Elecia White, *Making Embedded Systems*, 2.ª ed., O'Reilly, 2024. El libro más cercano a lo que
  hace este capítulo: cómo se estructura firmware de verdad, con un capítulo dedicado a la gestión de
  tareas y al superloop. Si vas a leer un solo libro de arquitectura de firmware, que sea este.
- Jack Ganssle, *The Art of Designing Embedded Systems*, 2.ª ed., Newnes, 2008. Clásico sobre
  presupuesto de tiempo, latencia de interrupciones y por qué medir en lugar de estimar.
- Michael Barr y Anthony Massa, *Programming Embedded Systems in C and C++*, 2.ª ed., O'Reilly, 2006.
  Cubre el camino completo desde el superloop hasta un RTOS mínimo, escribiéndolo.
- Philip Koopman, *Better Embedded System Software*, 2010. Capítulos cortos y concretos sobre lo que
  sale mal en firmware real; el de "Global Variables" y el de "Real Time" aplican directo acá.

**Sobre los temas puntuales**

- [Jack Ganssle: A Guide to Debouncing](http://www.ganssle.com/debouncing.htm). El estudio con
  osciloscopio de cuánto rebota un pulsador de verdad, y por qué el antirrebote se hace no bloqueante.
  Se aplica en el [módulo 5](../05_gpio/03-debounce-y-filtrado-de-entradas.md).
- [Arduino: BlinkWithoutDelay](https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/).
  El ejemplo que le enseñó el patrón a una generación entera. Es exactamente `millis() - t_prev >= T`.

**Del curso**

- [SysTick](../06_systick/): de dónde sale el contador de milisegundos que usa todo este capítulo.
- [Interrupciones](../07_interrupciones/): el NVIC, y las
  [secciones críticas](../07_interrupciones/03-secciones-criticas-y-atomicidad.md) para cuando la
  bandera compartida no alcanza.
- [Módulo 17 - Instrumentos de medición](../17_hardware_y_placa/03-instrumentos-de-medicion.md): cómo
  medir con el osciloscopio el tiempo de vuelta del superloop.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [16 - Redirigir `printf` a la UART](./16-redirigir-printf-a-uart.md) ·
**Siguiente:** [18 - Máquinas de estado](./18-maquinas-de-estado.md)
