# Módulo 19: PWM

El **PWM** (Pulse Width Modulation, modulación por ancho de pulso) es una técnica para "simular" un
valor analógico con una salida digital, prendiendo y apagando un pin muy rápido. Es de lo más usado en
la práctica:

- **brillo de un LED** (más tiempo prendido = más brillo),
- **velocidad de un motor** de continua,
- **posición de un servo**,
- generar tonos, controlar fuentes, etc.

Ya vimos en [Timers (módulo 8)](../08_timers/) que se puede generar PWM con el *match externo*. Pero
el LPC1769 tiene un **periférico PWM dedicado** (PWM1) con **6 canales** que comparten un mismo período,
ideal para manejar varios actuadores a la vez. Capítulo 24 del manual.

## La idea: duty cycle

Una señal PWM se define por dos números:

- el **período** (cada cuánto se repite),
- el **duty cycle** (qué fracción del período el pin está en alto).

```
 alto ┌────┐      ┌────┐      ┌────┐        duty = 25%  (poco brillo / poca velocidad)
 bajo─┘    └──────┘    └──────┘    └──
      |<-T->|

 alto ┌──────────┐  ┌──────────┐            duty = 75%  (mucho brillo / mucha velocidad)
 bajo─┘          └──┘          └──
```

Mismo período, distinto duty. El "valor analógico" que ve el actuador es el **promedio**: 25% de duty
≈ 25% de la tensión. Cambiando solo el duty controlás el LED, el motor o el servo.

## Recorrido

1. [01 - PWM a nivel registro](./01-pwm-registros.md)
   La arquitectura (TC/PC/PR + los 7 match registers), cómo `MR0` fija el período y `MR1–6` los puntos
   de conmutación, **single-edge vs double-edge** (`PCR`: `PWMSEL`/`PWMENA`), el **latch** (`LER`)
   bien explicado, `MCR`/`TCR`, el cálculo de frecuencia y resolución (LED, servo, motor) y el
   contraste con el match del Timer. Servo de ejemplo.
2. [02 - PWM con el driver CMSIS](./02-pwm-con-driver.md)
   `PWM_Init` / `PWM_MatchUpdate` y la correspondencia con cada registro; fade de un LED, double-edge
   con fase, e interrupción por período (`MR0`) para tablas de duty.

## El ritual de arranque aplicado al PWM
1. **Encender** → `PCONP` bit 6 (`PCPWM1`). El manual aclara que tras el reset ya viene en 1
   (el PWM es de los pocos periféricos encendidos de fábrica); igual conviene setearlo explícito.
2. **Clockear** → `PCLKSEL` (define junto al prescaler la resolución temporal).
3. **Pines** → `PINSEL` (PWM1.1 = P2.0, …, PWM1.6 = P2.5, función 01; cada canal tiene además un
   pin alternativo en P1: P1.18, P1.20, P1.21, P1.23, P1.24, P1.26, función 10).
4. **Configurar** → período (`MR0`), duty (`MR1..6`), habilitar salidas (`PCR`), latch (`LER`).
5. **Usar** → arrancar, y variar el duty cuando quieras.

## Cuándo leerlo
Después del [módulo 8 (Timers)](../08_timers/): el PWM es un timer especializado, comparte la idea de
prescaler y match.

## Manual
Capítulo 24: [`manual/ch24_pulse-width-modulator.pdf`](../../manual/ch24_pulse-width-modulator.pdf).

---

**Anterior:** [18 - Toolchain y entorno](../18_toolchain_y_entorno/) ·
**Siguiente:** [20 - Hardware y placa](../20_hardware_y_placa/)
