/* ============================================================================
 * Nivel basico: voltimetro visual con ADC y cuatro LEDs
 * ============================================================================
 *
 * Un potenciometro entrega entre 0 V y 3,3 V a AD0.0 (P0.23). El ADC convierte
 * esa tension a un numero de 12 bits y una barra de cuatro LEDs muestra el
 * nivel aproximado. Las variables adc_cuentas y adc_milivoltios se pueden
 * observar desde el debugger; no se usa UART.
 *
 * Hardware:
 *
 *     3,3 V ---- extremo del potenciometro
 *                  cursor ------------ P0.23 / AD0.0
 *     GND  ---- extremo del potenciometro
 *
 *     P0.22 --------------------------- LED 1 de la placa
 *     P0.21 ---- 330 ohm ----|>|---- GND   LED 2 externo
 *     P0.20 ---- 330 ohm ----|>|---- GND   LED 3 externo
 *     P0.19 ---- 330 ohm ----|>|---- GND   LED 4 externo
 *
 *  LPCXpresso LPC1769, CCLK = 100 MHz y PCLK_ADC = CCLK/4.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define LED_1       (1u << 22)
#define LED_2       (1u << 21)
#define LED_3       (1u << 20)
#define LED_4       (1u << 19)
#define TODOS_LED   (LED_1 | LED_2 | LED_3 | LED_4)

/* Son globales y volatile para poder verlas cambiar desde el debugger. */
volatile uint16_t adc_cuentas = 0u;
volatile uint32_t adc_milivoltios = 0u;


static void leds_inicializar(void)
{
    /* P0.19 a P0.22 como GPIO (funcion 00 en PINSEL1). */
    LPC_PINCON->PINSEL1 &= ~(0xFFu << 6);
    LPC_GPIO0->FIODIR |= TODOS_LED;
    LPC_GPIO0->FIOCLR = TODOS_LED;
}


static void adc_inicializar(void)
{
    /* 1) Alimentar el ADC y elegir PCLK_ADC = CCLK/4 = 25 MHz. */
    LPC_SC->PCONP |= (1u << 12);          /* PCADC */
    LPC_SC->PCLKSEL0 &= ~(3u << 24);

    /* 2) P0.23 como AD0.0 (funcion 01), sin pull-up ni pull-down. */
    LPC_PINCON->PINSEL1 &= ~(3u << 14);
    LPC_PINCON->PINSEL1 |=  (1u << 14);
    LPC_PINCON->PINMODE1 &= ~(3u << 14);
    LPC_PINCON->PINMODE1 |=  (2u << 14);  /* 10 = tri-state */

    /* 3) Canal 0, f_ADC = 25 MHz/(1+1) = 12,5 MHz, ADC encendido.
     * START queda en 000 hasta pedir cada conversion desde software. */
    LPC_ADC->ADCR = (1u << 0)             /* SEL: AD0.0 */
                  | (1u << 8)             /* CLKDIV = 1 */
                  | (1u << 21);           /* PDN = 1 */
}


static uint16_t adc_leer(void)
{
    uint32_t registro;

    /* START = 001: iniciar ahora. Primero se limpia todo el campo START. */
    LPC_ADC->ADCR &= ~(7u << 24);
    LPC_ADC->ADCR |=  (1u << 24);

    /* ADDR0.DONE se pone en uno al terminar. Leer ADDR0 limpia DONE, por eso
     * se conserva la lectura que hizo salir del lazo. */
    do {
        registro = LPC_ADC->ADDR0;
    } while ((registro & (1u << 31)) == 0u);

    return (uint16_t)((registro >> 4) & 0xFFFu);
}


static void leds_mostrar_nivel(uint16_t cuentas)
{
    uint32_t encendidos = 0u;

    /* Cada escalon representa aproximadamente el 20 % del fondo de escala.
     * Debajo de 0,66 V quedan todos apagados; por encima de 2,64 V, los cuatro. */
    encendidos = (cuentas >> 8 ) & 0xF;
    LPC_GPIO0->FIOCLR = TODOS_LED;
    LPC_GPIO0->FIOSET = encendidos;
}


int main(void)
{
    leds_inicializar();
    adc_inicializar();

    while (1) {
        adc_cuentas = adc_leer();

        /* Solo aritmetica entera: 0..4095 cuentas -> 0..3300 mV.
         * El producto maximo cabe holgadamente en uint32_t. */
       
        leds_mostrar_nivel(adc_cuentas);
    }
}
