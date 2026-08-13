#!/usr/bin/env python3
"""Prueba automática de las cinco formas con el loopback DAC a ADC."""

from __future__ import annotations

import argparse
import math
from statistics import median
import time

from protocol import (
    BAUD,
    COMMAND_SET,
    GeneratorStatus,
    MixedParser,
    ScopeFrame,
    WAVE_NAMES,
    build_command,
    estimate_frequency,
    open_serial,
)


TESTS = (
    (0, 20_000, 2800, 1650),
    (1, 20_000, 2400, 1650),
    (2, 10_000, 2200, 1500),
    (3, 5_000, 2000, 1500),
    (4, 5_000, 2000, 1500),
)


def ideal_sample(waveform: int, phase: float) -> float:
    phase %= 1.0
    if waveform == 0:
        return math.sin(2.0 * math.pi * phase)
    if waveform == 1:
        return 1.0 if phase < 0.5 else -1.0
    if waveform == 2:
        return 1.0 - 4.0 * abs(phase - 0.5)
    if waveform == 3:
        return 2.0 * phase - 1.0
    return 1.0 - 2.0 * phase


def normalized_shape_error(
    waveform: int, samples: tuple[int, ...], sample_rate_hz: int, frequency_hz: float
) -> float:
    """Compara la forma sin exigir fase, offset ni ganancia exactos."""
    valid = [(index, value) for index, value in enumerate(samples) if 8 < value < 4087]
    if len(valid) < 16:
        return float("inf")

    measured = [value for _index, value in valid]
    mean_measured = sum(measured) / len(measured)
    span = max(measured) - min(measured)
    if span < 16:
        return float("inf")

    best = float("inf")
    # ADC y DAC no arrancan en fase. Se prueban 200 orígenes posibles y, para
    # cada uno, se ajustan solamente ganancia y offset antes de calcular RMSE.
    for phase_step in range(200):
        phase_offset = phase_step / 200.0
        expected = [
            ideal_sample(
                waveform,
                phase_offset + index * frequency_hz / sample_rate_hz,
            )
            for index, _value in valid
        ]
        mean_expected = sum(expected) / len(expected)
        variance = sum((value - mean_expected) ** 2 for value in expected)
        if variance == 0.0:
            continue
        gain = sum(
            (x - mean_expected) * (y - mean_measured)
            for x, y in zip(expected, measured)
        ) / variance
        offset = mean_measured - gain * mean_expected
        rmse = math.sqrt(
            sum(
                (y - (gain * x + offset)) ** 2
                for x, y in zip(expected, measured)
            )
            / len(measured)
        )
        best = min(best, rmse / span)
    return best


def wait_result(connection, parser: MixedParser, sequence: int, timeout: float):
    deadline = time.monotonic() + timeout
    status = None
    frames = []
    while time.monotonic() < deadline:
        for message in parser.feed(connection.read(4096)):
            if isinstance(message, GeneratorStatus) and message.sequence == sequence:
                status = message
            elif isinstance(message, ScopeFrame) and status is not None:
                frames.append(message)
        if status is not None and len(frames) >= 3:
            return status, frames
    raise TimeoutError("no llegó la respuesta completa")


def main() -> None:
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument("--port", default="auto")
    argument_parser.add_argument("--baud", type=int, default=BAUD)
    argument_parser.add_argument("--timeout", type=float, default=3.0)
    args = argument_parser.parse_args()

    parser = MixedParser()
    failures = 0
    results = []
    with open_serial(args.port, args.baud) as connection:
        for sequence, (wave, frequency, vpp, offset) in enumerate(TESTS, start=1):
            connection.write(
                build_command(
                    COMMAND_SET, sequence, wave, frequency, vpp, offset, True
                )
            )
            connection.flush()
            try:
                status, frames = wait_result(
                    connection, parser, sequence, args.timeout
                )
            except TimeoutError as exc:
                failures += 1
                print(f"FALLO {WAVE_NAMES[wave]}: {exc}")
                continue
            results.append((wave, frequency, status, frames))

    # Se analiza después de cerrar el puerto. El ajuste de forma hace bastante
    # cálculo en Python; si se hiciera entre comandos, el sistema operativo
    # podría llenar su buffer serie mientras la placa continúa transmitiendo.
    for wave, frequency, status, frames in results:
        frequency_estimates = [
            value
            for frame in frames
            if (value := estimate_frequency(frame.samples, frame.output_rate_hz))
            is not None
        ]
        measured = median(frequency_estimates) if frequency_estimates else None
        measured_text = "sin medida" if measured is None else f"{measured:.1f} Hz"
        expected = status.actual_frequency_hz
        frequency_ok = (
            measured is not None and abs(measured - expected) / expected < 0.03
        )
        robust_spans = []
        for frame in frames:
            robust = [value for value in frame.samples if 8 < value < 4087]
            robust_spans.append(max(robust) - min(robust) if robust else 0)
        level_span = median(robust_spans)
        levels_ok = level_span > 500
        shape_error = median(
            normalized_shape_error(
                wave, frame.samples, frame.output_rate_hz, expected
            )
            for frame in frames
        )
        shape_ok = shape_error < 0.12
        valid = status.status == 0 and frequency_ok and levels_ok and shape_ok
        failures += not valid

        print(
            f"{'OK' if valid else 'FALLO'} {WAVE_NAMES[wave]:23s} "
            f"pedido={frequency:7d} Hz real={expected:9.3f} Hz "
            f"ADC={measured_text:14s} N={status.sample_count:3d} "
            f"códigos={min(min(frame.samples) for frame in frames):4d}.."
            f"{max(max(frame.samples) for frame in frames):4d} "
            f"error_forma={shape_error * 100:4.1f}%"
        )

    if parser.crc_errors or parser.format_errors:
        failures += 1
        print("FALLO de transporte: se recibieron tramas corruptas")
    print(
        f"Mensajes válidos={parser.valid_messages}, CRC erróneos={parser.crc_errors}, "
        f"formato erróneo={parser.format_errors}, bytes descartados={parser.discarded_bytes}"
    )
    raise SystemExit(1 if failures else 0)


if __name__ == "__main__":
    main()
