/*
 * main.c  —  Ejemplo de uso de nuestra librería mygpio.
 *
 * Parpadea un LED en P0.22 y, si se aprieta un botón en P2.10, lo apaga.
 * Notá que el main NO conoce ni una sola dirección de registro: todo está
 * encapsulado en mygpio. Eso es lo que hace una librería.
 */
#include "mygpio.h"

#define LED_PUERTO    0
#define LED_PIN       22
#define BOTON_PUERTO  2
#define BOTON_PIN     10

static void delay(volatile uint32_t n) { while (n--) { } }

int main(void)
{
    mygpio_dir(LED_PUERTO,   LED_PIN,   SALIDA);
    mygpio_dir(BOTON_PUERTO, BOTON_PIN, ENTRADA);

    while (1) {
        if (mygpio_read(BOTON_PUERTO, BOTON_PIN) == 0) {
            /* botón apretado (a GND con pull-up): mantener LED apagado */
            mygpio_clr(LED_PUERTO, LED_PIN);
        } else {
            mygpio_toggle(LED_PUERTO, LED_PIN);
            delay(1000000);
        }
    }
}
