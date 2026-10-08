/* ============================================================================
 * Protocolo digital de dos hilos - periferico
 *
 * P2.11: CLK, entrada con interrupciones GPIO por ambos flancos
 * P2.12: DATA, entrada salvo durante la confirmacion
 * P0.18..P0.21: salidas del comando 0x10
 * P0.22: salida temporizada del comando 0x20
 *
 * Timer1 produce una base de tiempo de 1 ms. La recepcion ocurre en la
 * interrupcion GPIO compartida EINT3_IRQHandler.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define PIN_CLK              (1u << 11)  /* P2.11 */
#define PIN_DATA             (1u << 12)  /* P2.12 */

#define SALIDAS_COMANDO      (0x0Fu << 18) /* P0.18 a P0.21 */
#define PIN_LED_TIEMPO       (1u << 22)    /* P0.22 */

#define TIMEOUT_MS           30u

typedef enum {
    PERIF_ESPERAR_INICIO,
    PERIF_RECIBIR_COMANDO,
    PERIF_RECIBIR_DATO,
    PERIF_ESPERAR_ACK_BAJO,
    PERIF_ACK_ACTIVO,
    PERIF_ESPERAR_RETIRO_ACK,
    PERIF_ESPERAR_FIN
} estado_periferico_t;

static volatile uint32_t milisegundos = 0u;
static volatile uint32_t ultima_actividad = 0u;
static volatile estado_periferico_t estado = PERIF_ESPERAR_INICIO;

static volatile uint8_t comando_rx = 0u;
static volatile uint8_t dato_rx = 0u;
static volatile uint8_t cantidad_bits = 0u;
static volatile uint8_t trama_valida = 0u;

static volatile uint8_t comando_pendiente = 0u;
static volatile uint8_t dato_pendiente = 0u;
static volatile uint8_t hay_comando = 0u;

static uint32_t periodo_toggle_ms = 0u;
static uint32_t ultimo_toggle_ms = 0u;

static uint8_t clk_leer(void)
{
    return (LPC_GPIO2->FIOPIN & PIN_CLK) ? 1u : 0u;
}

static uint8_t data_leer(void)
{
    return (LPC_GPIO2->FIOPIN & PIN_DATA) ? 1u : 0u;
}

static void data_bajo(void)
{
    LPC_GPIO2->FIOCLR = PIN_DATA;
    LPC_GPIO2->FIODIR |= PIN_DATA;
}

static void data_liberar(void)
{
    LPC_GPIO2->FIODIR &= ~PIN_DATA;
}

static uint8_t operacion_valida(uint8_t comando, uint8_t dato)
{
    if (comando == 0x10u) {
        return (dato <= 0x0Fu) ? 1u : 0u;
    }

    if (comando == 0x20u) {
        return ((dato >= 1u) && (dato <= 200u)) ? 1u : 0u;
    }

    return 0u;
}

void TIMER1_IRQHandler(void)
{
    LPC_TIM1->IR = (1u << 0);
    milisegundos++;
}

static void recibir_bit_en_flanco_ascendente(void)
{
    uint8_t bit = data_leer();

    if (estado == PERIF_RECIBIR_COMANDO) {
        comando_rx = (uint8_t)((comando_rx << 1) | bit);
        cantidad_bits++;

        if (cantidad_bits == 8u) {
            cantidad_bits = 0u;
            estado = PERIF_RECIBIR_DATO;
        }
    } else if (estado == PERIF_RECIBIR_DATO) {
        dato_rx = (uint8_t)((dato_rx << 1) | bit);
        cantidad_bits++;

        if (cantidad_bits == 8u) {
            cantidad_bits = 0u;
            trama_valida = operacion_valida(comando_rx, dato_rx);
            estado = PERIF_ESPERAR_ACK_BAJO;
        }
    } else if (estado == PERIF_ACK_ACTIVO) {
        /* El controlador esta leyendo la confirmacion en este flanco. */
        estado = PERIF_ESPERAR_RETIRO_ACK;
    }
}

static void atender_flanco_descendente_clk(void)
{
    if (estado == PERIF_ESPERAR_ACK_BAJO) {
        if (trama_valida != 0u) {
            data_bajo();             /* confirmacion = 0 */
        } else {
            data_liberar();          /* rechazo = 1 por el pull-up */
        }
        estado = PERIF_ACK_ACTIVO;
    } else if (estado == PERIF_ESPERAR_RETIRO_ACK) {
        data_liberar();
        estado = PERIF_ESPERAR_FIN;
    }
}

static void atender_flanco_data(uint8_t ascendente, uint8_t descendente)
{
    if (clk_leer() == 0u) {
        /* Con CLK bajo es un dato normal o un cambio de la confirmacion. */
        return;
    }

    if ((descendente != 0u) && (estado == PERIF_ESPERAR_INICIO)) {
        comando_rx = 0u;
        dato_rx = 0u;
        cantidad_bits = 0u;
        trama_valida = 0u;
        ultima_actividad = milisegundos;
        estado = PERIF_RECIBIR_COMANDO;
    } else if ((ascendente != 0u) && (estado == PERIF_ESPERAR_FIN)) {
        if (trama_valida != 0u) {
            comando_pendiente = comando_rx;
            dato_pendiente = dato_rx;
            hay_comando = 1u;
        }
        estado = PERIF_ESPERAR_INICIO;
    }
}

