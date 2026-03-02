import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.dates as mdates
from collections import deque
from datetime import datetime
import threading
import math
import time

DARK_BG    = "#0d0f14"
PANEL_BG   = "#13161e"
GRID_COLOR = "#1e2330"
TEXT_COLOR = "#8892a4"
ACCENT     = "#e8eaf0"

CHANNEL_COLORS = {
    "x":   "#4fc3f7",
    "y":   "#81c784",
    "z":   "#ff8a65",
    "mag": "#ce93d8",
}

CHANNEL_LABELS = {
    "x":   "X [mg]",
    "y":   "Y [mg]",
    "z":   "Z [mg]",
    "mag": "Magnitude [mg]",
}

plt.rcParams.update({
    "font.family":       "monospace",
    "axes.facecolor":    PANEL_BG,
    "figure.facecolor":  DARK_BG,
    "axes.edgecolor":    GRID_COLOR,
    "axes.labelcolor":   TEXT_COLOR,
    "xtick.color":       TEXT_COLOR,
    "ytick.color":       TEXT_COLOR,
    "xtick.labelsize":   8,
    "ytick.labelsize":   8,
    "axes.labelsize":    9,
    "axes.titlesize":    9,
    "grid.color":        GRID_COLOR,
    "grid.linewidth":    0.6,
    "lines.linewidth":   1.6,
    "lines.antialiased": True,
})


class AccelPlotter:
    def __init__(self, window_seconds=5, refresh_hz=20):
        self.window           = window_seconds
        self.refresh_interval = 1.0 / refresh_hz
        self.data             = deque()
        self._lock            = threading.Lock()

        self.fig, self.axes = plt.subplots(
            4, 1, figsize=(11, 8), sharex=True,
            gridspec_kw={"hspace": 0.08, "left": 0.08, "right": 0.97,
                         "top": 0.93, "bottom": 0.10}
        )

        self.fig.patch.set_facecolor(DARK_BG)
        self.fig.suptitle(
            "ACCELEROMETER  MONITOR",
            color=ACCENT, fontsize=11, fontweight="bold",
            x=0.52, y=0.975
        )

        self.lines = {}
        self._fills = {}

        for ax, key in zip(self.axes, ("x", "y", "z", "mag")):
            color = CHANNEL_COLORS[key]
            line, = ax.plot([], [], color=color, linewidth=1.8, solid_capstyle="round")
            self.lines[key] = line

            fill = ax.fill_between([], [], alpha=0.08, color=color)
            self._fills[key] = fill

            ax.set_ylabel(CHANNEL_LABELS[key], color=color, labelpad=8,
                          fontsize=8, fontweight="bold")
            ax.tick_params(axis="y", length=3, width=0.6)
            ax.tick_params(axis="x", length=3, width=0.6)
            ax.grid(True, axis="both", alpha=0.6)
            ax.spines[["top", "right"]].set_visible(False)
            ax.spines["left"].set_color(GRID_COLOR)
            ax.spines["bottom"].set_color(GRID_COLOR)
            for spine in ax.spines.values():
                spine.set_linewidth(0.6)

        time_fmt = mdates.DateFormatter("%H:%M:%S")
        self.axes[-1].xaxis.set_major_formatter(time_fmt)
        self.axes[-1].xaxis.set_major_locator(
            mdates.AutoDateLocator(minticks=4, maxticks=8)
        )
        self.axes[-1].set_xlabel("Time", color=TEXT_COLOR, labelpad=8)
        plt.setp(self.axes[-1].xaxis.get_majorticklabels(), rotation=0, ha="center")

    def add_sample(self, sample):
        ts  = sample["timestamp"]
        mag_local = math.sqrt(sample["x"]**2 + sample["y"]**2 + sample["z"]**2)
        if 'mag' in sample:
            mag = sample['mag']
            # print(f"Mag difference: {mag - mag_local:.3f}")
        else:
            mag = mag_local
        dt  = datetime.fromtimestamp(ts) if isinstance(ts, (int, float)) else ts
        with self._lock:
            self.data.append({"dt": dt, "x": sample["x"],
                               "y": sample["y"], "z": sample["z"], "mag": mag})
            self._prune()
            if len(self.data) > 1 and self.data[-1]["dt"].replace(tzinfo=None).second != self.data[-2]["dt"].replace(tzinfo=None).second:
                dt1 = self.data[-2]["dt"].replace(tzinfo=None)
                dt2 = self.data[-1]["dt"].replace(tzinfo=None)
                update_rate = 1 / (dt2 - dt1).total_seconds()
                print(f"Update rate: {update_rate:.3f} Hz", flush=True)

    def _prune(self):
        if not self.data:
            return
        cutoff = mdates.num2date(
            mdates.date2num(self.data[-1]["dt"]) - self.window / 86400
        )
        while self.data and self.data[0]["dt"].replace(tzinfo=None) < cutoff.replace(tzinfo=None):
            self.data.popleft()

    def _animate(self, _frame):
        with self._lock:
            if len(self.data) < 2:
                return list(self.lines.values())
            dts      = [d["dt"] for d in self.data]
            snapshot = {k: [d[k] for d in self.data] for k in ("x", "y", "z", "mag")}

        for key, ax in zip(("x", "y", "z", "mag"), self.axes):
            self.lines[key].set_data(dts, snapshot[key])
            self._fills[key].remove()
            self._fills[key] = ax.fill_between(
                dts, snapshot[key],
                alpha=0.08, color=CHANNEL_COLORS[key]
            )
            ax.relim()
            ax.autoscale_view()

        return list(self.lines.values())

    def run(self):
        interval_ms = int(self.refresh_interval * 1000)
        self._anim  = animation.FuncAnimation(
            self.fig, self._animate,
            interval=interval_ms, blit=False, cache_frame_data=False
        )
        plt.show()


if __name__ == "__main__":
    import random
    plotter = AccelPlotter(window_seconds=5)

    def producer():
        while True:
            plotter.add_sample({
                "timestamp": time.time(),
                "x": math.sin(time.time() * 2)   + random.gauss(0, 0.1),
                "y": math.cos(time.time() * 1.3)  + random.gauss(0, 0.1),
                "z": 9.81                          + random.gauss(0, 0.05),
            })
            time.sleep(0.02)

    threading.Thread(target=producer, daemon=True).start()
    plotter.run()