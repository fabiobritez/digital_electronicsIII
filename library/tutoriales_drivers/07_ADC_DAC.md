# Tutorial: Drivers ADC y DAC para LPC1769

## Introducción

El LPC1769 incluye **conversor Analógico-Digital (ADC)** y **conversor Digital-Analógico (DAC)** para interfaces con señales analógicas. Estos periféricos permiten leer sensores analógicos (temperatura, luz, presión) y generar señales analógicas (audio, control de actuadores).

### ADC (Analog-to-Digital Converter)

**Características principales:**
- **1 ADC** de 12 bits de resolución (valores 0-4095)
- **8 canales** de entrada (AD0.0 a AD0.7)
- **Velocidad de conversión**: hasta 200 kHz
- **Modos de operación**: Software trigger, Burst mode, Hardware trigger
- **Triggers por hardware**: EINT0, CAP0.1, MAT0.1, MAT0.3, MAT1.0, MAT1.1
- **Interrupciones** por canal o global
- **Rango de voltaje**: 0V a VREF (típicamente 3.3V)

### DAC (Digital-to-Analog Converter)

**Características principales:**
- **1 DAC** de 10 bits de resolución (valores 0-1023)
- **1 canal** de salida (AOUT)
- **Settling time**: 1μs (700μA) o 2.5μs (350μA)
- **Soporte DMA** para generación de formas de onda
- **Rango de salida**: 0V a VREF (típicamente 3.3V)

**Archivos del driver:**
- ADC Header: `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_adc.h`
- ADC Source: `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_adc.c`
- DAC Header: `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_dac.h`
- DAC Source: `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_dac.c`
- Ejemplos: `library/examples/ADC/` y `library/examples/DAC/`

---

## Pines de ADC y DAC en LPC1769

### Pines ADC

**Verificado en:** `library/examples/ADC/Polling/adc_polling_test.c` y `adc_burst_test.c`

| Canal | Pin    | Función PINSEL | Verificado en código |
|-------|--------|----------------|----------------------|
| AD0.0 | P0.23  | 1              | -                    |
| AD0.1 | P0.24  | 1              | -                    |
| AD0.2 | P0.25  | 1              | ✓ (línea 117)        |
| AD0.3 | P0.26  | 1              | ✓ (línea 177)        |
| AD0.4 | P1.30  | 3              | -                    |
| AD0.5 | P1.31  | 3              | ✓                    |
| AD0.6 | P0.3   | 2              | -                    |
| AD0.7 | P0.2   | 2              | -                    |

**Ejemplo de configuración ADC:**
```c
// AD0.2 en P0.25
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 1;
PinCfg.OpenDrain = 0;
PinCfg.Pinmode = 0;      // PINMODE_TRISTATE para entradas analógicas
PinCfg.Pinnum = 25;
PinCfg.Portnum = 0;
PINSEL_ConfigPin(&PinCfg);
```

### Pin DAC

**Verificado en:** `library/examples/DAC/SineWave/dac_sinewave_test.c` línea 73

| Señal | Pin   | Función PINSEL |
|-------|-------|----------------|
| AOUT  | P0.26 | 2              |

**Nota importante:** **P0.26 es compartido entre AD0.3 y AOUT**. No pueden usarse simultáneamente.

**Ejemplo de configuración DAC:**
```c
// AOUT en P0.26
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 2;      // Función 2 para DAC
PinCfg.OpenDrain = 0;
PinCfg.Pinmode = 0;
PinCfg.Pinnum = 26;
PinCfg.Portnum = 0;
PINSEL_ConfigPin(&PinCfg);
```

---

# PARTE 1: ADC (Analog-to-Digital Converter)

## Conceptos Fundamentales del ADC

### 1. Resolución y Conversión

El ADC del LPC1769 tiene **12 bits de resolución**:
- **Valores digitales**: 0 a 4095 (2^12 - 1)
- **Voltaje de referencia**: VREF (típicamente 3.3V)

**Fórmula de conversión:**
```
Voltaje_entrada = (ADC_value / 4095) * VREF

Ejemplo con VREF = 3.3V:
  ADC_value = 2048 → Voltaje = (2048/4095) * 3.3V ≈ 1.65V
  ADC_value = 4095 → Voltaje = 3.3V
  ADC_value = 0    → Voltaje = 0V
```

