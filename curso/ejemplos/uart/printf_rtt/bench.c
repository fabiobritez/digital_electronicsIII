/*
 * bench.c - Banco de pruebas de rendimiento de la consola RTT.
 *
 * Mide dos cosas que son distintas y se confunden todo el tiempo:
 *
 *   1. Lo que le CUESTA AL PROGRAMA imprimir (ciclos de CPU por printf).
 *      Es lo que importa si tenes plazos que cumplir.
 *   2. El CAUDAL que el enlace sostiene (bytes/s que llegan a la PC).
 *      Es lo que importa si queres volcar muchos datos.
 *
 * Los dos numeros no tienen nada que ver entre si: el primero lo pone el
 * Cortex-M3, el segundo lo pone cada cuanto el host pollea por SWD.
 *
 * COMO USARLO
 * -----------
 *     cd plantilla
 *     cp ../curso/ejemplos/uart/printf_rtt/rtt.[ch]  src/
 *     cp ../curso/ejemplos/uart/printf_rtt/bench.c   src/main.c
 *     make USE_CMSIS=1 flash
 *     make rtt
 *
 * Al arrancar corre solo la parte 1. Despues acepta teclas:
 *
 *     1  repetir el costo de CPU
 *     2  medir caudal sostenido (sin perder nada: se autolimita)
 *     3  rafaga a fondo (mide cuanto se descarta)
 *     0  quedarse quieto
 *
 * Del lado de la PC, medir_rtt.py automatiza todo esto.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "LPC17xx.h"
#include "rtt.h"

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define LED_MASK    (1u << 22)
#define CICLOS_US   100u            /* core a 100 MHz -> 100 ciclos = 1 us */

/* _write lo provee rtt.c; lo llamamos directo para medir la cola sin la libc */
extern int _write(int fd, const char *buf, int len);

static char linea[300];


static void encabezado(void)
{
    printf("\n");
    printf("===========================================================\n");
    printf("  Banco de pruebas de la consola RTT - LPC1769 a 100 MHz\n");
    printf("  1 ciclo = 10 ns\n");
    printf("===========================================================\n");
}


/* ---------------------------------------------------------------------------
 * PARTE 1: cuanto le cuesta al CPU una impresion
 * ---------------------------------------------------------------------------
 * Se mide tres veces el mismo texto por tres caminos, para separar el costo de
 * formatear del costo de encolar:
 *
 *   _write directo : solo la cola (memcpy + indices). El piso.
 *   printf literal : + el recorrido de la libc, sin interpretar nada
 *   printf %lu     : + formatear un entero
 * ------------------------------------------------------------------------ */
static void medir_costo_cpu(void)
{
    static const unsigned largos[] = { 8u, 16u, 32u, 64u, 128u, 256u };

    printf("\n--- 1) Costo de CPU por impresion ---\n\n");
    printf("largo | _write directo |  printf literal |  ciclos/byte\n");
    printf("------+----------------+-----------------+-------------\n");

    for (unsigned i = 0; i < sizeof largos / sizeof largos[0]; i++) {
        unsigned n = largos[i];

        /* Relleno reconocible: estas lineas SALEN por la consola (las
           estamos imprimiendo de verdad), asi que conviene que se vean como
           ruido y no como parte del informe. */
        memset(linea, '.', n);
        linea[0] = '#';
        linea[n - 1u] = '\n';
        linea[n]      = '\0';

        /* Esperar lugar en la cola para que la medicion no incluya un
           descarte, que seria mas rapido y mentiria a favor. */
        while (rtt_pendientes() > 256u) { }
        uint32_t t0 = DWT_CYCCNT;
        _write(1, linea, (int) n);
        uint32_t c_raw = DWT_CYCCNT - t0;

        while (rtt_pendientes() > 256u) { }
        t0 = DWT_CYCCNT;
        printf(linea);
        uint32_t c_printf = DWT_CYCCNT - t0;

        while (rtt_pendientes() > 256u) { }
        printf("%5u | %8lu ciclos | %9lu ciclos | %6lu\n",
               n, (unsigned long) c_raw, (unsigned long) c_printf,
               (unsigned long) (c_printf / n));
    }

    /* Costo de formatear, aislado */
    printf("\nFormateo (mismo destino, distinto trabajo de la libc):\n");

    while (rtt_pendientes() > 256u) { }
    uint32_t t0 = DWT_CYCCNT;
    printf("texto fijo sin ningun especificador de formato\n");
    uint32_t c_lit = DWT_CYCCNT - t0;

    while (rtt_pendientes() > 256u) { }
    t0 = DWT_CYCCNT;
    printf("un entero: %lu y otro: %lu\n", 1234567UL, 89UL);
    uint32_t c_int = DWT_CYCCNT - t0;

    while (rtt_pendientes() > 256u) { }
    t0 = DWT_CYCCNT;
    printf("hexa %08lX y cadena %s\n", 0xDEADBEEFUL, "hola");
    uint32_t c_hex = DWT_CYCCNT - t0;

    while (rtt_pendientes() > 256u) { }
    printf("\n  literal, sin %%      : %5lu ciclos (%lu us)\n",
           (unsigned long) c_lit, (unsigned long) (c_lit / CICLOS_US));
    printf("  con dos %%lu        : %5lu ciclos (%lu us)\n",
           (unsigned long) c_int, (unsigned long) (c_int / CICLOS_US));
    printf("  con %%08lX y %%s     : %5lu ciclos (%lu us)\n",
           (unsigned long) c_hex, (unsigned long) (c_hex / CICLOS_US));
    printf("\n  Referencia: la misma linea por UART a 115200 y polling\n");
    printf("  cuesta 409095 ciclos (4091 us).\n");
}


