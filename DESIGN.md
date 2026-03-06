# DESIGN.md — Sensor Acquisition & Processing Pipeline

## 1. Platform Assumptions

| Assumption | Value / Rationale |
|---|---|
| Word size | 32-bit (matches most ARM Cortex-M targets) |
| Host endianness | Little-endian by default; configurable via `ACCEL_VALUE_ENDIAN` macro (`0` = little, `1` = big) |
| Architecture | Bare-metal, no OS, no heap — all buffers statically allocated |
| Sample rate | 100 Hz — accelerometer is read every 4th ISR tick (`TIMER_TICKS_PER_ACCEL_SAMPLE = 4`) |
| I²C bus | Single-master, blocking read/write in driver context; up to `ACCEL_COMM_RETRY = 3` retries per transaction |
| Sample buffer can be small | 32 samples - The plotter is real-time, old samples missed loss importance quickly |

The program is structured as it would be on a Cortex-M: no dynamic allocation, no standard
library heap. All floating-point-like work is done with Q16.16 fixed-point integer math.

---

## 2. Data Formats & Fixed-Point Choices

### Sensitivity

**S = 2.44 mg/LSB** is derived directly from the sensor range and bit depth:

This maps the full positive half of the 12-bit signed range to the sensor's ±5 g span:

```
-2048 counts × 2.44 mg/LSB ≈ -5000 mg  (-5 g)
+2047 counts × 2.44 mg/LSB ≈ +4998 mg  (+5 g)
```

### 12-bit Unpacking

Each axis occupies a 16-bit little-endian word. Bits [15:12] are reserved and must be
masked. The raw value is sign-extended from 12 bits:

```c
int16_t accelerometer_parse_value(uint8_t l_byte, uint8_t h_byte) {
    int16_t value = ((int16_t)h_byte << 8) | (int16_t)l_byte; // LE assembly
    value = value & 0x0FFF;          // mask reserved bits [15:12]
    if (value & 0x0800)              // sign-extend from bit 11
        value -= 0x1000;
    return value;
}
```

Endianness is selected at compile time via `ACCEL_VALUE_ENDIAN` — `0` for little-endian
(default, matching the sensor), `1` for big-endian. The `#if` branch is resolved by the
preprocessor, so there is no runtime branch. Bytes are assembled with explicit shifts in
both cases, avoiding any struct casting or alignment-dependent behaviour.

### Q16.16 Fixed-Point

All axis values are converted to **Q16.16** (32-bit signed, 16 fractional bits) after
unpacking:

```
q_value = (int32_t)raw_counts * S_Q16
where S_Q16 = (int32_t)(2.4414 * 65536) = 160038
```

This gives sub-mg precision while staying in 32-bit integer arithmetic. Intermediate
products for `x² + y² + z²` are computed in 64-bit (`int64_t`) to prevent overflow before
the square root step.

### Magnitude (Integer Newton-Raphson √)

Magnitude is computed entirely in integer arithmetic using Newton-Raphson iteration:

```c
uint32_t isqrt32(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t x1;
    do {
        x1 = (x + n / x) >> 1;
    } while (x1 < x);
    return x;
}
```

This converges in ≤ 6 iterations for 32-bit inputs and requires no lookup tables or
floating-point. The result is the magnitude in Q16.16 mg units, which is then shifted right
by 16 to produce a whole-mg value for the output record.

---

## 3. ISR → Task Design & Queue Structure

### Timing Model

```
Timer ISR (400 Hz)
  │
  └─ Every 4th tick (100 Hz):
       i2c_mock_step()
       accelerometer_read_all() → sample_t
       queue_enqueue(&state.sample_queue, sample)
       state.sample_index++
                │
         app_run() — main super loop
           tcp_manage_connection()
           queue_dequeue() → sample
           app_do_work(): Filter × 3 axes → magnitude
           sample_serialize() → JSON string
           tcp_send()
```

### Ring Buffer

The queue is a **statically allocated ring buffer** of 32 entries using `head`, `tail`, and
`count` indices, all stored as `uint8_t` (wrapping naturally at 255, well above the 32-entry
capacity):

```c
#define QUEUE_SIZE (uint8_t)32

typedef struct {
    sample_t buffer[QUEUE_SIZE];
    uint8_t  capacity;
    uint8_t  head;     // read pointer (task advances this)
    uint8_t  tail;     // write pointer (ISR advances this)
    uint8_t  count;
} queue_t;
```

The ISR writes via `queue_enqueue()` (advances `tail`); the task reads via `queue_dequeue()`
(advances `head`). On a single-core bare-metal system the ISR cannot be preempted by the
task, so no mutex is required. On overflow, `queue_enqueue` caps `count` at `capacity` and
the oldest sample is silently overwritten — acceptable behaviour where stalling the ISR
would be worse than losing a sample.

A notable design point: `queue_get_tail_ptr()` is used in the ISR to obtain a pointer to the
next write slot **before** the read, allowing `accelerometer_read_all()` to fill the slot
in-place and avoid a redundant copy prior to `queue_enqueue()`.

### Memory Budget

| Structure | Size |
|---|---|
| Ring buffer (`queue_t`: 32 × `sizeof(sample_t)`) | ~640 bytes |
| Socket TX / RX buffers | ~512 bytes |
| Filter state (3 × `iir_filter_t`) | ~48 bytes |
| Driver + app state | ~64 bytes |
| Stack (estimated) | ~2 KB |
| **Total** | **< 4 KB** (well within 32 KB) |

---

## 4. Endianness & Bit Packing

