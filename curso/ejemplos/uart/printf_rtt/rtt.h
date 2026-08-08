/*
 * rtt.h - printf() por el cable del debugger, sin UART.
 *
 * Ver rtt.c para el como y el por que, y ../MEDICIONES.md seccion 8 para los
 * comandos de OpenOCD con los que se mira la salida.
 */
#ifndef RTT_H
#define RTT_H

#include <stdint.h>

/* Arma el bloque de control y configura el buffering de stdout.
   Llamalo antes del primer printf(). No necesita ningun periferico. */
void rtt_init(void);

/* Bytes descartados porque el host no leyo a tiempo. Si crece, o estas
   imprimiendo de mas o no hay nadie conectado al canal. */
uint32_t rtt_perdidos(void);

#endif /* RTT_H */
