# Tutorial: NVIC (Nested Vectored Interrupt Controller) para LPC1769

## Introducción

El **NVIC (Nested Vectored Interrupt Controller)** es el controlador de interrupciones del núcleo ARM Cortex-M3. Gestiona **todas las interrupciones** de los periféricos externos y las excepciones del sistema. Es fundamental para cualquier aplicación embedded que use interrupciones (GPIO, TIMER, UART, ADC, etc.).

**Características principales:**
- **35 interrupciones externas** configurables para LPC1769 (IRQ 0-34)
- **16 niveles de prioridad** (0-15, donde 0 es la máxima prioridad)
- **Soporte para anidamiento de interrupciones** (interrupciones de mayor prioridad pueden interrumpir a las de menor prioridad)
- **Prioridad configurable** individual para cada interrupción
- **Priority grouping** (división en preemption priority y sub-priority)
- **Vector Table** relocalizable (puede moverse a RAM)
- **Tail-chaining** y **late-arriving** optimizations (hardware automático)

**Archivos importantes:**
- CMSIS Core: `library/CMSISv2p00_LPC17xx/inc/core_cm3.h` (funciones principales)
- Driver LPC17xx: `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_nvic.h`
- Driver implementation: `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_nvic.c`
- IRQ definitions: `library/CMSISv2p00_LPC17xx/inc/LPC17xx.h`
- Ejemplos: `library/examples/NVIC/`

---

## Conceptos Fundamentales

### 1. Tipos de Interrupciones

El NVIC maneja dos tipos de interrupciones:

#### a) Interrupciones del Sistema (Core)
Estas son parte del Cortex-M3 y tienen **valores negativos** en `IRQn_Type`:

| IRQn                    | Valor | Descripción                       |
|-------------------------|-------|-----------------------------------|
| `MemoryManagement_IRQn` | -12   | Memory Management Interrupt       |
| `BusFault_IRQn`         | -11   | Bus Fault Interrupt              |
| `UsageFault_IRQn`       | -10   | Usage Fault Interrupt            |
| `SVCall_IRQn`           | -5    | Supervisor Call                  |
| `DebugMonitor_IRQn`     | -4    | Debug Monitor                    |
| `PendSV_IRQn`           | -2    | Pendable Service Request         |
| `SysTick_IRQn`          | -1    | System Tick Timer                |

#### b) Interrupciones Externas (Periféricos)
Estas son específicas del LPC1769 y tienen **valores positivos** (0-34):

**Verificado en:** `library/CMSISv2p00_LPC17xx/inc/LPC17xx.h` líneas 49-84

| IRQn                | Valor | Descripción                          |
|---------------------|-------|--------------------------------------|
| `WDT_IRQn`          | 0     | Watchdog Timer                       |
| `TIMER0_IRQn`       | 1     | Timer 0                              |
| `TIMER1_IRQn`       | 2     | Timer 1                              |
| `TIMER2_IRQn`       | 3     | Timer 2                              |
| `TIMER3_IRQn`       | 4     | Timer 3                              |
| `UART0_IRQn`        | 5     | UART 0                               |
| `UART1_IRQn`        | 6     | UART 1                               |
| `UART2_IRQn`        | 7     | UART 2                               |
| `UART3_IRQn`        | 8     | UART 3                               |
| `PWM1_IRQn`         | 9     | PWM 1                                |
| `I2C0_IRQn`         | 10    | I2C 0                                |
| `I2C1_IRQn`         | 11    | I2C 1                                |
| `I2C2_IRQn`         | 12    | I2C 2                                |
| `SPI_IRQn`          | 13    | SPI                                  |
| `SSP0_IRQn`         | 14    | SSP 0                                |
| `SSP1_IRQn`         | 15    | SSP 1                                |
| `PLL0_IRQn`         | 16    | PLL 0 Lock (Main PLL)                |
| `RTC_IRQn`          | 17    | Real Time Clock                      |
| `EINT0_IRQn`        | 18    | External Interrupt 0                 |
| `EINT1_IRQn`        | 19    | External Interrupt 1                 |
| `EINT2_IRQn`        | 20    | External Interrupt 2                 |
| `EINT3_IRQn`        | 21    | External Interrupt 3 **(GPIO)**      |
| `ADC_IRQn`          | 22    | ADC                                  |
| `BOD_IRQn`          | 23    | Brown-Out Detect                     |
| `USB_IRQn`          | 24    | USB                                  |
| `CAN_IRQn`          | 25    | CAN                                  |
| `DMA_IRQn`          | 26    | DMA                                  |
| `I2S_IRQn`          | 27    | I2S                                  |
| `ENET_IRQn`         | 28    | Ethernet                             |
| `RIT_IRQn`          | 29    | Repetitive Interrupt Timer           |
| `MCPWM_IRQn`        | 30    | Motor Control PWM                    |
| `QEI_IRQn`          | 31    | Quadrature Encoder Interface         |
| `PLL1_IRQn`         | 32    | PLL 1 Lock (USB PLL)                 |
| `USBActivity_IRQn`  | 33    | USB Activity                         |
| `CANActivity_IRQn`  | 34    | CAN Activity                         |

