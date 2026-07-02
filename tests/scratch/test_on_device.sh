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
