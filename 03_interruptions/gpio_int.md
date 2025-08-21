# Interrupciones de GPIO en Microcontroladores

## Documentación de Referencia
Para información detallada, consultar:
- Capítulo 9 y 3 del User Manual 

## Conceptos Fundamentales

### Tipos de Interrupciones GPIO

Las interrupciones de GPIO se clasifican en dos categorías principales:

1. **Interrupciones usando puertos estándar**: Cualquier pin de los puertos 0 y 2
2. **Interrupciones usando pines externos dedicados**: Pines específicos de interrupciones externas
 

## Interrupciones de GPIO0 y GPIO2

#### Características Operativas

- **Generación al microprocesador**: Los puertos 0 y 2 pueden generar interrupciones directamente
- **Configuración de flancos**: Cada pin puede programarse para interrumpir por:
  - Flanco ascendente
  - Flanco descendente  
  - Ambos flancos
- **Sincronización**: La detección de flanco es asíncrona, pero puede operar aunque el clock no esté presente (modo Power down)
- **Posición compartida en NVIC**: GPIO0 y GPIO2 comparten "External Interrupt 3", requiriendo verificación del pin causante en la ISR
- **Vista por software**: Algunos registros permiten consultar interrupciones pendientes

## Registros de Control

### Tabla 103: Mapa de Registros de Interrupciones GPIO

| Nombre | Descripción | Acceso | Reset | Dirección |
|--------|-------------|---------|--------|-----------|
| IntEnR | GPIO Interrupt Enable for Rising edge | R/W | 0 | IO0IntEnR: 0x4002 8090<br>IO2IntEnR: 0x4002 80B0 |
| IntEnF | GPIO Interrupt Enable for Falling edge | R/W | 0 | IO0IntEnF: 0x4002 8094<br>IO2IntEnF: 0x4002 80B4 |
| IntStatR | GPIO Interrupt Status for Rising edge | RO | 0 | IO0IntStatR: 0x4002 8084<br>IO2IntStatR: 0x4002 80A4 |
| IntStatF | GPIO Interrupt Status for Falling edge | RO | 0 | IO0IntStatF: 0x4002 8088<br>IO2IntStatF: 0x4002 80A8 |
| IntClr | GPIO Interrupt Clear | WO | 0 | IO0IntClr: 0x4002 808C<br>IO2IntClr: 0x4002 80AC |
| IntStatus | GPIO overall Interrupt Status | RO | 0 | IOIntStatus: 0x4002 8080 |

*Nota: El valor de reset refleja el estado almacenado solo en bits usados. No incluye contenido de bits reservados.*

Para más detalles sobre estos registros, consultar la sección 9.5.6 del User Manual.

### Descripción Detallada de Registros

#### IOIntStatus (0x4002 8080)
**Registro de solo lectura** que indica la presencia de interrupciones pendientes en todos los puertos que soportan interrupciones. Requiere solo un bit por cada puerto.

**Tabla 114: GPIO Overall Interrupt Status Register**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0Int | Port 0 GPIO interrupt pending<br>• 0: No hay interrupciones pendientes<br>• 1: Al menos una interrupción pendiente | 0 |
| 1 | - | Reservado (no definido) | NA |
| 2 | P2Int | Port 2 GPIO interrupt pending<br>• 0: No hay interrupciones pendientes<br>• 1: Al menos una interrupción pendiente | 0 |
| 31:2 | - | Reservado (no definido) | NA |

#### IO0IntEnR - IO2IntEnR (Habilitación por Flanco Ascendente)

Cada bit habilita la interrupción por flanco de subida para el pin correspondiente del puerto 0 o 2. Al colocar el bit en 1, se habilita la interrupción por flanco ascendente para el puerto y pin correspondiente.