**Nota importante:** `EINT3_IRQn` (21) es usado para **todas las interrupciones GPIO** de PORT0 y PORT2.

### 2. Niveles de Prioridad

El LPC1769 implementa **5 bits de prioridad** (`__NVIC_PRIO_BITS = 5`), lo que resulta en **32 niveles** (0-31). Sin embargo, en la práctica se usan solo los bits más significativos, dando **16 niveles efectivos** (0-15):

- **0**: Máxima prioridad (más urgente)
- **15**: Mínima prioridad (menos urgente)

**Importante:** Prioridad 0 tiene precedencia sobre cualquier otra.

### 3. Priority Grouping

El NVIC permite dividir los bits de prioridad en dos grupos:

- **Preemption Priority (Group Priority)**: Determina si una interrupción puede interrumpir a otra
- **Sub-Priority**: Desempate cuando dos interrupciones están pendientes simultáneamente

**Configuración con `NVIC_SetPriorityGrouping()`:**

| Group | Bits Preemption | Bits Sub | Niveles Preemption | Niveles Sub |
|-------|-----------------|----------|--------------------|-------------|
| 0     | 0               | 5        | 1                  | 32          |
| 1     | 1               | 4        | 2                  | 16          |
| 2     | 2               | 3        | 4                  | 8           |
| 3     | 3               | 2        | 8                  | 4           |
| **4** | **4**           | **1**    | **16**             | **2**       |
| 5     | 5               | 0        | 32                 | 1           |

**Valor común:** `NVIC_SetPriorityGrouping(4)` → 16 niveles de preemption, 2 de sub-priority

**Formato de prioridad** cuando se usa `NVIC_SetPriority()` con grouping=4:

```
Bits [7:3]: Preemption priority (0-15)
Bits [2:1]: Sub-priority (0-1)
Bit [0]:    No usado

Ejemplo: Priority value = 0x22 = 0010 0010
  -> Preemption = 0001 (1)
  -> Sub-priority = 01 (1)
```

### 4. Anidamiento de Interrupciones

- Una interrupción con **mayor preemption priority** (valor menor) puede interrumpir a una de menor prioridad
- Si dos interrupciones con la **misma preemption priority** están pendientes, se ejecuta primero la de mayor sub-priority
- Si tienen igual preemption y sub-priority, se ejecuta la de **menor IRQ number**

**Ejemplo:**
```c
NVIC_SetPriorityGrouping(4);
NVIC_SetPriority(TIMER0_IRQn, 0x10);  // Preemption=1, Sub=0
NVIC_SetPriority(UART0_IRQn, 0x20);   // Preemption=2, Sub=0

// Si UART0 está ejecutándose y llega TIMER0:
// -> TIMER0 interrumpe a UART0 (preemption 1 < 2)
```

### 5. Vector Table (Tabla de Vectores)

La tabla de vectores contiene las direcciones de los ISR (Interrupt Service Routines). Por defecto está en:
- **ROM mode:** Dirección `0x00000000`
- **RAM mode:** Dirección `0x10000000`

