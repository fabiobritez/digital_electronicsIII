# Módulo 4: PINSEL, la función de cada pin

El LPC1769 tiene muchos más periféricos que pines. La solución: **casi todos los pines son
multifunción**. Un mismo pin puede ser GPIO, o TXD de una UART, o un canal de ADC, o una salida de
PWM… según cómo lo configures. El bloque que decide **qué función cumple cada pin** se llama
**Pin Connect Block**, y se maneja con los registros **PINSEL / PINMODE / PINMODE_OD**.

> **Regla de oro:** PINSEL es **siempre el primer paso de conexión** de cualquier periférico que use
> pines (después de encenderlo y clockearlo, módulo 3). Si te olvidás, el periférico funciona pero
> "no sale por ninguna parte", porque el pin sigue en otra función.

## Recorrido

1. [01 - La función de los pines: PINSEL, PINMODE, PINMODE_OD](./01-funcion-de-los-pines.md)
   Qué hace cada registro, cómo se eligen los 2 bits de cada pin, y cómo configurarlos a mano.
2. [02 - Configurar pines con CMSIS y el driver PINSEL](./02-configurar-pines-con-cmsis.md)
   De `LPC_PINCON->PINSEL0` al cómodo `PINSEL_ConfigPin()`. Ejemplos para UART, ADC, I2C, PWM.
3. [03 - El mapa completo de registros y la fórmula del pin](./03-mapa-de-registros.md)
   PINSEL0-10, PINMODE0-9, PINMODE_OD0-4: qué controla cada uno y la fórmula para sacar registro y
   corrimiento de cualquier pin `Px.y`.
4. [04 - PINMODE, open-drain, I²C y tolerancia 5 V](./04-pinmode-opendrain-tolerancia.md)
   Repeater, tri-state para analógico, open-drain (I²C, wired-AND), el caso especial P0.27/P0.28 con
   `I2CPADCFG`, qué pines toleran 5 V y la corriente de drive.

> El material original de este tema quedó en [`_origen/`](./_origen/) por si querés compararlo.

## Antes de esto
Módulos [01 (registros)](../01_arquitectura_y_acceso_a_registros/) y
[03 (clock/power)](../03_clock_y_power/).

## Manual
**Capítulo 8** (Pin connect block): [`manual/ch08...`](../../manual/ch08_pin-connect-block.pdf).
Y el **Capítulo 7** (Pin configuration) para las tablas de qué función es cada pin:
[`manual/ch07...`](../../manual/ch07_pin-configuration.pdf).
