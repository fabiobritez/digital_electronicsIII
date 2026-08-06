/*
 * Voltimetro serial: ADC + UART con drivers CMSIS.
 *
 * Lee AD0.0 (P0.23) y manda la tension por UART0 (115200 8N1) una vez
 * por segundo, en milivoltios, sin usar float (el Cortex-M3 no tiene
 * FPU; ver curso/00_lenguaje_c/15-punto-fijo-vs-flotante.md).
 *
 * Es el ejercicio 1 del modulo 10, resuelto: integra ADC (modulo 10),
 * UART (modulo 9) y SysTick (modulo 6).
 */

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

static volatile uint32_t millis = 0;

void SysTick_Handler(void) {
    millis++;
}

static void adc_init(void) {
    // Pin AD0.0 = P0.23, función 1, tri-state (el driver no toca PINSEL)
    PINSEL_CFG_Type pin;
    pin.Portnum = 0; pin.Pinnum = 23; pin.Funcnum = 1;
    pin.Pinmode = PINSEL_PINMODE_TRISTATE; pin.OpenDrain = 0;
    PINSEL_ConfigPin(&pin);

    // 190000 y no 200000: la división entera del driver puede dejar
    // f_ADC > 13 MHz si pedís la tasa máxima justa (ver módulo 10, pág. 2).
    ADC_Init(LPC_ADC, 190000);
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
}

static uint16_t adc_leer(void) {
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);
    while (!(ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE))) { }
    return ADC_ChannelGetData(LPC_ADC, 0);     // 0..4095
}

static void uart_init(void) {
    PINSEL_CFG_Type pin;
    pin.Funcnum = 1; pin.OpenDrain = 0; pin.Pinmode = 0; pin.Portnum = 0;
    pin.Pinnum = 2; PINSEL_ConfigPin(&pin);    // P0.2 = TXD0
    pin.Pinnum = 3; PINSEL_ConfigPin(&pin);    // P0.3 = RXD0

    UART_CFG_Type cfg;
    UART_ConfigStructInit(&cfg);
    cfg.Baud_rate = 115200;
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &cfg);
    UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);
}

static void uart_print(const char *s) {
    while (*s) {
        UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)*s++);
    }
}

// Convierte un entero 0..99999 a texto. Sin printf: alcanza y no
// arrastra ~6 KB de biblioteca (ver curso/00_lenguaje_c/16-redirigir-printf-a-uart.md).
static void uart_print_u32(uint32_t v) {
    char buf[6];
    int i = 0;
    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && i < 5);
    while (i) {
        UART_SendByte((LPC_UART_TypeDef *)LPC_UART0, (uint8_t)buf[--i]);
    }
}

int main(void) {
    adc_init();
    uart_init();
    SysTick_Config(SystemCoreClock / 1000);   // tick de 1 ms

    uart_print("Voltimetro ADC (canal AD0.0 = P0.23)\r\n");

    uint32_t t_prev = 0;
    while (1) {
        if (millis - t_prev >= 1000) {        // cada 1 s, sin bloquear
            t_prev = millis;

            uint16_t cuentas = adc_leer();
            // 12 bits sobre 3.3 V: mV = cuentas * 3300 / 4096
            uint32_t mv = ((uint32_t)cuentas * 3300u) >> 12;

            uart_print_u32(mv / 1000);        // parte entera (V)
            uart_print(".");
            uint32_t frac = mv % 1000;        // milésimas, con ceros a la izquierda
            if (frac < 100) uart_print("0");
            if (frac < 10)  uart_print("0");
            uart_print_u32(frac);
            uart_print(" V\r\n");
        }
        // acá entrarían otras tareas del superloop (módulo 17)
    }
    return 0;
}
