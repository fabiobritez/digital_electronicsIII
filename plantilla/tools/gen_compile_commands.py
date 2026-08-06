#!/usr/bin/env python3
"""
gen_compile_commands.py - Genera compile_commands.json para clangd.

PARA QUE SIRVE
--------------
Los editores modernos no entienden tu codigo leyendolo: levantan un "language
server". Para C y C++ el estandar de hecho es clangd, y lo usan vim, neovim,
helix, emacs, Sublime, Zed y tambien VSCode si preferis clangd a la extension
de Microsoft.

El problema es que clangd necesita saber con que flags se compila cada archivo:
donde estan los headers (-I), que macros estan definidas (-D), que arquitectura
es. Si no lo sabe, te subraya en rojo codigo que compila perfecto, no encuentra
las definiciones y el autocompletado no anda.

compile_commands.json es el archivo estandar que lleva justamente esa
informacion: una entrada por archivo compilado, con el comando exacto.

COMO LO ARMA
------------
No duplica la logica del Makefile, que se desincronizaria al primer cambio.
Lo que hace es preguntarle al propio make que comandos ejecutaria:

    make -n -B V=1

La opcion -n imprime los comandos sin ejecutarlos y -B fuerza a que los
considere todos (aunque los .o ya esten al dia). De esa salida se filtran las
lineas de compilacion y se las convierte al formato JSON.

Como efecto util, siempre queda consistente con el Makefile de verdad.

USO
---
    make compile_commands.json      (lo normal)
    python3 tools/gen_compile_commands.py
"""

import json
import os
import shlex
import subprocess
import sys

SALIDA = "compile_commands.json"


def comandos_del_make():
    """Le pide a make la lista de comandos que ejecutaria, sin ejecutarlos."""
    try:
        r = subprocess.run(
            ["make", "-n", "-B", "V=1"],
            capture_output=True, text=True, check=False)
    except FileNotFoundError:
        raise SystemExit("gen_compile_commands: no encontre 'make' en el PATH")

    if r.returncode != 0 and not r.stdout.strip():
        sys.stderr.write(r.stderr)
        raise SystemExit("gen_compile_commands: 'make -n' fallo")

    return r.stdout.splitlines()


def es_compilacion(partes):
    """
    True si la linea es una compilacion de un .c a un .o.

    Se pide que aparezca '-c' y '-o', para descartar el comando de linkeo
    (que tambien invoca a gcc, pero no compila un archivo).
    """
    if not partes:
        return False
    if "gcc" not in os.path.basename(partes[0]):
        return False
    return "-c" in partes and "-o" in partes


def archivo_fuente(partes):
    for p in partes:
        if p.endswith(".c"):
            return p
    return None


def main():
    raiz = os.getcwd()
    entradas = []
    vistos = set()

    for linea in comandos_del_make():
        linea = linea.strip()
        if not linea:
            continue
        try:
            partes = shlex.split(linea)
        except ValueError:
            continue

        if not es_compilacion(partes):
            continue

        fuente = archivo_fuente(partes)
        if fuente is None or fuente in vistos:
            continue
        vistos.add(fuente)

        # clangd no conoce algunas opciones propias de gcc y se queja. Se las
        # saca: no cambian como se interpreta el codigo, solo como se genera.
        limpias = [p for p in partes
                   if not p.startswith("-specs=")
                   and not p.startswith("--specs=")
                   and p not in ("-MMD", "-MP")]

        entradas.append({
            "directory": raiz,
            "file": os.path.abspath(fuente),
            "arguments": limpias,
        })

    if not entradas:
        raise SystemExit(
            "gen_compile_commands: no encontre ninguna compilacion.\n"
            "Revisa que 'make -n V=1' imprima lineas con arm-none-eabi-gcc.")

    with open(SALIDA, "w", encoding="utf-8") as f:
        json.dump(entradas, f, indent=2)
        f.write("\n")

    print(f"gen_compile_commands: {len(entradas)} archivos -> {SALIDA}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
