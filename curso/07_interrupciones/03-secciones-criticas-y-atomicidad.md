# Secciones críticas y atomicidad

Ya sabemos compartir datos entre una interrupción (ISR) y el `main` con `volatile`. Pero `volatile`
resuelve **un** problema (que el compilador no "cachee" la variable) y deja otro abierto, más sutil y
más peligroso: las **condiciones de carrera**. Esta página es la que evita los bugs intermitentes que
"a veces andan y a veces no".

## El problema: una operación que parece una, son varias

Mirá esta línea, con `contador` compartido entre el `main` y una ISR:

```c
volatile uint32_t contador;
contador = contador + 1;   // parece atómico... no lo es
```

El CPU no hace eso de un saque. Lo hace en **tres pasos** (leer-modificar-escribir):

```
1. LDR  r0, [contador]   ; leer
2. ADD  r0, r0, #1        ; sumar
3. STR  r0, [contador]   ; escribir
```

Si una **interrupción cae justo entre el paso 1 y el 3**, y la ISR también toca `contador`, se pierde
una cuenta:

```
main: lee contador = 5
        --- INTERRUPCIÓN: la ISR hace contador = 10 ---
main: suma 1 sobre el 5 que tenía -> escribe 6   (¡se perdió el 10!)
```

`volatile` **no** evita esto: garantiza que cada acceso va a memoria, pero no que los tres pasos
ocurran sin interrupción en el medio. A eso se le llama una **operación no atómica**.

## Qué es atómico en Cortex-M3

- Leer o escribir **una sola vez** un dato de hasta 32 bits **alineado** (un `uint32_t`, un puntero)
  es atómico: ocurre en una instrucción, no se puede partir.
- **Leer-modificar-escribir** (`++`, `+=`, `|=`, `&=`) **no** es atómico: son varias instrucciones.
- Tocar una variable de **más de 32 bits** (un `uint64_t`, una `struct`, un `double`) **no** es
  atómico: son varias escrituras, y una interrupción puede colarse en el medio dejando el dato a
  medio actualizar (lo ves "roto": mitad viejo, mitad nuevo).

> Regla rápida: si tu variable compartida es un `uint32_t` y **solo la ISR la escribe** y **solo el
> main la lee** (o viceversa), `volatile` alcanza. En cuanto **los dos escriben**, o el dato es más
> grande que 32 bits, necesitás una **sección crítica**.

## La solución: secciones críticas

Una **sección crítica** es un tramo de código donde **deshabilitás las interrupciones** para que nadie
te interrumpa en el medio de una operación de varios pasos. CMSIS da dos funciones del núcleo:

```c
__disable_irq();   // apagar todas las interrupciones
// ... operación que tiene que ser indivisible ...
__enable_irq();    // volver a habilitarlas
```

Ejemplo, protegiendo el incremento compartido:

```c
__disable_irq();
contador = contador + 1;   // ahora ninguna ISR se mete en el medio
__enable_irq();
```

Y para leer un dato grande compartido sin que se actualice a medias:

```c
uint64_t copia;
__disable_irq();
copia = timestamp_64bits;   // copia coherente, la ISR no puede partir la lectura
__enable_irq();
// trabajar con 'copia', no con la variable compartida
```

### Reglas de oro de las secciones críticas

1. **Lo más cortas posible.** Mientras las interrupciones están apagadas, **todo** lo demás espera:
   un timer pierde precisión, un byte de UART se puede perder. Apagá, hacé lo mínimo, encendé.
2. **Cuidado con anidarlas.** Si una función con sección crítica llama a otra que también
   `__enable_irq()`, podés re-habilitar antes de tiempo. Para código que puede anidarse, se guarda y
   restaura el estado con `__get_PRIMASK()` / `__set_PRIMASK()`:
   ```c
   uint32_t primask = __get_PRIMASK();   // guardar si ya estaban apagadas
   __disable_irq();
   // ... sección crítica ...
   __set_PRIMASK(primask);               // restaurar el estado anterior
   ```
3. **No es lo mismo que prioridades.** Deshabilitar todo es un martillo. A veces alcanza con subir la
   prioridad de la ISR conflictiva o rediseñar para no compartir (ver abajo).

## El mejor truco: no compartir estado mutable

Muchas condiciones de carrera desaparecen con un buen diseño, sin secciones críticas:

- **Una sola dirección de flujo.** Que la ISR **solo escriba** y el main **solo lea** una bandera
  `volatile`. El patrón que ya usamos (la ISR levanta `flag`, el main la baja y procesa) es seguro
  para un `uint8_t`/`uint32_t` porque cada lado hace un único acceso atómico.
- **Buffers circulares (ring buffer).** Para pasar un flujo de datos de la ISR al main (ej. bytes de
  UART), un buffer circular con un índice de escritura (solo lo toca la ISR) y uno de lectura (solo
  el main) evita casi toda sección crítica. Es el patrón estándar para recibir por interrupción.
- **Doble buffer.** La ISR llena el buffer A mientras el main procesa el B; se intercambian. Común en
  ADC+DMA.

> Moraleja: la sección crítica es la herramienta cuando **tenés** que compartir; pero la mejor
> defensa es **diseñar para no compartir estado mutable** entre la ISR y el main.

## Síntomas de que te falta esto

Si tu programa "anda casi siempre pero a veces hace algo raro" (un contador que se saltea, un valor
que aparece corrupto una vez cada mil, un cuelgue esporádico imposible de reproducir), sospechá de una
condición de carrera entre una ISR y el main. Son los bugs más difíciles de encontrar justamente
porque dependen del *timing*. Revisá: ¿qué variables comparto? ¿son de más de 32 bits? ¿las escriben
los dos lados? ¿hago `+=`/`|=` sobre una compartida?

## Ejercicios
1. Hacé un contador que la ISR de un timer incrementa y un botón (otra ISR) resetea. Provocá la
   condición de carrera y después arreglala con una sección crítica.
2. Implementá un **ring buffer** para recibir bytes de UART por interrupción sin secciones críticas.
3. Compartí un `uint64_t` (timestamp) entre una ISR y el main; mostrá cómo se "rompe" la lectura sin
   protección y cómo la sección crítica lo arregla.

---

**Anterior:** [02 - EINT y GPIO](./02-eint-y-gpio.md) ·
**Módulo:** [Interrupciones](./README.md) · **Siguiente módulo:** [08 - Timers](../08_timers/)
