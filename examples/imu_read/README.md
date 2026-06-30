[English](README.md) | [中文](README_zh.md)

# imu_read

Reads the ICM-42670 IMU via the Linux IIO subsystem at 50 Hz for 100 samples. Prints a table of accelerometer (m/s²), gyroscope (deg/s), and temperature (°C) values for each sample, followed by min/max/average statistics. The device is at rest, so accel_y should read approximately −9.7 m/s² (gravity), and gyro channels should be near zero.

---

## Prerequisites

- No services need to be stopped.
- The IIO device must exist: `ls /sys/bus/iio/devices/` should show `iio:device0` or `iio:device1`.

## Hardware Required

The ICM-42670 IMU is integrated on the reCamera board. No external hardware is needed.

---

## Expected Output

```
=== IMU data read demo ===

IMU initialized, starting 50 Hz sampling for 100 samples...
Sample#   accel_x  accel_y  accel_z   gyro_x   gyro_y   gyro_z  temp(°C)
------------------------------------------------------------------------
[   1]    0.042   -9.712    0.187     0.003   -0.002    0.001    28.50
[   2]    0.039   -9.698    0.191     0.002   -0.001    0.002    28.51
...
[ 100]    0.041   -9.705    0.189     0.003   -0.002    0.001    28.52

=== Summary (100 samples) ===
Channel         Min        Max        Avg
----------------------------------------------
accel_x (g)   0.0380     0.0450     0.0412
accel_y (g)  -9.7200    -9.6900    -9.7050
accel_z (g)   0.1820     0.1950     0.1882
gyro_x (°/s)  0.0010     0.0050     0.0028
gyro_y (°/s) -0.0030     0.0000    -0.0015
gyro_z (°/s)  0.0000     0.0030     0.0012
temp (°C)    28.4900    28.5300    28.5100

Program exit.
```

The exact values depend on the device orientation and ambient temperature.

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/imu_read/imu_read`

---

## How to Deploy and Run

```bash
scp build-aarch64/examples/imu_read/imu_read root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/imu_read"
```

The demo runs for approximately 2 seconds (100 samples at 50 Hz) and exits automatically. Press Ctrl+C to stop early.

---

## Key Code Walkthrough

**IMU initialization** — `init()` auto-detects the IIO device node (`/dev/iio:device0` or `iio:device1`):

```cpp
rv1126b::IMU imu;
auto init_res = imu.init();
if (!init_res.ok()) {
    fprintf(stderr, "IMU init failed: %s\n",
            std::string(rv1126b::to_string(init_res.error())).c_str());
    return EXIT_FAILURE;
}
```

**Sampling loop** — `read()` returns a struct containing all six axes plus temperature. The loop sleeps for the remaining time within each 20 ms period to maintain a stable 50 Hz rate:

```cpp
constexpr auto SAMPLE_INTERVAL = std::chrono::milliseconds(20);  // 50 Hz
while (sample < 100) {
    auto t0 = std::chrono::steady_clock::now();
    auto res = imu.read();
    const auto& d = *res;
    // ... process d.accel_x, d.accel_y, d.accel_z, d.gyro_x, d.gyro_y, d.gyro_z, d.temperature
    auto elapsed = std::chrono::steady_clock::now() - t0;
    if (elapsed < SAMPLE_INTERVAL)
        std::this_thread::sleep_for(SAMPLE_INTERVAL - elapsed);
}
```

**Statistics** — A simple running min/max/sum accumulator produces the final summary without storing all 100 samples in memory.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `IMU init failed` | IIO device not present | Run `ls /sys/bus/iio/devices/` and `ls /dev/iio*`; if empty, the IIO driver may not be loaded (`modprobe icm42670` or check `dmesg \| grep icm`) |
| All accel values are 0.0 | IIO buffer not enabled or wrong channel names | Check `cat /sys/bus/iio/devices/iio\:device0/name` to confirm the device is the ICM-42670 |
| accel_y is positive ~+9.8 instead of −9.7 | Board is mounted upside-down | Normal: the sign depends on the physical orientation of the device |
| Sample rate is slower than 50 Hz | High system load | Reduce other running processes; the demo uses `sleep_for` to compensate for read latency |
