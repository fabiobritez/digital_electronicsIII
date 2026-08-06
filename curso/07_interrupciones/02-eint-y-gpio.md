# Interrupciones externas (EINT) y por GPIO

Queremos que un pin (un botón, una señal externa) **dispare una interrupción** en vez de andar
preguntando por polling. El LPC1769 ofrece **dos mecanismos distintos**, con bloques de hardware
distintos:

1. **EINT0–3:** cuatro entradas de interrupción **dedicadas**, cada una con su IRQ propia, en el
   bloque System Control.
2. **Interrupción por GPIO:** **cualquier** pin de los puertos **0 o 2** puede interrumpir por flanco,
   en el bloque GPIOINT. Todas comparten la IRQ de EINT3.

No son lo mismo y conviene no confundirlos: están en registros, bases y vectores diferentes.

## Mecanismo 1: EINT0–3 (entradas dedicadas)

Los EINT están en pines **fijos** (EINT0 = P2.10, EINT1 = P2.11, EINT2 = P2.12, EINT3 = P2.13) y se
configuran con tres registros del bloque System Control (`LPC_SC`):

| Registro | Dirección | Qué controla |
|----------|-----------|--------------|
| `EXTINT` | `0x400FC140` | Bandera: se pone en 1 cuando ocurrió el EINTx. Se **limpia escribiendo 1**. |
| `EXTMODE` | `0x400FC148` | 0 = sensible a **nivel**, 1 = sensible a **flanco** |
| `EXTPOLAR` | `0x400FC14C` | Por flanco: 0 = bajada, 1 = subida. Por nivel: 0 = nivel bajo, 1 = nivel alto. |

CMSIS: `LPC_SC->EXTINT`, `LPC_SC->EXTMODE`, `LPC_SC->EXTPOLAR` (ojo: el nombre del miembro es
`EXTPOLAR`, sin la "I" de "polarity").

Cada bit (0..3) corresponde a un EINT: el bit 0 es EINT0, el bit 1 es EINT1, etc.

### Lo que el datasheet aclara y conviene saber

El manual muestra el diagrama lógico de un EINT y de ahí salen un par de detalles que evitan bugs:

- **Hay un glitch filter** a la entrada (sincroniza con PCLK). Filtra pulsos parásitos muy cortos,
  pero **no** reemplaza al antirrebote de un botón mecánico (los rebotes duran ms, no ns).
- **En modo nivel, el flag se autolimpia solo cuando el pin vuelve a su estado inactivo.** Si el pin
  sigue activo y escribís 1 en `EXTINT`, el flag se vuelve a poner enseguida → reentrás en loop. En
  modo nivel **tenés que** atender la causa (que el pin se desactive), no solo limpiar.
- **Regla del manual, fácil de olvidar:** cada vez que cambiás el modo o la polaridad de un EINT
  (incluida la inicialización), hacelo **con esa IRQ deshabilitada en el NVIC** y después **limpiá el
  bit correspondiente en `EXTINT`** antes de habilitarla. Si no, un evento espurio del cambio de
  configuración queda colgado y "no se reconoce" el siguiente. Por eso en los ejemplos configuramos
  primero, limpiamos `EXTINT`, y recién al final habilitamos en el NVIC.

### Ejemplo: botón en EINT0 (P2.10) por flanco de bajada

```c
#include <LPC17xx.h>

volatile uint8_t flag = 0;

void EINT0_IRQHandler(void) {
    LPC_SC->EXTINT = (1u << 0);     // limpiar la bandera de EINT0 (escribir 1)
    flag = 1;                       // avisar al main
}

int main(void) {
    // 1) PINSEL: P2.10 como EINT0 (función 1)
    LPC_PINCON->PINSEL4 &= ~(0x3u << 20);
    LPC_PINCON->PINSEL4 |=  (0x1u << 20);

    // 2) Configurar el EINT: por flanco, de bajada
    LPC_SC->EXTMODE  |= (1u << 0);   // EINT0 sensible a flanco
    LPC_SC->EXTPOLAR &= ~(1u << 0);  // flanco de bajada
    LPC_SC->EXTINT    = (1u << 0);   // limpiar bandera tras configurar (¡obligatorio!)

    // 3) Habilitar en el NVIC
    NVIC_SetPriority(EINT0_IRQn, 5);
    NVIC_EnableIRQ(EINT0_IRQn);

    while (1) {
        if (flag) { flag = 0; /* atender el botón */ }
    }
}
```

> Driver CMSIS equivalente: en `lpc17xx_exti` tenés `EXTI_Config(&cfg)`, `EXTI_SetMode`,
> `EXTI_SetPolarity` y `EXTI_ClearEXTIFlag(EXTI_EINT0)`. Hacen exactamente lo de arriba con structs y
> enums (`EXTI_MODE_EDGE_SENSITIVE`, `EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE`). Internamente
> `EXTI_ClearEXTIFlag` hace `LPC_SC->EXTINT |= (1 << linea)`. Conocer el registro te deja entender qué
> hace el driver y debuggear cuando "no anda".

