/* ============================================================================
 * Nivel intermedio: muestreo uniforme del ADC con Timer0 e interrupcion
 * ============================================================================
 *
 * Timer0 hace alternar internamente MAT0.1 cada 5 ms. El ADC detecta solamente
 * los flancos ascendentes, por lo que toma exactamente 100 muestras/s de AD0.0.
 * Cada conversion genera una interrupcion. La ISR calcula un promedio movil de
 * 16 muestras y main presenta el resultado en cuatro LEDs.
 *
 * No se usa el pin fisico MAT0.1: la conexion Timer0 -> ADC es interna.
 *
 * Hardware: igual que en 01_voltimetro_leds (pote en P0.23 y LEDs P0.19..22).
 * Plataforma: LPCXpresso LPC1769, CCLK = 100 MHz, PCLK = CCLK/4.
 * Compilar la plantilla con USE_CMSIS=1.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define LED_1                (1u << 22)
#define LED_2                (1u << 21)
#define LED_3                (1u << 20)
#define LED_4                (1u << 19)
#define TODOS_LED            (LED_1 | LED_2 | LED_3 | LED_4)

#define MUESTRAS_POR_SEGUNDO 100u
#define TAMANIO_PROMEDIO      16u

/* La ISR escribe estas variables y main o el debugger las leen. */
volatile uint16_t adc_muestra_cruda = 0u;
volatile uint16_t adc_promedio = 0u;
volatile uint32_t adc_milivoltios = 0u;
volatile uint32_t adc_conversiones = 0u;
volatile uint32_t adc_overruns = 0u;


void ADC_IRQHandler(void)
{
    static uint16_t muestras[TAMANIO_PROMEDIO];
    static uint32_t suma = 0u;
    static uint8_t indice = 0u;
    static uint8_t cantidad = 0u;
    uint32_t registro = LPC_ADC->ADDR0;  /* leerlo reconoce la interrupcion */
    uint16_t nueva;

    /* Proteccion frente a una entrada espuria al handler. */
    if ((registro & (1u << 31)) == 0u) {
        return;
    }

    if ((registro & (1u << 30)) != 0u) {
        adc_overruns++;
    }

    nueva = (uint16_t)((registro >> 4) & 0xFFFu);
    adc_muestra_cruda = nueva;

    /* Promedio movil: quitar la muestra vieja y agregar la nueva.
     * Como el tamanio es potencia de dos, la mascara reemplaza al modulo. */
    suma -= muestras[indice];
    muestras[indice] = nueva;
    suma += nueva;
    indice = (uint8_t)((indice + 1u) & (TAMANIO_PROMEDIO - 1u));

    if (cantidad < TAMANIO_PROMEDIO) {
        cantidad++;
    }

    adc_promedio = (uint16_t)(suma / cantidad);
    adc_milivoltios = ((uint32_t)adc_promedio * 3300u) / 4095u;
    adc_conversiones++;
}


static void leds_inicializar(void)
{
    LPC_PINCON->PINSEL1 &= ~(0xFFu << 6);  /* P0.19..P0.22 = GPIO */
    LPC_GPIO0->FIODIR |= TODOS_LED;
    LPC_GPIO0->FIOCLR = TODOS_LED;
}


static void leds_mostrar_nivel(uint16_t cuentas)
{
    uint32_t encendidos = 0u;

    /* No se usa UART ni DAC. Las variables globales se observan con el debugger. */
    if (cuentas >= 820u)  { encendidos |= LED_1; }
    if (cuentas >= 1638u) { encendidos |= LED_2; }
    if (cuentas >= 2457u) { encendidos |= LED_3; }
    if (cuentas >= 3276u) { encendidos |= LED_4; }

    LPC_GPIO0->FIOCLR = TODOS_LED;
    LPC_GPIO0->FIOSET = encendidos;
}


static void adc_inicializar(void)
{
    LPC_SC->PCONP |= (1u << 12);          /* alimentar ADC */
    LPC_SC->PCLKSEL0 &= ~(3u << 24);      /* PCLK_ADC = 25 MHz */

    LPC_PINCON->PINSEL1 &= ~(3u << 14);
    LPC_PINCON->PINSEL1 |=  (1u << 14);   /* P0.23 = AD0.0 */
    LPC_PINCON->PINMODE1 &= ~(3u << 14);
    LPC_PINCON->PINMODE1 |=  (2u << 14);  /* sin pull-up ni pull-down */

    /* START = 100 selecciona como disparo el flanco ascendente de MAT0.1.
     * EDGE queda en 0 y BURST queda deshabilitado. */
    LPC_ADC->ADCR = (1u << 0)             /* canal AD0.0 */
                  | (1u << 8)             /* f_ADC = 12,5 MHz */
                  | (1u << 21)            /* ADC encendido */
                  | (4u << 24);           /* trigger MAT0.1 */

    /* Interrupcion del canal 0. ADGINTEN (bit 8) debe quedar en cero para
     * seleccionar las fuentes individuales ADINTEN0..7. */
    LPC_ADC->ADINTEN = (1u << 0);
    NVIC_ClearPendingIRQ(ADC_IRQn);
    NVIC_EnableIRQ(ADC_IRQn);
}


static void timer0_inicializar_disparo_adc(void)
{
    LPC_SC->PCONP |= (1u << 1);           /* alimentar Timer0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);       /* PCLK_Timer0 = 25 MHz */

    LPC_TIM0->TCR = (1u << 1);            /* frenar y resetear */
    LPC_TIM0->CTCR = 0u;                  /* modo timer */

    /* 25 MHz/(24+1) = 1 MHz: TC avanza una vez por microsegundo. */
    LPC_TIM0->PR = 24u;

    /* Hay un match cada 5 ms y MAT0.1 alterna en cada match. Solo uno de cada
     * dos es ascendente: 200 flancos/s / 2 = 100 conversiones/s. */
    LPC_TIM0->MR1 = (1000000u / (2u * MUESTRAS_POR_SEGUNDO)) - 1u;
    LPC_TIM0->MCR = (1u << 4);            /* reset de TC al coincidir MR1 */
    LPC_TIM0->EMR = (3u << 6);            /* EMC1 = toggle; MAT0.1 inicia en 0 */
    LPC_TIM0->IR = 0x3Fu;

    /* No se habilita la IRQ del Timer0: el match dispara directamente al ADC. */
    LPC_TIM0->TCR = (1u << 0);            /* arrancar */
}


int main(void)
{
    uint32_t conversion_anterior = 0u;

    leds_inicializar();
    adc_inicializar();
    timer0_inicializar_disparo_adc();

    while (1) {
        /* adc_promedio es de 16 bits: su lectura es atomica en Cortex-M3. */
        if (adc_conversiones != conversion_anterior) {
            conversion_anterior = adc_conversiones;
            leds_mostrar_nivel(adc_promedio);
        }

        /* El CPU sleep hasta la proxima interrupcion; el Timer y el ADC
         * siguen funcionando sin intervencion del programa principal. */
        __WFI();
    }
}