void EINT3_IRQHandler(void)
{
    uint32_t clk_r = LPC_GPIOINT->IO2IntStatR & PIN_CLK;
    uint32_t clk_f = LPC_GPIOINT->IO2IntStatF & PIN_CLK;
    uint32_t data_r = LPC_GPIOINT->IO2IntStatR & PIN_DATA;
    uint32_t data_f = LPC_GPIOINT->IO2IntStatF & PIN_DATA;

    /* Se limpian primero las banderas que se capturaron. Un cambio local de
     * DATA durante el handler puede generar una bandera nueva, que se
     * atendera en la siguiente entrada y sera ignorada si CLK esta bajo. */
    LPC_GPIOINT->IO2IntClr = clk_r | clk_f | data_r | data_f;

    if (data_r || data_f) {
        atender_flanco_data((data_r != 0u) ? 1u : 0u,
                            (data_f != 0u) ? 1u : 0u);
    }

    if (clk_r) {
        ultima_actividad = milisegundos;
        recibir_bit_en_flanco_ascendente();
    }

    if (clk_f) {
        ultima_actividad = milisegundos;
        atender_flanco_descendente_clk();
    }
}

static void gpio_inicializar(void)
{
    /* P2.11 y P2.12 como GPIO, no como EINT1/EINT2. */
    LPC_PINCON->PINSEL4 &= ~((3u << 22) | (3u << 24));

    /* Sin pulls internos: DATA tiene pull-up externo y CLK lo maneja la otra
     * placa. */
    LPC_PINCON->PINMODE4 &= ~((3u << 22) | (3u << 24));
    LPC_PINCON->PINMODE4 |=  ((2u << 22) | (2u << 24));

    LPC_GPIO2->FIODIR &= ~(PIN_CLK | PIN_DATA);
    LPC_GPIO2->FIOCLR = PIN_DATA;

    /* P0.18 a P0.22 como GPIO. PINSEL1: bits 4 a 13. */
    LPC_PINCON->PINSEL1 &= ~(0x3FFu << 4);
    LPC_GPIO0->FIODIR |= SALIDAS_COMANDO | PIN_LED_TIEMPO;
    LPC_GPIO0->FIOCLR = SALIDAS_COMANDO | PIN_LED_TIEMPO;

    /* P2.11 y P2.12 interrumpen por ambos flancos y comparten EINT3_IRQn. */
    LPC_GPIOINT->IO2IntEnR &= ~(PIN_CLK | PIN_DATA);
    LPC_GPIOINT->IO2IntEnF &= ~(PIN_CLK | PIN_DATA);
    LPC_GPIOINT->IO2IntClr = PIN_CLK | PIN_DATA;
    LPC_GPIOINT->IO2IntEnR |= PIN_CLK | PIN_DATA;
    LPC_GPIOINT->IO2IntEnF |= PIN_CLK | PIN_DATA;

    NVIC_ClearPendingIRQ(EINT3_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void timer1_inicializar(void)
{
    LPC_SC->PCONP |= (1u << 2);          /* encender Timer1 */
    LPC_SC->PCLKSEL0 &= ~(3u << 4);      /* PCLK = CCLK/4 = 25 MHz */

    LPC_TIM1->TCR = (1u << 1);
    LPC_TIM1->CTCR = 0u;
    LPC_TIM1->PR = 24u;                  /* un tick de TC = 1 us */
    LPC_TIM1->MR0 = 1000u - 1u;          /* un match cada 1 ms */
    LPC_TIM1->MCR = (1u << 0) | (1u << 1);
    LPC_TIM1->IR = 0x3Fu;

    NVIC_ClearPendingIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
    LPC_TIM1->TCR = (1u << 0);
}

static void ejecutar_comando(uint8_t comando, uint8_t dato)
{
    if (comando == 0x10u) {
        LPC_GPIO0->FIOCLR = SALIDAS_COMANDO;
        LPC_GPIO0->FIOSET = ((uint32_t)(dato & 0x0Fu) << 18);
    } else if (comando == 0x20u) {
        periodo_toggle_ms = (uint32_t)dato * 10u;
        ultimo_toggle_ms = milisegundos;
        LPC_GPIO0->FIOCLR = PIN_LED_TIEMPO;
    }
}

static void recuperar_por_timeout(void)
{
    data_liberar();
    comando_rx = 0u;
    dato_rx = 0u;
    cantidad_bits = 0u;
    trama_valida = 0u;
    estado = PERIF_ESPERAR_INICIO;
}

int main(void)
{
    gpio_inicializar();
    timer1_inicializar();

    while (1) {
        uint32_t ahora = milisegundos;

        if ((estado != PERIF_ESPERAR_INICIO) &&
            ((ahora - ultima_actividad) >= TIMEOUT_MS)) {
            recuperar_por_timeout();
        }

        if (hay_comando != 0u) {
            uint8_t comando = comando_pendiente;
            uint8_t dato = dato_pendiente;

            hay_comando = 0u;
            ejecutar_comando(comando, dato);
        }

        if ((periodo_toggle_ms != 0u) &&
            ((ahora - ultimo_toggle_ms) >= periodo_toggle_ms)) {
            ultimo_toggle_ms = ahora;
            LPC_GPIO0->FIOPIN ^= PIN_LED_TIEMPO;
        }
    }
}