### 2. Velocidad de Conversión

**Verificado en:** lpc17xx_adc.c líneas 71-78

El ADC requiere **65 ciclos de reloj ADC** para una conversión completa:

```c
// Fórmula del driver:
ADC_clock = PCLK_ADC / (CLKDIV + 1)
ADC_rate = ADC_clock / 65

// Ejemplo: rate deseado = 200kHz, PCLK = 25MHz
CLKDIV = (PCLK / (rate * 65)) - 1
CLKDIV = (25000000 / (200000 * 65)) - 1 ≈ 0.923 → 1
ADC_clock = 25MHz / 2 = 12.5MHz
ADC_rate real = 12.5MHz / 65 ≈ 192kHz
```

**Límite:** ADC_clock debe ser ≤ 13 MHz según especificación.

### 3. Modos de Operación

#### a) Software Trigger (ADC_START_NOW)
Conversión única iniciada por software:
```c
ADC_StartCmd(LPC_ADC, ADC_START_NOW);
while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));
uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
```

#### b) Burst Mode (ADC_START_CONTINUOUS)
Conversiones continuas automáticas:
```c
ADC_BurstCmd(LPC_ADC, ENABLE);
// El ADC convierte continuamente todos los canales habilitados
```

#### c) Hardware Trigger
Conversión disparada por eventos externos:
- `ADC_START_ON_EINT0`: Flanco en P2.10/EINT0
- `ADC_START_ON_CAP01`: Flanco en P1.27/CAP0.1
- `ADC_START_ON_MAT01`: Match en MAT0.1
- `ADC_START_ON_MAT03`: Match en MAT0.3
- `ADC_START_ON_MAT10`: Match en MAT1.0
- `ADC_START_ON_MAT11`: Match en MAT1.1

---

## Estructuras de Datos del ADC

### ADC_CHANNEL_SELECTION

Selección de canal:
```c
typedef enum {
    ADC_CHANNEL_0 = 0,  // AD0.0 (P0.23)
    ADC_CHANNEL_1,      // AD0.1 (P0.24)
    ADC_CHANNEL_2,      // AD0.2 (P0.25)
    ADC_CHANNEL_3,      // AD0.3 (P0.26)
    ADC_CHANNEL_4,      // AD0.4 (P1.30)
    ADC_CHANNEL_5,      // AD0.5 (P1.31)
    ADC_CHANNEL_6,      // AD0.6 (P0.3)
    ADC_CHANNEL_7       // AD0.7 (P0.2)
} ADC_CHANNEL_SELECTION;
```

### ADC_START_OPT

Opciones de inicio de conversión:
```c
typedef enum {
    ADC_START_CONTINUOUS = 0,  // Modo continuo (burst)
    ADC_START_NOW,             // Iniciar ahora (software)
    ADC_START_ON_EINT0,        // Trigger en EINT0
    ADC_START_ON_CAP01,        // Trigger en CAP0.1
    ADC_START_ON_MAT01,        // Trigger en MAT0.1
    ADC_START_ON_MAT03,        // Trigger en MAT0.3
    ADC_START_ON_MAT10,        // Trigger en MAT1.0
    ADC_START_ON_MAT11         // Trigger en MAT1.1
} ADC_START_OPT;
```

### ADC_TYPE_INT_OPT

Tipos de interrupción:
```c
typedef enum {
    ADC_ADINTEN0 = 0,  // Interrupción canal 0
    ADC_ADINTEN1,      // Interrupción canal 1
    ADC_ADINTEN2,      // Interrupción canal 2
    ADC_ADINTEN3,      // Interrupción canal 3
    ADC_ADINTEN4,      // Interrupción canal 4
    ADC_ADINTEN5,      // Interrupción canal 5
    ADC_ADINTEN6,      // Interrupción canal 6
    ADC_ADINTEN7,      // Interrupción canal 7
    ADC_ADGINTEN       // Interrupción global (cualquier canal)
} ADC_TYPE_INT_OPT;
```

---

## Funciones del Driver ADC

### Funciones de Inicialización

#### `ADC_Init()`
```c
void ADC_Init(LPC_ADC_TypeDef *ADCx, uint32_t rate);
```

