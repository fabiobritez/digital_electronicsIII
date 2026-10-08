# Nivel básico: voltímetro visual con ADC

Este ejemplo lee un potenciómetro con **AD0.0 (P0.23)** y representa su tensión con una barra de cuatro
LEDs. No usa UART, DAC ni drivers: el objetivo es seguir el recorrido completo de una conversión por
software mirando los registros.

## Qué se practica

- habilitar el ADC con *PCONP*;
- seleccionar la función analógica y el modo tri-state del pin;
- calcular *CLKDIV* para no superar los 13 MHz;
- iniciar una conversión con *START = 001* y esperar *DONE*;
- extraer los 12 bits de *ADDR0* y convertir cuentas a milivoltios;
- usar GPIO para mostrar un resultado sin una terminal serie.

## Conexiones

Usá un potenciómetro de **10 kΩ** (un valor cercano también sirve):

~~~text
3,3 V ----- extremo del potenciómetro
                 cursor ------------- P0.23 / AD0.0
GND ------- extremo del potenciómetro
~~~

P0.22 controla el LED de la LPCXpresso. Para completar la barra, conectá tres LEDs externos:

~~~text
P0.21 ---- resistencia de 330 Ω ----|>|---- GND
P0.20 ---- resistencia de 330 Ω ----|>|---- GND
P0.19 ---- resistencia de 330 Ω ----|>|---- GND
~~~

> La entrada ADC debe permanecer entre **0 V y 3,3 V**. No conectes 5 V a P0.23.

## Qué debería ocurrir

Al girar el potenciómetro se encienden progresivamente los LEDs. Los escalones están aproximadamente en
0,66 V, 1,32 V, 1,98 V y 2,64 V. En el debugger también se pueden agregar estas variables a *Watch* o
*Live Expressions*:

- *adc_cuentas*: resultado crudo, de 0 a 4095;
- *adc_milivoltios*: tensión aproximada, de 0 a 3300 mV.

El valor en milivoltios supone una referencia ideal de 3,3 V. Compararlo con un multímetro permite ver el
efecto de la tolerancia de la referencia y del potenciómetro.

## Compilar con la plantilla

Desde la raíz del repositorio:

~~~bash
cp curso/ejemplos/adc_dac/01_voltimetro_leds/main.c plantilla/src/main.c
cd plantilla
make clean
make USE_CMSIS=1
make USE_CMSIS=1 flash
~~~

*USE_CMSIS=1* hace que *SystemInit()* configure el core a 100 MHz. El código fija *PCLK_ADC* en 25 MHz y
usa *CLKDIV = 1*, por lo que el reloj de conversión queda en 12,5 MHz.

## Para experimentar

1. Cambiá los cuatro umbrales para dividir el rango en cuartos en vez de quintos.
2. Reemplazá 3300 por la tensión de referencia medida con un multímetro y compará el resultado.
3. Detené el programa después de *adc_leer()* y localizá *DONE* y *RESULT* dentro de *ADDR0*.

Teoría relacionada: [ADC y DAC a nivel registro](../../../10_adc_dac/01-adc-dac-registros.md).
