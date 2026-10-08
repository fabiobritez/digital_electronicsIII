/* ============================================================================
 * DAC sin DMA, parte 2: onda senoidal con tabla y Timer0 por interrupcion
 * ============================================================================
 *
 * Timer0 interrumpe cada 80 us (12,5 kHz). La ISR toma una de 50 muestras de
 * una tabla y la escribe en DACR. La onda resultante es de 12,5 kHz/50 = 250 Hz.
 * Mientras tanto, main duerme con WFI. No se usa DMA ni punto flotante.
 *
 * Salida: P0.26 / AOUT. Medir con osciloscopio respecto de GND.
 * Plataforma: LPCXpresso LPC1769, CCLK = 100 MHz, PCLK_Timer0 = CCLK/4.
 * Compilar la plantilla con USE_CMSIS=1.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define PCLK_TIMER0_HZ          25000000u
#define TICK_TIMER0_HZ           1000000u
#define TASA_ACTUALIZACION_HZ      12500u
#define CANTIDAD_MUESTRAS              50u

/* 512 + 400*sin(2*pi*n/50), calculada fuera del micro y redondeada.
 * El rango 112..912 deja margen respecto de ambos rieles. */
static const uint16_t tabla_seno[CANTIDAD_MUESTRAS] = {
    512u, 562u, 611u, 659u, 705u, 747u, 786u, 820u, 850u, 874u,
    892u, 905u, 911u, 911u, 905u, 892u, 874u, 850u, 820u, 786u,
    747u, 705u, 659u, 611u, 562u, 512u, 462u, 413u, 365u, 319u,
    277u, 238u, 204u, 174u, 150u, 132u, 119u, 113u, 113u, 119u,
    132u, 150u, 174u, 204u, 238u, 277u, 319u, 365u, 413u, 462u
};

/* Variables utiles para observar en Watch/Live Expressions. */
volatile uint16_t dac_ultimo_valor = 0u;
volatile uint32_t dac_indice = 0u;
volatile uint32_t muestras_generadas = 0u;


void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = (1u << 0);    /* reconocer la interrupcion de MR0 */

    dac_ultimo_valor = tabla_seno[dac_indice];
    LPC_DAC->DACR = ((uint32_t)dac_ultimo_valor << 6); /* BIAS=0 */

    dac_indice++;
    if (dac_indice == CANTIDAD_MUESTRAS) {
        dac_indice = 0u;
    }
    muestras_generadas++;
}


static void dac_inicializar(void)
{
    /* Seleccionar AOUT antes de acceder a cualquier registro del DAC. */
    LPC_PINCON->PINSEL1 &= ~(3u << 20);
    LPC_PINCON->PINSEL1 |=  (2u << 20);   /* P0.26, funcion 2 = AOUT */
    LPC_PINCON->PINMODE1 &= ~(3u << 20);
    LPC_PINCON->PINMODE1 |=  (2u << 20);  /* sin pull-up ni pull-down */

    LPC_DAC->DACCTRL = 0u;               /* contador interno y DMA apagados */
    LPC_DAC->DACR = (512u << 6);          /* nivel medio antes de arrancar */
}


static void timer0_inicializar(void)
{
    LPC_SC->PCONP |= (1u << 1);          /* alimentar Timer0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);      /* PCLK_Timer0 = CCLK/4 = 25 MHz */

    LPC_TIM0->TCR = (1u << 1);           /* detener y resetear */
    LPC_TIM0->CTCR = 0u;                 /* modo timer */
    LPC_TIM0->PR = (PCLK_TIMER0_HZ / TICK_TIMER0_HZ) - 1u;

    /* 1 MHz / 12 500 Hz = 80 ticks: un match cada 80 us. */
    LPC_TIM0->MR0 = (TICK_TIMER0_HZ / TASA_ACTUALIZACION_HZ) - 1u;
    LPC_TIM0->MCR = (1u << 0) | (1u << 1); /* interrumpir y resetear en MR0 */
    LPC_TIM0->IR = 0x3Fu;

    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = (1u << 0);           /* arrancar */
}


int main(void)
{
    dac_inicializar();
    timer0_inicializar();

    while (1) {
        /* Timer0 despierta al CPU 12 500 veces/s. La ISR escribe una muestra
         * y main vuelve a dormir; no hay espera activa como en el ejemplo 03. */
        __WFI();
    }
}
