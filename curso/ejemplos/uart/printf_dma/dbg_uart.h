/*
 * dbg_uart - salida de depuracion por UART0 que NO bloquea al CPU.
 *
 * printf() deja los bytes en una cola circular y vuelve enseguida; el GPDMA
 * los va sacando por la UART solo, sin intervencion del procesador.
 *
 * Contra la version por polling (curso/ejemplos/uart/printf_retarget.c), donde
 * el CPU se queda esperando el THRE bit a bit:
 *
 *     printf("x1234567\n")  con polling : ~874 us de CPU bloqueado
 *     printf("x1234567\n")  con DMA     : ~2 us  (el resto lo hace el DMA)
 *
 * Explicado en: curso/ejemplos/uart/printf_dma/README.md
 *
 * LO QUE ESTO NO ARREGLA: el cable sigue siendo de 115200 baud. El DMA saca al
 * CPU del camino, pero no acelera la linea. Si imprimis mas rapido de lo que la
 * UART puede transmitir, la cola se llena; esta implementacion DESCARTA los
 * bytes que no entran y los cuenta en dbg_uart_perdidos(). Es una decision
 * deliberada: en depuracion es preferible perder texto a frenar el programa que
 * estas tratando de observar.
 */
#ifndef DBG_UART_H
#define DBG_UART_H

#include <stdint.h>

/* Inicializa UART0 (115200 8N1, P0.2/P0.3) en modo DMA y el canal 0 del GPDMA.
   Requiere CCLK = 100 MHz, o sea SystemInit(): compilar con USE_CMSIS=1. */
void dbg_uart_init(void);

/* Bloquea hasta que la cola quede vacia y el ultimo bit haya salido del shift
   register. Usalo antes de un reset, un breakpoint o dentro del HardFault: sin
   esto, lo ultimo que imprimiste todavia esta en la cola y se pierde. */
void dbg_uart_flush(void);

/* Bytes descartados por cola llena desde el arranque. Si esto crece, estas
   imprimiendo mas rapido de lo que la linea puede sacar. */
uint32_t dbg_uart_perdidos(void);

/* Cuantos bytes hay esperando en la cola ahora mismo. */
uint32_t dbg_uart_pendientes(void);

#endif /* DBG_UART_H */
