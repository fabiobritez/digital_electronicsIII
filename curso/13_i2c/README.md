# Módulo 13: I2C (plus)

El **I2C** (Inter-Integrated Circuit, se pronuncia "i-cuadrado-ce") es un bus serial de **dos cables**
que conecta varios chips entre sí: sensores (temperatura, acelerómetros), memorias EEPROM, expansores
de puertos, displays OLED, RTCs externos. Su gracia es que con solo dos líneas, `SDA` (datos) y `SCL`
(reloj), podés colgar **muchos dispositivos** en paralelo, cada uno con una **dirección** distinta.

A diferencia de la UART (punto a punto, asíncrona), el I2C es un bus **síncrono** (tiene cable de
reloj) y **multi-dispositivo**, con un **maestro** (normalmente tu micro) que controla el reloj y se
dirige a cada **esclavo** por su dirección de 7 bits.

El LPC1769 tiene 3 controladores: I2C0, I2C1, I2C2. Capítulo 19 del manual.

## Recorrido

1. [01 - I2C: el protocolo y la máquina de estados](./01-i2c-registros.md)
   El bus a fondo (START/STOP/repeated START, direcciones 7/10 bits, R/W, ACK/NACK, open-drain y
   pull-ups, clock stretching, arbitraje), los registros, **todos los códigos de estado de `I2STAT`**
   (master-TX, master-RX, esclavo), por qué SET y CLEAR son registros separados, y el cálculo de
   velocidad con `I2SCLH`/`I2SCLL`.
2. [02 - I2C con el driver CMSIS](./02-i2c-con-driver.md)
   `I2C_MasterTransferData` para leer y escribir un sensor en pocas líneas, qué hace `I2C_Init` por
   dentro, scanner del bus y recuperación de un bus colgado.
3. [03 - I2C por interrupción y como esclavo](./03-i2c-interrupcion-esclavo.md)
   Atender el bus por interrupción (el `switch (I2STAT)` en la ISR), el LPC como **esclavo**
   (`I2ADR0..3`, `I2MASK`, General Call) y nociones de multi-maestro y modo monitor.

## El ritual de arranque aplicado al I2C
1. **Encender** → `PCONP` (I2C0 = bit 7, I2C1 = 19, I2C2 = 26).
2. **Clockear** → `PCLKSEL` (define la velocidad del bus junto a `I2SCLH/I2SCLL`).
3. **Pines** → `PINSEL` en función I2C, siempre con pull-ups externas. I2C0 (SDA0 = P0.27,
   SCL0 = P0.28) usa pads dedicados que ya son open-drain por hardware; I2C1/I2C2 van en pines
   comunes y ahí sí: **`PINMODE_OD` en open-drain** y sin pulls internas.
4. **Configurar** → velocidad (100 kHz estándar / 400 kHz fast).
5. **Usar** → transacciones maestro: start, dirección, datos, stop.

## Antes de esto
Módulos 3 (clock/power), 4 (PINSEL: clave el open-drain) y 7 (interrupciones, si lo usás por IRQ).

## Manual
Capítulo 19: [`manual/ch19_i2c0-1-2.pdf`](../../manual/ch19_i2c0-1-2.pdf). Ejemplos oficiales en
[`../../library/examples/I2C/`](../../library/examples/I2C/).

---

**Anterior:** [12 - Debug](../12_debug/) · **Siguiente:** [14 - SPI](../14_spi/)
