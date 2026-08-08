#!/usr/bin/env python3
"""
medir_rtt.py - Mide el rendimiento de la consola RTT desde la PC.

Se usa junto con el firmware bench.c. Levanta OpenOCD, se conecta al canal y
mide el caudal real, la latencia y la perdida, barriendo las dos perillas que
tiene el host: la velocidad del SWD y el intervalo de polleo.

USO
    # con el firmware bench.c ya grabado, desde plantilla/
    python3 ../curso/ejemplos/uart/printf_rtt/medir_rtt.py

    # solo escuchar, con un OpenOCD que ya esta corriendo
    python3 medir_rtt.py --solo-escuchar

    # barrer velocidad de SWD e intervalo de polleo
    python3 medir_rtt.py --barrido

Requiere: openocd 0.12+, python3 (sin librerias externas).
"""

import argparse
import os
import re
import signal
import socket
import subprocess
import sys
import time

PUERTO_RTT = 9090
CFG = "openocd/lpc1769.cfg"


# --------------------------------------------------------------------------
def lanzar_openocd(velocidad_khz, polleo_ms, cfg=CFG):
    """Arranca OpenOCD con el servidor RTT. Devuelve el proceso."""
    cmd = [
        "openocd", "-f", cfg,
        "-c", f"adapter speed {velocidad_khz}",
        "-c", "init",
        "-c", "reset run",
        "-c", 'rtt setup 0x10000000 0x8000 "SEGGER RTT"',
        "-c", f"rtt polling_interval {polleo_ms}",
        "-c", "rtt start",
        "-c", f"rtt server start {PUERTO_RTT} 0",
    ]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True, preexec_fn=os.setsid)
    # esperar a que encuentre el bloque de control
    t0 = time.time()
    salida = ""
    while time.time() - t0 < 10:
        linea = p.stdout.readline()
        if not linea:
            break
        salida += linea
        if "Listening on port" in linea and str(PUERTO_RTT) in linea:
            return p
        if "No control block found" in linea:
            break
    matar(p)
    print("No pude levantar el servidor RTT. Salida de OpenOCD:\n")
    print(salida)
    print("Causas tipicas: el firmware no llama a rtt_init(), otra herramienta")
    print("tiene tomada la sonda, o faltan las reglas udev.")
    sys.exit(1)


def matar(p):
    """Cierra OpenOCD con cuidado.

    Importa mas de lo que parece. OpenOCD le pide al kernel que suelte la
    interfaz USB de la sonda (que es HID) para poder hablarle en crudo, y se la
    devuelve recien al cerrarse. Si lo matas a lo bruto, esa interfaz queda sin
    driver: la sonda sigue apareciendo en lsusb pero ya nadie la puede abrir, y
    el siguiente openocd contesta "unable to find a matching CMSIS-DAP device".
    Se arregla desenchufando y volviendo a enchufar, pero mejor no llegar ahi.

    Por eso: primero SIGINT, que OpenOCD trata como "cerra prolijo"; SIGTERM
    despues; SIGKILL solo como ultimo recurso.
    """
    if not p or p.poll() is not None:
        return
    for sig, espera in ((signal.SIGINT, 5), (signal.SIGTERM, 3), (signal.SIGKILL, 2)):
        try:
            os.killpg(os.getpgid(p.pid), sig)
            p.wait(timeout=espera)
            return
        except subprocess.TimeoutExpired:
            continue
        except Exception:
            return
    # Darle un respiro al kernel para que reasocie el driver antes del proximo
    time.sleep(0.5)


# --------------------------------------------------------------------------
def conectar():
    for _ in range(20):
        try:
            s = socket.create_connection(("localhost", PUERTO_RTT), timeout=5)
            s.settimeout(1.0)
            return s
        except OSError:
            time.sleep(0.25)
    print("No pude conectarme al puerto", PUERTO_RTT)
    sys.exit(1)


def leer_hasta(s, marca, tope_s=30):
    """Acumula hasta ver 'marca' o agotar el tiempo. Devuelve (texto, bytes, seg)."""
    datos = b""
    t0 = time.time()
    while time.time() - t0 < tope_s:
        try:
            b = s.recv(65536)
            if not b:
                break
            datos += b
            if marca and marca.encode() in datos:
                break
        except socket.timeout:
            continue
    return datos.decode("utf-8", "replace"), len(datos), time.time() - t0


# --------------------------------------------------------------------------
def prueba_caudal(s, etiqueta):
    """Manda '2' (caudal sostenido) y cronometra del lado de la PC."""
    s.sendall(b"2")
    # el firmware marca el tramo con CAUDAL_INICIO / CAUDAL_FIN
    txt, _, _ = leer_hasta(s, "CAUDAL_INICIO", 15)
    t0 = time.time()
    txt, nbytes, _ = leer_hasta(s, "CAUDAL_FIN", 60)
    dt = time.time() - t0

    caudal_host = nbytes / dt if dt > 0 else 0

    # lo que reporto el micro
    m = re.search(r"caudal\s*:\s*(\d+) bytes/s", txt)
    caudal_micro = int(m.group(1)) if m else 0
    m = re.search(r"descartados\s*:\s*(\d+)", txt)
    descartados = int(m.group(1)) if m else -1

    # continuidad de los numeros de secuencia: detecta perdida real
    seq = [int(x) for x in re.findall(r"^(\d{8}) 0123456789", txt, re.M)]
    huecos = 0
    if len(seq) > 1:
        huecos = sum(1 for a, b in zip(seq, seq[1:]) if b != a + 1)

    print(f"  {etiqueta:<28} {caudal_host:9.0f} B/s   "
          f"micro dice {caudal_micro:7d} B/s   "
          f"lineas {len(seq):5d}  huecos {huecos:3d}  descart. {descartados}")
    return caudal_host


