#!/bin/bash
IP="192.168.4.200"
PASS="recamera.1"

echo "Deploying SDK lib..."
sshpass -p $PASS scp -o StrictHostKeyChecking=no lib/libreCamera_pro_sdk.so.1.0.0 root@$IP:/tmp/
sshpass -p $PASS ssh -o StrictHostKeyChecking=no root@$IP "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"

cat << 'EOF' > test_on_device.sh
#!/bin/sh
export LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib

echo "--- test hello_world ---"
/tmp/examples/hello_world/hello_world

echo "--- test imu_read ---"
timeout 3s /tmp/examples/imu_read/imu_read || true

echo "--- test gpio_control ---"
timeout 3s /tmp/examples/gpio_control/gpio_control || true

echo "--- test uart_echo ---"
timeout 3s /tmp/examples/uart_echo/uart_echo || true

echo "--- test isp_control (rkipc RUNNING) ---"
# rkipc might already be running, let's make sure
/oem/usr/bin/RkLunch.sh &
sleep 3
timeout 3s /tmp/examples/isp_control/isp_control || true

echo "--- STOPPING rkipc for camera examples ---"
/oem/usr/bin/RkLunch-stop.sh
sleep 2

echo "--- test capture_image ---"
timeout 5s /tmp/examples/capture_image/capture_image || true

echo "--- test camera_preview ---"
timeout 5s /tmp/examples/camera_preview/camera_preview || true

echo "--- test record_video ---"
timeout 5s /tmp/examples/record_video/record_video || true

echo "--- test rknn_yolo ---"
timeout 15s /tmp/examples/rknn_yolo/rknn_yolo || true

echo "--- test rtsp_server ---"
timeout 5s /tmp/examples/rtsp_server/rtsp_server || true

echo "--- test rtmp_push ---"
timeout 5s /tmp/examples/rtmp_push/rtmp_push || true

echo "--- test onvif_server ---"
timeout 5s /tmp/examples/onvif_server/onvif_server || true

echo "--- Restarting rkipc ---"
/oem/usr/bin/RkLunch.sh &
EOF

echo "Deploying test script and examples..."
sshpass -p $PASS scp -o StrictHostKeyChecking=no test_on_device.sh root@$IP:/tmp/
sshpass -p $PASS ssh -o StrictHostKeyChecking=no root@$IP "chmod +x /tmp/test_on_device.sh"
sshpass -p $PASS scp -r -o StrictHostKeyChecking=no build-aarch64/examples root@$IP:/tmp/

echo "Running tests on device..."
sshpass -p $PASS ssh -o StrictHostKeyChecking=no root@$IP "/tmp/test_on_device.sh"
