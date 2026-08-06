# Tutorial: Driver UART para LPC1769

## Introducción

El **UART (Universal Asynchronous Receiver/Transmitter)** es el periférico de comunicación serial asíncrona del LPC1769. Permite comunicación full-duplex con dispositivos externos como PCs, sensores, módulos GPS, etc. El LPC1769 cuenta con **4 puertos UART** (UART0, UART1, UART2, UART3) con características idénticas o similares.

**Características principales:**
- **4 UARTs** independientes (UART0-3)
- **Baudrates configurables** hasta 115200 bps (y superiores)
- **FIFO de 16 bytes** para TX y RX
- **Configuración flexible**: 5-8 bits de datos, 1-2 stop bits, paridad (none/odd/even/stick)
- **Modos de operación**: Polling y con interrupciones
- **Auto-baud**: Detección automática de baudrate
- **Control de flujo por hardware** (UART1: RTS/CTS)
- **RS-485** (UART1 solamente)
- **IrDA** (UART3 solamente)
- **Modem Full** (UART1 solamente: DTR, DSR, RI, DCD)

**Archivos del driver:**
- Header: `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_uart.h`
- Implementación: `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_uart.c`
- Ejemplos: `library/examples/UART/`

---

## Pines de UART en LPC1769

### UART0 (Estándar)

**Verificado en:** `library/examples/UART/Polling/uart_polling_test.c` líneas 90-97

| Señal | Pin   | Función PINSEL |
|-------|-------|----------------|
| TXD0  | P0.2  | 1              |
| RXD0  | P0.3  | 1              |

**Ejemplo de configuración:**
```c
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 1;
PinCfg.OpenDrain = 0;
PinCfg.Pinmode = 0;
PinCfg.Portnum = 0;
PinCfg.Pinnum = 2;  // TXD0
PINSEL_ConfigPin(&PinCfg);
PinCfg.Pinnum = 3;  // RXD0
PINSEL_ConfigPin(&PinCfg);
```

### UART1 (Full Modem + RS-485)

**Verificado en:** `library/examples/UART/Polling/uart_polling_test.c` líneas 104-111

| Señal | Pin   | Función PINSEL |
|-------|-------|----------------|
| TXD1  | P2.0  | 2              |
| RXD1  | P2.1  | 2              |
| RTS1  | P2.2  | 2 (opcional)   |
| CTS1  | P2.7  | 2 (opcional)   |
| DTR1  | P2.5  | 2 (opcional)   |
| DSR1  | P2.6  | 2 (opcional)   |
| DCD1  | P2.3  | 2 (opcional)   |
| RI1   | P2.4  | 2 (opcional)   |

### UART2 (Estándar)

**Nota:** Consultar **Tabla 8.5** del manual del usuario LPC17xx para pines alternativos.

| Señal | Pin   | Función PINSEL |
|-------|-------|----------------|
| TXD2  | P0.10 | 1              |
| RXD2  | P0.11 | 1              |

### UART3 (IrDA)

**Nota:** Consultar **Tabla 8.5** del manual del usuario LPC17xx para todos los pines.

| Señal | Pin   | Función PINSEL |
|-------|-------|----------------|
| TXD3  | P0.0  | 2              |
| RXD3  | P0.1  | 2              |

---

## Conceptos Fundamentales

### 1. Comunicación Serial Asíncrona

**UART** es asíncrono: no usa señal de reloj compartida. Ambos dispositivos deben estar configurados con el mismo:
- **Baudrate**: Velocidad de comunicación (bits por segundo)
- **Data bits**: Cantidad de bits por carácter (5, 6, 7, o 8)
- **Parity**: Bit de paridad para detección de errores (None, Even, Odd, Stick)
- **Stop bits**: Bits de parada (1 o 2)

**Formato de trama UART:**
```
 START | D0 D1 D2 D3 D4 D5 D6 D7 | PARITY | STOP
 (1bit)|      8 data bits        | (0-1)  | (1-2 bits)
```

### 2. Baudrates Comunes

El driver calcula automáticamente los divisores basándose en PCLK:

| Baudrate | Uso típico                    |
|----------|-------------------------------|
| 9600     | Dispositivos lentos, debug    |
| 19200    | Comunicación moderada         |
| 38400    | Comunicación rápida           |
| 57600    | Alta velocidad                |
| 115200   | Muy alta velocidad (común)    |

**Fórmula** (verificado en lpc17xx_uart.c:96-98):
```
BaudRate = PCLK / (16 * DLL * (1 + DivAddVal/MulVal))
```

El driver usa un algoritmo de optimización (lpc17xx_uart.c:103-135) para encontrar los mejores divisores con error < 3%.

### 3. FIFO (First In First Out)

Cada UART tiene **FIFOs de 16 bytes** para TX y RX. Beneficios:
- Reducir interrupciones (hasta 16 bytes antes de generar IRQ)
- Mejorar rendimiento en comunicaciones rápidas
- Trigger levels configurables: 1, 4, 8, o 14 caracteres

