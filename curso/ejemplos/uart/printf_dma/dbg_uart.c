/*
 * dbg_uart.c - printf() por UART0 sin bloquear al CPU, usando el GPDMA.
 *
 * La idea en tres piezas:
 *
 *   printf()  ->  _write()  ->  cola circular  ->  GPDMA  ->  THR  ->  P0.2
 *                 (productor)                     (consumidor)
 *
 * _write() copia bytes a la cola y vuelve. El GPDMA los saca por su cuenta,
 * pidiendo uno cada vez que la FIFO de transmision de la UART deja de estar
 * llena. Cuando el DMA termina un tramo, su interrupcion arranca el siguiente.
 * El CPU solo trabaja en el memcpy y en una ISR cortita cada tramo.
 *
 * Referencias del UM10360: capitulo 14 (UART, seccion 14.4.6.1 "DMA
 * Operation") y capitulo 31 (GPDMA).
 */

#include "dbg_uart.h"

#include <stdio.h>
#include "LPC17xx.h"

/* ---------------------------------------------------------------------------
 * La cola
 * ---------------------------------------------------------------------------
 * Tamano potencia de dos: asi el "dar la vuelta" es un AND y no un modulo (que
 * en el Cortex-M3 es una division por software, carisima).
 *
 * Vive en el banco 0 de AHB SRAM, no en la RAM principal. En el LPC176x el
 * GPDMA alcanza las dos (lo dice el UM10360 seccion 1.9: los 32 kB de SRAM
 * local son "accessible by the CPU and all three DMA controllers"), asi que no
 * es una obligacion sino una decision de rendimiento: la AHB SRAM esta en un
 * puerto esclavo separado de la matriz AHB, de modo que el DMA leyendo la cola
 * y el CPU trabajando en la RAM principal no se pisan. Son 16 kB que de otro
 * modo quedarian sin usar.
 * ------------------------------------------------------------------------ */
#define BUF_BITS   11u
#define BUF_SIZE   (1u << BUF_BITS)          /* 2048 bytes */
#define BUF_MASK   (BUF_SIZE - 1u)

#define DMA_MAX    4095u                     /* el campo TransferSize es de 12 bits */
#define CANAL      0u                        /* canal 0 del GPDMA */

static uint8_t cola[BUF_SIZE] __attribute__((section(".ahbram0"), aligned(4)));

static volatile uint32_t cabeza;             /* proximo byte a escribir (lo mueve _write) */
static volatile uint32_t lectura;            /* proximo byte a mandar   (lo mueve la ISR) */
static volatile uint32_t en_vuelo;           /* bytes del tramo que el DMA esta sacando */
static volatile uint32_t perdidos;           /* descartados por cola llena */

/* Bits de los registros del canal, con nombre (UM10360 tabla 553 y 554) */
#define CTRL_SBSIZE_1    (0u << 12)
#define CTRL_DBSIZE_1    (0u << 15)
#define CTRL_SWIDTH_BYTE (0u << 18)
#define CTRL_DWIDTH_BYTE (0u << 21)
#define CTRL_SI          (1u << 26)          /* incrementar la direccion fuente */
#define CTRL_I           (1u << 31)          /* interrumpir al terminar el tramo */

#define CFG_E            (1u << 0)           /* habilitar el canal */
#define CFG_DEST_UART0TX (8u << 6)           /* periferico destino: UART0 Tx */
#define CFG_M2P          (1u << 11)          /* memoria -> periferico */
#define CFG_IE           (1u << 14)          /* mascara de interrupcion por error */
#define CFG_ITC          (1u << 15)          /* mascara de interrupcion por fin de cuenta */


/* ---------------------------------------------------------------------------
 * arrancar_dma - si hay datos y el DMA esta libre, largar el proximo tramo.
 * ---------------------------------------------------------------------------
 * Seccion critica corta porque la llaman los dos lados: _write desde el hilo
 * principal y la ISR del DMA. Sin ella, una interrupcion que imprima en el
 * momento justo podria largar dos transferencias sobre el mismo canal.
 *
 * Un tramo nunca cruza el final del arreglo: el DMA incrementa direcciones
 * linealmente y no sabe nada de colas circulares. Cuando los datos dan la
 * vuelta, se mandan en dos tramos y del segundo se encarga la ISR.
 * ------------------------------------------------------------------------ */
