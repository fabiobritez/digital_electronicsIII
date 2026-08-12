#!/usr/bin/env python3
"""Visualizador en tiempo real, sin interpolacion ni suavizado."""

from __future__ import annotations

import argparse
from collections import deque
import threading
import time
import tkinter as tk
from tkinter import ttk

from protocol import (
    FLAG_ADC_OVERRUN,
    FLAG_CAPTURE_GAPS,
    FLAG_CAPTURE_LOSS,
    FLAG_DECIMATED,
    FLAG_DMA_ERROR,
    FrameParser,
    ScopeFrame,
    estimate_frequency,
    open_serial,
    serial_frames,
)


class SerialWorker(threading.Thread):
    def __init__(self, port: str, baud: int) -> None:
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.stop_event = threading.Event()
        self.lock = threading.Lock()
        self.latest: ScopeFrame | None = None
        self.error: str | None = None
        self.parser = FrameParser()
        self.frame_times: deque[float] = deque(maxlen=100)
        self.sequence_gaps = 0
        self.previous_sequence: int | None = None
        self.connection = None

    def run(self) -> None:
        try:
            self.connection = open_serial(self.port, self.baud)
            for frame in serial_frames(self.connection, self.parser):
                if self.stop_event.is_set():
                    break

                now = time.monotonic()
                with self.lock:
                    if self.previous_sequence is not None:
                        difference = (frame.sequence - self.previous_sequence) & 0xFFFFFFFF
                        if 1 < difference < 0x80000000:
                            self.sequence_gaps += difference - 1
                    self.previous_sequence = frame.sequence
                    self.latest = frame
                    self.frame_times.append(now)
        except Exception as exc:
            self.error = str(exc)
        finally:
            if self.connection is not None:
                self.connection.close()

    def stop(self) -> None:
        self.stop_event.set()
        if self.connection is not None:
            self.connection.close()

    def snapshot(self):
        with self.lock:
            frame = self.latest
            gaps = self.sequence_gaps
            times = tuple(self.frame_times)
        fps = 0.0
        if len(times) > 1 and times[-1] > times[0]:
            fps = (len(times) - 1) / (times[-1] - times[0])
        return frame, gaps, fps


