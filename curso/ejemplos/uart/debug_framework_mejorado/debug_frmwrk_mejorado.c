#include "debug_frmwrk_mejorado.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "LPC17xx.h"
#include "lpc17xx_uart.h"

#define UART_LSR_THRE_BIT   (1u << 5)
#define UART_LSR_TEMT_BIT   (1u << 6)
#define UART_IER_THRE_BIT   (1u << 1)
#define UART_IIR_ID_MASK    0x0Eu
#define UART_IIR_ID_THRE    0x02u

#define COLA_MASK           (DEBUG_COLA_SIZE - 1u)
#define UART_FIFO_TX        16u

/* Buscar dos dígitos de una vez reduce a la mitad la cantidad de divisiones
 * necesarias para convertir un uint32_t con ancho fijo. El costo es una tabla
 * de 200 bytes en Flash. */
static const char pares_decimales[200] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

static void decimal_fijo(char *destino, uint32_t valor, uint32_t digitos)
{
    uint32_t posicion = digitos;

    while (posicion >= 2u) {
        uint32_t cociente = valor / 100u;
        uint32_t resto = valor - cociente * 100u;
        posicion -= 2u;
        destino[posicion] = pares_decimales[resto * 2u];
        destino[posicion + 1u] = pares_decimales[resto * 2u + 1u];
        valor = cociente;
    }

    if (posicion != 0u) {
        destino[0] = (char) ('0' + valor);
    }
}

void debug_mejorado_u32_decimal(char destino[10], uint32_t valor)
{
    /* Caso más usado, desenrollado a propósito. La versión genérica ahorra
     * código fuente, pero el lazo termina costando más que las cuatro
     * divisiones por 100 cuando se compila con -Og. */
    uint32_t cociente = valor / 100u;
    uint32_t resto = valor - cociente * 100u;
    destino[8] = pares_decimales[resto * 2u];
    destino[9] = pares_decimales[resto * 2u + 1u];
    valor = cociente;

    cociente = valor / 100u;
    resto = valor - cociente * 100u;
    destino[6] = pares_decimales[resto * 2u];
    destino[7] = pares_decimales[resto * 2u + 1u];
    valor = cociente;

    cociente = valor / 100u;
    resto = valor - cociente * 100u;
    destino[4] = pares_decimales[resto * 2u];
    destino[5] = pares_decimales[resto * 2u + 1u];
    valor = cociente;

    cociente = valor / 100u;
    resto = valor - cociente * 100u;
    destino[2] = pares_decimales[resto * 2u];
    destino[3] = pares_decimales[resto * 2u + 1u];
    valor = cociente;

    destino[0] = pares_decimales[valor * 2u];
    destino[1] = pares_decimales[valor * 2u + 1u];
}

static void hexadecimal_fijo(char *destino, uint32_t valor, uint32_t digitos)
{
    static const char tabla[] = "0123456789ABCDEF";

    destino[0] = '0';
    destino[1] = 'x';
    for (uint32_t i = 0u; i < digitos; i++) {
        uint32_t desplazamiento = 4u * (digitos - i - 1u);
        destino[i + 2u] = tabla[(valor >> desplazamiento) & 0x0Fu];
    }
}

static void uart0_configurar(void)
{
    LPC_SC->PCONP |= (1u << 3);

#if DEBUG_BAUD == 921600
    /* PCLK_UART0 = CCLK = 100 MHz. DL=5, MULVAL=14, DIVADDVAL=5 da
     * 921052,6 baudios, con un error de -0,06 %. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);
    LPC_SC->PCLKSEL0 |=  (0x1u << 6);
    const uint8_t divisor = 5u;
#else
    /* PCLK_UART0 = CCLK/4 = 25 MHz. DL=10 y el mismo divisor fraccional da
     * 115131,6 baudios, también con un error de -0,06 %. */
    LPC_SC->PCLKSEL0 &= ~(0x3u << 6);
    const uint8_t divisor = 10u;
#endif

    /* Solo hace falta TXD0 en P0.2. P0.3 queda bajo control de la aplicación. */
    LPC_PINCON->PINSEL0 &= ~(0x3u << 4);
    LPC_PINCON->PINSEL0 |=  (0x1u << 4);

    LPC_UART0->LCR = 0x03u | (1u << 7);       /* 8N1 y DLAB=1 */
    LPC_UART0->DLM = 0u;
    LPC_UART0->DLL = divisor;
    LPC_UART0->FDR = (14u << 4) | 5u;
    LPC_UART0->LCR = 0x03u;                   /* DLAB=0 */
    LPC_UART0->IER = 0u;

#if DEBUG_BACKEND == DEBUG_BACKEND_DMA
    LPC_UART0->FCR = 0x0Fu;                   /* FIFO, reset y modo DMA */
#else
    LPC_UART0->FCR = 0x07u;                   /* FIFO y reset */
#endif
    LPC_UART0->TER = (1u << 7);
}


#if DEBUG_BACKEND == DEBUG_BACKEND_BLOQUES

