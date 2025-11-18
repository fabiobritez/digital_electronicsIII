# Electrónica Digital III - LPC1769

Repositorio educativo completo para la materia **Electrónica Digital III** con enfoque en programación de microcontroladores ARM Cortex-M3, específicamente el **LPC1769** de NXP.

---

## 📋 Contenido del Repositorio

### 🎓 [00_fundamentos](./00_fundamentos/)
Fundamentos de programación en C para sistemas embebidos
- **[c_basics](./00_fundamentos/c_basics/)** - Tutorial completo de C
  - Declaraciones, tipos de datos y constantes
  - Operadores (aritméticos, lógicos, bitwise)
  - Control de flujo (if/else, switch, loops)
  - Funciones y modularización
  - Punteros y manejo de memoria
  - Preprocesador y compilación
  - Tipos de datos avanzados (estructuras, uniones, enums)

### 📚 [01_tutoriales](./01_tutoriales/)
Tutoriales completos de todos los periféricos del LPC1769, organizados en orden de aprendizaje progresivo:

1. **[01_pinsel](./01_tutoriales/01_pinsel/)** - Configuración de pines (PINSEL, PINMODE, PINMODE_OD)
2. **[02_gpio](./01_tutoriales/02_gpio/)** - GPIO + MCUXpresso IDE
3. **[03_systick](./01_tutoriales/03_systick/)** - Timer SysTick
4. **[04_nvic](./01_tutoriales/04_nvic/)** - Sistema de interrupciones (NVIC)
5. **[05_timers](./01_tutoriales/05_timers/)** - Timers/Contadores
6. **[06_interrupciones](./01_tutoriales/06_interrupciones/)** - Interrupciones GPIO y EINT
7. **[07_dma](./01_tutoriales/07_dma/)** - GPDMA (Direct Memory Access)
8. **[08_uart](./01_tutoriales/08_uart/)** - Comunicación serial UART
9. **[09_adc_dac](./01_tutoriales/09_adc_dac/)** - Conversores ADC y DAC
10. **[10_debug_framework](./01_tutoriales/10_debug_framework/)** - Herramientas de depuración