Inicializa el ADC con la velocidad de conversión especificada.

**Parámetros:**
- `ADCx`: Siempre `LPC_ADC` (solo hay un ADC)
- `rate`: Velocidad de conversión en Hz (máximo 200000 = 200kHz)

**Comportamiento interno** (verificado en lpc17xx_adc.c:56-82):
1. Enciende el reloj del ADC (PCONP)
2. Calcula CLKDIV basándose en PCLK y rate deseado
3. Habilita el ADC (bit PDN)

**Ejemplo:**
```c
ADC_Init(LPC_ADC, 200000);  // 200 kHz (máxima velocidad)
```

#### `ADC_DeInit()`
```c
void ADC_DeInit(LPC_ADC_TypeDef *ADCx);
```

Deinicializa el ADC y apaga su reloj.

**Ejemplo:**
```c
ADC_DeInit(LPC_ADC);
```

### Funciones de Control de Canales

#### `ADC_ChannelCmd()`
```c
void ADC_ChannelCmd(LPC_ADC_TypeDef *ADCx, uint8_t Channel, FunctionalState NewState);
```

Habilita o deshabilita un canal específico.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `Channel`: `ADC_CHANNEL_0` a `ADC_CHANNEL_7`
- `NewState`: `ENABLE` o `DISABLE`

**Ejemplo:**
```c
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);   // Habilitar AD0.2
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_3, ENABLE);   // Habilitar AD0.3
```

### Funciones de Inicio de Conversión

#### `ADC_StartCmd()`
```c
void ADC_StartCmd(LPC_ADC_TypeDef *ADCx, uint8_t start_mode);
```

Inicia la conversión con el modo especificado.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `start_mode`: Ver `ADC_START_OPT`

**Ejemplo:**
```c
ADC_StartCmd(LPC_ADC, ADC_START_NOW);  // Conversión única ahora
```

#### `ADC_BurstCmd()`
```c
void ADC_BurstCmd(LPC_ADC_TypeDef *ADCx, FunctionalState NewState);
```

Habilita o deshabilita el modo burst (conversión continua).

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `NewState`: `ENABLE` o `DISABLE`

**Ejemplo:**
```c
ADC_BurstCmd(LPC_ADC, ENABLE);  // Conversiones continuas
```

#### `ADC_EdgeStartConfig()`
```c
void ADC_EdgeStartConfig(LPC_ADC_TypeDef *ADCx, uint8_t EdgeOption);
```

Configura el tipo de flanco para triggers por hardware.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `EdgeOption`:
  - `ADC_START_ON_RISING` (0): Flanco ascendente
  - `ADC_START_ON_FALLING` (1): Flanco descendente

**Ejemplo:**
```c
ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);  // Disparar en flanco ascendente
ADC_StartCmd(LPC_ADC, ADC_START_ON_EINT0);           // Trigger en EINT0
```

### Funciones de Lectura de Datos

#### `ADC_ChannelGetData()`
```c
uint16_t ADC_ChannelGetData(LPC_ADC_TypeDef *ADCx, uint8_t channel);
```

Lee el valor convertido de un canal específico.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `channel`: `ADC_CHANNEL_0` a `ADC_CHANNEL_7`

**Retorna:** Valor de 12 bits (0-4095)

**Ejemplo:**
```c
uint16_t adc_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
float voltage = (adc_value / 4095.0) * 3.3;  // Convertir a voltaje
```

#### `ADC_GlobalGetData()`
```c
uint32_t ADC_GlobalGetData(LPC_ADC_TypeDef *ADCx);
```

Lee el registro global de datos (GDR). Útil en modo burst.

**Retorna:** Registro completo (incluye canal y flags)

**Ejemplo:**
```c
uint32_t gdr = ADC_GlobalGetData(LPC_ADC);
uint16_t value = ADC_GDR_RESULT(gdr);    // Extraer valor
uint8_t channel = ADC_GDR_CH(gdr);       // Extraer canal
```

### Funciones de Estado

#### `ADC_ChannelGetStatus()`
```c
FlagStatus ADC_ChannelGetStatus(LPC_ADC_TypeDef *ADCx, uint8_t channel, uint32_t StatusType);
```

