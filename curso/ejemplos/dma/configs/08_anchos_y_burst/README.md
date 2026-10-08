# Anchos distintos y burst

El DMA lee 16 bytes consecutivos y los escribe como cuatro words de 32 bits. Así se puede observar
cómo se calcula el tamaño cuando origen y destino usan anchos diferentes.

## Configuración

`config_dma_m2m_bytes_a_words()` usa:

- origen byte, burst 32 e incremento habilitado;
- destino word, burst 8 e incremento habilitado;
- `TransferSize = cantidad_bytes / 4`, porque el contador representa transferencias del destino.

Los dos bursts transportan la misma cantidad de información: 32 bytes de origen equivalen a 8 words
de destino. El DMA agrupa cuatro bytes para formar cada word; no realiza una conversión numérica.

## Resultado esperado

En la LPC1769, que usa formato little-endian, `destino_demo[0]` queda en `0x03020100`. Verificá las
cuatro words desde el debugger. Las direcciones deben estar alineadas con el ancho configurado.

## Compilar

```bash
cp curso/ejemplos/dma/configs/08_anchos_y_burst/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
