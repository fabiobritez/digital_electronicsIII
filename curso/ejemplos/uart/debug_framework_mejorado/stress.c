/*
 * Prueba de esfuerzo del Debug Framework mejorado.
 *
 * Configuración prevista:
 *
 *   DEBUG_BACKEND=DEBUG_BACKEND_DMA
 *   DEBUG_BAUD=921600
 *
 * Los resultados quedan en resultados_stress para leerlos mediante GDB u
 * OpenOCD. La salida serie permite verificar la prueba de punta a punta.
 */

#include <stdint.h>

#include "LPC17xx.h"
#include "debug_frmwrk_mejorado.h"

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define CICLOS_US               100u
#define FRECUENCIA_CPU_HZ       100000000u
#define REPETICIONES_LATENCIA   5u
#define CANTIDAD_LARGOS         7u
#define CANTIDAD_RITMOS         5u
#define SEGUNDOS_POR_RITMO      2u
#define LINEAS_INUNDACION       100000u
#define LINEAS_BACKPRESSURE     5000u

#define LINEA   "[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C\n"
#define BYTES_LINEA ((uint32_t) (sizeof LINEA - 1u))

struct resultado_ritmo {
    uint32_t lineas_por_segundo;
    uint32_t intentadas;
    uint32_t aceptadas;
    uint32_t descartadas;
    uint32_t pendientes_al_terminar;
    uint32_t us_produccion;
    uint32_t us_hasta_vaciar;
};

struct resultado_debug_stress {
    uint32_t backend;
    uint32_t baud_configurado;
    uint32_t baud_real;
    uint32_t bytes_linea;
    uint32_t cola_asignada;

    uint32_t largos[CANTIDAD_LARGOS];
    uint32_t ciclos_write[CANTIDAD_LARGOS];

    struct resultado_ritmo ritmos[CANTIDAD_RITMOS];

    uint32_t inundacion_intentadas;
    uint32_t inundacion_aceptadas;
    uint32_t inundacion_descartadas;
    uint32_t inundacion_pendientes;
    uint32_t inundacion_us_produccion;
    uint32_t inundacion_us_hasta_vaciar;

    uint32_t backpressure_lineas;
    uint32_t backpressure_perdidas;
    uint32_t backpressure_us_total;
    uint32_t backpressure_us_esperando;

    uint32_t perdidos_totales;
    uint32_t terminado;
};

volatile struct resultado_debug_stress resultados_stress;

static uint8_t mensaje_largo[2000];

static const uint32_t largos[CANTIDAD_LARGOS] = {
    1u, 16u, 48u, 128u, 512u, 1024u, 2000u
};

static const uint32_t ritmos[CANTIDAD_RITMOS] = {
    1800u, 1900u, 1950u, 2000u, 2500u
};

static uint32_t mediana(uint32_t valores[REPETICIONES_LATENCIA])
{
    for (uint32_t i = 1u; i < REPETICIONES_LATENCIA; i++) {
        uint32_t actual = valores[i];
        uint32_t j = i;
        while (j > 0u && valores[j - 1u] > actual) {
            valores[j] = valores[j - 1u];
            j--;
        }
        valores[j] = actual;
    }
    return valores[REPETICIONES_LATENCIA / 2u];
}

static uint32_t medir_write(uint32_t largo, uint8_t relleno)
{
    uint32_t muestras[REPETICIONES_LATENCIA];

    for (uint32_t i = 0u; i < largo; i++) {
        mensaje_largo[i] = relleno;
    }
    mensaje_largo[largo - 1u] = '\n';

    for (uint32_t i = 0u; i < REPETICIONES_LATENCIA; i++) {
        debug_mejorado_flush();
        uint32_t t0 = DWT_CYCCNT;
        debug_mejorado_write(mensaje_largo, largo);
        muestras[i] = DWT_CYCCNT - t0;
        debug_mejorado_flush();
    }

    return mediana(muestras);
}

static void esperar_hasta(uint32_t instante)
{
    while ((int32_t) (DWT_CYCCNT - instante) < 0) {
    }
}