### EINT y wake-up desde Power-down

Los EINT tienen un superpoder: pueden **despertar al CPU desde Power-down** (sleep profundo), porque
su lógica de detección sigue alimentada por el "wakeup logic" aunque los relojes principales estén
apagados. Por eso un botón en EINT es la forma clásica de "apretá para despertar".

Dos cosas de borde:

- Para wakeup, el EINT se configura **sensible a nivel**: en el diagrama del manual, el detector de
  flanco está sincronizado con PCLK, que en Power-down está **parado**; un nivel mantenido llega
  igual a la lógica de wakeup. (El GPIO no tiene este problema: su detección de flanco es asíncrona,
  el manual lo remarca justamente para este caso.) El ejemplo de wakeup del propio manual usa nivel
  bajo en EINT0.
- **Post-wakeup hay que limpiar el bit de EXTINT.** Si despertaste con, digamos, EINT0 en nivel bajo
  y dejás `EXTINT[0]` en 1, **el próximo intento de entrar a Power-down falla**. El manual es
  explícito: "If EINT0 bit is left set to 1, subsequent attempts to invoke Power-down mode will fail".

## Mecanismo 2: interrupción por GPIO (puertos 0 y 2)

A veces el botón no cae en un pin EINT. Cualquier pin de los puertos **0** o **2** puede generar
interrupción **por flanco** (subida, bajada, o ambos). Vive en un bloque separado, `GPIOINT`, base
`0x40028080` (CMSIS: `LPC_GPIOINT`). Solo puertos 0 y 2: P1, P3, P4 **no** tienen esta capacidad.

| Registro | Tipo | Qué hace |
|----------|------|----------|
| `IO0IntEnR` / `IO2IntEnR` | R/W | Habilita interrupción por **flanco de subida** (rising), bit por pin |
| `IO0IntEnF` / `IO2IntEnF` | R/W | Habilita por **flanco de bajada** (falling) |
| `IO0IntStatR` / `IO2IntStatR` | RO | Estado: qué pin disparó por **subida** |
| `IO0IntStatF` / `IO2IntStatF` | RO | Estado: qué pin disparó por **bajada** |
| `IO0IntClr` / `IO2IntClr` | WO | Limpia la bandera (escribir 1 en el bit del pin) |
| `IntStatus` | RO | Resumen: bit 0 = hay int en puerto 0, bit 2 = hay int en puerto 2 |

(Sí, querés ambos flancos en un pin: poné su bit en `IntEnR` **y** en `IntEnF`.)

CMSIS: `LPC_GPIOINT->IO0IntEnF`, `LPC_GPIOINT->IO0IntStatF`, `LPC_GPIOINT->IO0IntClr`,
`LPC_GPIOINT->IntStatus`, y los `IO2*` equivalentes.

> **Detalle clave que confunde a todos:** *todas* las interrupciones por GPIO de los puertos 0 y 2
> comparten la **misma posición en el NVIC que EINT3** (`EINT3_IRQn`, IRQ 21). El handler se llama
> `EINT3_IRQHandler`, y adentro tenés que **demultiplexar**: averiguar si fue un EINT3 "de verdad" o
> qué pin de GPIO la generó, leyendo los registros de estado. El manual lo dice tal cual: "GPIO0 and
> GPIO2 interrupts share the same position in the NVIC with External Interrupt 3".

### Ejemplo: interrupción por flanco de bajada en P0.4

```c
#include <LPC17xx.h>

volatile uint8_t flag = 0;

void EINT3_IRQHandler(void) {          // ¡GPIO usa el vector de EINT3!
    if (LPC_GPIOINT->IO0IntStatF & (1u << 4)) {   // ¿fue P0.4 por bajada?
        LPC_GPIOINT->IO0IntClr = (1u << 4);       // limpiar ESE pin
        flag = 1;
    }
    // si compartís el vector con EINT3 real o con más pines, chequealos acá también
}

int main(void) {
    // P0.4 ya es GPIO por defecto. Habilitar int por flanco de bajada:
    LPC_GPIOINT->IO0IntEnF |= (1u << 4);
    NVIC_EnableIRQ(EINT3_IRQn);

    while (1) {
        if (flag) { flag = 0; /* atender */ }
    }
}
```

### Cómo demultiplexar bien el handler de EINT3

Como el vector es compartido, un handler robusto chequea **todas** las fuentes posibles, no asume que
fue una sola, y limpia exactamente la que atendió:

