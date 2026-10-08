/* ============================================================================
 * Ejemplo 1: semaforo con Timer0, programado a nivel de registros
 * ============================================================================
 *
 * El Timer0 genera una interrupcion cada 1 segundo. El programa principal usa
 * ese "tic" para mantener un semaforo sin delays bloqueantes:
 *
 *     ROJO (4 s) -> VERDE (4 s) -> AMARILLO (1 s) -> ROJO ...
 *
 * Hardware (P0.22 ya tiene el LED de la placa; los otros necesitan un LED
 * y una resistencia serie, por ejemplo 330 ohm):
 *
 *     P0.22 ---------------------------- LED rojo de la placa
 *     P0.21 ---- resistencia ----|>|---- GND   LED amarillo
 *     P0.20 ---- resistencia ----|>|---- GND   LED verde
 *
 * Plataforma: LPCXpresso LPC1769, CCLK = 100 MHz y PCLK_Timer0 = CCLK/4.
 * Compilar la plantilla con USE_CMSIS=1 para que SystemInit deje esos clocks.
 * CMSIS se usa solo para los nombres de registros: no se usa el driver TIM.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define LED_ROJO       (1u << 22)
#define LED_AMARILLO   (1u << 21)
#define LED_VERDE      (1u << 20)
#define TODOS_LOS_LED  (LED_ROJO | LED_AMARILLO | LED_VERDE)

typedef enum {
    SEMAFORO_ROJO,
    SEMAFORO_VERDE,
    SEMAFORO_AMARILLO
} estado_semaforo_t;

/* La ISR escribe esta variable y main la lee: por eso debe ser volatile. */
static volatile uint32_t segundos = 0u;


void TIMER0_IRQHandler(void)
{
    /* IR es write-1-to-clear: escribir un 1 limpia la bandera de MR0. */
    LPC_TIM0->IR = (1u << 0);
    segundos++;
}


static void gpio_inicializar(void)
{
    /* P0.20, P0.21 y P0.22: funcion 00 = GPIO.
     * En PINSEL1 ocupan los bits [9:8], [11:10] y [13:12]. */
    LPC_PINCON->PINSEL1 &= ~(0x3Fu << 8);

    LPC_GPIO0->FIODIR |= TODOS_LOS_LED;
    LPC_GPIO0->FIOCLR = TODOS_LOS_LED;
}


static void timer0_inicializar(void)
{
    /* 1) Encender Timer0 y fijar PCLK = CCLK/4 = 25 MHz. */
    LPC_SC->PCONP |= (1u << 1);          /* PCTIM0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);      /* PCLK_Timer0 = CCLK/4 */

    /* Frenar y resetear antes de configurarlo. */
    LPC_TIM0->TCR = (1u << 1);
    LPC_TIM0->CTCR = 0u;                 /* modo timer: cuenta PCLK */

    /* 25 MHz / (PR + 1) = 1 MHz: cada incremento de TC dura 1 us. */
    LPC_TIM0->PR = 24u;

    /* Un match cada 1 000 000 us = 1 s.
     * Con reset-on-match el periodo tiene MR0 + 1 ticks. */
    LPC_TIM0->MR0 = 1000000u - 1u;

    /* MCR: MR0I = interrumpir y MR0R = resetear TC en cada match. */
    LPC_TIM0->MCR = (1u << 0) | (1u << 1);
    LPC_TIM0->IR = 0x3Fu;                /* limpiar banderas anteriores */

    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);

    LPC_TIM0->TCR = (1u << 0);           /* arrancar */
}


static void semaforo_mostrar(estado_semaforo_t estado)
{
    LPC_GPIO0->FIOCLR = TODOS_LOS_LED;

    switch (estado) {
    case SEMAFORO_ROJO:
        LPC_GPIO0->FIOSET = LED_ROJO;
        break;

    case SEMAFORO_VERDE:
        LPC_GPIO0->FIOSET = LED_VERDE;
        break;

    case SEMAFORO_AMARILLO:
        LPC_GPIO0->FIOSET = LED_AMARILLO;
        break;
    }
}


static uint32_t duracion_del_estado(estado_semaforo_t estado)
{
    switch (estado) {
    case SEMAFORO_ROJO:     return 4u;
    case SEMAFORO_VERDE:    return 4u;
    case SEMAFORO_AMARILLO: return 1u;
    }

    return 1u; /* solo evita un warning si se corrompiera el estado */
}


int main(void)
{
    estado_semaforo_t estado = SEMAFORO_ROJO;
    uint32_t inicio_del_estado = 0u;

    gpio_inicializar();
    semaforo_mostrar(estado);
    timer0_inicializar();

    while (1) {
        /* La resta sin signo sigue funcionando aunque segundos desborde. */
        uint32_t ahora = segundos;

        if ((ahora - inicio_del_estado) >= duracion_del_estado(estado)) {
            inicio_del_estado = ahora;

            switch (estado) {
            case SEMAFORO_ROJO:
                estado = SEMAFORO_VERDE;
                break;
            case SEMAFORO_VERDE:
                estado = SEMAFORO_AMARILLO;
                break;
            case SEMAFORO_AMARILLO:
                estado = SEMAFORO_ROJO;
                break;
            }

            semaforo_mostrar(estado);
        }

        /* No hay delay: main queda libre para agregar un boton, UART, ADC... */
    }
}