Verifica el estado de un canal.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `channel`: `ADC_CHANNEL_0` a `ADC_CHANNEL_7`
- `StatusType`:
  - `ADC_DATA_DONE`: Conversión completada
  - `ADC_DATA_BURST`: Modo burst activo

**Retorna:** `SET` o `RESET`

**Ejemplo:**
```c
while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));
// Esperar hasta que conversión esté lista
```

### Funciones de Interrupciones

#### `ADC_IntConfig()`
```c
void ADC_IntConfig(LPC_ADC_TypeDef *ADCx, ADC_TYPE_INT_OPT IntType, FunctionalState NewState);
```

Habilita o deshabilita interrupciones.

**Parámetros:**
- `ADCx`: `LPC_ADC`
- `IntType`: Ver `ADC_TYPE_INT_OPT`
- `NewState`: `ENABLE` o `DISABLE`

**Ejemplo:**
```c
ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, ENABLE);  // Interrupción en canal 2
NVIC_EnableIRQ(ADC_IRQn);                       // Habilitar en NVIC
```

---

## Ejemplos Prácticos del ADC

### Ejemplo 1: Lectura Simple en Modo Polling

**Verificado en:** `library/examples/ADC/Polling/adc_polling_test.c`

```c
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    PINSEL_CFG_Type PinCfg;
    uint32_t adc_value;

    // 1. Configurar pin AD0.2 en P0.25
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar ADC a 200 kHz
    ADC_Init(LPC_ADC, 200000);

    // 3. Habilitar canal 2
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);

    while(1) {
        // 4. Iniciar conversión
        ADC_StartCmd(LPC_ADC, ADC_START_NOW);

        // 5. Esperar conversión completa
        while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));

        // 6. Leer valor
        adc_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);

        // 7. Convertir a voltaje
        float voltage = (adc_value / 4095.0) * 3.3;

        // Procesar datos...
        // Delay antes de próxima lectura
        for(volatile int i=0; i<1000000; i++);
    }
}
```

### Ejemplo 2: Modo Burst (Conversión Continua Multi-canal)

**Verificado en:** `library/examples/ADC/Burst/adc_burst_test.c`

```c
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    PINSEL_CFG_Type PinCfg;
    uint32_t adc_value_ch2, adc_value_ch3;

    // 1. Configurar pines AD0.2 (P0.25) y AD0.3 (P0.26)
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;

    PinCfg.Pinnum = 25;  // AD0.2
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 26;  // AD0.3
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar ADC
    ADC_Init(LPC_ADC, 200000);

    // 3. Habilitar canales 2 y 3
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_3, ENABLE);

    // 4. Habilitar modo burst
    ADC_BurstCmd(LPC_ADC, ENABLE);

    while(1) {
        // En modo burst, las conversiones son automáticas
        // Solo esperar y leer datos cuando DONE esté set

        // Leer canal 2
        if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)) {
            adc_value_ch2 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
        }

        // Leer canal 3
        if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_3, ADC_DATA_DONE)) {
            adc_value_ch3 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_3);
        }

        // Procesar datos...
        for(volatile int i=0; i<1000000; i++);
    }
}
```

### Ejemplo 3: ADC con Interrupciones

```c
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_nvic.h"

volatile uint16_t adc_value = 0;
volatile uint8_t adc_done = 0;

void ADC_IRQHandler(void) {
    // Leer valor si canal 2 está listo
    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)) {
        adc_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
        adc_done = 1;
    }
}

int main(void) {
    PINSEL_CFG_Type PinCfg;

    // 1. Configurar pin
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar ADC
    ADC_Init(LPC_ADC, 200000);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);

    // 3. Habilitar interrupción
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, ENABLE);

    // 4. Configurar NVIC
    NVIC_SetPriority(ADC_IRQn, ((0x01<<3)|0x01));
    NVIC_EnableIRQ(ADC_IRQn);

    while(1) {
        // Iniciar conversión
        ADC_StartCmd(LPC_ADC, ADC_START_NOW);

        // Esperar resultado por interrupción
        while (!adc_done);
        adc_done = 0;

        // Procesar adc_value
        float voltage = (adc_value / 4095.0) * 3.3;

        for(volatile int i=0; i<1000000; i++);
    }
}
```

---

# PARTE 2: DAC (Digital-to-Analog Converter)

