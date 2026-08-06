# Construyendo mi propia librería de GPIO

Vamos a construir, paso a paso, una mini-librería de GPIO para el LPC1769. Al final vas a tener tres
archivos (`mygpio.h`, `mygpio.c`, `main.c`) que ya están listos en
[`src/`](./src/) para que los compiles en MCUXpresso. Acá explicamos **por qué** cada parte es como
es.

> Esto es, en chiquito, lo que hace CMSIS: una **capa de dispositivo** (las structs y direcciones) y
> una **capa de driver** (las funciones). Lo viste en la [página anterior](./01-las-tres-capas.md).

## Paso 1: El header `mygpio.h`, describir el hardware

### 1a. Los calificadores de acceso
Primero, los mismos `__IO`/`__O`/`__I` que usa CMSIS. Son solo `volatile` con un nombre que documenta
si el registro es de lectura/escritura, solo-escritura o solo-lectura:

```c
#define __IO  volatile          /* lectura / escritura */
#define __O   volatile          /* solo escritura      */
#define __I   volatile const    /* solo lectura        */
```

### 1b. La struct del periférico (capa de dispositivo)
Describimos un puerto GPIO como una `struct` que calca el bloque de registros en memoria. Los offsets
tienen que coincidir **exactamente** con el manual (Cap. 9); por eso el `RESERVED0[3]` que rellena
los huecos:

```c
typedef struct {
    __IO uint32_t FIODIR;        /* +0x00 */
         uint32_t RESERVED0[3];  /* +0x04..0x0C */
    __IO uint32_t FIOMASK;       /* +0x10 */
    __IO uint32_t FIOPIN;        /* +0x14 */
    __IO uint32_t FIOSET;        /* +0x18 */
    __O  uint32_t FIOCLR;        /* +0x1C */
} MYGPIO_Port;
```

> Si te saltás el `RESERVED0`, `FIOMASK` quedaría en el offset 0x04 en vez de 0x10, y **todo lo de
> abajo se corre**: escribirías en el registro equivocado. La struct es un molde, tiene que ser
> exacto.

### 1c. Las direcciones base de cada puerto
Cada puerto es la misma struct, apoyada en una dirección distinta (cada 0x20):

```c
#define MYGPIO0  ((MYGPIO_Port *) 0x2009C000UL)
#define MYGPIO1  ((MYGPIO_Port *) 0x2009C020UL)
#define MYGPIO2  ((MYGPIO_Port *) 0x2009C040UL)
#define MYGPIO3  ((MYGPIO_Port *) 0x2009C060UL)
#define MYGPIO4  ((MYGPIO_Port *) 0x2009C080UL)
```

### 1d. La interfaz del driver (capa de funciones)
Por último declaramos **qué** va a poder hacer la librería (sin implementar todavía):

```c
typedef enum { ENTRADA = 0, SALIDA = 1 } MYGPIO_Dir;

void    mygpio_dir(uint8_t puerto, uint8_t pin, MYGPIO_Dir dir);
void    mygpio_set(uint8_t puerto, uint8_t pin);
void    mygpio_clr(uint8_t puerto, uint8_t pin);
void    mygpio_toggle(uint8_t puerto, uint8_t pin);
uint8_t mygpio_read(uint8_t puerto, uint8_t pin);
```

Todo esto, con su `#ifndef MYGPIO_H` de guarda, está en
[`src/mygpio.h`](./src/mygpio.h).

## Paso 2: La implementación `mygpio.c`, hacer el trabajo

Cada función hace, por dentro, exactamente lo que harías a registro pelado, pero el que usa la
librería no tiene que saberlo. Empezamos con una tabla que traduce "número de puerto" a su struct:

```c
#include "mygpio.h"

static MYGPIO_Port * const puertos[5] = {
    MYGPIO0, MYGPIO1, MYGPIO2, MYGPIO3, MYGPIO4
};
```

