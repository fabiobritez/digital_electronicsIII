# Tutorial: Driver TIMER para LPC1769

## Introducción

Los **Timers** del LPC1769 son periféricos versátiles que permiten medir tiempos, generar delays, capturar eventos externos, generar señales PWM por software, y contar eventos externos. El LPC1769 cuenta con **4 timers idénticos** (TIMER0, TIMER1, TIMER2, TIMER3), cada uno con las mismas capacidades.

**Características principales:**
- **4 timers** de 32 bits (TIMER0 a TIMER3)
- **Contador de 32 bits** (TC - Timer Counter)
- **Pre-escalador de 32 bits** (PC - Prescale Counter y PR - Prescale Register)
- **4 canales Match** (MR0 a MR3) para comparación
- **2 canales Capture** (CAP0 y CAP1) para captura de eventos
- **Modos de operación**: Timer mode y Counter mode (rising/falling/both edges)
- **Salidas externas Match** (MAT0.0 a MAT3.0) configurables
- **Interrupciones** por Match o Capture

**Archivos del driver:**
- Header: `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_timer.h`
- Implementación: `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_timer.c`
- Ejemplos: `library/examples/TIMER/`

---

## Conceptos Fundamentales

### 1. Modos de Operación

El timer puede operar en dos modos principales:

#### a) Timer Mode (Modo Temporizador)
El contador (TC) se incrementa con cada ciclo de reloj prescalado derivado del PCLK (reloj periférico). Este es el modo más común para generar delays, medir tiempos, y generar señales.

**Ecuación:**
```
Frecuencia_Timer = PCLK / (Prescale_Value + 1)
Tiempo_Match = (Match_Value + 1) / Frecuencia_Timer
```

#### b) Counter Mode (Modo Contador)
El contador (TC) se incrementa con eventos externos en los pines CAP (Capture). Puede contar en:
- **Rising edge** (flanco ascendente)
- **Falling edge** (flanco descendente)
- **Both edges** (ambos flancos)

### 2. Prescaler (Pre-escalador)

El prescaler divide la frecuencia del reloj para obtener la velocidad deseada del timer. Existen dos modos:

**a) TIM_PRESCALE_TICKVAL**: Valor directo de ticks
```c
// PR = PrescaleValue - 1
// Cada tick del timer ocurre cada (PrescaleValue) ciclos de PCLK
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_TICKVAL;
TIM_ConfigStruct.PrescaleValue = 100;  // Timer incrementa cada 100 ciclos de PCLK
```

**b) TIM_PRESCALE_USVAL**: Valor en microsegundos
```c
// El driver calcula automáticamente PR basado en PCLK
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 10;  // Timer incrementa cada 10 microsegundos
```

### 3. Match Channels (Canales de Comparación)

Cada timer tiene **4 canales Match** (MR0, MR1, MR2, MR3). Cuando el Timer Counter (TC) alcanza el valor en un Match Register, puede:

- **Generar interrupción** (`IntOnMatch`)
- **Resetear el contador** (`ResetOnMatch`)
- **Detener el timer** (`StopOnMatch`)
- **Controlar pin externo** (`ExtMatchOutputType`):
  - `TIM_EXTMATCH_NOTHING`: No hacer nada
  - `TIM_EXTMATCH_LOW`: Forzar salida a LOW
  - `TIM_EXTMATCH_HIGH`: Forzar salida a HIGH
  - `TIM_EXTMATCH_TOGGLE`: Toggle (cambiar estado)

### 4. Capture Channels (Canales de Captura)

Cada timer tiene **2 canales Capture** (CAP0, CAP1). Cuando ocurre un evento en el pin externo, el valor actual de TC se copia al Capture Register. Útil para:

- Medir frecuencias
- Medir anchos de pulso
- Detectar eventos externos con timestamp

---

## Pines de Timer en LPC1769

### Pines Match (Salidas)

Verificados en `library/examples/TIMER/PWMSignal/pwm_signal.c`:

| Timer | Canal | Pin    | Función PINSEL |
|-------|-------|--------|----------------|
| TIM0  | MAT0.0| P1.28  | 3              |
| TIM1  | MAT1.0| P1.22  | 3              |
| TIM2  | MAT2.0| P0.6   | 3              |
| TIM3  | MAT3.0| P0.10  | 3              |

**Nota:** Para otros canales Match (MAT0.1, MAT0.2, etc.) consultar la **Tabla 8.5** del manual del usuario LPC17xx.

