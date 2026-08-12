#!/usr/bin/env python3
"""Control y monitor en tiempo real del generador LPC1769."""

from __future__ import annotations

import argparse
from collections import deque
import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk

from protocol import (
    BAUD,
    COMMAND_GET,
    COMMAND_SET,
    FLAG_ADC_OVERRUN,
    FLAG_DMA_ERROR,
    GeneratorStatus,
    MixedParser,
    ScopeFrame,
    STATUS_TEXT,
    WAVE_NAMES,
    build_command,
    estimate_frequency,
    open_serial,
)


class SerialWorker(threading.Thread):
    def __init__(self, port: str, baud: int) -> None:
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.stop_event = threading.Event()
        self.outgoing: queue.Queue[bytes] = queue.Queue()
        self.lock = threading.Lock()
        self.latest_status: GeneratorStatus | None = None
        self.latest_scope: ScopeFrame | None = None
        self.error: str | None = None
        self.parser = MixedParser()
        self.frame_times: deque[float] = deque(maxlen=100)
        self.connection = None

    def send(self, packet: bytes) -> None:
        self.outgoing.put(packet)

    def run(self) -> None:
        try:
            self.connection = open_serial(self.port, self.baud)
            while not self.stop_event.is_set():
                while True:
                    try:
                        packet = self.outgoing.get_nowait()
                    except queue.Empty:
                        break
                    self.connection.write(packet)
                    self.connection.flush()

                chunk = self.connection.read(4096)
                if not chunk:
                    continue
                for message in self.parser.feed(chunk):
                    with self.lock:
                        if isinstance(message, GeneratorStatus):
                            self.latest_status = message
                        else:
                            self.latest_scope = message
                            self.frame_times.append(time.monotonic())
        except Exception as exc:
            if not self.stop_event.is_set():
                self.error = str(exc)
        finally:
            if self.connection is not None:
                self.connection.close()

    def stop(self) -> None:
        self.stop_event.set()

    def snapshot(self):
        with self.lock:
            status = self.latest_status
            scope = self.latest_scope
            times = tuple(self.frame_times)
        fps = 0.0
        if len(times) > 1 and times[-1] > times[0]:
            fps = (len(times) - 1) / (times[-1] - times[0])
        return status, scope, fps