/* ---------------------------------------------------------------------------
 * PARTE 2: caudal sostenido, sin perder un byte
 * ---------------------------------------------------------------------------
 * Antes de cada linea espera a que haya lugar de sobra en la cola. Asi el
 * micro produce exactamente al ritmo al que el host consume, y el caudal que
 * mida la PC es el caudal REAL del enlace, no el de la RAM.
 * ------------------------------------------------------------------------ */
static void medir_caudal(uint32_t lineas)
{
    printf("\n--- 2) Caudal sostenido (%lu lineas, sin perdidas) ---\n",
           (unsigned long) lineas);
    printf("CAUDAL_INICIO\n");
    rtt_flush();

    uint32_t perdidos_antes = rtt_perdidos();
    uint32_t bytes = 0;
    uint32_t t0 = DWT_CYCCNT;

    for (uint32_t i = 0; i < lineas; i++) {
        /* autolimitarse: no encolar si no hay lugar holgado */
        while (rtt_pendientes() > (RTT_UP_SIZE / 2u)) { }
        bytes += (uint32_t) printf("%08lu 0123456789abcdefghijklmnopqrstuvwxyz\n",
                                   (unsigned long) i);
    }
    rtt_flush();

    uint32_t ciclos = DWT_CYCCNT - t0;
    uint32_t us = ciclos / CICLOS_US;

    printf("CAUDAL_FIN\n");
    printf("  lineas          : %lu\n", (unsigned long) lineas);
    printf("  bytes           : %lu\n", (unsigned long) bytes);
    printf("  descartados     : %lu\n",
           (unsigned long) (rtt_perdidos() - perdidos_antes));
    printf("  tiempo          : %lu us\n", (unsigned long) us);
    if (us > 0u) {
        printf("  caudal          : %lu bytes/s\n",
               (unsigned long) ((uint64_t) bytes * 1000000u / us));
        printf("  equivale a      : %lu baudios de UART\n",
               (unsigned long) ((uint64_t) bytes * 10u * 1000000u / us));
    }
    printf("\n  (el micro se autolimito: este es el ritmo al que el HOST\n");
    printf("   consume, no la velocidad a la que el micro podria producir)\n");
}


/* ---------------------------------------------------------------------------
 * PARTE 3: rafaga a fondo
 * ---------------------------------------------------------------------------
 * Sin autolimitarse. Muestra la otra cara: la cola es un amortiguador de
 * rafagas, no un cano mas ancho.
 * ------------------------------------------------------------------------ */
static void medir_rafaga(uint32_t lineas)
{
    printf("\n--- 3) Rafaga a fondo (%lu lineas sin frenar) ---\n",
           (unsigned long) lineas);
    rtt_flush();

    uint32_t perdidos_antes = rtt_perdidos();
    uint32_t t0 = DWT_CYCCNT;
    uint32_t pedidos = 0;

    for (uint32_t i = 0; i < lineas; i++) {
        pedidos += (uint32_t) printf("%08lu 0123456789abcdefghijklmnopqrstuvwxyz\n",
                                     (unsigned long) i);
    }
    uint32_t ciclos = DWT_CYCCNT - t0;
    uint32_t descartados = rtt_perdidos() - perdidos_antes;

    rtt_flush();
    printf("\n  bytes pedidos   : %lu\n", (unsigned long) pedidos);
    printf("  bytes perdidos  : %lu (%lu%%)\n",
           (unsigned long) descartados,
           (unsigned long) (pedidos ? (descartados * 100u / pedidos) : 0u));
    printf("  tiempo          : %lu us\n", (unsigned long) (ciclos / CICLOS_US));
    printf("  costo por linea : %lu ciclos\n",
           (unsigned long) (ciclos / lineas));
    printf("\n  La cola absorbe rafagas, no ensancha el cano. Si imprimis\n");
    printf("  mas rapido de lo que el host lee, se descarta: es a proposito.\n");
}


static void menu(void)
{
    printf("\nTeclas:  1 costo de CPU   2 caudal sostenido   "
           "3 rafaga a fondo   0 quieto\n\n");
}


int main(void)
{
    rtt_init();
    DEMCR |= (1u << 24); DWT_CYCCNT = 0; DWT_CTRL |= 1u;
    LPC_GPIO0->FIODIR |= LED_MASK;

    /* Un respiro para que el host se conecte antes de largar el informe. */
    for (volatile uint32_t d = 0; d < 8000000u; d++) { }

    encabezado();
    medir_costo_cpu();
    menu();

    uint32_t n = 0;
    uint32_t quieto = 0;

    while (1) {
        int k = rtt_getchar();

        switch (k) {
        case '1': medir_costo_cpu();     menu(); break;
        case '2': medir_caudal(2000u);   menu(); break;
        case '3': medir_rafaga(2000u);   menu(); break;
        case '0': quieto = !quieto;              break;
        default:  break;
        }

        if (!quieto) {
            printf("listo. tick %lu (descartados %lu)\n",
                   (unsigned long) n, (unsigned long) rtt_perdidos());
            n++;
        }
        LPC_GPIO0->FIOPIN ^= LED_MASK;
        for (volatile uint32_t d = 0; d < 3000000u; d++) { }
    }
}
