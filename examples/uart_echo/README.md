[English](README.md) | [中文](README_zh.md)

# uart_echo

Opens `/dev/ttyS2` at 115200 8N1 and echoes every received byte back to the sender. Uses a 100 ms non-blocking read timeout so the loop stays responsive to Ctrl+C. Prints RX/TX byte counts and a hex dump (first 16 bytes) for each transaction.

---

## Prerequisites

- No services need to be stopped.
- `/dev/ttyS2` must be accessible. If it is in use by another process, either stop that process or change `UART_DEV` in `main.cpp` to `/dev/ttyS4`.

## Hardware Required

- A serial loopback cable (TX→RX) connected to the board's UART2 pins, **or**
- A USB-to-serial adapter connected to a host PC with a terminal (e.g. `minicom`, `picocom`, `screen`).

---

## Expected Output

After launch, the demo waits for input. Send data from a second terminal:

```bash
# On the device (second shell):
echo -n 'hello' > /dev/ttyS2
```

Demo output:

```
=== UART echo demo ===
Device: /dev/ttyS2  Baud: 115200

Serial port opened, entering echo loop...
Run in another terminal: echo 'hello' > /dev/ttyS2
Press Ctrl+C to exit.

[RX    5 bytes] 68 65 6C 6C 6F
[TX    5 bytes] echo done

=== Stats ===
Total RX: 5 bytes
Total TX: 5 bytes
Program exit.
```

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/uart_echo/uart_echo`

---

## How to Deploy and Run

```bash
scp build-aarch64/examples/uart_echo/uart_echo root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/uart_echo"
```

---

## Key Code Walkthrough

**UART configuration** — All line parameters are set in `UART::Config` before calling `open()`:

```cpp
rv1126b::UART uart("/dev/ttyS2");
rv1126b::UART::Config cfg;
cfg.baud      = 115200;
cfg.data_bits = 8;
cfg.stop_bits = rv1126b::UART::StopBits::One;
cfg.parity    = rv1126b::UART::Parity::None;
uart.open(cfg);
```

**Non-blocking read with timeout** — `read()` returns 0 bytes on timeout rather than blocking indefinitely, which allows the signal handler to stop the loop promptly:

```cpp
auto read_res = uart.read(rx_buf.data(), rx_buf.size());
size_t n = *read_res;
if (n == 0) continue;  // timeout, try again
```

**Write** — Sends the received bytes back verbatim:

```cpp
auto write_res = uart.write(rx_buf.data(), n);
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open /dev/ttyS2 failed` | Port in use by another process (e.g. getty) | `fuser /dev/ttyS2` to identify the user; disable the getty service or try `/dev/ttyS4` |
| No RX data received | Wrong UART port or cable not connected | Verify physical connections; try a loopback test (short TX to RX) |
| Garbled characters received | Baud rate mismatch | Match the baud rate on both sides |
| `permission denied` on /dev/ttyS2 | Not running as root | Run with `sudo` or add the user to the `dialout` group |
