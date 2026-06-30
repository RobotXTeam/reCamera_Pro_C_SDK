[English](README.md) | [中文](README_zh.md)

# imu_read

通过 Linux IIO 子系统以 50 Hz 的频率读取 ICM-42670 IMU，连续采集 100 个样本。打印每次采样的加速度（m/s²）、陀螺仪（deg/s）和温度（°C）值表，并在最后输出最小值/最大值/平均值统计信息。当设备处于静止状态时，`accel_y` 的读数应约为 −9.7 m/s²（重力加速度），且陀螺仪各通道的值应接近零。

---

## 依赖要求

- 不需要停止任何服务。
- IIO 设备必须存在：运行 `ls /sys/bus/iio/devices/` 应能看到 `iio:device0` 或 `iio:device1`。

## 硬件要求

ICM-42670 IMU 已经集成在 reCamera 板上。无需任何外部硬件。

---

## 预期输出

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

具体数值取决于设备的物理朝向和环境温度。

---

## 编译方法

```bash
./scripts/build.sh release --examples
```

二进制文件：`build-aarch64/examples/imu_read/imu_read`

---

## 部署与运行方法

```bash
scp build-aarch64/examples/imu_read/imu_read root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/imu_read"
```

该演示程序运行约 2 秒钟（以 50 Hz 采集 100 个样本）后自动退出。按 Ctrl+C 可提前停止。

---

## 核心代码解析

**IMU 初始化** — `init()` 函数会自动检测 IIO 设备节点（`/dev/iio:device0` 或 `iio:device1`）：

```cpp
rv1126b::IMU imu;
auto init_res = imu.init();
if (!init_res.ok()) {
    fprintf(stderr, "IMU init failed: %s\n",
            std::string(rv1126b::to_string(init_res.error())).c_str());
    return EXIT_FAILURE;
}
```

**采样循环** — `read()` 返回一个包含所有六轴数据和温度的结构体。循环会在每个 20 毫秒周期的剩余时间内休眠，以保持稳定的 50 Hz 采样率：

```cpp
constexpr auto SAMPLE_INTERVAL = std::chrono::milliseconds(20);  // 50 Hz
while (sample < 100) {
    auto t0 = std::chrono::steady_clock::now();
    auto res = imu.read();
    const auto& d = *res;
    // ... 处理 d.accel_x, d.accel_y, d.accel_z, d.gyro_x, d.gyro_y, d.gyro_z, d.temperature
    auto elapsed = std::chrono::steady_clock::now() - t0;
    if (elapsed < SAMPLE_INTERVAL)
        std::this_thread::sleep_for(SAMPLE_INTERVAL - elapsed);
}
```

**统计信息** — 简单的运行中极小值/极大值/求和累加器即可生成最终的统计摘要，无需将所有 100 个样本都存储在内存中。

---

## 故障排查

| 现象 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `IMU init failed` | IIO 设备不存在 | 运行 `ls /sys/bus/iio/devices/` 和 `ls /dev/iio*`；如果为空，可能是 IIO 驱动未加载（尝试运行 `modprobe icm42670` 或检查 `dmesg \| grep icm`） |
| 所有加速度值均为 0.0 | IIO 缓冲区未启用或通道名称错误 | 检查 `cat /sys/bus/iio/devices/iio\:device0/name` 以确认设备是 ICM-42670 |
| `accel_y` 是正数 ~+9.8 而不是 −9.7 | 板子安装颠倒 | 正常现象：正负号取决于设备的物理朝向 |
| 采样率低于 50 Hz | 系统负载过高 | 减少其他正在运行的进程；演示程序使用 `sleep_for` 来补偿读取延迟 |
