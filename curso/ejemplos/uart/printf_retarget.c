/*
 * printf() redirigido a UART0 (115200 8N1, TXD0 = P0.2, RXD0 = P0.3).
 *
 * Muestra el retargeting de la libreria estandar por el gancho __io_putchar():
 * a partir de ahi, printf(), puts(), putchar() y fwrite() salen por el cable
 * serie. El programa imprime un encabezado con el clock y los divisores que
 * quedaron configurados, despues un contador, y hace eco de lo que le escribas.
 *
 * Explicado paso a paso en: curso/00_lenguaje_c/16-redirigir-printf-a-uart.md
 * La UART a registro, en:   curso/09_uart/01-uart-registros.md
 *
 * ---------------------------------------------------------------------------
 * COMO COMPILARLO
 * ---------------------------------------------------------------------------
 * Con la plantilla del repo (probado asi):
 *
 *     cp curso/ejemplos/uart/printf_retarget.c plantilla/src/main.c
 *     cd plantilla && make USE_CMSIS=1 flash
 *
 * El USE_CMSIS=1 NO es opcional. Trae el SystemInit() de CMSIS, que engancha el
 * cristal de 12 MHz y la PLL y deja CCLK = 100 MHz; como PCLKSEL0 queda en 0,
 * PCLK_UART0 = CCLK/4 = 25 MHz, que es el numero con el que estan calculados
 * los divisores de uart0_init(). Sin el, el micro corre a 4 MHz (RC interno),
 * PCLK_UART0 = 1 MHz, el baudrate maximo posible es 62500 y 115200 no se puede
 * generar de ninguna manera: solo verias basura en la terminal.
 *
 * En MCUXpresso: importalo como fuente del proyecto. El proyecto generado ya
 * trae un _write() que llama a __io_putchar(); si el tuyo no lo tiene, pone
 * PROVEER_WRITE en 1 aca abajo.
 */

#include <stdint.h>
#include <stdio.h>

#include "LPC17xx.h"

/* 0 = tu proyecto ya tiene un _write() que llama a __io_putchar()
 *     (es el caso de plantilla/src/syscalls.c y de MCUXpresso)
 * 1 = no lo tiene, y lo define este archivo */
#define PROVEER_WRITE   0


/* --- Bits que usamos, con nombre ----------------------------------------- */
#define LCR_WLS_8BIT    (0x3u << 0)   /* 8 bits de datos */
#define LCR_DLAB        (1u << 7)     /* acceso a DLL/DLM */

#define FCR_FIFO_EN     (1u << 0)
#define FCR_RX_RESET    (1u << 1)
#define FCR_TX_RESET    (1u << 2)

#define TER_TXEN        (1u << 7)

#define LSR_RDR         (1u << 0)     /* hay byte recibido */
#define LSR_THRE        (1u << 5)     /* el THR quedo libre */

#define LED_MASK        (1u << 22)    /* LED de a bordo de la LPCXpresso */


/*
 * uart0_init - 115200 8N1, polling.
 *
 * Con PCLK_UART0 = 25 MHz el divisor entero solo no alcanza:
 *
 *     DL = 25e6 / (16 x 115200) = 13.57   -> 13 da +4.3%, 14 da -3.1%
 *
 * Los dos errores estan fuera de la tolerancia (~2%) y se ven como framing
 * errors. Por eso se usa el divisor fraccional:
 *
 *     DL = 10, MULVAL = 14, DIVADDVAL = 5
 *     baud = 25e6 / (16 x 10 x (1 + 5/14)) = 115131.6   -> error -0.06%
 *
 * Es la misma cuenta que UART_Init() del driver hace por barrido; aca esta
 * resuelta a mano para que se vea de donde sale cada numero.
 */
static void uart0_init(void)
{
    /* 1) Alimentar UART0 (PCONP bit 3). Viene encendida por reset, pero
     *    depender de un valor por defecto es como no configurarlo. */
    LPC_SC->PCONP |= (1u << 3);

    /* 2) PCLK_UART0: bits 7:6 de PCLKSEL0. 00 = CCLK/4 = 25 MHz. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);

    /* 3) PINSEL0: P0.2 = TXD0 y P0.3 = RXD0, los dos con la funcion 01.
     *    A P0.2 le tocan los bits 5:4 y a P0.3 los 7:6. */
    LPC_PINCON->PINSEL0 &= ~((0x3u << 4) | (0x3u << 6));
    LPC_PINCON->PINSEL0 |=  ((0x1u << 4) | (0x1u << 6));

    /* 4) PINMODE de P0.3 (RXD0) sin pull-down: la linea en reposo es un 1, y
     *    tirarla para abajo genera falsos bits de start. 10 = ni pull-up ni
     *    pull-down; el reset deja 00 (pull-up), que tambien sirve. */
    LPC_PINCON->PINMODE0 &= ~(0x3u << 6);
    LPC_PINCON->PINMODE0 |=  (0x2u << 6);

    /* 5) Formato 8N1 y abrir el acceso a los divisores. */
    LPC_UART0->LCR = LCR_WLS_8BIT | LCR_DLAB;

    /* 6) Los divisores calculados arriba. */
    LPC_UART0->DLM = 0;
    LPC_UART0->DLL = 10;
    LPC_UART0->FDR = (14u << 4) | (5u << 0);   /* MULVAL=14, DIVADDVAL=5 */

    /* 7) Bajar DLAB. Este es EL paso que todo el mundo se olvida: con DLAB en
     *    1, escribir "THR" escribe en realidad DLL y no sale ni un byte. */
    LPC_UART0->LCR = LCR_WLS_8BIT;

    /* 8) FIFOs habilitadas y vacias. */
    LPC_UART0->FCR = FCR_FIFO_EN | FCR_RX_RESET | FCR_TX_RESET;

    /* 9) Transmisor habilitado (viene asi por reset; explicito por las dudas). */
    LPC_UART0->TER = TER_TXEN;
}