Puede relocalizarse a RAM usando `NVIC_SetVTOR()` para aplicaciones avanzadas (bootloaders, etc.).

---

## Funciones del Driver (CMSIS)

Las funciones principales están definidas en **`core_cm3.h`** (parte de CMSIS):

### Habilitar/Deshabilitar Interrupciones

#### `NVIC_EnableIRQ()`
```c
static __INLINE void NVIC_EnableIRQ(IRQn_Type IRQn);
```

Habilita una interrupción específica en el NVIC.

**Parámetros:**
- `IRQn`: Número de interrupción (debe ser ≥ 0, no para interrupciones del sistema)

**Comportamiento interno** (verificado en core_cm3.h:928):
```c
NVIC->ISER[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
```
Escribe en el registro ISER (Interrupt Set Enable Register).

**Ejemplo:**
```c
NVIC_EnableIRQ(TIMER0_IRQn);   // Habilitar interrupción de TIMER0
NVIC_EnableIRQ(UART0_IRQn);    // Habilitar interrupción de UART0
NVIC_EnableIRQ(EINT3_IRQn);    // Habilitar interrupciones GPIO
```

#### `NVIC_DisableIRQ()`
```c
static __INLINE void NVIC_DisableIRQ(IRQn_Type IRQn);
```

Deshabilita una interrupción específica.

**Parámetros:**
- `IRQn`: Número de interrupción

**Comportamiento interno** (verificado en core_cm3.h:941):
```c
NVIC->ICER[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
```

**Ejemplo:**
```c
NVIC_DisableIRQ(TIMER0_IRQn);  // Deshabilitar TIMER0
```

### Configuración de Prioridades

#### `NVIC_SetPriority()`
```c
static __INLINE void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority);
```

Configura la prioridad de una interrupción.

**Parámetros:**
- `IRQn`: Número de interrupción (puede ser negativo para system interrupts)
- `priority`: Valor de prioridad (0-15 para LPC1769)
  - Si se usa priority grouping, este valor codifica preemption + sub-priority
  - Si no, es simplemente el nivel de prioridad

**Comportamiento interno** (verificado en core_cm3.h:1012-1018):
- Para IRQn < 0 (system): Escribe en `SCB->SHP[]`
- Para IRQn ≥ 0 (external): Escribe en `NVIC->IP[]`
- Ajusta automáticamente según `__NVIC_PRIO_BITS`

**Ejemplos:**

```c
// Sin grouping (prioridad simple)
NVIC_SetPriority(TIMER0_IRQn, 5);   // Prioridad 5
NVIC_SetPriority(UART0_IRQn, 10);   // Prioridad 10 (menor que TIMER0)

// Con grouping = 4 (4 bits preemption, 1 bit sub)
NVIC_SetPriorityGrouping(4);
// Formato: ((preemption << 3) | (sub << 1))
NVIC_SetPriority(TIMER0_IRQn, 0x10);  // Preemption=1, Sub=0
NVIC_SetPriority(UART0_IRQn, 0x12);   // Preemption=1, Sub=1
NVIC_SetPriority(ADC_IRQn, 0x20);     // Preemption=2, Sub=0
```

**Forma alternativa (más clara) con grouping:**
```c
NVIC_SetPriorityGrouping(4);
// Preemption priority 1, sub-priority 1
NVIC_SetPriority(EINT0_IRQn, ((0x01<<3)|0x01));
// Equivalente a: NVIC_SetPriority(EINT0_IRQn, 0x0A);
```

#### `NVIC_GetPriority()`
```c
static __INLINE uint32_t NVIC_GetPriority(IRQn_Type IRQn);
```

Lee la prioridad configurada de una interrupción.

**Parámetros:**
- `IRQn`: Número de interrupción

**Retorna:** Valor de prioridad

**Ejemplo:**
```c
uint32_t priority = NVIC_GetPriority(TIMER0_IRQn);
```

#### `NVIC_SetPriorityGrouping()`
```c
static __INLINE void NVIC_SetPriorityGrouping(uint32_t PriorityGroup);
```