static void arrancar_dma(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (en_vuelo == 0u && cabeza != lectura) {
        uint32_t n = (cabeza > lectura) ? (cabeza - lectura)     /* tramo contiguo */
                                        : (BUF_SIZE - lectura);  /* hasta el final */
        if (n > DMA_MAX) {
            n = DMA_MAX;
        }
        en_vuelo = n;

        LPC_GPDMACH0->DMACCSrcAddr  = (uint32_t) &cola[lectura];
        LPC_GPDMACH0->DMACCDestAddr = (uint32_t) &LPC_UART0->THR;
        LPC_GPDMACH0->DMACCLLI      = 0u;
        LPC_GPDMACH0->DMACCControl  = n
                                    | CTRL_SBSIZE_1 | CTRL_DBSIZE_1
                                    | CTRL_SWIDTH_BYTE | CTRL_DWIDTH_BYTE
                                    | CTRL_SI | CTRL_I;
        LPC_GPDMACH0->DMACCConfig   = CFG_E | CFG_DEST_UART0TX | CFG_M2P
                                    | CFG_IE | CFG_ITC;
    }

    __set_PRIMASK(primask);
}


/* ---------------------------------------------------------------------------
 * La ISR: un tramo termino, avanzar la cola y largar el siguiente.
 * ---------------------------------------------------------------------------
 * Corta a proposito. Es lo unico que el CPU gasta por cada tramo, sin importar
 * si el tramo eran 10 bytes o 2000.
 * ------------------------------------------------------------------------ */
void DMA_IRQHandler(void)
{
    if (LPC_GPDMA->DMACIntTCStat & (1u << CANAL)) {
        LPC_GPDMA->DMACIntTCClear = (1u << CANAL);
        lectura  = (lectura + en_vuelo) & BUF_MASK;
        en_vuelo = 0u;
        arrancar_dma();
    }

    if (LPC_GPDMA->DMACIntErrStat & (1u << CANAL)) {
        LPC_GPDMA->DMACIntErrClr = (1u << CANAL);
        en_vuelo = 0u;              /* soltar el canal para no quedar trabados */
        arrancar_dma();
    }
}


/* ---------------------------------------------------------------------------
 * Encolar un byte. Si no entra, se descarta y se cuenta.
 * ---------------------------------------------------------------------------
 * Lee "lectura" sin seccion critica y esta bien: la ISR solo la hace crecer, o
 * sea que solo puede LIBERAR lugar. Leer un valor viejo es conservador (como
 * mucho descarta un byte que hubiera entrado), nunca corrompe. Y en Cortex-M3
 * la lectura de un uint32_t alineado es atomica.
 * ------------------------------------------------------------------------ */
static void encolar(uint8_t c)
{
    /* Sin chequeo: _write() ya verifico que entra el mensaje entero. Se deja
       siempre un byte sin usar para poder distinguir "llena" de "vacia". */
    cola[cabeza] = c;
    cabeza = (cabeza + 1u) & BUF_MASK;
}


static uint32_t espacio_libre(void)
{
    return (BUF_SIZE - 1u) - ((cabeza - lectura) & BUF_MASK);
}


/* ---------------------------------------------------------------------------
 * _write - aca desemboca printf().
 * ---------------------------------------------------------------------------
 * Pisa el _write weak de plantilla/src/syscalls.c. Copia y vuelve: nunca espera
 * a la UART.
 * ------------------------------------------------------------------------ */
int _write(int fd, const char *buf, int len)
{
    (void) fd;

    /* Cuanto lugar necesita: cada '\n' se convierte en dos bytes (CR LF). */
    uint32_t necesita = (uint32_t) len;
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            necesita++;
        }
    }

    /* O ENTRA TODO EL MENSAJE O NO ENTRA NADA. Nunca se escribe la mitad.
     *
     * Si al llenarse la cola se recortara el mensaje, en la terminal
     * apareceria una linea incompleta que PARECE valida:
     *
     *     adc=2048 temp=25.4 C        <- buena
     *     adc=20                      <- recortada, y no hay como saberlo
     *
     * Depurando, eso es peor que perder la linea entera: te manda a buscar un
     * bug que no existe. Asi la garantia es fuerte y simple: TODO LO QUE VES
     * ESTA COMPLETO Y EN ORDEN. Lo que no entro queda contado en
     * dbg_uart_perdidos().
     *
     * Como stdout esta en buffering de linea, cada llamada trae una linea
     * entera: "mensaje" y "linea" son lo mismo. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    int entra = (espacio_libre() >= necesita);
    if (entra) {
        for (int i = 0; i < len; i++) {
            if (buf[i] == '\n') {
                encolar((uint8_t) '\r');   /* las terminales quieren CRLF */
            }
            encolar((uint8_t) buf[i]);
        }
    } else {
        perdidos += necesita;              /* el mensaje entero, no un pedazo */
    }
    __set_PRIMASK(primask);

    if (entra) {
        arrancar_dma();
    }
    return len;
}