### 💻 [02_ejemplos](./02_ejemplos/)
Ejemplos de código completos y funcionales para cada periférico:
- **gpio/** - Control de LEDs, lectura de botones
- **systick/** - Base de tiempo, delays
- **interrupciones/** - Manejo de interrupciones externas
- **timers/** - Patrones, PWM, medición de frecuencias
- **dma/** - Transferencias memoria-memoria, ADC-DMA, DAC-DMA
- **uart/** - Comunicación serial
- **adc_dac/** - Conversión de señales

### 📝 [03_ejercicios](./03_ejercicios/)
Ejercicios resueltos de parciales de años anteriores:
- **[2022](./03_ejercicios/2022/)** - Parciales 2022
- **[2023](./03_ejercicios/2023/)** - Parciales 2023
- **[2025](./03_ejercicios/2025/)** - Parciales 2025

Incluye análisis de errores comunes y mejores prácticas.

### 📦 [library](./library/)
Biblioteca oficial CMSIS v2.00 para LPC17xx:
- **CMSISv2p00_LPC17xx/** - Core CMSIS y drivers de periféricos
  - Headers del ARM Cortex-M3
  - 25+ drivers para periféricos (GPIO, UART, ADC, DAC, DMA, Timers, etc.)
- **examples/** - Más de 100 ejemplos oficiales organizados por periférico
  - ADC, CAN, DAC, EMAC (Ethernet), I2C, I2S, SPI
  - USB (Audio, CDC, HID), LCD, PWM, RTC, Watchdog
  - Y mucho más...

---

## 🚀 Guía de inicio rápido

### Para principiantes

1. **Empieza con C** → [00_fundamentos/c_basics](./00_fundamentos/c_basics/)
   - Aprende los fundamentos de C para embebidos

2. **Configura tu IDE** → [01_tutoriales/02_gpio/0-ide.md](./01_tutoriales/02_gpio/0-ide.md)
   - Instala MCUXpresso IDE

3. **Primer periférico** → [01_tutoriales/02_gpio](./01_tutoriales/02_gpio/)
   - Aprende GPIO (LEDs y botones)

4. **Prueba ejemplos** → [02_ejemplos/gpio](./02_ejemplos/gpio/)
   - Compila y carga código en tu placa

### Para estudiantes preparando parciales

1. **Repasa tutoriales** → [01_tutoriales](./01_tutoriales/)
2. **Practica con ejercicios** → [03_ejercicios](./03_ejercicios/)
3. **Analiza errores comunes** → Ver archivos `*_errores.md` en ejercicios
4. **Experimenta con ejemplos** → [02_ejemplos](./02_ejemplos/)

### Para desarrollo avanzado

1. **Explora la biblioteca CMSIS** → [library/CMSISv2p00_LPC17xx](./library/CMSISv2p00_LPC17xx/)
2. **Revisa ejemplos oficiales** → [library/examples](./library/examples/)
3. **Integra múltiples periféricos** → Ver ejercicios de parciales

---

## 🛠️ Hardware y Herramientas

### Microcontrolador: LPC1769
- **Core:** ARM Cortex-M3
- **Frecuencia:** Hasta 120 MHz
- **Flash:** 512 KB
- **RAM:** 64 KB (32 KB local, 32 KB AHB)
- **GPIO:** 70 pines de I/O
- **Periféricos:** UART, I2C, SPI, CAN, ADC, DAC, DMA, USB, Ethernet

### Software necesario
- **IDE:** MCUXpresso IDE (gratuito)
- **Compilador:** arm-none-eabi-gcc (incluido en MCUXpresso)
- **Debugger:** OpenOCD / LPC-Link (según tu placa)

---

## 📖 Estructura de aprendizaje recomendada

```
Nivel 1: Fundamentos
├── C para embebidos (00_fundamentos)
├── Configuración de pines (PINSEL)
└── GPIO básico

Nivel 2: Temporización
├── SysTick
├── Interrupciones (NVIC)
└── Timers

Nivel 3: Comunicación
├── UART
├── I2C / SPI (en library/examples)
└── CAN (en library/examples)

Nivel 4: Conversión y DMA
├── ADC / DAC
├── DMA
└── Integración ADC-DMA, DAC-DMA

Nivel 5: Aplicaciones avanzadas
├── USB (en library/examples)
├── Ethernet (en library/examples)
└── Proyectos integrados
```

---

## 📚 Recursos externos

### Documentación oficial
- [LPC1769 Datasheet](https://www.nxp.com/docs/en/data-sheet/LPC1769_68_67_66_65_64_63.pdf)
- [LPC17xx User Manual (UM10360)](https://www.nxp.com/docs/en/user-guide/UM10360.pdf)
- [ARM Cortex-M3 Technical Reference Manual](https://developer.arm.com/documentation/ddi0337/latest/)

### Estándares y bibliotecas
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/)
- [ARM CMSIS GitHub](https://github.com/ARM-software/CMSIS_5)

### Tutoriales recomendados
- "The C Programming Language" - Kernighan & Ritchie
- "Embedded C Programming" - Mark Siegesmund
- "The Definitive Guide to ARM Cortex-M3/M4" - Joseph Yiu

---

## 🤝 Contribuciones

Este es un repositorio educativo. Si encuentras errores o tienes sugerencias:
1. Abre un Issue describiendo el problema
2. Propone mejoras mediante Pull Requests
3. Comparte tus propios ejemplos y ejercicios

---

## 📄 Licencia

Material educativo para uso académico. La biblioteca CMSIS mantiene su licencia original de ARM.

---

## 🎯 Objetivos de aprendizaje

Al completar este material, serás capaz de:

✅ Programar en C para sistemas embebidos
✅ Configurar y usar periféricos del LPC1769
✅ Manejar interrupciones y prioridades
✅ Implementar comunicación serial (UART, I2C, SPI)
✅ Usar conversores ADC y DAC
✅ Optimizar transferencias con DMA
✅ Desarrollar aplicaciones embebidas complejas
✅ Depurar código en microcontroladores

---

## 📞 Contacto y soporte

Para preguntas sobre el material:
- Revisa primero los tutoriales y ejemplos
- Consulta los ejercicios resueltos
- Lee el User Manual del LPC17xx
- Consulta con tus docentes de la materia

---

**¡Feliz aprendizaje! 🚀**

*Última actualización: 2025*