Configura la división entre preemption priority y sub-priority.

**Parámetros:**
- `PriorityGroup`: Valor 0-7 (típicamente se usa 4)

**Comportamiento interno** (verificado en core_cm3.h:894-905):
Escribe en `SCB->AIRCR` con clave de protección `0x5FA`.

**Ejemplo:**
```c
NVIC_SetPriorityGrouping(4);  // 4 bits preemption, 1 bit sub-priority
```

#### `NVIC_GetPriorityGrouping()`
```c
static __INLINE uint32_t NVIC_GetPriorityGrouping(void);
```

Lee el valor actual de priority grouping.

**Retorna:** Valor 0-7

**Ejemplo:**
```c
uint32_t grouping = NVIC_GetPriorityGrouping();
```

### Manejo de Pending Interrupts

#### `NVIC_SetPendingIRQ()`
```c
static __INLINE void NVIC_SetPendingIRQ(IRQn_Type IRQn);
```

Marca una interrupción como pendiente (fuerza su ejecución).

**Parámetros:**
- `IRQn`: Número de interrupción

**Uso:** Debugging, simulación de eventos

**Ejemplo:**
```c
NVIC_SetPendingIRQ(TIMER0_IRQn);  // Forzar ejecución del ISR de TIMER0
```

#### `NVIC_GetPendingIRQ()`
```c
static __INLINE uint32_t NVIC_GetPendingIRQ(IRQn_Type IRQn);
```

Verifica si una interrupción está pendiente.

**Parámetros:**
- `IRQn`: Número de interrupción

**Retorna:** 0 (no pendiente) o 1 (pendiente)

**Ejemplo:**
```c
if (NVIC_GetPendingIRQ(TIMER0_IRQn)) {
    // TIMER0 interrupt está pendiente
}
```

#### `NVIC_ClearPendingIRQ()`
```c
static __INLINE void NVIC_ClearPendingIRQ(IRQn_Type IRQn);
```

Limpia el flag de interrupción pendiente.

**Parámetros:**
- `IRQn`: Número de interrupción

**Nota:** Normalmente no es necesario, el hardware lo hace automáticamente al entrar al ISR.

**Ejemplo:**
```c
NVIC_ClearPendingIRQ(TIMER0_IRQn);
```

---

## Funciones Adicionales del Driver LPC17xx

Definidas en **`lpc17xx_nvic.h`** y **`lpc17xx_nvic.c`**:

### `NVIC_DeInit()`
```c
void NVIC_DeInit(void);
```

Reinicia el NVIC a su estado por defecto.

**Comportamiento** (verificado en lpc17xx_nvic.c:61-76):
- Deshabilita todas las 32 interrupciones externas
- Limpia todos los pending flags
- Resetea todas las prioridades a 0

**Ejemplo:**
```c
NVIC_DeInit();  // Resetear todas las configuraciones NVIC
```

### `NVIC_SCBDeInit()`
```c
void NVIC_SCBDeInit(void);
```

Reinicia el SCB (System Control Block) a valores por defecto.

**Comportamiento** (verificado en lpc17xx_nvic.c:96-114):
- Resetea registros del SCB: ICSR, VTOR, AIRCR, SCR, CCR, SHP, etc.

**Ejemplo:**
```c
NVIC_SCBDeInit();
```

### `NVIC_SetVTOR()`
```c
void NVIC_SetVTOR(uint32_t offset);
```

Configura la dirección base de la Vector Table.

**Parámetros:**
- `offset`: Nueva dirección de la tabla de vectores
  - Debe estar alineada a 256 words (0x100)
  - Típicamente se usa para relocar a RAM: `0x10000000` o `0x20080000`

**Comportamiento interno** (verificado en lpc17xx_nvic.c:122-126):
```c
SCB->VTOR = offset;
```

**Ejemplo:**
```c
NVIC_SetVTOR(0x20080000);  // Relocar tabla a RAM
```

---

## Ejemplos Prácticos

### Ejemplo 1: Configuración Básica de Interrupciones

