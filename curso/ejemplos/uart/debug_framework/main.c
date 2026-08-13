/*
 * Banco de pruebas del Debug Framework de NXP para el LPC1769.
 *
 * Compara dos formas de producir exactamente la misma salida por UART0:
 *
 *   1. Las macros _DBG() y _DBD32() del framework.
 *   2. printf() redirigido a UARTPutChar(), la misma salida bloqueante que
 *      usa el framework por debajo.
 *
 * Tambien mide el caudal sostenido de _DBG(). El contador DWT mide el tiempo
 * que la llamada mantiene ocupado al programa, no la hora de llegada a la PC.
 *
 * Desde plantilla/:
 *
 *   cp ../curso/ejemplos/uart/debug_framework/main.c src/main.c
 *   make USE_CMSIS=1 flash
 *
 * Terminal: 115200 8N1 sobre UART0 (TXD0=P0.2, RXD0=P0.3).
 */

#include <stdint.h>
#include <stdio.h>

#include "LPC17xx.h"
#include "debug_frmwrk.h"

/* En esta versión antigua de CMSIS, LPC_UART0_TypeDef y LPC_UART_TypeDef
 * describen el mismo bloque de registros pero son tipos C distintos. El
 * framework espera el segundo. Este cast evita el warning de GCC moderno sin
 * cambiar la biblioteca original. */
#undef DEBUG_UART_PORT
#define DEBUG_UART_PORT  ((LPC_UART_TypeDef *) LPC_UART0)

#define DEMCR       (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL    (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t *) 0xE0001004)

#define LSR_TEMT       (1u << 6)
#define REPETICIONES   5u
#define LINEAS_CAUDAL  1000u
#define CICLOS_US      100u

/* Son 48 bytes, incluido el '\n'. Es la misma línea usada en las pruebas de
 * UART con DMA y RTT. */
#define TEXTO  "[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C\n"

static volatile uint32_t valor = 1234567u;

struct resultados_banco {
    uint32_t dbg_literal;
    uint32_t printf_literal;
    uint32_t dbg_numero;
    uint32_t printf_numero;
    uint32_t bytes_caudal;
    uint32_t us_caudal;
    uint32_t bytes_por_segundo;
    uint32_t terminado;
};

/* Además de imprimirlos, se dejan en RAM para poder leerlos con el debugger
 * si el conversor USB-serie no está disponible. */
volatile struct resultados_banco resultados;


/* La plantilla llama a este gancho desde _write(). No se traduce LF a CRLF
 * para que _DBG() y printf() transmitan exactamente los mismos bytes. */
int __io_putchar(int ch)
{
    UARTPutChar((LPC_UART_TypeDef *) DEBUG_UART_PORT, (uint8_t) ch);
    return ch;
}


static void drenar_uart(void)
{
    while (!(LPC_UART0->LSR & LSR_TEMT)) {
    }
}


static uint32_t mediana(uint32_t v[REPETICIONES])
{
    for (uint32_t i = 1u; i < REPETICIONES; i++) {
        uint32_t x = v[i];
        uint32_t j = i;
        while (j > 0u && v[j - 1u] > x) {
            v[j] = v[j - 1u];
            j--;
        }
        v[j] = x;
    }
    return v[REPETICIONES / 2u];
}


static uint32_t medir_dbg_literal(void)
{
    drenar_uart();
    uint32_t t0 = DWT_CYCCNT;
    _DBG(TEXTO);
    return DWT_CYCCNT - t0;
}


static uint32_t medir_printf_literal(void)
{
    drenar_uart();
    uint32_t t0 = DWT_CYCCNT;
    printf("%s", TEXTO);
    return DWT_CYCCNT - t0;
}


static uint32_t medir_dbg_numero(void)
{
    drenar_uart();
    uint32_t t0 = DWT_CYCCNT;
    _DBG("valor=");
    _DBD32(valor);
    _DBG("\n");
    return DWT_CYCCNT - t0;
}