class GeneratorApp:
    def __init__(self, root: tk.Tk, worker: SerialWorker, vref: float) -> None:
        self.root = root
        self.worker = worker
        self.vref = vref
        self.command_sequence = 0
        self.last_status_sequence: int | None = None
        self.last_scope_sequence: int | None = None
        self.last_drawn: tuple[ScopeFrame, tuple[int, ...], tuple[float, ...]] | None = None

        root.title("LPC1769 - generador de funciones")
        root.geometry("1180x760")
        root.minsize(850, 560)

        controls = ttk.LabelFrame(root, text="Generador", padding=8)
        controls.pack(fill=tk.X, padx=8, pady=(8, 4))

        ttk.Label(controls, text="Forma:").grid(row=0, column=0, sticky=tk.W)
        self.waveform = tk.StringVar(value=WAVE_NAMES[0])
        ttk.Combobox(
            controls,
            width=22,
            state="readonly",
            values=WAVE_NAMES,
            textvariable=self.waveform,
        ).grid(row=0, column=1, padx=(4, 16))

        ttk.Label(controls, text="Frecuencia [Hz]:").grid(row=0, column=2)
        self.frequency = tk.StringVar(value="20000")
        ttk.Entry(controls, width=10, textvariable=self.frequency).grid(
            row=0, column=3, padx=(4, 16)
        )

        ttk.Label(controls, text="Amplitud Vpp [mV]:").grid(row=0, column=4)
        self.vpp = tk.StringVar(value="2800")
        ttk.Entry(controls, width=8, textvariable=self.vpp).grid(
            row=0, column=5, padx=(4, 16)
        )

        ttk.Label(controls, text="Offset [mV]:").grid(row=0, column=6)
        self.offset = tk.StringVar(value="1650")
        ttk.Entry(controls, width=8, textvariable=self.offset).grid(
            row=0, column=7, padx=(4, 16)
        )

        self.monitor = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            controls, text="Monitor ADC", variable=self.monitor
        ).grid(row=0, column=8, padx=(0, 16))
        ttk.Button(controls, text="Aplicar", command=self.apply).grid(row=0, column=9)

        scope_controls = ttk.Frame(controls)
        scope_controls.grid(row=1, column=0, columnspan=10, sticky=tk.W, pady=(9, 0))
        self.trigger_enabled = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            scope_controls, text="Trigger ascendente", variable=self.trigger_enabled
        ).pack(side=tk.LEFT)
        ttk.Label(scope_controls, text="Histéresis [mV]:").pack(
            side=tk.LEFT, padx=(14, 4)
        )
        self.hysteresis_mv = tk.StringVar(value="100")
        ttk.Entry(scope_controls, width=6, textvariable=self.hysteresis_mv).pack(
            side=tk.LEFT
        )
        self.fractional_trigger = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            scope_controls,
            text="Alineación submuestra",
            variable=self.fractional_trigger,
        ).pack(side=tk.LEFT, padx=(14, 0))
        ttk.Label(scope_controls, text="Ventana [us]:").pack(
            side=tk.LEFT, padx=(18, 4)
        )
        self.window_us = tk.StringVar(value="500")
        ttk.Combobox(
            scope_controls,
            width=8,
            values=(100, 200, 500, 1000, 2000, 2700),
            textvariable=self.window_us,
        ).pack(side=tk.LEFT)

        self.generator_info = ttk.Label(root, padding=(10, 4), anchor=tk.W)
        self.generator_info.pack(fill=tk.X)
        self.canvas = tk.Canvas(root, background="#101418", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
        self.scope_info = ttk.Label(root, padding=(10, 5), anchor=tk.W)
        self.scope_info.pack(fill=tk.X)

        root.bind("<Return>", lambda _event: self.apply())
        root.bind("<Escape>", lambda _event: self.close())
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(100, self.request_current)
        root.after(30, self.update)

    def next_sequence(self) -> int:
        self.command_sequence = (self.command_sequence + 1) & 0xFFFFFFFF
        return self.command_sequence

    def request_current(self) -> None:
        self.worker.send(build_command(COMMAND_GET, self.next_sequence()))

    def apply(self) -> None:
        try:
            waveform = WAVE_NAMES.index(self.waveform.get())
            frequency = int(self.frequency.get())
            vpp = int(self.vpp.get())
            offset = int(self.offset.get())
        except ValueError:
            messagebox.showerror("Configuración inválida", "Ingresá valores enteros.")
            return

        maximum_frequency = 250_000 if waveform == 1 else 100_000
        high = offset + (vpp + 1) // 2
        low = offset - vpp // 2
        if not 1 <= frequency <= maximum_frequency:
            messagebox.showerror(
                "Frecuencia inválida",
                f"Para esta forma el rango aceptado es 1 a {maximum_frequency} Hz.",
            )
            return
        if vpp < 0 or offset < 0 or low < 0 or high > 3297:
            messagebox.showerror(
                "Nivel inválido",
                "La onda completa debe quedar entre 0 y 3297 mV.",
            )
            return

        self.worker.send(
            build_command(
                COMMAND_SET,
                self.next_sequence(),
                waveform,
                frequency,
                vpp,
                offset,
                self.monitor.get(),
            )
        )

    def close(self) -> None:
        self.worker.stop()
        self.root.destroy()

    def select_window(self, frame: ScopeFrame, status: GeneratorStatus):
        try:
            requested_us = max(10.0, float(self.window_us.get()))
            hysteresis_v = max(0.0, float(self.hysteresis_mv.get()) / 1000.0)
        except ValueError:
            requested_us = 500.0
            hysteresis_v = 0.1

        count = min(
            len(frame.samples),
            max(8, round(requested_us * frame.output_rate_hz / 1_000_000)),
        )
        samples = frame.samples
        start = 0
        trigger_position: float | None = None
        pretrigger = count // 10

        if self.trigger_enabled.get() and len(samples) > count:
            maximum_code = (1 << frame.sample_bits) - 1
            threshold = status.offset_mv / 1000.0 * maximum_code / self.vref
            arm_level = max(
                0.0, threshold - hysteresis_v * maximum_code / self.vref
            )
            last_start = len(samples) - count
            armed = any(sample <= arm_level for sample in samples[:pretrigger])

            for index in range(max(1, pretrigger), len(samples)):
                if samples[index - 1] <= arm_level:
                    armed = True
                stays_high = (
                    index + 2 < len(samples)
                    and samples[index + 1] > arm_level
                    and samples[index + 2] > arm_level
                )
                if not (
                    armed
                    and samples[index - 1] < threshold <= samples[index]
                    and samples[index] != samples[index - 1]
                    and stays_high
                ):
                    continue

                fraction = (threshold - samples[index - 1]) / (
                    samples[index] - samples[index - 1]
                )
                detected = index - 1 + fraction
                if not self.fractional_trigger.get():
                    detected = float(index)
                candidate = int(detected) - pretrigger
                if candidate > last_start:
                    break
                if candidate >= 0:
                    start = candidate
                    trigger_position = detected
                    break

            if trigger_position is None:
                return None

        selected = samples[start : start + count]
        if trigger_position is None:
            times = tuple(i * 1_000_000 / frame.output_rate_hz for i in range(count))
        else:
            times = tuple(
                (start + i - trigger_position) * 1_000_000 / frame.output_rate_hz
                for i in range(count)
            )
        return selected, times, trigger_position

    def draw(self, frame: ScopeFrame, status: GeneratorStatus) -> bool:
        selection = self.select_window(frame, status)
        if selection is None:
            return False
        samples, times_us, trigger_position = selection
        if len(samples) < 2:
            return False

        self.last_drawn = (frame, samples, times_us)
        canvas = self.canvas
        canvas.delete("all")
        width = max(canvas.winfo_width(), 100)
        height = max(canvas.winfo_height(), 100)
        left, right, top, bottom = 64, 18, 20, 44
        plot_width = max(1, width - left - right)
        plot_height = max(1, height - top - bottom)
        x_min, x_max = times_us[0], times_us[-1]
        if x_max <= x_min:
            x_max = x_min + 1.0

        for index in range(6):
            y = top + plot_height * index / 5
            voltage = self.vref * (5 - index) / 5
            canvas.create_line(left, y, left + plot_width, y, fill="#29343d")
            canvas.create_text(
                left - 8, y, text=f"{voltage:.2f} V", fill="#aebbc5", anchor=tk.E
            )
        for index in range(6):
            x = left + plot_width * index / 5
            value = x_min + (x_max - x_min) * index / 5
            canvas.create_line(x, top, x, top + plot_height, fill="#29343d")
            canvas.create_text(
                x, top + plot_height + 20, text=f"{value:.0f}", fill="#aebbc5"
            )
        canvas.create_text(
            left + plot_width / 2,
            height - 8,
            text="tiempo [us]",
            fill="#aebbc5",
        )

        maximum_code = (1 << frame.sample_bits) - 1

        def x_pixel(value: float) -> float:
            return left + (value - x_min) * plot_width / (x_max - x_min)

        def y_pixel(value: int) -> float:
            return top + (maximum_code - value) * plot_height / maximum_code

        # Sample and hold real: tramo horizontal y luego transición vertical.
        points: list[float] = [x_pixel(times_us[0]), y_pixel(samples[0])]
        for index in range(1, len(samples)):
            x = x_pixel(times_us[index])
            points.extend((x, y_pixel(samples[index - 1]), x, y_pixel(samples[index])))
        canvas.create_line(
            *points,
            fill="#55e06f",
            width=2,
            smooth=False,
            capstyle=tk.BUTT,
            joinstyle=tk.MITER,
        )
        if trigger_position is not None and x_min <= 0.0 <= x_max:
            x = x_pixel(0.0)
            canvas.create_line(
                x, top, x, top + plot_height, fill="#ff665c", dash=(5, 4)
            )
        return True

    def update(self) -> None:
        if self.worker.error is not None:
            self.scope_info.configure(text=f"Error serie: {self.worker.error}")
            self.root.after(250, self.update)
            return

        status, frame, fps = self.worker.snapshot()
        if status is not None and status.sequence != self.last_status_sequence:
            self.last_status_sequence = status.sequence
            if status.status == 0:
                self.waveform.set(WAVE_NAMES[status.waveform])
                self.frequency.set(str(status.requested_frequency_hz))
                self.vpp.set(str(status.vpp_mv))
                self.offset.set(str(status.offset_mv))
                self.monitor.set(status.monitor_enabled)
            status_text = STATUS_TEXT.get(status.status, f"error {status.status}")
            actual = status.actual_frequency_hz
            requested = status.requested_frequency_hz
            error_ppm = 0.0 if requested == 0 else (actual - requested) * 1e6 / requested
            alerts = []
            if status.flags & 1:
                alerts.append("error DMA")
            if status.flags & 2:
                alerts.append("desborde RX UART")
            alert_text = ", ".join(alerts) if alerts else "sin errores"
            self.generator_info.configure(
                text=(
                    f"{status_text} | {WAVE_NAMES[status.waveform]} | "
                    f"{actual:.3f} Hz ({error_ppm:+.0f} ppm) | "
                    f"{status.sample_count} muestras/período | "
                    f"DAC {status.update_rate_hz / 1000:.3f} kS/s | "
                    f"{status.vpp_mv} mVpp, offset {status.offset_mv} mV | "
                    f"generación {status.generation} | {alert_text}"
                )
            )
            if not status.monitor_enabled:
                self.scope_info.configure(
                    text="Monitor ADC desactivado. La última captura queda congelada."
                )

        if (
            status is not None
            and status.monitor_enabled
            and frame is not None
            and frame.sequence != self.last_scope_sequence
        ):
            self.last_scope_sequence = frame.sequence
            trigger_locked = self.draw(frame, status)
            measured = estimate_frequency(frame.samples, frame.output_rate_hz)
            measured_text = "sin medida" if measured is None else f"{measured:.2f} Hz"
            maximum_code = (1 << frame.sample_bits) - 1
            minimum_v = min(frame.samples) * self.vref / maximum_code
            maximum_v = max(frame.samples) * self.vref / maximum_code
            alerts = []
            if frame.flags & FLAG_ADC_OVERRUN:
                alerts.append("OVERRUN ADC")
            if frame.flags & FLAG_DMA_ERROR:
                alerts.append("ERROR DMA")
            alert_text = ", ".join(alerts) if alerts else "captura válida"
            trigger_text = (
                "trigger libre"
                if not self.trigger_enabled.get()
                else "trigger LOCK" if trigger_locked else "trigger esperando"
            )
            self.scope_info.configure(
                text=(
                    f"ADC {frame.output_rate_hz / 1000:.3f} kS/s | "
                    f"medida {measured_text} | {minimum_v:.3f} a {maximum_v:.3f} V | "
                    f"{trigger_text} | {fps:.1f} tramas/s | {alert_text}"
                )
            )

        self.root.after(30, self.update)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Controla el generador LPC1769 y muestra su monitor ADC."
    )
    parser.add_argument("--port", default="auto", help="Puerto serie o 'auto'")
    parser.add_argument("--baud", type=int, default=BAUD)
    parser.add_argument("--vref", type=float, default=3.3)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    root = tk.Tk()
    worker = SerialWorker(args.port, args.baud)
    GeneratorApp(root, worker, args.vref)
    worker.start()
    root.mainloop()


if __name__ == "__main__":
    main()