**Tabla 115: GPIO Interrupt Enable for Rising Edge (IO0IntEnR - 0x4002 8090)**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0.0ER | Enable rising edge interrupt for P0.0<br>• 0: Deshabilitado<br>• 1: Habilitado | 0 |
| 1 | P0.1ER | Enable rising edge interrupt for P0.1 | 0 |
| 2 | P0.2ER | Enable rising edge interrupt for P0.2 | 0 |
| ... | ... | ... | ... |
| 15 | P0.15ER | Enable rising edge interrupt for P0.15 | 0 |
| 16 | P0.16ER | Enable rising edge interrupt for P0.16 | 0 |

#### IO0IntEnF - IO2IntEnF (Habilitación por Flanco Descendente)

Cada bit habilita la interrupción por flanco de bajada para el pin correspondiente del puerto 0 o 2.

**Tabla 117: GPIO Interrupt Enable for Falling Edge (IO0IntEnF - 0x4002 8094)**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0.0EF | Enable falling edge interrupt for P0.0<br>• 0: Deshabilitado<br>• 1: Habilitado | 0 |
| 1 | P0.1EF | Enable falling edge interrupt for P0.1 | 0 |
| ... | ... | ... | ... |
| 15 | P0.15EF | Enable falling edge interrupt for P0.15 | 0 |
| 16 | P0.16EF | Enable falling edge interrupt for P0.16 | 0 |

#### IO0IntStatR - IO2IntStatR (Estado por Flanco Ascendente)

Registro de **solo lectura** que indica si hay una interrupción por flanco ascendente pendiente. Si un bit está en 1, significa que se detectó un flanco ascendente en el pin correspondiente.

**Tabla 119: GPIO Interrupt Status for Rising Edge (IO0IntStatR - 0x4002 8084)**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0.0REI | Status of Rising Edge Interrupt for P0.0<br>• 0: No detectado<br>• 1: Interrupción generada por flanco ascendente | 0 |
| 1 | P0.1REI | Status of Rising Edge Interrupt for P0.1 | 0 |
| ... | ... | ... | ... |
| 15 | P0.15REI | Status of Rising Edge Interrupt for P0.15 | 0 |
| 16 | P0.16REI | Status of Rising Edge Interrupt for P0.16 | 0 |

#### IO0IntStatF - IO2IntStatF (Estado por Flanco Descendente)

Registro de **solo lectura** que indica si hay una interrupción por flanco descendente pendiente.

**Tabla 121: GPIO Interrupt Status for Falling Edge (IO0IntStatF - 0x4002 8088)**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0.0FEI | Status of Falling Edge Interrupt for P0.0<br>• 0: No detectado<br>• 1: Interrupción generada por flanco descendente | 0 |
| 1 | P0.1FEI | Status of Falling Edge Interrupt for P0.1 | 0 |
| ... | ... | ... | ... |
| 7 | P0.7FEI | Status of Falling Edge Interrupt for P0.7 | 0 |

#### IO0IntClr - IO2IntClr (Limpieza de Interrupciones)

Al escribir un 1 en cualquier bit de este registro, se limpia la interrupción pendiente del bit correspondiente.

**Tabla 123: GPIO Interrupt Clear Register (IO0IntClr - 0x4002 808C)**

| Bit | Símbolo | Descripción | Reset |
|-----|---------|-------------|--------|
| 0 | P0.0CI | Clear GPIO port Interrupts for P0.0<br>• 0: Los bits en IOxIntStatR e IOxStatF no cambian<br>• 1: Los bits correspondientes se limpian | 0 |
| 1 | P0.1CI | Clear GPIO port Interrupts for P0.1 | 0 |
| ... | ... | ... | ... |
| 6 | P0.6CI | Clear GPIO port Interrupts for P0.6 | 0 | 

 

Para ver un ejemplo sobre esto, ver el archivo `src/gpioInt.c`.

---

## Interrupciones externas

Se cuenta con **cuatro interrupciones externas**, estas son **EINT0, EINT1, EINT2 y EINT3**.
Esto **no** es una función de GPIO sino que es una funcionalidad específica de un pin; por lo tanto es necesario **configurar ese pin con dicha funcionalidad**.

