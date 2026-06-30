[English](README.md) | [中文](README_zh.md)

# gpio_control

演示 GPIO 输出（LED 以 1 Hz 频率闪烁）和 GPIO 输入（电平变化轮询）。使用 GPIO2_PB6（全局引脚 78）作为输出，GPIO0_PA0（引脚 0）作为输入。包含一个优雅的 Ctrl+C 处理程序用于停止循环并释放 GPIO。

---

## 预置条件

- 不需要停止任何服务。
- GPIO sysfs 接口必须可用：`ls /sys/class/gpio/` 应该列出 `export` 和 `unexport`。

## 硬件要求

- **输出演示**：一个带有限流电阻的 LED（或万用表/示波器）连接到 GPIO2_PB6（全局引脚 78）。
- **输入演示**：任何逻辑电平信号或开关连接到 GPIO0_PA0（全局引脚 0）。演示运行 5 秒钟并打印每次的电平变化；在初步测试时可以允许引脚悬空。

### GPIO 引脚号计算

```
全局引脚号 = controller × 32 + bank_offset × 8 + bit
GPIO2_PB6  = 2 × 32 + 1 × 8 + 6 = 78
GPIO0_PA0  = 0 × 32 + 0 × 8 + 0 = 0
```

验证设备上可用的 GPIO：

```bash
cat /sys/kernel/debug/gpio
ls /sys/class/gpio/
```

---

## 预期输出

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

## 如何编译

```bash
./scripts/build.sh release --examples
```

二进制文件：`build-aarch64/examples/gpio_control/gpio_control`

---

## 如何部署和运行

```bash
scp build-aarch64/examples/gpio_control/gpio_control root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/gpio_control"
```

---

## 关键代码解读

**GPIO 构造与方向** — GPIO 类接受全局引脚号和明确的方向：

```cpp
rv1126b::GPIO led(78);
led.open(rv1126b::GPIO::Direction::Output);
```

**设置与清除** — `set(1)` 驱动引脚拉高；`set(0)` 驱动其拉低：

```cpp
led.set(1);
std::this_thread::sleep_for(std::chrono::milliseconds(500));
led.set(0);
std::this_thread::sleep_for(std::chrono::milliseconds(500));
```

**输入轮询** — `get()` 以 `Result<int>` 的形式返回当前电平：

```cpp
rv1126b::GPIO input_gpio(0);
input_gpio.open(rv1126b::GPIO::Direction::Input);
auto level_res = input_gpio.get();
if (level_res.ok())
    printf("Level: %s\n", *level_res ? "HIGH" : "LOW");
```

**信号处理** — 一个原子标志可以实现干净的退出，而不会使 GPIO 保持在导出状态：

```cpp
static std::atomic<bool> g_stop{false};
std::signal(SIGINT, [](int){ g_stop.store(true); });
```

退出前始终调用 `led.close()` 以取消导出 GPIO 并释放 sysfs 条目。

---

## 故障排除

| 症状 | 可能的原因 | 解决方法 |
|---------|-------------|-----|
| `open GPIO pin 78 failed` | 该引脚在此主板型号上不可用 | 尝试引脚 0 或检查 `cat /sys/kernel/debug/gpio` |
| `open GPIO pin 78 failed: permission denied` | root 无法访问 sysfs GPIO | 验证您是否以 root 身份运行：`whoami` |
| LED 不闪烁但没有报错 | 引脚正确，但 LED 极性或电阻接线存在问题 | 检查硬件连接；使用万用表进行探测 |
| 输入始终读取为 LOW（低电平） | 输入悬空 | 连接上拉电阻或使用逻辑信号驱动该引脚 |