**Patrón típico** para habilitar una interrupción de periférico:

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_nvic.h"

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Procesar evento
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}

int main(void) {
    // 1. Configurar el periférico (TIMER0)
    // ... código de inicialización del timer ...

    // 2. Configurar prioridad (opcional, default es 0)
    NVIC_SetPriority(TIMER0_IRQn, 5);

    // 3. Habilitar interrupción en NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);

    // 4. Iniciar periférico
    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1) {
        // Main loop
    }
}
```

### Ejemplo 2: Múltiples Interrupciones con Diferentes Prioridades

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_gpio.h"

void TIMER0_IRQHandler(void) {
    // Alta prioridad - procesamiento rápido
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void UART0_IRQHandler(void) {
    // Prioridad media
    uint8_t data = UART_ReceiveByte(LPC_UART0);
    // Procesar data
}

void EINT3_IRQHandler(void) {
    // Baja prioridad - GPIO interrupts
    if (GPIO_GetIntStatus(0, 10, 1)) {
        GPIO_ClearInt(0, (1<<10));
        // Procesar evento
    }
}

int main(void) {
    // Configurar prioridades (0 = máxima, 15 = mínima)
    NVIC_SetPriority(TIMER0_IRQn, 2);   // Alta prioridad
    NVIC_SetPriority(UART0_IRQn, 8);    // Prioridad media
    NVIC_SetPriority(EINT3_IRQn, 12);   // Baja prioridad

    // Habilitar interrupciones
    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(UART0_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);

    while(1);
}
```

### Ejemplo 3: Priority Grouping con Preemption

**Verificado en:** `library/examples/NVIC/Priority/nvic_priority.c`

Este ejemplo muestra dos modos de anidamiento:

#### Modo 1: Tail-chaining (mismo grupo, diferente sub-priority)

```c
#include "lpc17xx_nvic.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"

void EINT0_IRQHandler(void) {
    uint8_t i;
    EXTI_ClearEXTIFlag(EXTI_EINT0);
    for (i = 0; i < 10; i++) {
        GPIO_SetValue(1, (1<<29));
        delay();
        GPIO_ClearValue(1, (1<<29));
        delay();
    }
}

void EINT3_IRQHandler(void) {
    uint8_t j;
    if (GPIO_GetIntStatus(0, 25, 1)) {
        GPIO_ClearInt(0, (1<<25));
        for (j = 0; j < 10; j++) {
            GPIO_SetValue(1, (1<<28));
            delay();
            GPIO_ClearValue(1, (1<<28));
            delay();
        }
    }
}

int main(void) {
    // Configurar GPIO outputs
    GPIO_SetDir(1, (1<<28)|(1<<29), 1);

    // Configurar EINT0 (P2.10)
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 1;
    PinCfg.Portnum = 2;
    PinCfg.Pinnum = 10;
    PINSEL_ConfigPin(&PinCfg);

    EXTI_InitTypeDef EXTICfg;
    EXTI_Init();
    EXTICfg.EXTI_Line = EXTI_EINT0;
    EXTICfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTICfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    EXTI_Config(&EXTICfg);

    // TAIL-CHAINING: mismo grupo, diferente sub-priority
    NVIC_SetPriorityGrouping(4);  // 16 grupos, 2 sub-levels
    NVIC_SetPriority(EINT0_IRQn, 2);  // 000:10 -> Group 0, sub 2
    NVIC_SetPriority(EINT3_IRQn, 1);  // 000:01 -> Group 0, sub 1

    // Como están en el mismo grupo de preemption, NO se interrumpen
    // Si ambas están pendientes, se ejecuta primero sub-priority menor (EINT3)
    // Luego se ejecuta EINT0 (tail-chaining optimizado por hardware)

    NVIC_EnableIRQ(EINT0_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);

    GPIO_IntCmd(0, (1<<25), 1);  // Habilitar GPIO P0.25 interrupt

    while(1);
}
```

#### Modo 2: Late-arriving (diferentes grupos de preemption)