## Conceptos Fundamentales del DAC

### 1. Resolución y Conversión

El DAC del LPC1769 tiene **10 bits de resolución**:
- **Valores digitales**: 0 a 1023 (2^10 - 1)
- **Voltaje de referencia**: VREF (típicamente 3.3V)

**Fórmula de conversión:**
```
Voltaje_salida = (DAC_value / 1023) * VREF

Ejemplo con VREF = 3.3V:
  DAC_value = 512  → Voltaje = (512/1023) * 3.3V ≈ 1.65V
  DAC_value = 1023 → Voltaje = 3.3V
  DAC_value = 0    → Voltaje = 0V
```

### 2. Settling Time y Corriente

El DAC tiene dos modos de operación:

| Modo                  | Settling Time | Corriente Máxima |
|-----------------------|---------------|------------------|
| **Alta velocidad**    | 1 μs          | 700 μA           |
| **Baja corriente**    | 2.5 μs        | 350 μA           |

Configurado con `DAC_SetBias()`.

### 3. Registro de Valor DAC

El valor DAC se escribe en bits [15:6] del registro DACR:
```c
#define DAC_VALUE(n)  ((uint32_t)((n & 0x3FF) << 6))

// Ejemplo:
DAC_UpdateValue(LPC_DAC, 512);  // Escribe 512 << 6 en DACR
```

---

## Estructuras de Datos del DAC

### DAC_CURRENT_OPT

Opciones de corriente/velocidad:
```c
typedef enum {
    DAC_MAX_CURRENT_700uA = 0,  // 1μs settling, 700μA max
    DAC_MAX_CURRENT_350uA       // 2.5μs settling, 350μA max
} DAC_CURRENT_OPT;
```

### DAC_CONVERTER_CFG_Type

Configuración del DAC para DMA:
```c
typedef struct {
    uint8_t DBLBUF_ENA;  // Double buffering: 0=Disable, 1=Enable
    uint8_t CNT_ENA;     // Timeout counter: 0=Disable, 1=Enable
    uint8_t DMA_ENA;     // DMA access: 0=Disable, 1=Enable
    uint8_t RESERVED;
} DAC_CONVERTER_CFG_Type;
```

---

## Funciones del Driver DAC

### Funciones de Inicialización

#### `DAC_Init()`
```c
void DAC_Init(LPC_DAC_TypeDef *DACx);
```

Inicializa el DAC.

**Parámetros:**
- `DACx`: Siempre `LPC_DAC` (solo hay un DAC)

**Ejemplo:**
```c
DAC_Init(LPC_DAC);
```

### Funciones de Actualización de Valor

#### `DAC_UpdateValue()`
```c
void DAC_UpdateValue(LPC_DAC_TypeDef *DACx, uint32_t dac_value);
```

Actualiza el valor de salida del DAC.

**Parámetros:**
- `DACx`: `LPC_DAC`
- `dac_value`: Valor de 10 bits (0-1023)

**Ejemplo:**
```c
DAC_UpdateValue(LPC_DAC, 512);  // Salida ≈ 1.65V (con VREF=3.3V)
DAC_UpdateValue(LPC_DAC, 1023); // Salida ≈ 3.3V
DAC_UpdateValue(LPC_DAC, 0);    // Salida ≈ 0V
```

### Funciones de Configuración

#### `DAC_SetBias()`
```c
void DAC_SetBias(LPC_DAC_TypeDef *DACx, uint32_t bias);
```

Configura el modo de velocidad/corriente.

**Parámetros:**
- `DACx`: `LPC_DAC`
- `bias`:
  - `DAC_MAX_CURRENT_700uA`: Alta velocidad (1μs)
  - `DAC_MAX_CURRENT_350uA`: Baja corriente (2.5μs)

**Ejemplo:**
```c
DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_700uA);  // Máxima velocidad
```

#### `DAC_ConfigDAConverterControl()`
```c
void DAC_ConfigDAConverterControl(LPC_DAC_TypeDef *DACx, DAC_CONVERTER_CFG_Type *DAC_ConverterConfigStruct);
```

Configura control de conversión para uso con DMA.

**Parámetros:**
- `DACx`: `LPC_DAC`
- `DAC_ConverterConfigStruct`: Puntero a estructura de configuración

