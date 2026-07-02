#!/bin/bash
# demo_env.sh - (PC端执行) 一键配置环境与启停摄像头服务

if [ "$1" != "0" ] && [ "$1" != "1" ]; then
    echo "用法: $0 [0|1]"
    echo "  0 : 关闭摄像头服务，创建 /lib_c++ 并部署全局动态库环境"
    echo "  1 : 恢复摄像头服务到原始出厂状态"
    exit 1
fi

# 交互式获取设备信息
while [ -z "$IP" ]; do
    read -p "请输入设备 IP 地址: " IP
done

while [ -z "$USER" ]; do
    read -p "请输入用户名: " USER
done

while [ -z "$PASS" ]; do
    read -s -p "请输入密码: " PASS
    echo
done

# 获取当前脚本所在目录，推导工程根目录
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SDK_ROOT="$(dirname "$DIR")"
LIB_FILE="$SDK_ROOT/lib/libreCamera_pro_sdk.so.1.0.0"

if [ "$1" = "0" ]; then
    echo ">> 1. 正在停止设备 ($IP) 的 rkipc 摄像头服务..."
    sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no "$USER@$IP" "/oem/usr/bin/RkLunch-stop.sh >/dev/null 2>&1"

    echo ">> 2. 检查设备全局运行环境..."
    CHECK_ENV=$(sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no "$USER@$IP" "if [ -f /lib_c++/libreCamera_pro_sdk.so.1 ] && grep -q 'LD_LIBRARY_PATH=/lib_c++' /etc/profile 2>/dev/null; then echo 'READY'; else echo 'NEED_SETUP'; fi" 2>/dev/null)

    if echo "$CHECK_ENV" | grep -q "READY"; then
        echo "✅ 检测到设备全局环境 (/lib_c++) 已经配置就绪，跳过上传和配置。"
        echo "💡 现在你可以 SSH 进入设备，在任何目录下无感执行 ./hello_world 或 ./rknn_yolo 了！"
    else
        echo ">> 3. 创建持久化库目录 /lib_c++ 并配置系统级全局动态链接..."
        sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no "$USER@$IP" "
            mkdir -p /lib_c++
            if [ -f /etc/ld.so.conf ] || [ -d /etc/ld.so.conf.d ]; then
                grep -q '/lib_c++' /etc/ld.so.conf 2>/dev/null || echo '/lib_c++' >> /etc/ld.so.conf
                ldconfig 2>/dev/null
            fi
            grep -q 'LD_LIBRARY_PATH=/lib_c++' /etc/profile 2>/dev/null || echo 'export LD_LIBRARY_PATH=/lib_c++:\$LD_LIBRARY_PATH' >> /etc/profile
        "

        echo ">> 4. 正在上传 SDK 库到设备 /lib_c++ ..."
        if [ -f "$LIB_FILE" ]; then
            sshpass -p "$PASS" scp -o StrictHostKeyChecking=no "$LIB_FILE" "$USER@$IP:/lib_c++/"
            sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no "$USER@$IP" "
                cd /lib_c++ 
                ln -sf libreCamera_pro_sdk.so.1.0.0 libreCamera_pro_sdk.so.1
                ldconfig 2>/dev/null
            "
            echo "✅ 部署完成！系统级全局库路径已生效。"
            echo "💡 现在你可以 SSH 进入设备，在任何目录下无感执行 ./hello_world 或 ./rknn_yolo 了！"
        else
            echo "❌ 找不到预编译的 SDK 库: $LIB_FILE"
        fi
    fi

elif [ "$1" = "1" ]; then
    echo ">> 正在恢复设备 ($IP) 的 rkipc 摄像头服务..."
    # 恢复服务前，清理可能占用相机设备的任何自定义进程
    sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no "$USER@$IP" "
        fuser -k -9 /dev/video* 2>/dev/null
        /oem/usr/bin/RkLunch-stop.sh >/dev/null 2>&1
        nohup /oem/usr/bin/RkLunch.sh </dev/null >/dev/null 2>&1 &
    "
    echo "✅ 摄像头服务已恢复原始出厂状态正常运行！(/lib_c++ 库环境被保留)"
fi