### Pines Capture (Entradas)

Verificado en `library/examples/TIMER/Capture/timer_capture.c` y `freqmeasure.c`:

| Timer | Canal | Pin    | Función PINSEL |
|-------|-------|--------|----------------|
| TIM0  | CAP0.0| P1.26  | 3              |

**Nota:** Para otros canales Capture consultar la **Tabla 8.5** del manual del usuario LPC17xx.

---

## Estructuras de Datos

### 1. TIM_TIMERCFG_Type

Configuración básica del timer (prescaler):

```c
typedef struct {
    uint8_t PrescaleOption;   // TIM_PRESCALE_TICKVAL o TIM_PRESCALE_USVAL
    uint32_t PrescaleValue;   // Valor del prescaler
} TIM_TIMERCFG_Type;
```

**Campos:**
- `PrescaleOption`:
  - `TIM_PRESCALE_TICKVAL`: PrescaleValue es el número de ticks de PCLK
  - `TIM_PRESCALE_USVAL`: PrescaleValue es el tiempo en microsegundos
- `PrescaleValue`: Valor del prescaler (significado depende de PrescaleOption)

### 2. TIM_MATCHCFG_Type

Configuración de canales Match:

```c
typedef struct {
    uint8_t MatchChannel;         // Canal Match: 0, 1, 2, o 3
    uint8_t IntOnMatch;           // ENABLE o DISABLE
    uint8_t StopOnMatch;          // ENABLE o DISABLE
    uint8_t ResetOnMatch;         // ENABLE o DISABLE
    uint8_t ExtMatchOutputType;   // Tipo de salida externa
    uint32_t MatchValue;          // Valor de comparación
} TIM_MATCHCFG_Type;
```

**Campos:**
- `MatchChannel`: Canal a usar (0 a 3)
- `IntOnMatch`: Generar interrupción cuando TC == MRn
- `StopOnMatch`: Detener timer cuando TC == MRn
- `ResetOnMatch`: Resetear TC a 0 cuando TC == MRn
- `ExtMatchOutputType`:
  - `TIM_EXTMATCH_NOTHING` (0): No afectar pin externo
  - `TIM_EXTMATCH_LOW` (1): Pin externo a LOW
  - `TIM_EXTMATCH_HIGH` (2): Pin externo a HIGH
  - `TIM_EXTMATCH_TOGGLE` (3): Toggle pin externo
- `MatchValue`: Valor con el que comparar TC

### 3. TIM_CAPTURECFG_Type

Configuración de canales Capture:

```c
typedef struct {
    uint8_t CaptureChannel;   // Canal Capture: 0 o 1
    uint8_t RisingEdge;       // ENABLE o DISABLE
    uint8_t FallingEdge;      // ENABLE o DISABLE
    uint8_t IntOnCaption;     // ENABLE o DISABLE
} TIM_CAPTURECFG_Type;
```

**Campos:**
- `CaptureChannel`: Canal a usar (0 o 1)
- `RisingEdge`: Capturar en flanco ascendente
- `FallingEdge`: Capturar en flanco descendente
- `IntOnCaption`: Generar interrupción al capturar

---

## Funciones del Driver

### Funciones de Inicialización

#### `TIM_Init()`
```c
void TIM_Init(LPC_TIM_TypeDef *TIMx, TIM_MODE_OPT TimerCounterMode, void *TIM_ConfigStruct);
```

Inicializa el timer con configuración básica.

**Parámetros:**
- `TIMx`: Timer a usar (`LPC_TIM0`, `LPC_TIM1`, `LPC_TIM2`, `LPC_TIM3`)
- `TimerCounterMode`: Modo de operación:
  - `TIM_TIMER_MODE`: Modo timer (basado en PCLK)
  - `TIM_COUNTER_RISING_MODE`: Contador en flanco ascendente
  - `TIM_COUNTER_FALLING_MODE`: Contador en flanco descendente
  - `TIM_COUNTER_ANY_MODE`: Contador en ambos flancos
- `TIM_ConfigStruct`: Puntero a `TIM_TIMERCFG_Type` o `TIM_COUNTERCFG_Type`

**Comportamiento interno** (verificado en lpc17xx_timer.c:287):
- Enciende el reloj del timer (PCONP)
- Configura PCLK divisor = CCLK/4
- Resetea TC y PC a 0
- Configura el prescaler según PrescaleOption
- Limpia interrupciones pendientes