```c
int main(void) {
    // ... misma configuración de pines y periféricos ...

    // LATE-ARRIVING: diferentes grupos de preemption
    NVIC_SetPriorityGrouping(4);
    NVIC_SetPriority(EINT0_IRQn, 0);   // 000:00 -> Preemption 0 (máxima)
    NVIC_SetPriority(EINT3_IRQn, 4);   // 001:00 -> Preemption 1

    // EINT0 puede interrumpir a EINT3 (preemption 0 < 1)
    // Si EINT3 está ejecutándose y llega EINT0, se interrumpe (anidamiento)

    NVIC_EnableIRQ(EINT0_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);

    GPIO_IntCmd(0, (1<<25), 1);

    while(1);
}
```

### Ejemplo 4: Relocalización de Vector Table

**Verificado en:** `library/examples/NVIC/VecTable_Relocation/vt_relocation.c`

```c
#include "lpc17xx_nvic.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_gpio.h"
#include <string.h>

#define VTOR_OFFSET  0x20080000  // Nueva dirección en RAM

volatile FunctionalState Cur_State = DISABLE;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();

    // Toggle P1.28
    if (Cur_State == ENABLE) {
        GPIO_ClearValue(1, (1<<28));
        Cur_State = DISABLE;
    } else {
        GPIO_SetValue(1, (1<<28));
        Cur_State = ENABLE;
    }
}

int main(void) {
    // Configurar P1.28 como salida
    GPIO_SetDir(1, (1<<28), 1);

    // Relocar Vector Table a RAM
    NVIC_SetVTOR(VTOR_OFFSET);

    // Copiar tabla de vectores original a nueva ubicación
    #if(__RAM_MODE__ == 0)  // ROM mode
        memcpy((void*)VTOR_OFFSET, (void*)0x00000000, 256*4);
    #else  // RAM mode
        memcpy((void*)VTOR_OFFSET, (void*)0x10000000, 256*4);
    #endif

    // Ahora las interrupciones usan la tabla relocada
    SYSTICK_InternalInit(100);  // 100ms
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    // P1.28 debe blinkar si la relocalización funcionó
    while(1);
}
```

**Uso práctico:** Bootloaders que saltan a aplicaciones en otras direcciones.

### Ejemplo 5: Deshabilitar Temporalmente Interrupciones

```c
#include "lpc17xx_nvic.h"

void critical_section(void) {
    // Deshabilitar una interrupción específica
    NVIC_DisableIRQ(TIMER0_IRQn);

    // Sección crítica - TIMER0 no puede interrumpir
    // ... código sensible ...

    // Re-habilitar
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void global_critical_section(void) {
    // Deshabilitar TODAS las interrupciones (CMSIS)
    __disable_irq();

    // Sección crítica global
    // ... código sensible ...

    // Re-habilitar todas
    __enable_irq();
}
```

---

## Errores Comunes y Soluciones

### Error 1: ISR nunca se ejecuta

**Síntoma:** El handler nunca se llama

**Causas y soluciones:**

1. **No habilitado en NVIC:**
```c
// INCORRECTO: Falta NVIC_EnableIRQ()
TIM_Init(LPC_TIM0, ...);
TIM_ConfigMatch(LPC_TIM0, ...);
TIM_Cmd(LPC_TIM0, ENABLE);
// ISR nunca se ejecuta!

// CORRECTO:
TIM_Init(LPC_TIM0, ...);
TIM_ConfigMatch(LPC_TIM0, ...);
NVIC_EnableIRQ(TIMER0_IRQn);  // ¡Crítico!
TIM_Cmd(LPC_TIM0, ENABLE);
```

2. **Nombre de ISR incorrecto:**
```c
// INCORRECTO: Nombre debe coincidir exactamente
void Timer0_IRQHandler(void) {  // ¡Mal! Debe ser TIMER0_IRQHandler
    // ...
}

// CORRECTO:
void TIMER0_IRQHandler(void) {  // Nombre exacto
    // ...
}
```

Verificar nombres en el archivo de startup (startup_LPC17xx.s).

