# ADC a memoria: bloque y ping-pong

El ADC muestrea AD0.0 a 10 ksample/s y el DMA alterna dos buffers de 32 words. Cada interrupción
indica que uno de los buffers quedó completo y puede procesarse mientras el otro se llena.

## Conexión

```text
3,3 V ---- extremo del potenciómetro
             cursor ------------ P0.23 / AD0.0
GND ------ extremo del potenciómetro
```

`ADC_PinConfig(ADC_CHANNEL_0)` configura P0.23 como entrada analógica. La entrada debe permanecer
entre 0 V y 3,3 V.

## Bloque finito

`config_dma_adc_bloque()` guarda una cantidad definida de muestras y luego detiene el canal. `ADGDR`
permanece como origen fijo y el destino avanza por el buffer.

## Ping-pong

`config_dma_adc_ping_pong()` enlaza los buffers en forma circular:

```text
ADC ──→ buffer A ──→ buffer B ──→ buffer A ──→ ...
```

Cada LLI describe un buffer y genera una interrupción al completarlo. Mientras el DMA llena uno, el
programa puede procesar el otro. Si el procesamiento demora demasiado, el DMA volverá a escribir un
buffer antes de que sus datos hayan sido utilizados.

Para probar el bloque finito, reemplazá `config_dma_adc_ping_pong()` por
`config_dma_adc_bloque()`. El arranque no cambia: primero se habilita el canal DMA y después las
conversiones del ADC. La primera interrupción indica que el único buffer quedó completo.

## Decisiones de configuración

El bit `DONE` del ADC genera cada request. Se usa burst 1 porque llega una muestra por conversión, y
el canal 0 tiene prioridad máxima para reducir el riesgo de overrun. Observá `bloques_completos` y
los dos buffers; el resultado se obtiene con `ADC_GDR_RESULT(buffer[i])`.

## Compilar

```bash
cp curso/ejemplos/dma/configs/03_adc_p2m/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
