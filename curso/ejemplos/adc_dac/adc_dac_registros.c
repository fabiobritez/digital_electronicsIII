/*
 * ADC + DAC a registro: "passthrough" analogico.
 *
 * Lee un potenciometro en AD0.0 (P0.23) y saca la misma tension por
 * AOUT (P0.26). Girando el pote, la salida sigue a la entrada: se puede
 * verificar con un multimetro o un LED con buffer.
 *
 * Detalle no obvio: el ADC entrega 12 bits (0..4095) y el DAC acepta
 * 10 bits (0..1023), asi que hay que descartar los 2 bits menos
 * significativos (>> 2).
 *
 * Explicado en: curso/10_adc_dac/01-adc-dac-registros.md
 * Supone PCLK_ADC = 25 MHz (valor por defecto).
 */

#include "LPC17xx.h"

void adc_init(void) {
    // 1) Encender el ADC (PCONP bit 12 = PCADC). En reset viene apagado.
    LPC_SC->PCONP |= (1u << 12);

    // 2) Pin P0.23 como AD0.0 (función 1) y SIN resistencias (tri-state).
    LPC_PINCON->PINSEL1  &= ~(0x3u << 14);
    LPC_PINCON->PINSEL1  |=  (0x1u << 14);    // función 1 = AD0.0
    LPC_PINCON->PINMODE1 &= ~(0x3u << 14);
    LPC_PINCON->PINMODE1 |=  (0x2u << 14);    // 10 = tri-state

    // 3) ADCR: canal 0, CLKDIV para quedar <= 13 MHz, PDN=1, START=000.
    //    Con PCLK_ADC = 25 MHz: 25/(CLKDIV+1) <= 13 -> CLKDIV=1 -> 12.5 MHz.
    LPC_ADC->ADCR = (1u << 0)        // SEL: canal 0
                  | (1u << 8)        // CLKDIV = 1
                  | (1u << 21);      // PDN = 1 (encendido). START queda en 000.
}

uint16_t adc_read_ch0(void) {
    // Lanzar una conversión: START = 001 (bit 24). No tocar el resto de ADCR.
    LPC_ADC->ADCR &= ~(0x7u << 24);
    LPC_ADC->ADCR |=  (0x1u << 24);

    while (!(LPC_ADC->ADDR0 & (1u << 31))) { }   // esperar DONE del canal 0
    return (LPC_ADC->ADDR0 >> 4) & 0xFFF;        // RESULT en bits 15:4 -> 12 bits
}

void dac_init(void) {
    // P0.26 como AOUT (función 2). El DAC NO usa PCONP.
    LPC_PINCON->PINSEL1 &= ~(0x3u << 20);
    LPC_PINCON->PINSEL1 |=  (0x2u << 20);    // función 2 = AOUT
    LPC_PINCON->PINMODE1 &= ~(0x3u << 20);
    LPC_PINCON->PINMODE1 |=  (0x2u << 20);   // tri-state
}

void dac_write(uint16_t valor10bits) {
    // VALUE en bits 15:6. Preservar BIAS si ya estaba seteado.
    uint32_t reg = LPC_DAC->DACR & (1u << 16);    // conservar BIAS
    reg |= (uint32_t)(valor10bits & 0x3FF) << 6;
    LPC_DAC->DACR = reg;
}

int main(void) {
    adc_init();
    dac_init();

    while (1) {
        uint16_t muestra = adc_read_ch0();   // 0..4095
        dac_write(muestra >> 2);             // 0..1023: la salida sigue al pote
    }
    return 0;
}
