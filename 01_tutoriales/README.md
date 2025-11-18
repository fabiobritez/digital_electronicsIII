# Tutoriales - LPC1769

Esta carpeta contiene todos los tutoriales completos sobre los periféricos del microcontrolador LPC1769 (ARM Cortex-M3).

## Orden recomendado de estudio

Sigue este orden para un aprendizaje progresivo:

### 1. Configuración básica de pines
- **[01_pinsel](./01_pinsel/)** - Configuración de funciones de pines (PINSEL, PINMODE, PINMODE_OD)

### 2. Entrada/Salida digital
- **[02_gpio](./02_gpio/)** - GPIO (General Purpose Input/Output)
  - Instalación de MCUXpresso IDE
  - Configuración de pines como entrada/salida
  - Control de LEDs, lectura de botones

### 3. Temporización básica
- **[03_systick](./03_systick/)** - Timer SysTick del Cortex-M3
  - Temporizador de 24 bits
  - Interrupciones periódicas
  - Base de tiempo para aplicaciones

### 4. Sistema de interrupciones
- **[04_nvic](./04_nvic/)** - NVIC (Nested Vectored Interrupt Controller)
  - Prioridades de interrupción
  - Configuración del NVIC
  - 35 interrupciones vectorizadas, 32 niveles de prioridad

### 5. Interrupciones GPIO
- **[06_interrupciones](./06_interrupciones/)** - Interrupciones por GPIO
  - Interrupciones externas (EINT)
  - Interrupciones por cambio de estado en pines
  - Aplicaciones prácticas

### 6. Timers avanzados
- **[05_timers](./05_timers/)** - Timers/Contadores
  - Timers de 32 bits
  - Modos: Match, Capture, PWM
  - Aplicaciones de temporización precisa

### 7. Transferencia de datos avanzada
- **[07_dma](./07_dma/)** - GPDMA (General Purpose DMA)
  - Transferencias sin uso de CPU
  - Memoria a memoria, periférico a memoria
  - Linked List Items (LLI)

### 8. Comunicación serial
- **[08_uart](./08_uart/)** - UART (Universal Asynchronous Receiver/Transmitter)
  - Comunicación serie asíncrona
  - Configuración de baudrate
  - Transmisión y recepción de datos

### 9. Conversión de señales
- **[09_adc_dac](./09_adc_dac/)** - ADC y DAC
  - Conversor Analógico-Digital (ADC)
  - Conversor Digital-Analógico (DAC)
  - Modos de conversión

### 10. Herramientas de depuración
- **[10_debug_framework](./10_debug_framework/)** - Debug Framework
  - Herramientas de debugging
  - Printf para sistemas embebidos
  - Utilidades de depuración

---

## Estructura de cada tutorial

Cada tutorial contiene:
- 📖 **README.md** - Guía principal del tema
- 📄 **Archivos .md** - Documentación detallada
- 📁 **src/** - Código fuente de ejemplo (cuando aplica)
- 🖼️ **img/** - Imágenes y diagramas (cuando aplica)

---

## Recursos adicionales

- **Ejemplos prácticos**: Ver [/02_ejemplos](../02_ejemplos/)
- **Ejercicios de parcial**: Ver [/03_ejercicios](../03_ejercicios/)
- **Biblioteca CMSIS**: Ver [/library](../library/)

---

## Referencias

- **Datasheet LPC1769**: [NXP LPC1769 Datasheet](https://www.nxp.com/docs/en/data-sheet/LPC1769_68_67_66_65_64_63.pdf)
- **User Manual LPC17xx**: [UM10360](https://www.nxp.com/docs/en/user-guide/UM10360.pdf)
- **CMSIS Documentation**: [ARM CMSIS](https://arm-software.github.io/CMSIS_5/)
