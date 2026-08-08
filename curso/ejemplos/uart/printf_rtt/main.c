/*
 * Demostracion de la consola por el debugger: printf() y entrada de teclado
 * por el cable SWD, sin UART, sin pines y sin conversor USB-serie.
 *
 * Compilar y grabar (desde plantilla/):
 *     cp ../curso/ejemplos/uart/printf_rtt/rtt.h  src/
 *     cp ../curso/ejemplos/uart/printf_rtt/rtt.c  src/
 *     cp ../curso/ejemplos/uart/printf_rtt/main.c src/
 *     make USE_CMSIS=1 flash
 *     make rtt                 <- levanta el servidor y se conecta
 *
 * Guia completa: curso/12_debug/03-consola-por-el-debugger-rtt.md
 *
 * Nota: USE_CMSIS=1 no hace falta por el baudrate (no hay UART), pero se usa
 * igual para que el core corra a 100 MHz y los ciclos medidos se puedan
 * comparar con los de los otros dos ejemplos.
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

static void delay_lazos(volatile uint32_t lazos)
{
    while (lazos--) { __asm__ volatile ("nop"); }
}

int main(void)
{
    rtt_init();
    DEMCR |= (1u << 24); DWT_CYCCNT = 0; DWT_CTRL |= 1u;
    LPC_GPIO0->FIODIR |= LED_MASK;

    printf("\n=== consola por el debugger (RTT) ===\n");

    uint32_t t0 = DWT_CYCCNT;
    printf(TEXTO);
    uint32_t c = DWT_CYCCNT - t0;

    printf("%u caracteres: %lu ciclos (%lu us de CPU)\n",
           (unsigned) (sizeof TEXTO - 1u),
           (unsigned long) c, (unsigned long) (c / 100u));
    printf("Por UART y polling lo mismo cuesta 4091 us; por UART y DMA, 36 us.\n");
    printf("\nEscribi algo y apreta Enter: el micro te contesta.\n");
    printf("(las teclas viajan por el mismo cable SWD)\n\n");

    uint32_t n = 0;
    uint32_t recibidos = 0;

    while (1) {
        /* Entrada desde la PC. No bloquea: si no hay nada, sigue de largo. */
        int k;
        while ((k = rtt_getchar()) >= 0) {
            recibidos++;
            if (k == '\r' || k == '\n') {
                printf("  <- recibi %lu caracteres\n", (unsigned long) recibidos);
                recibidos = 0;
            } else {
                printf("  <- tecla 0x%02X '%c'\n",
                       (unsigned) k, (k >= 32 && k < 127) ? k : '.');
            }
        }

        printf("tick %lu (descartados %lu, pendientes %lu)\n",
               (unsigned long) n,
               (unsigned long) rtt_perdidos(),
               (unsigned long) rtt_pendientes());

        LPC_GPIO0->FIOPIN ^= LED_MASK;
        delay_lazos(5000000u);
        n++;
    }
}