**Ejemplo:**
```c
TIM_TIMERCFG_Type TIM_ConfigStruct;
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 100;  // 100 microsegundos por tick
TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
```

#### `TIM_DeInit()`
```c
void TIM_DeInit(LPC_TIM_TypeDef *TIMx);
```

Apaga el timer y desactiva su alimentación.

**Parámetros:**
- `TIMx`: Timer a apagar

**Ejemplo:**
```c
TIM_DeInit(LPC_TIM0);
```

### Funciones de Control

#### `TIM_Cmd()`
```c
void TIM_Cmd(LPC_TIM_TypeDef *TIMx, FunctionalState NewState);
```

Habilita o deshabilita el contador del timer.

**Parámetros:**
- `TIMx`: Timer a controlar
- `NewState`: `ENABLE` o `DISABLE`

**Ejemplo:**
```c
TIM_Cmd(LPC_TIM0, ENABLE);   // Iniciar timer
TIM_Cmd(LPC_TIM0, DISABLE);  // Detener timer
```

#### `TIM_ResetCounter()`
```c
void TIM_ResetCounter(LPC_TIM_TypeDef *TIMx);
```

Resetea TC y PC a 0 en el siguiente flanco de PCLK.

**Parámetros:**
- `TIMx`: Timer a resetear

**Ejemplo:**
```c
TIM_ResetCounter(LPC_TIM0);
```

### Funciones de Match

#### `TIM_ConfigMatch()`
```c
void TIM_ConfigMatch(LPC_TIM_TypeDef *TIMx, TIM_MATCHCFG_Type *TIM_MatchConfigStruct);
```

Configura un canal Match.

**Parámetros:**
- `TIMx`: Timer a configurar
- `TIM_MatchConfigStruct`: Puntero a estructura de configuración

**Comportamiento interno** (verificado en lpc17xx_timer.c:450):
- Escribe MatchValue en el registro MRn correspondiente (MR0, MR1, MR2, MR3)
- Configura bits de MCR (Match Control Register) para interrupt/reset/stop
- Configura bits de EMR (External Match Register) para salida externa

**Ejemplo:**
```c
TIM_MATCHCFG_Type TIM_MatchConfigStruct;
TIM_MatchConfigStruct.MatchChannel = 0;
TIM_MatchConfigStruct.IntOnMatch = ENABLE;
TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
TIM_MatchConfigStruct.StopOnMatch = DISABLE;
TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
TIM_MatchConfigStruct.MatchValue = 10000;
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
```

#### `TIM_UpdateMatchValue()`
```c
void TIM_UpdateMatchValue(LPC_TIM_TypeDef *TIMx, uint8_t MatchChannel, uint32_t MatchValue);
```

Actualiza el valor Match sin reconfigurar otros parámetros.

**Parámetros:**
- `TIMx`: Timer a actualizar
- `MatchChannel`: Canal Match (0 a 3)
- `MatchValue`: Nuevo valor de comparación

**Ejemplo:**
```c
TIM_UpdateMatchValue(LPC_TIM0, 0, 5000);  // Cambiar MR0 a 5000
```

### Funciones de Capture

#### `TIM_ConfigCapture()`
```c
void TIM_ConfigCapture(LPC_TIM_TypeDef *TIMx, TIM_CAPTURECFG_Type *TIM_CaptureConfigStruct);
```

Configura un canal Capture.

**Parámetros:**
- `TIMx`: Timer a configurar
- `TIM_CaptureConfigStruct`: Puntero a estructura de configuración

**Comportamiento interno** (verificado en lpc17xx_timer.c:542):
- Configura bits en CCR (Capture Control Register)
- Habilita captura en rising edge, falling edge, o ambos
- Habilita/deshabilita interrupción por captura

**Ejemplo:**
```c
TIM_CAPTURECFG_Type TIM_CaptureConfigStruct;
TIM_CaptureConfigStruct.CaptureChannel = 0;
TIM_CaptureConfigStruct.RisingEdge = ENABLE;
TIM_CaptureConfigStruct.FallingEdge = DISABLE;
TIM_CaptureConfigStruct.IntOnCaption = ENABLE;
TIM_ConfigCapture(LPC_TIM0, &TIM_CaptureConfigStruct);
```

