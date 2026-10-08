# Memoria a DAC: bloque y anillo

El DAC genera continuamente una onda triangular de 32 muestras. Su timeout interno solicita al DMA
cada nueva word y determina la frecuencia de muestreo.

## Conexión

Conectá un osciloscopio entre **P0.26/AOUT** y GND. No conectes una carga de baja impedancia
directamente al DAC.

## Reproducción única

`config_dma_dac_bloque()` recorre la tabla una vez y se detiene. La dirección de memoria incrementa,
mientras que `DACR` permanece como destino fijo.

## Reproducción continua

`config_dma_dac_anillo()` utiliza una LLI que apunta a sí misma. Al terminar la tabla, el DMA vuelve
a su primera muestra y la señal se repite sin intervención del CPU.

El canal se inicia antes de habilitar las requests del DAC. Así el DMA ya está esperando cuando el
timeout solicita la primera muestra.

## Formato y frecuencia

Las muestras ya están formateadas con `DAC_VALUE()`: el DMA mueve words completas y no transforma
los diez bits del valor. Con un clock periférico de 25 MHz y 781 ticks, la tasa es cercana a 32
ksample/s y la tabla de 32 muestras produce una señal de aproximadamente 1 kHz.

## Compilar

```bash
cp curso/ejemplos/dma/configs/04_dac_m2p/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
