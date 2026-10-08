#include <stddef.h>
#include <stdint.h>

#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#define LED_P022 (1u << 22)

static const uint32_t patrones_led[] = {0u, LED_P022, 0u, LED_P022};
static uint32_t muestras_gpio[32];

static uint32_t control_gpio_salida(void)
{
    const size_t cantidad = sizeof(patrones_led) / sizeof(patrones_led[0]);
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de words del bloque.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una word por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una word por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Origen de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Destino de 32 bits.
    control |= GPDMA_DMACCxControl_SI; // Avanza por la tabla de patrones.
    return control;
}

static uint32_t control_gpio_entrada(void)
{
    const size_t cantidad = sizeof(muestras_gpio) / sizeof(muestras_gpio[0]);
    uint32_t control = 0u;
    control |= GPDMA_DMACCxControl_TransferSize(cantidad); // Cantidad de muestras.
    control |= GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1); // Lee una word por request.
    control |= GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1); // Escribe una word por request.
    control |= GPDMA_DMACCxControl_SWidth(GPDMA_WORD); // Origen de 32 bits.
    control |= GPDMA_DMACCxControl_DWidth(GPDMA_WORD); // Destino de 32 bits.
    control |= GPDMA_DMACCxControl_DI; // Avanza por el buffer de muestras.
    return control;
}

static void config_pin_led(void)
{
    PINSEL_CFG_T led;
    led.port = PORT_0; // Puerto 0.
    led.pin = PIN_22; // Pin del LED.
    led.func = PINSEL_FUNC_00; // Función GPIO.
    led.mode = PINSEL_PULLUP; // Habilita el pull-up.
    led.openDrain = DISABLE; // Salida push-pull.

    PINSEL_ConfigPin(&led);
    GPIO_SetDir(PORT_0, LED_P022, GPIO_OUTPUT);
}

static void config_pin_entrada(void)
{
    PINSEL_CFG_T entrada;
    entrada.port = PORT_0; // Puerto 0.
    entrada.pin = PIN_10; // Pin que se muestrea.
    entrada.func = PINSEL_FUNC_00; // Función GPIO.
    entrada.mode = PINSEL_PULLUP; // Pulsador activo en bajo.
    entrada.openDrain = DISABLE; // Sin drenador abierto.
    PINSEL_ConfigPin(&entrada);
    GPIO_SetDir(PORT_0, 1u << 10, GPIO_INPUT);
}

// GPIO no genera requests. MAT0.0/MAT0.1 se usan solamente como reloj.
// La request de MAT y la dirección FIOPIN se configuran por separado.
static void config_timer0_match_periodico(TIM_MATCH_CH match)
{
    const uint32_t periodo_us = 250000u;
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

static GPDMA_LLI_T lli_gpio_salida_anillo;

// Reproduce estados completos de GPIO0, una word por MAT0.0.
void config_dma_gpio_salida_periodica(void)
{
    const uint32_t *patrones = patrones_led;
    const uint32_t control = control_gpio_salida();
    LPC_GPDMACH_TypeDef *canal = LPC_GPDMACH6;

    config_pin_led();
    config_timer0_match_periodico(TIM_MATCH_0);
    LPC_SC->DMAREQSEL |= (1u << 0); // Línea 8: MAT0.0, no UART0 Tx.

    lli_gpio_salida_anillo.srcAddr = (uint32_t)(uintptr_t)patrones; // Reinicia la tabla de estados.
    lli_gpio_salida_anillo.dstAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Escribe el puerto completo.
    lli_gpio_salida_anillo.nextLLI = (uint32_t)(uintptr_t)&lli_gpio_salida_anillo; // Repite en forma circular.
    lli_gpio_salida_anillo.control = control; // Origen avanza; GPIO queda fijo.

    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_6); // Limpia error anterior.
    canal->DMACCSrcAddr = (uint32_t)(uintptr_t)patrones; // Tabla de estados.
    canal->DMACCDestAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO0.
    canal->DMACCLLI = (uint32_t)(uintptr_t)&lli_gpio_salida_anillo; // Repite la tabla.
    canal->DMACCControl = control; // Origen incremental y destino fijo.
    canal->DMACCConfig =
        GPDMA_DMACCxConfig_TransferType(GPDMA_M2P) | // Memoria a periférico.
        GPDMA_DMACCxConfig_DestPeripheral(8u); // MAT0.0 genera la request.
}

// Toma N snapshots de GPIO0, uno por MAT0.1.
void config_dma_gpio_entrada_periodica(void)
{
    uint32_t *muestras = muestras_gpio;
    const uint32_t control = control_gpio_entrada();
    LPC_GPDMACH_TypeDef *canal = LPC_GPDMACH0;

    config_pin_entrada();
    config_timer0_match_periodico(TIM_MATCH_1);
    LPC_SC->DMAREQSEL |= (1u << 1); // Línea 9: MAT0.1, no UART0 Rx.

    LPC_GPDMA->DMACIntTCClear = GPDMA_ChannelBit(GPDMA_CH_0); // Limpia TC anterior.
    LPC_GPDMA->DMACIntErrClr = GPDMA_ChannelBit(GPDMA_CH_0); // Limpia error anterior.
    canal->DMACCSrcAddr = (uint32_t)(uintptr_t)&LPC_GPIO0->FIOPIN; // Puerto GPIO0.
    canal->DMACCDestAddr = (uint32_t)(uintptr_t)muestras; // Buffer de muestras.
    canal->DMACCLLI = 0u; // Transferencia finita.
    canal->DMACCControl = control; // Origen fijo y destino incremental.
    canal->DMACCConfig =
        GPDMA_DMACCxConfig_TransferType(GPDMA_P2M) | // Periférico a memoria.
        GPDMA_DMACCxConfig_SrcPeripheral(9u); // MAT0.1 genera la request.
}

int main(void)
{
    GPDMA_Init(); // Inicializa el controlador DMA.
    // Configura un patrón circular con un cambio cada 250 ms.
    config_dma_gpio_salida_periodica(); // Configura MAT0.0 y el canal 6.
    GPDMA_ChannelStart(GPDMA_CH_6); // Deja el canal listo para recibir requests.
    TIM_Enable(LPC_TIM0); // Comienza a generar requests periódicas.
    while (1) {} // El DMA alterna el LED sin intervención del CPU.
}
