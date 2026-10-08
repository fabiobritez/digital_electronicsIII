# IRQ, polling, pausa y parada

Una transferencia M2M circular mantiene el canal activo y permite comparar distintas formas de
observarlo, pausarlo y detenerlo.

## Interrupción y polling

`DMA_IRQHandler()` revisa TC y error de los ocho canales. En cambio,
`esperar_dma_por_polling()` consulta el estado hasta que termina un bloque, sin habilitar el NVIC.

## Pausa y reanudación

`pausar_y_reanudar_dma()` activa `Halt`, conserva las direcciones y el contador actuales, y luego
continúa desde ese mismo punto. Pausar no equivale a comenzar nuevamente.

## Parada

`detener_dma_sin_perder_datos()` bloquea requests nuevas, espera que se vacíe el FIFO y deshabilita
el canal. `GPDMA_ChannelStop()` es inmediato y puede descartar datos pendientes.

## Transferencia circular

`config_dma_m2m_anillo()` enlaza la LLI consigo misma para mantener el canal activo. Cada vuelta
genera TC, pero no existe un “fin del anillo”.

Después de una parada limpia hay que ejecutar otra vez la función de configuración antes de
reiniciar. Colocá breakpoints en la ISR y en cada operación para observar `Active`, `Halt` y `Enable`.

## Compilar

```bash
cp curso/ejemplos/dma/configs/10_irq_polling_y_parada/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
