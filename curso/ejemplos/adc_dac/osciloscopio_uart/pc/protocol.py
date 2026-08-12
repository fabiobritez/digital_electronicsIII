"""Parser del protocolo binario del osciloscopio LPC1769."""

from __future__ import annotations

from dataclasses import dataclass
import struct
from typing import Iterator

import serial
from serial.tools import list_ports


MAGIC = b"LPCS"
VERSION = 1
HEADER_SIZE = 32
HEADER = struct.Struct("<4sBBHIIIIHBBHH")

FLAG_CAPTURE_GAPS = 1 << 0
FLAG_ADC_OVERRUN = 1 << 1
FLAG_DMA_ERROR = 1 << 2
FLAG_DECIMATED = 1 << 3
FLAG_CAPTURE_LOSS = 1 << 4


@dataclass(frozen=True)
class ScopeFrame:
    flags: int
    sequence: int
    capture_rate_hz: int
    output_rate_hz: int
    signal_rate_hz: int
    sample_bits: int
    adc_channel: int
    samples: tuple[int, ...]
    wire_bytes: int


def crc16_ccitt(data: bytes | bytearray | memoryview, crc: int = 0xFFFF) -> int:
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def find_default_port() -> str:
    ports = list(list_ports.comports())
    cp210x = [port.device for port in ports if port.vid == 0x10C4]
    if len(cp210x) == 1:
        return cp210x[0]

    usb_serial = [
        port.device
        for port in ports
        if "ttyUSB" in port.device or "ttyACM" in port.device
    ]
    if len(usb_serial) == 1:
        return usb_serial[0]

    available = ", ".join(port.device for port in ports) or "ninguno"
    raise RuntimeError(
        "No pude elegir el puerto automaticamente. "
        f"Puertos detectados: {available}. Usá --port /dev/ttyUSB0."
    )


class FrameParser:
    def __init__(self) -> None:
        self.buffer = bytearray()
        self.received_bytes = 0
        self.valid_frames = 0
        self.crc_errors = 0
        self.format_errors = 0
        self.discarded_bytes = 0

    def feed(self, data: bytes) -> list[ScopeFrame]:
        self.received_bytes += len(data)
        self.buffer.extend(data)
        frames: list[ScopeFrame] = []

        while True:
            position = self.buffer.find(MAGIC)
            if position < 0:
                keep = min(len(self.buffer), len(MAGIC) - 1)
                discarded = len(self.buffer) - keep
                if discarded:
                    del self.buffer[:discarded]
                    self.discarded_bytes += discarded
                break

            if position:
                del self.buffer[:position]
                self.discarded_bytes += position

            if len(self.buffer) < HEADER_SIZE:
                break

            fields = HEADER.unpack_from(self.buffer)
            (
                magic,
                version,
                flags,
                header_size,
                sequence,
                capture_rate,
                output_rate,
                signal_rate,
                sample_count,
                sample_bits,
                adc_channel,
                payload_length,
                expected_crc,
            ) = fields

            bytes_per_sample = 2 if sample_bits == 12 else 1
            valid = (
                magic == MAGIC
                and version == VERSION
                and header_size == HEADER_SIZE
                and sample_bits in (8, 12)
                and 0 < sample_count <= 4095
                and payload_length == sample_count * bytes_per_sample
                and payload_length <= 8190
                and capture_rate > 0
                and output_rate > 0
            )
            if not valid:
                del self.buffer[0]
                self.format_errors += 1
                continue

            frame_length = HEADER_SIZE + payload_length
            if len(self.buffer) < frame_length:
                break

            header_without_crc = memoryview(self.buffer)[:30]
            payload = memoryview(self.buffer)[HEADER_SIZE:frame_length]
            actual_crc = crc16_ccitt(header_without_crc)
            actual_crc = crc16_ccitt(payload, actual_crc)

            if actual_crc != expected_crc:
                del payload
                del header_without_crc
                del self.buffer[0]
                self.crc_errors += 1
                continue

            if sample_bits == 12:
                samples = struct.unpack(f"<{sample_count}H", payload)
            else:
                samples = tuple(payload)

            del payload
            del header_without_crc
            del self.buffer[:frame_length]

            frames.append(
                ScopeFrame(
                    flags=flags,
                    sequence=sequence,
                    capture_rate_hz=capture_rate,
                    output_rate_hz=output_rate,
                    signal_rate_hz=signal_rate,
                    sample_bits=sample_bits,
                    adc_channel=adc_channel,
                    samples=samples,
                    wire_bytes=frame_length,
                )
            )
            self.valid_frames += 1

        return frames


def open_serial(port: str, baud: int = 921600) -> serial.Serial:
    selected = find_default_port() if port == "auto" else port
    connection = serial.Serial(selected, baudrate=baud, timeout=0.1)
    connection.reset_input_buffer()
    return connection


def serial_frames(connection: serial.Serial, parser: FrameParser) -> Iterator[ScopeFrame]:
    while connection.is_open:
        chunk = connection.read(4096)
        if not chunk:
            continue
        yield from parser.feed(chunk)


def estimate_frequency(samples: tuple[int, ...], sample_rate_hz: int) -> float | None:
    if len(samples) < 4:
        return None

    low = min(samples)
    high = max(samples)
    if high - low < 4:
        return None
    threshold = (low + high) / 2.0
    arm_level = threshold - (high - low) * 0.05

    crossings: list[float] = []
    armed = samples[0] <= arm_level
    previous = samples[0]
    for index, current in enumerate(samples[1:], start=1):
        if current <= arm_level:
            armed = True

        stays_high = (
            index + 2 < len(samples)
            and samples[index + 1] > arm_level
            and samples[index + 2] > arm_level
        )
        if armed and previous < threshold <= current and current != previous and stays_high:
            fraction = (threshold - previous) / (current - previous)
            crossings.append((index - 1) + fraction)
            armed = False
        previous = current

    if len(crossings) < 2:
        return None
    periods = [b - a for a, b in zip(crossings, crossings[1:])]
    ordered = sorted(periods)
    median = ordered[len(ordered) // 2]
    inliers = [period for period in periods if median * 0.65 <= period <= median * 1.35]
    if not inliers:
        return None
    return sample_rate_hz / (sum(inliers) / len(inliers))
