[English](README.md) | [中文](README_zh.md)

# gpio_control

Demonstrates GPIO output (LED blink at 1 Hz) and GPIO input (level-change polling). Uses GPIO2_PB6 (global pin 78) as the output and GPIO0_PA0 (pin 0) as the input. A graceful Ctrl+C handler stops the loop and releases the GPIO.

---

## Prerequisites

- No services need to be stopped.
- GPIO sysfs interface must be available: `ls /sys/class/gpio/` should list `export` and `unexport`.

## Hardware Required

- **Output demo**: An LED with a current-limiting resistor (or a multimeter/oscilloscope) connected to GPIO2_PB6 (global pin 78).
- **Input demo**: Any logic-level signal or switch connected to GPIO0_PA0 (global pin 0). The demo runs for 5 seconds and prints each level change; leaving the pin floating is acceptable for initial testing.

### GPIO pin number calculation

```
Global pin = controller × 32 + bank_offset × 8 + bit
GPIO2_PB6  = 2 × 32 + 1 × 8 + 6 = 78
GPIO0_PA0  = 0 × 32 + 0 × 8 + 0 = 0
```

Verify available GPIO on the device:

```bash
cat /sys/kernel/debug/gpio
ls /sys/class/gpio/
```

---

## Expected Output

```
=== GPIO output demo: LED blink (pin 78) ===
GPIO pin 78 configured as output
Blinking, press Ctrl+C to stop...

  [ 1] LED ON
  [ 1] LED OFF
  [ 2] LED ON
  [ 2] LED OFF
  ...
  [10] LED ON
  [10] LED OFF
GPIO output demo done.

=== GPIO input demo: level detect (pin 0) ===
GPIO pin 0 configured as input, reading for 5 seconds...
  [0.00s] GPIO pin 0 level change -> LOW
GPIO input demo done.

Program exit.
```

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/gpio_control/gpio_control`

---

## How to Deploy and Run

```bash
scp build-aarch64/examples/gpio_control/gpio_control root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/gpio_control"
```

---

## Key Code Walkthrough

**GPIO construction and direction** — The GPIO class takes the global pin number and an explicit direction:

```cpp
rv1126b::GPIO led(78);
led.open(rv1126b::GPIO::Direction::Output);
```

**Set and clear** — `set(1)` drives the pin high; `set(0)` drives it low:

```cpp
led.set(1);
std::this_thread::sleep_for(std::chrono::milliseconds(500));
led.set(0);
std::this_thread::sleep_for(std::chrono::milliseconds(500));
```

**Input polling** — `get()` returns the current level as `Result<int>`:

```cpp
rv1126b::GPIO input_gpio(0);
input_gpio.open(rv1126b::GPIO::Direction::Input);
auto level_res = input_gpio.get();
if (level_res.ok())
    printf("Level: %s\n", *level_res ? "HIGH" : "LOW");
```

**Signal handler** — An atomic flag enables clean exit without leaving the GPIO exported:

```cpp
static std::atomic<bool> g_stop{false};
std::signal(SIGINT, [](int){ g_stop.store(true); });
```

Always call `led.close()` before exit to unexport the GPIO and release the sysfs entry.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open GPIO pin 78 failed` | Pin not available on this board variant | Try pin 0 or check `cat /sys/kernel/debug/gpio` |
| `open GPIO pin 78 failed: permission denied` | sysfs GPIO not accessible as root | Verify you are running as root: `whoami` |
| LED does not blink but no error | Pin is correct but LED polarity or resistor wiring issue | Check the hardware connection; probe with a multimeter |
| Input always reads LOW | Floating input | Connect a pull-up resistor or drive the pin with a logic signal |
