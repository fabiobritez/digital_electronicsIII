# DAC sin DMA 2: seno por tabla e interrupción

Este ejemplo genera una onda senoidal de aproximadamente **250 Hz** en **P0.26/AOUT**. Timer0 produce
una interrupción cada 80 us y la ISR escribe en el DAC la siguiente entrada de una tabla de 50
muestras. El `while` principal duerme con `WFI`; no hay DMA.

~~~text
Timer0, 12,5 kHz --> TIMER0_IRQHandler --> tabla[índice] --> DACR --> AOUT
       despierta al CPU          índice 0..49
~~~

Es la continuación del [ejemplo triangular](../03_dac_triangular_polling/): conserva el Timer y el
acceso al DAC, pero reemplaza el cálculo de la triangular por una tabla y el *polling* por una ISR.

## Qué se practica

- representar una forma de onda con una tabla de muestras;
- configurar una interrupción periódica de Timer0;
- reconocer la interrupción escribiendo en `T0IR`;
- compartir variables con una ISR usando `volatile`;
- relacionar frecuencia de muestreo, cantidad de muestras y frecuencia de salida;
- sacar el cálculo en punto flotante fuera del micro;
- usar `WFI` cuando el programa principal no tiene trabajo.

## Conexión y medición

Conectá una punta de osciloscopio de alta impedancia entre **P0.26/AOUT** y **GND**. No conectes una
fuente externa, un LED, un parlante ni una carga pesada directamente a AOUT. Un multímetro solo
mostrará el valor medio, cercano a 1,65 V.

## Cuentas de frecuencia y tensión

Timer0 queda con un tick de 1 us y genera un match cada 80 ticks:

~~~text
f_actualización = 1 MHz / 80 = 12 500 muestras/s
f_seno = 12 500 / 50 muestras por período = 250 Hz
~~~

La tabla contiene valores enteros calculados previamente con:

~~~text
tabla[n] = redondear(512 + 400 × sin(2πn/50))
~~~

No se evalúa `sin()` durante la ejecución. El offset de 512 centra la onda alrededor de la mitad de
la referencia y la amplitud de 400 deja el rango entre 112 y 912, aproximadamente 0,36 V a 2,94 V si
`VREF = 3,3 V`.

## Qué debería verse

El osciloscopio debería indicar unos 250 Hz, cerca de 2,58 V pico a pico y un nivel medio cercano a
1,65 V. Al ampliar la base de tiempo aparecen los escalones de 80 us que forman el seno.

En el debugger:

| Variable | Valor esperado |
|----------|----------------|
| `dac_indice` | recorre continuamente 0..49 |
| `dac_ultimo_valor` | sigue los valores de `tabla_seno` |
| `muestras_generadas` | aumenta unas 12 500 veces por segundo |

## Compilar con la plantilla

Desde la raíz del repositorio:

~~~bash
cp curso/ejemplos/adc_dac/04_dac_seno_interrupcion/main.c plantilla/src/main.c
cd plantilla
make clean
make USE_CMSIS=1
make USE_CMSIS=1 flash
~~~

## Para experimentar

1. Duplicá `MR0 + 1`. ¿Por qué la onda pasa de 250 Hz a 125 Hz sin cambiar la tabla?
2. Modificá el offset 512 o la amplitud 400 al generar la tabla. Verificá siempre que cada muestra
   permanezca entre 0 y 1023.
3. Reemplazá la tabla por 25 muestras y conservá 12,5 kHz. Predecí la frecuencia y compará la forma.
4. Agregá en `main` un contador u otra tarea corta. Comprobá que la ISR mantiene la señal periódica.
5. Medí cuánto tarda `TIMER0_IRQHandler`. ¿Hasta qué tasa de actualización sería razonable hacerlo con
   interrupciones antes de justificar DMA?

Teoría relacionada: [DAC a nivel registro](../../../10_adc_dac/01-adc-dac-registros.md),
[interrupciones](../../../07_interrupciones/01-nvic-y-vectores.md) y
[Timers a nivel registro](../../../08_timers/01-timers-registros.md).
