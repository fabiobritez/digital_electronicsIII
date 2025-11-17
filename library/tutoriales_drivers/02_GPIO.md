# Tutorial GPIO - General Purpose Input/Output (LPC1769)

## Índice
1. [Introducción](#introducción)
2. [¿Qué es GPIO?](#qué-es-gpio)
3. [Conceptos fundamentales](#conceptos-fundamentales)
4. [Funciones del driver](#funciones-del-driver)
5. [Ejemplos prácticos](#ejemplos-prácticos)
6. [Interrupciones GPIO](#interrupciones-gpio)
7. [Casos de uso comunes](#casos-de-uso-comunes)
8. [Errores comunes](#errores-comunes)
9. [Registro de hardware](#registro-de-hardware)
10. [Ejercicios propuestos](#ejercicios-propuestos)

---

## Introducción

**GPIO** (General Purpose Input/Output) es el periférico más básico y fundamental de cualquier microcontrolador. Permite usar los pines como:
- **Entradas digitales**: Leer el estado de botones, sensores, señales digitales
- **Salidas digitales**: Controlar LEDs, relés, señales de control

> ⚠️ **IMPORTANTE**: Antes de usar GPIO, **siempre debes configurar PINSEL** para establecer el pin en función GPIO (FUNC_0).

---

## ¿Qué es GPIO?

GPIO permite controlar individualmente los pines del microcontrolador como entradas o salidas digitales simples.

### Características del GPIO en LPC1769

- **5 puertos** (PORT0, PORT1, PORT2, PORT3, PORT4)
- Hasta **32 pines por puerto** (no todos están disponibles físicamente)
- **Operaciones atómicas** con registros SET y CLR
- **Interrupciones** por flanco (rising/falling edge)
- **Acceso rápido** mediante Fast GPIO (FIO)
- **Máscara programable** para proteger pines

---

## Conceptos fundamentales

### 1. Dirección del pin (DIR)

Cada pin debe configurarse como **entrada** o **salida**:

| Dirección | Valor | Uso típico |
|-----------|-------|------------|
| Entrada | 0 | Leer botones, sensores, señales externas |
| Salida | 1 | Controlar LEDs, relés, señales de control |

```c
// Configurar dirección
GPIO_SetDir(portNum, bitValue, dir);
//          puerto    máscara    0=entrada, 1=salida
```

---

### 2. Valor del pin (PIN/SET/CLR)

Para **salidas**, controlas el estado del pin:
- **SET**: Poner pin en alto (1, ~3.3V)
- **CLR**: Poner pin en bajo (0, 0V)

Para **entradas**, lees el estado actual del pin.

---

### 3. Máscaras de bits

Los pines se controlan usando **máscaras** de 32 bits:

```c
// Ejemplos de máscaras
#define PIN_0    (1 << 0)    // 0x00000001
#define PIN_5    (1 << 5)    // 0x00000020
#define PIN_22   (1 << 22)   // 0x00400000

// Múltiples pines
#define PINS_0_1_2  ((1<<0) | (1<<1) | (1<<2))  // 0x00000007
```

---

### 4. GPIO vs FIO (Fast GPIO)

El LPC1769 tiene **dos formas** de acceder a GPIO:

| Tipo | Velocidad | Uso recomendado |
|------|-----------|-----------------|
| **GPIO** | Más lento | Compatibilidad con código legacy |
| **FIO** (Fast GPIO) | **Más rápido** | **Siempre preferir en LPC1769** |

> En este tutorial usaremos las funciones **GPIO** que internamente usan FIO en el LPC1769.

---

## Funciones del driver

### 1. Configurar dirección: `GPIO_SetDir()`

```c
void GPIO_SetDir(uint8_t portNum, uint32_t bitValue, uint8_t dir);
```

**Parámetros:**
- `portNum`: Número de puerto (0-4)
- `bitValue`: Máscara de bits de los pines a configurar
- `dir`: Dirección (0 = entrada, 1 = salida)

**Ejemplo:**
```c
// P0.22 como salida
GPIO_SetDir(0, (1<<22), 1);

// P2.0 y P2.1 como entradas
GPIO_SetDir(2, (1<<0)|(1<<1), 0);
```

---

### 2. Escribir en pins (poner en alto): `GPIO_SetValue()`

```c
void GPIO_SetValue(uint8_t portNum, uint32_t bitValue);
```

Pone los pines especificados en **alto** (1, ~3.3V).

**Ejemplo:**
```c
// Encender LED en P0.22
GPIO_SetValue(0, (1<<22));
```

---

### 3. Limpiar pins (poner en bajo): `GPIO_ClearValue()`

```c
void GPIO_ClearValue(uint8_t portNum, uint32_t bitValue);
```

Pone los pines especificados en **bajo** (0, 0V).

**Ejemplo:**
```c
// Apagar LED en P0.22
GPIO_ClearValue(0, (1<<22));
```

---

### 4. Leer estado del puerto: `GPIO_ReadValue()`

```c
uint32_t GPIO_ReadValue(uint8_t portNum);
```

Lee el estado de **todos los pines** del puerto.

**Retorno:** Valor de 32 bits con el estado de cada pin

**Ejemplo:**
```c
uint32_t estado = GPIO_ReadValue(0);

// Verificar si P0.10 está en alto
if (estado & (1<<10)) {
    // Pin en alto
}
```

---

### 5. Interrupciones GPIO

```c
// Habilitar interrupción
void GPIO_IntCmd(uint8_t portNum, uint32_t bitValue, uint8_t edgeState);

// Verificar estado de interrupción
FunctionalState GPIO_GetIntStatus(uint8_t portNum, uint32_t pinNum, uint8_t edgeState);

// Limpiar bandera de interrupción
void GPIO_ClearInt(uint8_t portNum, uint32_t bitValue);
```

**Estados de flanco (edgeState):**
- `0`: Flanco descendente (falling edge)
- `1`: Flanco ascendente (rising edge)

---

## Ejemplos prácticos

### Ejemplo 1: LED simple (parpadeo)

```c
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define LED_PIN     (1 << 22)   // P0.22
#define LED_PORT    0

void delay_ms(uint32_t ms) {
    // Delay simple (NO usar en producción, usar SysTick)
    for (uint32_t i = 0; i < ms * 10000; i++) {
        __asm("nop");
    }
}

void init_led(void) {
    PINSEL_CFG_Type pin_cfg;

    // 1. Configurar PINSEL (P0.22 como GPIO)
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_22;
    pin_cfg.Funcnum = PINSEL_FUNC_0;  // GPIO
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // 2. Configurar dirección (salida)
    GPIO_SetDir(LED_PORT, LED_PIN, 1);

    // 3. Apagar LED inicialmente
    GPIO_ClearValue(LED_PORT, LED_PIN);
}

int main(void) {
    init_led();

    while (1) {
        GPIO_SetValue(LED_PORT, LED_PIN);     // Encender
        delay_ms(500);
        GPIO_ClearValue(LED_PORT, LED_PIN);   // Apagar
        delay_ms(500);
    }
}
```

---

### Ejemplo 2: Leer botón con pull-up

```c
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define BUTTON_PIN     (1 << 10)   // P2.10
#define BUTTON_PORT    2
#define LED_PIN        (1 << 22)   // P0.22
#define LED_PORT       0

void init_button(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P2.10 como GPIO con pull-up
    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Pinnum = PINSEL_PIN_10;
    pin_cfg.Funcnum = PINSEL_FUNC_0;
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;  // Pull-up interno
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar como entrada
    GPIO_SetDir(BUTTON_PORT, BUTTON_PIN, 0);
}

uint8_t leer_boton(void) {
    uint32_t estado = GPIO_ReadValue(BUTTON_PORT);

    // Botón presionado conecta a GND → lee 0
    if (estado & BUTTON_PIN) {
        return 0;  // No presionado (pull-up → alto)
    } else {
        return 1;  // Presionado (conectado a GND → bajo)
    }
}

int main(void) {
    init_led();
    init_button();

    while (1) {
        if (leer_boton()) {
            GPIO_SetValue(LED_PORT, LED_PIN);   // Encender si presionado
        } else {
            GPIO_ClearValue(LED_PORT, LED_PIN); // Apagar si no presionado
        }
    }
}
```

---

### Ejemplo 3: Control de múltiples LEDs

```c
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

// LEDs en P1: pines 28, 29, 31
#define LED1    (1 << 28)
#define LED2    (1 << 29)
#define LED3    (1UL << 31)  // UL para evitar overflow en bit 31
#define ALL_LEDS (LED1 | LED2 | LED3)

void init_leds(void) {
    PINSEL_CFG_Type pin_cfg;

    pin_cfg.Portnum = PINSEL_PORT_1;
    pin_cfg.Funcnum = PINSEL_FUNC_0;
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    // Configurar pines 28, 29, 31 como GPIO
    for (uint8_t pin = 28; pin <= 31; pin++) {
        if (pin == 30) continue;  // Saltar pin 30
        pin_cfg.Pinnum = pin;
        PINSEL_ConfigPin(&pin_cfg);
    }

    // Todos como salida
    GPIO_SetDir(1, ALL_LEDS, 1);

    // Apagar todos
    GPIO_ClearValue(1, ALL_LEDS);
}

void secuencia_leds(void) {
    // Encender uno por uno
    GPIO_SetValue(1, LED1);
    delay_ms(200);
    GPIO_SetValue(1, LED2);
    delay_ms(200);
    GPIO_SetValue(1, LED3);
    delay_ms(200);

    // Apagar todos
    GPIO_ClearValue(1, ALL_LEDS);
    delay_ms(200);
}

int main(void) {
    init_leds();

    while (1) {
        secuencia_leds();
    }
}
```

---

### Ejemplo 4: Lectura de bus de datos de 8 bits

```c
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define DATA_BUS_MASK   0x000000FF  // P2.0 - P2.7
#define DATA_PORT       2

void init_data_bus_input(void) {
    PINSEL_CFG_Type pin_cfg;

    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Funcnum = PINSEL_FUNC_0;
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    // Configurar P2.0 a P2.7 como GPIO
    for (uint8_t i = 0; i < 8; i++) {
        pin_cfg.Pinnum = i;
        PINSEL_ConfigPin(&pin_cfg);
    }

    // Configurar como entrada
    GPIO_SetDir(DATA_PORT, DATA_BUS_MASK, 0);
}

uint8_t read_data_bus(void) {
    uint32_t value = GPIO_ReadValue(DATA_PORT);
    return (uint8_t)(value & DATA_BUS_MASK);
}

int main(void) {
    init_data_bus_input();

    while (1) {
        uint8_t datos = read_data_bus();
        // Procesar datos...
    }
}
```

---

### Ejemplo 5: Toggle (invertir estado)

```c
#include "lpc17xx_gpio.h"

#define LED_PIN     (1 << 22)
#define LED_PORT    0

void toggle_led(void) {
    uint32_t estado = GPIO_ReadValue(LED_PORT);

    if (estado & LED_PIN) {
        // Si está encendido, apagar
        GPIO_ClearValue(LED_PORT, LED_PIN);
    } else {
        // Si está apagado, encender
        GPIO_SetValue(LED_PORT, LED_PIN);
    }
}

int main(void) {
    init_led();

    while (1) {
        toggle_led();
        delay_ms(500);
    }
}
```

---

## Interrupciones GPIO

Las interrupciones GPIO permiten reaccionar a eventos en pines sin polling constante.

### Configuración de interrupciones

```c
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define BUTTON_PIN     (1 << 10)
#define BUTTON_PORT    2

volatile uint8_t button_pressed = 0;

void init_button_interrupt(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar pin
    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Pinnum = PINSEL_PIN_10;
    pin_cfg.Funcnum = PINSEL_FUNC_0;
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // Entrada
    GPIO_SetDir(BUTTON_PORT, BUTTON_PIN, 0);

    // Habilitar interrupción por flanco descendente
    GPIO_IntCmd(BUTTON_PORT, BUTTON_PIN, 0);  // 0 = falling edge

    // Limpiar bandera
    GPIO_ClearInt(BUTTON_PORT, BUTTON_PIN);

    // Habilitar interrupción en NVIC
    NVIC_EnableIRQ(EINT3_IRQn);  // EINT3 para GPIO Port 2
}

// Handler de interrupción
void EINT3_IRQHandler(void) {
    // Verificar si fue el botón
    if (GPIO_GetIntStatus(BUTTON_PORT, 10, 0)) {
        button_pressed = 1;

        // Limpiar bandera
        GPIO_ClearInt(BUTTON_PORT, BUTTON_PIN);
    }
}

int main(void) {
    init_led();
    init_button_interrupt();

    while (1) {
        if (button_pressed) {
            button_pressed = 0;
            toggle_led();
        }
    }
}
```

### Vectores de interrupción GPIO

| Puerto | Vector de interrupción |
|--------|------------------------|
| PORT0 | `EINT3_IRQn` |
| PORT2 | `EINT3_IRQn` |
| Otros | `EINT3_IRQn` |

> Todos los puertos GPIO comparten el mismo vector `EINT3`, por lo que debes verificar qué pin generó la interrupción.

---

## Casos de uso comunes

### Caso 1: Semáforo con LEDs

```c
#define LED_ROJO    (1 << 0)  // P2.0
#define LED_AMARILLO (1 << 1)  // P2.1
#define LED_VERDE   (1 << 2)  // P2.2
#define LED_PORT    2

void semaforo(void) {
    // Verde
    GPIO_SetValue(LED_PORT, LED_VERDE);
    delay_ms(5000);
    GPIO_ClearValue(LED_PORT, LED_VERDE);

    // Amarillo
    GPIO_SetValue(LED_PORT, LED_AMARILLO);
    delay_ms(2000);
    GPIO_ClearValue(LED_PORT, LED_AMARILLO);

    // Rojo
    GPIO_SetValue(LED_PORT, LED_ROJO);
    delay_ms(5000);
    GPIO_ClearValue(LED_PORT, LED_ROJO);
}
```

---

### Caso 2: Teclado matricial 4x4

```c
// Filas como salidas (P2.0-P2.3)
#define ROWS_MASK   0x0000000F
// Columnas como entradas (P2.4-P2.7)
#define COLS_MASK   0x000000F0

void init_keypad(void) {
    // Configurar PINSEL...

    // Filas como salidas
    GPIO_SetDir(2, ROWS_MASK, 1);

    // Columnas como entradas con pull-up
    GPIO_SetDir(2, COLS_MASK, 0);
}

uint8_t scan_keypad(void) {
    for (uint8_t row = 0; row < 4; row++) {
        // Activar fila (poner en bajo)
        GPIO_ClearValue(2, ROWS_MASK);
        GPIO_SetValue(2, ~(1 << row) & ROWS_MASK);

        // Leer columnas
        uint32_t cols = GPIO_ReadValue(2);

        for (uint8_t col = 0; col < 4; col++) {
            if (!(cols & (1 << (col + 4)))) {
                // Tecla presionada
                return (row * 4) + col;
            }
        }
    }
    return 0xFF;  // Ninguna tecla
}
```

---

### Caso 3: Display 7 segmentos

```c
// Segmentos a-g conectados a P2.0-P2.6
#define SEG_PORT    2

const uint8_t digitos[10] = {
    0x3F, // 0: a,b,c,d,e,f
    0x06, // 1: b,c
    0x5B, // 2: a,b,d,e,g
    0x4F, // 3: a,b,c,d,g
    0x66, // 4: b,c,f,g
    0x6D, // 5: a,c,d,f,g
    0x7D, // 6: a,c,d,e,f,g
    0x07, // 7: a,b,c
    0x7F, // 8: todos
    0x6F  // 9: a,b,c,d,f,g
};

void mostrar_digito(uint8_t num) {
    if (num > 9) return;

    GPIO_ClearValue(SEG_PORT, 0x7F);  // Apagar todos
    GPIO_SetValue(SEG_PORT, digitos[num]);
}
```

---

## Errores comunes

### ❌ Error 1: No configurar dirección

```c
// INCORRECTO - Falta SetDir
GPIO_SetValue(0, (1<<22));  // ❌ Pin puede estar como entrada

// CORRECTO
GPIO_SetDir(0, (1<<22), 1);  // ✅ Configurar como salida primero
GPIO_SetValue(0, (1<<22));
```

---

### ❌ Error 2: Olvidar PINSEL

```c
// INCORRECTO - Falta PINSEL
GPIO_SetDir(0, (1<<22), 1);  // ❌ Pin puede tener otra función

// CORRECTO
PINSEL_ConfigPin(&pin_cfg);  // ✅ Primero PINSEL
GPIO_SetDir(0, (1<<22), 1);
```

---

### ❌ Error 3: Máscara incorrecta

```c
// INCORRECTO - Confundir número de pin con máscara
GPIO_SetValue(0, 22);  // ❌ Activa pines 1, 2 y 4 (0x16)

// CORRECTO
GPIO_SetValue(0, (1<<22));  // ✅ Solo pin 22
```

---

### ❌ Error 4: No limpiar bandera de interrupción

```c
void EINT3_IRQHandler(void) {
    // INCORRECTO - Falta limpiar bandera
    button_pressed = 1;  // ❌ Interrupción se dispara infinitamente
}

// CORRECTO
void EINT3_IRQHandler(void) {
    button_pressed = 1;
    GPIO_ClearInt(2, (1<<10));  // ✅ Limpiar bandera
}
```

---

### ❌ Error 5: Desbordamiento en bit 31

```c
// INCORRECTO - Overflow en bit 31
#define LED (1 << 31)  // ❌ Overflow si int es 32 bits con signo

// CORRECTO
#define LED (1UL << 31)  // ✅ Usar UL para unsigned long
```

---

## Registro de hardware

### Registros principales del GPIO

| Registro | Función |
|----------|---------|
| `FIODIR` | Dirección de pines (0=entrada, 1=salida) |
| `FIOPIN` | Lectura/escritura del puerto completo |
| `FIOSET` | Poner pines en alto (solo escritura) |
| `FIOCLR` | Poner pines en bajo (solo escritura) |
| `FIOMASK` | Máscara para proteger pines |

### Ejemplo de acceso directo (para referencia)

```c
// Esto lo hace automáticamente GPIO_SetValue()
LPC_GPIO0->FIOSET = (1 << 22);  // Poner P0.22 en alto
LPC_GPIO0->FIOCLR = (1 << 22);  // Poner P0.22 en bajo
```

> **No es necesario** acceder directamente a los registros. Usa siempre las funciones del driver.

---

## Ejercicios propuestos

### Ejercicio 1: Contador binario

Implementa un contador de 0 a 15 mostrando el valor en 4 LEDs conectados a P2.0-P2.3.

```c
void contador_binario(void) {
    // TU CÓDIGO AQUÍ
}
```

---

### Ejercicio 2: Botón con antirrebote

Implementa una función que lea un botón con antirrebote por software (debouncing).

```c
uint8_t leer_boton_debounce(void) {
    // TU CÓDIGO AQUÍ
}
```

---

### Ejercicio 3: Secuencia de LEDs tipo "auto fantástico"

Crea una secuencia donde 8 LEDs se encienden de izquierda a derecha y luego de derecha a izquierda.

---

### Ejercicio 4: Control de motor paso a paso

Implementa el control de un motor paso a paso usando 4 pines GPIO para las bobinas.

---

## Resumen

### Puntos clave

1. **PINSEL primero**, luego GPIO
2. **SetDir() define entrada/salida**
3. **SetValue() pone en alto, ClearValue() en bajo**
4. **ReadValue() lee el estado del puerto completo**
5. **Usa máscaras** (1 << pin) para operaciones con bits
6. **UL para bit 31** para evitar overflow
7. **Interrupciones comparten EINT3**
8. **Limpia bandera** en el handler de interrupción

### Flujo típico

```c
// 1. Configurar PINSEL
PINSEL_ConfigPin(&pin_cfg);

// 2. Configurar dirección
GPIO_SetDir(port, pin_mask, dir);

// 3. Salidas: escribir valores
GPIO_SetValue(port, pin_mask);
GPIO_ClearValue(port, pin_mask);

// 4. Entradas: leer valores
uint32_t state = GPIO_ReadValue(port);
```

---

## Referencias

- **Manual del usuario LPC17xx**: Capítulo 9 - General Purpose Input/Output (GPIO)
- **Tutorial anterior**: [PINSEL - Pin Select](./01_PINSEL.md)
- **Siguiente tutorial**: [SYSTICK - System Timer](./03_SYSTICK.md)

---
