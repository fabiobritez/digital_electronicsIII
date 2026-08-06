/*
 * mygpio.c  —  Implementación de la mini librería de GPIO.
 *
 * Cada función hace, por dentro, exactamente lo que harías "a registro pelado".
 * La gracia es que el resto del programa no tiene que saber direcciones ni máscaras.
 */
#include "mygpio.h"

/* Tabla interna: traduce un número de puerto (0..4) a su struct.
 * Es un detalle de implementación: el usuario de la librería no la ve. */
static MYGPIO_Port * const puertos[5] = {
    MYGPIO0, MYGPIO1, MYGPIO2, MYGPIO3, MYGPIO4
};

void mygpio_dir(uint8_t puerto, uint8_t pin, MYGPIO_Dir dir)
{
    if (puerto > 4) return;                 /* guarda simple */
    if (dir == SALIDA)
        puertos[puerto]->FIODIR |=  (1u << pin);   /* poner el bit -> salida */
    else
        puertos[puerto]->FIODIR &= ~(1u << pin);   /* borrar el bit -> entrada */
}

void mygpio_set(uint8_t puerto, uint8_t pin)
{
    if (puerto > 4) return;
    puertos[puerto]->FIOSET = (1u << pin);  /* SET: escribir 1 no afecta a los otros pines */
}

void mygpio_clr(uint8_t puerto, uint8_t pin)
{
    if (puerto > 4) return;
    puertos[puerto]->FIOCLR = (1u << pin);  /* CLR: idem */
}

void mygpio_toggle(uint8_t puerto, uint8_t pin)
{
    if (puerto > 4) return;
    puertos[puerto]->FIOPIN ^= (1u << pin); /* XOR sobre el estado actual (leer-modificar-escribir:
                                               simple, pero no atómico como SET/CLR; ver módulo 5) */
}

uint8_t mygpio_read(uint8_t puerto, uint8_t pin)
{
    if (puerto > 4) return 0;
    return (puertos[puerto]->FIOPIN >> pin) & 1u;
}