#### `TIM_GetCaptureValue()`
```c
uint32_t TIM_GetCaptureValue(LPC_TIM_TypeDef *TIMx, TIM_COUNTER_INPUT_OPT CaptureChannel);
```

Lee el valor capturado en un canal Capture.

**Parámetros:**
- `TIMx`: Timer a leer
- `CaptureChannel`: `TIM_COUNTER_INCAP0` (0) o `TIM_COUNTER_INCAP1` (1)

**Retorna:** Valor del Timer Counter en el momento de la captura

**Ejemplo:**
```c
uint32_t captured_value = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
```

### Funciones de Interrupciones

#### `TIM_GetIntStatus()`
```c
FlagStatus TIM_GetIntStatus(LPC_TIM_TypeDef *TIMx, TIM_INT_TYPE IntFlag);
```

Verifica si una interrupción Match está pendiente.

**Parámetros:**
- `TIMx`: Timer a verificar
- `IntFlag`:
  - `TIM_MR0_INT`: Match channel 0
  - `TIM_MR1_INT`: Match channel 1
  - `TIM_MR2_INT`: Match channel 2
  - `TIM_MR3_INT`: Match channel 3
  - `TIM_CR0_INT`: Capture channel 0
  - `TIM_CR1_INT`: Capture channel 1

**Retorna:** `SET` o `RESET`

**Ejemplo:**
```c
if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
    // Match 0 interrupt occurred
}
```

#### `TIM_GetIntCaptureStatus()`
```c
FlagStatus TIM_GetIntCaptureStatus(LPC_TIM_TypeDef *TIMx, TIM_INT_TYPE IntFlag);
```

Verifica si una interrupción Capture está pendiente.

**Parámetros:**
- `TIMx`: Timer a verificar
- `IntFlag`: `TIM_CR0_INT` (0) o `TIM_CR1_INT` (1)

**Retorna:** `SET` o `RESET`

**Ejemplo:**
```c
if (TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
    // Capture 0 interrupt occurred
}
```

#### `TIM_ClearIntPending()`
```c
void TIM_ClearIntPending(LPC_TIM_TypeDef *TIMx, TIM_INT_TYPE IntFlag);
```

Limpia una interrupción Match pendiente. **DEBE llamarse en el ISR**.

**Parámetros:**
- `TIMx`: Timer a limpiar
- `IntFlag`: Tipo de interrupción a limpiar (ver `TIM_GetIntStatus`)

**Ejemplo:**
```c
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Manejar interrupción
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // Limpiar flag
    }
}
```

#### `TIM_ClearIntCapturePending()`
```c
void TIM_ClearIntCapturePending(LPC_TIM_TypeDef *TIMx, TIM_INT_TYPE IntFlag);
```

Limpia una interrupción Capture pendiente. **DEBE llamarse en el ISR**.

**Parámetros:**
- `TIMx`: Timer a limpiar
- `IntFlag`: `TIM_CR0_INT` o `TIM_CR1_INT`

**Ejemplo:**
```c
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
        uint32_t captured = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
        // Procesar valor capturado
        TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT);  // Limpiar flag
    }
}
```

---

## Ejemplos Prácticos

### Ejemplo 1: Match con Interrupción (Toggle LED cada 1 segundo)

**Verificado en:** `library/examples/TIMER/Interrupt_Match/timer_int_match.c`

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

volatile uint8_t timer_flag = 0;

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        timer_flag = 1;  // Señal para main loop
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}

int main(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // Configurar P1.28 como MAT0.0
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 28;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar timer con prescale de 100us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 100;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

    // Configurar Match channel 0
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;       // Generar interrupción
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;     // Resetear contador
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;     // No detener
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;  // Toggle pin
    // Match value: 10000 * 100us = 1,000,000us = 1 segundo --> 1 Hz
    TIM_MatchConfigStruct.MatchValue = 10000;
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

    // Configurar NVIC
    NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER0_IRQn);

    // Iniciar timer
    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1) {
        if (timer_flag) {
            timer_flag = 0;
            // Procesar evento de match
        }
    }
}
```

**Explicación:**
- **Prescale:** 100 microsegundos por tick
- **Match Value:** 10000 ticks = 1 segundo
- **ExtMatchOutputType:** `TIM_EXTMATCH_TOGGLE` hace que P1.28 cambie de estado cada segundo
- **ResetOnMatch:** El contador se resetea automáticamente, generando una señal periódica

---

### Ejemplo 2: Match en Modo Polling (sin interrupción)

**Verificado en:** `library/examples/TIMER/Polling_Match/timer_poll_match.c`

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // Configurar P1.28 como MAT0.0
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 28;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar timer con prescale de 100us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 100;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

    // Configurar Match channel 0
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;  // Flag se setea, pero no genera IRQ
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    // 1000 * 100us = 100ms --> 10Hz
    TIM_MatchConfigStruct.MatchValue = 1000;
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

    // NO se habilita NVIC - modo polling
    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1) {
        // Esperar a que ocurra match (polling)
        while (!TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT));

        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // Limpiar flag

        // Procesar evento cada 100ms
    }
}
```

