/* Ejemplo mínimo del Debug Framework mejorado. */

#include <stdint.h>

#include "LPC17xx.h"
#include "debug_frmwrk_mejorado.h"

#define LED_MASK (1u << 22)

static void demora(volatile uint32_t vueltas)
{
    while (vueltas-- != 0u) {
        __asm__ volatile ("nop");
    }
}

int main(void)
{
    debug_mejorado_init();
    LPC_GPIO0->FIODIR |= LED_MASK;

    DBG_LINE("Debug Framework mejorado iniciado");

    uint32_t contador = 0u;
    while (1) {
        DBG_MSG("contador=");
        DBG_DEC32(contador);
        DBG_MSG(" perdidos=");
        DBG_DEC32(debug_mejorado_perdidos());
        DBG_MSG("\r\n");

        LPC_GPIO0->FIOPIN ^= LED_MASK;
        demora(5000000u);
        contador++;
    }
}
