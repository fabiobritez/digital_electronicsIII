# Módulo 8: Timers 0–3

Los **Timers** son contadores de 32 bits que cuentan pulsos de reloj. Con ellos medís tiempo con
precisión, generás eventos periódicos, medís el ancho o la frecuencia de una señal (modo *capture*) o
generás señales (modo *match* / PWM). El LPC1769 tiene **4 timers idénticos** (Timer 0 a 3).

A diferencia del SysTick (24 bits, parte del núcleo), estos son periféricos de NXP de 32 bits, más
potentes y flexibles. Capítulo 21 del manual.

## Recorrido

1. [01 - Timers a nivel registro](./01-timers-registros.md)
   La arquitectura (`TC`, `PR`/`PC`), el mapa de registros, `MCR` a fondo (los 3 bits por match), `IR` y
   el write-1-to-clear, el cálculo de tiempo, demora por polling, los 4 timers y sus pines, y el
   contraste con SysTick / PWM.
2. [02 - Timers con el driver CMSIS](./02-timers-con-driver.md)
   Qué hace `TIM_Init` por debajo, `TIM_ConfigMatch` pensando en microsegundos, varios canales de match
   en un mismo timer, el timer disparando el ADC (Timer → ADC → DMA) y los errores comunes.
3. [03 - Capture, counter y match externo](./03-capture-y-medicion.md)
   `CCR`/`CR0`-`CR1` para medir ancho de pulso, período y frecuencia (frecuencímetro); el modo counter
   (`CTCR`) para contar eventos externos; `EMR` y los pines `MATn.x` para generar señales por hardware
   (onda cuadrada y PWM rudimentario); y el contraste completo con PWM1, SysTick y QEI.

## El ritual de arranque aplicado a un timer
1. **Encender** → `PCONP` (Timer0 = bit 1, Timer1 = 2, Timer2 = 22, Timer3 = 23).
2. **Clockear** → `PCLKSEL` (por defecto CCLK/4 = 25 MHz; afecta todos los cálculos de tiempo).
3. **Pines** (solo si usás capture/match en pines) → `PINSEL`.
4. **Configurar** → prescaler (`PR`), match (`MR`), qué hacer al hacer match (`MCR`).
5. **Usar** → arrancar con `TCR`, atender la interrupción.

## Antes de esto
Módulos 3 (clock/power, clave para los tiempos) y 7 (interrupciones, el timer suele interrumpir).

## Manual
Capítulo 21: [`manual/ch21_timer-0-1-2-3.pdf`](../../manual/ch21_timer-0-1-2-3.pdf). El RIT
(capítulo 22), un timer mínimo solo-interrupciones, se presenta brevemente en la página 1. Para PWM
dedicado, ver el [módulo 16 - PWM](../16_pwm/). Material original en [`_origen/`](./_origen/).

---

**Anterior:** [07 - Interrupciones](../07_interrupciones/) · **Siguiente:** [09 - UART](../09_uart/)
