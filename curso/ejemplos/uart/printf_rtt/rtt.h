/* ============================================================================
 * rtt.h - Consola de depuracion por el cable del debugger (SWD)
 * ============================================================================
 *
 * printf() y scanf() sin UART, sin pines y sin periferico: el programa escribe
 * en una cola en RAM y el debugger la lee POR SWD mientras el micro corre.
 * La unidad de debug del Cortex-M3 accede a memoria en paralelo al CPU, asi
 * que el programa ni se entera.
 *
 * Medido en una LPCXpresso LPC1769, linea de 48 caracteres:
 *
 *     printf por UART, polling : 4091 us de CPU bloqueado
 *     printf por UART, DMA     :   36 us
 *     printf por RTT           :   24 us
 *
 * USO MINIMO
 * ----------
 *     #include "rtt.h"
 *     int main(void) {
 *         rtt_init();                  // antes del primer printf
 *         printf("hola\n");
 *     }
 *
 * Del lado de la PC, ver la guia completa en:
 *     curso/12_debug/03-consola-por-el-debugger-rtt.md
 *
 * FUNCIONA CON LAS DOS LIBC
 * -------------------------
 * Define los enganches de newlib (_write/_read, que es lo que usa la plantilla
 * del repo y el toolchain de linea de comandos) y los de Redlib
 * (__sys_write/__sys_readc, que es lo que usa MCUXpresso por defecto). No hay
 * que configurar nada: se compila el que corresponda y el otro lo descarta el
 * linker con --gc-sections.
 *
 * QUE NO HACE
 * -----------
 * - No necesita el debugger para CORRER, pero si para VER algo. En un equipo
 *   sin debugger conectado, los printf se descartan (y se cuentan).
 * - No es reentrante. Ver la nota sobre ISRs mas abajo.
 * ========================================================================= */

#ifndef RTT_H
#define RTT_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Configuracion. Se puede pisar desde el Makefile con -DRTT_UP_SIZE=2048
 * ------------------------------------------------------------------------ */

/* Cola de salida (micro -> PC). Mas grande = aguanta mas rafagas sin perder.
   Tiene que entrar en la RAM: 1 KB de los 32 KB del LPC1769. */
#ifndef RTT_UP_SIZE
#define RTT_UP_SIZE     1024u
#endif

/* Cola de entrada (PC -> micro), para getchar()/scanf(). Chica alcanza:
   se escribe a velocidad de tecleo. */
#ifndef RTT_DOWN_SIZE
#define RTT_DOWN_SIZE   32u
#endif

/* Que hacer cuando la cola de salida se llena porque el host no lee:
 *   0 = DESCARTAR lo que no entra y seguir  (por defecto, y lo correcto
 *       para depurar: nunca frena el programa que estas observando)
 *   1 = BLOQUEAR hasta que haya lugar       (no perdes nada, pero si no hay
 *       nadie leyendo el programa se cuelga ahi para siempre)
 */
#ifndef RTT_BLOQUEANTE
#define RTT_BLOQUEANTE  0
#endif

/* 1 = protege la escritura con una seccion critica, para poder imprimir desde
 *     una ISR sin corromper la cola.
 *
 * OJO: esto hace segura LA COLA, no printf(). printf() sigue sin ser
 * reentrante (estado global y, segun el caso, heap). Si de verdad necesitas
 * mensajes desde una ISR, lo correcto sigue siendo que la ISR levante una
 * bandera y el main imprima. Esto es una red de seguridad, no un permiso.
 */
#ifndef RTT_SEGURO_ISR
#define RTT_SEGURO_ISR  0
#endif


/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------ */

/* Arma el bloque de control y configura el buffering de stdout.
   Llamalo una vez, antes del primer printf(). No toca ningun periferico. */
void rtt_init(void);

/* Bytes que se descartaron porque el host no leia. Si esto crece:
   o no hay nadie conectado al canal, o estas imprimiendo de mas. */
uint32_t rtt_perdidos(void);

/* Bytes esperando a que el host los lea. */
uint32_t rtt_pendientes(void);

/* Espera a que el host termine de leer todo lo pendiente.
 *
 * Sirve antes de un reset, de un breakpoint o dentro de un HardFault: sin
 * esto, lo ultimo que imprimiste todavia esta en la cola y no lo ves nunca.
 *
 * Tiene un tope de espera para que NO se cuelgue si no hay nadie leyendo.
 * Devuelve 1 si se vacio, 0 si se canso de esperar. */
int rtt_flush(void);

/* Un byte mandado desde la PC, o -1 si no hay nada. No bloquea.
   getchar() y scanf() ya pasan por aca solos. */
int rtt_getchar(void);

#endif /* RTT_H */