3. **Periférico no genera interrupción:**
```c
// INCORRECTO: Interrupción no habilitada en el periférico
TIM_MatchConfigStruct.IntOnMatch = DISABLE;  // ¡No genera IRQ!
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
NVIC_EnableIRQ(TIMER0_IRQn);

// CORRECTO:
TIM_MatchConfigStruct.IntOnMatch = ENABLE;  // Habilitar en periférico
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
NVIC_EnableIRQ(TIMER0_IRQn);  // Habilitar en NVIC
```

### Error 2: Prioridad incorrecta causa problemas de anidamiento

**Síntoma:** Una interrupción no puede interrumpir a otra cuando debería

**Causa:** Configuración errónea de priority grouping

```c
// INCORRECTO: Sin grouping, todo es sub-priority (no hay anidamiento)
NVIC_SetPriority(TIMER0_IRQn, 0);
NVIC_SetPriority(UART0_IRQn, 5);
// UART0 NO puede interrumpir a TIMER0

// CORRECTO: Con grouping para permitir anidamiento
NVIC_SetPriorityGrouping(4);  // 4 bits preemption
NVIC_SetPriority(TIMER0_IRQn, 0x10);  // Preemption 1
NVIC_SetPriority(UART0_IRQn, 0x00);   // Preemption 0 (mayor)
// Ahora UART0 SÍ puede interrumpir a TIMER0
```

### Error 3: No limpiar flag de interrupción en periférico

**Síntoma:** ISR se ejecuta continuamente en loop infinito

```c
// INCORRECTO: No limpia flag del periférico
void TIMER0_IRQHandler(void) {
    // Procesar evento
    // Falta limpiar flag!
    // ISR se ejecuta infinitamente
}

// CORRECTO:
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Procesar evento
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // ¡Limpiar flag!
    }
}
```

**Importante:** El NVIC limpia automáticamente su propio pending flag, pero el **periférico** debe limpiarlo manualmente.

### Error 4: Confusión entre IRQn_Type valores

**Síntoma:** Compilador da error al usar número directo

```c
// INCORRECTO: Usar número mágico
NVIC_EnableIRQ(1);  // ¿Qué interrupción es?

// CORRECTO: Usar constante simbólica
NVIC_EnableIRQ(TIMER0_IRQn);  // Claro y legible
```

### Error 5: Usar `NVIC_SetPriority()` con valor fuera de rango

**Síntoma:** Comportamiento inesperado de prioridades

```c
// INCORRECTO: Valor fuera del rango efectivo (0-15 para LPC1769)
NVIC_SetPriority(TIMER0_IRQn, 20);  // > 15, se trunca

// CORRECTO:
NVIC_SetPriority(TIMER0_IRQn, 10);  // 0-15
```

### Error 6: Relocar Vector Table sin copiar contenido

**Síntoma:** Después de `NVIC_SetVTOR()`, el sistema se cuelga

```c
// INCORRECTO: Cambiar VTOR sin copiar tabla
NVIC_SetVTOR(0x20080000);
// ¡La memoria en 0x20080000 está vacía! → Hard Fault

// CORRECTO: Copiar tabla antes de cambiar VTOR
memcpy((void*)0x20080000, (void*)0x00000000, 256*4);
NVIC_SetVTOR(0x20080000);
```

---

## Patrón de Uso Típico

### Configuración Estándar de Interrupción

```c
// 1. Configurar periférico
TIM_TIMERCFG_Type TIM_ConfigStruct;
TIM_MATCHCFG_Type TIM_MatchConfigStruct;
// ... configurar estructuras ...
TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
TIM_MatchConfigStruct.IntOnMatch = ENABLE;  // Habilitar INT en periférico
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

// 2. Configurar NVIC (opcional: prioridad)
NVIC_SetPriority(TIMER0_IRQn, 5);

// 3. Habilitar interrupción en NVIC
NVIC_EnableIRQ(TIMER0_IRQn);

// 4. Iniciar periférico
TIM_Cmd(LPC_TIM0, ENABLE);

// 5. Implementar ISR
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Procesar evento
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // Limpiar flag
    }
}
```

### Configuración con Priority Grouping

