Las definiciones de los Handlers de interrupciones se encuentran en el archivo `startup_LPC17xx.c` del proyecto.

## ¿Qué es un Handler de interrupción?

Un handler de interrupción es una función que se ejecuta cuando ocurre una interrupción.

Si observamos en el archivo `startup_LPC17xx.c` del proyecto, vemos que se define un handler para cada interrupción y tiene asignado un ALIAS.

## ¿Qué hace `ALIAS(IntDefaultHandler)`?

```c
#define ALIAS(f) __attribute__((weak, alias (#f)))
```

* `weak`: el símbolo es **débil**. Si en tu programa se define una función **con el mismo nombre**, esa definición (fuerte) **gana automáticamente**.
* `alias("IntDefaultHandler")`: el nombre declarado **no apunta a su propio código**, sino que **es un alias** de `IntDefaultHandler`.

Ejemplo:

```c
void TIMER0_IRQHandler(void) ALIAS(IntDefaultHandler);
```

Mientras no escribas en tu programa `TIMER0_IRQHandler`, el **vector** de TIMER0 apuntará al **mismo código** que `IntDefaultHandler` (normalmente un bucle infinito). Pero si lo escribes:

```c
void TIMER0_IRQHandler(void) { /* ...tu ISR... */ }
```

esa definición **reemplaza** a la débil y, sin tocar el startup, el NVIC saltará a tu ISR.

## ¿Dónde “vive” el vector y cómo se llena?

En el archivo `startup_LPC17xx.c` del proyecto, vemos que se define un array de punteros a función:
```c
extern void (* const g_pfnVectors[])(void);
__attribute__ ((used,section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    &_vStackTop,          // [0] SP inicial
    ResetISR,             // [1] Reset
    NMI_Handler,          // [2]
    HardFault_Handler,    // [3]
    ...
    SysTick_Handler,      // [15]

    // Periféricos LPC17xx (a partir de [16])
    WDT_IRQHandler,       // [16] 0x40
    TIMER0_IRQHandler,    // [17] 0x44
    TIMER1_IRQHandler,    // [18] 0x48
    ...
    I2C2_IRQHandler,      // [28] 0x70
    ...
};
```
Explicación:

* **Sección `.isr_vector`**: el *linker script* coloca esta sección al **comienzo del Flash**. Así, tras reset, el procesador la encuentra en la dirección base (el M3 usa el registro **VTOR** para saber dónde está).
* **Entrada \[0]**: no es función, es un **puntero al tope de pila** (`&_vStackTop` exportado por el *linker*).
* **Entradas \[1..15]**: excepciones del **núcleo** (Reset, NMI, HardFault, etc.).
* **Entradas \[16+]**: **IRQs de periféricos**. El comentario `0x40, 0x44, ...` son los **offsets** en bytes desde el inicio de la tabla (cada entrada son 4 bytes).
* **`__valid_user_code_checksum`**: para LPC17xx hay una **palabra de checksum** en un slot reservado (aquí en la posición que muestran) que la ROM de arranque verifica. Por eso ves ese símbolo “hueco” para que el *linker* lo coloque.
* **`__attribute__((used))`**: evita que el compilador/ligador elimine el array por optimizaciones.

En resumen: la **tabla de vectores** es un **array de punteros a función**. Cada nombre en tu `g_pfnVectors` debe existir. Por defecto, existen gracias a esas declaraciones `ALIAS(IntDefaultHandler)`.

# ¿Cómo decide el MCU a dónde saltar?

1. Un periférico (ej., TIMER0) **dispara** su IRQ.
2. El **NVIC** determina el **número de IRQ** y calcula el índice `16 + IRQn`.
3. Lee de la tabla la **dirección** almacenada en ese índice.
4. El Cortex-M3 apila contexto y **salta** a esa dirección.

Si no definiste el handler, ese nombre (p. ej. `TIMER0_IRQHandler`) es **un alias débil** que **apunta a `IntDefaultHandler`** → caerás en el “loop infinito” de default.

# ¿Qué tengo que hacer para que “vaya al mío”?

* Escribir la función **con el nombre exacto**:

  ```c
  void TIMER0_IRQHandler(void) {
      // 1) leer/chequear flags del periférico
      // 2) limpiar el flag que causó la IRQ
      // 3) hacer tu servicio
  }
  ```
* **Habilitar** la interrupción en el **NVIC** y en el **periférico**:

  ```c
  NVIC_EnableIRQ(TIMER0_IRQn);  // NVIC
  // ... configurar TIMER0 y habilitar sus interrupciones internas ...
  ```
* (Opcional) **Prioridad**:

  ```c
  NVIC_SetPriority(TIMER0_IRQn, 3); // 0 = más alta (depende de bits implementados)
  ```

Con eso, la entrada `TIMER0_IRQHandler` en la tabla **ya apuntaba a un símbolo**; solo que ahora ese símbolo **lo aportas tú** (fuerte), sustituyendo al alias débil.
 
## Buenas prácticas 

* **Nombre exacto** del handler = el que está en el vector (respetar mayúsculas/minúsculas).
 
* **No olvides limpiar el flag** del periférico dentro del ISR; si no, la IRQ se re-dispara.
* **No uses bloqueos largos** en el ISR (ni `printf` pesado). Mantén el handler **corto** y pasa trabajo al *thread* principal (p. ej., con flags/colas).
* **Weak vs fuerte**: solo puede haber **una** definición fuerte del mismo símbolo en todo el proyecto (o el *linker* dará error).

## Mini-checklist

1. Configurá el **periférico** (ej. TIMER0 match, enable de su interrupción interna).
2. `NVIC_SetPriority(TIMER0_IRQn, N);` (opcional)
3. `NVIC_EnableIRQ(TIMER0_IRQn);`
4. Implementá `void TIMER0_IRQHandler(void)` y **limpiá el flag** adentro.
5. Verificá que tu handler figura **una sola vez** (fuerte) y que el nombre coincide con el del vector.
