/*
 * rtt.c - printf() por el cable del debugger, sin UART y sin gastar pines.
 *
 * La idea es sorprendentemente simple: el programa escribe en una cola circular
 * en RAM, y el debugger la LEE POR SWD mientras el micro corre, sin frenarlo.
 * No hace falta ningun periferico: el acceso a memoria por SWD lo hace la
 * unidad de debug del Cortex-M3, que funciona en paralelo al CPU.
 *
 * El formato del bloque de control es el de SEGGER RTT, que es el que
 * entienden OpenOCD (rtt setup / rtt server), J-Link y pyOCD. No usamos el
 * codigo de SEGGER: son cuarenta lineas y se entienden enteras.
 *
 * MEDIDO EN PLACA, linea de 48 caracteres:
 *
 *     printf por UART, polling : 4091 us de CPU bloqueado
 *     printf por UART, DMA     :   36 us
 *     printf por RTT           :   24 us     <- este
 *
 * COMO SE MIRA (ver ../MEDICIONES.md seccion 8):
 *
 *     openocd -f openocd/lpc1769.cfg -c "init" -c "reset run" \
 *       -c 'rtt setup 0x10000000 0x8000 "SEGGER RTT"' \
 *       -c "rtt start" -c "rtt server start 9090 0"
 *     nc localhost 9090        (en otra terminal)
 *
 * VENTAJAS sobre la UART: no gasta pines ni periferico, no hay baudrate que
 * calcular, no hace falta conversor USB-serie, y es mas rapido.
 *
 * LIMITACIONES, que son reales:
 *  - Necesita el debugger conectado y OpenOCD corriendo. En un equipo en el
 *    campo no tenes nada.
 *  - Si nadie lee el canal, la cola se llena y se descarta (igual que en la
 *    version por DMA: el hardware no puede inventar ancho de banda).
 *  - El caudal depende de cada cuanto pollea el host, no de un reloj fijo.
 */

#include "rtt.h"

#include <stdio.h>

#define UP_SIZE     1024u
#define DOWN_SIZE   16u

static char up_buf[UP_SIZE];
static char down_buf[DOWN_SIZE];

/* Descriptor de un canal, tal como lo espera el host. Los campos y su orden no
   son negociables: es un contrato binario. */
typedef struct {
    const char *sName;
    char       *pBuffer;
    unsigned    SizeOfBuffer;
    unsigned    WrOff;          /* lo escribe el micro */
    unsigned    RdOff;          /* lo escribe el HOST  */
    unsigned    Flags;          /* 0 = si esta lleno, descarta y sigue */
} canal_t;

typedef struct {
    char    acID[16];
    int     MaxNumUpBuffers;
    int     MaxNumDownBuffers;
    canal_t aUp[1];
    canal_t aDown[1];
} rtt_cb_t;

/* volatile: el host escribe RdOff por atras, sin que el compilador se entere */
static volatile rtt_cb_t cb __attribute__((aligned(4)));

static uint32_t perdidos;
static char     buf_stdout[128];


void rtt_init(void)
{
    cb.aUp[0].sName        = "Terminal";
    cb.aUp[0].pBuffer      = up_buf;
    cb.aUp[0].SizeOfBuffer = UP_SIZE;
    cb.aUp[0].WrOff        = 0u;
    cb.aUp[0].RdOff        = 0u;
    cb.aUp[0].Flags        = 0u;

    cb.aDown[0].sName        = "Terminal";
    cb.aDown[0].pBuffer      = down_buf;
    cb.aDown[0].SizeOfBuffer = DOWN_SIZE;
    cb.aDown[0].WrOff        = 0u;
    cb.aDown[0].RdOff        = 0u;
    cb.aDown[0].Flags        = 0u;

    cb.MaxNumUpBuffers   = 1;
    cb.MaxNumDownBuffers = 1;

    /* El identificador se arma caracter por caracter y AL FINAL, a proposito.
     *
     * Si estuviera como literal ("SEGGER RTT"), el compilador lo dejaria
     * tambien en .rodata, y el host —que encuentra el bloque escaneando la
     * memoria en busca de esa cadena— podria toparse con la copia equivocada.
     * Y escribirlo ultimo evita que el host encuentre un bloque a medio
     * construir si se conecta justo durante el arranque. */
    static const char id[16] = {'S','E','G','G','E','R',' ','R','T','T',
                                0,0,0,0,0,0};
    for (int i = 15; i >= 0; i--) {
        cb.acID[i] = id[i];
    }
    __asm__ volatile ("dsb" ::: "memory");

    /* Buffering de linea, con buffer propio: igual que en la version por DMA,
     * sin buffer newlib llamaria a _write una vez por caracter. Y dandoselo
     * nosotros evitamos que el primer printf pida 1032 bytes al heap. */
    setvbuf(stdout, buf_stdout, _IOLBF, sizeof buf_stdout);
}


/* Pisa el _write weak de plantilla/src/syscalls.c. */
int _write(int fd, const char *buf, int len)
{
    (void) fd;

    unsigned wr = cb.aUp[0].WrOff;

    for (int i = 0; i < len; i++) {
        unsigned siguiente = wr + 1u;
        if (siguiente == UP_SIZE) {
            siguiente = 0u;
        }
        if (siguiente == cb.aUp[0].RdOff) {     /* el host todavia no leyo */
            perdidos++;
            break;
        }
        up_buf[wr] = buf[i];
        wr = siguiente;
    }

    /* Barrera: que los datos esten en memoria ANTES de publicar el indice.
       Si el host viera el WrOff nuevo con el buffer todavia sin escribir,
       leeria basura. */
    __asm__ volatile ("dmb" ::: "memory");
    cb.aUp[0].WrOff = wr;

    return len;
}


uint32_t rtt_perdidos(void) { return perdidos; }
