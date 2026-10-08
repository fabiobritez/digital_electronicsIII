#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

#define LED_P022 (1u << 22)

static const uint32_t led_encendido = LED_P022;
static const uint32_t patrones_burst[4] = {0u, LED_P022, 0u, LED_P022};

static uint32_t control_single(void)
{
    const size_t cantidad = 1u;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una word.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una word.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Lee words de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Escribe FIOPIN completo.
    return control;
}

static uint32_t control_burst(void)
{
    const size_t cantidad = 4u;
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cuatro words.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_4); // Lee las cuatro words.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_4); // Escribe las cuatro words.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Lee words de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Escribe FIOPIN completo.
    control |= GPDMA_DMACCxControl_SI; // Avanza por la tabla de patrones.
    return control;
}

static void config_pin_led(void)
{
    PINSEL_CFG_T pin;
    pin.port = PORT_0; // Puerto 0.
    pin.pin = PIN_22; // Pin del LED.
    pin.func = PINSEL_FUNC_00; // Función GPIO.
    pin.mode = PINSEL_PULLUP; // Habilita el pull-up.
    pin.openDrain = DISABLE; // Salida push-pull.
    PINSEL_ConfigPin(&pin);
    GPIO_SetDir(PORT_0, LED_P022, GPIO_OUTPUT);
}

// Una request de software permite avanzar una transferencia paso a paso.
// Se usa una línea sin un productor de hardware activo.
void config_dma_gpio_request_software(void)
{
    const uint32_t *valor = &led_encendido;
    const uint32_t control = control_single();

    config_pin_led();
    LPC_SC->DMAREQSEL |= (1u << 0); // La línea 8 selecciona MAT0.0.
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia error anterior.
    LPC_GPDMACH6->DMACCSrcAddr = (uint32_t)(uintptr_t)valor; // Word que se escribirá.
    LPC_GPDMACH6->DMACCDestAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO destino.
    LPC_GPDMACH6->DMACCLLI = 0u; // Sin LLI.
    LPC_GPDMACH6->DMACCControl = control; // Una word y origen fijo.
    LPC_GPDMACH6->DMACCConfig =
        GPDMA_DMACCxConfig_TransferType(GPDMA_M2P) | // Memoria a periférico.
        GPDMA_DMACCxConfig_DestPeripheral(8u); // Request de software sobre MAT0.0.
}

void disparar_dma_gpio_request_software(void)
{
    DMA_SoftRequest(GPDMA_MAT0_0); // Genera una single request.
}

// Una burst request consume las cuatro transferencias configuradas en DBSize.
void config_dma_gpio_burst_software(void)
{
    const uint32_t *patrones = patrones_burst;
    const uint32_t control = control_burst();

    config_pin_led();
    LPC_SC->DMAREQSEL |= (1u << 0); // La línea 8 selecciona MAT0.0.
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia error anterior.
    LPC_GPDMACH6->DMACCSrcAddr = (uint32_t)(uintptr_t)patrones; // Inicio de la tabla.
    LPC_GPDMACH6->DMACCDestAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO destino.
    LPC_GPDMACH6->DMACCLLI = 0u; // Sin LLI.
    LPC_GPDMACH6->DMACCControl = control; // Cuatro words y origen incremental.
    LPC_GPDMACH6->DMACCConfig =
        GPDMA_DMACCxConfig_TransferType(GPDMA_M2P) | // Memoria a periférico.
        GPDMA_DMACCxConfig_DestPeripheral(8u); // Request de software sobre MAT0.0.
}

void disparar_dma_gpio_burst_software(void)
{
    DMA_SoftBurstRequest(GPDMA_MAT0_0); // Genera una burst request.
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    config_dma_gpio_request_software(); // Configura una única escritura al GPIO.
    GPDMA_ChannelStart(GPDMA_CH_6); // Deja el canal esperando una request.
    disparar_dma_gpio_request_software(); // Genera la request desde software.
    while (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, GPDMA_CH_6) == SET && GPDMA_IntGetStatus(GPDMA_RAW_INTERR, GPDMA_CH_6) == RESET) {} // Espera por polling.
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, GPDMA_CH_6) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_6); // Limpia el error detectado.
        while (1) {}
    }
    while (1) {} // El LED conserva el valor escrito por el DMA.
}
