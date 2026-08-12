"""Protocolo binario del generador y de su monitor ADC."""

from __future__ import annotations

from dataclasses import dataclass
import struct

import serial
from serial.tools import list_ports


BAUD = 921600
COMMAND_SET = 1
COMMAND_GET = 2

COMMAND_MAGIC = b"GENC"
STATUS_MAGIC = b"GENR"
SCOPE_MAGIC = b"LPCS"
VERSION = 1

COMMAND = struct.Struct("<4sBBBBIIHHHH")
STATUS = struct.Struct("<4sBBBBIIIHHHHIHH")
SCOPE = struct.Struct("<4sBBHIIIIHBBHH")

STATUS_TEXT = {
    0: "configuración aplicada",
    1: "CRC incorrecto",
    2: "versión de protocolo incompatible",
    3: "comando desconocido",
    10: "forma de onda inválida",
    11: "frecuencia fuera de rango",
    12: "amplitud u offset fuera de rango",
    13: "no existe una temporización válida",
}

WAVE_NAMES = (
    "Senoidal",
    "Cuadrada",
    "Triangular",
    "Serrucho ascendente",
    "Serrucho descendente",
)

FLAG_CAPTURE_GAPS = 1 << 0
FLAG_ADC_OVERRUN = 1 << 1
FLAG_DMA_ERROR = 1 << 2


@dataclass(frozen=True)
class GeneratorStatus:
    status: int
    waveform: int
    monitor_enabled: bool
    sequence: int
    requested_frequency_hz: int
    actual_frequency_millihz: int
    vpp_mv: int
    offset_mv: int
    sample_count: int
    counter_value: int
    generation: int
    flags: int

    @property
    def actual_frequency_hz(self) -> float:
        return self.actual_frequency_millihz / 1000.0

    @property
    def update_rate_hz(self) -> float:
        return 25_000_000.0 / (self.counter_value + 1)


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


def build_command(
    command: int,
    sequence: int,
    waveform: int = 0,
    frequency_hz: int = 0,
    vpp_mv: int = 0,
    offset_mv: int = 0,
    monitor_enabled: bool = True,
) -> bytes:
    packet = bytearray(
        COMMAND.pack(
            COMMAND_MAGIC,
            VERSION,
            command,
            waveform,
            int(monitor_enabled),
            sequence,
            frequency_hz,
            vpp_mv,
            offset_mv,
            0,
            0,
        )
    )
    struct.pack_into("<H", packet, 22, crc16_ccitt(packet[:22]))
    return bytes(packet)


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
        "No pude elegir el puerto automáticamente. "
        f"Puertos detectados: {available}. Usá --port /dev/ttyUSB0."
    )


def open_serial(port: str, baud: int = BAUD) -> serial.Serial:
    selected = find_default_port() if port == "auto" else port
    connection = serial.Serial(selected, baudrate=baud, timeout=0.02)
    connection.reset_input_buffer()
    return connection


class MixedParser:
    """Separa respuestas GENR y capturas LPCS dentro del mismo flujo."""

    def __init__(self) -> None:
        self.buffer = bytearray()
        self.received_bytes = 0
        self.valid_messages = 0
        self.crc_errors = 0
        self.format_errors = 0
        self.discarded_bytes = 0

    def _next_magic(self) -> int:
        positions = [
            position
            for position in (
                self.buffer.find(STATUS_MAGIC),
                self.buffer.find(SCOPE_MAGIC),
            )
            if position >= 0
        ]
        return min(positions) if positions else -1

    def feed(self, data: bytes) -> list[GeneratorStatus | ScopeFrame]:
        self.received_bytes += len(data)
        self.buffer.extend(data)
        messages: list[GeneratorStatus | ScopeFrame] = []

        while True:
            position = self._next_magic()
            if position < 0:
                keep = min(len(self.buffer), 3)
                discarded = len(self.buffer) - keep
                if discarded:
                    del self.buffer[:discarded]
                    self.discarded_bytes += discarded
                break
            if position:
                del self.buffer[:position]
                self.discarded_bytes += position

            if self.buffer.startswith(STATUS_MAGIC):
                message = self._parse_status()
            else:
                message = self._parse_scope()

            if message is None:
                break
            if message is False:
                continue
            messages.append(message)
            self.valid_messages += 1

        return messages

    def _parse_status(self) -> GeneratorStatus | bool | None:
        if len(self.buffer) < STATUS.size:
            return None
        fields = STATUS.unpack_from(self.buffer)
        expected_crc = fields[-1]
        actual_crc = crc16_ccitt(memoryview(self.buffer)[:34])
        if fields[1] != VERSION or actual_crc != expected_crc:
            del self.buffer[0]
            if actual_crc != expected_crc:
                self.crc_errors += 1
            else:
                self.format_errors += 1
            return False

        del self.buffer[: STATUS.size]
        return GeneratorStatus(
            status=fields[2],
            waveform=fields[3],
            monitor_enabled=bool(fields[4]),
            sequence=fields[5],
            requested_frequency_hz=fields[6],
            actual_frequency_millihz=fields[7],
            vpp_mv=fields[8],
            offset_mv=fields[9],
            sample_count=fields[10],
            counter_value=fields[11],
            generation=fields[12],
            flags=fields[13],
        )

    def _parse_scope(self) -> ScopeFrame | bool | None:
        if len(self.buffer) < SCOPE.size:
            return None

        fields = SCOPE.unpack_from(self.buffer)
        (
            _magic,
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
            version == VERSION
            and header_size == SCOPE.size
            and sample_bits in (8, 12)
            and 0 < sample_count <= 4095
            and payload_length == sample_count * bytes_per_sample
            and capture_rate > 0
            and output_rate > 0
        )
        if not valid:
            del self.buffer[0]
            self.format_errors += 1
            return False

        frame_length = SCOPE.size + payload_length
        if len(self.buffer) < frame_length:
            return None

        header_without_crc = memoryview(self.buffer)[:30]
        payload = memoryview(self.buffer)[SCOPE.size:frame_length]
        actual_crc = crc16_ccitt(header_without_crc)
        actual_crc = crc16_ccitt(payload, actual_crc)
        if actual_crc != expected_crc:
            del payload
            del header_without_crc
            del self.buffer[0]
            self.crc_errors += 1
            return False

        if sample_bits == 12:
            samples = struct.unpack(f"<{sample_count}H", payload)
        else:
            samples = tuple(payload)
        del payload
        del header_without_crc
        del self.buffer[:frame_length]

        return ScopeFrame(
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


def estimate_frequency(samples: tuple[int, ...], sample_rate_hz: int) -> float | None:
    """Estima por cruces ascendentes y rechaza pulsos aislados del ADC."""
    if len(samples) < 8:
        return None
    low = min(samples)
    high = max(samples)
    if high - low < 8:
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
            crossings.append(index - 1 + fraction)
            armed = False
        previous = current

    if len(crossings) < 2:
        return None
    periods = [right - left for left, right in zip(crossings, crossings[1:])]
    median = sorted(periods)[len(periods) // 2]
    inliers = [period for period in periods if median * 0.65 <= period <= median * 1.35]
    if not inliers:
        return None
    return sample_rate_hz / (sum(inliers) / len(inliers))
