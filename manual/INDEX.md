# Índice del User Manual UM10360 (LPC176x/5x): dividido por capítulos

El manual original (`../UM10360.pdf`, 851 páginas) está dividido aquí en **35 sub-PDFs**, uno
por capítulo, para que consultar un periférico puntual sea rápido y no tengas que abrir las 851
páginas. Cada archivo conserva el contenido íntegro del capítulo.

> **Cómo usar este índice:** buscá el periférico que estás estudiando en la primera tabla
> (está ordenada por módulo de la materia, no por número de capítulo). La columna "Páginas
> orig." te dice en qué página del PDF completo está, por si necesitás citarla.

---

## Mapa: módulo de la materia → capítulo del manual

| Módulo / Tutorial del repo | Capítulo del manual | Archivo | Páginas orig. |
|---|---|---|---|
| **Fundamentos / arquitectura** | | | |
| Memoria y mapa de direcciones | Cap. 2: Memory map | [ch02](ch02_memory-map.pdf) | 14–18 |
| Control de sistema (reset, EINT) | Cap. 3: System control | [ch03](ch03_system-control.pdf) | 19–30 |
| **Clock y power** (PLL, PCONP, PCLKSEL) | Cap. 4: Clocking and power control | [ch04](ch04_clocking-and-power-control.pdf) | 31–69 |
| Flash accelerator | Cap. 5: Flash accelerator | [ch05](ch05_flash-accelerator.pdf) | 70–73 |
| **PINSEL / pines** | | | |
| Configuración de pines (electrical) | Cap. 7: Pin configuration | [ch07](ch07_pin-configuration.pdf) | 93–113 |
| **PINSEL / PINMODE** (función de pin) | Cap. 8: Pin connect block | [ch08](ch08_pin-connect-block.pdf) | 114–129 |
| **GPIO** | Cap. 9: GPIO | [ch09](ch09_general-purpose-input-output.pdf) | 130–150 |
| **Interrupciones** | | | |
| **NVIC** | Cap. 6: NVIC | [ch06](ch06_nested-vectored-interrupt-controller.pdf) | 74–92 |
| Interrupciones externas (EINT0-3) | Cap. 3 §3.6: External interrupts | [ch03](ch03_system-control.pdf) | 24–28 |
| **Timers y temporización** | | | |
| **SysTick** | Cap. 23: System Tick Timer | [ch23](ch23_system-tick-timer.pdf) | 515–519 |
| **Timers 0/1/2/3** | Cap. 21: Timer 0/1/2/3 | [ch21](ch21_timer-0-1-2-3.pdf) | 501–511 |
| RIT (Repetitive Interrupt Timer) | Cap. 22: RIT | [ch22](ch22_repetitive-interrupt-timer.pdf) | 512–514 |
| Watchdog | Cap. 28: WDT | [ch28](ch28_watchdog-timer.pdf) | 580–584 |
| RTC | Cap. 27: RTC | [ch27](ch27_real-time-clock-and-backup-registers.pdf) | 569–579 |
| **PWM** | | | |
| **PWM** | Cap. 24: Pulse Width Modulator | [ch24](ch24_pulse-width-modulator.pdf) | 520–532 |
| Motor control PWM | Cap. 25: Motor control PWM | [ch25](ch25_motor-control-pwm.pdf) | 533–553 |
| QEI (encoder en cuadratura) | Cap. 26: QEI | [ch26](ch26_quadrature-encoder-interface.pdf) | 554–568 |
| **Comunicación serial** | | | |
| **UART0/2/3** | Cap. 14: UART0/2/3 | [ch14](ch14_uart0-2-3.pdf) | 308–327 |
| UART1 (con modem) | Cap. 15: UART1 | [ch15](ch15_uart1.pdf) | 328–352 |
| **I2C** | Cap. 19: I2C0/1/2 | [ch19](ch19_i2c0-1-2.pdf) | 439–483 |
| **SPI** | Cap. 17: SPI | [ch17](ch17_spi.pdf) | 412–422 |
| **SSP** (SPI/SSI mejorado) | Cap. 18: SSP0/1 | [ch18](ch18_ssp0-1.pdf) | 423–438 |
| I2S (audio) | Cap. 20: I2S | [ch20](ch20_i2s.pdf) | 484–500 |
| CAN | Cap. 16: CAN1/2 | [ch16](ch16_can1-2.pdf) | 353–411 |
| **Conversión analógica** | | | |
| **ADC** | Cap. 29: ADC | [ch29](ch29_analog-to-digital-converter.pdf) | 585–592 |
| **DAC** | Cap. 30: DAC | [ch30](ch30_digital-to-analog-converter.pdf) | 593–596 |
| **DMA** | | | |
| **GPDMA** | Cap. 31: General Purpose DMA | [ch31](ch31_general-purpose-dma.pdf) | 597–625 |
| **Conectividad avanzada** | | | |
| Ethernet | Cap. 10: Ethernet | [ch10](ch10_ethernet.pdf) | 151–222 |
| USB device | Cap. 11: USB device | [ch11](ch11_usb-device-controller.pdf) | 223–278 |
| USB host | Cap. 12: USB host | [ch12](ch12_usb-host-controller.pdf) | 279–282 |
| USB OTG | Cap. 13: USB OTG | [ch13](ch13_usb-otg.pdf) | 283–307 |
| **Programación / debug** | | | |
| Flash memory interface (IAP/ISP) | Cap. 32: Flash interface | [ch32](ch32_flash-memory-interface-and-programming.pdf) | 626–652 |
| JTAG / SWD / Trace | Cap. 33: JTAG/SWD | [ch33](ch33_jtag-serial-wire-debug-and-trace.pdf) | 653–655 |
| **Cortex-M3 (núcleo ARM)** | Cap. 34: Appendix Cortex-M3 user guide | [ch34](ch34_appendix-cortex-m3-user-guide.pdf) | 656–811 |
| Introducción general | Cap. 1: Introductory information | [ch01](ch01_introductory-information.pdf) | 5–13 |
| Información suplementaria | Cap. 35: Supplementary information | [ch35](ch35_supplementary-information.pdf) | 812–851 |

