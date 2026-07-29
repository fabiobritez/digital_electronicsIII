# Módulo 3: Clock y Power

> **El paso que todos se olvidan.** Antes de configurar *cualquier* periférico (timer, UART, ADC,
> SPI…), hay que hacer dos cosas que no son obvias y que, si faltan, hacen que "el código se vea bien
> pero no pase nada":
>
> 1. **Darle energía** al periférico → registro `PCONP`.
> 2. **Darle un reloj** (clock) → registros `PCLKSEL`.
>
> Un periférico sin energía o sin clock está **muerto**: escribís sus registros y no responde. Este
> módulo te enseña a "encenderlo" y por qué su frecuencia de reloj afecta todos los cálculos que
> vienen después (períodos de timer, baudrate de UART, etc.).

## Por qué este módulo va antes que los periféricos

En el LPC1769, para ahorrar consumo, **varios periféricos arrancan apagados** tras el reset. El CPU
funciona, el GPIO viene encendido por defecto, pero el ADC, los Timers 2/3, las UART2/3, el DMA o el
USB están sin clock hasta que vos prendés su bit en `PCONP`. Esta es una de las causas #1 de "no me
anda y no sé por qué" en la materia.

Además, cada periférico recibe su reloj derivado del reloj del CPU (`CCLK`), y por defecto ese reloj
viene **dividido por 4**. Saber a qué frecuencia corre tu periférico es imprescindible para calcular
tiempos.

## Recorrido

1. [01 - Power: encender periféricos con PCONP](./01-power-pconp.md)
   El registro `PCONP` bit por bit, qué arranca encendido de verdad (el valor de reset y lo que pisa
   `SystemInit`), clock gating y el patrón "siempre prendelo primero".
2. [02 - Clock de periféricos: PCLKSEL](./02-clock-pclksel.md)
   Cómo cada periférico recibe su reloj, el mapa completo de `PCLKSEL0/1`, los divisores (/4 por
   defecto, /1, /2, /8, y la rareza /6 del CAN) y por qué cambiar el `PCLK` te rompe los baudrates.
3. [03 - El árbol de clock y la PLL0 (cómo llegamos a 100 MHz)](./03-arbol-de-clock-y-pll.md)
   Los osciladores (IRC/main/RTC), `SCS`, `CLKSRCSEL`, la PLL0 (`Fcco = 2·M·Fin/N`, rango 275–550 MHz),
   `CCLKCFG`, y la **secuencia exacta de feed** (`0xAA`/`0x55`) con `PLOCK`. (Más avanzado.)
4. [04 - El flash accelerator y los wait states](./04-flash-accelerator.md)
   `FLASHCFG`/`FLASHTIM`: por qué a 100 MHz necesitás 5 ciclos de acceso y qué se cuelga si lo
   configurás mal. (Más avanzado.)
5. [05 - Clock del USB (PLL1) y CLKOUT](./05-usb-clock-y-clkout.md)
   Los 48 MHz del USB (PLL1 o `USBCLKCFG`) y cómo sacar un reloj interno por un pin para depurarlo.
   (Más avanzado / opcional.)
6. [06 - Modos de bajo consumo](./06-modos-de-bajo-consumo.md)
   La otra cara de "Power": dormir el micro para ahorrar batería (`__WFI()`, Sleep/Deep Sleep/Power-down).

## Requisito
[Módulo 1](../01_arquitectura_y_acceso_a_registros/) (saber escribir registros) y manejar máscaras
de bits del [módulo 0, cap. 02](../00_lenguaje_c/02-operadores.md).

## Manual de referencia
- **Capítulo 4** (clock y power): [`manual/ch04_clocking-and-power-control.pdf`](../../manual/ch04_clocking-and-power-control.pdf).
- **Capítulo 5** (flash accelerator, para `FLASHCFG`): [`manual/ch05_flash-accelerator.pdf`](../../manual/ch05_flash-accelerator.pdf).
- **Capítulo 3** (system control, parte del PLL y `SCS`): [`manual/ch03_system-control.pdf`](../../manual/ch03_system-control.pdf).