### 4. Line Status Register (LSR)

El LSR contiene el estado actual del UART:

| Bit | Nombre | Descripción |
|-----|--------|-------------|
| 0   | RDR    | Receive Data Ready (hay datos en RX FIFO) |
| 1   | OE     | Overrun Error (datos perdidos) |
| 2   | PE     | Parity Error |
| 3   | FE     | Framing Error (stop bit inválido) |
| 4   | BI     | Break Interrupt |
| 5   | THRE   | Transmit Holding Register Empty (TX FIFO vacío) |
| 6   | TEMT   | Transmitter Empty (TX completamente vacío) |
| 7   | RXFE   | Error in RX FIFO |

---

## Estructuras de Datos

### 1. UART_CFG_Type

Configuración básica del UART:

```c
typedef struct {
    uint32_t Baud_rate;           // Baudrate (ej: 9600, 115200)
    UART_PARITY_Type Parity;      // UART_PARITY_NONE, ODD, EVEN, SP_1, SP_0
    UART_DATABIT_Type Databits;   // UART_DATABIT_5, 6, 7, 8
    UART_STOPBIT_Type Stopbits;   // UART_STOPBIT_1, UART_STOPBIT_2
} UART_CFG_Type;
```

**Valores típicos:**
```c
UART_CFG_Type UARTConfigStruct;
UARTConfigStruct.Baud_rate = 115200;        // 115200 bps
UARTConfigStruct.Databits = UART_DATABIT_8; // 8 bits
UARTConfigStruct.Parity = UART_PARITY_NONE; // Sin paridad
UARTConfigStruct.Stopbits = UART_STOPBIT_1; // 1 stop bit
```

### 2. UART_FIFO_CFG_Type

Configuración del FIFO:

```c
typedef struct {
    FunctionalState FIFO_ResetRxBuf;  // ENABLE: Resetear RX FIFO
    FunctionalState FIFO_ResetTxBuf;  // ENABLE: Resetear TX FIFO
    FunctionalState FIFO_DMAMode;     // ENABLE: Habilitar DMA
    UART_FITO_LEVEL_Type FIFO_Level;  // Nivel de trigger RX
} UART_FIFO_CFG_Type;
```

**Niveles de trigger:**
- `UART_FIFO_TRGLEV0`: 1 carácter (genera IRQ con 1 byte)
- `UART_FIFO_TRGLEV1`: 4 caracteres
- `UART_FIFO_TRGLEV2`: 8 caracteres
- `UART_FIFO_TRGLEV3`: 14 caracteres

### 3. UART_INT_Type

Tipos de interrupciones:

```c
typedef enum {
    UART_INTCFG_RBR = 0,   // RBR Interrupt (Receive Data Available)
    UART_INTCFG_THRE,      // THR Interrupt (Transmit Holding Register Empty)
    UART_INTCFG_RLS,       // RX Line Status Interrupt
    UART1_INTCFG_MS,       // Modem status (UART1 only)
    UART1_INTCFG_CTS,      // CTS signal transition (UART1 only)
    UART_INTCFG_ABEO,      // End of auto-baud
    UART_INTCFG_ABTO       // Auto-baud time-out
} UART_INT_Type;
```

---

## Funciones del Driver

### Funciones de Inicialización

#### `UART_Init()`
```c
void UART_Init(LPC_UART_TypeDef *UARTx, UART_CFG_Type *UART_ConfigStruct);
```

Inicializa el UART con la configuración especificada.

**Parámetros:**
- `UARTx`: UART a usar (`LPC_UART0`, `LPC_UART1`, `LPC_UART2`, `LPC_UART3`)
- `UART_ConfigStruct`: Puntero a estructura de configuración

**Comportamiento interno** (verificado en lpc17xx_uart.c:186-380):
1. Enciende el reloj del UART (PCONP)
2. Resetea y limpia FIFOs
3. Limpia registros RBR/THR
4. Deshabilita interrupciones
5. Calcula y configura divisores para el baudrate
6. Configura LCR (Line Control Register): data bits, parity, stop bits

**Ejemplo:**
```c
UART_CFG_Type UARTConfigStruct;
UARTConfigStruct.Baud_rate = 9600;
UARTConfigStruct.Databits = UART_DATABIT_8;
UARTConfigStruct.Parity = UART_PARITY_NONE;
UARTConfigStruct.Stopbits = UART_STOPBIT_1;
UART_Init(LPC_UART0, &UARTConfigStruct);
```

#### `UART_ConfigStructInit()`
```c
void UART_ConfigStructInit(UART_CFG_Type *UART_InitStruct);
```

Inicializa la estructura de configuración con valores por defecto.

**Valores por defecto** (verificado en lpc17xx_uart.c:442-448):
- Baud_rate: 9600
- Databits: 8 bits
- Parity: None
- Stopbits: 1

**Ejemplo:**
```c
UART_CFG_Type UARTConfigStruct;
UART_ConfigStructInit(&UARTConfigStruct);  // 9600 bps, 8N1
// Modificar si es necesario:
UARTConfigStruct.Baud_rate = 115200;
UART_Init(LPC_UART0, &UARTConfigStruct);
```