class ScopeApp:
    def __init__(self, root: tk.Tk, worker: SerialWorker, vref: float,
                 initial_window_us: float) -> None:
        self.root = root
        self.worker = worker
        self.vref = vref
        self.last_sequence: int | None = None

        root.title("LPC1769 - ADC por UART")
        root.geometry("1100x700")
        root.minsize(760, 480)

        controls = ttk.Frame(root, padding=6)
        controls.pack(fill=tk.X)

        self.trigger_enabled = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            controls, text="Trigger ascendente", variable=self.trigger_enabled
        ).pack(side=tk.LEFT)

        ttk.Label(controls, text="Nivel [V]:").pack(side=tk.LEFT, padx=(12, 4))
        self.trigger_level_v = tk.DoubleVar(value=vref / 2.0)
        ttk.Entry(controls, width=6, textvariable=self.trigger_level_v).pack(side=tk.LEFT)

        ttk.Label(controls, text="Histéresis [mV]:").pack(side=tk.LEFT, padx=(12, 4))
        self.trigger_hysteresis_mv = tk.DoubleVar(value=100.0)
        ttk.Entry(
            controls, width=6, textvariable=self.trigger_hysteresis_mv
        ).pack(side=tk.LEFT)

        self.fractional_trigger = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            controls, text="Alineación submuestra", variable=self.fractional_trigger
        ).pack(side=tk.LEFT, padx=(12, 0))

        ttk.Label(controls, text="Ventana [us]:").pack(side=tk.LEFT, padx=(18, 4))
        self.window_us = tk.DoubleVar(value=initial_window_us)
        window_box = ttk.Combobox(
            controls,
            width=8,
            textvariable=self.window_us,
            values=(100, 200, 500, 1000, 2000, 5000),
        )
        window_box.pack(side=tk.LEFT)

        self.canvas = tk.Canvas(root, background="#101418", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        self.status = ttk.Label(root, padding=6, anchor=tk.W)
        self.status.pack(fill=tk.X)

        root.bind("<Escape>", lambda _event: self.close())
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(30, self.update)

    def close(self) -> None:
        self.worker.stop()
        self.root.destroy()

    def select_window(self, frame: ScopeFrame):
        samples = frame.samples
        requested = max(8, round(self.window_us.get() * frame.output_rate_hz / 1_000_000))
        count = min(len(samples), requested)
        pretrigger = count // 10
        trigger_position: float | None = None
        start = 0

        if self.trigger_enabled.get() and len(samples) > count:
            maximum_code = (1 << frame.sample_bits) - 1
            try:
                level_v = min(max(self.trigger_level_v.get(), 0.0), self.vref)
                hysteresis_v = max(self.trigger_hysteresis_mv.get(), 0.0) / 1000.0
            except tk.TclError:
                level_v = self.vref / 2.0
                hysteresis_v = 0.1

            threshold = level_v * maximum_code / self.vref
            arm_level = max(0.0, threshold - hysteresis_v * maximum_code / self.vref)
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
                crossing = (
                    armed
                    and samples[index - 1] < threshold <= samples[index]
                    and samples[index] != samples[index - 1]
                    and stays_high
                )
                if not crossing:
                    continue

                fraction = (threshold - samples[index - 1]) / (
                    samples[index] - samples[index - 1]
                )
                detected_position = (index - 1) + fraction
                if not self.fractional_trigger.get():
                    detected_position = float(index)

                candidate = int(detected_position) - pretrigger
                if candidate > last_start:
                    break
                if candidate >= 0:
                    trigger_position = detected_position
                    start = candidate
                    break

            if trigger_position is None:
                return None

        selected = samples[start:start + count]
        if trigger_position is None:
            times_us = tuple(i * 1_000_000 / frame.output_rate_hz for i in range(count))
        else:
            times_us = tuple(
                (start + i - trigger_position) * 1_000_000 / frame.output_rate_hz
                for i in range(count)
            )
        return selected, times_us, trigger_position

    def draw(self, frame: ScopeFrame) -> bool:
        selection = self.select_window(frame)
        if selection is None:
            return False
        samples, times_us, trigger_position = selection
        if len(samples) < 2:
            return False

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

        def x_pixel(time_us: float) -> float:
            return left + (time_us - x_min) * plot_width / (x_max - x_min)

        def y_pixel(value: int) -> float:
            return top + (maximum_code - value) * plot_height / maximum_code

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
            trigger_x = x_pixel(0.0)
            canvas.create_line(
                trigger_x,
                top,
                trigger_x,
                top + plot_height,
                fill="#ff665c",
                dash=(5, 4),
            )
        return True

    def update(self) -> None:
        if self.worker.error is not None:
            self.status.configure(text=f"Error serie: {self.worker.error}")
            return

        frame, sequence_gaps, fps = self.worker.snapshot()
        if frame is not None and frame.sequence != self.last_sequence:
            self.last_sequence = frame.sequence
            trigger_locked = self.draw(frame)

            frequency = estimate_frequency(frame.samples, frame.output_rate_hz)
            frequency_text = "sin medida" if frequency is None else f"{frequency / 1000:.2f} kHz"
            maximum_code = (1 << frame.sample_bits) - 1
            minimum_v = min(frame.samples) * self.vref / maximum_code
            maximum_v = max(frame.samples) * self.vref / maximum_code

            mode = "bloques 12 bit" if frame.flags & FLAG_CAPTURE_GAPS else "continuo 8 bit"
            alerts: list[str] = []
            if frame.flags & FLAG_ADC_OVERRUN:
                alerts.append("OVERRUN ADC")
            if frame.flags & FLAG_DMA_ERROR:
                alerts.append("ERROR DMA")
            if frame.flags & FLAG_CAPTURE_LOSS:
                alerts.append("bloque perdido")
            if frame.flags & FLAG_DECIMATED:
                alerts.append("decimado x3")
            alert_text = ", ".join(alerts) if alerts else "sin errores informados"
            if self.trigger_enabled.get():
                trigger_text = "trigger LOCK" if trigger_locked else "trigger esperando"
            else:
                trigger_text = "trigger libre"

            self.status.configure(
                text=(
                    f"{mode} | trama {frame.sequence} | ADC {frame.capture_rate_hz / 1000:.3f} kS/s "
                    f"| datos {frame.output_rate_hz / 1000:.3f} kS/s | {frame.sample_bits} bit "
                    f"| señal {frequency_text} | {minimum_v:.2f} a {maximum_v:.2f} V "
                    f"| {trigger_text} | {fps:.1f} tramas/s "
                    f"| saltos de secuencia {sequence_gaps} | {alert_text}"
                )
            )

        self.root.after(30, self.update)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Visualiza las capturas del LPC1769 con sample and hold."
    )
    parser.add_argument("--port", default="auto", help="Puerto serie o 'auto'")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--vref", type=float, default=3.3)
    parser.add_argument("--window-us", type=float, default=500.0)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    root = tk.Tk()
    worker = SerialWorker(args.port, args.baud)
    ScopeApp(root, worker, args.vref, args.window_us)
    worker.start()
    root.mainloop()


if __name__ == "__main__":
    main()
