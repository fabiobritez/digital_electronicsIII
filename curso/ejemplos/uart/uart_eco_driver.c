/*
 * UART0 con el driver CMSIS: eco serial (115200 baud, 8N1).
 *
 * Mismo comportamiento que uart_eco_registros.c, pero dejando que
 * lpc17xx_uart calcule el divisor de baudrate (incluido el fraccional,
 * imprescindible a 115200 con PCLK de 25 MHz).
 *
 * Explicado en: curso/09_uart/02-uart-con-driver.md
 */

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void uart0_init(void) {
    // Pines: el driver NO configura PINSEL, lo hacés vos.
    PINSEL_CFG_Type pin;
    pin.Funcnum   = 1;                      // función 1 = TXD0/RXD0
    pin.OpenDrain = 0;
    pin.Pinmode   = 0;                      // pull-up por defecto
    pin.Portnum   = 0;
    pin.Pinnum    = 2; PINSEL_ConfigPin(&pin);   // P0.2 = TXD0
    pin.Pinnum    = 3; PINSEL_ConfigPin(&pin);   // P0.3 = RXD0

    // Configuración de la UART
    UART_CFG_Type cfg;
    UART_ConfigStructInit(&cfg);            // defaults: 9600, 8 bits, sin paridad, 1 stop
    cfg.Baud_rate = 115200;
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &cfg);   // PCONP + LCR + DLL/DLM/FDR

    // FIFOs: config por defecto (FIFO ON, trigger 1 char, resetea RX y TX)
    UART_FIFO_CFG_Type fifo;
    UART_FIFOConfigStructInit(&fifo);
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifo);

    UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);   // habilitar transmisor (TER)
}

int main(void) {
    uart0_init();

    uint8_t msg[] = "UART lista (driver CMSIS). Escribi algo:\r\n";
    UART_Send((LPC_UART_TypeDef *)LPC_UART0, msg, sizeof(msg) - 1, BLOCKING);

    while (1) {
        uint8_t c;
        // NONE_BLOCKING: devuelve 0 si no llegó nada; el CPU queda libre
        // para hacer otra cosa entre byte y byte.
        if (UART_Receive((LPC_UART_TypeDef *)LPC_UART0, &c, 1, NONE_BLOCKING)) {
            UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, c);   // eco
        }
    }
    return 0;
}