**Nota importante:** Incluso en modo polling, **debe limpiarse el flag** con `TIM_ClearIntPending()`, aunque no se use NVIC.

---

### Ejemplo 3: Generación PWM por Software (múltiples duty cycles)

**Verificado en:** `library/examples/TIMER/PWMSignal/pwm_signal.c`

Este ejemplo genera señales PWM usando 4 timers con diferentes duty cycles (12.5%, 25%, 37.5%, 50%).

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

FunctionalState PWM0_State = ENABLE;

void TIMER0_IRQHandler(void) {
    TIM_Cmd(LPC_TIM0, DISABLE);
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    TIM_ResetCounter(LPC_TIM0);

    if (PWM0_State == ENABLE) {
        // Duty cycle 12.5%: LOW durante 100 ticks
        TIM_UpdateMatchValue(LPC_TIM0, 0, 100);
        PWM0_State = DISABLE;
    } else {
        // HIGH durante 700 ticks
        // Total periodo: 100 + 700 = 800 ticks
        // Duty = 700/800 = 87.5% HIGH = 12.5% LOW
        TIM_UpdateMatchValue(LPC_TIM0, 0, 700);
        PWM0_State = ENABLE;
    }

    TIM_Cmd(LPC_TIM0, ENABLE);
}

int main(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // Configurar P1.28 como MAT0.0
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 28;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar timer con prescale de 100us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 100;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

    // Configurar Match con toggle
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_MatchConfigStruct.MatchValue = 700;  // Valor inicial
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

    // Configurar NVIC
    NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1);
}
```

**Explicación:**
- El ISR alterna entre dos valores Match: 100 y 700
- Con `TIM_EXTMATCH_TOGGLE`, el pin cambia de estado en cada match
- Periodo total = 100 + 700 = 800 ticks = 80ms
- Duty cycle = 700/800 = 87.5% (pin HIGH 87.5% del tiempo)

---

### Ejemplo 4: Captura de Señal (Medición de Frecuencia)

**Verificado en:** `library/examples/TIMER/FreqMeasure/freqmeasure.c`

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

volatile uint32_t captured_value = 0;
volatile uint8_t capture_done = 0;
volatile uint8_t first_capture = 1;
volatile uint8_t count = 0;

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
        TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT);

        if (first_capture) {
            // Primeras capturas para estabilizar
            TIM_Cmd(LPC_TIM0, DISABLE);
            TIM_ResetCounter(LPC_TIM0);
            TIM_Cmd(LPC_TIM0, ENABLE);
            count++;
            if (count == 5) first_capture = 0;  // Ya estabilizado
        } else {
            count = 0;
            capture_done = 1;
            captured_value = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
        }
    }
}

int main(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_CAPTURECFG_Type TIM_CaptureConfigStruct;
    PINSEL_CFG_Type PinCfg;
    uint32_t frequency;

    // Configurar P1.26 como CAP0.0
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 26;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar timer con prescale de 1us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

    // Configurar Capture en rising edge
    TIM_CaptureConfigStruct.CaptureChannel = 0;
    TIM_CaptureConfigStruct.RisingEdge = ENABLE;
    TIM_CaptureConfigStruct.FallingEdge = DISABLE;
    TIM_CaptureConfigStruct.IntOnCaption = ENABLE;
    TIM_ConfigCapture(LPC_TIM0, &TIM_CaptureConfigStruct);

    // Configurar NVIC
    NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_ResetCounter(LPC_TIM0);
    TIM_Cmd(LPC_TIM0, ENABLE);

    // Esperar medición
    while (!capture_done);

    // Calcular frecuencia
    // Periodo (us) = captured_value (cada tick es 1us)
    // Frecuencia (Hz) = 1,000,000 / Periodo(us)
    frequency = 1000000 / captured_value;

    // frequency contiene la frecuencia medida en Hz

    while(1);
}
```

