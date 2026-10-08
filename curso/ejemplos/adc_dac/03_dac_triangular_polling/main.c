/* ============================================================================
 * DAC sin DMA, parte 1: onda triangular con Timer0 consultado por polling
 * ============================================================================
 *
 * Timer0 marca una actualizacion cada 100 us. El programa espera el flag de
 * match, calcula la siguiente muestra de una triangular y la escribe en DACR.
 * Con 100 muestras por periodo se obtiene una onda de 100 Hz.
 *
 * Salida: P0.26 / AOUT. Medir con osciloscopio respecto de GND.
 * Plataforma: LPCXpresso LPC1769, CCLK = 100 MHz, PCLK_Timer0 = CCLK/4.
 * Compilar la plantilla con USE_CMSIS=1.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define PCLK_TIMER0_HZ          25000000u
#define TICK_TIMER0_HZ           1000000u
#define TASA_ACTUALIZACION_HZ      10000u
#define MUESTRAS_POR_PERIODO         100u

#define DAC_MINIMO                   112u
#define DAC_MAXIMO                   912u
#define DAC_PASO                      16u

/* Variables utiles para observar en Watch/Live Expressions. */
volatile uint16_t dac_ultimo_valor = 0u;
volatile uint32_t muestras_generadas = 0u;


static void dac_escribir(uint16_t valor)
{
    /* VALUE ocupa DACR[15:6]. BIAS=0: settling maximo de 1 us. */
    LPC_DAC->DACR = ((uint32_t)(valor & 0x3FFu) << 6);
    dac_ultimo_valor = valor;
}


static void dac_inicializar(void)
{
    /* El manual exige seleccionar AOUT antes de acceder a los registros DAC. */
    LPC_PINCON->PINSEL1 &= ~(3u << 20);
    LPC_PINCON->PINSEL1 |=  (0b10 << 20);   /* P0.26, funcion 2 = AOUT */
    LPC_PINCON->PINMODE1 &= ~(3u << 20);
    LPC_PINCON->PINMODE1 |=  (2u << 20);  /* sin pull-up ni pull-down */

    /* No se usa el contador interno ni DMA; cada escritura llega directo. */
    LPC_DAC->DACCTRL = 0u;
    dac_escribir(512u);                  /* arrancar cerca de 1,65 V */
}


static void timer0_inicializar(void)
{
    LPC_SC->PCONP |= (1u << 1);          /* alimentar Timer0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);      /* PCLK_Timer0 = CCLK/4 = 25 MHz */

    LPC_TIM0->TCR = (1u << 1);           /* detener y resetear */
    LPC_TIM0->CTCR = 0u;                 /* modo timer */

    /* 25 MHz/(24+1) = 1 MHz: TC avanza una vez por microsegundo. */
    LPC_TIM0->PR = (PCLK_TIMER0_HZ / TICK_TIMER0_HZ) - 1u;
    LPC_TIM0->MR0 = (TICK_TIMER0_HZ / TASA_ACTUALIZACION_HZ) - 1u;
    /* MR0I levanta IR[0] y MR0R reinicia el periodo. La IRQ permanece
     * deshabilitada en el NVIC: el flag se consulta por polling. */
    LPC_TIM0->MCR = (1u << 0) | (1u << 1);
    LPC_TIM0->IR = 0x3Fu;                /* limpiar flags anteriores */
    LPC_TIM0->TCR = (1u << 0);           /* arrancar */
}


static uint16_t triangular_calcular(uint32_t fase)
{
    /* 50 escalones ascendentes y 50 descendentes. El rango queda alejado de
     * los rieles: 112..912 equivale aproximadamente a 0,36..2,94 V. */
    if (fase < (MUESTRAS_POR_PERIODO / 2u)) {
        return (uint16_t)(DAC_MINIMO + DAC_PASO * fase);
    }

    return (uint16_t)(DAC_MAXIMO
                    - DAC_PASO * (fase - MUESTRAS_POR_PERIODO / 2u));
}


int main(void)
{
    uint32_t fase = 0u;

    dac_inicializar();
    timer0_inicializar();

    while (1) {
        /* El match ocurre por hardware; el CPU solo consulta y limpia el flag.
         * No se habilita TIMER0_IRQn en este primer ejemplo. */
        while ((LPC_TIM0->IR & (1u << 0)) == 0u) { }
        LPC_TIM0->IR = (1u << 0);

        dac_escribir(triangular_calcular(fase));
        muestras_generadas++;

        fase++;
        if (fase == MUESTRAS_POR_PERIODO) {
            fase = 0u;
        }
    }
}
