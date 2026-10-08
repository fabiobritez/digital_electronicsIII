# Requests DMA generadas por software

Una escritura en `DMACSoftSReq` genera la request que enciende el LED de P0.22. Esto permite probar
el canal paso a paso sin esperar un evento de Timer0.

## Conexión

P0.22 controla el LED de la LPCXpresso. También puede conectarse un LED externo con resistencia.

## Single request

`config_dma_gpio_request_software()` deja el canal esperando. Una llamada a
`disparar_dma_gpio_request_software()` mueve una word al GPIO y enciende el LED. El origen queda fijo
porque solo se utiliza un valor. La escritura usa `FIOPIN`, por lo que actualiza el puerto GPIO0
completo y no solamente el bit del LED.

## Burst request

`config_dma_gpio_burst_software()` configura cuatro transferencias y un origen incremental. Una
llamada a `disparar_dma_gpio_burst_software()` permite consumir las cuatro words del burst.

## Request sin periférico activo

La línea de MAT0.0 identifica la request, pero Timer0 permanece detenido: la señal se genera por
software. No deben activarse al mismo tiempo el timer y la request manual sobre la misma línea.

Los registros `last request` del controlador no tienen una fuente compatible en los periféricos del
LPC1769 y por eso no se utilizan.

## Compilar

```bash
cp curso/ejemplos/dma/configs/09_request_software/main.c plantilla/src/main.c
make -C plantilla USE_CMSIS=1
```
