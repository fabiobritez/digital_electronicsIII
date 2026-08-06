# Ejemplos de UART

Dos versiones del mismo programa (eco serial: todo lo que llega se devuelve), siguiendo la
progresión del curso:

| Archivo | Nivel | Baudrate |
|---------|-------|----------|
| [`uart_eco_registros.c`](./uart_eco_registros.c) | A registro (DLAB, DLL/DLM, LSR, polling) | 9600 |
| [`uart_eco_driver.c`](./uart_eco_driver.c) | Driver CMSIS (`UART_Init`, `UART_Send/Receive`) | 115200 |

Para probarlos: conectá la placa por el puente UART-USB (o el adaptador que tengas en
P0.2/P0.3), abrí una terminal serie con el baudrate correcto y escribí: cada tecla debería
volver como eco.

Teoría y explicación paso a paso: [módulo 9: UART](../../09_uart/).

> ¿Por qué el de registro usa 9600 y el de driver 115200? Porque a 115200 con PCLK de 25 MHz
> el divisor entero no alcanza (error > 3%) y hace falta el divisor fraccional, que el driver
> calcula solo. Está explicado en la [página 1 del módulo 9](../../09_uart/01-uart-registros.md).
