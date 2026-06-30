[English](README.md) | [中文](README_zh.md)

# uart_echo

以 115200 8N1 的配置打开 `/dev/ttyS2`，并将接收到的每个字节回显（echo）给发送方。使用 100 毫秒非阻塞读取超时，以便循环能够持续响应 Ctrl+C。为每次数据传输打印接收/发送（RX/TX）字节数和十六进制转储（前 16 个字节）。

---

## 预备知识

- 不需要停止任何服务。
- 必须能够访问 `/dev/ttyS2`。如果它正被其他进程使用，请停止该进程或将 `main.cpp` 中的 `UART_DEV` 更改为 `/dev/ttyS4`。

## 硬件要求

- 连接到开发板 UART2 引脚的串口环回线（TX→RX），**或**
- 连接到主机 PC 并配有终端程序（如 `minicom`、`picocom`、`screen`）的 USB 转串口适配器。

---

## 预期输出

启动后，该演示程序等待输入。从第二个终端发送数据：

```bash
# 在设备上（第二个 shell）：
echo -n 'hello' > /dev/ttyS2
```

演示程序输出：

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

## 编译方法

```bash
./scripts/build.sh release --examples
```

二进制文件路径：`build-aarch64/examples/uart_echo/uart_echo`

---

## 部署与运行方法

```bash
scp build-aarch64/examples/uart_echo/uart_echo root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/uart_echo"
```

---

## 核心代码解读

**UART 配置** — 在调用 `open()` 之前，所有线路参数均在 `UART::Config` 中设置：

```cpp
rv1126b::UART uart("/dev/ttyS2");
rv1126b::UART::Config cfg;
cfg.baud      = 115200;
cfg.data_bits = 8;
cfg.stop_bits = rv1126b::UART::StopBits::One;
cfg.parity    = rv1126b::UART::Parity::None;
uart.open(cfg);
```

**带超时的非阻塞读取** — 发生超时时，`read()` 返回 0 字节，而不是无限期阻塞，这使得信号处理程序能够及时停止循环：

```cpp
auto read_res = uart.read(rx_buf.data(), rx_buf.size());
size_t n = *read_res;
if (n == 0) continue;  // timeout, try again
```

**写入** — 将接收到的字节原封不动地发回：

```cpp
auto write_res = uart.write(rx_buf.data(), n);
```

---

## 故障排除

| 现象 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `open /dev/ttyS2 failed` | 端口被其他进程（如 getty）占用 | 使用 `fuser /dev/ttyS2` 找出占用用户；禁用 getty 服务或尝试 `/dev/ttyS4` |
| 没有接收到 RX 数据 | UART 端口错误或未连接线缆 | 检查物理连接；尝试进行环回测试（短接 TX 和 RX） |
| 收到乱码字符 | 波特率不匹配 | 使两端的波特率保持一致 |
| 对 /dev/ttyS2 `permission denied` | 未以 root 身份运行 | 使用 `sudo` 运行或将用户添加到 `dialout` 组 |
