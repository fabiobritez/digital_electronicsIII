/* ============================================================================
 * Ejemplo 2: juego de reflejos con Timer0, programado a nivel de registros
 * ============================================================================
 *
 * 1. El alumno presiona el boton para iniciar.
 * 2. Tras una espera pseudoaleatoria de 2 a 5 segundos se enciende el LED.
 * 3. El alumno presiona nuevamente: el Timer0 mide el tiempo de reaccion.
 * 4. El LED muestra el resultado: un destello por cada 100 ms (maximo 9).
 *
 * Si se presiona antes de que se encienda el LED, hubo partida adelantada y
 * se muestran tres destellos rapidos.
 *
 * Hardware:
 *     - LED de la placa: P0.22, activo en alto.
 *     - Pulsador entre P2.10 y GND. Se usa el pull-up interno.
 *
 * Plataforma: LPCXpresso LPC1769, CCLK = 100 MHz y PCLK_Timer0 = CCLK/4.
 * Compilar la plantilla con USE_CMSIS=1 para que SystemInit deje esos clocks.
 * CMSIS se usa solo para nombres de registros/intrinsics: no se usa TIM_Init.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define LED             (1u << 22)
#define BOTON           (1u << 10)

#define ANTIRREBOTE_MS  30u
#define ESPERA_MIN_MS   2000u
#define ESPERA_MAX_MS   5000u
#define RESULTADO_ERROR UINT32_MAX

typedef enum {
    JUEGO_LISTO,
    JUEGO_ESPERANDO_SENAL,
    JUEGO_MIDIENDO,
    JUEGO_MOSTRANDO
} estado_juego_t;

/* Estas variables se comparten con la interrupcion. */
static volatile uint8_t senal_generada = 0u;
static volatile uint32_t instante_senal_ms = 0u;

/* Resultado de la ultima partida. Es global para mirarlo desde el debugger.
 * UINT32_MAX significa "se adelanto". */
volatile uint32_t tiempo_reaccion_ms = 0u;


void TIMER0_IRQHandler(void)
{
    if ((LPC_TIM0->IR & (1u << 0)) != 0u) {
        LPC_TIM0->IR = (1u << 0);          /* write-1-to-clear */
        LPC_TIM0->MCR &= ~(1u << 0);       /* match one-shot: no repetir IRQ */

        /* MR0 guarda el instante exacto programado. Usarlo evita sumar a la
         * medicion la latencia de entrada a la ISR. */
        instante_senal_ms = LPC_TIM0->MR0;
        senal_generada = 1u;
        LPC_GPIO0->FIOSET = LED;
    }
}


static void hardware_inicializar(void)
{
    /* P0.22 como GPIO. */
    LPC_PINCON->PINSEL1 &= ~(3u << 12);
    LPC_GPIO0->FIODIR |= LED;
    LPC_GPIO0->FIOCLR = LED;

    /* P2.10 como GPIO de entrada, con pull-up interno (PINMODE = 00). */
    LPC_PINCON->PINSEL4 &= ~(3u << 20);
    LPC_PINCON->PINMODE4 &= ~(3u << 20);
    LPC_GPIO2->FIODIR &= ~BOTON;

    /* Timer0 libre, con un tick de 1 ms.
     * PCLK = 25 MHz; PR + 1 = 25000; 25000 / 25 MHz = 1 ms. */
    LPC_SC->PCONP |= (1u << 1);
    LPC_SC->PCLKSEL0 &= ~(3u << 2);

    LPC_TIM0->TCR = (1u << 1);             /* reset */
    LPC_TIM0->CTCR = 0u;                    /* modo timer */
    LPC_TIM0->PR = 25000u - 1u;
    LPC_TIM0->MCR = 0u;                     /* todavia sin match */
    LPC_TIM0->IR = 0x3Fu;

    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = (1u << 0);              /* cuenta libre desde ahora */
}


/* Devuelve 1 una sola vez por cada presion valida. En instante_pulsacion
 * entrega el momento del primer flanco, antes del antirrebote, para no sumar
 * 30 ms artificiales a la medicion. No bloquea el programa. */
static uint8_t boton_presionado(uint32_t ahora_ms,
                               uint32_t *instante_pulsacion)
{
    static uint8_t muestra_anterior = 1u;
    static uint8_t estado_estable = 1u;
    static uint32_t instante_del_cambio = 0u;
    uint8_t muestra = ((LPC_GPIO2->FIOPIN & BOTON) != 0u) ? 1u : 0u;

    if (muestra != muestra_anterior) {
        muestra_anterior = muestra;
        instante_del_cambio = ahora_ms;
    }

    if ((muestra != estado_estable) &&
        ((ahora_ms - instante_del_cambio) >= ANTIRREBOTE_MS)) {
        estado_estable = muestra;
        if (estado_estable == 0u) {
            *instante_pulsacion = instante_del_cambio;
            return 1u;
        }
    }

    return 0u;
}


