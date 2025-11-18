# Tutorial de Debug Framework para LPC1769

**Autor:** Tutorial generado para la materia Electrónica Digital III
**Fecha:** 2024
**Microcontrolador:** LPC1769 (ARM Cortex-M3)
**Archivos fuente verificados:**
- `library/CMSISv2p00_LPC17xx/Drivers/inc/debug_frmwrk.h`
- `library/CMSISv2p00_LPC17xx/Drivers/src/debug_frmwrk.c`

---

## Índice

1. [Introducción](#1-introducción)
2. [Configuración del framework](#2-configuración-del-framework)
3. [Macros de debugging](#3-macros-de-debugging)
4. [Funciones disponibles](#4-funciones-disponibles)
5. [Inicialización](#5-inicialización)
6. [Ejemplos prácticos](#6-ejemplos-prácticos)
7. [Errores comunes](#7-errores-comunes)
8. [Referencias](#8-referencias)

---

## 1. Introducción

El **Debug Framework** es una biblioteca simple y liviana para facilitar el debugging de aplicaciones en el LPC1769 mediante comunicación UART. Provee macros y funciones que permiten imprimir strings, números decimales, números hexadecimales y caracteres individuales a través de un puerto UART configurado.

### ¿Por qué usar este framework?

✅ **Debugging simple:** Imprime mensajes de debug fácilmente sin configurar UART manualmente
✅ **Macros convenientes:** `_DBG()`, `_DBH()`, `_DBD()` para diferentes tipos de datos
✅ **Configuración automática:** Inicializa UART con una sola llamada
✅ **Estandarizado:** Usado en todos los ejemplos de NXP

### Uso típico

```c
#include "debug_frmwrk.h"

int main(void) {
    debug_frmwrk_init();  // Inicializar UART0 a 115200 bps

    _DBG("Hola mundo!\n\r");           // Imprimir string
    _DBD32(12345);                      // Imprimir decimal de 32 bits
    _DBH32(0xABCD1234);                 // Imprimir hex de 32 bits

    while(1);
}
```

---

## 2. Configuración del framework

### 2.1. Selección del puerto UART

El framework puede usar **UART0 o UART1** para debugging. Se configura mediante la macro `USED_UART_DEBUG_PORT` en `debug_frmwrk.h:26`:

```c
#define USED_UART_DEBUG_PORT    0  // Usar UART0 (por defecto)
```

**Opciones:**
- `0`: Usar UART0 (P0.2/TXD, P0.3/RXD)
- `1`: Usar UART1 (P0.15/TXD, P0.16/RXD)

**⚠️ NOTA:** Para cambiar el puerto UART, debes modificar este define en el archivo `debug_frmwrk.h`.

### 2.2. Pines según puerto UART

**UART0 (por defecto):**
| Pin | Función | Configuración |
|-----|---------|---------------|
| P0.2 | TXD0 | Función 1 |
| P0.3 | RXD0 | Función 1 |

Configurado automáticamente en `debug_frmwrk.c:252-263`.

**UART1:**
| Pin | Función | Configuración |
|-----|---------|---------------|
| P0.15 | TXD1 | Función 1 |
| P0.16 | RXD1 | Función 1 |

Configurado automáticamente en `debug_frmwrk.c:264-276`.

### 2.3. Parámetros de comunicación

La función `debug_frmwrk_init()` configura el UART con los siguientes parámetros (verificado en `debug_frmwrk.c:278-292`):

```c
Baudrate:    115200 bps
Data bits:   8
Stop bits:   1
Paridad:     Ninguna
Control:     Sin control de flujo
```

---

## 3. Macros de debugging

El framework provee macros convenientes definidas en `debug_frmwrk.h:34-43`:

### 3.1. Impresión de strings

```c
#define _DBG(x)    _db_msg(DEBUG_UART_PORT, x)
```

**Descripción:** Imprime un string sin newline
**Parámetro:** String terminado en null
**Retorno:** Ninguno

**Ejemplo:**
```c
_DBG("ADC value: ");
```

---

```c
#define _DBG_(x)   _db_msg_(DEBUG_UART_PORT, x)
```

**Descripción:** Imprime un string **con newline** (`\n\r`) al final
**Parámetro:** String terminado en null
**Retorno:** Ninguno

**Ejemplo:**
```c
_DBG_("Transferencia completa");  // Imprime y va a nueva línea
```

---

### 3.2. Impresión de caracteres

```c
#define _DBC(x)    _db_char(DEBUG_UART_PORT, x)
```

**Descripción:** Imprime un carácter individual
**Parámetro:** `uint8_t` - carácter a imprimir
**Retorno:** Ninguno

**Ejemplo:**
```c
_DBC('A');     // Imprime el carácter 'A'
_DBC('\n');    // Imprime newline
```

---

### 3.3. Impresión de números decimales

```c
#define _DBD(x)      _db_dec(DEBUG_UART_PORT, x)
#define _DBD16(x)    _db_dec_16(DEBUG_UART_PORT, x)
#define _DBD32(x)    _db_dec_32(DEBUG_UART_PORT, x)
```

**Descripción:** Imprime un número en formato decimal
**Parámetros:**
- `_DBD(x)`: `uint8_t` (8 bits) - rango 0-255
- `_DBD16(x)`: `uint16_t` (16 bits) - rango 0-65535
- `_DBD32(x)`: `uint32_t` (32 bits) - rango 0-4294967295

**Retorno:** Ninguno

**Ejemplos:**
```c
uint8_t  valor8 = 123;
uint16_t valor16 = 45678;
uint32_t valor32 = 1234567890;

_DBD(valor8);      // Imprime "123"
_DBD16(valor16);   // Imprime "45678"
_DBD32(valor32);   // Imprime "1234567890"
```

**⚠️ IMPORTANTE:** Los números se imprimen con **todos los dígitos**, incluyendo ceros a la izquierda:

```c
_DBD(5);      // Imprime "005" (siempre 3 dígitos para 8-bit)
_DBD16(42);   // Imprime "00042" (siempre 5 dígitos para 16-bit)
_DBD32(100);  // Imprime "0000000100" (siempre 10 dígitos para 32-bit)
```

---

### 3.4. Impresión de números hexadecimales

```c
#define _DBH(x)      _db_hex(DEBUG_UART_PORT, x)
#define _DBH16(x)    _db_hex_16(DEBUG_UART_PORT, x)
#define _DBH32(x)    _db_hex_32(DEBUG_UART_PORT, x)
```

**Descripción:** Imprime un número en formato hexadecimal con prefijo `0x`
**Parámetros:**
- `_DBH(x)`: `uint8_t` (8 bits)
- `_DBH16(x)`: `uint16_t` (16 bits)
- `_DBH32(x)`: `uint32_t` (32 bits)

**Retorno:** Ninguno

**Ejemplos:**
```c
uint8_t  reg8 = 0xAB;
uint16_t reg16 = 0x1234;
uint32_t reg32 = 0xDEADBEEF;

_DBH(reg8);      // Imprime "0xAB"
_DBH16(reg16);   // Imprime "0x1234"
_DBH32(reg32);   // Imprime "0xDEADBEEF"
```

**Formato de salida:**
- 8-bit: `0xXX` (2 dígitos hex)
- 16-bit: `0xXXXX` (4 dígitos hex)
- 32-bit: `0xXXXXXXXX` (8 dígitos hex)

**📝 Nota:** Los dígitos hexadecimales A-F se imprimen en **mayúsculas** (verificado en `debug_frmwrk.c:183`).

---

### 3.5. Lectura de caracteres

```c
#define _DG    _db_get_char(DEBUG_UART_PORT)
```

**Descripción:** Lee un carácter desde el UART (modo bloqueante)
**Parámetros:** Ninguno
**Retorno:** `uint8_t` - carácter recibido

**Ejemplo:**
```c
uint8_t tecla;

_DBG("Presiona una tecla: ");
tecla = _DG;       // Espera hasta recibir un carácter
_DBC(tecla);       // Echo del carácter recibido
```

---

## 4. Funciones disponibles

Además de las macros, el framework provee funciones que pueden ser llamadas directamente. Todas están documentadas en `debug_frmwrk.h:57-67`.

### 4.1. Funciones de transmisión

```c
void UARTPutChar(LPC_UART_TypeDef *UARTx, uint8_t ch);
```

**Descripción:** Transmite un carácter por el UART especificado
**Parámetros:**
- `UARTx`: Puntero al UART (LPC_UART0, LPC_UART1, etc.)
- `ch`: Carácter a transmitir

**Implementación (debug_frmwrk.c:54-57):**
```c
void UARTPutChar (LPC_UART_TypeDef *UARTx, uint8_t ch)
{
    UART_Send(UARTx, &ch, 1, BLOCKING);  // Envío bloqueante
}
```

**Ejemplo:**
```c
UARTPutChar(LPC_UART0, 'A');
UARTPutChar(LPC_UART0, '\n');
```

---

```c
void UARTPuts(LPC_UART_TypeDef *UARTx, const void *str);
```

**Descripción:** Transmite un string sin newline
**Parámetros:**
- `UARTx`: Puntero al UART
- `str`: String terminado en null

**Implementación (debug_frmwrk.c:79-87):**
```c
void UARTPuts(LPC_UART_TypeDef *UARTx, const void *str)
{
    uint8_t *s = (uint8_t *) str;

    while (*s) {
        UARTPutChar(UARTx, *s++);
    }
}
```

**Ejemplo:**
```c
UARTPuts(LPC_UART0, "Temperatura: ");
```

---

```c
void UARTPuts_(LPC_UART_TypeDef *UARTx, const void *str);
```

**Descripción:** Transmite un string **con newline** (`\n\r`)
**Parámetros:**
- `UARTx`: Puntero al UART
- `str`: String terminado en null

**Implementación (debug_frmwrk.c:96-100):**
```c
void UARTPuts_(LPC_UART_TypeDef *UARTx, const void *str)
{
    UARTPuts(UARTx, str);
    UARTPuts(UARTx, "\n\r");  // Agrega newline + carriage return
}
```

**Ejemplo:**
```c
UARTPuts_(LPC_UART0, "Inicializacion completa");
```

---

### 4.2. Funciones de números decimales

```c
void UARTPutDec(LPC_UART_TypeDef *UARTx, uint8_t decnum);
void UARTPutDec16(LPC_UART_TypeDef *UARTx, uint16_t decnum);
void UARTPutDec32(LPC_UART_TypeDef *UARTx, uint32_t decnum);
```

**Descripción:** Transmite un número en formato decimal
**Parámetros:**
- `UARTx`: Puntero al UART
- `decnum`: Número a imprimir (8, 16 o 32 bits)

**Formato de salida:**
- `UARTPutDec()`: 3 dígitos (000-255)
- `UARTPutDec16()`: 5 dígitos (00000-65535)
- `UARTPutDec32()`: 10 dígitos (0000000000-4294967295)

**Implementación de UARTPutDec32 (debug_frmwrk.c:145-167):**
```c
void UARTPutDec32(LPC_UART_TypeDef *UARTx, uint32_t decnum)
{
    uint8_t c1 = decnum % 10;
    uint8_t c2 = (decnum/10) % 10;
    uint8_t c3 = (decnum/100) % 10;
    // ... hasta c10
    UARTPutChar(UARTx, '0'+c10);
    UARTPutChar(UARTx, '0'+c9);
    // ... hasta c1
}
```

**Ejemplo:**
```c
UARTPutDec(LPC_UART0, 42);        // Imprime "042"
UARTPutDec16(LPC_UART0, 1234);    // Imprime "01234"
UARTPutDec32(LPC_UART0, 999999);  // Imprime "0000999999"
```

---

### 4.3. Funciones de números hexadecimales

```c
void UARTPutHex(LPC_UART_TypeDef *UARTx, uint8_t hexnum);
void UARTPutHex16(LPC_UART_TypeDef *UARTx, uint16_t hexnum);
void UARTPutHex32(LPC_UART_TypeDef *UARTx, uint32_t hexnum);
```

**Descripción:** Transmite un número en formato hexadecimal con prefijo `0x`
**Parámetros:**
- `UARTx`: Puntero al UART
- `hexnum`: Número a imprimir (8, 16 o 32 bits)

**Formato de salida:**
- `UARTPutHex()`: `0xXX` (2 dígitos)
- `UARTPutHex16()`: `0xXXXX` (4 dígitos)
- `UARTPutHex32()`: `0xXXXXXXXX` (8 dígitos)

**Implementación de UARTPutHex32 (debug_frmwrk.c:212-222):**
```c
void UARTPutHex32 (LPC_UART_TypeDef *UARTx, uint32_t hexnum)
{
    uint8_t nibble, i;

    UARTPuts(UARTx, "0x");
    i = 7;
    do {
        nibble = (hexnum >> (4*i)) & 0x0F;
        UARTPutChar(UARTx, (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble));
    } while (i--);
}
```

**Ejemplo:**
```c
UARTPutHex(LPC_UART0, 0xAB);        // Imprime "0xAB"
UARTPutHex16(LPC_UART0, 0x1234);    // Imprime "0x1234"
UARTPutHex32(LPC_UART0, 0xDEADBEEF); // Imprime "0xDEADBEEF"
```

---

### 4.4. Función de recepción

```c
uint8_t UARTGetChar(LPC_UART_TypeDef *UARTx);
```

**Descripción:** Recibe un carácter del UART (modo bloqueante)
**Parámetros:**
- `UARTx`: Puntero al UART

**Retorno:** `uint8_t` - carácter recibido

**Implementación (debug_frmwrk.c:65-70):**
```c
uint8_t UARTGetChar (LPC_UART_TypeDef *UARTx)
{
    uint8_t tmp = 0;
    UART_Receive(UARTx, &tmp, 1, BLOCKING);
    return(tmp);
}
```

**Ejemplo:**
```c
uint8_t tecla = UARTGetChar(LPC_UART0);
UARTPutChar(LPC_UART0, tecla);  // Echo
```

---

## 5. Inicialización

### 5.1. Función de inicialización

```c
void debug_frmwrk_init(void);
```

**Descripción:** Inicializa el framework de debug configurando el UART seleccionado
**Parámetros:** Ninguno
**Retorno:** Ninguno

**Acciones realizadas (debug_frmwrk.c:247-304):**

1. **Configura pines del UART:**
   - UART0: P0.2 (TXD), P0.3 (RXD) → Función 1
   - UART1: P0.15 (TXD), P0.16 (RXD) → Función 1

2. **Inicializa UART:**
   - Baudrate: 115200 bps
   - 8 bits de datos
   - 1 bit de stop
   - Sin paridad
   - Sin control de flujo

3. **Habilita transmisión:**
   ```c
   UART_TxCmd((LPC_UART_TypeDef*)DEBUG_UART_PORT, ENABLE);
   ```

4. **Asigna punteros a funciones:**
   ```c
   _db_msg = UARTPuts;
   _db_msg_ = UARTPuts_;
   _db_char = UARTPutChar;
   _db_hex = UARTPutHex;
   _db_hex_16 = UARTPutHex16;
   _db_hex_32 = UARTPutHex32;
   _db_dec = UARTPutDec;
   _db_dec_16 = UARTPutDec16;
   _db_dec_32 = UARTPutDec32;
   _db_get_char = UARTGetChar;
   ```

### 5.2. Ejemplo de inicialización

```c
#include "debug_frmwrk.h"

int main(void)
{
    // Inicializar framework
    debug_frmwrk_init();

    // Ya se puede usar inmediatamente
    _DBG_("Sistema inicializado");

    while(1) {
        // Tu código aquí
    }
}
```

---

## 6. Ejemplos prácticos

### Ejemplo 1: Menú de bienvenida

Este patrón se usa en **todos** los ejemplos de NXP.

```c
#include "debug_frmwrk.h"

uint8_t menu[] =
"********************************************************************************\n\r"
"Hola NXP Semiconductors \n\r"
" Ejemplo de ADC \n\r"
"\t - MCU: LPC1769 \n\r"
"\t - Core: ARM Cortex-M3 \n\r"
"\t - Comunicacion via: UART0 - 115200 bps \n\r"
" Este ejemplo muestra como usar el ADC en modo polling \n\r"
"********************************************************************************\n\r";

void print_menu(void)
{
    _DBG(menu);
}

int main(void)
{
    debug_frmwrk_init();
    print_menu();

    while(1);
}
```

**Salida:**
```
********************************************************************************
Hola NXP Semiconductors
 Ejemplo de ADC
     - MCU: LPC1769
     - Core: ARM Cortex-M3
     - Comunicacion via: UART0 - 115200 bps
 Este ejemplo muestra como usar el ADC en modo polling
********************************************************************************
```

---

### Ejemplo 2: Debugging de valores ADC

```c
#include "debug_frmwrk.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"

int main(void)
{
    PINSEL_CFG_Type PinCfg;
    uint16_t adc_value;

    // Inicializar debug framework
    debug_frmwrk_init();

    _DBG_("Inicializando ADC...");

    // Configurar P0.25 como AD0.2
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar ADC
    ADC_Init(LPC_ADC, 200000);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);

    _DBG_("ADC listo. Leyendo valores...\n\r");

    while(1) {
        // Iniciar conversión
        ADC_StartCmd(LPC_ADC, ADC_START_NOW);

        // Esperar resultado
        while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));

        // Obtener valor
        adc_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);

        // Imprimir en múltiples formatos
        _DBG("Valor ADC (dec): ");
        _DBD16(adc_value);
        _DBG(" | (hex): ");
        _DBH16(adc_value);
        _DBG_("");  // Nueva línea

        // Delay simple
        for(uint32_t i = 0; i < 1000000; i++);
    }
}
```

**Salida:**
```
Inicializando ADC...
ADC listo. Leyendo valores...

Valor ADC (dec): 01234 | (hex): 0x04D2
Valor ADC (dec): 02048 | (hex): 0x0800
Valor ADC (dec): 03891 | (hex): 0x0F33
...
```

---

### Ejemplo 3: Debugging de estados de GPDMA

```c
#include "debug_frmwrk.h"
#include "lpc17xx_gpdma.h"

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
            _DBG_("DMA: Terminal Count OK");
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
            _DBG_("DMA: ERROR!");
        }
    }
}

int main(void)
{
    GPDMA_Channel_CFG_Type GPDMACfg;
    uint32_t src_buffer[256];
    uint32_t dst_buffer[256];

    debug_frmwrk_init();

    _DBG_("Inicializando GPDMA...");

    // Inicializar buffers
    for (uint32_t i = 0; i < 256; i++) {
        src_buffer[i] = i;
        dst_buffer[i] = 0;
    }

    // Configurar DMA
    GPDMA_Init();

    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)src_buffer;
    GPDMACfg.DstMemAddr = (uint32_t)dst_buffer;
    GPDMACfg.TransferSize = 256;
    GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = 0;
    GPDMACfg.DMALLI = 0;

    GPDMA_Setup(&GPDMACfg);

    _DBG_("Iniciando transferencia DMA...");

    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);

    // Esperar completado
    while ((Channel0_TC == 0) && (Channel0_Err == 0));

    if (Channel0_TC > 0) {
        _DBG("Transferencia exitosa. TC count: ");
        _DBD32(Channel0_TC);
        _DBG_("");
    }

    if (Channel0_Err > 0) {
        _DBG("Errores detectados: ");
        _DBD32(Channel0_Err);
        _DBG_("");
    }

    while(1);
}
```

**Salida:**
```
Inicializando GPDMA...
Iniciando transferencia DMA...
DMA: Terminal Count OK
Transferencia exitosa. TC count: 0000000001
```

---

### Ejemplo 4: Menú interactivo

```c
#include "debug_frmwrk.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

void print_menu(void)
{
    _DBG_("\n\r===== MENU =====");
    _DBG_("1. Encender LED");
    _DBG_("2. Apagar LED");
    _DBG_("3. Toggle LED");
    _DBG_("0. Salir");
    _DBG("Opcion: ");
}

int main(void)
{
    uint8_t opcion;
    PINSEL_CFG_Type PinCfg;

    debug_frmwrk_init();

    // Configurar P0.22 como GPIO (LED)
    PinCfg.Funcnum = 0;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 22;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    GPIO_SetDir(0, (1<<22), 1);  // Output

    _DBG_("Sistema de control de LED");

    while(1) {
        print_menu();

        // Leer opción del usuario
        opcion = _DG;
        _DBC(opcion);  // Echo
        _DBG_("");     // Nueva línea

        switch(opcion) {
            case '1':
                GPIO_SetValue(0, (1<<22));
                _DBG_("LED encendido");
                break;

            case '2':
                GPIO_ClearValue(0, (1<<22));
                _DBG_("LED apagado");
                break;

            case '3':
                if (GPIO_ReadValue(0) & (1<<22)) {
                    GPIO_ClearValue(0, (1<<22));
                    _DBG_("LED apagado (toggle)");
                } else {
                    GPIO_SetValue(0, (1<<22));
                    _DBG_("LED encendido (toggle)");
                }
                break;

            case '0':
                _DBG_("Saliendo...");
                while(1);
                break;

            default:
                _DBG_("Opcion invalida");
                break;
        }
    }
}
```

**Salida:**
```
Sistema de control de LED

===== MENU =====
1. Encender LED
2. Apagar LED
3. Toggle LED
0. Salir
Opcion: 1
LED encendido

===== MENU =====
1. Encender LED
2. Apagar LED
3. Toggle LED
0. Salir
Opcion: 3
LED apagado (toggle)
...
```

---

### Ejemplo 5: Debugging de registros

```c
#include "debug_frmwrk.h"
#include "LPC17xx.h"

void print_register(char *name, uint32_t value)
{
    _DBG(name);
    _DBG(": ");
    _DBH32(value);
    _DBG_("");
}

void dump_gpio_registers(void)
{
    _DBG_("\n\r===== GPIO0 Registers =====");
    print_register("FIODIR ", LPC_GPIO0->FIODIR);
    print_register("FIOMASK", LPC_GPIO0->FIOMASK);
    print_register("FIOPIN ", LPC_GPIO0->FIOPIN);
    print_register("FIOSET ", LPC_GPIO0->FIOSET);
    print_register("FIOCLR ", LPC_GPIO0->FIOCLR);
}

void dump_uart_registers(void)
{
    _DBG_("\n\r===== UART0 Registers =====");
    print_register("RBR/THR", LPC_UART0->RBR);
    print_register("IER    ", LPC_UART0->IER);
    print_register("IIR    ", LPC_UART0->IIR);
    print_register("LCR    ", LPC_UART0->LCR);
    print_register("LSR    ", LPC_UART0->LSR);
}

int main(void)
{
    debug_frmwrk_init();

    _DBG_("===== Register Dump Utility =====");

    dump_gpio_registers();
    dump_uart_registers();

    while(1);
}
```

**Salida:**
```
===== Register Dump Utility =====

===== GPIO0 Registers =====
FIODIR : 0x00400000
FIOMASK: 0x00000000
FIOPIN : 0x00000000
FIOSET : 0x00000000
FIOCLR : 0x00000000

===== UART0 Registers =====
RBR/THR: 0x00000000
IER    : 0x00000000
IIR    : 0x00000001
LCR    : 0x00000003
LSR    : 0x00000060
```

---

## 7. Errores comunes

### Error 1: Olvidar llamar `debug_frmwrk_init()`

**Síntoma:** No se imprime nada por UART o el programa se cuelga.

**Causa:** El UART no está inicializado.

**Solución:**

```c
int main(void)
{
    debug_frmwrk_init();  // ¡CRÍTICO! Llamar primero

    _DBG_("Hola mundo");  // Ahora funciona

    while(1);
}
```

---

### Error 2: Usar puerto UART incorrecto en terminal

**Síntoma:** No se recibe nada en la terminal serial.

**Causa:** El terminal está conectado a un puerto diferente al configurado.

**Solución:**
1. Verificar qué UART usa el framework en `debug_frmwrk.h:26`:
   ```c
   #define USED_UART_DEBUG_PORT    0  // UART0
   ```

2. Conectar el terminal al puerto correcto:
   - UART0: P0.2 (TXD), P0.3 (RXD)
   - UART1: P0.15 (TXD), P0.16 (RXD)

3. Verificar configuración del terminal:
   - Baudrate: **115200 bps**
   - Data bits: 8
   - Stop bits: 1
   - Paridad: Ninguna

---

### Error 3: Confundir `_DBG()` con `_DBG_()`

**Síntoma:** Todos los mensajes aparecen en la misma línea.

**Causa:** `_DBG()` **no** agrega newline al final.

**Solución:**

```c
// ❌ INCORRECTO: Todo en una línea
_DBG("Mensaje 1");
_DBG("Mensaje 2");
// Salida: "Mensaje 1Mensaje 2"

// ✅ CORRECTO: Usar _DBG_() para nueva línea
_DBG_("Mensaje 1");
_DBG_("Mensaje 2");
// Salida:
// "Mensaje 1
//  Mensaje 2"

// ✅ CORRECTO: O agregar \n\r manualmente
_DBG("Mensaje 1\n\r");
_DBG("Mensaje 2\n\r");
```

---

### Error 4: Formato incorrecto de newline

**Síntoma:** En la terminal aparecen saltos de línea pero sin retorno de carro (escalera).

**Causa:** Usar solo `\n` en lugar de `\n\r`.

**Solución:**

```c
// ❌ INCORRECTO: Solo \n
_DBG("Linea 1\n");
_DBG("Linea 2\n");
// Salida:
// "Linea 1
//         Linea 2
//                 ..."  (efecto escalera)

// ✅ CORRECTO: Usar \n\r (newline + carriage return)
_DBG("Linea 1\n\r");
_DBG("Linea 2\n\r");
// O simplemente usar _DBG_()
_DBG_("Linea 1");
_DBG_("Linea 2");
```

---

### Error 5: Imprimir decimales con ceros no deseados

**Síntoma:** Los números decimales se imprimen con muchos ceros a la izquierda.

**Causa:** Las funciones `_DBD()`, `_DBD16()`, `_DBD32()` **siempre** imprimen todos los dígitos.

**Solución:**

Opción A - Aceptar el formato con ceros:
```c
uint32_t temp = 25;
_DBG("Temperatura: ");
_DBD32(temp);  // Imprime "0000000025"
```

Opción B - Crear función custom sin ceros:
```c
void print_dec_no_zeros(uint32_t num)
{
    char buffer[12];  // Máximo 10 dígitos + null
    sprintf(buffer, "%u", num);
    _DBG(buffer);
}

// Uso:
print_dec_no_zeros(25);  // Imprime "25" (sin ceros)
```

**📝 Nota:** La función `sprintf()` requiere incluir `<stdio.h>` y puede aumentar el tamaño del código significativamente.

---

### Error 6: Usar funciones sin habilitar `_DBGFWK`

**Síntoma:** Errores de linkeo (undefined reference).

**Causa:** El código del debug framework está condicionado por `#ifdef _DBGFWK` (línea 33 en debug_frmwrk.c).

**Solución:** Verificar que `_DBGFWK` esté definido en `lpc17xx_libcfg.h` o en las opciones del compilador:

```c
// En lpc17xx_libcfg.h:
#define _DBGFWK
```

O en las opciones del compilador (gcc):
```
-D_DBGFWK
```

---

### Error 7: Confusión con printf()

**Síntoma:** Intentar usar `printf()` pero no funciona.

**Causa:** El debug framework **NO incluye `printf()`**. La función `_printf()` está comentada en el código (líneas 224-240 en debug_frmwrk.c).

**Solución:** Usar las funciones disponibles:

```c
// ❌ INCORRECTO: printf no está disponible
printf("Temperatura: %d C\n", temp);

// ✅ CORRECTO: Usar funciones del framework
_DBG("Temperatura: ");
_DBD32(temp);
_DBG_(" C");
```

Si realmente necesitas `printf()`, debes:
1. Incluir `<stdio.h>`
2. Implementar `_write()` para redirigir a UART
3. Esto aumentará significativamente el tamaño del código

---

### Error 8: Bloqueo en `_DG` (lectura)

**Síntoma:** El programa se queda esperando indefinidamente.

**Causa:** `_DG` (y `UARTGetChar()`) es **bloqueante**, espera hasta recibir un carácter.

**Solución:**

Opción A - Asegurar que se enviará un carácter:
```c
_DBG("Presiona Enter: ");
char c = _DG;  // Espera hasta que el usuario presione una tecla
```

Opción B - Verificar antes de leer (requiere acceso directo a registros):
```c
// Verificar si hay dato disponible
if (LPC_UART0->LSR & (1<<0)) {  // Bit RDR (Receiver Data Ready)
    char c = _DG;
    // Procesar carácter
}
```

---

### Error 9: Caracteres corruptos en terminal

**Síntoma:** Se reciben caracteres extraños o basura en la terminal.

**Causa:** Baudrate incorrecto en el terminal.

**Solución:**
1. Verificar baudrate del framework: **115200 bps** (hardcoded en debug_frmwrk.c:286)
2. Configurar terminal con:
   - Baudrate: 115200
   - Data: 8 bits
   - Stop: 1 bit
   - Paridad: None

Si necesitas otro baudrate, debes modificar `debug_frmwrk.c:286`:
```c
UARTConfigStruct.Baud_rate = 9600;  // O el baudrate que necesites
```

---

### Error 10: Problemas con punteros a función

**Síntoma:** Crash o comportamiento errático al usar las macros.

**Causa:** Los punteros a función no están inicializados (si no se llamó `debug_frmwrk_init()`).

**Verificación (debug_frmwrk.c:294-303):**
```c
void debug_frmwrk_init(void)
{
    // ... inicialización UART ...

    // Asignar punteros a funciones
    _db_msg = UARTPuts;
    _db_msg_ = UARTPuts_;
    _db_char = UARTPutChar;
    // ... etc
}
```

**Solución:** Siempre llamar `debug_frmwrk_init()` antes de usar cualquier macro.

---

## 8. Referencias

### Documentos oficiales
- **UM10360:** LPC176x/5x User Manual (Capítulo 14: UART)

### Archivos del driver
- `library/CMSISv2p00_LPC17xx/Drivers/inc/debug_frmwrk.h` - Macros y prototipos
- `library/CMSISv2p00_LPC17xx/Drivers/src/debug_frmwrk.c` - Implementación

### Tutoriales relacionados
- `06_UART.md` - Para entender el funcionamiento del UART
- `01_PINSEL.md` - Para configuración de pines

### Uso en ejemplos

El debug framework se usa en **TODOS** los ejemplos de NXP. Algunos ejemplos:
- `library/examples/ADC/Polling/adc_polling_test.c`
- `library/examples/GPDMA/Ram_2_Ram_Test/gpdma_r2r_test.c`
- `library/examples/UART/DMA/uart_dma_test.c`
- `library/examples/TIMER/Gen_Diff_Delay/gen_diff_delay.c`

---

**Fin del tutorial de Debug Framework para LPC1769**
