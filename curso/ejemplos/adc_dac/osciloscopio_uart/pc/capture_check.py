#!/usr/bin/env python3
"""Prueba de recepcion sin interfaz grafica."""

from __future__ import annotations

import argparse
import statistics
import time

from protocol import (
    FLAG_ADC_OVERRUN,
    FLAG_CAPTURE_GAPS,
    FLAG_CAPTURE_LOSS,
    FLAG_DMA_ERROR,
    FrameParser,
    estimate_frequency,
    open_serial,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Valida tramas del osciloscopio LPC1769.")
    parser.add_argument("--port", default="auto", help="Puerto serie o 'auto'")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--frames", type=int, default=20)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--expect-hz", type=float, default=20000.0)
    parser.add_argument("--tolerance-percent", type=float, default=5.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    parser = FrameParser()
    connection = open_serial(args.port, args.baud)
    selected_port = connection.port
    start = time.monotonic()
    frames = []

    try:
        deadline = start + args.timeout
        while len(frames) < args.frames and time.monotonic() < deadline:
            chunk = connection.read(4096)
            if chunk:
                frames.extend(parser.feed(chunk))
    finally:
        connection.close()

    elapsed = time.monotonic() - start
    if not frames:
        print(f"ERROR: no llegó ninguna trama desde {selected_port} en {elapsed:.1f} s")
        return 1

    frequencies = [
        value
        for frame in frames
        if (value := estimate_frequency(frame.samples, frame.output_rate_hz)) is not None
    ]
    sequences = [frame.sequence for frame in frames]
    sequence_gaps = sum(
        max(0, ((b - a) & 0xFFFFFFFF) - 1)
        for a, b in zip(sequences, sequences[1:])
        if ((b - a) & 0xFFFFFFFF) < 0x80000000
    )
    wire_bytes = sum(frame.wire_bytes for frame in frames)
    last = frames[-1]
    all_samples = [sample for frame in frames for sample in frame.samples]
    maximum_code = (1 << last.sample_bits) - 1
    mode = "bloques con pausas" if last.flags & FLAG_CAPTURE_GAPS else "streaming continuo"

    print(f"Puerto:                 {selected_port} a {args.baud} baud")
    print(f"Modo:                   {mode}, {last.sample_bits} bits")
    print(f"Tramas válidas:         {len(frames)}")
    print(f"Errores CRC/formato:    {parser.crc_errors}/{parser.format_errors}")
    print(f"Saltos de secuencia:    {sequence_gaps}")
    print(f"Frecuencia ADC:         {last.capture_rate_hz:,} muestras/s")
    print(f"Frecuencia transmitida: {last.output_rate_hz:,} muestras/s")
    print(f"Muestras por trama:     {len(last.samples)}")
    print(f"Caudal observado:       {wire_bytes / elapsed:,.0f} bytes/s")
    print(f"Código mínimo/máximo:   {min(all_samples)}/{max(all_samples)} de {maximum_code}")
    if frequencies:
        print(f"Frecuencia medida:      {statistics.mean(frequencies):,.1f} Hz")

    bad_flags = FLAG_ADC_OVERRUN | FLAG_DMA_ERROR | FLAG_CAPTURE_LOSS
    flagged = sum(1 for frame in frames if frame.flags & bad_flags)
    print(f"Tramas con alerta:      {flagged}")

    failed = parser.crc_errors != 0 or parser.format_errors != 0 or flagged != 0
    if frequencies and args.expect_hz > 0:
        error_percent = abs(statistics.mean(frequencies) - args.expect_hz) * 100 / args.expect_hz
        print(f"Error contra esperado:  {error_percent:.2f} %")
        failed |= error_percent > args.tolerance_percent

    print("Resultado:              " + ("FALLÓ" if failed else "OK"))
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
