/*
 * Demostracion de printf() por RTT: sale por el cable del debugger, sin UART,
 * sin pines y sin conversor USB-serie.
 *
 * Compilar y grabar (desde plantilla/):
 *     cp ../curso/ejemplos/uart/printf_rtt/rtt.[ch] src/
 *     cp ../curso/ejemplos/uart/printf_rtt/main.c   src/
 *     make USE_CMSIS=1 flash
 *
 * Mirar la salida: ver ../MEDICIONES.md seccion 8.
 *
 * Nota: aca USE_CMSIS=1 no hace falta por el baudrate (no hay UART), pero se
 * usa igual para que el core corra a 100 MHz y los ciclos medidos se comparen
 * con los de los otros dos ejemplos.
 */

#include <stdint.h>
#include <stdio.h>

#include "LPC17xx.h"
#include "rtt.h"

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define LED_MASK    (1u << 22)
#define TEXTO       "[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C\n"

int main(void)
{
    rtt_init();
    DEMCR |= (1u << 24); DWT_CYCCNT = 0; DWT_CTRL |= 1u;
    LPC_GPIO0->FIODIR |= LED_MASK;

    printf("\n=== printf por SWD (RTT): sin UART ni pines ===\n");

    uint32_t t0 = DWT_CYCCNT;
    printf(TEXTO);
    uint32_t c = DWT_CYCCNT - t0;

    printf("%u caracteres: %lu ciclos (%lu us)\n",
           (unsigned) (sizeof TEXTO - 1u),
           (unsigned long) c, (unsigned long) (c / 100u));
    printf("Compara con 4091 us por UART/polling y 36 us por UART/DMA.\n\n");

    uint32_t n = 0;
    while (1) {
        printf("tick %lu (descartados %lu)\n",
               (unsigned long) n, (unsigned long) rtt_perdidos());
        LPC_GPIO0->FIOPIN ^= LED_MASK;
        for (volatile int d = 0; d < 2000000; d++) { }
        n++;
    }
}
