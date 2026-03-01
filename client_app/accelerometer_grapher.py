"""
SensorGrapher - Real-time rolling plot for x, y, z, and magnitude sensor data.

Usage:
    grapher = SensorGrapher(max_samples=200, title="IMU Sensor Data")
    grapher.start()

    # Feed samples (e.g. from a thread or loop):
    grapher.add_sample(x=1.0, y=-0.5, z=9.8)

    grapher.stop()

Or use as a context manager:
    with SensorGrapher() as grapher:
        for sample in data_source:
            grapher.add_sample(**sample)
"""

import math
import threading
import time
import collections
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.gridspec import GridSpec


class AccelerometerGrapher:
    """
    Real-time rolling line graph for 3-axis sensor data (x, y, z) and magnitude.

    Parameters
    ----------
    max_samples : int
        Number of samples visible in the rolling window. Default 200.
    update_interval_ms : int
        Animation refresh rate in milliseconds. Default 50 (≈20 fps).
    title : str
        Window / figure title.
    figsize : tuple
        Matplotlib figure size (width, height) in inches.
    colors : dict
        Line colors keyed by 'x', 'y', 'z', 'mag'.
    ylim : tuple or None
        Fixed y-axis limits (min, max). None = auto-scale.
    """

    DEFAULT_COLORS = {"x": "#E74C3C", "y": "#2ECC71", "z": "#3498DB", "mag": "#F39C12"}

    def __init__(
        self,
        max_samples: int = 200,
        update_interval_ms: int = 50,
        title: str = "Sensor Data",
        figsize: tuple = (12, 8),
        colors: dict = None,
        ylim: tuple = None,
    ):
        self.max_samples = max_samples
        self.update_interval_ms = update_interval_ms
        self.title = title
        self.figsize = figsize
        self.colors = colors or self.DEFAULT_COLORS
        self.ylim = ylim

        # Thread-safe ring buffers
        self._lock = threading.Lock()
        self._buffers = {
            ch: collections.deque(maxlen=max_samples) for ch in ("x", "y", "z", "mag")
        }
        self._timestamps: collections.deque = collections.deque(maxlen=max_samples)
        self._sample_count = 0

        self._fig = None
        self._ani = None
        self._running = False

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def add_sample(self, x: float, y: float, z: float, mag: float = None):
        """
        Append a new sensor reading.

        Parameters
        ----------
        x, y, z : float
            Axis values (e.g. acceleration in m/s², magnetic field in µT, etc.)
        mag : float, optional
            Magnitude. If None it is computed as sqrt(x²+y²+z²).
        """
        if mag is None:
            mag = math.sqrt(x * x + y * y + z * z)
        with self._lock:
            self._sample_count += 1
            self._timestamps.append(self._sample_count)
            self._buffers["x"].append(x)
            self._buffers["y"].append(y)
            self._buffers["z"].append(z)
            self._buffers["mag"].append(mag)

    def start(self):
        """
        Open the plot window and start the animation.

        This call BLOCKS the calling thread (required on macOS and most GUI
        backends — the event loop must run on the main thread).

        Feed data from a background thread via add_sample() while this runs:

            def producer():
                while True:
                    grapher.add_sample(x, y, z)
                    time.sleep(0.02)

            t = threading.Thread(target=producer, daemon=True)
            t.start()
            grapher.start()   # blocks here; close the window to exit
        """
        self._running = True
        self._build_and_run()

    def stop(self):
        """Close the plot window programmatically."""
        self._running = False
        if self._fig is not None:
            plt.close(self._fig)

    # Context-manager support — producer thread must be started separately
    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.stop()

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def _build_and_run(self):
        plt.style.use("dark_background")
        self._fig = plt.figure(figsize=self.figsize)
        self._fig.suptitle(self.title, fontsize=14, fontweight="bold", color="white")

        gs = GridSpec(4, 1, figure=self._fig, hspace=0.55)

        channels = ("x", "y", "z", "mag")
        labels = ("X Axis", "Y Axis", "Z Axis", "Magnitude")
        self._axes = {}
        self._lines = {}

        for i, (ch, label) in enumerate(zip(channels, labels)):
            ax = self._fig.add_subplot(gs[i])
            ax.set_ylabel(label, color=self.colors[ch], fontsize=9)
            ax.tick_params(colors="gray", labelsize=7)
            ax.spines[:].set_color("#444444")
            ax.set_facecolor("#111111")
            if self.ylim:
                ax.set_ylim(*self.ylim)
            # Only show x-axis label on the bottom subplot
            if i < len(channels) - 1:
                ax.set_xticklabels([])
            else:
                ax.set_xlabel("Sample #", color="gray", fontsize=8)

            (line,) = ax.plot([], [], color=self.colors[ch], linewidth=1.2, antialiased=True)
            self._axes[ch] = ax
            self._lines[ch] = line

        self._ani = animation.FuncAnimation(
            self._fig,
            self._update_plot,
            interval=self.update_interval_ms,
            blit=True,
            cache_frame_data=False,
        )

        plt.show()  # blocks until window closed
        self._running = False

    def _update_plot(self, _frame):
        with self._lock:
            ts = list(self._timestamps)
            data = {ch: list(self._buffers[ch]) for ch in ("x", "y", "z", "mag")}

        updated = []
        for ch in ("x", "y", "z", "mag"):
            line = self._lines[ch]
            ax = self._axes[ch]
            if ts:
                line.set_data(ts, data[ch])
                ax.set_xlim(ts[0], ts[-1] + 1)
                if not self.ylim and data[ch]:
                    margin = (max(data[ch]) - min(data[ch])) * 0.1 or 0.5
                    ax.set_ylim(min(data[ch]) - margin, max(data[ch]) + margin)
            updated.append(line)

        return updated