```c
// Configurar grouping globalmente (una sola vez)
NVIC_SetPriorityGrouping(4);  // 16 preemption levels, 2 sub-levels

// Configurar prioridades de múltiples periféricos
NVIC_SetPriority(TIMER0_IRQn, 0x00);  // Preemption 0 (máxima), sub 0
NVIC_SetPriority(UART0_IRQn, 0x10);   // Preemption 1, sub 0
NVIC_SetPriority(ADC_IRQn, 0x12);     // Preemption 1, sub 1
NVIC_SetPriority(EINT3_IRQn, 0x20);   // Preemption 2, sub 0

// Habilitar todas
NVIC_EnableIRQ(TIMER0_IRQn);
NVIC_EnableIRQ(UART0_IRQn);
NVIC_EnableIRQ(ADC_IRQn);
NVIC_EnableIRQ(EINT3_IRQn);

// Resultado:
// - TIMER0 puede interrumpir a UART0, ADC, EINT3
// - UART0 puede interrumpir a ADC y EINT3
// - Si UART0 y ADC están pendientes simultáneamente, UART0 se ejecuta primero (sub-priority 0 < 1)
```

---

## Notas Importantes sobre LPC1769

1. **EINT3 compartido:** `EINT3_IRQn` se usa para **TODAS** las interrupciones GPIO (PORT0 y PORT2). Dentro del ISR, usar `GPIO_GetIntStatus()` para determinar qué pin causó la interrupción.

2. **Default priority:** Sin llamar a `NVIC_SetPriority()`, todas las interrupciones tienen prioridad 0 (máxima). Esto significa que **no hay anidamiento** por defecto.

3. **Priority bits:** El LPC1769 usa los 5 bits más significativos del byte de prioridad. Los 3 bits bajos se ignoran.

4. **Critical sections:** Usar `__disable_irq()` y `__enable_irq()` (CMSIS) para secciones críticas globales. Para deshabilitar una interrupción específica, usar `NVIC_DisableIRQ()`.

5. **Startup code:** Los nombres de los ISR están definidos en `startup_LPC17xx.s`. Deben coincidir exactamente (case-sensitive).

6. **Stack usage:** Cada ISR consume stack. Con anidamiento profundo, asegurar suficiente stack size.

7. **Latencia:** La latencia de interrupción del Cortex-M3 es de **12 ciclos** (sin tail-chaining).

8. **BASEPRI register:** Cortex-M3 tiene un registro BASEPRI para enmascarar interrupciones por debajo de cierta prioridad (no cubierto en este driver, pero disponible en CMSIS).

---

## Referencias

- **CMSIS Core:** `library/CMSISv2p00_LPC17xx/inc/core_cm3.h`
- **Driver header:** `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_nvic.h`
- **Driver source:** `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_nvic.c`
- **IRQ definitions:** `library/CMSISv2p00_LPC17xx/inc/LPC17xx.h`
- **Ejemplos verificados:**
  - `library/examples/NVIC/Priority/nvic_priority.c`
  - `library/examples/NVIC/VecTable_Relocation/vt_relocation.c`
- **ARM Documentation:** Cortex-M3 Technical Reference Manual
- **LPC17xx Manual:** UM10360, Capítulo 34: Nested Vectored Interrupt Controller

---

## Resumen

El **NVIC** es el controlador central de interrupciones:

- **Gestiona 35 interrupciones externas** del LPC1769
- **16 niveles de prioridad** configurables por interrupción
- **Priority grouping** permite división en preemption + sub-priority para anidamiento
- **Funciones principales** (CMSIS): `NVIC_EnableIRQ()`, `NVIC_SetPriority()`
- **Anidamiento** permite que interrupciones de alta prioridad interrumpan a las de baja

**Puntos clave:**
1. Siempre habilitar con `NVIC_EnableIRQ()` después de configurar el periférico
2. Configurar prioridades según criticidad de cada tarea
3. Usar `NVIC_SetPriorityGrouping(4)` para permitir anidamiento
4. Limpiar flags del periférico en el ISR
5. Nombres de ISR deben coincidir con startup code

El NVIC es esencial para cualquier aplicación que use interrupciones en el LPC1769.
