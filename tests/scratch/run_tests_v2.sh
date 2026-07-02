#!/bin/bash
IP="192.168.4.200"
PASS="recamera.1"

echo "Copying SDK library..."
sshpass -p $PASS scp -o StrictHostKeyChecking=no lib/libreCamera_pro_sdk.so.1.0.0 root@$IP:/tmp/
sshpass -p $PASS ssh -o StrictHostKeyChecking=no root@$IP "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"

echo "Copying examples..."
sshpass -p $PASS scp -r -o StrictHostKeyChecking=no build-aarch64/examples root@$IP:/tmp/

cat << 'EOF' > test_on_device_v2.sh
#!/bin/sh
export LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib

run_with_timeout() {
    echo "--- test $1 ---"
    $2 &
    PID=$!
    sleep $3
    kill -9 $PID 2>/dev/null || true
    wait $PID 2>/dev/null || true
    echo "--- end $1 ---"
}

run_with_timeout "hello_world" "/tmp/examples/hello_world/hello_world" 3
run_with_timeout "imu_read" "/tmp/examples/imu_read/imu_read" 3
run_with_timeout "gpio_control" "/tmp/examples/gpio_control/gpio_control" 3
run_with_timeout "uart_echo" "/tmp/examples/uart_echo/uart_echo" 3

echo "--- test isp_control (rkipc RUNNING) ---"
/oem/usr/bin/RkLunch.sh >/dev/null 2>&1 &
sleep 3
run_with_timeout "isp_control" "/tmp/examples/isp_control/isp_control" 3

echo "--- STOPPING rkipc for camera examples ---"
/oem/usr/bin/RkLunch-stop.sh >/dev/null 2>&1
sleep 2

run_with_timeout "capture_image" "/tmp/examples/capture_image/capture_image" 5
run_with_timeout "camera_preview" "/tmp/examples/camera_preview/camera_preview" 5
run_with_timeout "record_video" "/tmp/examples/record_video/record_video" 5
run_with_timeout "rknn_yolo" "/tmp/examples/rknn_yolo/rknn_yolo" 10
run_with_timeout "rtsp_server" "/tmp/examples/rtsp_server/rtsp_server" 5
run_with_timeout "rtmp_push" "/tmp/examples/rtmp_push/rtmp_push" 5
run_with_timeout "onvif_server" "/tmp/examples/onvif_server/onvif_server" 5

echo "--- Restarting rkipc ---"
/oem/usr/bin/RkLunch.sh >/dev/null 2>&1 &
EOF

echo "Running tests..."
sshpass -p $PASS scp -o StrictHostKeyChecking=no test_on_device_v2.sh root@$IP:/tmp/
sshpass -p $PASS ssh -o StrictHostKeyChecking=no root@$IP "chmod +x /tmp/test_on_device_v2.sh && /tmp/test_on_device_v2.sh"
