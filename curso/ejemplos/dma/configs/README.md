# Catálogo de configuraciones DMA (LPC1769)

Estos ejemplos muestran cómo cambia una configuración DMA según el origen, el destino y la señal
que marca el ritmo. Conviene recorrerlos en orden y, antes de ejecutar cada uno, predecir qué
direcciones incrementan, qué request inicia cada transferencia y cuándo termina el canal.

Cada carpeta incluye las conexiones necesarias cuando corresponden, una configuración lista para
ejecutar y variantes para comparar. Las funciones `config_dma_*` permiten identificar con facilidad
qué parte del programa prepara el DMA.

## Recorrido sugerido

| Ejemplo | Flujo | Sin LLI | Con LLI | Qué se aprende |
|---|---|---|---|---|
| [`01_m2m/`](./01_m2m/) | M2M | sí | cadena finita | copia, fill, scatter/gather, prioridad baja |
| [`02_gpio_y_timer/`](./02_gpio_y_timer/) | M2P/P2M | sí | anillo | GPIO no genera requests; MATx.y da el ritmo |
| [`03_adc_p2m/`](./03_adc_p2m/) | P2M | sí | ping-pong | `ADGDR` fijo, buffer incremental, `ADINTEN`, overrun |
| [`04_dac_m2p/`](./04_dac_m2p/) | M2P | sí | anillo | `DACR` fijo, words preformateadas, timeout y doble buffer |
| [`05_uart/`](./05_uart/) | M2P/P2M | sí | no | las cuatro UART, FIFO DMA, dos canales para full-duplex |
| [`06_uart_rx_ping_pong/`](./06_uart_rx_ping_pong/) | P2M | no | anillo | recepción continua en dos buffers |
| [`07_p2p_uart/`](./07_p2p_uart/) | P2P | sí | anillo | requests en ambos extremos y formatos compatibles |
| [`08_anchos_y_burst/`](./08_anchos_y_burst/) | M2M | sí | no | packing byte→word y unidades de `TransferSize` |
| [`09_request_software/`](./09_request_software/) | M2P | sí | no | single/burst request manual; requests `last` no soportadas |
| [`10_irq_polling_y_parada/`](./10_irq_polling_y_parada/) | todos | — | — | IRQ, polling, pause/resume y parada limpia |


## Matriz de posibilidades útiles

| Origen    | Destino  | Tipo | Request que marca el ritmo | Condición principal                                                 |
| --------- | -------- | ---- | -------------------------- | ------------------------------------------------------------------- |
| RAM/Flash | RAM      | M2M  | ninguna                    | válido; usar canal 7 o prioridad baja                               |
| RAM       | GPIO     | M2P  | MATx.y o software          | válido; el timer es request, GPIO es la dirección destino           |
| GPIO      | RAM      | P2M  | MATx.y o software          | válido; muestreo periódico de un puerto                             |
| ADC       | RAM      | P2M  | ADC `DONE`                 | válido; simple o ping-pong                                          |
| RAM       | DAC      | M2P  | timeout interno del DAC    | válido; simple o anillo                                             |
| RAM       | UARTx Tx | M2P  | FIFO Tx                    | válido para UART0, 1, 2 y 3                                         |
| UARTx Rx  | RAM      | P2M  | FIFO Rx                    | válido para UART0, 1, 2 y 3                                         |
| UART Rx   | UART Tx  | P2P  | ambos FIFO                 | válido si coinciden ancho y ritmo                                   |
| ADC       | DAC      | P2P  | ADC y DAC                  | **no convierte formatos**; `ADGDR` no es una word lista para `DACR` |


No toda combinación P2P es válida. El periférico de origen debe poder producir datos, el de destino
debe poder consumirlos y ambos deben usar formatos y velocidades compatibles.

## Antes de habilitar un canal

1. Identificá quién genera cada request: M2M no necesita; M2P/P2M necesita una; P2P necesita dos.
2. Verificá `DMAREQSEL` cuando se usan las líneas compartidas 8 a 15.
3. Habilitá la request en el periférico además de habilitar el GPDMA.
4. Calculá `TransferSize` en transferencias del bus de destino y respetá el rango 1 a 4095.
5. Alineá las direcciones y elegí el ancho correspondiente al registro o FIFO.
6. Incrementá solamente los extremos que recorren memoria; los registros de periférico quedan fijos.
7. Elegí un burst que el FIFO pueda sostener y una prioridad que proteja las entradas con riesgo de
   pérdida.
8. Decidí si la transferencia termina, continúa con otra LLI o forma un anillo, y en qué punto debe
   interrumpir.
9. Mantené buffers y LLI en memoria mientras el DMA pueda utilizarlos y definí un orden de arranque
   que no pierda la primera request.

En los extremos que corresponden a memoria, `srcConn` o `dstConn` quedan ignorados. Los ejemplos
usan `0` en esos campos porque el driver exige igualmente un valor dentro del rango válido. Ese
cero coincide con la primera conexión del enum; no significa “sin conexión” y sólo es seguro cuando
el tipo de transferencia garantiza que ese extremo está en memoria.


## Cómo compilar un ejemplo

```bash
cp curso/ejemplos/dma/configs/03_adc_p2m/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```

Cada ejemplo llama `GPDMA_Init()` una sola vez. No debe repetirse al agregar un segundo canal porque
resetea los ocho canales y borraría configuraciones concurrentes.

## Interrupciones

Una ISR común debe revisar TC y error por canal, empezando por el canal de mayor prioridad, y limpiar
los dos estados que correspondan. El driver usa `intTC` para colocar el bit `I` en el bloque inicial
y habilitar la máscara `ITC` del canal. Cada LLI tiene su propio bit `I`; cuando sólo debe interrumpir
una LLI posterior, se configura el bloque inicial sin TC y luego se habilita `ITC` directamente en
`DMACCConfig`. En un anillo, poner `I` en cada descriptor produce una IRQ por bloque, no una IRQ de
“fin del anillo” (el anillo nunca termina).

## Para consultar

- UM10360, capítulo 31: conexiones (tabla 544), registros de canal (tablas 564–566), programación,
flow control y scatter/gather.
- UM10360, capítulos 14, 21, 29 y 30: UART, timers, ADC y DAC.
- `library/CMSISv2p00_LPC17xx/UPSTREAM.md`: origen, commit y ajustes locales del driver modernizado.
