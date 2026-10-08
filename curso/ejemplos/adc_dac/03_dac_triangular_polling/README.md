# DAC sin DMA 1: onda triangular por polling

Este ejemplo genera una onda triangular de aproximadamente **100 Hz** en **P0.26/AOUT**. Timer0 fija
el instante de cada actualización y el programa consulta su flag de *match*. No usa demoras por
software, no habilita la interrupción en el NVIC y no usa DMA.

~~~text
Timer0 (cada 100 us) --> flag MR0 --> calcular muestra --> DACR --> P0.26/AOUT
                            CPU consulta y limpia el flag
~~~

Es un primer paso útil porque separa dos ideas: el Timer decide **cuándo** y el valor escrito en el DAC
decide **qué tensión** aparece.

## Qué se practica

- seleccionar P0.26 en función AOUT antes de acceder al DAC;
- ubicar el dato de 10 bits en `DACR[15:6]`;
- relacionar cuentas y tensión con `VOUT = VALUE × VREF / 1024`;
- usar Timer0 como base de tiempo estable;
- construir una triangular sin una tabla de muestras;
- distinguir frecuencia de actualización de frecuencia de la onda.

## Conexión y medición

Conectá una punta de osciloscopio de alta impedancia entre **P0.26/AOUT** y **GND**. No hace falta
inyectar ninguna señal al micro.

> AOUT es una salida analógica de señal: no conectes un LED, parlante, motor ni otra carga pesada de
> forma directa. Tampoco conectes una tensión externa a AOUT. Para manejar una carga se necesita una
> etapa buffer o de potencia.

Un multímetro debería indicar cerca de 1,65 V, el valor medio, pero no permite ver la forma de onda.

## De dónde salen las frecuencias

Con `CCLK = 100 MHz`, el ejemplo selecciona `PCLK_Timer0 = CCLK/4 = 25 MHz`:

~~~text
25 MHz / (PR + 1) = 25 MHz / 25 = 1 MHz
1 MHz / (MR0 + 1) = 1 MHz / 100 = 10 kHz
10 kHz / 100 muestras = 100 Hz
~~~

La salida usa valores de 112 a 912. Si `VREF = 3,3 V`, el rango ideal es:

~~~text
Vmin = 112 × 3,3 / 1024 = 0,36 V
Vmax = 912 × 3,3 / 1024 = 2,94 V
~~~

En el osciloscopio se ven pequeños escalones: el DAC mantiene cada muestra durante 100 us. Eso no es
un error, sino la reconstrucción de orden cero que realiza un DAC.

## Qué mirar en el debugger

| Variable | Valor esperado |
|----------|----------------|
| `dac_ultimo_valor` | sube y baja entre 112 y 912 |
| `muestras_generadas` | aumenta unas 10 000 veces por segundo |

También se puede detener el programa después de `dac_escribir()` y comprobar que
`DACR[15:6] == dac_ultimo_valor`.

## Compilar con la plantilla

Desde la raíz del repositorio:

~~~bash
cp curso/ejemplos/adc_dac/03_dac_triangular_polling/main.c plantilla/src/main.c
cd plantilla
make clean
make USE_CMSIS=1
make USE_CMSIS=1 flash
~~~

`USE_CMSIS=1` hace que `SystemInit()` deje el core a 100 MHz, valor usado en los cálculos del Timer0.

## Para experimentar

1. Cambiá `MUESTRAS_POR_PERIODO` y ajustá `DAC_PASO` para conservar el rango. ¿Qué ocurre con la
   frecuencia de salida si no modificás el Timer?
2. Cambiá `TASA_ACTUALIZACION_HZ` a 5000. Predecí la nueva frecuencia antes de medirla.
3. Reemplazá `triangular_calcular()` por una señal diente de sierra.
4. Medí el tiempo que el CPU pasa esperando el flag. El segundo ejemplo elimina esa espera con una
   interrupción.

Teoría relacionada: [DAC a nivel registro](../../../10_adc_dac/01-adc-dac-registros.md) y
[Timers a nivel registro](../../../08_timers/01-timers-registros.md).