Estos pines al configurarse como interrupción externa permiten interrumpir al procesador por **nivel alto/bajo** o por **flanco ascendente/descendente**.

Es importante destacar que **cada una de las interrupciones externas tiene un vector de interrupción propio asociado** (**salvo EINT3** que lo comparte con las interrupciones de **GPIO**).

### Registros asociados a la configuración de EINT

* En **PINSEL4** se observa que los **bits 20:27** están asignados a la configuración de las **interrupciones externas**.
* Los registros de control son:

  * **EXTINT** — *External Interrupt Flag register* (`0x400F C140`)
  * **EXTMODE** — *External Interrupt Mode register* (`0x400F C148`)
  * **EXTPOLAR** — *External Interrupt Polarity register* (`0x400F C14C`)

#### EXTINT

Cuando un pin es configurado como interrupción externa, el **nivel o flanco** en ese pin (seleccionado por los registros **EXTMODE** y **EXTPOLAR**) activará una **bandera de interrupción** en este registro con una solicitud al NVIC. En caso que las interrupciones de dicho pin estén habilitadas, causará una interrupción al microprocesador.

* **Escribir un `1` en los bits EINT0 a EINT3** **borra** el correspondiente bit.
* En **modo por nivel**, la interrupción se borra **solo cuando el pin está en su estado inactivo**.

#### EXTMODE

Los bits en este registro seleccionan si cada pin es **level-sensitive** o **edge-sensitive**.

#### EXTPOLAR

* En modo **level-sensitive**, este bit selecciona si el correspondiente pin es **high-active** o **low-active**.
* En modo **edge-sensitive**, este bit selecciona si el pin es sensible a **falling-edge** o **rising-edge**.


---

# Ejemplo paso a paso (GPIO  y NVIC)

1. **Seleccionar función GPIO del pin**

   * Para P2.12: `PINSEL4` bits `[25:24] = 00` (GPIO).
2. **Configurar dirección**

   * P2.12 como **entrada**: `FIO2DIR` bit 12 = 0.
   * P0.22 como **salida**: `FIO0DIR` bit 22 = 1.
3. **Limpiar estados previos en GPIOINT**

   * `IO2IntClr` = `1 << 12` (borra cualquier latcheo anterior).
4. **Elegir flanco(s)**

   * `IO2IntEnR |= (1 << 12)` para flanco **ascendente**
     (opcional: `IO2IntEnF |= (1 << 12)` para descendente).
5. **Configurar prioridad en NVIC**

   * Escribir el **byte de prioridad** (5 bits efectivos en `[7:3]`) para **EINT3\_IRQn** (la IRQ que atiende GPIO 0/2).
6. **Limpiar pendiente (pending) en NVIC**

   * `ICPR[0] |= (1 << EINT3_IRQn)`.
7. **Habilitar NVIC**

   * `ISER[0] |= (1 << EINT3_IRQn)`.
8. **En el handler (`EINT3_IRQHandler`)**

   * Leer `IO2IntStatR/F` para saber qué pin/edge disparó.
   * **Limpiar** el/los bits en `IO2IntClr` (si no, re-entra).
   * Hacer la acción (ej.: *toggle* del LED).

---

# Opción “A mano” (acceso directo a registros con punteros CMSIS)

