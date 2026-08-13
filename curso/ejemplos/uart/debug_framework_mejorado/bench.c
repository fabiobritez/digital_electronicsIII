/*
 * Banco de pruebas del Debug Framework mejorado.
 *
 * Compilar desde plantilla/ con uno de estos backends:
 *
 *   DEBUG_BACKEND_BLOQUES
 *   DEBUG_BACKEND_IRQ
 *   DEBUG_BACKEND_DMA
 *
 * y con DEBUG_BAUD=115200 o DEBUG_BAUD=921600. Los resultados quedan en la
 * estructura global resultados_mejorado para leerlos mediante GDB o OpenOCD.
 */

#include <stdint.h>

#include "LPC17xx.h"
#include "debug_frmwrk_mejorado.h"

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define REPETICIONES       5u
#define REP_CONVERSION     1000u
#define LINEAS_CAUDAL      1000u
#define LINEAS_RAFAGA      200u
#define CICLOS_US          100u

#define TEXTO  "[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C\n"

static volatile uint32_t valor = 1234567u;
static volatile uint32_t sumidero;

struct resultados_debug_mejorado {
    uint32_t backend;
    uint32_t baud_configurado;
    uint32_t baud_real;
    uint32_t ciclos_conversion_original;
    uint32_t ciclos_conversion_mejorada;
    uint32_t ciclos_literal_48;
    uint32_t ciclos_numero_17;
    uint32_t bytes_caudal;
    uint32_t us_caudal;
    uint32_t bytes_por_segundo;
    uint32_t perdidos_caudal;
    uint32_t bytes_rafaga;
    uint32_t ciclos_rafaga;
    uint32_t pendientes_rafaga;
    uint32_t perdidos_rafaga;
    uint32_t terminado;
};

volatile struct resultados_debug_mejorado resultados_mejorado;


/* Reproduce el cálculo del framework de NXP, pero guarda los caracteres en un
 * buffer para separar la conversión de la espera de la UART. */
__attribute__((noinline))
static void conversion_original(char destino[10], uint32_t numero)
{
    uint8_t c1 = numero % 10u;
    uint8_t c2 = (numero / 10u) % 10u;
    uint8_t c3 = (numero / 100u) % 10u;
    uint8_t c4 = (numero / 1000u) % 10u;
    uint8_t c5 = (numero / 10000u) % 10u;
    uint8_t c6 = (numero / 100000u) % 10u;
    uint8_t c7 = (numero / 1000000u) % 10u;
    uint8_t c8 = (numero / 10000000u) % 10u;
    uint8_t c9 = (numero / 100000000u) % 10u;
    uint8_t c10 = numero / 1000000000u;

    destino[0] = (char) ('0' + c10);
    destino[1] = (char) ('0' + c9);
    destino[2] = (char) ('0' + c8);
    destino[3] = (char) ('0' + c7);
    destino[4] = (char) ('0' + c6);
    destino[5] = (char) ('0' + c5);
    destino[6] = (char) ('0' + c4);
    destino[7] = (char) ('0' + c3);
    destino[8] = (char) ('0' + c2);
    destino[9] = (char) ('0' + c1);
}

static uint32_t medir_conversion_original(void)
{
    char buffer[10];
    uint32_t suma = 0u;
    uint32_t t0 = DWT_CYCCNT;

    for (uint32_t i = 0u; i < REP_CONVERSION; i++) {
        conversion_original(buffer, valor + i);
        suma += (uint32_t) buffer[i % 10u];
    }

    uint32_t ciclos = DWT_CYCCNT - t0;
    sumidero = suma;
    return ciclos / REP_CONVERSION;
}

static uint32_t medir_conversion_mejorada(void)
{
    char buffer[10];
    uint32_t suma = 0u;
    uint32_t t0 = DWT_CYCCNT;

    for (uint32_t i = 0u; i < REP_CONVERSION; i++) {
        debug_mejorado_u32_decimal(buffer, valor + i);
        suma += (uint32_t) buffer[i % 10u];
    }

    uint32_t ciclos = DWT_CYCCNT - t0;
    sumidero = suma;
    return ciclos / REP_CONVERSION;
}

