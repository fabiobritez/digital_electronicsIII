# Nivel intermedio: muestreo uniforme y promedio móvil

Este ejemplo mide un potenciómetro a **100 muestras por segundo**, pero el *while* principal no decide
cuándo convertir. Timer0 genera un evento periódico interno, ese evento dispara el ADC y el final de la
conversión ejecuta *ADC_IRQHandler()*.

~~~text
Timer0 / MAT0.1  -->  AD0.0  -->  ADC_IRQHandler  -->  promedio de 16  -->  LEDs
    período exacto        muestra de 12 bits           muestras             y debugger
~~~

No usa UART, DAC ni DMA. Integra contenidos de GPIO, interrupciones y timers vistos en las unidades
anteriores.

## Qué se practica

- disparar el ADC con el match *MAT0.1*, sin conectar un pin entre periféricos;
- obtener una frecuencia de muestreo conocida;
- atender *ADC_IRQn* y reconocerla leyendo *ADDR0*;
- compartir datos entre una ISR y *main* con *volatile*;
- detectar *OVERRUN*;
- reducir ruido con un promedio móvil, sin usar *float*;
- dormir el CPU con *WFI* mientras trabajan los periféricos.

## Conexiones

Son las mismas que en el [ejemplo básico](../01_voltimetro_leds/): potenciómetro entre 3,3 V y GND, con
el cursor en **P0.23/AD0.0**. P0.22 usa el LED de la placa y los otros tres LEDs se conectan a P0.21,
P0.20 y P0.19 mediante resistencias de 330 Ω.

La señal *MAT0.1* **no sale por un pin**: el Timer0 y el ADC están conectados dentro del LPC1769.

## Cómo se obtienen 100 muestras/s

1. *PCLK_Timer0 = 25 MHz* y *PR = 24*, entonces *TC* avanza cada 1 µs.
2. *MR1 = 4999* produce un match cada 5000 µs, es decir, 200 matches/s.
3. *EMC1 = toggle* hace alternar MAT0.1 en cada match.
4. El ADC reacciona solo al flanco ascendente: hay un flanco ascendente cada dos matches, o **100 por
   segundo**.

Una conversión tarda unos 5,2 µs con el reloj del ADC en 12,5 MHz, mucho menos que los 10 ms entre
muestras. Por eso un nuevo disparo no encuentra al ADC ocupado.

## Qué mirar en el debugger

Agregá estas variables a *Watch* o *Live Expressions*:

| Variable | Significado |
|----------|-------------|
| *adc_muestra_cruda* | Última muestra, con todo su pequeño ruido |
| *adc_promedio* | Promedio móvil de las últimas 16 muestras |
| *adc_milivoltios* | Promedio convertido a 0..3300 mV |
| *adc_conversiones* | Debe crecer aproximadamente 100 veces por segundo |
| *adc_overruns* | Debe permanecer en cero |

Dejá quieto el potenciómetro y compará *adc_muestra_cruda* con *adc_promedio*: la segunda debería variar
menos. Al moverlo rápido también se ve el costo del filtrado: el promedio tarda unas muestras en alcanzar
el valor nuevo.

## Compilar con la plantilla

Desde la raíz del repositorio:

~~~bash
cp curso/ejemplos/adc_dac/02_muestreo_uniforme/main.c plantilla/src/main.c
cd plantilla
make clean
make USE_CMSIS=1
make USE_CMSIS=1 flash
~~~

## Para experimentar

1. Cambiá *MUESTRAS_POR_SEGUNDO* a 10, 50 o 500 y verificá *adc_conversiones*.
2. Cambiá *TAMANIO_PROMEDIO* a 4, 8 o 32. Debe seguir siendo una potencia de dos porque el índice usa una
   máscara en lugar del operador módulo.
3. Quitá temporalmente el promedio de la barra de LEDs y usá *adc_muestra_cruda*. ¿Se nota alguna
   diferencia cerca de un umbral?
4. Calculá el mayor valor válido de *MUESTRAS_POR_SEGUNDO* antes de que el tiempo de conversión sea el
   límite.

Teoría relacionada: [ADC a nivel registro](../../../10_adc_dac/01-adc-dac-registros.md) y
[muestreo, Nyquist y aliasing](../../../10_adc_dac/03-muestreo-nyquist-y-aliasing.md).
