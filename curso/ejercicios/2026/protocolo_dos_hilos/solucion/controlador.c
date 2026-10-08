/* ============================================================================
 * Protocolo digital de dos hilos - controlador
 *
 * P2.11: CLK, salida push-pull
 * P2.12: DATA, solamente se fuerza a 0 o se libera como entrada
 * P0.22: indica si la ultima trama fue aceptada
 *
 * Timer0 interrumpe cada 500 us. La ISR avanza exactamente una fase de la
 * maquina, sin delays ni espera activa.
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define PIN_CLK          (1u << 11)  /* P2.11 */
#define PIN_DATA         (1u << 12)  /* P2.12 */
#define PIN_LED_ESTADO   (1u << 22)  /* P0.22 */

#define MEDIO_BIT_US     500u
#define ESPERA_TRAMAS    3000u       /* 3000 medios bits = 1,5 s */

typedef enum {
    CTRL_REPOSO,
    CTRL_INICIO,
    CTRL_BIT_BAJO,
    CTRL_BIT_ALTO,
    CTRL_ACK_BAJO,
    CTRL_ACK_ALTO,
    CTRL_ACK_FIN_BAJO,
    CTRL_FIN_CLK_ALTO
} estado_controlador_t;

typedef struct {
    uint8_t comando;
    uint8_t dato;
} operacion_t;

static const operacion_t demostracion[] = {
    {0x10u, 0x01u},
    {0x10u, 0x02u},
    {0x10u, 0x04u},
    {0x10u, 0x08u},
    {0x20u, 25u},        /* alternar P0.22 cada 250 ms */
    {0x20u, 100u}        /* alternar P0.22 cada 1 s */
};

static volatile estado_controlador_t estado = CTRL_REPOSO;
static volatile uint32_t medios_bits = 0u;
static volatile uint16_t trama_tx = 0u;
static volatile uint8_t posicion = 0u;
static volatile uint8_t ack_recibido = 1u;
static volatile uint8_t transferencia_lista = 0u;

static void clk_bajo(void)
{
    LPC_GPIO2->FIOCLR = PIN_CLK;
}

static void clk_alto(void)
{
    LPC_GPIO2->FIOSET = PIN_CLK;
}

/* Nunca se fuerza DATA a uno. Primero se deja el latch en cero y despues el
 * pin se hace salida. */
static void data_bajo(void)
{
    LPC_GPIO2->FIOCLR = PIN_DATA;
    LPC_GPIO2->FIODIR |= PIN_DATA;
}

static void data_liberar(void)
{
    LPC_GPIO2->FIODIR &= ~PIN_DATA;
}

static uint8_t data_leer(void)
{
    return (LPC_GPIO2->FIOPIN & PIN_DATA) ? 1u : 0u;
}

static void colocar_bit(uint8_t indice)
{
    uint8_t bit = (uint8_t)((trama_tx >> (15u - indice)) & 1u);

    if (bit != 0u) {
        data_liberar();
    } else {
        data_bajo();
    }
}

void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = (1u << 0);      /* MR0, write-1-to-clear */
    medios_bits++;

    switch (estado) {
    case CTRL_REPOSO:
        break;

    case CTRL_INICIO:
        /* DATA ya esta baja con CLK alto. Ahora comienza el primer nivel
         * bajo y se prepara el primer bit. */
        clk_bajo();
        posicion = 0u;
        colocar_bit(posicion);
        estado = CTRL_BIT_BAJO;
        break;

    case CTRL_BIT_BAJO:
        /* El dato ya tuvo medio periodo de establecimiento. El flanco
         * ascendente hace que el periferico lo muestree. */
        clk_alto();
        estado = CTRL_BIT_ALTO;
        break;

    case CTRL_BIT_ALTO:
        clk_bajo();
        posicion++;

        if (posicion < 16u) {
            colocar_bit(posicion);
            estado = CTRL_BIT_BAJO;
        } else {
            /* El periferico toma el control de DATA para responder. La
             * liberacion ocurre con CLK bajo. */
            data_liberar();
            estado = CTRL_ACK_BAJO;
        }
        break;

    case CTRL_ACK_BAJO:
        clk_alto();
        ack_recibido = data_leer();       /* 0 = aceptada, 1 = rechazada */
        estado = CTRL_ACK_ALTO;
        break;

    case CTRL_ACK_ALTO:
        /* Se baja CLK para que el periferico pueda retirar su respuesta. El
         * controlador vuelve a llevar DATA a cero para preparar el fin. */
        clk_bajo();
        data_bajo();
        estado = CTRL_ACK_FIN_BAJO;
        break;

    case CTRL_ACK_FIN_BAJO:
        clk_alto();
        estado = CTRL_FIN_CLK_ALTO;
        break;

    case CTRL_FIN_CLK_ALTO:
        /* Flanco ascendente de DATA mientras CLK esta alto: fin. */
        data_liberar();
        estado = CTRL_REPOSO;
        transferencia_lista = 1u;
        break;
    }
}