static uint32_t medir_literal(void)
{
    debug_mejorado_flush();
    uint32_t t0 = DWT_CYCCNT;
    _DBG(TEXTO);
    uint32_t ciclos = DWT_CYCCNT - t0;
    debug_mejorado_flush();
    return ciclos;
}

static uint32_t medir_numero(void)
{
    debug_mejorado_flush();
    uint32_t t0 = DWT_CYCCNT;
    _DBG("valor=");
    _DBD32(valor);
    _DBG("\n");
    uint32_t ciclos = DWT_CYCCNT - t0;
    debug_mejorado_flush();
    return ciclos;
}

static uint32_t mediana(uint32_t valores[REPETICIONES])
{
    for (uint32_t i = 1u; i < REPETICIONES; i++) {
        uint32_t actual = valores[i];
        uint32_t j = i;
        while (j > 0u && valores[j - 1u] > actual) {
            valores[j] = valores[j - 1u];
            j--;
        }
        valores[j] = actual;
    }
    return valores[REPETICIONES / 2u];
}

int main(void)
{
    uint32_t literal[REPETICIONES];
    uint32_t numero[REPETICIONES];

    debug_mejorado_init();
    DEMCR |= (1u << 24);
    DWT_CYCCNT = 0u;
    DWT_CTRL |= 1u;

    resultados_mejorado.backend = DEBUG_BACKEND;
    resultados_mejorado.baud_configurado = DEBUG_BAUD;
    resultados_mejorado.baud_real = debug_mejorado_baud_real();
    resultados_mejorado.ciclos_conversion_original =
        medir_conversion_original();
    resultados_mejorado.ciclos_conversion_mejorada =
        medir_conversion_mejorada();

    for (uint32_t i = 0u; i < REPETICIONES; i++) {
        literal[i] = medir_literal();
        numero[i] = medir_numero();
    }
    resultados_mejorado.ciclos_literal_48 = mediana(literal);
    resultados_mejorado.ciclos_numero_17 = mediana(numero);

    debug_mejorado_flush();
    uint32_t perdidos_antes = debug_mejorado_perdidos();
    uint32_t t0 = DWT_CYCCNT;
    for (uint32_t i = 0u; i < LINEAS_CAUDAL; i++) {
        while (debug_mejorado_libres() < (sizeof TEXTO - 1u)) {
        }
        _DBG(TEXTO);
    }
    debug_mejorado_flush();
    uint32_t ciclos = DWT_CYCCNT - t0;

    resultados_mejorado.bytes_caudal =
        LINEAS_CAUDAL * (uint32_t) (sizeof TEXTO - 1u);
    resultados_mejorado.us_caudal = ciclos / CICLOS_US;
    resultados_mejorado.bytes_por_segundo = (uint32_t)
        ((uint64_t) resultados_mejorado.bytes_caudal * 1000000u /
         resultados_mejorado.us_caudal);
    resultados_mejorado.perdidos_caudal =
        debug_mejorado_perdidos() - perdidos_antes;

    debug_mejorado_flush();
    perdidos_antes = debug_mejorado_perdidos();
    t0 = DWT_CYCCNT;
    for (uint32_t i = 0u; i < LINEAS_RAFAGA; i++) {
        _DBG(TEXTO);
    }
    ciclos = DWT_CYCCNT - t0;

    resultados_mejorado.bytes_rafaga =
        LINEAS_RAFAGA * (uint32_t) (sizeof TEXTO - 1u);
    resultados_mejorado.ciclos_rafaga = ciclos;
    resultados_mejorado.pendientes_rafaga = debug_mejorado_pendientes();
    resultados_mejorado.perdidos_rafaga =
        debug_mejorado_perdidos() - perdidos_antes;
    debug_mejorado_flush();

    resultados_mejorado.terminado = 0xC0DEF00Du;

    while (1) {
    }
}
