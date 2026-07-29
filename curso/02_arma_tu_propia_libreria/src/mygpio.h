/*
 * mygpio.h  —  Mini librería de GPIO para el LPC1769, hecha a mano.
 *
 * Objetivo didáctico: mostrar que una "librería" no es magia. Esto es nuestra
 * versión mínima de las capas 2 (dispositivo) y 3 (driver) de CMSIS, solo para GPIO.
 *
 * Material de Electrónica Digital III.
 */
#ifndef MYGPIO_H
#define MYGPIO_H

#include <stdint.h>

/* ----- Capa "núcleo": calificadores de acceso (igual que CMSIS) ----- */
#define __IO  volatile          /* lectura / escritura */
#define __O   volatile          /* solo escritura      */
#define __I   volatile const    /* solo lectura        */

/* ----- Capa "dispositivo": la struct de un puerto GPIO del LPC176x ----- *
 * Tiene que ser un molde EXACTO del bloque de registros en memoria.
 * Los offsets (ver comentarios) deben coincidir con el manual (Cap. 9).   */
typedef struct {
    __IO uint32_t FIODIR;        /* +0x00  dirección: 1 = salida, 0 = entrada */
         uint32_t RESERVED0[3];  /* +0x04..0x0C  huecos (no usados)           */
    __IO uint32_t FIOMASK;       /* +0x10  máscara de acceso                  */
    __IO uint32_t FIOPIN;        /* +0x14  estado de los pines                */
    __IO uint32_t FIOSET;        /* +0x18  escribir 1 -> pin a alto           */
    __O  uint32_t FIOCLR;        /* +0x1C  escribir 1 -> pin a bajo           */
} MYGPIO_Port;

/* Direcciones base de cada puerto (Cap. 9 del manual). Cada puerto cada 0x20. */
#define MYGPIO0  ((MYGPIO_Port *) 0x2009C000UL)
#define MYGPIO1  ((MYGPIO_Port *) 0x2009C020UL)
#define MYGPIO2  ((MYGPIO_Port *) 0x2009C040UL)
#define MYGPIO3  ((MYGPIO_Port *) 0x2009C060UL)
#define MYGPIO4  ((MYGPIO_Port *) 0x2009C080UL)

/* ----- Capa "driver": funciones cómodas ----- */
typedef enum { ENTRADA = 0, SALIDA = 1 } MYGPIO_Dir;

/* Configura el pin (puerto, pin) como ENTRADA o SALIDA. */
void mygpio_dir(uint8_t puerto, uint8_t pin, MYGPIO_Dir dir);

/* Pone el pin en alto (1). */
void mygpio_set(uint8_t puerto, uint8_t pin);

/* Pone el pin en bajo (0). */
void mygpio_clr(uint8_t puerto, uint8_t pin);

/* Invierte el estado del pin. */
void mygpio_toggle(uint8_t puerto, uint8_t pin);

/* Devuelve 1 si el pin está en alto, 0 si está en bajo. */
uint8_t mygpio_read(uint8_t puerto, uint8_t pin);

#endif /* MYGPIO_H */