#### `UART_DeInit()`
```c
void UART_DeInit(LPC_UART_TypeDef* UARTx);
```

Deinicializa el UART y apaga su reloj.

**Parámetros:**
- `UARTx`: UART a deinicializar

**Ejemplo:**
```c
UART_DeInit(LPC_UART0);
```

### Funciones de FIFO

#### `UART_FIFOConfig()`
```c
void UART_FIFOConfig(LPC_UART_TypeDef *UARTx, UART_FIFO_CFG_Type *FIFOCfg);
```

Configura los FIFOs del UART.

**Parámetros:**
- `UARTx`: UART a configurar
- `FIFOCfg`: Puntero a estructura de configuración FIFO

**Ejemplo:**
```c
UART_FIFO_CFG_Type UARTFIFOConfigStruct;
UARTFIFOConfigStruct.FIFO_ResetRxBuf = ENABLE;       // Resetear RX FIFO
UARTFIFOConfigStruct.FIFO_ResetTxBuf = ENABLE;       // Resetear TX FIFO
UARTFIFOConfigStruct.FIFO_DMAMode = DISABLE;         // Sin DMA
UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV2; // IRQ con 8 bytes
UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);
```

#### `UART_FIFOConfigStructInit()`
```c
void UART_FIFOConfigStructInit(UART_FIFO_CFG_Type *UART_FIFOInitStruct);
```

Inicializa la estructura FIFO con valores por defecto:
- FIFO habilitado
- DMA deshabilitado
- Trigger level 0 (1 carácter)
- Reset de ambos FIFOs

**Ejemplo:**
```c
UART_FIFO_CFG_Type UARTFIFOConfigStruct;
UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);
```

### Funciones de Transmisión y Recepción

#### `UART_SendByte()`
```c
void UART_SendByte(LPC_UART_TypeDef* UARTx, uint8_t Data);
```

Transmite un byte individual. **Bloqueante** hasta que THR esté listo.

**Parámetros:**
- `UARTx`: UART a usar
- `Data`: Byte a transmitir (8 bits)

**Comportamiento** (verificado en lpc17xx_uart.c:461-474):
Escribe directamente en THR (Transmit Holding Register).

**Ejemplo:**
```c
UART_SendByte(LPC_UART0, 'A');
UART_SendByte(LPC_UART0, '\n');
```

#### `UART_ReceiveByte()`
```c
uint8_t UART_ReceiveByte(LPC_UART_TypeDef* UARTx);
```

Recibe un byte individual. **No verifica si hay datos disponibles**.

**Parámetros:**
- `UARTx`: UART a usar

**Retorna:** Byte recibido del RBR (Receive Buffer Register)

**Comportamiento** (verificado en lpc17xx_uart.c:486-498):
Lee directamente de RBR.

**Ejemplo:**
```c
// Verificar primero si hay datos
if (UART_GetLineStatus(LPC_UART0) & UART_LSR_RDR) {
    uint8_t received = UART_ReceiveByte(LPC_UART0);
    // Procesar byte recibido
}
```

#### `UART_Send()`
```c
uint32_t UART_Send(LPC_UART_TypeDef *UARTx, uint8_t *txbuf,
                   uint32_t buflen, TRANSFER_BLOCK_Type flag);
```

Transmite un buffer de datos.

**Parámetros:**
- `UARTx`: UART a usar
- `txbuf`: Puntero al buffer de transmisión
- `buflen`: Cantidad de bytes a transmitir
- `flag`:
  - `BLOCKING`: Bloquea hasta transmitir todo
  - `NONE_BLOCKING`: Transmite lo que pueda y retorna

**Retorna:** Cantidad de bytes efectivamente transmitidos

**Ejemplo:**
```c
uint8_t message[] = "Hello World\r\n";
UART_Send(LPC_UART0, message, sizeof(message)-1, BLOCKING);
```

#### `UART_Receive()`
```c
uint32_t UART_Receive(LPC_UART_TypeDef *UARTx, uint8_t *rxbuf,
                      uint32_t buflen, TRANSFER_BLOCK_Type flag);
```

Recibe datos en un buffer.

**Parámetros:**
- `UARTx`: UART a usar
- `rxbuf`: Puntero al buffer de recepción
- `buflen`: Tamaño máximo del buffer
- `flag`:
  - `BLOCKING`: Bloquea hasta recibir `buflen` bytes
  - `NONE_BLOCKING`: Recibe lo disponible y retorna

**Retorna:** Cantidad de bytes efectivamente recibidos

**Ejemplo:**
```c
uint8_t buffer[32];
uint32_t len = UART_Receive(LPC_UART0, buffer, sizeof(buffer), NONE_BLOCKING);
if (len > 0) {
    // Procesar datos recibidos
}
```

### Funciones de Control

#### `UART_TxCmd()`
```c
void UART_TxCmd(LPC_UART_TypeDef *UARTx, FunctionalState NewState);
```

