# GPIO periódico usando requests de timer

El LED de P0.22 cambia de estado cada 250 ms. MAT0.0 genera una request interna por período y el DMA
escribe el nuevo valor en el GPIO; no hace falta llevar la señal MAT0.0 a un pin.

## Conexiones

- P0.22: LED de la LPCXpresso o LED externo con resistencia.
- P0.10: entrada opcional para probar `config_dma_gpio_entrada_periodica()`; puede conectarse un
  pulsador a GND porque se configura pull-up interno.

## Salida periódica

`config_dma_gpio_salida_periodica()` recorre una tabla de estados y escribe cada word en `FIOPIN`.
El origen incrementa, el registro GPIO queda fijo y MAT0.0 solicita una transferencia cada 250 ms.
La LLI apunta a sí misma para repetir la tabla indefinidamente. Como `FIOPIN` representa el puerto
completo, la tabla también define el estado de cualquier otro pin de GPIO0 configurado como salida.

## Muestreo periódico

`config_dma_gpio_entrada_periodica()` realiza el recorrido inverso: lee siempre `FIOPIN` y avanza
por `muestras_gpio`. MAT0.1 marca el instante de cada lectura y la transferencia termina al llenar
el buffer.

## Request y dirección

Este ejemplo separa dos conceptos: MAT0.x indica cuándo transferir y `FIOPIN` indica dónde leer o
escribir. Por eso el canal se configura directamente en sus registros: la request del timer no
obliga a transferir desde o hacia el registro `MRx`.

Compará ambas variantes y observá qué extremo incrementa en cada dirección.

## Compilar

```bash
cp curso/ejemplos/dma/configs/02_gpio_y_timer/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
