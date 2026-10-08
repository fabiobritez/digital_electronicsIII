#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#define DMA_MAX_TRANSFERENCIAS 4095u
#define LED_P022 (1u << 22)

static const uint32_t patrones_led[] = {0u, LED_P022, 0u, LED_P022};
static uint32_t muestras_gpio[32];

static uint32_t control_lli(size_t cantidad, bool incrementar_origen, bool incrementar_destino, bool irq)
{
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words del bloque.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una word por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una word por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Origen de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Destino de 32 bits.
    if (incrementar_origen) {
        control |= GPDMA_DMACCxControl_SI; // Avanza la dirección de origen.
    }
    if (incrementar_destino) {
        control |= GPDMA_DMACCxControl_DI; // Avanza la dirección de destino.
    }
    if (irq) {
        control |= GPDMA_DMACCxControl_I; // Interrumpe al terminar el bloque.
    }
    return control;
}

static void config_pines_gpio(void)
{
    PINSEL_CFG_T led;
    led.port = PORT_0; // Puerto 0.
    led.pin = PIN_22; // Pin del LED.
    led.func = PINSEL_FUNC_00; // Función GPIO.
    led.mode = PINSEL_PULLUP; // Habilita el pull-up.
    led.openDrain = DISABLE; // Salida push-pull.

    PINSEL_CFG_T entrada;
    entrada.port = PORT_0; // Puerto 0.
    entrada.pin = PIN_10; // Pin que se muestrea.
    entrada.func = PINSEL_FUNC_00; // Función GPIO.
    entrada.mode = PINSEL_PULLUP; // Pulsador activo en bajo.
    entrada.openDrain = DISABLE; // Sin drenador abierto.
    PINSEL_ConfigPin(&led);
    PINSEL_ConfigPin(&entrada);
    GPIO_SetDir(PORT_0, LED_P022, GPIO_OUTPUT);
    GPIO_SetDir(PORT_0, 1u << 10, GPIO_INPUT);
}

// GPIO no genera requests. MAT0.0/MAT0.1 se usan solamente como reloj.
// La request de MAT y la dirección FIOPIN se configuran por separado.
static void config_timer0_match_periodico(TIM_MATCH_CH match, uint32_t periodo_us)
{
    TIM_TIMERCFG_T timer;
    timer.prescaleOpt = TIM_US; // Cuenta en microsegundos.
    timer.prescaleValue = 1u; // Un conteo por microsegundo.

    TIM_MATCHCFG_T evento;
    evento.channel = match; // Selecciona MAT0.0 o MAT0.1.
    evento.intEn = DISABLE; // El evento alimenta al DMA, no al CPU.
    evento.stopEn = DISABLE; // El timer continúa contando.
    evento.resetEn = ENABLE; // Reinicia en cada período.
    evento.extOpt = TIM_NOTHING; // No modifica un pin MAT.
    evento.matchValue = periodo_us; // Período entre requests.
    TIM_InitTimer(LPC_TIM0, &timer);
    TIM_ConfigMatch(LPC_TIM0, &evento);
}

static void config_canal_dma_raw(LPC_GPDMACH_TypeDef *ch, GPDMA_CH numero, uintptr_t origen, uintptr_t destino, const GPDMA_LLI_T *siguiente, uint32_t control, GPDMA_TRANSFER_TYPE tipo, uint8_t request_origen, uint8_t request_destino)
{
    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(numero);
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(numero);
    ch->DMACCSrcAddr = (uint32_t)origen; // Dirección leída por el DMA.
    ch->DMACCDestAddr = (uint32_t)destino; // Dirección escrita por el DMA.
    ch->DMACCLLI = (uint32_t)(uintptr_t)siguiente; // Próximo descriptor o NULL.
    ch->DMACCControl = control; // Tamaño, anchos e incrementos.
    ch->DMACCConfig = GPDMA_DMACCxConfig_TransferType(tipo) | GPDMA_DMACCxConfig_SrcPeripheral(request_origen) | GPDMA_DMACCxConfig_DestPeripheral(request_destino) | GPDMA_DMACCxConfig_IE | GPDMA_DMACCxConfig_ITC; // Flujo, requests e IRQ.
}

static GPDMA_LLI_T gpio_salida_anillo;

// Reproduce estados completos de GPIO0, una word por MAT0.0.
Status config_dma_gpio_salida_periodica(const uint32_t *patrones, size_t cantidad, uint32_t periodo_us)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS || periodo_us == 0u) {
        return ERROR;
    }
    const uint32_t control = control_lli(cantidad, true, false, false);

    config_pines_gpio();
    config_timer0_match_periodico(TIM_MATCH_0, periodo_us);
    LPC_SC->DMAREQSEL |= (1u << 0); // Línea 8: MAT0.0, no UART0 Tx.

    gpio_salida_anillo.srcAddr = (uint32_t)(uintptr_t)patrones; // Reinicia la tabla de estados.
    gpio_salida_anillo.dstAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Escribe el puerto completo.
    gpio_salida_anillo.nextLLI = (uint32_t)(uintptr_t)&gpio_salida_anillo; // Repite en forma circular.
    gpio_salida_anillo.control = control; // Origen avanza; GPIO queda fijo.

    config_canal_dma_raw(LPC_GPDMACH6, GPDMA_CH_6, (uintptr_t)patrones, (uintptr_t)&LPC_GPIO0->FIOPIN, &gpio_salida_anillo, control, GPDMA_M2P, 0u, 8u);
    return SUCCESS;
}

// Toma N snapshots de GPIO0, uno por MAT0.1.
Status config_dma_gpio_entrada_periodica(uint32_t *muestras, size_t cantidad, uint32_t periodo_us)
{
    if (cantidad == 0u || cantidad > DMA_MAX_TRANSFERENCIAS || periodo_us == 0u) {
        return ERROR;
    }
    const uint32_t control = control_lli(cantidad, false, true, true);

    config_pines_gpio();
    config_timer0_match_periodico(TIM_MATCH_1, periodo_us);
    LPC_SC->DMAREQSEL |= (1u << 1); // Línea 9: MAT0.1, no UART0 Rx.

    config_canal_dma_raw(LPC_GPDMACH0, GPDMA_CH_0, (uintptr_t)&LPC_GPIO0->FIOPIN, (uintptr_t)muestras, NULL, control, GPDMA_P2M, 9u, 0u);
    return SUCCESS;
}

int main(void)
{
    (void)muestras_gpio; // Buffer reservado para probar la variante GPIO a memoria.
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura un patrón circular con un cambio cada 250 ms.
    if (config_dma_gpio_salida_periodica(patrones_led, sizeof(patrones_led) / sizeof(patrones_led[0]), 250000u) != SUCCESS) {
        while (1) {} // Se detiene si la configuración no es válida.
    }
    GPDMA_ChannelStart(GPDMA_CH_6); // Deja el canal listo para recibir requests.
    TIM_Enable(LPC_TIM0); // Comienza a generar requests periódicas.
    while (1) {} // El DMA alterna el LED sin intervención del CPU.
}
