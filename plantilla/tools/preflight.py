#!/usr/bin/env python3
"""
preflight.py - Chequeos de sanidad ANTES de grabar la placa.

POR QUE EXISTE
--------------
Grabar es la parte facil. Lo caro es el rato que perdes cuando grabaste bien,
la herramienta dijo "Verified OK", y la placa no hace absolutamente nada. Todas
las causas tipicas de eso se pueden detectar en el archivo, en la PC, antes de
tocar el hardware. Eso es lo que hace este script.

Los cinco chequeos, en orden de que tan seguido arruinan el dia:

  1. CHECKSUM DEL VECTOR 7. La boot ROM suma las primeras 8 palabras de la
     tabla de vectores y exige cero. Si no da cero, no corre tu programa y se
     queda en modo ISP. Es la causa numero uno del "grabe y no hace nada".
     (Detalle completo en lpc_checksum.py.)

  2. STACK POINTER INICIAL. La primera palabra de la tabla. Si no apunta a una
     direccion valida de RAM, el primer push del Reset_Handler escribe en el
     aire y el micro se cuelga antes de llegar a main.

  3. BIT THUMB DEL RESET_HANDLER. El Cortex-M3 solo ejecuta Thumb-2, y lo
     senaliza con el bit 0 de la direccion de salto en 1. Si el vector 1 es
     par, el chip toma un UsageFault en la primera instruccion, antes de
     ejecutar una sola linea tuya. El compilador lo pone bien solo, pero si
     armas la tabla a mano en asm es un clasico.

  4. LA PALABRA DE CRP (0x000002FC). Esta es la unica que puede dejarte la
     placa inservible PARA SIEMPRE, asi que se chequea aunque casi nunca pase.
     Ver la explicacion larga mas abajo.

  5. QUE ENTRE EN LA FLASH. Redundante con el linker, que ya aborta si no
     entra, pero es gratis y cubre el caso de grabar un .bin suelto.

USO
---
    python3 tools/preflight.py build/firmware.elf     # o un .bin

    make preflight        # lo mismo, y ya lo corre "make flash" solo

Devuelve 0 si esta todo bien y 1 si algo esta mal, asi que sirve tal cual en
un script o en CI.
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lpc_checksum import calcular_checksum   # noqa: E402


# --- El mapa de memoria del LPC1769 (capitulo 2 del UM10360) ----------------
FLASH_INICIO = 0x00000000
FLASH_TAM    = 512 * 1024
RAM_INICIO   = 0x10000000
RAM_TAM      = 32 * 1024

VECTORES_SUMADOS = 8


# --- La palabra de Code Read Protection -------------------------------------
# El LPC1769 lee la palabra en 0x000002FC al arrancar. Si encuentra uno de
# estos cuatro patrones exactos, aplica el nivel de proteccion correspondiente.
# Cualquier otro valor (incluido 0xFFFFFFFF, que es la FLASH borrada) no activa
# nada. Por eso el riesgo real es bajisimo: tiene que caer un valor exacto de
# 32 bits. Pero como CRP3 es IRREVERSIBLE, se chequea igual.
CRP_OFFSET = 0x2FC

CRP_VALORES = {
    0x12345678: ("CRP1", "bloquea la lectura por SWD; el ISP sigue disponible"),
    0x87654321: ("CRP2", "ademas limita los comandos de ISP"),
    0x43218765: ("CRP3", "DESHABILITA SWD E ISP PARA SIEMPRE: la placa queda "
                         "inservible salvo que tu programa reprograme la FLASH solo"),
    0x4E697370: ("NO_ISP", "deshabilita el bootloader ISP; te quedas sin la red "
                           "de seguridad para recuperar la placa sin sonda"),
}


class Resultado:
    """Acumula los chequeos para poder imprimir un resumen al final."""

    def __init__(self):
        self.fallos = 0
        self.avisos = 0

    def ok(self, titulo, detalle):
        print(f"  [ OK ]   {titulo:<28} {detalle}")

    def aviso(self, titulo, detalle):
        self.avisos += 1
        print(f"  [AVISO]  {titulo:<28} {detalle}")

    def error(self, titulo, detalle):
        self.fallos += 1
        print(f"  [FALLA]  {titulo:<28} {detalle}")


def leer_imagen(ruta):
    """
    Devuelve los bytes tal como van a quedar en la FLASH, empezando en la
    direccion 0. Acepta .elf o .bin.

    Para el .elf se recorren los program headers y se copia cada segmento
    cargable a su direccion FISICA (la LMA, p_paddr, que es donde el grabador
    lo va a poner), no a la virtual. La diferencia importa: .data tiene la VMA
    en RAM pero se guarda en FLASH, y lo que se graba es la LMA.

    Es exactamente lo que hace "objcopy -O binary", reimplementado en veinte
    lineas para no depender de pyelftools.
    """
    with open(ruta, "rb") as f:
        datos = f.read()

    if datos[:4] != b"\x7fELF":
        return datos, "BIN"            # un .bin plano ya es la imagen

    if datos[4] != 1 or datos[5] != 1:
        raise SystemExit("preflight: solo se soporta ELF32 little-endian")

    e_phoff = struct.unpack_from("<I", datos, 0x1C)[0]
    e_phentsize = struct.unpack_from("<H", datos, 0x2A)[0]
    e_phnum = struct.unpack_from("<H", datos, 0x2C)[0]

    PT_LOAD = 1
    imagen = bytearray()
    for i in range(e_phnum):
        p_type, p_offset, _vaddr, p_paddr, p_filesz = struct.unpack_from(
            "<IIIII", datos, e_phoff + i * e_phentsize)
        # Solo los segmentos con contenido que van a la FLASH. p_filesz es lo
        # que ocupa EN EL ARCHIVO: .bss tiene p_filesz 0 y no se graba.
        if p_type != PT_LOAD or p_filesz == 0:
            continue
        if not (FLASH_INICIO <= p_paddr < FLASH_INICIO + FLASH_TAM):
            continue
        fin = p_paddr - FLASH_INICIO + p_filesz
        if len(imagen) < fin:
            # El hueco entre segmentos queda en 0xFF, que es la FLASH borrada.
            imagen.extend(b"\xff" * (fin - len(imagen)))
        imagen[p_paddr - FLASH_INICIO:fin] = datos[p_offset:p_offset + p_filesz]

    if not imagen:
        raise SystemExit(
            "preflight: el ELF no tiene ningun segmento cargable en la FLASH. "
            "Revisa el linker script.")

    return bytes(imagen), "ELF"


def chequear(ruta):
    r = Resultado()
    imagen, tipo = leer_imagen(ruta)

    print(f"\nChequeos previos al grabado: {ruta} [{tipo}]\n")

    if len(imagen) < VECTORES_SUMADOS * 4:
        r.error("tamano minimo",
                f"solo {len(imagen)} bytes: no hay ni tabla de vectores")
        return r

    vectores = list(struct.unpack_from("<%dI" % VECTORES_SUMADOS, imagen, 0))

    # --- 1. Checksum del vector 7 -------------------------------------------
    suma = sum(vectores) & 0xFFFFFFFF
    esperado = calcular_checksum(vectores[:7])
    if suma == 0:
        r.ok("checksum de la boot ROM",
             f"la suma de las 8 palabras da 0 (vector 7 = 0x{vectores[7]:08X})")
    else:
        r.error("checksum de la boot ROM",
                f"la suma da 0x{suma:08X} y tiene que dar 0. El vector 7 "
                f"deberia ser 0x{esperado:08X} y es 0x{vectores[7]:08X}. "
                f"Corregilo con: python3 tools/lpc_checksum.py {ruta}")

    # --- 2. Stack pointer inicial -------------------------------------------
    sp = vectores[0]
    ram_fin = RAM_INICIO + RAM_TAM
    if RAM_INICIO < sp <= ram_fin:
        if sp % 8:
            r.aviso("stack pointer inicial",
                    f"0x{sp:08X} no esta alineado a 8. El AAPCS de ARM pide "
                    f"que el stack este alineado a 8 en las llamadas.")
        else:
            r.ok("stack pointer inicial",
                 f"0x{sp:08X} (tope de la RAM: 0x{ram_fin:08X})")
    else:
        r.error("stack pointer inicial",
                f"0x{sp:08X} cae fuera de la RAM "
                f"(0x{RAM_INICIO:08X} a 0x{ram_fin:08X}). El micro se cuelga "
                f"en el primer push, antes de llegar a main.")

    # --- 3. Bit Thumb del Reset_Handler -------------------------------------
    reset = vectores[1]
    if not reset & 1:
        r.error("bit Thumb del Reset_Handler",
                f"0x{reset:08X} es par. El Cortex-M3 solo ejecuta Thumb-2 y "
                f"exige el bit 0 en 1: asi salta a un UsageFault en la primera "
                f"instruccion.")
    elif not (FLASH_INICIO <= (reset & ~1) < FLASH_INICIO + FLASH_TAM):
        r.error("direccion del Reset_Handler",
                f"0x{reset:08X} cae fuera de la FLASH")
    else:
        r.ok("Reset_Handler",
             f"0x{reset:08X} (bit Thumb en 1, dentro de la FLASH)")

    # --- 4. La palabra de CRP ------------------------------------------------
    if len(imagen) > CRP_OFFSET + 4:
        crp = struct.unpack_from("<I", imagen, CRP_OFFSET)[0]
        if crp in CRP_VALORES:
            nombre, que_hace = CRP_VALORES[crp]
            r.error(f"CRP en 0x{CRP_OFFSET:03X}",
                    f"vale 0x{crp:08X} = {nombre}: {que_hace}. "
                    f"NO GRABES ESTO salvo que sea a proposito.")
        else:
            r.ok(f"CRP en 0x{CRP_OFFSET:03X}",
                 f"0x{crp:08X}, no coincide con ningun patron de proteccion")
    else:
        r.ok(f"CRP en 0x{CRP_OFFSET:03X}",
             f"la imagen termina en 0x{len(imagen):X}, no llega a esa palabra")

    # --- 5. Que entre en la FLASH -------------------------------------------
    if len(imagen) > FLASH_TAM:
        r.error("tamano",
                f"{len(imagen)} bytes no entran en los {FLASH_TAM} de FLASH")
    else:
        pct = 100.0 * len(imagen) / FLASH_TAM
        r.ok("tamano",
             f"{len(imagen)} bytes de {FLASH_TAM} ({pct:.2f}% de la FLASH)")

    return r


def main():
    p = argparse.ArgumentParser(
        description="Chequea que un firmware para LPC1769 vaya a arrancar, "
                    "antes de grabarlo.")
    p.add_argument("archivo", help="firmware .elf o .bin")
    args = p.parse_args()

    try:
        r = chequear(args.archivo)
    except FileNotFoundError:
        raise SystemExit(f"preflight: no existe el archivo {args.archivo}")

    print()
    if r.fallos:
        print(f"  {r.fallos} problema(s): esta imagen no va a arrancar bien. "
              f"No la grabes todavia.\n")
        return 1

    if r.avisos:
        print(f"  Todo en orden, con {r.avisos} aviso(s). Listo para grabar.\n")
    else:
        print("  Todo en orden. Listo para grabar.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
