# Tutorial PINSEL - Pin Select (LPC1769)

## Índice
1. [Introducción](#introducción)
2. [¿Qué es PINSEL?](#qué-es-pinsel)
3. [Conceptos fundamentales](#conceptos-fundamentales)
4. [Estructuras y funciones del driver](#estructuras-y-funciones-del-driver)
5. [Ejemplos prácticos](#ejemplos-prácticos)
6. [Casos de uso comunes](#casos-de-uso-comunes)
7. [Errores comunes](#errores-comunes)
8. [Registro de hardware](#registro-de-hardware)
9. [Ejercicios propuestos](#ejercicios-propuestos)

---

## Introducción

El módulo **PINSEL** (Pin Select) es el bloque de hardware responsable de configurar la **función** de cada pin del microcontrolador LPC1769.

### ¿Por qué es importante?

En el LPC1769, la mayoría de los pines son **multifunción**, es decir, pueden operar en diferentes modos:
- GPIO (entrada/salida digital simple)
- UART (comunicación serial)
- I2C (bus de comunicación)
- SPI (comunicación serial)
- ADC (convertidor analógico-digital)
- PWM (modulación por ancho de pulso)
- Y muchas otras funciones

> **PINSEL es SIEMPRE el primer paso** antes de usar cualquier periférico en el LPC1769.

---

## ¿Qué es PINSEL?

PINSEL es un **bloque de configuración** que mapea pines físicos a funciones específicas del hardware.

### Analogía didáctica

Imagina que cada pin es como un conector Swiss Army Knife (navaja suiza) que puede funcionar de diferentes maneras según cómo lo configures:
- **Función 0**: GPIO (cuchillo simple)
- **Función 1**: Primera función alternativa (destornillador)
- **Función 2**: Segunda función alternativa (abrelatas)
- **Función 3**: Tercera función alternativa (tijeras)

PINSEL es el mecanismo que **selecciona** cuál de estas "herramientas" quieres usar.

---

## Conceptos fundamentales

### 1. Funciones de pin

Cada pin del LPC1769 puede tener hasta **4 funciones diferentes**:

| Función | Valor | Descripción |
|---------|-------|-------------|
| `PINSEL_FUNC_0` | 0 | Función por defecto (generalmente GPIO) |
| `PINSEL_FUNC_1` | 1 | Primera función alternativa |
| `PINSEL_FUNC_2` | 2 | Segunda función alternativa |
| `PINSEL_FUNC_3` | 3 | Tercera función alternativa o reservada |

### Ejemplo: Pin P0.0

El pin **P0.0** puede configurarse como:
- **Función 0**: GPIO (P0.0)
- **Función 1**: RD1 (CAN)
- **Función 2**: TXD3 (UART3)
- **Función 3**: SDA1 (I2C)

> Para saber qué función corresponde a cada pin, **consulta el manual del usuario LPC17xx** (Tabla de Pin Functions).

---

### 2. Puertos y pines

El LPC1769 tiene **5 puertos** (PORT 0-4), cada uno con hasta 32 pines:

```c
#define PINSEL_PORT_0     0    // PORT 0
#define PINSEL_PORT_1     1    // PORT 1
#define PINSEL_PORT_2     2    // PORT 2
#define PINSEL_PORT_3     3    // PORT 3
#define PINSEL_PORT_4     4    // PORT 4
```

Cada pin se numera de 0 a 31:

```c
#define PINSEL_PIN_0      0
#define PINSEL_PIN_1      1
// ...
#define PINSEL_PIN_31     31
```

---

### 3. Modos de resistencia (Pull-up/Pull-down)

Los pines pueden configurarse con resistencias internas:

| Modo | Valor | Descripción |
|------|-------|-------------|
| `PINSEL_PINMODE_PULLUP` | 0 | Resistencia pull-up interna (~40kΩ) |
| `PINSEL_PINMODE_TRISTATE` | 2 | Sin resistencia (alta impedancia) |
| `PINSEL_PINMODE_PULLDOWN` | 3 | Resistencia pull-down interna (~40kΩ) |

**¿Cuándo usar cada modo?**

- **Pull-up**: Para botones/entradas que se conectan a GND
- **Pull-down**: Para entradas que se conectan a VCC
- **Tri-state**: Para minimizar consumo o cuando hay resistencia externa

---

### 4. Modo Open Drain

Permite configurar pines en modo **colector abierto**:

| Modo | Valor | Descripción |
|------|-------|-------------|
| `PINSEL_PINMODE_NORMAL` | 0 | Modo push-pull normal |
| `PINSEL_PINMODE_OPENDRAIN` | 1 | Modo open drain |

**Uso típico**: buses I2C, comunicaciones de un solo cable

---

## Estructuras y funciones del driver

### Estructura principal: `PINSEL_CFG_Type`

Esta estructura agrupa toda la configuración de un pin:

```c
typedef struct {
    uint8_t Portnum;      // Número de puerto (0-4)
    uint8_t Pinnum;       // Número de pin (0-31)
    uint8_t Funcnum;      // Función (0-3)
    uint8_t Pinmode;      // Modo de resistencia
    uint8_t OpenDrain;    // Modo open drain
} PINSEL_CFG_Type;
```

---

### Función principal: `PINSEL_ConfigPin()`

```c
void PINSEL_ConfigPin(PINSEL_CFG_Type *PinCfg);
```

Configura un pin según los parámetros de la estructura.

**Parámetros:**
- `PinCfg`: Puntero a estructura con la configuración del pin

**Retorno:** Ninguno

---

### Otras funciones disponibles

```c
// Configurar función especial de trace (depuración)
void PINSEL_ConfigTraceFunc(FunctionalState NewState);

// Configurar pines I2C0 en modo especial
void PINSEL_SetI2C0Pins(uint8_t i2cPinMode, FunctionalState filterSlewRateEnable);
```

---

## Ejemplos prácticos

### Ejemplo 1: Configurar pin como GPIO

```c
#include "lpc17xx_pinsel.h"

void configurar_pin_como_gpio(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P0.22 como GPIO
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_22;
    pin_cfg.Funcnum = PINSEL_FUNC_0;           // Función 0 = GPIO
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;   // Con pull-up
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL; // Modo normal

    PINSEL_ConfigPin(&pin_cfg);
}
```

---

### Ejemplo 2: Configurar pines para UART0

UART0 usa los siguientes pines en el LPC1769:
- **TXD0**: P0.2 (Función 1)
- **RXD0**: P0.3 (Función 1)

```c
#include "lpc17xx_pinsel.h"

void configurar_pines_uart0(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P0.2 como TXD0 (transmisión)
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_2;
    pin_cfg.Funcnum = PINSEL_FUNC_1;           // TXD0
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE; // Sin resistencia
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar P0.3 como RXD0 (recepción)
    pin_cfg.Pinnum = PINSEL_PIN_3;
    pin_cfg.Funcnum = PINSEL_FUNC_1;           // RXD0
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&pin_cfg);
}
```

---

### Ejemplo 3: Configurar pines para ADC

El ADC del LPC1769 tiene canales en diferentes pines. Por ejemplo:
- **AD0.0**: P0.23 (Función 1)
- **AD0.1**: P0.24 (Función 1)
- **AD0.2**: P0.25 (Función 1)

```c
#include "lpc17xx_pinsel.h"

void configurar_pin_adc(uint8_t canal) {
    PINSEL_CFG_Type pin_cfg;

    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_23 + canal;     // P0.23, P0.24, P0.25...
    pin_cfg.Funcnum = PINSEL_FUNC_1;            // Función ADC
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;  // Sin resistencia para señal analógica
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pin_cfg);
}

// Uso
int main(void) {
    configurar_pin_adc(0);  // Configurar AD0.0 (P0.23)
    configurar_pin_adc(1);  // Configurar AD0.1 (P0.24)

    // Ahora se puede inicializar y usar el ADC
    // ...
}
```

---

### Ejemplo 4: Configurar pines para I2C

I2C usa pines en modo **open drain** con pull-up externas:
- **SDA0**: P0.27 (Función 1)
- **SCL0**: P0.28 (Función 1)

```c
#include "lpc17xx_pinsel.h"

void configurar_pines_i2c0(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar SDA0 (P0.27)
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_27;
    pin_cfg.Funcnum = PINSEL_FUNC_1;           // I2C SDA
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE; // Sin pull-up interno
    pin_cfg.OpenDrain = PINSEL_PINMODE_OPENDRAIN; // IMPORTANTE: Open drain
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar SCL0 (P0.28)
    pin_cfg.Pinnum = PINSEL_PIN_28;
    pin_cfg.Funcnum = PINSEL_FUNC_1;           // I2C SCL
    PINSEL_ConfigPin(&pin_cfg);

    // Configuración especial para I2C0 (modo estándar)
    PINSEL_SetI2C0Pins(PINSEL_I2C_Normal_Mode, ENABLE);
}
```

---

### Ejemplo 5: Configurar múltiples pines con función for

Cuando necesitas configurar varios pines similares:

```c
#include "lpc17xx_pinsel.h"

void configurar_bus_datos_gpio(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P2.0 a P2.7 como GPIO (bus de 8 bits)
    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Funcnum = PINSEL_FUNC_0;           // GPIO
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    for (uint8_t i = 0; i < 8; i++) {
        pin_cfg.Pinnum = i;
        PINSEL_ConfigPin(&pin_cfg);
    }
}
```

---

## Casos de uso comunes

### Caso 1: LED conectado a GPIO

```c
void init_led(void) {
    PINSEL_CFG_Type pin_cfg;

    // LED en P0.22
    pin_cfg.Portnum = PINSEL_PORT_0;
    pin_cfg.Pinnum = PINSEL_PIN_22;
    pin_cfg.Funcnum = PINSEL_FUNC_0;           // GPIO
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE; // Sin resistencia
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pin_cfg);

    // Después configurar dirección con GPIO_SetDir()
}
```

---

### Caso 2: Botón con pull-up interno

```c
void init_boton(void) {
    PINSEL_CFG_Type pin_cfg;

    // Botón conectado a GND en P2.10
    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Pinnum = PINSEL_PIN_10;
    pin_cfg.Funcnum = PINSEL_FUNC_0;            // GPIO
    pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;    // Pull-up para botón a GND
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pin_cfg);
}
```

---

### Caso 3: PWM para controlar servo

PWM1.1 está en P2.0 (Función 1):

```c
void init_pwm_servo(void) {
    PINSEL_CFG_Type pin_cfg;

    // PWM1.1 en P2.0
    pin_cfg.Portnum = PINSEL_PORT_2;
    pin_cfg.Pinnum = PINSEL_PIN_0;
    pin_cfg.Funcnum = PINSEL_FUNC_1;           // PWM1.1
    pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pin_cfg);

    // Luego inicializar el periférico PWM
}
```

---

## Errores comunes

### ❌ Error 1: No configurar PINSEL antes de usar un periférico

```c
// INCORRECTO - Falta configuración PINSEL
void mal_ejemplo(void) {
    UART_Init(LPC_UART0, &uart_cfg);  // ❌ Pines no configurados
}

// CORRECTO
void buen_ejemplo(void) {
    configurar_pines_uart0();         // ✅ Primero PINSEL
    UART_Init(LPC_UART0, &uart_cfg);  // ✅ Luego el periférico
}
```

---

### ❌ Error 2: Función incorrecta

```c
// INCORRECTO - P0.2 con función 0 no es TXD0
pin_cfg.Pinnum = PINSEL_PIN_2;
pin_cfg.Funcnum = PINSEL_FUNC_0;  // ❌ Esto es GPIO, no UART

// CORRECTO - TXD0 es función 1 en P0.2
pin_cfg.Funcnum = PINSEL_FUNC_1;  // ✅ Función correcta
```

> **Tip**: Siempre verifica en el manual del usuario qué función corresponde a cada pin.

---

### ❌ Error 3: Olvidar modo open drain para I2C

```c
// INCORRECTO - I2C necesita open drain
pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL;  // ❌ No funcionará

// CORRECTO
pin_cfg.OpenDrain = PINSEL_PINMODE_OPENDRAIN;  // ✅ Open drain
```

---

### ❌ Error 4: Pull-up/Pull-down en pines analógicos

```c
// INCORRECTO - ADC necesita entrada en alta impedancia
pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;  // ❌ Afecta la lectura

// CORRECTO
pin_cfg.Pinmode = PINSEL_PINMODE_TRISTATE;  // ✅ Sin resistencias
```

---

## Registro de hardware

El driver PINSEL accede a los siguientes registros del hardware:

### Registros PINSEL0-PINSEL10

Cada registro controla la función de 16 pines (2 bits por pin):

| Registro | Pines controlados |
|----------|-------------------|
| PINSEL0 | P0.0 - P0.15 |
| PINSEL1 | P0.16 - P0.31 |
| PINSEL2 | P1.0 - P1.15 |
| PINSEL3 | P1.16 - P1.31 |
| PINSEL4 | P2.0 - P2.15 |
| ... | ... |

### Ejemplo de acceso directo (para referencia)

```c
// Esto lo hace automáticamente PINSEL_ConfigPin()
LPC_PINCON->PINSEL0 &= ~(0x03 << (2 * 2));  // Limpiar bits de P0.2
LPC_PINCON->PINSEL0 |= (0x01 << (2 * 2));   // Función 1 para P0.2
```

> **No es necesario** acceder directamente a los registros. Usa siempre `PINSEL_ConfigPin()`.

---

## Ejercicios propuestos

### Ejercicio 1: Configurar LED RGB

Configura 3 pines GPIO para controlar un LED RGB:
- LED Rojo: P0.22
- LED Verde: P0.21
- LED Azul: P0.20

```c
void init_led_rgb(void) {
    // TU CÓDIGO AQUÍ
}
```

---

### Ejercicio 2: Configurar UART1

UART1 usa:
- TXD1: P0.15 (Función 1)
- RXD1: P0.16 (Función 1)

Implementa la función de configuración.

---

### Ejercicio 3: Configurar 4 canales ADC

Configura AD0.0, AD0.1, AD0.2 y AD0.3 (P0.23-P0.26) usando un bucle.

---

### Ejercicio 4: Bus paralelo de 8 bits

Configura P2.0-P2.7 como GPIO para un bus de datos paralelo con pull-up.

---

## Resumen

### Puntos clave

1. **PINSEL configura la función de cada pin** (GPIO, UART, ADC, PWM, etc.)
2. **Siempre configurar PINSEL ANTES** de usar cualquier periférico
3. **Consultar el manual** para saber qué función corresponde a cada pin
4. **Usar `PINSEL_ConfigPin()`** en lugar de acceso directo a registros
5. **I2C requiere modo open drain**
6. **ADC requiere modo tri-state (sin resistencias)**
7. **Pull-up para botones conectados a GND**

### Flujo típico de configuración

```c
// 1. Incluir header
#include "lpc17xx_pinsel.h"

// 2. Declarar estructura
PINSEL_CFG_Type pin_cfg;

// 3. Configurar parámetros
pin_cfg.Portnum = PINSEL_PORT_X;
pin_cfg.Pinnum = PINSEL_PIN_Y;
pin_cfg.Funcnum = PINSEL_FUNC_Z;
pin_cfg.Pinmode = ...;
pin_cfg.OpenDrain = ...;

// 4. Aplicar configuración
PINSEL_ConfigPin(&pin_cfg);

// 5. Inicializar periférico correspondiente
```

---

## Referencias

- **Manual del usuario LPC17xx**: Capítulo 8 - Pin Connect Block
- **Datasheet LPC1769**: Tabla de funciones de pines
- **CMSIS Driver**: `lpc17xx_pinsel.h` y `lpc17xx_pinsel.c`

---

**Siguiente tutorial**: [GPIO - General Purpose Input/Output](./02_GPIO.md)