Habilita o deshabilita el transmisor.

**Parámetros:**
- `UARTx`: UART a controlar
- `NewState`: `ENABLE` o `DISABLE`

**Importante:** Debe llamarse antes de transmitir datos.

**Ejemplo:**
```c
UART_TxCmd(LPC_UART0, ENABLE);   // Habilitar transmisor
UART_Send(LPC_UART0, data, len, BLOCKING);
UART_TxCmd(LPC_UART0, DISABLE);  // Deshabilitar si es necesario
```

#### `UART_CheckBusy()`
```c
FlagStatus UART_CheckBusy(LPC_UART_TypeDef *UARTx);
```

Verifica si el UART está ocupado transmitiendo.

**Parámetros:**
- `UARTx`: UART a verificar

**Retorna:**
- `SET`: UART ocupado (TX en progreso)
- `RESET`: UART libre

**Ejemplo:**
```c
UART_Send(LPC_UART0, data, len, BLOCKING);
while (UART_CheckBusy(LPC_UART0) == SET);  // Esperar completar TX
```

### Funciones de Interrupciones

#### `UART_IntConfig()`
```c
void UART_IntConfig(LPC_UART_TypeDef *UARTx, UART_INT_Type UARTIntCfg,
                    FunctionalState NewState);
```

Habilita o deshabilita una interrupción específica.

**Parámetros:**
- `UARTx`: UART a configurar
- `UARTIntCfg`: Tipo de interrupción (ver `UART_INT_Type`)
- `NewState`: `ENABLE` o `DISABLE`

**Tipos de interrupciones más comunes:**
- `UART_INTCFG_RBR`: Datos recibidos disponibles
- `UART_INTCFG_THRE`: THR vacío (listo para transmitir)
- `UART_INTCFG_RLS`: Error en línea (overrun, parity, framing)

**Ejemplo:**
```c
UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);  // Habilitar RX INT
UART_IntConfig(LPC_UART0, UART_INTCFG_RLS, ENABLE);  // Habilitar errores
NVIC_EnableIRQ(UART0_IRQn);  // Habilitar en NVIC
```

#### `UART_GetIntId()`
```c
uint32_t UART_GetIntId(LPC_UART_TypeDef* UARTx);
```

Obtiene el ID de la interrupción pendiente.

**Parámetros:**
- `UARTx`: UART a verificar

**Retorna:** Valor del IIR (Interrupt Identification Register)

**Valores importantes:**
- `UART_IIR_INTID_RLS`: Error en línea (máxima prioridad)
- `UART_IIR_INTID_RDA`: Datos recibidos disponibles
- `UART_IIR_INTID_CTI`: Character timeout (FIFO parcialmente lleno)
- `UART_IIR_INTID_THRE`: THR vacío

**Ejemplo:**
```c
void UART0_IRQHandler(void) {
    uint32_t intsrc = UART_GetIntId(LPC_UART0);
    uint32_t tmp = intsrc & UART_IIR_INTID_MASK;

    if (tmp == UART_IIR_INTID_RDA) {
        // Procesar datos recibidos
    } else if (tmp == UART_IIR_INTID_THRE) {
        // THR vacío, enviar más datos
    }
}
```

#### `UART_GetLineStatus()`
```c
uint8_t UART_GetLineStatus(LPC_UART_TypeDef* UARTx);
```

Lee el Line Status Register (LSR).

**Parámetros:**
- `UARTx`: UART a leer

**Retorna:** Valor del LSR (ver tabla en sección "Conceptos Fundamentales")

**Ejemplo:**
```c
uint8_t lsr = UART_GetLineStatus(LPC_UART0);
if (lsr & UART_LSR_RDR) {
    // Datos disponibles
}
if (lsr & UART_LSR_OE) {
    // Overrun error - datos perdidos
}
if (lsr & UART_LSR_PE) {
    // Parity error
}
```

---

## Ejemplos Prácticos

### Ejemplo 1: UART en Modo Polling (Echo Simple)