**Explicación:**
- Se captura el valor de TC en cada flanco ascendente
- El primer ciclo resetea el contador para sincronizar
- El segundo ciclo captura el periodo completo de la señal
- Con prescale de 1μs: `Frecuencia = 1,000,000 / captured_value`

---

### Ejemplo 5: Generar Señales con Diferentes Delays

**Verificado en:** `library/examples/TIMER/Gen_Diff_Delay/gen_diff_delay.c`

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

volatile uint8_t toggle = 1;
volatile uint32_t T1 = 100;  // 100ms
volatile uint32_t T2 = 300;  // 300ms

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        TIM_Cmd(LPC_TIM0, DISABLE);
        TIM_ResetCounter(LPC_TIM0);

        if (toggle) {
            // Configurar delay T1
            TIM_UpdateMatchValue(LPC_TIM0, 0, T1 * 10);  // T1 en ms, prescale 100us
            toggle = 0;
        } else {
            // Configurar delay T2
            TIM_UpdateMatchValue(LPC_TIM0, 0, T2 * 10);
            toggle = 1;
        }

        TIM_Cmd(LPC_TIM0, ENABLE);
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}

int main(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // Configurar P1.28 como MAT0.0
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 28;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar timer con prescale de 100us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 100;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

    // Configurar Match
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;
    TIM_MatchConfigStruct.ResetOnMatch = DISABLE;  // Manual reset en ISR
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_MatchConfigStruct.MatchValue = T1 * 10;
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

    // Configurar NVIC
    NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1);
}
```

**Explicación:**
- Genera una señal que está HIGH durante T1 y LOW durante T2 (o viceversa)
- El ISR alterna entre dos valores Match
- Útil para generar señales asimétricas o protocolos de comunicación

---

## Modo Counter (Contador Externo)

**Verificado en:** `library/examples/SysTick/STCLK/systick_stclk.c` (usa TIMER para generar señal externa)

El modo Counter permite contar eventos externos en lugar de ciclos de reloj:

```c
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    TIM_COUNTERCFG_Type TIM_CounterConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;

    // Nota: En Counter mode, se usa TIM_COUNTERCFG_Type en lugar de TIM_TIMERCFG_Type

    // Configurar input para counter (CAP input)
    TIM_CounterConfigStruct.CountInputSelect = TIM_COUNTER_INCAP0;  // Usar CAP0 como entrada

    // Inicializar en modo counter - rising edge
    TIM_Init(LPC_TIM0, TIM_COUNTER_RISING_MODE, &TIM_CounterConfigStruct);

    // Configurar Match para contar hasta 100 eventos
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_MatchConfigStruct.MatchValue = 100;  // Contar 100 eventos
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

    TIM_Cmd(LPC_TIM0, ENABLE);

    while(1);
}
```

**Modos Counter disponibles:**
- `TIM_COUNTER_RISING_MODE`: Incrementar en flanco ascendente
- `TIM_COUNTER_FALLING_MODE`: Incrementar en flanco descendente
- `TIM_COUNTER_ANY_MODE`: Incrementar en ambos flancos

---

## Errores Comunes y Soluciones

### Error 1: No se genera interrupción

**Síntoma:** El ISR nunca se ejecuta

**Causas posibles:**
1. **No habilitar NVIC:**
```c
// INCORRECTO: Falta habilitar NVIC
TIM_MatchConfigStruct.IntOnMatch = ENABLE;
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
TIM_Cmd(LPC_TIM0, ENABLE);

// CORRECTO:
TIM_MatchConfigStruct.IntOnMatch = ENABLE;
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
NVIC_EnableIRQ(TIMER0_IRQn);  // ¡No olvidar!
TIM_Cmd(LPC_TIM0, ENABLE);
```

2. **No limpiar flag en ISR:**
```c
// INCORRECTO: No limpia flag, ISR se ejecuta una sola vez
void TIMER0_IRQHandler(void) {
    // Procesar interrupción
    // Falta TIM_ClearIntPending()
}

