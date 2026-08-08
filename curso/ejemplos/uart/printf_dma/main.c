/*
 * Demostracion y medicion de dbg_uart: printf() por DMA contra printf() por
 * polling, las dos cosas en el mismo binario y con el mismo texto, para que la
 * comparacion sea honesta.
 *
 * Se mide con el contador de ciclos del Cortex-M3 (DWT->CYCCNT). A 100 MHz,
 * 1 ciclo = 10 ns.
 *
 * Compilar y grabar (desde plantilla/):
 *     cp ../curso/ejemplos/uart/printf_dma/dbg_uart.[ch] src/
 *     cp ../curso/ejemplos/uart/printf_dma/main.c        src/
 *     make USE_CMSIS=1 flash
 */

#include <stdint.h>
#include <stdio.h>

#include "LPC17xx.h"
#include "dbg_uart.h"

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define LSR_THRE    (1u << 5)
#define LSR_TEMT    (1u << 6)
#define LED_MASK    (1u << 22)

volatile uint32_t n = 1234567;


/* La version por polling, para tener con que comparar dentro del mismo
   programa: escribe cada byte al THR esperando a que se libere. */
static void enviar_bloqueante(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            while (!(LPC_UART0->LSR & LSR_THRE)) { }
            LPC_UART0->THR = '\r';
        }
        while (!(LPC_UART0->LSR & LSR_THRE)) { }
        LPC_UART0->THR = (uint8_t) *s++;
    }
}

static void drenar(void)
{
    while (!(LPC_UART0->LSR & LSR_TEMT)) { }
}

static void delay_lazos(volatile uint32_t lazos)
{
    while (lazos--) { __asm__ volatile ("nop"); }
}


/* Una linea de depuracion de largo realista (54 caracteres). Con textos muy
   cortos la FIFO de 16 bytes de la UART se traga casi todo y la version por
   polling parece mas rapida de lo que es. */
#define TEXTO   "[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C\n"

int main(void)
{
    dbg_uart_init();            /* configura tambien el buffering de stdout */
    DEMCR |= (1u << 24); DWT_CYCCNT = 0; DWT_CTRL |= 1u;

    LPC_GPIO0->FIODIR |= LED_MASK;

    printf("\n=== printf por DMA contra printf por polling ===\n");
    printf("linea de prueba: %u caracteres\n", (unsigned) (sizeof TEXTO - 1u));
    dbg_uart_flush();

    uint32_t t0, c_dma, c_bloq;

    /* --- printf por DMA: se mide lo que tarda en VOLVER, que es lo que le
     *     cuesta al programa. Los bytes siguen saliendo despues, solos. */
    drenar();
    t0 = DWT_CYCCNT;
    printf(TEXTO);
    c_dma = DWT_CYCCNT - t0;

    dbg_uart_flush();

    /* --- El mismo texto, por polling. Aca "volver" y "terminar de transmitir"
     *     son casi lo mismo: por eso bloquea. */
    drenar();
    t0 = DWT_CYCCNT;
    enviar_bloqueante(TEXTO);
    c_bloq = DWT_CYCCNT - t0;

    printf("\nciclos que le cuesta al CPU (10 ns cada uno):\n");
    printf("  printf por DMA      : %6lu  (%lu us)\n",
           (unsigned long) c_dma, (unsigned long) (c_dma / 100u));
    printf("  printf por polling  : %6lu  (%lu us)\n",
           (unsigned long) c_bloq, (unsigned long) (c_bloq / 100u));
    if (c_dma > 0u) {
        printf("  el CPU trabaja %lu veces menos\n",
               (unsigned long) (c_bloq / c_dma));
    }
    dbg_uart_flush();

    /* --- Lo que el DMA NO arregla: la linea sigue siendo de 115200.
     *     Imprimimos de golpe mucho mas de lo que entra en la cola y miramos
     *     cuanto se descarta. */
    printf("\nprueba de saturacion: 200 lineas de golpe...\n");
    dbg_uart_flush();

    uint32_t antes = dbg_uart_perdidos();
    t0 = DWT_CYCCNT;
    for (uint32_t i = 0; i < 200u; i++) {
        printf("linea de relleno numero %lu\n", (unsigned long) i);
    }
    uint32_t c_rafaga = DWT_CYCCNT - t0;
    uint32_t pendientes = dbg_uart_pendientes();
    uint32_t descartados = dbg_uart_perdidos() - antes;

    dbg_uart_flush();
    printf("\n  las 200 llamadas tardaron %lu ciclos (%lu us)\n",
           (unsigned long) c_rafaga, (unsigned long) (c_rafaga / 100u));
    printf("  quedaban %lu bytes en la cola al terminar\n",
           (unsigned long) pendientes);
    printf("  se descartaron %lu bytes por cola llena\n",
           (unsigned long) descartados);
    printf("\n  Eso es lo esperado: el DMA saca al CPU del camino, pero no\n");
    printf("  acelera el cable. Si imprimis mas rapido de lo que la linea\n");
    printf("  transmite, o descartas o volves a bloquear. Aca se descarta.\n");
    dbg_uart_flush();

    printf("\nAhora imprime un tick por segundo sin bloquear nunca.\n\n");

    uint32_t k = 0;
    while (1) {
        printf("tick %lu  (perdidos hasta ahora: %lu)\n",
               (unsigned long) k, (unsigned long) dbg_uart_perdidos());
        LPC_GPIO0->FIOPIN ^= LED_MASK;
        delay_lazos(5000000u);
        k++;
    }
}