**Verificado en:** `library/examples/UART/Polling/uart_polling_test.c`

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    UART_CFG_Type UARTConfigStruct;
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;
    PINSEL_CFG_Type PinCfg;
    uint8_t buffer[10];
    uint32_t len;

    // 1. Configurar pines UART0: P0.2 (TXD0), P0.3 (RXD0)
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 2;  // TXD0
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 3;  // RXD0
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar UART con valores por defecto (9600 bps, 8N1)
    UART_ConfigStructInit(&UARTConfigStruct);
    UART_Init(LPC_UART0, &UARTConfigStruct);

    // 3. Configurar FIFO
    UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
    UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);

    // 4. Habilitar transmisor
    UART_TxCmd(LPC_UART0, ENABLE);

    // 5. Enviar mensaje de bienvenida
    uint8_t msg[] = "UART Polling Demo - Echo Test\r\n";
    UART_Send(LPC_UART0, msg, sizeof(msg)-1, BLOCKING);

    // 6. Loop principal: echo de caracteres
    while(1) {
        len = UART_Receive(LPC_UART0, buffer, sizeof(buffer), NONE_BLOCKING);
        if (len > 0) {
            // Echo back
            UART_Send(LPC_UART0, buffer, len, BLOCKING);
        }
    }
}
```

**Explicación:**
- **Modo polling**: No usa interrupciones, consulta continuamente
- **NONE_BLOCKING**: `UART_Receive()` retorna inmediatamente con datos disponibles
- **BLOCKING**: `UART_Send()` espera hasta transmitir todo
- **Echo**: Devuelve cada carácter recibido

---

### Ejemplo 2: UART con Interrupciones y Ring Buffer

**Verificado en:** `library/examples/UART/Interrupt/uart_interrupt_test.c`

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_nvic.h"

// Ring buffer para RX y TX
#define UART_RING_BUFSIZE 256
#define __BUF_MASK (UART_RING_BUFSIZE-1)
#define __BUF_IS_FULL(head, tail) ((tail&__BUF_MASK)==((head+1)&__BUF_MASK))
#define __BUF_IS_EMPTY(head, tail) ((head&__BUF_MASK)==(tail&__BUF_MASK))
#define __BUF_INCR(bufidx) (bufidx=(bufidx+1)&__BUF_MASK)

typedef struct {
    __IO uint32_t tx_head, tx_tail;
    __IO uint32_t rx_head, rx_tail;
    __IO uint8_t  tx[UART_RING_BUFSIZE];
    __IO uint8_t  rx[UART_RING_BUFSIZE];
} UART_RING_BUFFER_T;

UART_RING_BUFFER_T rb;
volatile FlagStatus TxIntStat;

// ISR de UART0
void UART0_IRQHandler(void) {
    uint32_t intsrc = UART_GetIntId(LPC_UART0);
    uint32_t tmp = intsrc & UART_IIR_INTID_MASK;

    // Receive Line Status (errores)
    if (tmp == UART_IIR_INTID_RLS) {
        uint8_t lsr = UART_GetLineStatus(LPC_UART0);
        lsr &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);
        if (lsr) {
            // Manejar errores
        }
    }

    // Receive Data Available o Character Timeout
    if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI)) {
        UART_IntReceive();
    }

    // Transmit Holding Empty
    if (tmp == UART_IIR_INTID_THRE) {
        UART_IntTransmit();
    }
}

void UART_IntReceive(void) {
    uint8_t tmpc;
    uint32_t rLen;

    while(1) {
        rLen = UART_Receive(LPC_UART0, &tmpc, 1, NONE_BLOCKING);
        if (rLen) {
            // Guardar en ring buffer RX
            if (!__BUF_IS_FULL(rb.rx_head, rb.rx_tail)) {
                rb.rx[rb.rx_head] = tmpc;
                __BUF_INCR(rb.rx_head);
            }
        } else {
            break;  // No más datos
        }
    }
}

void UART_IntTransmit(void) {
    // Deshabilitar INT temporalmente
    UART_IntConfig(LPC_UART0, UART_INTCFG_THRE, DISABLE);

    // Esperar THR vacío
    while (UART_CheckBusy(LPC_UART0) == SET);

    // Transmitir desde ring buffer
    while (!__BUF_IS_EMPTY(rb.tx_head, rb.tx_tail)) {
        if (UART_Send(LPC_UART0, (uint8_t *)&rb.tx[rb.tx_tail], 1, NONE_BLOCKING)) {
            __BUF_INCR(rb.tx_tail);
        } else {
            break;
        }
    }

    // Re-habilitar INT si hay más datos
    if (!__BUF_IS_EMPTY(rb.tx_head, rb.tx_tail)) {
        TxIntStat = SET;
        UART_IntConfig(LPC_UART0, UART_INTCFG_THRE, ENABLE);
    } else {
        TxIntStat = RESET;
    }
}

uint32_t UARTSend(uint8_t *txbuf, uint8_t buflen) {
    uint8_t *data = txbuf;
    uint32_t bytes = 0;

    // Deshabilitar INT temporalmente
    UART_IntConfig(LPC_UART0, UART_INTCFG_THRE, DISABLE);

    // Copiar a ring buffer TX
    while ((buflen > 0) && (!__BUF_IS_FULL(rb.tx_head, rb.tx_tail))) {
        rb.tx[rb.tx_head] = *data++;
        __BUF_INCR(rb.tx_head);
        bytes++;
        buflen--;
    }

    // Iniciar transmisión si estaba detenida
    if (TxIntStat == RESET) {
        UART_IntTransmit();
    } else {
        UART_IntConfig(LPC_UART0, UART_INTCFG_THRE, ENABLE);
    }

    return bytes;
}

uint32_t UARTReceive(uint8_t *rxbuf, uint8_t buflen) {
    uint8_t *data = rxbuf;
    uint32_t bytes = 0;

    // Deshabilitar INT temporalmente
    UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, DISABLE);

    // Leer de ring buffer RX
    while ((buflen > 0) && (!__BUF_IS_EMPTY(rb.rx_head, rb.rx_tail))) {
        *data++ = rb.rx[rb.rx_tail];
        __BUF_INCR(rb.rx_tail);
        bytes++;
        buflen--;
    }

    // Re-habilitar INT
    UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);

    return bytes;
}

int main(void) {
    UART_CFG_Type UARTConfigStruct;
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // 1. Configurar pines
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 2;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 3;
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar UART
    UART_ConfigStructInit(&UARTConfigStruct);
    UART_Init(LPC_UART0, &UARTConfigStruct);

    // 3. Configurar FIFO
    UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
    UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);

    // 4. Habilitar transmisor
    UART_TxCmd(LPC_UART0, ENABLE);

    // 5. Habilitar interrupciones
    UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);  // RX
    UART_IntConfig(LPC_UART0, UART_INTCFG_RLS, ENABLE);  // Errores

    // 6. Resetear ring buffers
    rb.rx_head = rb.rx_tail = 0;
    rb.tx_head = rb.tx_tail = 0;
    TxIntStat = RESET;

    // 7. Configurar NVIC
    NVIC_SetPriority(UART0_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(UART0_IRQn);

    // 8. Enviar mensaje de bienvenida
    uint8_t msg[] = "UART Interrupt Demo with Ring Buffer\r\n";
    UARTSend(msg, sizeof(msg)-1);

    // 9. Loop principal
    uint8_t buffer[10];
    uint32_t len;
    while(1) {
        len = UARTReceive(buffer, sizeof(buffer));
        if (len > 0) {
            // Echo back
            UARTSend(buffer, len);
        }
    }
}
```

