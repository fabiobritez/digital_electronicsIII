#!/usr/bin/env python3
"""
lpc_checksum.py - Inyecta el checksum de la tabla de vectores de los LPC17xx.

EL PROBLEMA
-----------
La boot ROM del LPC1769 no arranca cualquier cosa que encuentre en la FLASH.
Antes de darle el control a tu programa hace una verificacion de sanidad:

    suma las primeras 8 palabras (32 bits) de la tabla de vectores
    y exige que el resultado, en aritmetica de 32 bits, sea CERO.

Como las primeras 7 palabras son el stack pointer inicial y los handlers de
excepcion (o sea, valores que no elegis vos), la unica forma de que la suma
de cero es poner en la palabra 8 -- el vector 7, offset 0x1C, que ARM declara
"reservado" -- el complemento a dos de la suma de las otras siete:

    vector[7] = (2^32 - (vector[0] + ... + vector[6])) mod 2^32

Si ese valor esta mal, la boot ROM concluye que la FLASH esta vacia o corrupta
y se queda en modo ISP esperando por la UART0. El sintoma que ves vos es:
"grabe la placa, dice que grabo bien, y no hace nada".

Esta es la razon por la que existe este archivo. El detalle esta en el
capitulo 32 del UM10360 (seccion "Criterion for Valid User Code").

QUIEN LO HACE NORMALMENTE
-------------------------
Depende de con que grabes, y esa inconsistencia es justamente el problema:

    openocd     lo parchea al vuelo (opcion calc_checksum del driver lpc2000).
                Como el archivo en disco queda distinto de lo grabado, el
                "verify" despues falla y te tira un warning feo.
    pyocd       NO lo parchea. Graba tu imagen tal cual: si el checksum esta
                mal, la placa no arranca.
    lpc21isp    lo parchea el solo.
    MCUXpresso  lo parchea el solo.

Inyectandolo nosotros en tiempo de build, los cuatro caminos funcionan igual
y el "verify" pasa limpio. Es una linea en el Makefile y te ahorra un dia
entero de "a mi no me anda".

USO
---
    python3 lpc_checksum.py firmware.bin      # parchea (.bin o .elf)
    python3 lpc_checksum.py --check firmware.bin   # solo verifica, no escribe
"""

import argparse
import struct
import sys

# El vector 7 (offset 0x1C) es el que guarda el checksum.
CHECKSUM_VECTOR = 7
CHECKSUM_OFFSET = CHECKSUM_VECTOR * 4
VECTORS_CHECKED = 8          # la boot ROM suma las primeras 8 palabras
MASK32 = 0xFFFFFFFF


def calcular_checksum(palabras_0_a_6):
    """Complemento a dos de la suma de los primeros 7 vectores."""
    suma = sum(palabras_0_a_6) & MASK32
    return (-suma) & MASK32


def buscar_offset_en_elf(datos):
    """
    Devuelve el offset dentro del archivo .elf donde vive la direccion fisica
    0x00000000 (o sea, el arranque de la tabla de vectores en la FLASH).

    Parsea a mano los program headers del ELF32 little-endian, para no
    depender de pyelftools: la plantilla tiene que funcionar con un Python
    recien instalado y nada mas.
    """
    if datos[:4] != b"\x7fELF":
        return None
    if datos[4] != 1:                      # EI_CLASS: 1 = ELF32
        raise SystemExit("lpc_checksum: solo se soporta ELF de 32 bits")
    if datos[5] != 1:                      # EI_DATA: 1 = little endian
        raise SystemExit("lpc_checksum: solo se soporta ELF little-endian")

    e_phoff = struct.unpack_from("<I", datos, 0x1C)[0]
    e_phentsize = struct.unpack_from("<H", datos, 0x2A)[0]
    e_phnum = struct.unpack_from("<H", datos, 0x2C)[0]

    PT_LOAD = 1
    for i in range(e_phnum):
        base = e_phoff + i * e_phentsize
        p_type, p_offset, _p_vaddr, p_paddr, p_filesz = struct.unpack_from(
            "<IIIII", datos, base)
        if p_type != PT_LOAD:
            continue
        # Nos interesa el segmento cargado en la FLASH (direccion FISICA, la
        # LMA) que contenga las 8 primeras palabras.
        fin = p_paddr + p_filesz
        if p_paddr <= 0 and fin >= VECTORS_CHECKED * 4:
            return p_offset + (0 - p_paddr)

    raise SystemExit(
        "lpc_checksum: no encontre en el ELF un segmento cargable que cubra "
        "la direccion 0x0. Revisa el linker script.")


def procesar(ruta, solo_verificar=False, silencioso=False):
    with open(ruta, "rb") as f:
        datos = bytearray(f.read())

    if len(datos) < VECTORS_CHECKED * 4:
        raise SystemExit(f"lpc_checksum: {ruta} es demasiado chico "
                         f"({len(datos)} bytes): no hay tabla de vectores.")

    offset_tabla = buscar_offset_en_elf(datos)
    tipo = "ELF"
    if offset_tabla is None:
        offset_tabla = 0          # un .bin plano arranca en la direccion 0
        tipo = "BIN"

    vectores = list(struct.unpack_from(
        "<%dI" % VECTORS_CHECKED, datos, offset_tabla))
    actual = vectores[CHECKSUM_VECTOR]
    esperado = calcular_checksum(vectores[:CHECKSUM_VECTOR])

    if solo_verificar:
        ok = (actual == esperado)
        if not silencioso:
            estado = "OK" if ok else "MAL"
            print(f"lpc_checksum: {ruta} [{tipo}] checksum {estado} "
                  f"(tiene 0x{actual:08X}, deberia ser 0x{esperado:08X})")
        return 0 if ok else 1

    if actual == esperado:
        if not silencioso:
            print(f"lpc_checksum: {ruta} [{tipo}] ya tenia el checksum "
                  f"correcto (0x{esperado:08X})")
        return 0

    struct.pack_into("<I", datos, offset_tabla + CHECKSUM_OFFSET, esperado)
    with open(ruta, "wb") as f:
        f.write(datos)

    if not silencioso:
        print(f"lpc_checksum: {ruta} [{tipo}] checksum inyectado en el vector "
              f"{CHECKSUM_VECTOR} (0x{CHECKSUM_OFFSET:02X}): 0x{esperado:08X}")
    return 0


def main():
    p = argparse.ArgumentParser(
        description="Inyecta el checksum de la tabla de vectores que exige "
                    "la boot ROM de los LPC17xx.")
    p.add_argument("archivo", help="firmware .bin o .elf")
    p.add_argument("--check", action="store_true",
                   help="solo verificar; no modifica el archivo. "
                        "Devuelve 1 si el checksum esta mal.")
    p.add_argument("-q", "--quiet", action="store_true",
                   help="no imprimir nada")
    args = p.parse_args()

    try:
        return procesar(args.archivo, args.check, args.quiet)
    except FileNotFoundError:
        raise SystemExit(f"lpc_checksum: no existe el archivo {args.archivo}")


if __name__ == "__main__":
    sys.exit(main())
