# Ejemplos Prácticos - LPC1769

Esta carpeta contiene ejemplos de código completos y funcionales para cada periférico del LPC1769.

## 📂 Estructura

### GPIO - Entrada/Salida Digital
- **[gpio/](./gpio/)** - Ejemplos de uso de GPIO
  - Implementación de handlers GPIO
  - Control de LEDs
  - Lectura de botones

### SysTick - Temporizador del Sistema
- **[systick/](./systick/)** - Ejemplos de SysTick
  - Configuración de interrupciones periódicas
  - Base de tiempo de 10ms
  - Aplicaciones de temporización

### Interrupciones
- **[interrupciones/](./interrupciones/)** - Ejemplos de interrupciones
  - Interrupciones GPIO
  - Manejo de NVIC
  - Prioridades de interrupción

### Timers
- **[timers/](./timers/)** - Ejemplos de timers
  - **patterns/** - Generación de patrones
  - **lineas/** - Control de líneas
  - Match, Capture, PWM

### DMA - Acceso Directo a Memoria
- **[dma/](./dma/)** - Ejemplos de GPDMA
  - `lli_example.c` - Linked List Items
  - `m2m.c` - Memoria a memoria
  - `adc_dma_simple.c` - ADC con DMA
  - `dac_dma_sin.c` - DAC con DMA (señal sinusoidal)

### UART - Comunicación Serial
- **[uart/](./uart/)** - Ejemplos de UART
  - Transmisión y recepción de datos
  - Configuración de baudrate

### ADC/DAC - Conversión de Señales
- **[adc_dac/](./adc_dac/)** - Ejemplos de conversores
  - ADC: Conversión analógica a digital
  - DAC: Generación de señales analógicas

---

## 🚀 Cómo usar estos ejemplos

1. **Estudia primero el tutorial correspondiente** en [/01_tutoriales](../01_tutoriales/)
2. **Lee el código del ejemplo** para entender la implementación
3. **Importa el proyecto en MCUXpresso IDE**
4. **Compila y carga** en tu LPC1769
5. **Experimenta** modificando parámetros y funcionalidad

---

## 📚 Más ejemplos

Para más de 100 ejemplos adicionales de la biblioteca CMSIS oficial, ver:
- **[/library/examples](../library/examples/)** - Ejemplos organizados por periférico

Incluye ejemplos avanzados de:
- ADC (Burst, DMA, Hardware Trigger)
- CAN Bus
- DAC (DMA, Speaker, Wave Generation)
- Ethernet (EMAC, uIP stack TCP/IP)
- I2C, I2S, SPI, SSP
- LCD (Nokia 6610, QVGA TFT)
- MCPWM (Motor Control PWM)
- USB (Audio, CDC, HID)
- RTC, Watchdog, y más...

---

## 💡 Consejos

- Cada ejemplo está diseñado para ser **autocontenido**
- Los ejemplos usan la **biblioteca CMSIS** ubicada en `/library`
- Revisa los **comentarios en el código** para entender cada sección
- Consulta el **User Manual del LPC17xx** para detalles de registros

---

## 🔗 Enlaces útiles

- **Tutoriales**: [/01_tutoriales](../01_tutoriales/)
- **Ejercicios de parcial**: [/03_ejercicios](../03_ejercicios/)
- **Fundamentos de C**: [/00_fundamentos/c_basics](../00_fundamentos/c_basics/)