static void probar_ritmo(uint32_t indice, uint32_t lineas_por_segundo)
{
    volatile struct resultado_ritmo *resultado =
        &resultados_stress.ritmos[indice];
    uint32_t intentadas = lineas_por_segundo * SEGUNDOS_POR_RITMO;
    uint32_t periodo = FRECUENCIA_CPU_HZ / lineas_por_segundo;
    uint32_t perdidos_antes = debug_mejorado_perdidos();

    debug_mejorado_flush();
    uint32_t t0 = DWT_CYCCNT;
    uint32_t proximo = t0;

    for (uint32_t i = 0u; i < intentadas; i++) {
        esperar_hasta(proximo);
        debug_mejorado_write(LINEA, BYTES_LINEA);
        proximo += periodo;
    }

    uint32_t ciclos_produccion = DWT_CYCCNT - t0;
    uint32_t pendientes = debug_mejorado_pendientes();
    uint32_t bytes_descartados =
        debug_mejorado_perdidos() - perdidos_antes;
    debug_mejorado_flush();
    uint32_t ciclos_totales = DWT_CYCCNT - t0;

    resultado->lineas_por_segundo = lineas_por_segundo;
    resultado->intentadas = intentadas;
    resultado->descartadas = bytes_descartados / BYTES_LINEA;
    resultado->aceptadas = intentadas - resultado->descartadas;
    resultado->pendientes_al_terminar = pendientes;
    resultado->us_produccion = ciclos_produccion / CICLOS_US;
    resultado->us_hasta_vaciar = ciclos_totales / CICLOS_US;
}

static void probar_inundacion(void)
{
    uint32_t perdidos_antes = debug_mejorado_perdidos();

    debug_mejorado_flush();
    uint32_t t0 = DWT_CYCCNT;
    for (uint32_t i = 0u; i < LINEAS_INUNDACION; i++) {
        debug_mejorado_write(LINEA, BYTES_LINEA);
    }

    uint32_t ciclos_produccion = DWT_CYCCNT - t0;
    uint32_t pendientes = debug_mejorado_pendientes();
    uint32_t bytes_descartados =
        debug_mejorado_perdidos() - perdidos_antes;
    debug_mejorado_flush();
    uint32_t ciclos_totales = DWT_CYCCNT - t0;

    resultados_stress.inundacion_intentadas = LINEAS_INUNDACION;
    resultados_stress.inundacion_descartadas =
        bytes_descartados / BYTES_LINEA;
    resultados_stress.inundacion_aceptadas =
        LINEAS_INUNDACION - resultados_stress.inundacion_descartadas;
    resultados_stress.inundacion_pendientes = pendientes;
    resultados_stress.inundacion_us_produccion =
        ciclos_produccion / CICLOS_US;
    resultados_stress.inundacion_us_hasta_vaciar =
        ciclos_totales / CICLOS_US;
}

static void probar_backpressure(void)
{
    uint32_t ciclos_esperando = 0u;
    uint32_t perdidos_antes = debug_mejorado_perdidos();

    debug_mejorado_flush();
    uint32_t t0 = DWT_CYCCNT;
    for (uint32_t i = 0u; i < LINEAS_BACKPRESSURE; i++) {
        if (debug_mejorado_libres() < BYTES_LINEA) {
            uint32_t inicio_espera = DWT_CYCCNT;
            while (debug_mejorado_libres() < BYTES_LINEA) {
            }
            ciclos_esperando += DWT_CYCCNT - inicio_espera;
        }
        debug_mejorado_write(LINEA, BYTES_LINEA);
    }
    debug_mejorado_flush();
    uint32_t ciclos_totales = DWT_CYCCNT - t0;

    resultados_stress.backpressure_lineas = LINEAS_BACKPRESSURE;
    resultados_stress.backpressure_perdidas =
        (debug_mejorado_perdidos() - perdidos_antes) / BYTES_LINEA;
    resultados_stress.backpressure_us_total = ciclos_totales / CICLOS_US;
    resultados_stress.backpressure_us_esperando =
        ciclos_esperando / CICLOS_US;
}

int main(void)
{
    debug_mejorado_init();
    DEMCR |= (1u << 24);
    DWT_CYCCNT = 0u;
    DWT_CTRL |= 1u;

    resultados_stress.backend = DEBUG_BACKEND;
    resultados_stress.baud_configurado = DEBUG_BAUD;
    resultados_stress.baud_real = debug_mejorado_baud_real();
    resultados_stress.bytes_linea = BYTES_LINEA;
    resultados_stress.cola_asignada = DEBUG_COLA_SIZE;

    for (uint32_t i = 0u; i < CANTIDAD_LARGOS; i++) {
        resultados_stress.largos[i] = largos[i];
        resultados_stress.ciclos_write[i] =
            medir_write(largos[i], (uint8_t) ('A' + i));
    }

    for (uint32_t i = 0u; i < CANTIDAD_RITMOS; i++) {
        probar_ritmo(i, ritmos[i]);
    }

    probar_inundacion();
    probar_backpressure();

    resultados_stress.perdidos_totales = debug_mejorado_perdidos();
    resultados_stress.terminado = 0x51AE55EDu;

    while (1) {
    }
}