// CORRECTO:
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Procesar interrupción
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // ¡Limpiar flag!
    }
}
```

### Error 2: Match value incorrecto

**Síntoma:** El timer genera evento a frecuencia incorrecta

**Causa:** No tomar en cuenta el prescaler

```c
// INCORRECTO: Quiero delay de 1 segundo pero solo dura 1ms
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 1000;  // 1000us = 1ms por tick
TIM_MatchConfigStruct.MatchValue = 1000;  // 1000 ticks = 1000ms? NO!
// ERROR: 1000 ticks * 1ms/tick = 1 segundo, pero el cálculo es incorrecto

// CORRECTO: Cálculo explícito
// Queremos 1 segundo con prescale de 1ms
// 1 segundo = 1000 ms = 1000 ticks (con 1ms/tick)
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 1000;  // 1ms por tick
TIM_MatchConfigStruct.MatchValue = 1000 - 1;  // 1000 ticks = 1 segundo
// Nota: Algunos ejemplos usan MatchValue sin restar 1, depende del ResetOnMatch timing
```

**Fórmula correcta:**
```
Tiempo_total = MatchValue * (PrescaleValue / 1,000,000)  [si PrescaleOption = USVAL]
Tiempo_total = MatchValue * PrescaleValue / PCLK         [si PrescaleOption = TICKVAL]
```

### Error 3: Capture no funciona

**Síntoma:** No se capturan valores o siempre captura 0

**Causas posibles:**
1. **Pin no configurado como CAPn:**
```c
// INCORRECTO: Pin configurado como GPIO
GPIO_SetDir(1, (1<<26), 0);

// CORRECTO: Pin configurado como CAP0.0
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 3;  // CAP0.0
PinCfg.Portnum = 1;
PinCfg.Pinnum = 26;
PINSEL_ConfigPin(&PinCfg);
```

2. **Edge mal configurado:**
```c
// INCORRECTO: No se habilita ningún edge
TIM_CaptureConfigStruct.RisingEdge = DISABLE;
TIM_CaptureConfigStruct.FallingEdge = DISABLE;

// CORRECTO: Al menos un edge habilitado
TIM_CaptureConfigStruct.RisingEdge = ENABLE;  // O FallingEdge, o ambos
```

### Error 4: Variables no marcadas como volatile

**Síntoma:** El programa no responde a cambios en variables modificadas en ISR

```c
// INCORRECTO:
uint8_t timer_flag = 0;

void TIMER0_IRQHandler(void) {
    timer_flag = 1;
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

int main(void) {
    // ... configuración ...
    while (!timer_flag);  // ¡Puede optimizarse y nunca salir!
}

// CORRECTO:
volatile uint8_t timer_flag = 0;  // volatile es CRÍTICO

void TIMER0_IRQHandler(void) {
    timer_flag = 1;
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

int main(void) {
    // ... configuración ...
    while (!timer_flag);  // Ahora funciona correctamente
}
```

### Error 5: Usar TIMER sin inicializar PINSEL

**Síntoma:** Los pines externos MAT o CAP no funcionan

```c
// INCORRECTO: Falta configurar PINSEL
TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
// El pin P1.28 no cambia porque está como GPIO, no como MAT0.0

// CORRECTO: Configurar PINSEL primero
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 3;  // MAT0.0
PinCfg.Portnum = 1;
PinCfg.Pinnum = 28;
PINSEL_ConfigPin(&PinCfg);
// Luego configurar Match
TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
```

---

## Patrón de Uso Típico

### 1. Match con Interrupción (Temporizador Periódico)

```c
// 1. Configurar PINSEL si se usa salida externa
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 3;
PinCfg.Portnum = 1;
PinCfg.Pinnum = 28;  // MAT0.0
PINSEL_ConfigPin(&PinCfg);

// 2. Configurar e inicializar Timer
TIM_TIMERCFG_Type TIM_ConfigStruct;
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 100;  // 100us por tick
TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

// 3. Configurar Match
TIM_MATCHCFG_Type TIM_MatchConfigStruct;
TIM_MatchConfigStruct.MatchChannel = 0;
TIM_MatchConfigStruct.IntOnMatch = ENABLE;
TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
TIM_MatchConfigStruct.StopOnMatch = DISABLE;
TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
TIM_MatchConfigStruct.MatchValue = 10000;  // 1 segundo
TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);

// 4. Configurar NVIC
NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
NVIC_EnableIRQ(TIMER0_IRQn);

// 5. Iniciar Timer
TIM_Cmd(LPC_TIM0, ENABLE);

// 6. ISR
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Procesar evento
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}
```

### 2. Capture con Interrupción (Medición)

```c
// 1. Configurar PINSEL para CAP
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 3;
PinCfg.Portnum = 1;
PinCfg.Pinnum = 26;  // CAP0.0
PINSEL_ConfigPin(&PinCfg);