**Ejemplo:**
```c
DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
DAC_ConverterConfigStruct.CNT_ENA = SET;      // Habilitar timeout
DAC_ConverterConfigStruct.DMA_ENA = SET;      // Habilitar DMA
DAC_ConverterConfigStruct.DBLBUF_ENA = RESET; // Sin double buffer
DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
```

#### `DAC_SetDMATimeOut()`
```c
void DAC_SetDMATimeOut(LPC_DAC_TypeDef *DACx, uint32_t time_out);
```

Configura el timeout para DMA.

**Parámetros:**
- `DACx`: `LPC_DAC`
- `time_out`: Valor de timeout (16 bits)

**Ejemplo:**
```c
// Para generar 60Hz con 60 muestras:
uint32_t timeout = PCLK / (60 * 60);  // PCLK / (freq * samples)
DAC_SetDMATimeOut(LPC_DAC, timeout);
```

---

## Ejemplos Prácticos del DAC

### Ejemplo 1: Salida DC Simple

```c
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    PINSEL_CFG_Type PinCfg;

    // 1. Configurar pin AOUT en P0.26
    PinCfg.Funcnum = 2;      // Función 2 = DAC
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // 2. Inicializar DAC
    DAC_Init(LPC_DAC);

    // 3. Configurar para alta velocidad
    DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_700uA);

    // 4. Establecer valor (ejemplo: 1.65V)
    DAC_UpdateValue(LPC_DAC, 512);  // 512/1023 * 3.3V ≈ 1.65V

    while(1);
}
```

### Ejemplo 2: Generar Onda Triangular

```c
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"

int main(void) {
    PINSEL_CFG_Type PinCfg;
    uint16_t dac_value = 0;
    int8_t direction = 1;  // 1=subir, -1=bajar

    // Configurar pin
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar DAC
    DAC_Init(LPC_DAC);
    DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_700uA);

    while(1) {
        // Actualizar DAC
        DAC_UpdateValue(LPC_DAC, dac_value);

        // Incrementar/decrementar
        if (direction == 1) {
            dac_value += 10;
            if (dac_value >= 1023) direction = -1;
        } else {
            dac_value -= 10;
            if (dac_value <= 0) direction = 1;
        }

        // Delay para controlar frecuencia
        for(volatile int i=0; i<1000; i++);
    }
}
```

### Ejemplo 3: Generar Onda Senoidal con DMA

**Verificado en:** `library/examples/DAC/SineWave/dac_sinewave_test.c`