**Ventajas del modo con interrupciones:**
- **No bloqueante**: El CPU puede hacer otras tareas
- **Eficiente**: Solo procesa cuando hay datos
- **Ring buffer**: Evita pérdida de datos si el main loop está ocupado

---

### Ejemplo 3: Cambiar Baudrate en Runtime

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void change_baudrate(uint32_t new_baudrate) {
    // 1. Deshabilitar transmisor
    UART_TxCmd(LPC_UART0, DISABLE);

    // 2. Esperar que TX termine
    while (UART_CheckBusy(LPC_UART0) == SET);

    // 3. Reconfigurar con nuevo baudrate
    UART_CFG_Type UARTConfigStruct;
    UARTConfigStruct.Baud_rate = new_baudrate;
    UARTConfigStruct.Databits = UART_DATABIT_8;
    UARTConfigStruct.Parity = UART_PARITY_NONE;
    UARTConfigStruct.Stopbits = UART_STOPBIT_1;
    UART_Init(LPC_UART0, &UARTConfigStruct);

    // 4. Re-habilitar transmisor
    UART_TxCmd(LPC_UART0, ENABLE);
}

int main(void) {
    // ... configuración de pines ...

    // Iniciar con 9600 bps
    UART_CFG_Type UARTConfigStruct;
    UARTConfigStruct.Baud_rate = 9600;
    UARTConfigStruct.Databits = UART_DATABIT_8;
    UARTConfigStruct.Parity = UART_PARITY_NONE;
    UARTConfigStruct.Stopbits = UART_STOPBIT_1;
    UART_Init(LPC_UART0, &UARTConfigStruct);

    UART_TxCmd(LPC_UART0, ENABLE);

    uint8_t msg1[] = "Running at 9600 bps\r\n";
    UART_Send(LPC_UART0, msg1, sizeof(msg1)-1, BLOCKING);

    // Cambiar a 115200 bps
    change_baudrate(115200);

    uint8_t msg2[] = "Now running at 115200 bps\r\n";
    UART_Send(LPC_UART0, msg2, sizeof(msg2)-1, BLOCKING);

    while(1);
}
```

---

### Ejemplo 4: Printf via UART (Redirección de stdio)

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include <stdio.h>

// Redirigir fputc a UART0
int fputc(int ch, FILE *f) {
    UART_SendByte(LPC_UART0, (uint8_t)ch);
    return ch;
}

int main(void) {
    UART_CFG_Type UARTConfigStruct;
    PINSEL_CFG_Type PinCfg;

    // Configurar pines
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 2;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 3;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar UART
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = 115200;  // Alta velocidad para printf
    UART_Init(LPC_UART0, &UARTConfigStruct);
    UART_TxCmd(LPC_UART0, ENABLE);

    // Ahora podemos usar printf
    printf("Hello from LPC1769!\r\n");
    printf("System running at %d MHz\r\n", SystemCoreClock/1000000);

    int counter = 0;
    while(1) {
        printf("Counter: %d\r\n", counter++);
        // Delay
        for(volatile int i=0; i<1000000; i++);
    }
}
```

**Nota:** Para que `printf()` funcione correctamente, puede requerir configuración adicional del toolchain (semihosting o retargeting).

---

## Errores Comunes y Soluciones

### Error 1: No se reciben o transmiten datos

**Síntoma:** UART configurado pero no hay comunicación