```c
#include "LPC17xx.h"

/* Configuración de pines */
#define LED_PORT        LPC_GPIO0
#define LED_BIT         22

#define GPIO_IN_PORT    LPC_GPIO2
#define GPIO_IN_BIT     12

/* EINT3 atiende las interrupciones de GPIO (puertos 0 y 2) */
#define GPIO_NVIC_IRQn  EINT3_IRQn

int main(void) {
    /* --- 1) PINSEL: P0.22 (LED) a GPIO, P2.12 (input) a GPIO --- */
    /* P0.22 -> PINSEL1 bits [13:12] = 00 (GPIO) */
    LPC_PINCON->PINSEL1 &= ~(0x3u << 12);

    /* P2.12 -> PINSEL4 bits [25:24] = 00 (GPIO) */
    LPC_PINCON->PINSEL4 &= ~(0x3u << 24);

    /* --- 2) Direcciones --- */
    /* LED como salida */
    LED_PORT->FIODIR |=  (1u << LED_BIT);
    /* Input como entrada */
    GPIO_IN_PORT->FIODIR &= ~(1u << GPIO_IN_BIT);

    /* LED apagado al inicio */
    LED_PORT->FIOCLR = (1u << LED_BIT);

    /* --- 3) Limpiar estados previos en GPIOINT (port 2) --- */
    LPC_GPIOINT->IO2IntClr = (1u << GPIO_IN_BIT);

    /* --- 4) Habilitar flanco ascendente en P2.12 (opcional: descendente) --- */
    LPC_GPIOINT->IO2IntEnR |= (1u << GPIO_IN_BIT);
    // LPC_GPIOINT->IO2IntEnF |= (1u << GPIO_IN_BIT);  // si querés flanco bajo también

    /* --- 5) Prioridad NVIC (5 bits en [7:3]) para EINT3 --- */
    /* EINT3_IRQn típicamente vale 21 → byte NVIC->IP[21] */
    const uint32_t prio = 10u & 0x1Fu;      /* 0 = más alta, 31 = más baja */
    NVIC->IP[GPIO_NVIC_IRQn] = (uint8_t)(prio << 3);

    /* --- 6) Limpiar pendiente previo en NVIC --- */
    NVIC->ICPR[0] = (1u << GPIO_NVIC_IRQn);

    /* --- 7) Habilitar IRQ en NVIC --- */
    NVIC->ISER[0] = (1u << GPIO_NVIC_IRQn);

    while (1) {
        __WFI();  /* duerme; despierta con la IRQ */
    }
}

/* --- 8) Handler de EINT3 (GPIO 0/2) --- */
void EINT3_IRQHandler(void) {
    /* Chequear qué disparó: rising / falling en port 2, bit 12 */
    uint32_t statR = LPC_GPIOINT->IO2IntStatR;
    uint32_t statF = LPC_GPIOINT->IO2IntStatF;

    if ((statR | statF) & (1u << GPIO_IN_BIT)) {
        /* Limpiar SIEMPRE el flag del pin, sino re-entra en bucle */
        LPC_GPIOINT->IO2IntClr = (1u << GPIO_IN_BIT);

        /* Acción: toggle LED P0.22   */
        LED_PORT->FIOPIN ^= (1u << LED_BIT); 
    }

    /* (Opcional) Manejar también GPIO0 leyendo IO0IntStatR/F y limpiando IO0IntClr */
}
```

---

# Opción B — Con **funciones CMSIS** (más legible)

```c
#include "LPC17xx.h"

/* ... */

int main(void) {
    /* .... lo mismo que en el ejemplo anterior .... */

    /* NVIC: prioridad, clear pending y enable */
    NVIC_SetPriority(GPIO_NVIC_IRQn, 10);
    NVIC_ClearPendingIRQ(GPIO_NVIC_IRQn);
    NVIC_EnableIRQ(GPIO_NVIC_IRQn);

    while (1) {
        __WFI();
    }
}

void EINT3_IRQHandler(void) {
    uint32_t statR = LPC_GPIOINT->IO2IntStatR;
    uint32_t statF = LPC_GPIOINT->IO2IntStatF;

    if ((statR | statF) & (1u << GPIO_IN_BIT)) {
        LPC_GPIOINT->IO2IntClr = (1u << GPIO_IN_BIT);   // limpiar flag
        LED_PORT->FIOPIN ^= (1u << LED_BIT);            // toggle LED
    }
}
```
 