/* ------------------------------------------------------------------------ */
void dbg_uart_init(void)
{
    /* --- UART0: 115200 8N1 con PCLK = 25 MHz -------------------------------
     * Mismos divisores que el ejemplo por polling: DL=10, MULVAL=14,
     * DIVADDVAL=5 -> 115131.6 baud, error -0.06%. */
    LPC_SC->PCONP |= (1u << 3);
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);

    LPC_PINCON->PINSEL0 &= ~((0x3u << 4) | (0x3u << 6));
    LPC_PINCON->PINSEL0 |=  ((0x1u << 4) | (0x1u << 6));

    LPC_UART0->LCR = 0x03u | (1u << 7);         /* 8N1, DLAB=1 */
    LPC_UART0->DLM = 0u;
    LPC_UART0->DLL = 10u;
    LPC_UART0->FDR = (14u << 4) | 5u;
    LPC_UART0->LCR = 0x03u;                     /* DLAB=0 */

    /* FIFOs encendidas y DMA Mode Select (FCR bit 3). El bit de DMA solo tiene
     * efecto con las FIFOs habilitadas: lo dice el UM10360 en 14.4.6.1. Con
     * esto, la UART pide un byte al DMA cada vez que su FIFO de transmision
     * deja de estar llena. */
    LPC_UART0->FCR = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3);
    LPC_UART0->TER = (1u << 7);

    /* --- GPDMA ------------------------------------------------------------ */
    LPC_SC->PCONP |= (1u << 29);                /* PCGPDMA */
    LPC_GPDMA->DMACConfig = 1u;                 /* habilitado, little endian */
    while (!(LPC_GPDMA->DMACConfig & 1u)) {
    }
    LPC_GPDMA->DMACIntTCClear = 0xFFu;          /* arrancar sin banderas viejas */
    LPC_GPDMA->DMACIntErrClr  = 0xFFu;
    LPC_GPDMACH0->DMACCConfig = 0u;             /* canal 0 apagado hasta que haya datos */

    cabeza = lectura = en_vuelo = perdidos = 0u;

    NVIC_EnableIRQ(DMA_IRQn);

    /* --- Buffering: acá el criterio se DA VUELTA respecto del polling -------
     *
     * Con la salida por polling conviene setvbuf(_IONBF): sin buffer, cada byte
     * sale al instante y si el programa se cuelga ya viste el ultimo mensaje.
     *
     * Con DMA eso es contraproducente. Sin buffer, newlib llama a _write() UNA
     * VEZ POR CARACTER, y cada llamada arranca una transferencia de 1 byte con
     * su interrupcion de fin: diez caracteres son diez ISRs. Medido en placa,
     * eso convierte un printf de 20 us en uno de 68 us.
     *
     * Con buffering de linea, newlib acumula hasta el '\n' y entrega la linea
     * entera de un saque: una transferencia, una ISR. Y no perdemos la garantia
     * de "lo ultimo que ves es lo ultimo que paso", porque la cola se vacia
     * sola por DMA; para el caso extremo (un cuelgue) esta dbg_uart_flush().
     *
     * El buffer lo damos nosotros para que newlib no lo pida al heap: si le
     * pasas NULL, el primer printf reserva 1032 bytes con malloc. */
    static char buf_stdout[128];
    setvbuf(stdout, buf_stdout, _IOLBF, sizeof buf_stdout);
}


void dbg_uart_flush(void)
{
    while (cabeza != lectura || en_vuelo != 0u) {
        arrancar_dma();                 /* por si quedo parado */
    }
    while (!(LPC_UART0->LSR & (1u << 6))) {     /* TEMT: el shift register tambien */
    }
}


uint32_t dbg_uart_perdidos(void)   { return perdidos; }

uint32_t dbg_uart_pendientes(void) { return (cabeza - lectura) & BUF_MASK; }