```c
void EINT3_IRQHandler(void) {
    // ¿Fue el EINT3 dedicado (P2.13)?
    if (LPC_SC->EXTINT & (1u << 3)) {
        LPC_SC->EXTINT = (1u << 3);     // limpiar EINT3
        // ... atender EINT3 ...
    }

    // ¿Fue GPIO del puerto 0?
    if (LPC_GPIOINT->IntStatus & (1u << 0)) {
        uint32_t r = LPC_GPIOINT->IO0IntStatR;   // pines que subieron
        uint32_t f = LPC_GPIOINT->IO0IntStatF;   // pines que bajaron
        // ... atender según r/f ...
        LPC_GPIOINT->IO0IntClr = r | f;          // limpiar solo lo atendido
    }

    // ¿Fue GPIO del puerto 2?
    if (LPC_GPIOINT->IntStatus & (1u << 2)) {
        uint32_t r = LPC_GPIOINT->IO2IntStatR;
        uint32_t f = LPC_GPIOINT->IO2IntStatF;
        // ...
        LPC_GPIOINT->IO2IntClr = r | f;
    }
}
```

`IntStatus` es el atajo para saber **rápido** si vale la pena mirar cada puerto. Leé los `IntStat`
antes de limpiar (limpiar primero te haría perder qué pin fue). Driver CMSIS equivalente:
`GPIO_GetIntStatus(port, pin, edge)` y `GPIO_ClearInt(port, mask)`, que el ejemplo de fábrica
`GPIO_Interrupt/gpio_int.c` usa así dentro de su `EINT3_IRQHandler`.

### GPIO y wake-up

Igual que los EINT, **los puertos 0 y 2 pueden despertar de Power-down**. Es la alternativa si tu pin
de wakeup no es un EINT. (El manual lo lista entre las aplicaciones del GPIO: "Bringing the part out
of Power-down mode".)

## EINT vs GPIO: cuándo usar cada uno

| | EINT0–3 | Interrupción por GPIO |
|--|---------|------------------------|
| Pines | Fijos (P2.10–P2.13) | Cualquiera de puertos 0 y 2 |
| Bloque | System Control (`LPC_SC`) | `LPC_GPIOINT` |
| IRQ | Una por cada EINT (18–21) | Todas comparten EINT3 (21) |
| Sensibilidad | Nivel o flanco | Solo flanco (subida/bajada/ambos) |
| Handler | Directo (`EINT0_IRQHandler`) | Hay que demultiplexar en `EINT3_IRQHandler` |
| Wake de Power-down | Sí (mejor en nivel) | Sí (puertos 0 y 2) |

Regla práctica: si tu pin coincide con un EINT, usá EINT (más simple, sin demux). Si no, GPIO. Y si
ya usás GPIO **y** EINT3, acordate de que el handler de EINT3 tiene que chequear ambas fuentes.

## El error #1 en la placa real: rebote del botón

Un botón mecánico "rebota": un solo apriete genera muchos flancos en pocos ms, y por lo tanto muchas
interrupciones. El glitch filter del EINT no alcanza (filtra ns, no ms). Soluciones:

- **Antirrebote por tiempo:** en el handler, ignorar nuevos flancos durante ~50 ms usando el contador
  de SysTick del módulo 6 (guardás el tick del último flanco aceptado y descartás los que llegan
  antes de los 50 ms).
- **Filtro de hardware:** capacitor + resistencia (o un Schmitt trigger).

No lo dejes pasar: en los parciales con botones por interrupción, el debounce es lo que distingue una
solución que "anda en la simulación" de una que anda en la placa.

## Errores típicos (lista de chequeo)

- **No limpiar el flag** (`EXTINT` o `IOxIntClr`) → reentrás en loop infinito.
- **Limpiar el flag en la última línea** del handler → la escritura puede no propagarse a tiempo y
  reentrás una vez. Limpiá temprano.
- **Olvidar limpiar `EXTINT` tras cambiar el modo** → primer evento perdido o espurio.
- **No demultiplexar EINT3** cuando usás GPIO interrupts → atendés la fuente equivocada o ninguna.
- **Modo nivel sin atender la causa** → el flag se repone y reentrás.
- **Handler con nombre mal escrito** → no se engancha en la tabla (ver página 01); tu ISR nunca corre.
- **Compartir variable con el `main` sin `volatile`** → el `main` no ve el cambio. Y si los dos
  escriben, ni `volatile` alcanza → [página 03](./03-secciones-criticas-y-atomicidad.md).

## Ejercicios

1. Contador que **incrementa** con EINT0 (P2.10) y se **resetea** con una interrupción por GPIO en
   P0.x. Demultiplexá bien en `EINT3_IRQHandler`.
2. Agregá **antirrebote** de 50 ms con SysTick al ejemplo de GPIO.
3. Dos botones en P0.x y P2.y, ambos por interrupción GPIO: en `EINT3_IRQHandler`, distinguí cuál fue
   usando `IntStatus` y los `IntStat`.
4. Configurá EINT0 en **modo nivel bajo** para despertar de Power-down; en el post-wakeup, limpiá
   `EXTINT[0]` y verificá que podés volver a entrar a Power-down.

---

**Anterior:** [01 - NVIC y vectores](./01-nvic-y-vectores.md) ·
**Siguiente:** [03 - Secciones críticas y atomicidad](./03-secciones-criticas-y-atomicidad.md)
