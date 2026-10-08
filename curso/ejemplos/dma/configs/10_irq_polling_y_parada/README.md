# IRQ, polling, pausa y parada

El programa ejecuta tres demostraciones M2M en orden: una transferencia finita atendida por
interrupción, otra transferencia finita observada por polling y un anillo usado para practicar el
control del canal.

## Interrupción y polling

`config_dma_m2m_interrupcion()` habilita el bit `I` del bloque y la máscara `ITC` del canal.
`DMA_IRQHandler()` revisa TC y error de los ocho canales y limpia cada estado atendido.

`config_dma_m2m_polling()` no habilita interrupciones. `esperar_dma_por_polling()` consulta si el
canal sigue habilitado y revisa `RAW_INTERR`, porque el estado raw puede leerse aunque la
interrupción de error esté enmascarada. El polling sólo se usa con esta transferencia finita.

## Pausa y reanudación

`pausar_y_reanudar_dma()` activa `Halt`, que impide iniciar transferencias nuevas, conserva las
direcciones y el contador actuales, y luego continúa desde ese mismo punto. `Pause` no espera a que
el FIFO quede vacío ni equivale a comenzar nuevamente.

## Parada

`detener_dma_sin_perder_datos()` bloquea requests nuevas, espera que se vacíe el FIFO y deshabilita
el canal. `GPDMA_ChannelStop()` es inmediato y puede descartar datos pendientes.

## Transferencia circular

`config_dma_m2m_anillo()` enlaza la LLI consigo misma para mantener el canal activo. No genera TC en
cada vuelta: un anillo M2M se repite muy rápido y esas interrupciones impedirían que el programa
hiciera trabajo útil. Los errores sí permanecen habilitados.

Después de una parada limpia hay que ejecutar otra vez la función de configuración antes de
reiniciar. Colocá breakpoints en la ISR y en cada operación para observar `Active`, `Halt` y `Enable`.

## Compilar

```bash
cp curso/ejemplos/dma/configs/10_irq_polling_y_parada/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
