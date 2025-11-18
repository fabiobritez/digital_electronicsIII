# Tutorial de GPDMA (General Purpose DMA) para LPC1769

**Autor:** Tutorial generado para la materia Electrónica Digital III
**Fecha:** 2024
**Microcontrolador:** LPC1769 (ARM Cortex-M3)
**Archivos fuente verificados:**
- `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_gpdma.h`
- `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_gpdma.c`
- `library/examples/GPDMA/Ram_2_Ram_Test/gpdma_r2r_test.c`
- `library/examples/GPDMA/Link_list/link_list.c`
- `library/examples/UART/DMA/uart_dma_test.c`
- `library/examples/ADC/DMA/adc_dma_test.c`

---

## Índice

1. [Introducción](#1-introducción)
2. [Características del GPDMA](#2-características-del-gpdma)
3. [Arquitectura del GPDMA](#3-arquitectura-del-gpdma)
4. [Canales DMA](#4-canales-dma)
5. [Tipos de transferencia](#5-tipos-de-transferencia)
6. [Conexiones periféricas](#6-conexiones-periféricas)
7. [Configuración del GPDMA](#7-configuración-del-gpdma)
8. [Listas enlazadas (Linked Lists)](#8-listas-enlazadas-linked-lists)
9. [Interrupciones del GPDMA](#9-interrupciones-del-gpdma)
10. [Funciones del driver](#10-funciones-del-driver)
11. [Ejemplos prácticos](#11-ejemplos-prácticos)
12. [Errores comunes](#12-errores-comunes)
13. [Referencias](#13-referencias)

---

## 1. Introducción

El **GPDMA (General Purpose Direct Memory Access)** del LPC1769 es un controlador DMA de 8 canales que permite transferir datos entre memoria y periféricos (o entre memorias) **sin intervención de la CPU**. Esto libera al procesador para realizar otras tareas mientras el DMA maneja las transferencias de datos.

### ¿Cuándo usar DMA?

- **Transferencias grandes de datos** desde/hacia periféricos (UART, ADC, DAC, SSP, I2S)
- **Generación de señales complejas** con DAC (ej: ondas senoidales)
- **Copia de bloques de memoria** de forma eficiente
- **Reducir carga de CPU** en aplicaciones que requieren transferencias continuas

### Ventajas del DMA

✅ **Libera la CPU** para otras tareas
✅ **Transferencias rápidas** sin overhead de software
✅ **Bajo consumo de energía** (CPU puede entrar en sleep)
✅ **Soporte para múltiples periféricos** simultáneamente

---

## 2. Características del GPDMA

### Características principales (verificadas en lpc17xx_gpdma.h)

- **8 canales DMA independientes** (canal 0 = mayor prioridad, canal 7 = menor prioridad)
- **4 tipos de transferencia:**
  - Memory-to-Memory (M2M)
  - Memory-to-Peripheral (M2P)
  - Peripheral-to-Memory (P2M)
  - Peripheral-to-Peripheral (P2P)

- **Anchos de transferencia:** 8 bits (byte), 16 bits (halfword), 32 bits (word)
- **Tamaños de burst:** 1, 4, 8, 16, 32, 64, 128, 256 transferencias
- **24 conexiones periféricas** soportadas
- **Linked List support:** transferencias encadenadas
- **Interrupciones:** Terminal Count (TC) y Error por canal

### Especificaciones técnicas

| Característica | Especificación |
|----------------|----------------|
| Número de canales | 8 (0-7) |
| Prioridad de canales | Canal 0 = máxima, Canal 7 = mínima |
| Ancho de bus AHB | 32 bits |
| Máx. transferencia por vez | 4095 ítems |
| Direccionamiento | Incremento/decremento de dirección |
| Interrupciones | 2 por canal (TC y Error) |

---

## 3. Arquitectura del GPDMA

### Diagrama de bloques simplificado

```
                    ┌──────────────────────────────────┐
                    │      GPDMA Controller            │
                    │                                  │
   ┌────────────────┤  8 Canales DMA (0-7)            │
   │                │  Prioridad: 0 > 1 > ... > 7     │
   │                └──────────────┬───────────────────┘
   │                               │
   │         ┌─────────────────────┴──────────────────┐
   │         │                                        │
   ▼         ▼                                        ▼
┌──────┐  ┌──────┐                              ┌──────────┐
│ RAM  │  │ Flash│                              │Periféricos│
│      │  │      │                              │ SSP, UART│
│      │  │      │                              │ ADC, DAC │
└──────┘  └──────┘                              │ I2S, etc │
                                                 └──────────┘
```

### Flujo de transferencia DMA

1. **CPU configura el canal DMA:**
   - Dirección fuente y destino
   - Tamaño de transferencia
   - Tipo de transferencia
   - Ancho de datos y burst size

2. **CPU habilita el canal DMA**

3. **DMA toma control del bus AHB** y transfiere datos

4. **Al finalizar:**
   - DMA genera interrupción Terminal Count (TC)
   - El canal se deshabilita automáticamente (o continúa con linked list)

---

## 4. Canales DMA

El LPC1769 tiene **8 canales DMA** (0-7) independientes.

### Prioridad de canales

Los canales tienen prioridad fija basada en su número:

```
Canal 0  ←  Prioridad más alta
Canal 1
Canal 2
Canal 3
Canal 4
Canal 5
Canal 6
Canal 7  ←  Prioridad más baja
```

**⚠️ IMPORTANTE:** Si dos canales solicitan el bus simultáneamente, **el canal con número más bajo tiene prioridad**.

### Registro de canales habilitados

Puedes verificar qué canales están activos usando:

```c
if (LPC_GPDMA->DMACEnbldChns & (1 << channel_num)) {
    // El canal está habilitado
}
```

---

## 5. Tipos de transferencia

El GPDMA soporta 4 tipos de transferencia (definidos en `lpc17xx_gpdma.h:78-81`):

### 5.1. Memory-to-Memory (M2M)

Copia datos entre dos regiones de memoria.

```c
#define GPDMA_TRANSFERTYPE_M2M    ((0UL))  // Memoria a Memoria
```

**Características:**
- Ambas direcciones (fuente y destino) se incrementan
- Burst size óptimo: 32 (línea 263 en lpc17xx_gpdma.c)
- **Uso típico:** Copiar buffers, inicializar memoria

**Ejemplo de configuración:**
```c
GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
GPDMACfg.SrcMemAddr = (uint32_t)&source_buffer;
GPDMACfg.DstMemAddr = (uint32_t)&dest_buffer;
GPDMACfg.TransferSize = BUFFER_SIZE;
GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;  // 32 bits
GPDMACfg.SrcConn = 0;  // No usado en M2M
GPDMACfg.DstConn = 0;  // No usado en M2M
```

### 5.2. Memory-to-Peripheral (M2P)

Transfiere datos desde memoria hacia un periférico.

```c
#define GPDMA_TRANSFERTYPE_M2P    ((1UL))  // Memoria a Periférico
```

**Características:**
- Dirección fuente se incrementa
- Dirección destino es fija (registro FIFO del periférico)
- El periférico controla el ritmo de transferencia
- **Uso típico:** Transmitir por UART, enviar datos al DAC, transmitir por SSP

**Ejemplo de configuración (UART Tx):**
```c
GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
GPDMACfg.SrcMemAddr = (uint32_t)&tx_buffer;
GPDMACfg.DstMemAddr = 0;  // No usado, se obtiene de tabla interna
GPDMACfg.TransferSize = strlen(tx_buffer);
GPDMACfg.SrcConn = 0;  // No usado en M2P
GPDMACfg.DstConn = GPDMA_CONN_UART0_Tx;  // Destino: UART0 Tx
```

### 5.3. Peripheral-to-Memory (P2M)

Transfiere datos desde un periférico hacia memoria.

```c
#define GPDMA_TRANSFERTYPE_P2M    ((2UL))  // Periférico a Memoria
```

**Características:**
- Dirección fuente es fija (registro FIFO del periférico)
- Dirección destino se incrementa
- El periférico controla el ritmo de transferencia
- **Uso típico:** Recibir por UART, adquirir datos del ADC

**Ejemplo de configuración (ADC):**
```c
GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
GPDMACfg.SrcMemAddr = 0;  // No usado, se obtiene de tabla interna
GPDMACfg.DstMemAddr = (uint32_t)&adc_buffer;
GPDMACfg.TransferSize = NUM_SAMPLES;
GPDMACfg.SrcConn = GPDMA_CONN_ADC;  // Fuente: ADC
GPDMACfg.DstConn = 0;  // No usado en P2M
```

### 5.4. Peripheral-to-Peripheral (P2P)

Transfiere datos directamente entre dos periféricos.

```c
#define GPDMA_TRANSFERTYPE_P2P    ((3UL))  // Periférico a Periférico
```

**Características:**
- Ambas direcciones son fijas
- Útil para routing de datos entre periféricos
- **Uso típico:** ADC → DAC, I2S → DAC

---

## 6. Conexiones periféricas

El GPDMA del LPC1769 soporta **24 conexiones periféricas** diferentes (definidas en `lpc17xx_gpdma.h:52-75`).

### Tabla completa de conexiones (verificada en lpc17xx_gpdma.c:86-111)

| Número | Constante | Periférico | Dirección | Burst | Ancho |
|--------|-----------|------------|-----------|-------|-------|
| 0 | `GPDMA_CONN_SSP0_Tx` | SSP0 | Transmit | 4 | Byte |
| 1 | `GPDMA_CONN_SSP0_Rx` | SSP0 | Receive | 4 | Byte |
| 2 | `GPDMA_CONN_SSP1_Tx` | SSP1 | Transmit | 4 | Byte |
| 3 | `GPDMA_CONN_SSP1_Rx` | SSP1 | Receive | 4 | Byte |
| 4 | `GPDMA_CONN_ADC` | ADC | - | 4 | Word |
| 5 | `GPDMA_CONN_I2S_Channel_0` | I2S | Channel 0 | 32 | Word |
| 6 | `GPDMA_CONN_I2S_Channel_1` | I2S | Channel 1 | 32 | Word |
| 7 | `GPDMA_CONN_DAC` | DAC | - | 1 | Byte |
| 8 | `GPDMA_CONN_UART0_Tx` | UART0 | Transmit | 1 | Byte |
| 9 | `GPDMA_CONN_UART0_Rx` | UART0 | Receive | 1 | Byte |
| 10 | `GPDMA_CONN_UART1_Tx` | UART1 | Transmit | 1 | Byte |
| 11 | `GPDMA_CONN_UART1_Rx` | UART1 | Receive | 1 | Byte |
| 12 | `GPDMA_CONN_UART2_Tx` | UART2 | Transmit | 1 | Byte |
| 13 | `GPDMA_CONN_UART2_Rx` | UART2 | Receive | 1 | Byte |
| 14 | `GPDMA_CONN_UART3_Tx` | UART3 | Transmit | 1 | Byte |
| 15 | `GPDMA_CONN_UART3_Rx` | UART3 | Receive | 1 | Byte |
| 16 | `GPDMA_CONN_MAT0_0` | TIMER0 | Match 0 | 1 | Word |
| 17 | `GPDMA_CONN_MAT0_1` | TIMER0 | Match 1 | 1 | Word |
| 18 | `GPDMA_CONN_MAT1_0` | TIMER1 | Match 0 | 1 | Word |
| 19 | `GPDMA_CONN_MAT1_1` | TIMER1 | Match 1 | 1 | Word |
| 20 | `GPDMA_CONN_MAT2_0` | TIMER2 | Match 0 | 1 | Word |
| 21 | `GPDMA_CONN_MAT2_1` | TIMER2 | Match 1 | 1 | Word |
| 22 | `GPDMA_CONN_MAT3_0` | TIMER3 | Match 0 | 1 | Word |
| 23 | `GPDMA_CONN_MAT3_1` | TIMER3 | Match 1 | 1 | Word |

**📝 Notas:**
- Los valores de Burst y Ancho son **valores optimizados** definidos en las tablas `GPDMA_LUTPerBurst` (líneas 130-155) y `GPDMA_LUTPerWid` (líneas 159-184) en `lpc17xx_gpdma.c`
- El driver usa estas tablas internamente para configurar automáticamente los parámetros óptimos

### Direcciones de periféricos (Tabla LUT)

El driver mantiene una tabla interna `GPDMA_LUTPerAddr[]` (líneas 86-111 en lpc17xx_gpdma.c) con las direcciones de los registros FIFO de cada periférico:

```c
const uint32_t GPDMA_LUTPerAddr[] = {
    ((uint32_t)&LPC_SSP0->DR),      // SSP0 Tx/Rx
    ((uint32_t)&LPC_SSP1->DR),      // SSP1 Tx/Rx
    ((uint32_t)&LPC_ADC->ADGDR),    // ADC
    ((uint32_t)&LPC_DAC->DACR),     // DAC
    ((uint32_t)&LPC_UART0->THR),    // UART0 Tx
    ((uint32_t)&LPC_UART0->RBR),    // UART0 Rx
    // ... etc
};
```

**No necesitas especificar estas direcciones manualmente**, el driver las obtiene automáticamente usando `SrcConn` o `DstConn`.

---

## 7. Configuración del GPDMA

### 7.1. Estructura de configuración

La estructura `GPDMA_Channel_CFG_Type` (líneas 317-378 en lpc17xx_gpdma.h) define todos los parámetros de un canal DMA:

```c
typedef struct {
    uint32_t ChannelNum;     // Número de canal (0-7)
    uint32_t TransferSize;   // Tamaño de transferencia (máx 4095)
    uint32_t TransferWidth;  // Ancho: BYTE, HALFWORD, WORD (solo M2M)
    uint32_t SrcMemAddr;     // Dirección fuente (M2M, M2P)
    uint32_t DstMemAddr;     // Dirección destino (M2M, P2M)
    uint32_t TransferType;   // M2M, M2P, P2M, P2P
    uint32_t SrcConn;        // Conexión periférico fuente (P2M, P2P)
    uint32_t DstConn;        // Conexión periférico destino (M2P, P2P)
    uint32_t DMALLI;         // Linked List Item (0 si no se usa)
} GPDMA_Channel_CFG_Type;
```

### 7.2. Anchos de transferencia

```c
#define GPDMA_WIDTH_BYTE       ((0UL))  // 8 bits
#define GPDMA_WIDTH_HALFWORD   ((1UL))  // 16 bits
#define GPDMA_WIDTH_WORD       ((2UL))  // 32 bits
```

**📝 Nota:** `TransferWidth` **solo se usa en transferencias M2M**. Para transferencias con periféricos, el driver usa automáticamente el ancho óptimo de la tabla `GPDMA_LUTPerWid`.

### 7.3. Tamaños de burst

```c
#define GPDMA_BSIZE_1     ((0UL))  // Burst de 1 transferencia
#define GPDMA_BSIZE_4     ((1UL))  // Burst de 4 transferencias
#define GPDMA_BSIZE_8     ((2UL))  // Burst de 8 transferencias
#define GPDMA_BSIZE_16    ((3UL))  // Burst de 16 transferencias
#define GPDMA_BSIZE_32    ((4UL))  // Burst de 32 transferencias
#define GPDMA_BSIZE_64    ((5UL))  // Burst de 64 transferencias
#define GPDMA_BSIZE_128   ((6UL))  // Burst de 128 transferencias
#define GPDMA_BSIZE_256   ((7UL))  // Burst de 256 transferencias
```

**⚠️ IMPORTANTE:** Los burst sizes también son configurados automáticamente por el driver según la tabla `GPDMA_LUTPerBurst`.

### 7.4. Procedimiento de configuración

**Paso 1: Inicializar el controlador GPDMA**

```c
GPDMA_Init();
```

Esta función (líneas 200-218 en lpc17xx_gpdma.c):
- Habilita el clock del GPDMA
- Resetea todos los canales
- Limpia todas las interrupciones

**Paso 2: Configurar la estructura del canal**

```c
GPDMA_Channel_CFG_Type GPDMACfg;

GPDMACfg.ChannelNum = 0;           // Usar canal 0
GPDMACfg.SrcMemAddr = (uint32_t)&src;
GPDMACfg.DstMemAddr = (uint32_t)&dst;
GPDMACfg.TransferSize = SIZE;
GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;
GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
GPDMACfg.SrcConn = 0;
GPDMACfg.DstConn = 0;
GPDMACfg.DMALLI = 0;               // Sin linked list
```

**Paso 3: Configurar el canal con los parámetros**

```c
Status result = GPDMA_Setup(&GPDMACfg);
if (result == ERROR) {
    // El canal ya está habilitado, necesita deshabilitarse primero
}
```

La función `GPDMA_Setup()` (líneas 229-353 en lpc17xx_gpdma.c):
- Verifica que el canal esté disponible
- Configura direcciones, tamaño, burst, ancho según el tipo de transferencia
- **NO habilita el canal** (se hace manualmente con `GPDMA_ChannelCmd`)

**Paso 4: Configurar interrupciones (opcional)**

```c
NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));
NVIC_EnableIRQ(DMA_IRQn);
```

**Paso 5: Habilitar el canal**

```c
GPDMA_ChannelCmd(0, ENABLE);
```

**Paso 6: Esperar finalización**

Opción A - Polling:
```c
while ((Channel0_TC == 0) && (Channel0_Err == 0));
```

Opción B - Por interrupción (ver sección 9).

---

## 8. Listas enlazadas (Linked Lists)

Las **Linked Lists** permiten encadenar múltiples transferencias DMA **sin intervención de la CPU** entre cada transferencia.

### 8.1. Estructura de Linked List Item

```c
typedef struct {
    uint32_t SrcAddr;   // Dirección fuente
    uint32_t DstAddr;   // Dirección destino
    uint32_t NextLLI;   // Dirección del siguiente LLI (0 = fin)
    uint32_t Control;   // Control word (size, width, burst, etc)
} GPDMA_LLI_Type;
```

### 8.2. Campo Control

El campo `Control` se construye manualmente con los siguientes bits:

```c
Control = TransferSize           // Bits 0-11: Tamaño (0-4095)
        | (SBSize << 12)         // Bits 12-14: Source Burst Size
        | (DBSize << 15)         // Bits 15-17: Dest Burst Size
        | (SWidth << 18)         // Bits 18-20: Source Width
        | (DWidth << 21)         // Bits 21-23: Dest Width
        | (SI << 26)             // Bit 26: Source Increment (1=sí)
        | (DI << 27)             // Bit 27: Dest Increment (1=sí)
        | (I << 31);             // Bit 31: Terminal Count Interrupt (1=sí)
```

### 8.3. Ejemplo completo de Linked List

Este ejemplo está verificado de `library/examples/GPDMA/Link_list/link_list.c:206-224`:

```c
GPDMA_LLI_Type DMA_LLI_Struct[2];

// Primera transferencia: Buffer1 → Destino[0..15]
DMA_LLI_Struct[0].SrcAddr = (uint32_t)&DMASrc_Buffer1;
DMA_LLI_Struct[0].DstAddr = (uint32_t)&DMADest_Buffer;
DMA_LLI_Struct[0].NextLLI = (uint32_t)&DMA_LLI_Struct[1];  // Apunta al siguiente
DMA_LLI_Struct[0].Control = (DMA_SIZE/2)     // 16 words
                          | (2<<18)          // Source width = 32 bits
                          | (2<<21)          // Dest width = 32 bits
                          | (1<<26)          // Source increment
                          | (1<<27);         // Dest increment

// Segunda transferencia: Buffer2 → Destino[16..31]
DMA_LLI_Struct[1].SrcAddr = (uint32_t)&DMASrc_Buffer2;
DMA_LLI_Struct[1].DstAddr = ((uint32_t)&DMADest_Buffer) + (DMA_SIZE/2)*4;
DMA_LLI_Struct[1].NextLLI = 0;               // Fin de la lista
DMA_LLI_Struct[1].Control = (DMA_SIZE/2)
                          | (2<<18)
                          | (2<<21)
                          | (1<<26)
                          | (1<<27);

// Configurar canal con linked list
GPDMACfg.DMALLI = (uint32_t)&DMA_LLI_Struct[0];  // Apuntar al primer LLI
```

### 8.4. Ventajas de Linked Lists

✅ **Sin intervención de CPU** entre transferencias
✅ **Transferencias encadenadas automáticas**
✅ **Ideal para buffers circulares** o múltiples bloques
✅ **Reduce latencia** entre transferencias

---

## 9. Interrupciones del GPDMA

El GPDMA genera **2 tipos de interrupciones por canal:**

1. **Terminal Count (TC):** Se genera cuando la transferencia se completa exitosamente
2. **Error:** Se genera cuando ocurre un error durante la transferencia

### 9.1. Vector de interrupción

Todos los canales DMA comparten el mismo vector de interrupción:

```c
void DMA_IRQHandler(void);
```

**IRQ número:** `DMA_IRQn`

### 9.2. Funciones de manejo de interrupciones

**Verificar estado de interrupción:**

```c
IntStatus GPDMA_IntGetStatus(GPDMA_Status_Type type, uint8_t channel);
```

Tipos de estado disponibles (enum `GPDMA_Status_Type` en lpc17xx_gpdma.h:297-304):

```c
GPDMA_STAT_INT        // Estado general de interrupción
GPDMA_STAT_INTTC      // Terminal Count interrupt status
GPDMA_STAT_INTERR     // Error interrupt status
GPDMA_STAT_RAWINTTC   // Raw Terminal Count status (sin máscara)
GPDMA_STAT_RAWINTERR  // Raw Error status (sin máscara)
GPDMA_STAT_ENABLED_CH // Estado de canal habilitado
```

**Limpiar interrupciones:**

```c
void GPDMA_ClearIntPending(GPDMA_StateClear_Type type, uint8_t channel);
```

Tipos de limpieza (enum `GPDMA_StateClear_Type` en lpc17xx_gpdma.h:309-312):

```c
GPDMA_STATCLR_INTTC   // Limpiar Terminal Count interrupt
GPDMA_STATCLR_INTERR  // Limpiar Error interrupt
```

### 9.3. Ejemplo de handler de interrupción

Este ejemplo está verificado de `library/examples/GPDMA/Ram_2_Ram_Test/gpdma_r2r_test.c:86-102`:

```c
volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    // Verificar si el canal 0 tiene interrupción pendiente
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {

        // Verificar si es Terminal Count
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;  // Incrementar contador
        }

        // Verificar si es Error
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;  // Incrementar contador de errores
        }
    }
}
```

### 9.4. Handler para múltiples canales

Este ejemplo está verificado de `library/examples/UART/DMA/uart_dma_test.c:78-122`:

```c
volatile uint32_t Channel0_TC = 0, Channel0_Err = 0;
volatile uint32_t Channel1_TC = 0, Channel1_Err = 0;

void DMA_IRQHandler(void)
{
    uint32_t tmp;

    // Escanear todos los canales
    for (tmp = 0; tmp <= 7; tmp++) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INT, tmp)) {

            // Check Terminal Count
            if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, tmp)) {
                GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, tmp);

                switch (tmp) {
                    case 0:
                        Channel0_TC++;
                        GPDMA_ChannelCmd(0, DISABLE);
                        break;
                    case 1:
                        Channel1_TC++;
                        GPDMA_ChannelCmd(1, DISABLE);
                        break;
                }
            }

            // Check Error
            if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, tmp)) {
                GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, tmp);

                switch (tmp) {
                    case 0:
                        Channel0_Err++;
                        GPDMA_ChannelCmd(0, DISABLE);
                        break;
                    case 1:
                        Channel1_Err++;
                        GPDMA_ChannelCmd(1, DISABLE);
                        break;
                }
            }
        }
    }
}
```

---

## 10. Funciones del driver

### 10.1. Inicialización

```c
void GPDMA_Init(void);
```

**Descripción:** Inicializa el controlador GPDMA
**Parámetros:** Ninguno
**Retorno:** Ninguno

**Acciones realizadas (lpc17xx_gpdma.c:200-218):**
- Habilita el clock del GPDMA
- Resetea configuración de todos los canales (0-7)
- Limpia todas las interrupciones pendientes

**Ejemplo:**
```c
GPDMA_Init();
```

---

### 10.2. Configuración de canal

```c
Status GPDMA_Setup(GPDMA_Channel_CFG_Type *GPDMAChannelConfig);
```

**Descripción:** Configura un canal DMA con los parámetros especificados
**Parámetros:**
- `GPDMAChannelConfig`: Puntero a estructura de configuración

**Retorno:**
- `SUCCESS`: Canal configurado correctamente
- `ERROR`: Canal ya está habilitado (debe deshabilitarse primero)

**Verificación interna (lpc17xx_gpdma.c:234-237):**
```c
if (LPC_GPDMA->DMACEnbldChns & (GPDMA_DMACEnbldChns_Ch(GPDMAChannelConfig->ChannelNum))) {
    return ERROR;  // Canal ocupado
}
```

**Ejemplo:**
```c
GPDMA_Channel_CFG_Type GPDMACfg;
GPDMACfg.ChannelNum = 0;
// ... (configurar otros campos)
if (GPDMA_Setup(&GPDMACfg) == ERROR) {
    // Error: canal ya habilitado
}
```

---

### 10.3. Habilitar/Deshabilitar canal

```c
void GPDMA_ChannelCmd(uint8_t channelNum, FunctionalState NewState);
```

**Descripción:** Habilita o deshabilita un canal DMA
**Parámetros:**
- `channelNum`: Número de canal (0-7)
- `NewState`: `ENABLE` o `DISABLE`

**Retorno:** Ninguno

**Ejemplos:**
```c
GPDMA_ChannelCmd(0, ENABLE);   // Habilitar canal 0
GPDMA_ChannelCmd(0, DISABLE);  // Deshabilitar canal 0
```

**⚠️ IMPORTANTE:** Siempre deshabilita el canal antes de reconfigurar con `GPDMA_Setup()`.

---

### 10.4. Verificación de estado

```c
IntStatus GPDMA_IntGetStatus(GPDMA_Status_Type type, uint8_t channel);
```

**Descripción:** Verifica el estado de interrupción de un canal
**Parámetros:**
- `type`: Tipo de estado a verificar
  - `GPDMA_STAT_INT`: Interrupción general
  - `GPDMA_STAT_INTTC`: Terminal Count
  - `GPDMA_STAT_INTERR`: Error
  - `GPDMA_STAT_RAWINTTC`: TC raw (sin máscara)
  - `GPDMA_STAT_RAWINTERR`: Error raw (sin máscara)
  - `GPDMA_STAT_ENABLED_CH`: Canal habilitado
- `channel`: Número de canal (0-7)

**Retorno:**
- `SET`: La condición está activa
- `RESET`: La condición no está activa

**Ejemplos:**
```c
// Verificar si canal 0 completó transferencia
if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0) == SET) {
    // Transferencia completa
}

// Verificar si canal está habilitado
if (GPDMA_IntGetStatus(GPDMA_STAT_ENABLED_CH, 0) == SET) {
    // Canal está activo
}
```

---

### 10.5. Limpieza de interrupciones

```c
void GPDMA_ClearIntPending(GPDMA_StateClear_Type type, uint8_t channel);
```

**Descripción:** Limpia interrupciones pendientes de un canal
**Parámetros:**
- `type`: Tipo de interrupción a limpiar
  - `GPDMA_STATCLR_INTTC`: Limpiar Terminal Count
  - `GPDMA_STATCLR_INTERR`: Limpiar Error
- `channel`: Número de canal (0-7)

**Retorno:** Ninguno

**Ejemplo:**
```c
// Limpiar interrupción de Terminal Count del canal 0
GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);

// Limpiar interrupción de Error del canal 1
GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 1);
```

---

## 11. Ejemplos prácticos

### Ejemplo 1: Transferencia Memory-to-Memory (M2M)

Este ejemplo copia un buffer de memoria a otro usando DMA.

**Archivo fuente:** `library/examples/GPDMA/Ram_2_Ram_Test/gpdma_r2r_test.c`

```c
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

#define DMA_SIZE    256  // 256 words = 1024 bytes

uint32_t source_buffer[DMA_SIZE];
uint32_t dest_buffer[DMA_SIZE];

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
        }
    }
}

int main(void)
{
    GPDMA_Channel_CFG_Type GPDMACfg;

    // Inicializar buffers
    for (uint32_t i = 0; i < DMA_SIZE; i++) {
        source_buffer[i] = i;
        dest_buffer[i] = 0;
    }

    // Configurar interrupción DMA
    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar canal 0
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)source_buffer;
    GPDMACfg.DstMemAddr = (uint32_t)dest_buffer;
    GPDMACfg.TransferSize = DMA_SIZE;
    GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;  // 32 bits
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = 0;
    GPDMACfg.DMALLI = 0;

    GPDMA_Setup(&GPDMACfg);

    // Resetear flags
    Channel0_TC = 0;
    Channel0_Err = 0;

    // Habilitar interrupción y canal
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);

    // Esperar finalización
    while ((Channel0_TC == 0) && (Channel0_Err == 0));

    // Verificar datos
    for (uint32_t i = 0; i < DMA_SIZE; i++) {
        if (source_buffer[i] != dest_buffer[i]) {
            // Error en la transferencia
            while(1);
        }
    }

    // ¡Éxito!
    while(1);
}
```

**Explicación:**
1. Se inicializan dos buffers: `source_buffer` con datos, `dest_buffer` en cero
2. Se configura el DMA para transferencia M2M de 256 words (1024 bytes)
3. El DMA copia automáticamente todos los datos
4. La interrupción Terminal Count indica finalización
5. Se verifica que todos los datos se copiaron correctamente

---

### Ejemplo 2: UART Transmit con DMA (M2P)

Este ejemplo transmite un string por UART0 usando DMA.

**Archivo fuente:** `library/examples/UART/DMA/uart_dma_test.c:198-255`

```c
#include "lpc17xx_uart.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

uint8_t tx_buffer[] = "Hola! Este mensaje se transmite por DMA\n\r";

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
        }
    }
}

int main(void)
{
    PINSEL_CFG_Type PinCfg;
    UART_CFG_Type UARTConfigStruct;
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;
    GPDMA_Channel_CFG_Type GPDMACfg;

    // Configurar pines UART0: P0.2 (TXD), P0.3 (RXD)
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 2;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 3;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar UART0 a 115200 bps
    UART_ConfigStructInit(&UARTConfigStruct);
    UARTConfigStruct.Baud_rate = 115200;
    UART_Init(LPC_UART0, &UARTConfigStruct);

    // Configurar FIFO con DMA habilitado
    UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
    UARTFIFOConfigStruct.FIFO_DMAMode = ENABLE;  // ¡CRÍTICO!
    UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);

    UART_TxCmd(LPC_UART0, ENABLE);

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar interrupción
    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

    // Configurar canal 0: Memoria → UART0 Tx
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)&tx_buffer;
    GPDMACfg.DstMemAddr = 0;  // No usado (dirección obtenida automáticamente)
    GPDMACfg.TransferSize = sizeof(tx_buffer);
    GPDMACfg.TransferWidth = 0;  // No usado en M2P
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    GPDMACfg.SrcConn = 0;  // No usado en M2P
    GPDMACfg.DstConn = GPDMA_CONN_UART0_Tx;  // UART0 Transmit
    GPDMACfg.DMALLI = 0;

    GPDMA_Setup(&GPDMACfg);

    // Resetear flags
    Channel0_TC = 0;
    Channel0_Err = 0;

    // Habilitar interrupción y canal
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);

    // Esperar finalización
    while ((Channel0_TC == 0) && (Channel0_Err == 0));

    // Transmisión completa
    while(1);
}
```

**⚠️ IMPORTANTE:** Para usar DMA con UART, **DEBES habilitar el modo DMA en el FIFO:**

```c
UARTFIFOConfigStruct.FIFO_DMAMode = ENABLE;
```

---

### Ejemplo 3: ADC con DMA (P2M)

Este ejemplo adquiere muestras del ADC continuamente usando DMA.

**Archivo fuente:** `library/examples/ADC/DMA/adc_dma_test.c:125-248`

```c
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

#define NUM_SAMPLES  100

uint32_t adc_samples[NUM_SAMPLES];

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
        }
    }
}

int main(void)
{
    PINSEL_CFG_Type PinCfg;
    GPDMA_Channel_CFG_Type GPDMACfg;

    // Configurar P0.25 como AD0.2
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar ADC a 200kHz
    ADC_Init(LPC_ADC, 200000);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, SET);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar interrupción
    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

    // Configurar canal 0: ADC → Memoria
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = 0;  // No usado (dirección obtenida automáticamente)
    GPDMACfg.DstMemAddr = (uint32_t)&adc_samples;
    GPDMACfg.TransferSize = NUM_SAMPLES;
    GPDMACfg.TransferWidth = 0;  // No usado en P2M
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
    GPDMACfg.SrcConn = GPDMA_CONN_ADC;  // ADC como fuente
    GPDMACfg.DstConn = 0;  // No usado en P2M
    GPDMACfg.DMALLI = 0;

    GPDMA_Setup(&GPDMACfg);

    // Resetear flags
    Channel0_TC = 0;
    Channel0_Err = 0;

    // Habilitar interrupción y canal
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);

    // Iniciar conversiones del ADC
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);

    // Esperar finalización
    while ((Channel0_TC == 0) && (Channel0_Err == 0));

    // Deshabilitar canal
    GPDMA_ChannelCmd(0, DISABLE);

    // Procesar muestras
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        uint16_t value = ADC_DR_RESULT(adc_samples[i]);  // Extraer bits 4-15
        float voltage = (value / 4095.0) * 3.3;
        // ... usar voltage
    }

    while(1);
}
```

**Explicación:**
- El ADC genera conversiones y las escribe en su registro ADGDR
- El DMA lee automáticamente ADGDR y lo copia a `adc_samples[]`
- Después de 100 muestras, el DMA genera interrupción Terminal Count
- Los datos se procesan usando la macro `ADC_DR_RESULT()` para extraer el valor

---

### Ejemplo 4: DAC con DMA para generar onda senoidal

Este ejemplo genera una onda senoidal de 60Hz usando DAC + DMA.

**Archivo fuente:** `library/examples/DAC/DMA/dac_dma.c` (ver tutorial 07_ADC_DAC.md para ejemplo completo)

```c
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"

#define NUM_SINE_SAMPLES  60
#define SIGNAL_FREQ_Hz    60

// Tabla lookup de onda senoidal (60 muestras, 10 bits)
uint32_t dac_sine_lut[NUM_SINE_SAMPLES];

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
        }
    }
}

int main(void)
{
    PINSEL_CFG_Type PinCfg;
    GPDMA_Channel_CFG_Type GPDMACfg;
    DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
    uint32_t i, tmp;

    // Configurar P0.26 como AOUT
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Generar tabla lookup de onda senoidal
    for (i = 0; i < NUM_SINE_SAMPLES; i++) {
        double sin_value = sin(2.0 * 3.14159 * i / NUM_SINE_SAMPLES);
        uint32_t dac_value = (uint32_t)((sin_value + 1.0) * 511.5);  // 0-1023
        dac_sine_lut[i] = DAC_VALUE(dac_value);
    }

    // Inicializar DAC
    DAC_Init(LPC_DAC);

    // Configurar DAC para DMA
    DAC_ConverterConfigStruct.CNT_ENA = SET;      // Habilitar contador de timeout
    DAC_ConverterConfigStruct.DMA_ENA = SET;      // Habilitar DMA
    DAC_ConverterConfigStruct.DBLBUF_ENA = RESET;
    DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);

    // Calcular timeout para 60Hz con 60 muestras = 3600 muestras/seg
    uint32_t PCLK = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_DAC);
    uint32_t dac_timeout = PCLK / (SIGNAL_FREQ_Hz * NUM_SINE_SAMPLES);
    DAC_SetDMATimeOut(LPC_DAC, dac_timeout);

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar interrupción
    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

    // Configurar canal 0: Memoria → DAC (cíclico con linked list)
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)dac_sine_lut;
    GPDMACfg.DstMemAddr = 0;  // DAC (automático)
    GPDMACfg.TransferSize = NUM_SINE_SAMPLES;
    GPDMACfg.TransferWidth = 0;
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = GPDMA_CONN_DAC;
    GPDMACfg.DMALLI = 0;  // Sin linked list (para ejemplo simple)

    GPDMA_Setup(&GPDMACfg);

    // Habilitar DMA
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelCmd(0, ENABLE);

    // El DAC genera una señal senoidal de 60Hz continuamente
    while(1) {
        // Monitorear si hubo error
        if (Channel0_Err > 0) {
            // Error en DMA
            break;
        }
    }
}
```

**Explicación:**
- Se crea una tabla lookup con 60 muestras de una senoidal
- El DAC está configurado en modo DMA con timeout
- Cada timeout, el DAC solicita un dato al DMA
- El DMA transfiere automáticamente los valores de la tabla al DAC
- El resultado es una onda senoidal de 60Hz en el pin AOUT (P0.26)

---

### Ejemplo 5: Linked List para transferencias múltiples

Este ejemplo usa linked list para transferir dos buffers secuencialmente sin intervención de CPU.

**Archivo fuente:** `library/examples/GPDMA/Link_list/link_list.c:206-246`

```c
#include "lpc17xx_gpdma.h"

#define DMA_SIZE  32

uint32_t buffer1[DMA_SIZE/2] = {0x01, 0x02, 0x03, /* ... */, 0x10};
uint32_t buffer2[DMA_SIZE/2] = {0x11, 0x12, 0x13, /* ... */, 0x20};
uint32_t destination[DMA_SIZE];

volatile uint32_t Channel0_TC = 0;
volatile uint32_t Channel0_Err = 0;

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
            Channel0_TC++;
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
            Channel0_Err++;
        }
    }
}

int main(void)
{
    GPDMA_Channel_CFG_Type GPDMACfg;
    GPDMA_LLI_Type DMA_LLI_Struct[2];

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar primer Linked List Item: buffer1 → destination[0..15]
    DMA_LLI_Struct[0].SrcAddr = (uint32_t)&buffer1;
    DMA_LLI_Struct[0].DstAddr = (uint32_t)&destination;
    DMA_LLI_Struct[0].NextLLI = (uint32_t)&DMA_LLI_Struct[1];  // Apuntar al siguiente
    DMA_LLI_Struct[0].Control = (DMA_SIZE/2)     // 16 words
                              | (2<<18)          // Source width = 32 bits (WORD)
                              | (2<<21)          // Dest width = 32 bits (WORD)
                              | (1<<26)          // Source increment
                              | (1<<27);         // Dest increment

    // Configurar segundo Linked List Item: buffer2 → destination[16..31]
    DMA_LLI_Struct[1].SrcAddr = (uint32_t)&buffer2;
    DMA_LLI_Struct[1].DstAddr = ((uint32_t)&destination) + (DMA_SIZE/2)*4;  // Offset
    DMA_LLI_Struct[1].NextLLI = 0;               // Fin de lista
    DMA_LLI_Struct[1].Control = (DMA_SIZE/2)
                              | (2<<18)
                              | (2<<21)
                              | (1<<26)
                              | (1<<27);

    // Configurar canal 0 con linked list
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)buffer1;     // Primera fuente
    GPDMACfg.DstMemAddr = (uint32_t)destination;  // Primer destino
    GPDMACfg.TransferSize = DMA_SIZE;
    GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = 0;
    GPDMACfg.DMALLI = (uint32_t)&DMA_LLI_Struct[0];  // Apuntar al primer LLI

    GPDMA_Setup(&GPDMACfg);

    // Configurar interrupción
    NVIC_DisableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(DMA_IRQn);

    // Resetear flags
    Channel0_TC = 0;
    Channel0_Err = 0;

    // Habilitar canal
    GPDMA_ChannelCmd(0, ENABLE);

    // Esperar que AMBAS transferencias terminen
    while ((Channel0_TC == 0) && (Channel0_Err == 0));

    // Verificar resultados
    // destination[0..15] debe contener buffer1
    // destination[16..31] debe contener buffer2

    while(1);
}
```

**Explicación del flujo:**
1. Se configuran 2 Linked List Items en un array
2. LLI[0] apunta a LLI[1] mediante `NextLLI`
3. LLI[1] tiene `NextLLI = 0` (fin de lista)
4. El canal se configura apuntando a LLI[0] mediante `DMALLI`
5. Al habilitar el canal:
   - DMA transfiere buffer1 → destination[0..15]
   - **Automáticamente** carga LLI[1] y continúa
   - DMA transfiere buffer2 → destination[16..31]
   - Genera interrupción TC al finalizar TODO

**Ventaja:** Las dos transferencias ocurren **sin intervención de CPU**.

---

## 12. Errores comunes

### Error 1: Olvidar habilitar el modo DMA en periféricos

**Síntoma:** El DMA no se activa o no transfiere datos.

**Causa:** Algunos periféricos requieren habilitar explícitamente el modo DMA en su configuración.

**Solución:**

Para **UART:**
```c
UART_FIFO_CFG_Type UARTFIFOConfigStruct;
UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
UARTFIFOConfigStruct.FIFO_DMAMode = ENABLE;  // ¡Crítico!
UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);
```

Para **DAC:**
```c
DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
DAC_ConverterConfigStruct.DMA_ENA = SET;  // Habilitar DMA
DAC_ConverterConfigStruct.CNT_ENA = SET;  // Habilitar timeout counter
DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
```

---

### Error 2: Canal ya habilitado al llamar GPDMA_Setup()

**Síntoma:** `GPDMA_Setup()` retorna `ERROR`.

**Causa:** El canal DMA ya está habilitado de una operación anterior.

**Solución:** Deshabilitar el canal antes de reconfigurar:

```c
// Deshabilitar canal primero
GPDMA_ChannelCmd(0, DISABLE);

// Esperar a que se deshabilite completamente
while (GPDMA_IntGetStatus(GPDMA_STAT_ENABLED_CH, 0) == SET);

// Ahora reconfigurar
if (GPDMA_Setup(&GPDMACfg) == ERROR) {
    // Todavía hay error
}
```

---

### Error 3: No limpiar interrupciones en el handler

**Síntoma:** El handler de interrupción se ejecuta continuamente.

**Causa:** No se limpiaron las banderas de interrupción.

**Solución:** Siempre limpiar las interrupciones:

```c
void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)) {
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);  // ¡Limpiar!
            // ... procesar
        }
        if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
            GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);  // ¡Limpiar!
            // ... procesar error
        }
    }
}
```

---

### Error 4: TransferSize excede 4095

**Síntoma:** Solo se transfieren 4095 elementos o menos de lo esperado.

**Causa:** El campo `TransferSize` es de 12 bits, máximo 4095 (0xFFF).

**Solución:** Para transferencias grandes, usar linked lists:

```c
#define BUFFER_SIZE  10000

// Dividir en bloques de 4095
GPDMA_LLI_Type LLI[3];  // Necesitamos 3 bloques (4095 + 4095 + 1810)

LLI[0].Control = 4095 | ...;
LLI[0].NextLLI = (uint32_t)&LLI[1];

LLI[1].Control = 4095 | ...;
LLI[1].NextLLI = (uint32_t)&LLI[2];

LLI[2].Control = 1810 | ...;  // Resto
LLI[2].NextLLI = 0;
```

---

### Error 5: Alineación incorrecta de buffers

**Síntoma:** Error de DMA o transferencias incorrectas.

**Causa:** Los buffers no están alineados correctamente para transferencias WORD o HALFWORD.

**Solución:** Asegurar alineación correcta:

```c
// Para transferencias WORD (32 bits), alinear a 4 bytes
uint32_t buffer[100] __attribute__((aligned(4)));

// O usar tipos nativos que ya están alineados
uint32_t buffer[100];  // Automáticamente alineado a 4 bytes
```

---

### Error 6: Olvidar incrementar direcciones

**Síntoma:** El DMA transfiere siempre el mismo dato.

**Causa:** En transferencias M2M, los bits de incremento de dirección no están configurados.

**Explicación:** En transferencias M2M configuradas con `GPDMA_Setup()`, el driver automáticamente configura los bits SI (Source Increment) y DI (Destination Increment) (líneas 267-268 en lpc17xx_gpdma.c). Sin embargo, si estás configurando **Linked Lists manualmente**, debes incluir estos bits:

```c
// En Control word de Linked List:
DMA_LLI.Control = (size)
                | (2<<18)    // Source width
                | (2<<21)    // Dest width
                | (1<<26)    // SI: Source Increment ¡IMPORTANTE!
                | (1<<27);   // DI: Dest Increment ¡IMPORTANTE!
```

---

### Error 7: Periférico no configurado correctamente

**Síntoma:** El DMA no inicia la transferencia o se detiene inmediatamente.

**Causa:** El periférico fuente/destino no está configurado o inicializado.

**Solución:** Configurar el periférico **antes** de habilitar el DMA:

Para **ADC:**
```c
// Primero configurar ADC
ADC_Init(LPC_ADC, 200000);
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, SET);  // Habilitar interrupción del canal

// LUEGO configurar DMA
GPDMA_Setup(&GPDMACfg);
GPDMA_ChannelCmd(0, ENABLE);

// FINALMENTE iniciar conversión
ADC_StartCmd(LPC_ADC, ADC_START_NOW);
```

Para **UART:**
```c
// Primero configurar UART y habilitar FIFO DMA
UART_Init(LPC_UART0, &UARTConfigStruct);
UARTFIFOConfigStruct.FIFO_DMAMode = ENABLE;
UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);
UART_TxCmd(LPC_UART0, ENABLE);

// LUEGO configurar y habilitar DMA
GPDMA_Setup(&GPDMACfg);
GPDMA_ChannelCmd(0, ENABLE);
```

---

### Error 8: Valores incorrectos en NextLLI de Linked Lists

**Síntoma:** Transferencia se detiene después del primer bloque o comportamiento errático.

**Causa:** El campo `NextLLI` debe estar **alineado a 4 bytes** y apuntar a una estructura válida.

**Solución:**

```c
// ❌ INCORRECTO: Dirección no alineada o inválida
DMA_LLI_Struct[0].NextLLI = 0x12345678;  // Dirección arbitraria

// ✅ CORRECTO: Apuntar a otra estructura LLI válida
DMA_LLI_Struct[0].NextLLI = (uint32_t)&DMA_LLI_Struct[1];

// ✅ CORRECTO: Último item debe tener NextLLI = 0
DMA_LLI_Struct[1].NextLLI = 0;
```

**⚠️ IMPORTANTE:** Las direcciones de los LLI deben ser accesibles durante toda la transferencia (no pueden estar en stack de una función que retorna).

---

### Error 9: No verificar Terminal Count antes de acceder a datos

**Síntoma:** Los datos procesados están incompletos o son incorrectos.

**Causa:** Se accede al buffer de destino antes de que el DMA termine la transferencia.

**Solución:** Siempre esperar Terminal Count:

```c
// Opción 1: Polling
while (Channel0_TC == 0);

// Opción 2: Por interrupción
volatile uint8_t dma_complete = 0;

void DMA_IRQHandler(void) {
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
        dma_complete = 1;  // Marcar como completo
    }
}

// En main:
while (!dma_complete);  // Esperar
// Ahora es seguro acceder a los datos
```

---

### Error 10: Usar mismo canal para múltiples periféricos simultáneamente

**Síntoma:** Solo una transferencia funciona o comportamiento errático.

**Causa:** Un canal DMA solo puede manejar una transferencia a la vez.

**Solución:** Usar canales diferentes para transferencias simultáneas:

```c
// ❌ INCORRECTO: Mismo canal para UART Tx y UART Rx
GPDMACfg.ChannelNum = 0;
GPDMACfg.DstConn = GPDMA_CONN_UART0_Tx;
GPDMA_Setup(&GPDMACfg);
GPDMA_ChannelCmd(0, ENABLE);

GPDMACfg.ChannelNum = 0;  // ¡Mismo canal!
GPDMACfg.SrcConn = GPDMA_CONN_UART0_Rx;
GPDMA_Setup(&GPDMACfg);  // ERROR: canal ya habilitado
GPDMA_ChannelCmd(0, ENABLE);

// ✅ CORRECTO: Canales diferentes
// Canal 0 para UART Tx
GPDMACfg.ChannelNum = 0;
GPDMACfg.DstConn = GPDMA_CONN_UART0_Tx;
GPDMA_Setup(&GPDMACfg);
GPDMA_ChannelCmd(0, ENABLE);

// Canal 1 para UART Rx
GPDMACfg.ChannelNum = 1;
GPDMACfg.SrcConn = GPDMA_CONN_UART0_Rx;
GPDMA_Setup(&GPDMACfg);
GPDMA_ChannelCmd(1, ENABLE);
```

---

## 13. Referencias

### Documentos oficiales
- **UM10360:** LPC176x/5x User Manual (Capítulo 31: GPDMA)
- **CMSIS:** Cortex Microcontroller Software Interface Standard

### Archivos del driver
- `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_gpdma.h` - Definiciones y prototipos
- `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_gpdma.c` - Implementación del driver

### Ejemplos verificados
- `library/examples/GPDMA/Ram_2_Ram_Test/gpdma_r2r_test.c` - Memory to Memory
- `library/examples/GPDMA/Link_list/link_list.c` - Linked Lists
- `library/examples/UART/DMA/uart_dma_test.c` - UART con DMA
- `library/examples/ADC/DMA/adc_dma_test.c` - ADC con DMA
- `library/examples/DAC/DMA/dac_dma.c` - DAC con DMA

### Tutoriales relacionados
- `04_TIMER.md` - Para uso de TIMER Match con DMA
- `06_UART.md` - Para configuración de UART
- `07_ADC_DAC.md` - Para configuración de ADC y DAC

---

**Fin del tutorial de GPDMA para LPC1769**
