#ifndef DEBUG_FRMWRK_MEJORADO_H
#define DEBUG_FRMWRK_MEJORADO_H

#include <stdint.h>

/* Backends disponibles. Se elige uno con EXTRA_CFLAGS, por ejemplo:
 *
 *   -DDEBUG_BACKEND=DEBUG_BACKEND_DMA -DDEBUG_BAUD=921600
 */
#define DEBUG_BACKEND_BLOQUES  1
#define DEBUG_BACKEND_IRQ      2
#define DEBUG_BACKEND_DMA      3

#ifndef DEBUG_BACKEND
#define DEBUG_BACKEND DEBUG_BACKEND_DMA
#endif

#ifndef DEBUG_BAUD
#define DEBUG_BAUD 115200
#endif

#ifndef DEBUG_COLA_SIZE
#define DEBUG_COLA_SIZE 2048u
#endif

#if DEBUG_BACKEND != DEBUG_BACKEND_BLOQUES && \
    DEBUG_BACKEND != DEBUG_BACKEND_IRQ && \
    DEBUG_BACKEND != DEBUG_BACKEND_DMA
#error "DEBUG_BACKEND no es válido"
#endif

#if DEBUG_BAUD != 115200 && DEBUG_BAUD != 921600
#error "Este ejemplo admite DEBUG_BAUD=115200 o DEBUG_BAUD=921600"
#endif

#if (DEBUG_COLA_SIZE & (DEBUG_COLA_SIZE - 1u)) != 0
#error "DEBUG_COLA_SIZE debe ser una potencia de dos"
#endif

void debug_mejorado_init(void);

/* Salida básica. debug_mejorado_write() evita recorrer el string cuando el
 * largo ya es conocido. En los backends asíncronos, cada llamada entra
 * completa o se descarta completa. */
void debug_mejorado_puts(const char *s);
void debug_mejorado_puts_line(const char *s);
void debug_mejorado_write(const void *datos, uint32_t largo);
void debug_mejorado_char(uint8_t c);

/* Mantienen el formato del framework original: decimal con ceros a la
 * izquierda y hexadecimal con prefijo 0x. */
void debug_mejorado_dec8(uint8_t valor);
void debug_mejorado_dec16(uint16_t valor);
void debug_mejorado_dec32(uint32_t valor);
void debug_mejorado_hex8(uint8_t valor);
void debug_mejorado_hex16(uint16_t valor);
void debug_mejorado_hex32(uint32_t valor);

/* Convierte sin transmitir. Se expone para poder medir la mejora de la
 * conversión decimal por separado. El destino debe tener diez bytes. */
void debug_mejorado_u32_decimal(char destino[10], uint32_t valor);

/* flush espera hasta que salgan también los bits de la FIFO y del registro de
 * desplazamiento. No se debe llamar con interrupciones deshabilitadas cuando
 * el backend seleccionado es DEBUG_BACKEND_IRQ o DEBUG_BACKEND_DMA. */
void debug_mejorado_flush(void);

uint32_t debug_mejorado_pendientes(void);
uint32_t debug_mejorado_libres(void);
uint32_t debug_mejorado_perdidos(void);
uint32_t debug_mejorado_baud_real(void);

/* API recomendada. */
#define DBG_MSG(s)      debug_mejorado_puts(s)
#define DBG_LINE(s)     debug_mejorado_puts_line(s)
#define DBG_CHAR(c)     debug_mejorado_char((uint8_t) (c))
#define DBG_DEC8(n)     debug_mejorado_dec8((uint8_t) (n))
#define DBG_DEC16(n)    debug_mejorado_dec16((uint16_t) (n))
#define DBG_DEC32(n)    debug_mejorado_dec32((uint32_t) (n))
#define DBG_HEX8(n)     debug_mejorado_hex8((uint8_t) (n))
#define DBG_HEX16(n)    debug_mejorado_hex16((uint16_t) (n))
#define DBG_HEX32(n)    debug_mejorado_hex32((uint32_t) (n))

/* Alias para portar ejemplos que ya usan el framework de NXP. */
#define _DBG(s)         DBG_MSG(s)
#define _DBG_(s)        DBG_LINE(s)
#define _DBC(c)         DBG_CHAR(c)
#define _DBD(n)         DBG_DEC8(n)
#define _DBD16(n)       DBG_DEC16(n)
#define _DBD32(n)       DBG_DEC32(n)
#define _DBH(n)         DBG_HEX8(n)
#define _DBH16(n)       DBG_HEX16(n)
#define _DBH32(n)       DBG_HEX32(n)

#endif