// 2. Configurar e inicializar Timer
TIM_TIMERCFG_Type TIM_ConfigStruct;
TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
TIM_ConfigStruct.PrescaleValue = 1;  // 1us por tick
TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);

// 3. Configurar Capture
TIM_CAPTURECFG_Type TIM_CaptureConfigStruct;
TIM_CaptureConfigStruct.CaptureChannel = 0;
TIM_CaptureConfigStruct.RisingEdge = ENABLE;
TIM_CaptureConfigStruct.FallingEdge = DISABLE;
TIM_CaptureConfigStruct.IntOnCaption = ENABLE;
TIM_ConfigCapture(LPC_TIM0, &TIM_CaptureConfigStruct);

// 4. Configurar NVIC
NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
NVIC_EnableIRQ(TIMER0_IRQn);

// 5. Iniciar Timer
TIM_ResetCounter(LPC_TIM0);
TIM_Cmd(LPC_TIM0, ENABLE);

// 6. ISR
volatile uint32_t captured = 0;
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
        captured = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
        TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT);
    }
}
```

---

## Notas Importantes sobre LPC1769

1. **PCLK Configuration:** El driver `TIM_Init()` configura automáticamente el PCLK divisor a CCLK/4 (verificado en lpc17xx_timer.c:299-316). Si necesitas otra configuración, usa `CLKPWR_SetPCLKDiv()` después de `TIM_Init()`.

2. **Vectores de Interrupción:**
   - TIMER0: `TIMER0_IRQn` → `TIMER0_IRQHandler()`
   - TIMER1: `TIMER1_IRQn` → `TIMER1_IRQHandler()`
   - TIMER2: `TIMER2_IRQn` → `TIMER2_IRQHandler()`
   - TIMER3: `TIMER3_IRQn` → `TIMER3_IRQHandler()`

3. **Reset Counter:** `TIM_ResetCounter()` sincroniza el reset con el próximo flanco de PCLK (lpc17xx_timer.c:423).

4. **Match con Reset:** Cuando `ResetOnMatch = ENABLE`, el contador se resetea en el **próximo tick** después del match, no instantáneamente.

5. **External Match:** Solo funciona si el pin está configurado como MATn.x mediante PINSEL.

6. **Capture Registers:** Son de solo lectura. El valor se carga automáticamente en el evento de captura.

7. **Prescale con USVAL:** El driver calcula automáticamente el valor de PR basado en el PCLK actual (lpc17xx_timer.c:85-93).

---

## Referencias

- **Driver Header:** `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_timer.h`
- **Driver Source:** `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_timer.c`
- **Ejemplos verificados:**
  - `library/examples/TIMER/Interrupt_Match/timer_int_match.c`
  - `library/examples/TIMER/Polling_Match/timer_poll_match.c`
  - `library/examples/TIMER/PWMSignal/pwm_signal.c`
  - `library/examples/TIMER/FreqMeasure/freqmeasure.c`
  - `library/examples/TIMER/Capture/timer_capture.c`
  - `library/examples/TIMER/Gen_Diff_Delay/gen_diff_delay.c`
- **Manual del usuario:** LPC17xx User Manual (UM10360), Capítulo 21: Timer/Counter
- **Pin functions:** Tabla 8.5 del manual del usuario LPC17xx

---

## Resumen

Los **Timers del LPC1769** son periféricos extremadamente versátiles:

- **Match channels** permiten generar eventos periódicos, delays, señales PWM por software, y controlar salidas externas
- **Capture channels** permiten medir frecuencias, anchos de pulso, y capturar timestamps de eventos
- **Counter mode** permite contar eventos externos
- **4 timers idénticos** ofrecen gran flexibilidad

**Puntos clave:**
1. Siempre configurar PINSEL para pines MAT/CAP
2. Elegir prescaler apropiado para la resolución deseada
3. En ISR, siempre limpiar flags con `TIM_ClearIntPending()`
4. Marcar variables compartidas ISR-main como `volatile`
5. Habilitar NVIC con `NVIC_EnableIRQ()` para interrupciones

Este driver es fundamental para aplicaciones que requieren control preciso de tiempo y medición de señales externas.
