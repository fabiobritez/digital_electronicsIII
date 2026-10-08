/* ============================================================================
 * Deserializador serial par/impar, con Timer0 en lugar de SysTick
 * ============================================================================
 *
 *
 * Que hace:
 *
 *   - Cada T = 10 ms el Timer0 interrumpe y se toma una muestra de P0.0.
 *   - Los bits de indice par (b0, b2, b4, ...) salen por P0.2.
 *   - Los bits de indice impar (b1, b3, b5, ...) salen por P0.1.
 *     Cada salida cambia cada 2 bits, asi que su periodo efectivo es 2T.
 *   - Los primeros 16 bits son solo de arranque (MODO_STARTUP): hasta que el
 *     buffer no tenga 16 bits validos no se compara nada ni se sacan datos.
 *   - Se guardan los ultimos 16 bits en un "buffer deslizante" y a partir del
 *     bit 16, en cada bit nuevo, se compara con dos patrones de control:
 *
 *         0xF628 -> modo CERO:     las dos salidas quedan en 0.
 *         0x28F6 -> modo REPETIR:  se vuelve a sacar el ultimo patron valido.
 *
 *     Cualquier otro valor de los ultimos 16 bits vuelve al modo NORMAL.
 *
 * Pinout:
 *
 *     P0.0 <---- entrada serial (un bit cada T = 10 ms)
 *     P0.1 ----> bits impares
 *     P0.2 ----> bits pares
 * ========================================================================== */

#include <stdint.h>
#include "LPC17xx.h"

#define PIN_ENTRADA        (1u << 0)   /* P0.0 */
#define PIN_SALIDA_IMPAR   (1u << 1)   /* P0.1 */
#define PIN_SALIDA_PAR     (1u << 2)   /* P0.2 */

#define PATRON_CERO        0xF628u
#define PATRON_REPETIR     0x28F6u

/* Periodo T de la entrada serial, en microsegundos (10 ms). */
#define PERIODO_T_US       10000u

typedef enum {
    MODO_STARTUP,    /* arranque: hay menos de 16 bits en el buffer         */
    MODO_NORMAL,     /* cada bit va a su salida segun la paridad del indice */
    MODO_CERO,       /* las dos salidas forzadas a 0                        */
    MODO_REPETIR     /* se repite ultimo_patron, bit por bit                */
} modo_t;

/* --- lo que escribe la ISR y lee main: volatile --- */
static volatile uint8_t bit_muestreado = 0u;
static volatile uint8_t hay_muestra    = 0u;

/* --- estado del deserializador (solo lo toca main) --- */
static uint16_t buffer         = 0u;  /* ultimos 16 bits recibidos          */
static uint8_t  bits_recibidos = 0u;  /* solo se usa durante el arranque    */
static uint8_t  indice_bit     = 0u;  /* 0 = bit par, 1 = bit impar         */

static modo_t   modo = MODO_STARTUP;

static uint16_t ultimo_patron  = 0u;  /* ultimo valor "normal" del buffer   */
static uint8_t  hay_patron     = 0u;  /* 1 cuando ultimo_patron es valido   */
static uint8_t  pos_repeticion = 0u;  /* que bit de ultimo_patron toca      */


void TIMER0_IRQHandler(void)
{
    /* IR es write-1-to-clear: escribir un 1 limpia la bandera de MR0. */
    LPC_TIM0->IR = (1u << 0);

    /* La ISR hace lo minimo: leer el pin en el instante justo y avisar.
     * Todo el procesamiento queda en main, donde es facil de seguir. */
    bit_muestreado = (LPC_GPIO0->FIOPIN & PIN_ENTRADA) ? 1u : 0u;
    hay_muestra = 1u;
}


static void gpio_inicializar(void)
{
    /* P0.0, P0.1 y P0.2: funcion 00 = GPIO.
     * En PINSEL0 ocupan los bits [1:0], [3:2] y [5:4]. */
    LPC_PINCON->PINSEL0 &= ~(0x3Fu << 0);

    /* P0.0 sin pull-up ni pull-down (modo 10) porque la senal la maneja
     * el generador externo. Si se prueba con un pulsador a GND, conviene
     * dejar el pull-up por defecto (modo 00) y borrar estas dos lineas. */
    LPC_PINCON->PINMODE0 &= ~(3u << 0);
    LPC_PINCON->PINMODE0 |=  (2u << 0);

    LPC_GPIO0->FIODIR &= ~PIN_ENTRADA;
    LPC_GPIO0->FIODIR |=  (PIN_SALIDA_PAR | PIN_SALIDA_IMPAR);
    LPC_GPIO0->FIOCLR  =  (PIN_SALIDA_PAR | PIN_SALIDA_IMPAR);
}


