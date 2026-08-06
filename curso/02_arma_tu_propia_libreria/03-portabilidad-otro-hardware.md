# Llevarla a otro hardware (abrir la mente)

Ya tenés tu librería `mygpio` para el LPC1769. La pregunta que abre la cabeza es:

> **¿Y si mañana uso otro micro? ¿Tengo que reescribir toda mi aplicación?**

La respuesta, si diseñás bien, es **no**. Y entender por qué es uno de los aprendizajes más valiosos
de la materia, porque te saca de la idea de que "el código sirve para un solo chip".

## Qué cambia y qué no cuando cambiás de chip

Pensemos en pasar el LED parpadeante del LPC1769 a, por ejemplo, un **STM32** o un **AVR**:

| Parte | ¿Cambia? | Por qué |
|-------|----------|---------|
| La idea "configurar pin como salida, ponerlo en alto/bajo" | **No** | Todo micro con GPIO hace esto |
| Las **direcciones** de los registros | **Sí** | Cada chip las pone en otro lado |
| Los **nombres** de los registros | **Sí** | En STM32 es `GPIOA->ODR`, en AVR es `PORTB` |
| Cómo se **configura la dirección** del pin | **Sí** | STM32 usa 2 bits por pin (`MODER`), AVR un registro `DDRx` |
| Tu `main.c` (la lógica de la aplicación) | **No, si está bien diseñado** | Solo llama a `mygpio_set(...)` |

La conclusión: si tu aplicación **nunca toca registros directamente** y solo usa tu API
(`mygpio_set`, `mygpio_clr`…), entonces para cambiar de chip solo reescribís **la implementación**
de la librería, no la aplicación. A esa capa que "esconde" el hardware se la llama **HAL** (Hardware
Abstraction Layer, capa de abstracción de hardware).

## El truco: separar la *interfaz* de la *implementación*

Tu `mygpio.h` define **qué** se puede hacer (la interfaz):

```c
void mygpio_dir(uint8_t puerto, uint8_t pin, MYGPIO_Dir dir);
void mygpio_set(uint8_t puerto, uint8_t pin);
void mygpio_clr(uint8_t puerto, uint8_t pin);
```

El `.c` define **cómo** se hace (la implementación, atada al chip). La gracia es que podés tener
**varios `.c` para el mismo `.h`**, uno por plataforma:

```
mygpio.h            <- interfaz, igual para todos
mygpio_lpc1769.c    <- implementación para LPC1769  (LPC_GPIO0->FIODIR ...)
mygpio_stm32.c      <- implementación para STM32     (GPIOA->MODER ...)
mygpio_avr.c        <- implementación para AVR        (DDRB, PORTB ...)
```

Al compilar, elegís **un** `.c` según el target. Tu `main.c` ni se entera: siempre llama
`mygpio_set(0, 22)`. Eso es exactamente la filosofía detrás de Arduino (`digitalWrite()` funciona
igual en placas con chips totalmente distintos) y de los HAL de los fabricantes.

### Ejemplo: la misma función para tres chips

```c
/* ---- mygpio_lpc1769.c ---- */
void mygpio_set(uint8_t puerto, uint8_t pin) {
    puertos_lpc[puerto]->FIOSET = (1u << pin);
}

/* ---- mygpio_stm32.c ---- */
void mygpio_set(uint8_t puerto, uint8_t pin) {
    puertos_stm[puerto]->BSRR = (1u << pin);   /* en STM32, BSRR pone el pin en alto */
}

/* ---- mygpio_avr.c ---- */
void mygpio_set(uint8_t puerto, uint8_t pin) {
    *(port_avr[puerto]) |= (1u << pin);         /* en AVR, PORTx |= ... */
}
```

Misma firma, misma semántica ("poné el pin en alto"), implementación distinta. **La aplicación es
portable.**

## Decisiones de diseño que vale la pena pensar

Cuando diseñás tu propia librería, aparecen preguntas que no tienen una única respuesta correcta:
son ingeniería. Vale la pena discutirlas en clase:

- **¿API por número (`mygpio_set(0,22)`) o por "objeto pin"?**
  Algunos diseños prefieren `typedef struct { puerto, pin } Pin;` y pasás un `Pin`. Más expresivo,
  un poco más pesado.
- **¿Chequeo de errores o velocidad?** Nuestras funciones tienen `if (puerto > 4) return;`. Eso es
  seguro pero gasta ciclos. En un driver de alto rendimiento quizás lo sacás y confiás en el que
  llama. *Trade-off* clásico.
- **¿`inline` para los accesos chiquitos?** Una función que solo hace `FIOSET = ...` puede convenir
  marcarla `static inline` en el header, para que no haya costo de llamada. CMSIS hace mucho esto.
- **¿Configurás también el PINSEL y el power adentro, o lo dejás afuera?** Decisión de "cuánto
  esconde" la librería. CMSIS lo deja separado (vos llamás PINSEL aparte); un HAL más "amigable"
  podría hacerlo todo junto.

No hay una respuesta única: **eso es diseñar software**. Lo importante es que ahora **vos** tomás
esas decisiones, en lugar de aceptar las que tomó otro.

## La idea que te tenés que llevar

CMSIS, el HAL de STM32, el core de Arduino… todos son **la misma receta**: structs que mapean
registros + funciones que las usan + una interfaz estable que esconde el chip. No son cajas
cerradas. Son código que:

- podés **leer** (y deberías, cuando algo no anda),
- podés **modificar** (cuando el driver no hace lo que necesitás),
- y podés **reemplazar** por el tuyo (cuando querés control total o cambiás de hardware).

> En la materia vas a usar CMSIS porque es práctico y es el estándar. Pero ahora sabés que, si
> hiciera falta, **podrías escribir el tuyo**. Esa es la diferencia entre *usar* un micro y
> *dominarlo*.

## Ejercicios para abrir más la cabeza

1. **Extendé `mygpio`** con `mygpio_write_port(puerto, valor)` que escriba los 32 pines de una vez.
2. Agregá una versión `static inline` de `mygpio_set`/`mygpio_clr` en el header y compará el código
   ensamblador generado (en MCUXpresso, "Disassembly") con la versión como función normal.
3. **Diseñá la interfaz** (solo el `.h`, sin implementar) de una mini-librería de UART portable.
   ¿Qué funciones expondrías? ¿`uart_init(baudrate)`, `uart_send(byte)`, `uart_recv()`? Discutí qué
   detalles del chip esconderías y cuáles dejarías configurables.
4. Tomá el driver real [`lpc17xx_gpio.c`](../../library/CMSISv2p00_LPC17xx/src/) y compará tu
   `mygpio.c` con él. ¿Qué hace de más? ¿Qué simplificaste vos?

---

**Anterior:** [01 - Las tres capas](./01-las-tres-capas.md) ·
**Siguiente módulo:** [03 - Clock y Power](../03_clock_y_power/)