static uint32_t perdidos;

static void entregar_partes(const char *a, uint32_t largo_a,
                            const char *b, uint32_t largo_b)
{
    if (largo_a != 0u) {
        (void) UART_Send((LPC_UART_TypeDef *) LPC_UART0,
                         (uint8_t *) (uintptr_t) a, largo_a, BLOCKING);
    }
    if (largo_b != 0u) {
        (void) UART_Send((LPC_UART_TypeDef *) LPC_UART0,
                         (uint8_t *) (uintptr_t) b, largo_b, BLOCKING);
    }
}

#else

#if DEBUG_BACKEND == DEBUG_BACKEND_DMA
static uint8_t cola[DEBUG_COLA_SIZE]
    __attribute__((section(".ahbram0"), aligned(4)));
#else
static uint8_t cola[DEBUG_COLA_SIZE] __attribute__((aligned(4)));
#endif

static volatile uint32_t cabeza;
static volatile uint32_t lectura;
static volatile uint32_t perdidos;

static inline uint32_t entrar_critica(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void salir_critica(uint32_t primask)
{
    __set_PRIMASK(primask);
}

static uint32_t espacio_libre(void)
{
    return (DEBUG_COLA_SIZE - 1u) - ((cabeza - lectura) & COLA_MASK);
}

static void copiar_en_cola(uint32_t *posicion, const char *datos, uint32_t largo)
{
    uint32_t hasta_final = DEBUG_COLA_SIZE - *posicion;
    uint32_t primero = (largo < hasta_final) ? largo : hasta_final;

    memcpy(&cola[*posicion], datos, primero);
    if (largo > primero) {
        memcpy(&cola[0], datos + primero, largo - primero);
    }
    *posicion = (*posicion + largo) & COLA_MASK;
}

#if DEBUG_BACKEND == DEBUG_BACKEND_IRQ

static void llenar_fifo(void)
{
    uint32_t lugares = UART_FIFO_TX;
    while (lugares != 0u && lectura != cabeza) {
        LPC_UART0->THR = cola[lectura];
        lectura = (lectura + 1u) & COLA_MASK;
        lugares--;
    }
}

static void iniciar_salida(void)
{
    uint32_t primask = entrar_critica();
    if (cabeza != lectura) {
        /* Si la FIFO ya está vacía, se la carga ahora. Esto evita depender de
         * que el periférico genere una interrupción solo por habilitar THRE
         * cuando ese estado ya estaba activo. */
        if ((LPC_UART0->LSR & UART_LSR_THRE_BIT) != 0u) {
            llenar_fifo();
        }

        if (cabeza != lectura) {
            LPC_UART0->IER |= UART_IER_THRE_BIT;
        } else {
            LPC_UART0->IER &= ~UART_IER_THRE_BIT;
        }
    }
    salir_critica(primask);
}

void UART0_IRQHandler(void)
{
    uint32_t identificador = LPC_UART0->IIR & UART_IIR_ID_MASK;

    if (identificador == UART_IIR_ID_THRE) {
        llenar_fifo();

        if (lectura == cabeza) {
            LPC_UART0->IER &= ~UART_IER_THRE_BIT;
        }
    }
}

#else

#define DMA_CANAL          0u
#define DMA_MAX_TRAMO      4095u
#define CTRL_SI            (1u << 26)
#define CTRL_I             (1u << 31)
#define CFG_E              (1u << 0)
#define CFG_DEST_UART0TX   (8u << 6)
#define CFG_M2P            (1u << 11)
#define CFG_IE             (1u << 14)
#define CFG_ITC            (1u << 15)

static volatile uint32_t en_vuelo;

static void iniciar_salida(void)
{
    uint32_t primask = entrar_critica();

    if (en_vuelo == 0u && cabeza != lectura) {
        uint32_t largo = (cabeza > lectura) ? (cabeza - lectura)
                                             : (DEBUG_COLA_SIZE - lectura);
        if (largo > DMA_MAX_TRAMO) {
            largo = DMA_MAX_TRAMO;
        }
        en_vuelo = largo;

        LPC_GPDMACH0->DMACCSrcAddr = (uint32_t) &cola[lectura];
        LPC_GPDMACH0->DMACCDestAddr = (uint32_t) &LPC_UART0->THR;
        LPC_GPDMACH0->DMACCLLI = 0u;
        LPC_GPDMACH0->DMACCControl = largo | CTRL_SI | CTRL_I;
        LPC_GPDMACH0->DMACCConfig = CFG_E | CFG_DEST_UART0TX | CFG_M2P
                                  | CFG_IE | CFG_ITC;
    }

    salir_critica(primask);
}

void DMA_IRQHandler(void)
{
    if ((LPC_GPDMA->DMACIntTCStat & (1u << DMA_CANAL)) != 0u) {
        LPC_GPDMA->DMACIntTCClear = (1u << DMA_CANAL);
        lectura = (lectura + en_vuelo) & COLA_MASK;
        en_vuelo = 0u;
        iniciar_salida();
    }

    if ((LPC_GPDMA->DMACIntErrStat & (1u << DMA_CANAL)) != 0u) {
        LPC_GPDMA->DMACIntErrClr = (1u << DMA_CANAL);
        en_vuelo = 0u;
        iniciar_salida();
    }
}

#endif

static void entregar_partes(const char *a, uint32_t largo_a,
                            const char *b, uint32_t largo_b)
{
    uint32_t total = largo_a + largo_b;
    if (total == 0u) {
        return;
    }

    uint32_t primask = entrar_critica();
    if (total >= DEBUG_COLA_SIZE || espacio_libre() < total) {
        perdidos += total;
        salir_critica(primask);
        return;
    }

    uint32_t nueva_cabeza = cabeza;
    copiar_en_cola(&nueva_cabeza, a, largo_a);
    if (largo_b != 0u) {
        copiar_en_cola(&nueva_cabeza, b, largo_b);
    }
    __asm__ volatile ("dmb" ::: "memory");
    cabeza = nueva_cabeza;
    salir_critica(primask);

    iniciar_salida();
}

#endif


void debug_mejorado_init(void)
{
    perdidos = 0u;

#if DEBUG_BACKEND != DEBUG_BACKEND_BLOQUES
    cabeza = 0u;
    lectura = 0u;
#endif

    uart0_configurar();

#if DEBUG_BACKEND == DEBUG_BACKEND_IRQ
    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_EnableIRQ(UART0_IRQn);
#elif DEBUG_BACKEND == DEBUG_BACKEND_DMA
    en_vuelo = 0u;
    LPC_SC->PCONP |= (1u << 29);
    /* La solicitud 8 es compartida con MAT0.0. Un cero selecciona UART0 Tx. */
    LPC_SC->DMAREQSEL &= ~(1u << 0);
    LPC_GPDMA->DMACConfig = 1u;
    while ((LPC_GPDMA->DMACConfig & 1u) == 0u) {
    }
    LPC_GPDMACH0->DMACCConfig = 0u;
    LPC_GPDMA->DMACIntTCClear = (1u << DMA_CANAL);
    LPC_GPDMA->DMACIntErrClr = (1u << DMA_CANAL);
    NVIC_ClearPendingIRQ(DMA_IRQn);
    NVIC_EnableIRQ(DMA_IRQn);
#endif
}

void debug_mejorado_write(const void *datos, uint32_t largo)
{
    entregar_partes((const char *) datos, largo, NULL, 0u);
}

void debug_mejorado_puts(const char *s)
{
    entregar_partes(s, (uint32_t) strlen(s), NULL, 0u);
}

void debug_mejorado_puts_line(const char *s)
{
    static const char fin[] = "\r\n";
    entregar_partes(s, (uint32_t) strlen(s), fin, 2u);
}

void debug_mejorado_char(uint8_t c)
{
    char dato = (char) c;
    debug_mejorado_write(&dato, 1u);
}

void debug_mejorado_dec8(uint8_t valor)
{
    char salida[3];
    decimal_fijo(salida, valor, sizeof salida);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_dec16(uint16_t valor)
{
    char salida[5];
    decimal_fijo(salida, valor, sizeof salida);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_dec32(uint32_t valor)
{
    char salida[10];
    debug_mejorado_u32_decimal(salida, valor);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_hex8(uint8_t valor)
{
    char salida[4];
    hexadecimal_fijo(salida, valor, 2u);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_hex16(uint16_t valor)
{
    char salida[6];
    hexadecimal_fijo(salida, valor, 4u);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_hex32(uint32_t valor)
{
    char salida[10];
    hexadecimal_fijo(salida, valor, 8u);
    debug_mejorado_write(salida, sizeof salida);
}

void debug_mejorado_flush(void)
{
#if DEBUG_BACKEND == DEBUG_BACKEND_BLOQUES
    while ((LPC_UART0->LSR & UART_LSR_TEMT_BIT) == 0u) {
    }
#else
    iniciar_salida();
    while (cabeza != lectura) {
    }
#if DEBUG_BACKEND == DEBUG_BACKEND_DMA
    while (en_vuelo != 0u) {
    }
#endif
    while ((LPC_UART0->LSR & UART_LSR_TEMT_BIT) == 0u) {
    }
#endif
}

uint32_t debug_mejorado_pendientes(void)
{
#if DEBUG_BACKEND == DEBUG_BACKEND_BLOQUES
    return 0u;
#else
    return (cabeza - lectura) & COLA_MASK;
#endif
}

uint32_t debug_mejorado_libres(void)
{
#if DEBUG_BACKEND == DEBUG_BACKEND_BLOQUES
    return UINT_MAX;
#else
    return espacio_libre();
#endif
}

uint32_t debug_mejorado_perdidos(void)
{
    return perdidos;
}

uint32_t debug_mejorado_baud_real(void)
{
#if DEBUG_BAUD == 921600
    return 921053u;
#else
    return 115132u;
#endif
}