static void uart0_send_byte(uint8_t c)
{
    while (!(LPC_UART0->LSR & LSR_THRE)) { }   /* esperar THRE */
    LPC_UART0->THR = c;
}


/*
 * __io_putchar - el gancho que engancha printf() a la UART.
 *
 * En plantilla/src/syscalls.c hay una version weak que tira los caracteres a la
 * basura, y un _write() que la llama byte por byte. Al definirla aca (fuerte),
 * el linker se queda con esta y todo printf/puts/putchar sale por P0.2.
 *
 * La traduccion de LF a CRLF es para las terminales serie: con solo '\n' el
 * cursor baja pero no vuelve al margen, y el texto sale en escalera.
 */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart0_send_byte('\r');
    }
    uart0_send_byte((uint8_t) ch);
    return ch;
}

#if PROVEER_WRITE
int _write(int fd, const char *buf, int len)
{
    (void) fd;
    for (int i = 0; i < len; i++) {
        __io_putchar(buf[i]);
    }
    return len;
}
#endif


static void delay_lazos(volatile uint32_t lazos)
{
    while (lazos--) {
        __asm__ volatile ("nop");
    }
}


int main(void)
{
    uart0_init();

    /* Sin buffer: cada byte sale en el momento. Si el programa se cuelga
     * despues de un printf, con buffering el mensaje nunca llega a la terminal
     * y uno termina buscando el bug en el lugar equivocado. */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* El LED sirve de senal de vida independiente de la UART: si parpadea pero
     * no ves texto, el problema esta en el cable o en la terminal, no en el
     * firmware. */
    LPC_GPIO0->FIODIR |= LED_MASK;

    printf("\n");
    printf("=========================================\n");
    printf("  LPC1769 - printf() por UART0\n");
    printf("  115200 8N1, TXD0 = P0.2, RXD0 = P0.3\n");
    printf("=========================================\n");
    printf("SystemCoreClock = %lu Hz\n", (unsigned long) SystemCoreClock);
    printf("PCLK_UART0      = %lu Hz\n", (unsigned long) (SystemCoreClock / 4u));

    /* Leer los divisores es un buen recordatorio de la trampa del DLAB: con
     * DLAB=0 esas dos direcciones NO son DLL/DLM, son RBR e IER. Para verlos de
     * verdad hay que levantar DLAB y bajarlo enseguida. */
    printf("con DLAB=0 leo: 0x%02X 0x%02X  (no son DLL/DLM: son RBR e IER)\n",
           (unsigned) LPC_UART0->DLL,
           (unsigned) LPC_UART0->DLM);

    LPC_UART0->LCR |= LCR_DLAB;
    unsigned dll = (unsigned) LPC_UART0->DLL;
    unsigned dlm = (unsigned) LPC_UART0->DLM;
    LPC_UART0->LCR &= ~LCR_DLAB;

    printf("con DLAB=1 leo: DLL=%u DLM=%u FDR=0x%02X (MULVAL=%u DIVADDVAL=%u)\n",
           dll, dlm,
           (unsigned) LPC_UART0->FDR,
           (unsigned) ((LPC_UART0->FDR >> 4) & 0xFu),
           (unsigned) (LPC_UART0->FDR & 0xFu));

    printf("\nEscribi algo en la terminal y te lo devuelvo.\n\n");

    uint32_t n = 0;

    while (1) {
        /* Eco de lo que llegue por RXD0, para probar el otro sentido. */
        while (LPC_UART0->LSR & LSR_RDR) {
            uint8_t c = (uint8_t) LPC_UART0->RBR;
            printf("[rx] 0x%02X '%c'\n", c, (c >= 32 && c < 127) ? c : '.');
        }

        printf("tick %lu\n", (unsigned long) n);

        LPC_GPIO0->FIOPIN ^= LED_MASK;
        delay_lazos(2500000u);      /* ~0.5 s con el core a 100 MHz */
        n++;
    }
}
