#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"

#define LED_P022 (1u << 22)

static const uint32_t led_encendido = LED_P022;
static const uint32_t patrones_burst[4] = {0u, LED_P022, 0u, LED_P022};

static uint32_t control_dma(size_t cantidad, GPDMA_BURST_SIZE burst, bool incrementar_origen)
{
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words.
    control |= GPDMA_DMACCxControl_SBSize(burst); // Burst de lectura elegido.
    control |= GPDMA_DMACCxControl_DBSize(burst); // Burst de escritura elegido.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Lee words de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Escribe FIOPIN completo.
    if (incrementar_origen) {
        control |= GPDMA_DMACCxControl_SI; // Avanza por la tabla de patrones.
    }
    control |= GPDMA_DMACCxControl_I; // Interrumpe al completar.
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
void config_dma_gpio_request_software(const uint32_t *valor)
{
    const uint32_t control = control_dma(1u, GPDMA_BSIZE_1, false);

    config_pin_led();
    LPC_SC->DMAREQSEL |= (1u << 0); // La línea 8 selecciona MAT0.0.
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia error anterior.
    LPC_GPDMACH6->DMACCSrcAddr = (uint32_t)(uintptr_t)valor; // Word que se escribirá.
    LPC_GPDMACH6->DMACCDestAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO destino.
    LPC_GPDMACH6->DMACCLLI = 0u; // Sin LLI.
    LPC_GPDMACH6->DMACCControl = control; // Una word y origen fijo.
    LPC_GPDMACH6->DMACCConfig = GPDMA_DMACCxConfig_TransferType(GPDMA_M2P) | GPDMA_DMACCxConfig_DestPeripheral(8u) | GPDMA_DMACCxConfig_IE | GPDMA_DMACCxConfig_ITC; // M2P, request 8 e IRQ.
}

void disparar_dma_gpio_request_software(void)
{
    DMA_SoftRequest(GPDMA_MAT0_0); // Genera una single request.
}

// Una burst request consume las cuatro transferencias configuradas en DBSize.
void config_dma_gpio_burst_software(const uint32_t patrones[4])
{
    const uint32_t control = control_dma(4u, GPDMA_BSIZE_4, true);

    config_pin_led();
    LPC_SC->DMAREQSEL |= (1u << 0); // La línea 8 selecciona MAT0.0.
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia error anterior.
    LPC_GPDMACH6->DMACCSrcAddr = (uint32_t)(uintptr_t)patrones; // Inicio de la tabla.
    LPC_GPDMACH6->DMACCDestAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO destino.
    LPC_GPDMACH6->DMACCLLI = 0u; // Sin LLI.
    LPC_GPDMACH6->DMACCControl = control; // Cuatro words y origen incremental.
    LPC_GPDMACH6->DMACCConfig = GPDMA_DMACCxConfig_TransferType(GPDMA_M2P) | GPDMA_DMACCxConfig_DestPeripheral(8u) | GPDMA_DMACCxConfig_IE | GPDMA_DMACCxConfig_ITC; // M2P, request 8 e IRQ.
}

void disparar_dma_gpio_burst_software(void)
{
    DMA_SoftBurstRequest(GPDMA_MAT0_0); // Genera una burst request.
}

int main(void)
{
    (void)patrones_burst; // Tabla reservada para probar la variante burst.
    GPDMA_Init(); // Inicializa el controlador DMA.
    config_dma_gpio_request_software(&led_encendido); // Configura una única escritura al GPIO.
    GPDMA_ChannelStart(GPDMA_CH_6); // Deja el canal esperando una request.
    disparar_dma_gpio_request_software(); // Genera la request desde software.
    while (GPDMA_IntGetStatus(GPDMA_ENABLED_CH, GPDMA_CH_6) == SET) {} // Espera por polling.
    while (1) {} // El LED conserva el valor escrito por el DMA.
}
