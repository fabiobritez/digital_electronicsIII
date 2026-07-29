# Módulo 21: Periféricos adicionales (plus)

El LPC1769 tiene más periféricos de los que se ven en una cursada típica. Este módulo los reúne como
**referencia de consulta**: no hace falta dominarlos para aprobar, pero saber que existen y cómo se
encaran te abre la puerta a proyectos más ambiciosos (un datalogger con hora real, un sistema robusto
que se auto-recupera, control de motores, audio, redes).

Todos siguen el **mismo molde** que ya conocés: encenderlos por `PCONP`, clockearlos por `PCLKSEL`,
conectar sus pines por `PINSEL`, configurar sus registros y usarlos. Y todos tienen su driver CMSIS y
ejemplos oficiales. La progresión registro → driver del resto del curso aplica igual acá.

## Periféricos

| # | Periférico | Para qué sirve | Dificultad |
|---|-----------|----------------|-----------|
| 1 | [RTC](./01-rtc.md) | Reloj de tiempo real: fecha y hora, alarmas, muy bajo consumo | Baja |
| 2 | [Watchdog (WDT)](./02-watchdog.md) | Resetear el sistema si el firmware se cuelga | Baja |
| 3 | [QEI](./03-qei.md) | Leer encoders en cuadratura (posición/velocidad de motores) | Media |
| 4 | [CAN: protocolo y bit timing](./04-can.md) | Bus robusto multinodo: frame, arbitraje, `CANBTR`, modos, errores | Media-alta |
| 4b | [CAN: filtro de aceptación y driver](./04b-can-filtro.md) | Acceptance Filter (FullCAN/SFF/EFF), recepción por IRQ, transceiver, driver | Media-alta |
| 5 | [I2S](./05-i2s.md) | Audio digital (ADC/DAC de audio, códecs) | Media-alta |
| 6 | [Ethernet: EMAC y descriptores](./06-ethernet.md) | El controlador Ethernet por hardware: tramas, DMA, descriptores | Alta |
| 7 | [Ethernet: del EMAC a TCP/IP](./07-ethernet-tcpip.md) | PHY, RMII/MDIO y la pila TCP/IP (lwIP/uIP) | Alta |

## Por dónde empezar
- **RTC** y **WDT** son los más cortos y los más útiles en proyectos reales: un reloj que mantiene la
  hora y un perro guardián que rescata un sistema colgado. Empezá por ahí.
- **QEI** es natural si vas a control de motores (combina bien con PWM, módulo 19).
- **CAN**, **I2S** y **Ethernet** son temas grandes; acá va lo justo para entender qué resuelven y cómo
  se ven sus registros, con punteros al manual y a los ejemplos para profundizar.

## Antes de esto
Las bases (módulos 0–4) y, según el periférico, interrupciones (módulo 7) y DMA (módulo 11). Cada
página lo aclara.

## Manual y ejemplos
Cada periférico tiene su capítulo en [`../../manual/`](../../manual/) y ejemplos oficiales en
[`../../library/examples/`](../../library/examples/). Los links están en cada página.

---

**Anterior:** [20 - Hardware y placa](../20_hardware_y_placa/) · **Volver al** [índice del curso](../README.md)