def prueba_latencia(s, n=15):
    """Ida y vuelta: mando una tecla, cronometro hasta que el micro contesta.

    Mide el lazo completo PC -> cola de bajada -> micro -> cola de subida -> PC,
    o sea dos polleos. Es el numero que sentis cuando usas la consola a mano.
    """
    s.sendall(b"0")                      # dejarlo quieto
    time.sleep(1.0)
    try:
        s.recv(65536)
    except socket.timeout:
        pass

    muestras = []
    for _ in range(n):
        try:
            s.recv(65536)
        except socket.timeout:
            pass
        t0 = time.time()
        s.sendall(b"1")                  # pide el informe de costo de CPU
        s.settimeout(3.0)
        try:
            while True:
                b = s.recv(65536)
                if b and b"Costo de CPU" in b:
                    muestras.append((time.time() - t0) * 1000.0)
                    break
                if not b:
                    break
        except socket.timeout:
            pass
        leer_hasta(s, "Teclas:", 5)      # dejar que termine de imprimir
    s.sendall(b"0")

    if muestras:
        muestras.sort()
        print(f"  ida y vuelta (PC->micro->PC): min {min(muestras):.0f} ms   "
              f"mediana {muestras[len(muestras)//2]:.0f} ms   "
              f"max {max(muestras):.0f} ms   ({len(muestras)} muestras)")
    else:
        print("  no pude medir la latencia")


# --------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--solo-escuchar", action="store_true",
                    help="no levantar OpenOCD; usar uno que ya corre")
    ap.add_argument("--barrido", action="store_true",
                    help="barrer velocidad de SWD e intervalo de polleo")
    ap.add_argument("--velocidad", type=int, default=1000,
                    help="velocidad del SWD en kHz (por defecto 1000)")
    ap.add_argument("--polleo", type=int, default=100,
                    help="intervalo de polleo de RTT en ms (por defecto 100)")
    args = ap.parse_args()

    if not os.path.exists(CFG):
        print(f"No encuentro {CFG}. Corre esto desde el directorio plantilla/.")
        sys.exit(1)

    if args.solo_escuchar:
        s = conectar()
        print("\n== Informe del micro ==\n")
        txt, _, _ = leer_hasta(s, "Teclas:", 30)
        print(txt)
        print("\n== Caudal ==")
        prueba_caudal(s, "tal como esta")
        print("\n== Latencia ==")
        prueba_latencia(s)
        s.close()
        return

    if args.barrido:
        print("\n=== Barrido: que perilla mueve el caudal de RTT ===\n")
        print("  Las dos importan, pero en momentos distintos:")
        print("   - con el polleo por defecto (100 ms), ESE es el cuello;")
        print("     bajarlo a 50 ms duplica el caudal.")
        print("   - por debajo de 50 ms deja de ganarse nada: ahi el techo")
        print("     pasa a ser la velocidad del SWD.")
        print("   - y arriba de 4 MHz tampoco gana mas: el techo final lo")
        print("     pone la sonda, no el chip.\n")
        combinaciones = [
            (1000, 100), (1000, 50), (1000, 20), (1000, 10), (1000, 1),
            (500, 10), (2000, 10), (4000, 10), (8000, 10),
        ]
        for vel, pol in combinaciones:
            p = lanzar_openocd(vel, pol)
            try:
                s = conectar()
                leer_hasta(s, "Teclas:", 30)
                prueba_caudal(s, f"SWD {vel} kHz, polleo {pol} ms")
                s.close()
            finally:
                matar(p)
                time.sleep(0.5)
        return

    p = lanzar_openocd(args.velocidad, args.polleo)
    try:
        s = conectar()
        print("\n== Informe del micro (costo de CPU) ==\n")
        txt, _, _ = leer_hasta(s, "Teclas:", 30)
        print(txt)

        print("== Caudal sostenido, medido desde la PC ==")
        prueba_caudal(s, f"SWD {args.velocidad} kHz, polleo {args.polleo} ms")

        print("\n== Rafaga a fondo (cuanto se descarta) ==")
        s.sendall(b"3")
        txt, _, _ = leer_hasta(s, "es a proposito", 40)
        for linea in txt.splitlines():
            if any(k in linea for k in ("bytes pedidos", "bytes perdidos",
                                        "tiempo", "costo por linea")):
                print("  " + linea.strip())

        print("\n== Latencia de ida y vuelta ==")
        prueba_latencia(s)
        s.close()
    finally:
        matar(p)


if __name__ == "__main__":
    main()