**Causas y soluciones:**

1. **Pines no configurados:**
```c
// INCORRECTO: Olvidar configurar PINSEL
UART_Init(LPC_UART0, &UARTConfigStruct);

// CORRECTO: Configurar pines primero
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 1;
PinCfg.Portnum = 0;
PinCfg.Pinnum = 2;
PINSEL_ConfigPin(&PinCfg);
PinCfg.Pinnum = 3;
PINSEL_ConfigPin(&PinCfg);
UART_Init(LPC_UART0, &UARTConfigStruct);
```

2. **Transmisor no habilitado:**
```c
// INCORRECTO: Olvidar habilitar TX
UART_Init(LPC_UART0, &UARTConfigStruct);
UART_Send(LPC_UART0, data, len, BLOCKING);  // No transmite

// CORRECTO:
UART_Init(LPC_UART0, &UARTConfigStruct);
UART_TxCmd(LPC_UART0, ENABLE);  // ¡Crítico!
UART_Send(LPC_UART0, data, len, BLOCKING);
```

3. **Baudrate incorrecto:**
```c
// Ambos lados deben usar el MISMO baudrate
// LPC1769: 115200 bps
// PC/Terminal: Debe estar configurado también a 115200 bps
```

### Error 2: Caracteres corruptos o basura

**Síntoma:** Se reciben caracteres incorrectos

**Causas:**

1. **Mismatch de configuración:**
```c
// INCORRECTO: LPC1769 usa 8N1, pero PC usa 7E1
// Resultado: Caracteres incorrectos

// CORRECTO: Ambos lados con la misma configuración
UARTConfigStruct.Databits = UART_DATABIT_8;
UARTConfigStruct.Parity = UART_PARITY_NONE;
UARTConfigStruct.Stopbits = UART_STOPBIT_1;
// Terminal PC: 115200, 8, N, 1
```

2. **Baudrate error > 3%:**
```c
// El driver verifica error < 3% (lpc17xx_uart.c:137)
// Si el PCLK no permite un baudrate preciso, puede haber errores

// Verificar:
// - PCLK configurado correctamente
// - Baudrate estándar (9600, 19200, 38400, 57600, 115200)
```

### Error 3: Datos perdidos (Overrun Error)

**Síntoma:** `UART_LSR_OE` se setea, datos perdidos

**Causas:**

1. **No leer RX lo suficientemente rápido:**
```c
// INCORRECTO: Loop lento, RX FIFO se llena
while(1) {
    long_task();  // Tarea larga
    len = UART_Receive(LPC_UART0, buffer, sizeof(buffer), NONE_BLOCKING);
}

// CORRECTO: Usar interrupciones para no perder datos
UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);
NVIC_EnableIRQ(UART0_IRQn);
```

2. **FIFO trigger level muy alto:**
```c
// INCORRECTO: Trigger 14 bytes, pero datos llegan rápido
UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV3;  // 14 bytes

// CORRECTO: Trigger más bajo para respuesta rápida
UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV1;  // 4 bytes
```

### Error 4: Interrupción no se ejecuta

**Síntoma:** ISR nunca se llama

**Causas:**

1. **No habilitar en NVIC:**
```c
// INCORRECTO: Falta NVIC_EnableIRQ
UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);

// CORRECTO:
UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);
NVIC_EnableIRQ(UART0_IRQn);  // ¡No olvidar!
```

2. **Nombre de ISR incorrecto:**
```c
// INCORRECTO: Nombre no coincide con vector table
void UART_0_IRQHandler(void) {  // Mal nombre
    // ...
}

// CORRECTO:
void UART0_IRQHandler(void) {  // Nombre exacto
    // ...
}
```

### Error 5: Ring buffer se llena

**Síntoma:** Datos se pierden en modo interrupt

**Causa:** Ring buffer muy pequeño o main loop no consume datos

```c
// INCORRECTO: Buffer pequeño y no se lee
#define UART_RING_BUFSIZE 16  // Muy pequeño

// CORRECTO:
#define UART_RING_BUFSIZE 256  // Más grande

// Y en main loop, consumir datos regularmente:
while(1) {
    uint32_t len = UARTReceive(buffer, sizeof(buffer));
    if (len > 0) {
        process_data(buffer, len);  // Procesar inmediatamente
    }
}
```

---

## Patrón de Uso Típico

### Configuración Estándar (Modo Polling)

