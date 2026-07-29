# Módulo 9: UART, comunicación serial

La **UART** es el periférico de comunicación serial más usado: con dos pines (TX y RX) tu micro habla
con una PC (vía un adaptador USB-serial), con un módulo (GPS, Bluetooth, ESP) o con otro micro. Es,
además, la herramienta de depuración #1: mandar texto para ver qué está pasando (`printf` por serial).

El LPC1769 tiene 4 UARTs: UART0, 2 y 3 (básicas, Capítulo 14) y UART1 (con control de módem,
Capítulo 15).

## Recorrido

1. [01 - UART a nivel registro](./01-uart-registros.md)
   El frame en la línea, todos los registros (la trampa del DLAB), el cálculo del baudrate con divisor
   fraccional (con ejemplos numéricos y % de error), las FIFOs, los flags de error de `LSR`, y
   transmitir/recibir por polling (eco serial).
2. [02 - UART con el driver CMSIS](./02-uart-con-driver.md)
   `UART_Init` con baudrate directo, la verdad sobre el caste `(LPC_UART_TypeDef *)`, el trigger de la
   FIFO, y redirigir `printf` a la UART.
3. [03 - UART por interrupción, RS-485 y flow control](./03-uart-interrupcion-y-rs485.md)
   `IER`/`IIR`/`FCR`, prioridad de interrupciones y el Character Time-out, manejo de errores de línea,
   RX/TX por interrupción con ring buffer y por DMA, auto-baud, y los modos de UART1 (flow control
   RTS/CTS y RS-485 multidrop con auto address/auto direction).

## El ritual de arranque aplicado a la UART
1. **Encender** → `PCONP` (UART0 = bit 3, UART1 = bit 4, UART2 = bit 24, UART3 = bit 25).
2. **Clockear** → `PCLKSEL` (el baudrate depende de este PCLK; por reset es CCLK/4).
3. **Pines** → `PINSEL` (UART0: TXD0 = P0.2, RXD0 = P0.3, función 1) y `PINMODE` si querés pull-up/down.
4. **Configurar** → baudrate (`DLL`/`DLM`/`FDR`, ojo con el DLAB) y formato (`LCR`, 8N1).
5. **Usar** → `THR` para transmitir, `RBR` para recibir, `LSR` para el estado; opcional FIFO (`FCR`)
   e interrupciones (`IER`).

## Antes de esto
Módulos 3 (clock/power, define el baudrate) y 7 (interrupciones, para recepción no bloqueante).

## Código listo para probar
En [`../ejemplos/uart/`](../ejemplos/uart/) está el eco serial en sus dos versiones (a registro y
con driver), compilado y listo para cargar.

## Manual
UART0/2/3: Capítulo 14 ([`manual/ch14_uart0-2-3.pdf`](../../manual/ch14_uart0-2-3.pdf)). UART1 (modem
y RS-485): Capítulo 15 ([`manual/ch15_uart1.pdf`](../../manual/ch15_uart1.pdf)). Material original en
[`_origen/`](./_origen/).

---

**Anterior:** [08 - Timers](../08_timers/) · **Siguiente:** [10 - ADC/DAC](../10_adc_dac/)