static void timer0_inicializar(void)
{
    /* 1) Encender Timer0 y fijar PCLK = CCLK/4 = 25 MHz. */
    LPC_SC->PCONP |= (1u << 1);          /* PCTIM0 */
    LPC_SC->PCLKSEL0 &= ~(3u << 2);      /* PCLK_Timer0 = CCLK/4 */

    /* 2) Frenar y resetear antes de configurar. */
    LPC_TIM0->TCR = (1u << 1);
    LPC_TIM0->CTCR = 0u;                 /* modo timer: cuenta PCLK */

    /* 3) 25 MHz / (PR + 1) = 1 MHz: cada incremento de TC dura 1 us. */
    LPC_TIM0->PR = 24u;

    /* 4) Un match cada PERIODO_T_US microsegundos.
     *    Con reset-on-match el periodo tiene MR0 + 1 ticks. */
    LPC_TIM0->MR0 = PERIODO_T_US - 1u; // 10 ms

    /* 5) MCR: MR0I = interrumpir y MR0R = resetear TC en cada match. */
    LPC_TIM0->MCR = (1u << 0) | (1u << 1);
    LPC_TIM0->IR = 0x3Fu;                /* limpiar banderas anteriores */

    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);

    LPC_TIM0->TCR = (1u << 0);           /* arrancar */
}


/* Manda un bit a la salida que le corresponde segun indice_bit. */
static void escribir_salida(uint8_t bit)
{
    uint32_t pin = (indice_bit == 0u) ? PIN_SALIDA_PAR : PIN_SALIDA_IMPAR;

    if (bit) {
        LPC_GPIO0->FIOSET = pin;
    } else {
        LPC_GPIO0->FIOCLR = pin;
    }
}


/* Decide el modo mirando los ultimos 16 bits recibidos. */
static void actualizar_modo(void)
{
    if (buffer == PATRON_CERO) {
        modo = MODO_CERO;
    } else if (buffer == PATRON_REPETIR) {
        /* Sin un patron guardado no hay nada que repetir: se sigue normal. */
        if (hay_patron) {
            modo = MODO_REPETIR;
            pos_repeticion = 0u;
        }
    } else {
        /* Cualquier otro valor saca al sistema del modo especial y pasa a
         * ser el "ultimo patron valido" para una futura repeticion. */
        modo = MODO_NORMAL;
        ultimo_patron = buffer;
        hay_patron = 1u;
    }
}


static void procesar_bit(uint8_t bit)
{
    uint8_t bit_repetido;

    /* 1) Entra el bit nuevo por la derecha y el mas viejo se cae por la
     *    izquierda: eso es el buffer deslizante de 16 bits. */
    buffer = (uint16_t)((buffer << 1) | bit);

    /* 2) MODO_STARTUP: con los primeros 15 bits el buffer todavia tiene
     *    basura en los lugares que no se llenaron, asi que no se compara con
     *    los patrones ni se saca nada por las salidas. Con el bit 16 el
     *    buffer ya esta completo: se sale del arranque y ese mismo bit se
     *    procesa normalmente (no se pierde ningun bit). */
    if (modo == MODO_STARTUP) {
        bits_recibidos++;

        if (bits_recibidos < 16u) {
            /* Igual se lleva la cuenta de la paridad, para que el bit 16
             * caiga en la salida que le corresponde por su indice. */
            indice_bit ^= 1u;  // 0 -> 1 y si era 1->0
            return;
        }

        modo = MODO_NORMAL;
    }

    /* 3) Ya hay 16 bits validos: se compara en cada bit nuevo y se saca. */
    actualizar_modo();

    switch (modo) {
    case MODO_NORMAL:
        escribir_salida(bit);
        break;

    case MODO_CERO:
        LPC_GPIO0->FIOCLR = ((1u << 2) | PIN_SALIDA_IMPAR); // las dos salidas forzadas a 0
        break;

    case MODO_REPETIR:
        /* Se sacan los bits de ultimo_patron del mas significativo al
         * menos significativo, y se vuelve a empezar cada 16 bits. */
        bit_repetido = (uint8_t)((ultimo_patron >> (15u - pos_repeticion)) & 1u);
        escribir_salida(bit_repetido);
        pos_repeticion = (uint8_t)((pos_repeticion + 1u) & 0x0Fu);
        break;

    case MODO_STARTUP:
        /* Imposible aca: el if de arriba ya salio del arranque. */
        break;
    }

    /* 4) El proximo bit tiene la paridad opuesta: 0 -> 1 -> 0 -> 1 ... */
    indice_bit ^= 1u; // 0 -> 1 y si era 1->0
}


int main(void)
{
    gpio_inicializar();
    timer0_inicializar();

    while (1) {
        if (hay_muestra) {
            hay_muestra = 0u;            /* se consume la muestra */
            procesar_bit(bit_muestreado);
        }

    }
}