```c
// 1. Configurar pines UART
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 1;          // UART0: función 1
PinCfg.OpenDrain = 0;
PinCfg.Pinmode = 0;
PinCfg.Portnum = 0;
PinCfg.Pinnum = 2;           // TXD0
PINSEL_ConfigPin(&PinCfg);
PinCfg.Pinnum = 3;           // RXD0
PINSEL_ConfigPin(&PinCfg);

// 2. Inicializar UART (valores por defecto o personalizados)
UART_CFG_Type UARTConfigStruct;
UART_ConfigStructInit(&UARTConfigStruct);
UARTConfigStruct.Baud_rate = 115200;  // Cambiar baudrate si se desea
UART_Init(LPC_UART0, &UARTConfigStruct);

// 3. Configurar FIFO (opcional, usa defaults si no se llama)
UART_FIFO_CFG_Type UARTFIFOConfigStruct;
UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);

// 4. Habilitar transmisor
UART_TxCmd(LPC_UART0, ENABLE);

// 5. Usar UART
uint8_t msg[] = "Hello\r\n";
UART_Send(LPC_UART0, msg, sizeof(msg)-1, BLOCKING);

uint8_t buffer[32];
uint32_t len = UART_Receive(LPC_UART0, buffer, sizeof(buffer), NONE_BLOCKING);
```

### Configuración con Interrupciones

```c
// 1-4. Igual que polling (configurar pines, init, FIFO, TX enable)
// ...

// 5. Habilitar interrupciones en UART
UART_IntConfig(LPC_UART0, UART_INTCFG_RBR, ENABLE);   // RX
UART_IntConfig(LPC_UART0, UART_INTCFG_RLS, ENABLE);   // Errores

// 6. Configurar NVIC
NVIC_SetPriority(UART0_IRQn, ((0x01<<3)|0x01));
NVIC_EnableIRQ(UART0_IRQn);

// 7. Implementar ISR
void UART0_IRQHandler(void) {
    uint32_t intsrc = UART_GetIntId(LPC_UART0);
    uint32_t tmp = intsrc & UART_IIR_INTID_MASK;

    if (tmp == UART_IIR_INTID_RDA || tmp == UART_IIR_INTID_CTI) {
        // Datos recibidos
        uint8_t byte = UART_ReceiveByte(LPC_UART0);
        // Procesar byte
    }

    if (tmp == UART_IIR_INTID_THRE) {
        // TX buffer vacío, enviar más datos si hay
    }

    if (tmp == UART_IIR_INTID_RLS) {
        // Error en línea
        uint8_t lsr = UART_GetLineStatus(LPC_UART0);
        // Manejar errores
    }
}
```

---

## Notas Importantes sobre LPC1769

1. **4 UARTs idénticos:** UART0, UART1, UART2, UART3 tienen las mismas funciones básicas.

2. **Características especiales:**
   - **UART1**: Full modem (RTS/CTS/DTR/DSR/RI/DCD), RS-485
   - **UART3**: IrDA

3. **PCLK:** El baudrate depende del PCLK. El driver lo calcula automáticamente, pero si se cambia el PCLK del sistema, reinicializar el UART.

4. **FIFO:** Los FIFOs de 16 bytes son críticos para aplicaciones con alta tasa de datos. Configurar trigger level apropiado.

5. **Line Status:** Siempre verificar errores de línea (OE, PE, FE) en aplicaciones críticas.

6. **Vectores de interrupción:**
   - UART0: `UART0_IRQn` → `UART0_IRQHandler()`
   - UART1: `UART1_IRQn` → `UART1_IRQHandler()`
   - UART2: `UART2_IRQn` → `UART2_IRQHandler()`
   - UART3: `UART3_IRQn` → `UART3_IRQHandler()`

7. **Divisor fraccionario:** El driver usa FDR (Fractional Divider Register) para obtener baudrates precisos (lpc17xx_uart.c:146-147).

8. **Blocking timeout:** El timeout para modo BLOCKING es `0xFFFFFFFF` (prácticamente infinito). No usar BLOCKING si puede haber pérdida de conexión.

---

## Referencias

- **Driver header:** `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_uart.h`
- **Driver source:** `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_uart.c`
- **Ejemplos verificados:**
  - `library/examples/UART/Polling/uart_polling_test.c`
  - `library/examples/UART/Interrupt/uart_interrupt_test.c`
  - `library/examples/UART/AutoBaud/uart_autobaud_test.c`
  - `library/examples/UART/UART1_FullModem/uart_fullmodem_test.c`
  - `library/examples/UART/RS485_Master/rs485_master.c`
- **Manual del usuario:** LPC17xx User Manual (UM10360), Capítulo 14: UART
- **Pin functions:** Tabla 8.5 del manual del usuario LPC17xx

---

## Resumen

El **driver UART del LPC1769** ofrece comunicación serial completa:

- **4 UARTs** para múltiples interfaces simultáneas
- **Modos flexibles**: Polling (simple) e Interrupciones (eficiente)
- **FIFOs de 16 bytes** para optimizar rendimiento
- **Baudrates configurables** con cálculo automático de divisores
- **Detección de errores** (overrun, parity, framing)

**Puntos clave:**
1. Siempre configurar PINSEL antes de inicializar UART
2. Habilitar transmisor con `UART_TxCmd(ENABLE)`
3. Para aplicaciones con alta tasa de datos, usar modo con interrupciones y ring buffers
4. Verificar Line Status para detectar errores de comunicación
5. Ambos lados deben usar la misma configuración (baudrate, data bits, parity, stop bits)

El UART es fundamental para debugging, comunicación con PC, y interfaces con sensores/módulos externos.