```c
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"

#define NUM_SINE_SAMPLE  60
#define SINE_FREQ_IN_HZ  60
#define PCLK_DAC_IN_MHZ  25

int main(void) {
    PINSEL_CFG_Type PinCfg;
    DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
    GPDMA_Channel_CFG_Type GPDMACfg;
    GPDMA_LLI_Type DMA_LLI_Struct;
    uint32_t dac_sine_lut[NUM_SINE_SAMPLE];

    // Generar tabla lookup de seno (0-90 grados, luego espejado)
    uint32_t sin_0_to_90_16_samples[16] = {
        0, 1045, 2079, 3090, 4067,
        5000, 5877, 6691, 7431, 8090,
        8660, 9135, 9510, 9781, 9945, 10000
    };

    // Crear tabla completa de seno (60 muestras)
    for(uint32_t i=0; i<NUM_SINE_SAMPLE; i++) {
        if (i <= 15) {
            dac_sine_lut[i] = 512 + 512 * sin_0_to_90_16_samples[i] / 10000;
            if (i == 15) dac_sine_lut[i] = 1023;
        } else if (i <= 30) {
            dac_sine_lut[i] = 512 + 512 * sin_0_to_90_16_samples[30-i] / 10000;
        } else if (i <= 45) {
            dac_sine_lut[i] = 512 - 512 * sin_0_to_90_16_samples[i-30] / 10000;
        } else {
            dac_sine_lut[i] = 512 - 512 * sin_0_to_90_16_samples[60-i] / 10000;
        }
        dac_sine_lut[i] = (dac_sine_lut[i] << 6);  // Shift para registro DAC
    }

    // Configurar pin AOUT
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Preparar DMA Linked List Item (circular)
    DMA_LLI_Struct.SrcAddr = (uint32_t)dac_sine_lut;
    DMA_LLI_Struct.DstAddr = (uint32_t)&(LPC_DAC->DACR);
    DMA_LLI_Struct.NextLLI = (uint32_t)&DMA_LLI_Struct;  // Loop circular
    DMA_LLI_Struct.Control = NUM_SINE_SAMPLE
                            | (2 << 18)  // Source width 32-bit
                            | (2 << 21)  // Dest width 32-bit
                            | (1 << 26); // Source increment

    // Inicializar GPDMA
    GPDMA_Init();

    // Configurar canal DMA
    GPDMACfg.ChannelNum = 0;
    GPDMACfg.SrcMemAddr = (uint32_t)dac_sine_lut;
    GPDMACfg.DstMemAddr = 0;
    GPDMACfg.TransferSize = NUM_SINE_SAMPLE;
    GPDMACfg.TransferWidth = 0;
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;  // Memory to Peripheral
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = GPDMA_CONN_DAC;
    GPDMACfg.DMALLI = (uint32_t)&DMA_LLI_Struct;
    GPDMA_Setup(&GPDMACfg);

    // Configurar DAC
    DAC_ConverterConfigStruct.CNT_ENA = SET;
    DAC_ConverterConfigStruct.DMA_ENA = SET;
    DAC_ConverterConfigStruct.DBLBUF_ENA = RESET;
    DAC_Init(LPC_DAC);

    // Calcular timeout para 60Hz con 60 muestras
    uint32_t timeout = (PCLK_DAC_IN_MHZ * 1000000) / (SINE_FREQ_IN_HZ * NUM_SINE_SAMPLE);
    DAC_SetDMATimeOut(LPC_DAC, timeout);
    DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);

    // Iniciar DMA
    GPDMA_ChannelCmd(0, ENABLE);

    // DAC genera onda senoidal de 60Hz continuamente
    while(1);
}
```

---

## Errores Comunes y Soluciones

### Error 1: Conflicto de Pines P0.26

**Síntoma:** DAC o AD0.3 no funcionan correctamente

**Causa:** P0.26 es compartido

```c
// INCORRECTO: Usar ambos simultáneamente
PinCfg.Funcnum = 1;  // AD0.3
PINSEL_ConfigPin(&PinCfg);  // P0.26
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_3, ENABLE);

PinCfg.Funcnum = 2;  // DAC
PINSEL_ConfigPin(&PinCfg);  // ¡Mismo pin!
DAC_UpdateValue(LPC_DAC, 512);

// CORRECTO: Usar solo uno u otro
// Opción 1: Solo DAC
PinCfg.Funcnum = 2;
PINSEL_ConfigPin(&PinCfg);
DAC_UpdateValue(LPC_DAC, 512);

// Opción 2: Solo ADC en otro canal
PinCfg.Pinnum = 25;   // P0.25 para AD0.2
PinCfg.Funcnum = 1;
PINSEL_ConfigPin(&PinCfg);
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
```

### Error 2: ADC_rate > 200kHz

**Síntoma:** Conversiones incorrectas o erráticas

```c
// INCORRECTO: rate muy alto
ADC_Init(LPC_ADC, 500000);  // 500kHz > máximo permitido

// CORRECTO: Máximo 200kHz
ADC_Init(LPC_ADC, 200000);  // 200kHz (máximo seguro)
```

### Error 3: No habilitar canal antes de conversión

**Síntoma:** No se obtienen datos válidos

```c
// INCORRECTO: Olvidar habilitar canal
ADC_Init(LPC_ADC, 200000);
ADC_StartCmd(LPC_ADC, ADC_START_NOW);  // ¡No habilitado!

// CORRECTO:
ADC_Init(LPC_ADC, 200000);
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);  // Habilitar canal
ADC_StartCmd(LPC_ADC, ADC_START_NOW);
```

### Error 4: DAC valor fuera de rango

**Síntoma:** Salida incorrecta o saturada

```c
// INCORRECTO: Valor de 12 bits en DAC de 10 bits
DAC_UpdateValue(LPC_DAC, 4095);  // > 1023

// CORRECTO: Rango 0-1023
DAC_UpdateValue(LPC_DAC, 1023);  // Máximo permitido
```

