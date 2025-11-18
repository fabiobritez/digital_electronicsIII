# Tutorial SYSTICK - System Timer (LPC1769)

## Índice
1. [Introducción](#introducción)
2. [¿Qué es SysTick?](#qué-es-systick)
3. [Características del SysTick](#características-del-systick)
4. [Funciones del driver](#funciones-del-driver)
5. [Ejemplos prácticos](#ejemplos-prácticos)
6. [Casos de uso comunes](#casos-de-uso-comunes)
7. [Errores comunes](#errores-comunes)
8. [Registros de hardware](#registros-de-hardware)
9. [Ejercicios propuestos](#ejercicios-propuestos)

---

## Introducción

**SysTick** (System Tick Timer) es un temporizador de **24 bits** que forma parte del núcleo **ARM Cortex-M3**. Está diseñado específicamente para generar **interrupciones periódicas** a intervalos precisos.

### Usos típicos

- **Base de tiempo** para el sistema operativo (RTOS)
- **Delays** precisos (retardos)
- **Polling periódico** de sensores
- **Generación de eventos** a intervalos regulares
- **Timeouts** para comunicaciones

> ⚠️ **Importante**: SysTick es parte del Cortex-M3, NO es un periférico del LPC1769. Está disponible en **todos** los microcontroladores Cortex-M3/M4.

---

## ¿Qué es SysTick?

SysTick es un **contador descendente** de 24 bits que:

1. **Cuenta hacia abajo** desde un valor de recarga (RELOAD)
2. Cuando llega a **0**, genera una **interrupción**
3. Se **recarga automáticamente** y vuelve a contar

### Diagrama conceptual

```
RELOAD = 999  ─┐
               │
               ▼
    999 → 998 → 997 → ... → 2 → 1 → 0 → ¡Interrupción!
                                        │
                                        └─> Recarga a 999
```

---

## Características del SysTick

### Características generales

- **Contador de 24 bits** (rango: 0 a 16,777,215)
- **Dos fuentes de reloj**:
  - Reloj interno de la CPU (SystemCoreClock)
  - Reloj externo STCLK (pin P3.26)
- **Interrupción dedicada** con prioridad configurable
- **Auto-recarga** automática
- **Registro de control** simple

### Limitaciones importantes

| Característica | Valor |
|----------------|-------|
| **Bits del contador** | 24 bits (0x00FFFFFF) |
| **Valor máximo RELOAD** | 16,777,215 |
| **Tiempo máximo** | Depende del reloj |
| **Precisión** | 1 tick de reloj |

### Cálculo del tiempo máximo

Con SystemCoreClock = 100 MHz:

```c
Tiempo_max = (2^24 - 1) / (100,000,000) = 0.167 segundos ≈ 167 ms
```

---

## Funciones del driver

El driver CMSIS proporciona las siguientes funciones:

### 1. Inicialización con reloj interno: `SYSTICK_InternalInit()`

```c
void SYSTICK_InternalInit(uint32_t time);
```

Configura SysTick usando el **reloj de la CPU** (SystemCoreClock).

**Parámetros:**
- `time`: Intervalo de tiempo en **milisegundos**

**Funcionamiento interno:**
```c
RELOAD = (SystemCoreClock / 1000) * time - 1
```

**Ejemplo:**
```c
// Interrupción cada 10 ms
SYSTICK_InternalInit(10);
```

**Tiempo máximo permitido:**
```c
tiempo_max = (2^24) / (SystemCoreClock / 1000)  // en milisegundos
```

> ⚠️ Si el tiempo solicitado excede el máximo, la función entra en un **bucle infinito** (error fatal).

---

### 2. Inicialización con reloj externo: `SYSTICK_ExternalInit()`

```c
void SYSTICK_ExternalInit(uint32_t freq, uint32_t time);
```

Configura SysTick usando un **reloj externo** (STCLK en pin P3.26).

**Parámetros:**
- `freq`: Frecuencia del reloj externo en **Hz**
- `time`: Intervalo de tiempo en **milisegundos**

**Ejemplo:**
```c
// Reloj externo de 5 kHz, interrupción cada 10 ms
SYSTICK_ExternalInit(5000, 10);
```

> **Nota**: Debes configurar P3.26 como STCLK usando PINSEL antes de llamar esta función.

---

### 3. Habilitar/deshabilitar contador: `SYSTICK_Cmd()`

```c
void SYSTICK_Cmd(FunctionalState NewState);
```

Inicia o detiene el contador SysTick.

**Parámetros:**
- `NewState`: `ENABLE` o `DISABLE`

**Ejemplo:**
```c
SYSTICK_Cmd(ENABLE);   // Iniciar el contador
SYSTICK_Cmd(DISABLE);  // Detener el contador
```

---

### 4. Habilitar/deshabilitar interrupción: `SYSTICK_IntCmd()`

```c
void SYSTICK_IntCmd(FunctionalState NewState);
```

Habilita o deshabilita la **interrupción** de SysTick.

**Ejemplo:**
```c
SYSTICK_IntCmd(ENABLE);   // Habilitar interrupción
SYSTICK_IntCmd(DISABLE);  // Deshabilitar interrupción
```

---

### 5. Leer valor actual: `SYSTICK_GetCurrentValue()`

```c
uint32_t SYSTICK_GetCurrentValue(void);
```

Lee el valor **actual** del contador (cuenta regresiva).

**Retorno:** Valor de 24 bits del contador

**Ejemplo:**
```c
uint32_t valor_actual = SYSTICK_GetCurrentValue();
```

---

### 6. Limpiar bandera: `SYSTICK_ClearCounterFlag()`

```c
void SYSTICK_ClearCounterFlag(void);
```

Limpia la bandera COUNTFLAG.

**Ejemplo:**
```c
void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();  // Limpiar bandera
    // Tu código aquí...
}
```

---

### 7. Función CMSIS estándar: `SysTick_Config()`

Esta es la función **estándar de CMSIS**, disponible en todos los Cortex-M:

```c
uint32_t SysTick_Config(uint32_t ticks);
```

Configura SysTick en **un solo paso**:
- Establece el valor de recarga
- Habilita el contador
- Habilita la interrupción
- Usa el reloj de la CPU

**Parámetros:**
- `ticks`: Número de ticks del reloj entre interrupciones

**Retorno:**
- `0`: Éxito
- `1`: Error (ticks > 0xFFFFFF)

**Ejemplo:**
```c
// Interrupción cada 1 ms (asumiendo SystemCoreClock = 100 MHz)
SysTick_Config(SystemCoreClock / 1000);
```

---

## Ejemplos prácticos

### Ejemplo 1: Interrupción cada 10 ms (reloj interno)

```c
#include "lpc17xx_systick.h"
#include "lpc17xx_gpio.h"

volatile uint32_t contador_ms = 0;

// Handler de interrupción SysTick
void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    contador_ms += 10;  // Incrementar cada 10 ms
}

int main(void) {
    // Inicializar SysTick para 10 ms
    SYSTICK_InternalInit(10);

    // Habilitar interrupción
    SYSTICK_IntCmd(ENABLE);

    // Habilitar contador
    SYSTICK_Cmd(ENABLE);

    while (1) {
        // El contador se incrementa automáticamente
        // cada 10 ms en la interrupción
    }
}
```

---

### Ejemplo 2: LED parpadeante con SysTick

```c
#include "lpc17xx_systick.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define LED_PIN (1 << 22)  // P0.22

volatile uint8_t toggle = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();

    if (toggle) {
        GPIO_SetValue(0, LED_PIN);
    } else {
        GPIO_ClearValue(0, LED_PIN);
    }
    toggle = !toggle;
}

int main(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P0.22 como GPIO
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_22;
    pin_cfg.Funcnum = PINSEL_FUNC_0;
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar como salida
    GPIO_SetDir(0, LED_PIN, 1);

    // SysTick cada 500 ms
    SYSTICK_InternalInit(500);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1) {
        // LED parpadea automáticamente cada 500 ms
    }
}
```

---

### Ejemplo 3: Delay no bloqueante

```c
#include "lpc17xx_systick.h"

volatile uint32_t ticks_ms = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    ticks_ms++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ticks_ms;
    while ((ticks_ms - start) < ms);
}

int main(void) {
    // SysTick cada 1 ms
    SYSTICK_InternalInit(1);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1) {
        GPIO_SetValue(0, (1<<22));
        delay_ms(1000);  // Esperar 1 segundo

        GPIO_ClearValue(0, (1<<22));
        delay_ms(1000);  // Esperar 1 segundo
    }
}
```

---

### Ejemplo 4: Uso de SysTick_Config() (CMSIS estándar)

```c
#include "LPC17xx.h"

volatile uint32_t contador_1ms = 0;

void SysTick_Handler(void) {
    // NO necesita limpiar bandera (CMSIS lo hace automáticamente)
    contador_1ms++;
}

int main(void) {
    // Configurar SysTick para 1 ms
    // SystemCoreClock es definido por CMSIS (frecuencia del CPU)
    SysTick_Config(SystemCoreClock / 1000);

    while (1) {
        // contador_1ms se incrementa cada 1 ms
    }
}
```

---

### Ejemplo 5: Reloj externo STCLK

```c
#include "lpc17xx_systick.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    // Toggle P0.0
    static uint8_t state = 0;
    if (state) {
        GPIO_SetValue(0, (1<<0));
    } else {
        GPIO_ClearValue(0, (1<<0));
    }
    state = !state;
}

int main(void) {
    PINSEL_CFG_Type pin_cfg;
    TIM_TIMERCFG_Type tim_cfg;
    TIM_MATCHCFG_Type match_cfg;

    // Configurar P3.26 como STCLK (Función 1)
    pin_cfg.Portnum = PINSEL_PORT_3;
    pin_cfg.Pinnum = PINSEL_PIN_26;
    pin_cfg.Funcnum = PINSEL_FUNC_1;  // STCLK
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar Timer0 para generar 10 kHz en MAT0.0
    // que se conecta a STCLK internamente
    tim_cfg.PrescaleOption = TIM_PRESCALE_USVAL;
    tim_cfg.PrescaleValue = 10;  // 10 µs

    match_cfg.MatchChannel = 0;
    match_cfg.IntOnMatch = FALSE;
    match_cfg.ResetOnMatch = TRUE;
    match_cfg.StopOnMatch = FALSE;
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    match_cfg.MatchValue = 10;  // 10 * 10µs = 100µs → 10 kHz

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &tim_cfg);
    TIM_ConfigMatch(LPC_TIM0, &match_cfg);
    TIM_Cmd(LPC_TIM0, ENABLE);

    // SysTick con reloj externo de 5 kHz (10kHz/2)
    // Interrupción cada 10 ms
    SYSTICK_ExternalInit(5000, 10);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1);
}
```

---

## Casos de uso comunes

### Caso 1: Sistema de tiempo real simple

```c
typedef struct {
    uint32_t horas;
    uint32_t minutos;
    uint32_t segundos;
    uint32_t milisegundos;
} RTC_Time;

volatile RTC_Time tiempo_actual = {0, 0, 0, 0};

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();

    tiempo_actual.milisegundos++;

    if (tiempo_actual.milisegundos >= 1000) {
        tiempo_actual.milisegundos = 0;
        tiempo_actual.segundos++;

        if (tiempo_actual.segundos >= 60) {
            tiempo_actual.segundos = 0;
            tiempo_actual.minutos++;

            if (tiempo_actual.minutos >= 60) {
                tiempo_actual.minutos = 0;
                tiempo_actual.horas++;

                if (tiempo_actual.horas >= 24) {
                    tiempo_actual.horas = 0;
                }
            }
        }
    }
}

int main(void) {
    // SysTick cada 1 ms
    SYSTICK_InternalInit(1);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1) {
        // Usar tiempo_actual para lo que necesites
    }
}
```

---

### Caso 2: Muestreo periódico de sensor

```c
#define SAMPLE_INTERVAL_MS  100  // Muestrear cada 100 ms

volatile uint8_t flag_muestreo = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    flag_muestreo = 1;  // Señalizar que hay que muestrear
}

int main(void) {
    // Inicializar ADC, etc.

    // SysTick cada 100 ms
    SYSTICK_InternalInit(SAMPLE_INTERVAL_MS);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1) {
        if (flag_muestreo) {
            flag_muestreo = 0;

            // Leer sensor
            uint16_t valor = ADC_Read(0);

            // Procesar datos
            procesar_muestra(valor);
        }
    }
}
```

---

### Caso 3: Timeout para comunicación UART

```c
#define TIMEOUT_MS  1000  // 1 segundo

volatile uint32_t timeout_counter = 0;
volatile uint8_t timeout_flag = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();

    if (timeout_counter > 0) {
        timeout_counter--;
        if (timeout_counter == 0) {
            timeout_flag = 1;  // Timeout!
        }
    }
}

void iniciar_timeout(uint32_t ms) {
    timeout_counter = ms;
    timeout_flag = 0;
}

int main(void) {
    // SysTick cada 1 ms
    SYSTICK_InternalInit(1);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    while (1) {
        // Iniciar timeout de 1 segundo
        iniciar_timeout(TIMEOUT_MS);

        // Esperar respuesta UART
        while (!UART_DataReady() && !timeout_flag);

        if (timeout_flag) {
            // Manejo de timeout
            error_comunicacion();
        } else {
            // Datos recibidos
            char dato = UART_Read();
        }
    }
}
```

---

## Errores comunes

### ❌ Error 1: Tiempo mayor al máximo permitido

```c
// INCORRECTO - Puede exceder 24 bits
SYSTICK_InternalInit(200);  // 200 ms puede ser demasiado

// Verificar tiempo máximo:
// tiempo_max = (2^24) / (SystemCoreClock / 1000)
```

**Solución**: Calcular el tiempo máximo según tu frecuencia de reloj.

---

### ❌ Error 2: Olvidar limpiar la bandera

```c
// INCORRECTO
void SysTick_Handler(void) {
    // ❌ Falta limpiar bandera
    contador++;
}

// CORRECTO
void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();  // ✅ Limpiar bandera
    contador++;
}
```

> **Nota**: Si usas `SysTick_Config()`, NO necesitas limpiar la bandera manualmente.

---

### ❌ Error 3: No habilitar contador o interrupción

```c
// INCORRECTO - Falta habilitar
SYSTICK_InternalInit(10);  // ❌ Solo configura, no inicia

// CORRECTO
SYSTICK_InternalInit(10);
SYSTICK_IntCmd(ENABLE);   // ✅ Habilitar interrupción
SYSTICK_Cmd(ENABLE);      // ✅ Habilitar contador
```

---

### ❌ Error 4: Variables no volátiles

```c
// INCORRECTO
uint32_t contador = 0;  // ❌ No es volatile

void SysTick_Handler(void) {
    contador++;
}

// CORRECTO
volatile uint32_t contador = 0;  // ✅ Volatile

void SysTick_Handler(void) {
    contador++;
}
```

> Variables modificadas en ISR **deben ser volatile**.

---

### ❌ Error 5: Código largo en el handler

```c
// INCORRECTO - Handler muy largo
void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();

    // ❌ Demasiado código en ISR
    procesar_datos_complejos();
    enviar_por_uart();
    actualizar_display();
}

// CORRECTO - Handler corto
volatile uint8_t flag_procesar = 0;

void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    flag_procesar = 1;  // ✅ Solo bandera
}

int main(void) {
    while (1) {
        if (flag_procesar) {
            flag_procesar = 0;
            procesar_datos_complejos();
        }
    }
}
```

---

## Registros de hardware

SysTick tiene 4 registros principales (parte de Cortex-M3):

### Registros SysTick

| Registro | Dirección | Descripción |
|----------|-----------|-------------|
| `CTRL` | 0xE000E010 | Control y estado |
| `LOAD` | 0xE000E014 | Valor de recarga (24 bits) |
| `VAL` | 0xE000E018 | Valor actual del contador |
| `CALIB` | 0xE000E01C | Calibración (solo lectura) |

### Registro CTRL (Control)

| Bit | Nombre | Descripción |
|-----|--------|-------------|
| 0 | ENABLE | 1 = Contador habilitado |
| 1 | TICKINT | 1 = Interrupción habilitada |
| 2 | CLKSOURCE | 0 = Externo (STCLK), 1 = CPU clock |
| 16 | COUNTFLAG | 1 = Contador llegó a 0 (auto-limpia al leer) |

---

## Ejercicios propuestos

### Ejercicio 1: Implementar un cronómetro

Crea un cronómetro que cuente décimas de segundo usando SysTick y muestre el tiempo por UART.

---

### Ejercicio 2: Anti-rebote por tiempo

Usa SysTick para implementar anti-rebote de 50 ms en un botón.

---

### Ejercicio 3: Parpadeo de 3 LEDs con diferentes frecuencias

- LED 1: 500 ms
- LED 2: 1000 ms
- LED 3: 250 ms

Usa un solo SysTick como base de tiempo.

---

### Ejercicio 4: Secuencia temporizada

Crea una secuencia de LEDs que se encienda paso a paso con intervalos de 200 ms.

---

## Resumen

### Puntos clave

1. **SysTick es parte del Cortex-M3**, no del LPC17xx específicamente
2. **Contador de 24 bits** (máximo 16,777,215)
3. **Dos fuentes de reloj**: interno (CPU) o externo (STCLK)
4. **Auto-recarga** automática
5. **Handler**: `SysTick_Handler()`
6. **Usar `volatile`** para variables compartidas con ISR
7. **Mantener el handler corto** (solo banderas)

### Funciones principales

| Función | Uso |
|---------|-----|
| `SYSTICK_InternalInit(ms)` | Configurar con reloj CPU |
| `SYSTICK_ExternalInit(freq, ms)` | Configurar con reloj externo |
| `SYSTICK_Cmd(ENABLE/DISABLE)` | Iniciar/detener contador |
| `SYSTICK_IntCmd(ENABLE/DISABLE)` | Habilitar/deshabilitar interrupción |
| `SysTick_Config(ticks)` | Función CMSIS estándar (todo en uno) |

### Flujo típico

```c
// 1. Inicializar SysTick
SYSTICK_InternalInit(10);  // 10 ms

// 2. Habilitar interrupción
SYSTICK_IntCmd(ENABLE);

// 3. Habilitar contador
SYSTICK_Cmd(ENABLE);

// 4. Implementar handler
void SysTick_Handler(void) {
    SYSTICK_ClearCounterFlag();
    // Tu código aquí
}
```

---

## Referencias

- **ARM Cortex-M3 Technical Reference Manual**: Capítulo SysTick Timer
- **Manual del usuario LPC17xx**: Sección de interrupciones
- **CMSIS Documentation**: SysTick functions

---

**Tutorial anterior**: [GPIO - General Purpose Input/Output](./02_GPIO.md)
**Siguiente tutorial**: [TIMER - Temporizadores de propósito general](./04_TIMER.md)
