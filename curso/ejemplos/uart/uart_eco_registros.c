/*
 * UART0 a registro: eco serial por polling (9600 baud, 8N1).
 *
 * Todo lo que llega por RXD0 (P0.3) se devuelve por TXD0 (P0.2).
 * Probalo con una terminal serie (PuTTY, minicom, screen) a 9600 8N1.
 *
 * Explicado paso a paso en: curso/09_uart/01-uart-registros.md
 * Supone PCLK_UART0 = 25 MHz (CCLK 100 MHz / 4, valores por defecto).
 */

#include "LPC17xx.h"

void uart0_init(void) {
    // 1) Encender UART0 (PCONP bit 3). Por reset ya viene encendida, pero
    //    hacerlo explícito es buena costumbre.
    LPC_SC->PCONP |= (1u << 3);

    // 2) (Clock) Dejamos PCLK_UART0 en CCLK/4 (valor por reset) = 25 MHz.

    // 3) PINSEL: P0.2 = TXD0, P0.3 = RXD0 (función 1)
    LPC_PINCON->PINSEL0 &= ~((0x3u << 4) | (0x3u << 6));
    LPC_PINCON->PINSEL0 |=  ((0x1u << 4) | (0x1u << 6));

    // 4) Formato 8N1 y abrir acceso a los divisores (DLAB=1)
    LPC_UART0->LCR = (0x3u << 0)    // WLS = 8 bits
                   | (1u << 7);     // DLAB = 1

    // 5) Baudrate 9600 con PCLK = 25 MHz -> DL = 163, sin fraccional
    LPC_UART0->DLM = 0;
    LPC_UART0->DLL = 163;
    LPC_UART0->FDR = (1u << 4);     // MULVAL=1, DIVADDVAL=0 -> fraccional neutro

    // 6) Cerrar el acceso a divisores (DLAB=0) para poder usar RBR/THR
    LPC_UART0->LCR = (0x3u << 0);   // 8N1, DLAB=0  <-- ¡NO te olvides de este paso!

    // 7) Habilitar y limpiar las FIFOs (bit0=enable, bit1=reset RX, bit2=reset TX)
    LPC_UART0->FCR = (1u << 0) | (1u << 1) | (1u << 2);

    // 8) Habilitar el transmisor (TER bit 7). Viene en 1 por reset.
    LPC_UART0->TER = (1u << 7);
}

void uart0_send_byte(uint8_t c) {
    while (!(LPC_UART0->LSR & (1u << 5))) { }   // esperar THRE
    LPC_UART0->THR = c;
}

void uart0_send_string(const char *s) {
    while (*s) uart0_send_byte((uint8_t)*s++);
}

uint8_t uart0_read_byte(void) {
    while (!(LPC_UART0->LSR & (1u << 0))) { }   // esperar RDR
    return LPC_UART0->RBR;
}

int main(void) {
    uart0_init();
    uart0_send_string("UART lista. Escribi algo:\r\n");
    while (1) {
        uint8_t c = uart0_read_byte();   // bloquea hasta que llegue un byte
        uart0_send_byte(c);              // lo devuelve (eco)
    }
    return 0;
}