### Error 5: No esperar conversión en polling

**Síntoma:** Leer valores antiguos o basura

```c
// INCORRECTO: No esperar DONE
ADC_StartCmd(LPC_ADC, ADC_START_NOW);
uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);  // Puede no estar listo

// CORRECTO: Esperar flag DONE
ADC_StartCmd(LPC_ADC, ADC_START_NOW);
while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));
uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
```

---

## Patrón de Uso Típico

### ADC: Lectura Simple
```c
// 1. Configurar pin
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 1;
PinCfg.Portnum = 0;
PinCfg.Pinnum = 25;  // AD0.2
PINSEL_ConfigPin(&PinCfg);

// 2. Inicializar ADC
ADC_Init(LPC_ADC, 200000);

// 3. Habilitar canal
ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);

// 4. Loop de lectura
while(1) {
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);
    while (!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)));
    uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
    float voltage = (value / 4095.0) * 3.3;
    // Procesar...
}
```

### DAC: Salida Simple
```c
// 1. Configurar pin
PINSEL_CFG_Type PinCfg;
PinCfg.Funcnum = 2;
PinCfg.Portnum = 0;
PinCfg.Pinnum = 26;  // AOUT
PINSEL_ConfigPin(&PinCfg);

// 2. Inicializar DAC
DAC_Init(LPC_DAC);
DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_700uA);

// 3. Actualizar valor
DAC_UpdateValue(LPC_DAC, 512);  // 1.65V
```

---

## Notas Importantes sobre LPC1769

1. **Un solo ADC:** Solo hay un ADC con 8 canales, no 8 ADCs independientes.

2. **Un solo DAC:** Solo hay un DAC con una salida.

3. **Pin compartido:** P0.26 no puede ser AD0.3 y AOUT simultáneamente.

4. **Límite de velocidad ADC:** Máximo 200 kHz. ADC_clock debe ser ≤ 13 MHz.

5. **Resolución:**
   - ADC: 12 bits (0-4095)
   - DAC: 10 bits (0-1023)

6. **VREF:** Típicamente 3.3V, pero verificar diseño del hardware.

7. **Vector de interrupción ADC:** `ADC_IRQn` → `ADC_IRQHandler()`

8. **Aplicaciones comunes:**
   - ADC: Sensores de temperatura, luz, potenciómetros, acelerómetros
   - DAC: Generación de audio, control de motores, señales de referencia

---

## Referencias

- **ADC Header:** `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_adc.h`
- **ADC Source:** `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_adc.c`
- **DAC Header:** `library/CMSISv2p00_LPC17xx/Drivers/inc/lpc17xx_dac.h`
- **DAC Source:** `library/CMSISv2p00_LPC17xx/Drivers/src/lpc17xx_dac.c`
- **Ejemplos verificados:**
  - `library/examples/ADC/Polling/adc_polling_test.c`
  - `library/examples/ADC/Burst/adc_burst_test.c`
  - `library/examples/DAC/SineWave/dac_sinewave_test.c`
- **Manual del usuario:** LPC17xx User Manual (UM10360)
  - Capítulo 29: ADC
  - Capítulo 30: DAC
- **Pin functions:** Tabla 8.5 del manual del usuario LPC17xx

---

## Resumen

Los **conversores ADC y DAC del LPC1769** permiten interfaz completa con el mundo analógico:

**ADC:**
- **12 bits**, 8 canales, hasta 200 kHz
- **Modos flexibles**: Polling, Burst, Hardware trigger, Interrupciones
- **Aplicaciones**: Sensores analógicos, mediciones de voltaje

**DAC:**
- **10 bits**, 1 canal de salida
- **Dos modos**: Alta velocidad (1μs) o baja corriente (2.5μs)
- **Soporte DMA**: Para generación de formas de onda complejas
- **Aplicaciones**: Generación de audio, control analógico, señales de referencia

**Puntos clave:**
1. Configurar PINSEL correctamente antes de usar ADC/DAC
2. P0.26 es compartido entre AD0.3 y AOUT - usar solo uno
3. ADC requiere esperar flag DONE en modo polling
4. DAC con DMA permite generar señales complejas eficientemente
5. Convertir valores digitales a voltaje: `V = (value / max_value) * VREF`