static void gpio_inicializar(void)
{
    /* P2.11 y P2.12 como GPIO: PINSEL4 bits 23:22 y 25:24 en 00. */
    LPC_PINCON->PINSEL4 &= ~((3u << 22) | (3u << 24));

    /* Sin pulls internos. DATA necesita el pull-up externo de 4,7 kohm. */
    LPC_PINCON->PINMODE4 &= ~((3u << 22) | (3u << 24));
    LPC_PINCON->PINMODE4 |=  ((2u << 22) | (2u << 24));

    LPC_GPIO2->FIOCLR = PIN_CLK | PIN_DATA;
    LPC_GPIO2->FIODIR |= PIN_CLK;
    data_liberar();
    clk_alto();

    /* LED de estado P0.22. */
    LPC_PINCON->PINSEL1 &= ~(3u << 12);
    LPC_GPIO0->FIODIR |= PIN_LED_ESTADO;
    LPC_GPIO0->FIOCLR = PIN_LED_ESTADO;
}

static void timer0_inicializar(void)
{
    LPC_SC->PCONP |= (1u << 1);          /* encender Timer0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);      /* PCLK = CCLK/4 = 25 MHz */

    LPC_TIM0->TCR = (1u << 1);
    LPC_TIM0->CTCR = 0u;
    LPC_TIM0->PR = 24u;                  /* un tick de TC = 1 us */
    LPC_TIM0->MR0 = MEDIO_BIT_US - 1u;
    LPC_TIM0->MCR = (1u << 0) | (1u << 1); /* interrumpir y resetear */
    LPC_TIM0->IR = 0x3Fu;

    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = (1u << 0);
}

/* Retorna 1 si pudo iniciar la transferencia y 0 si el enlace sigue ocupado. */
static uint8_t protocolo_iniciar(uint8_t comando, uint8_t dato)
{
    if (estado != CTRL_REPOSO) {
        return 0u;
    }

    /* Reiniciar la fase garantiza que el inicio dure 500 us completos,
     * independientemente del instante en que main llame a esta funcion. */
    LPC_TIM0->TCR = (1u << 1);
    LPC_TIM0->IR = (1u << 0);
    NVIC_ClearPendingIRQ(TIMER0_IRQn);

    trama_tx = (uint16_t)(((uint16_t)comando << 8) | dato);
    transferencia_lista = 0u;
    ack_recibido = 1u;

    /* Condicion de inicio: DATA baja mientras CLK permanece alto. */
    clk_alto();
    data_bajo();
    estado = CTRL_INICIO;
    LPC_TIM0->TCR = (1u << 0);
    return 1u;
}

int main(void)
{
    uint32_t instante_envio;
    uint8_t indice_demo = 0u;

    gpio_inicializar();
    timer0_inicializar();
    instante_envio = medios_bits;

    while (1) {
        uint32_t ahora = medios_bits;

        if (transferencia_lista != 0u) {
            transferencia_lista = 0u;

            if (ack_recibido == 0u) {
                LPC_GPIO0->FIOSET = PIN_LED_ESTADO;
            } else {
                LPC_GPIO0->FIOCLR = PIN_LED_ESTADO;
            }
        }

        if ((estado == CTRL_REPOSO) &&
            ((ahora - instante_envio) >= ESPERA_TRAMAS)) {
            const operacion_t *op = &demostracion[indice_demo];

            if (protocolo_iniciar(op->comando, op->dato) != 0u) {
                instante_envio = ahora;
                indice_demo++;
                if (indice_demo >= (uint8_t)(sizeof(demostracion) /
                                              sizeof(demostracion[0]))) {
                    indice_demo = 0u;
                }
            }
        }
    }
}