- The sensor outputs **little-endian** 16-bit words. Endianness is handled at compile time
  via `ACCEL_VALUE_ENDIAN` in `accelerometer.h` — no runtime branch, no struct punning.
- Reserved bits [15:12] are masked with `& 0x0FFF` before sign extension to ensure
  correctness even if the mock device sets them non-zero.
- Sign extension is done explicitly (`value -= 0x1000` when bit 11 is set) rather than
  relying on implementation-defined right-shift behaviour on signed types.
- The TCP wire format carries JSON text, so endianness is not a concern on the wire; numeric
  values are serialised as decimal strings.

---

## 5. Error Handling

### I²C Errors

Both `accelerometer_read_reg` and `accelerometer_write_reg` retry the I²C transaction up to
`ACCEL_COMM_RETRY = 3` times before returning `STATUS_ERROR_I2C`. This handles transient
bus glitches without stalling the ISR for long. Register address validation is also performed
upfront — an invalid register returns `STATUS_ERROR_INVALID_DATA` immediately without
consuming retry budget.

If all retries fail on a sample read in `accelerometer_read_all()`, the error propagates back
to `timer_isr()`, which returns early without enqueuing the sample — the slot is simply
skipped.

### WHO_AM_I Check

On startup, `app_init()` calls `accelerometer_verify()` which reads register `0x00` and
compares the result against `ACCEL_I_AM = 0x42`. If the value doesn't match or the read
fails, `app_init()` returns early without completing TCP setup — the system will not stream
data until a valid device is present.

### Socket Errors

`app_run()` calls `tcp_manage_connection()` on every loop iteration before attempting any
send. If the connection is not up (`state.tcp.connected == false`), the dequeue and send are
skipped entirely for that tick — the sample remains in the queue and will be sent once the
connection is restored. This means samples can accumulate in the ring buffer during a
disconnect, bounded by the queue's 32-entry capacity.

---

## 6. Low-Pass Filter (IIR / EMA)

A first-order IIR exponential moving average is applied **per axis** before magnitude
computation, in `app_do_work()`:


Each filter is initialised with `FIXED_FROM_FLOAT(IIR_FILTER_ALPHA)` and the configured
`IIR_FILTER_MODE`. The filter operates entirely in Q16.16 fixed-point arithmetic:

```
filtered = alpha × raw + (1 − alpha) × filtered_prev
```

With `alpha = 1/8` (a power of two, implemented as a right-shift to avoid a general
multiply), the −3 dB cutoff is approximately **14 Hz** at 100 Hz sample rate — enough to
attenuate high-frequency vibration noise while preserving motion dynamics. Magnitude is
computed on the filtered axes, so the output represents smoothed vector length.

---

## 7. Client Design (Python)

### Language & Libraries

| Concern | Choice |
|---|---|
| Language | Python 3 |
| Socket | `socket` (stdlib) |
| Plotting | `matplotlib` with `FuncAnimation` |
| Storage | `jsonlines` / stdlib `json` |
| Reconnect loop | `time.sleep` + exception catch |

Python was chosen for rapid development and matplotlib's excellent live-plot support.

### Socket & Protocol Framing

The firmware exposes a **TCP socket** (default `localhost:9000`). Each record is a
**fixed-size JSON object** terminated by a newline (`\n`), making framing trivial with
Python's `socket.makefile()`:

```json
{"index": 1042, "x": -312.123, "y": 88.342, "z": 981.129, "mag": 1034.388}\n
```

Using newline-delimited JSON means:
- No length-prefix parsing logic needed.
- Human-readable for debugging.
- Trivially parsed with `json.loads()`.

The client reads line-by-line; a truncated line (disconnect mid-record) is detected by
the absence of `\n` and discarded, then a reconnect is attempted.

### Reconnection

```python
while True:
    try:
        connect_and_stream()
    except (ConnectionRefusedError, OSError) as e:
        log(f"Disconnected: {e}, retrying in 2s...")
        time.sleep(2)
```

### Log File Format

Samples are appended to **`samples.txt`** (JSON Lines) — one JSON object per line.
This format is append-friendly (no need to re-write the file), trivially parseable by
pandas/jq for post-analysis, and human-readable.

### Real-Time Plotting

`matplotlib.animation.FuncAnimation` drives a 4-subplot figure updating at ~10 Hz:

- **X, Y, Z axis (mg)** — three separate line plots showing the last 500 samples.
- **Magnitude (mg)** — a fourth line plot on the same time axis.

The animation callback drains a `collections.deque(maxlen=500)` that is populated by the
socket-reading thread, so the network I/O and plot rendering never block each other.

---

## 8. What I'd Do Next (With More Time)

- **Extend unit testing** — Flush out unit test using [ZOMBIES](https://blog.wingman-sw.com/tdd-guided-by-zombies) 
 structure for each file. Also add parallel unit test for MACRO configurations.
- **Fix VS Code / Matplotlib Key Crash** - Deep in the matplotlib is a crash.
- **Latency analysis** — Instrument the ISR>task>socket path with a free-running 32-bit
  cycle counter to measure worst-case jitter and confirm the
  100 Hz deadline is always met.
- **Power management** — Gate the accelerometer between samples using the CTRL ENABLE
  bit and put the MCU into a WFI (Wait For Interrupt) sleep in the idle loop to reduce
  current draw between 100 Hz ticks.
- **Protocol hardening** — Add identity and even CRC. Weigh binary + CRC vs json text depending on the end use case.
- **Clean** - Clean and comment more
- **Plot Quality of Life** - Reduce jitter on rapid update and add more useful UI