Y las funciones, que son las máscaras de bits del módulo 1, encapsuladas:

```c
void mygpio_dir(uint8_t puerto, uint8_t pin, MYGPIO_Dir dir) {
    if (puerto > 4) return;
    if (dir == SALIDA) puertos[puerto]->FIODIR |=  (1u << pin);
    else               puertos[puerto]->FIODIR &= ~(1u << pin);
}

void mygpio_set(uint8_t puerto, uint8_t pin) {
    if (puerto > 4) return;
    puertos[puerto]->FIOSET = (1u << pin);   // SET: escribir 1 no afecta a los otros pines
}

void mygpio_clr(uint8_t puerto, uint8_t pin) {
    if (puerto > 4) return;
    puertos[puerto]->FIOCLR = (1u << pin);
}

void mygpio_toggle(uint8_t puerto, uint8_t pin) {
    if (puerto > 4) return;
    puertos[puerto]->FIOPIN ^= (1u << pin);   // leer-modificar-escribir (no atómico; ver módulo 5)
}

uint8_t mygpio_read(uint8_t puerto, uint8_t pin) {
    if (puerto > 4) return 0;
    return (puertos[puerto]->FIOPIN >> pin) & 1u;
}
```

Fijate cómo cada función es una sola línea de manipulación de registros (más una guarda). El archivo
completo está en [`src/mygpio.c`](./src/mygpio.c).

> **Decisiones que ya tomaste sin darte cuenta:** la tabla `puertos` es `static` (privada del `.c`,
> no se ve desde afuera); las funciones tienen `if (puerto > 4) return;` como guarda. Más sobre estas
> decisiones de diseño en la [página 3](./03-portabilidad-otro-hardware.md).

## Paso 3: Usarla en `main.c`

Y acá está el premio: el `main` **no conoce ni una dirección de registro**. Solo usa la interfaz:

```c
#include "mygpio.h"

#define LED_PUERTO 0
#define LED_PIN    22
#define BOTON_PUERTO 2
#define BOTON_PIN    10

static void delay(volatile uint32_t n) { while (n--) {} }

int main(void) {
    mygpio_dir(LED_PUERTO,   LED_PIN,   SALIDA);
    mygpio_dir(BOTON_PUERTO, BOTON_PIN, ENTRADA);

    while (1) {
        if (mygpio_read(BOTON_PUERTO, BOTON_PIN) == 0)
            mygpio_clr(LED_PUERTO, LED_PIN);
        else {
            mygpio_toggle(LED_PUERTO, LED_PIN);
            delay(1000000);
        }
    }
}
```

(También en [`src/main.c`](./src/main.c).)

## Cómo compilarlo en MCUXpresso
1. Creá un proyecto para el LPC1769 (con el startup/CMSIS-Core, que provee la tabla de vectores y
   `SystemInit`).
2. Agregá `mygpio.h`, `mygpio.c` y este `main.c` al proyecto.
3. Compilá y cargá. Deberías ver el LED parpadear y apagarse al apretar el botón.

> No necesitás `lpc17xx_gpio.c` de NXP: tu `mygpio` lo reemplaza. Eso es justamente el punto.

## Lo que acabás de demostrarte

Escribiste, vos, una librería con la **misma arquitectura que CMSIS**: structs que mapean registros +
funciones que las usan + una interfaz que esconde el hardware. Cuando en el resto de la materia uses
`GPIO_SetDir()` de NXP, ya sabés exactamente qué tiene adentro, porque construiste el equivalente.

En la [próxima página](./03-portabilidad-otro-hardware.md): cómo esta misma librería se lleva a
**otro microcontrolador** cambiando solo la implementación, sin tocar el `main`.

---

**Anterior:** [01 - Las tres capas](./01-las-tres-capas.md) ·
**Siguiente:** [03 - Llevarla a otro hardware](./03-portabilidad-otro-hardware.md)