static uint32_t espera_pseudoaleatoria(uint32_t ahora_ms)
{
    /* El instante humano de la pulsacion aporta variacion. No es azar
     * criptografico; solo evita que el alumno pueda memorizar la espera. */
    static uint32_t estado = 0x6D2B79F5u;
    estado ^= ahora_ms;
    estado ^= estado << 13;
    estado ^= estado >> 17;
    estado ^= estado << 5;

    return ESPERA_MIN_MS +
           (estado % (ESPERA_MAX_MS - ESPERA_MIN_MS + 1u));
}


static void programar_senal(uint32_t ahora_ms)
{
    uint32_t espera_ms = espera_pseudoaleatoria(ahora_ms);

    senal_generada = 0u;
    LPC_GPIO0->FIOCLR = LED;

    /* Timer0 queda libre. MR0 es un instante absoluto futuro; la resta y la
     * comparacion del hardware siguen siendo validas aun si TC desborda. */
    LPC_TIM0->MCR &= ~7u;                    /* deshabilitar canal MR0 */
    LPC_TIM0->MR0 = ahora_ms + espera_ms;
    LPC_TIM0->IR = (1u << 0);                /* borrar bandera anterior */
    LPC_TIM0->MCR |= (1u << 0);              /* MR0I, sin reset ni stop */
}


static void cancelar_senal(void)
{
    uint32_t primask = __get_PRIMASK();

    /* La seccion critica evita que un match simultaneo vuelva a encender el
     * LED justo mientras main esta cancelando la partida. */
    __disable_irq();
    LPC_TIM0->MCR &= ~(1u << 0);
    LPC_TIM0->IR = (1u << 0);
    senal_generada = 0u;
    __set_PRIMASK(primask);

    LPC_GPIO0->FIOCLR = LED;
}


static uint8_t destellos_del_resultado(uint32_t reaccion_ms)
{
    uint32_t cantidad = (reaccion_ms + 50u) / 100u; /* redondear a 100 ms */

    if (cantidad < 1u) {
        cantidad = 1u;
    }
    if (cantidad > 9u) {
        cantidad = 9u;
    }

    return (uint8_t)cantidad;
}


/* Comparacion de tiempos que tolera el desborde del contador de 32 bits,
 * siempre que el plazo este a menos de 2^31 ticks. */
static uint8_t plazo_cumplido(uint32_t ahora, uint32_t plazo)
{
    return ((int32_t)(ahora - plazo) >= 0) ? 1u : 0u;
}


int main(void)
{
    estado_juego_t estado = JUEGO_LISTO;
    uint8_t destellos_pendientes = 0u;
    uint8_t led_encendido = 0u;
    uint8_t partida_adelantada = 0u;
    uint32_t proximo_cambio_ms = 0u;

    hardware_inicializar();

    while (1) {
        uint32_t ahora_ms = LPC_TIM0->TC;
        uint32_t instante_pulsacion_ms = 0u;

        /* Procesar primero la senal. Si coincide con una pulsacion, esa
         * pulsacion cuenta como reaccion y no como partida adelantada. */
        if ((estado == JUEGO_ESPERANDO_SENAL) && (senal_generada != 0u)) {
            senal_generada = 0u;
            estado = JUEGO_MIDIENDO;
        }

        if (boton_presionado(ahora_ms, &instante_pulsacion_ms) != 0u) {
            switch (estado) {
            case JUEGO_LISTO:
                programar_senal(instante_pulsacion_ms);
                estado = JUEGO_ESPERANDO_SENAL;
                break;

            case JUEGO_ESPERANDO_SENAL:
                cancelar_senal();
                tiempo_reaccion_ms = RESULTADO_ERROR;
                partida_adelantada = 1u;
                destellos_pendientes = 3u;
                led_encendido = 0u;
                proximo_cambio_ms = ahora_ms + 500u;
                estado = JUEGO_MOSTRANDO;
                break;

            case JUEGO_MIDIENDO:
                tiempo_reaccion_ms =
                    instante_pulsacion_ms - instante_senal_ms;
                LPC_GPIO0->FIOCLR = LED;
                partida_adelantada = 0u;
                destellos_pendientes =
                    destellos_del_resultado(tiempo_reaccion_ms);
                led_encendido = 0u;
                proximo_cambio_ms = ahora_ms + 700u;
                estado = JUEGO_MOSTRANDO;
                break;

            case JUEGO_MOSTRANDO:
                /* Ignorar el boton mientras se presenta el resultado. */
                break;
            }
        }

        /* Secuencia no bloqueante para mostrar el resultado. */
        if ((estado == JUEGO_MOSTRANDO) &&
            (plazo_cumplido(ahora_ms, proximo_cambio_ms) != 0u)) {

            if (led_encendido != 0u) {
                LPC_GPIO0->FIOCLR = LED;
                led_encendido = 0u;
                destellos_pendientes--;
                proximo_cambio_ms = ahora_ms +
                    ((partida_adelantada != 0u) ? 100u : 200u);
            } else if (destellos_pendientes != 0u) {
                LPC_GPIO0->FIOSET = LED;
                led_encendido = 1u;
                proximo_cambio_ms = ahora_ms +
                    ((partida_adelantada != 0u) ? 100u : 150u);
            } else {
                /* Fin del resultado. Hace falta soltar y volver a presionar
                 * el boton para empezar otra partida. */
                estado = JUEGO_LISTO;
            }
        }
    }
}