static uint32_t medir_printf_numero(void)
{
    drenar_uart();
    uint32_t t0 = DWT_CYCCNT;
    printf("valor=%010lu\n", (unsigned long) valor);
    return DWT_CYCCNT - t0;
}


int main(void)
{
    uint32_t c_dbg_literal[REPETICIONES];
    uint32_t c_printf_literal[REPETICIONES];
    uint32_t c_dbg_numero[REPETICIONES];
    uint32_t c_printf_numero[REPETICIONES];

    debug_frmwrk_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    DEMCR |= (1u << 24);
    DWT_CYCCNT = 0u;
    DWT_CTRL |= 1u;

    /* Da tiempo a abrir la terminal después de grabar. */
    for (volatile uint32_t d = 0u; d < 10000000u; d++) {
    }

    printf("\n=== Debug Framework de NXP: banco de pruebas ===\n");
    printf("LPC1769 a %lu Hz, UART0 115200 8N1\n",
           (unsigned long) SystemCoreClock);
    printf("Cada resultado es la mediana de %u repeticiones.\n\n",
           (unsigned) REPETICIONES);

    for (uint32_t i = 0u; i < REPETICIONES; i++) {
        c_dbg_literal[i] = medir_dbg_literal();
        c_printf_literal[i] = medir_printf_literal();
        c_dbg_numero[i] = medir_dbg_numero();
        c_printf_numero[i] = medir_printf_numero();
    }

    uint32_t dbg_literal = mediana(c_dbg_literal);
    uint32_t printf_literal = mediana(c_printf_literal);
    uint32_t dbg_numero = mediana(c_dbg_numero);
    uint32_t printf_numero = mediana(c_printf_numero);

    resultados.dbg_literal = dbg_literal;
    resultados.printf_literal = printf_literal;
    resultados.dbg_numero = dbg_numero;
    resultados.printf_numero = printf_numero;

    drenar_uart();
    printf("Costo de la llamada, con la UART vacía al empezar:\n");
    printf("  _DBG, literal de 48 B       : %lu ciclos, %lu us\n",
           (unsigned long) dbg_literal,
           (unsigned long) (dbg_literal / CICLOS_US));
    printf("  printf, el mismo literal    : %lu ciclos, %lu us\n",
           (unsigned long) printf_literal,
           (unsigned long) (printf_literal / CICLOS_US));
    printf("  _DBG + _DBD32, 17 B         : %lu ciclos, %lu us\n",
           (unsigned long) dbg_numero,
           (unsigned long) (dbg_numero / CICLOS_US));
    printf("  printf con %%010lu, 17 B     : %lu ciclos, %lu us\n",
           (unsigned long) printf_numero,
           (unsigned long) (printf_numero / CICLOS_US));

    printf("\nCaudal sostenido: %u líneas de %u B con _DBG...\n",
           (unsigned) LINEAS_CAUDAL, (unsigned) (sizeof TEXTO - 1u));
    drenar_uart();
    uint32_t t0 = DWT_CYCCNT;
    for (uint32_t i = 0u; i < LINEAS_CAUDAL; i++) {
        _DBG(TEXTO);
    }
    uint32_t ciclos = DWT_CYCCNT - t0;
    drenar_uart();

    uint32_t bytes = LINEAS_CAUDAL * (uint32_t) (sizeof TEXTO - 1u);
    uint32_t us = ciclos / CICLOS_US;
    resultados.bytes_caudal = bytes;
    resultados.us_caudal = us;
    resultados.bytes_por_segundo =
        (uint32_t) ((uint64_t) bytes * 1000000u / us);
    resultados.terminado = 0xC0DEF00Du;

    printf("  bytes transmitidos          : %lu\n", (unsigned long) bytes);
    printf("  tiempo dentro de _DBG       : %lu us\n", (unsigned long) us);
    printf("  caudal                       : %lu B/s\n",
           (unsigned long) resultados.bytes_por_segundo);
    printf("\nFin de la prueba.\n");

    while (1) {
    }
}