---

## Atajos a registros clave (nivel registro)

Para la primera parte de la materia (a nivel de registros), estos son los registros que más vas
a tocar y dónde encontrarlos:

| Registro | Para qué | Capítulo / archivo |
|---|---|---|
| `PCONP` (0x400FC0C4) | Encender/apagar la alimentación de cada periférico | Cap. 4 §4.8.9: [ch04](ch04_clocking-and-power-control.pdf) |
| `PCLKSEL0/1` | Divisor del clock de cada periférico | Cap. 4 §4.7.3: [ch04](ch04_clocking-and-power-control.pdf) |
| `PINSEL0..10` | Función de cada pin | Cap. 8: [ch08](ch08_pin-connect-block.pdf) |
| `PINMODE0..9` | Pull-up / pull-down / tri-state | Cap. 8: [ch08](ch08_pin-connect-block.pdf) |
| `FIODIR / FIOSET / FIOCLR / FIOPIN / FIOMASK` | GPIO (dirección y nivel) | Cap. 9: [ch09](ch09_general-purpose-input-output.pdf) |
| `EXTINT / EXTMODE / EXTPOLAR` | Interrupciones externas EINT0-3 | Cap. 3 §3.6: [ch03](ch03_system-control.pdf) |
| `IO0IntEnR/F`, `IO2IntEnR/F`, `IO0IntStat` | Interrupciones por GPIO | Cap. 9: [ch09](ch09_general-purpose-input-output.pdf) |
| `ISER0/1`, `ICER0/1`, `IPR0..` | NVIC (habilitar / prioridad) | Cap. 6: [ch06](ch06_nested-vectored-interrupt-controller.pdf) |
| `T[0-3]TCR/TC/MR0/MCR/CCR` | Timers | Cap. 21: [ch21](ch21_timer-0-1-2-3.pdf) |
| `STCTRL / STRELOAD / STCURR` | SysTick | Cap. 23: [ch23](ch23_system-tick-timer.pdf) |
| `U[0-3]RBR/THR/LCR/DLL/DLM/LSR` | UART | Cap. 14: [ch14](ch14_uart0-2-3.pdf) |
| `AD0CR / AD0GDR / AD0DR0-7 / AD0STAT` | ADC | Cap. 29: [ch29](ch29_analog-to-digital-converter.pdf) |
| `DACR` | DAC | Cap. 30: [ch30](ch30_digital-to-analog-converter.pdf) |
| `GPDMA` (DMACConfig, canales, LLI) | DMA | Cap. 31: [ch31](ch31_general-purpose-dma.pdf) |
| `PWM1MR0-6, PWM1MCR, PWM1PCR, PWM1LER` | PWM | Cap. 24: [ch24](ch24_pulse-width-modulator.pdf) |

> **Nota sobre numeración de páginas:** los visores PDF abren por número de página *física*
> (1–851). La numeración impresa en el pie del manual puede diferir en ±10. Las columnas de
> arriba usan la página *física* del PDF.

---

*Generado a partir de `UM10360.pdf` Rev. (NXP). Para regenerar el split, ver `../tools/split_manual.py`.*
