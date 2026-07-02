// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// rknn_yolo — RKNN YOLOX-S 目标检测示例
// 单摄像头异步推理架构：摄像头回调推流，子线程推理。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>
#include <rv1126b/npu.hpp>
#include <rv1126b/encoder.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "font8x8_basic.h"

class UdpMjpegSender {
    int sock = -1;
    struct sockaddr_in dest_addr{};
public:
    bool init(const std::string& ip, int port) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return false;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr);
        return true;
    }
    bool send_frame(const uint8_t* data, size_t size) {
        if (sock < 0) return false;
        size_t sent = 0;
        while (sent < size) {
            size_t chunk = std::min(size - sent, (size_t)60000);
            sendto(sock, data + sent, chunk, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            sent += chunk;
        }
        return true;
    }
    void close_socket() {
        if (sock >= 0) close(sock);
        sock = -1;
    }
};

static void draw_char_nv12(uint8_t* nv12_data, int w, int h, char c, int x, int y, uint8_t Y, uint8_t U, uint8_t V, int scale = 2) {
    if (c < 0 || c > 127) c = '?';
    char* bitmap = font8x8_basic[(int)c];
    for (int r = 0; r < 8; ++r) {
        for (int c_bit = 0; c_bit < 8; ++c_bit) {
            if (bitmap[r] & (1 << c_bit)) {
                for (int sr = 0; sr < scale; ++sr) {
                    for (int sc = 0; sc < scale; ++sc) {
                        int px = x + c_bit * scale + sc;
                        int py = y + r * scale + sr;
                        if (px >= 0 && px < w && py >= 0 && py < h) {
                            nv12_data[py * w + px] = Y;
                            int uv_offset = w * h + (py / 2) * w + (px & ~1);
                            nv12_data[uv_offset] = U;
                            nv12_data[uv_offset + 1] = V;
                        }
                    }
                }
            }
        }
    }
}

static void draw_text_nv12(uint8_t* nv12_data, int w, int h, const char* text, int x, int y, uint8_t Y, uint8_t U, uint8_t V, int scale = 2) {
    int cx = x;
    while (*text) {
        draw_char_nv12(nv12_data, w, h, *text, cx, y, Y, U, V, scale);
        cx += 8 * scale;
        text++;
    }
}

static void fill_rect_nv12(uint8_t* nv12_data, int w, int h, int x1, int y1, int x2, int y2, uint8_t Y, uint8_t U, uint8_t V) {
    x1 = std::max(0, x1); y1 = std::max(0, y1);
    x2 = std::min(w - 1, x2); y2 = std::min(h - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            nv12_data[y * w + x] = Y;
            int uv_offset = w * h + (y / 2) * w + (x & ~1);
            nv12_data[uv_offset] = U;
            nv12_data[uv_offset + 1] = V;
        }
    }
}

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/) {
    g_stop.store(true, std::memory_order_relaxed);
}

static const char* COCO_CLASSES[] = {
    "person","bicycle","car","motorcycle","airplane","bus","train","truck",
    "boat","traffic light","fire hydrant","stop sign","parking meter","bench",
    "bird","cat","dog","horse","sheep","cow"
};
static constexpr int NUM_CLASSES = 80;

struct Detection {
    float x1, y1, x2, y2;
    float confidence;
    int   class_id;
};

static float iou(const Detection& a, const Detection& b) {
    float ix1 = std::max(a.x1, b.x1);
    float iy1 = std::max(a.y1, b.y1);
    float ix2 = std::min(a.x2, b.x2);
    float iy2 = std::min(a.y2, b.y2);

    float iw = std::max(0.0f, ix2 - ix1);
    float ih = std::max(0.0f, iy2 - iy1);
    float inter = iw * ih;
    float area_a = (a.x2 - a.x1) * (a.y2 - a.y1);
    float area_b = (b.x2 - b.x1) * (b.y2 - b.y1);
    return inter / (area_a + area_b - inter + 1e-6f);
}

static std::vector<Detection> nms(std::vector<Detection>& dets, float iou_thresh) {
    std::sort(dets.begin(), dets.end(), [](const Detection& a, const Detection& b) {
        return a.confidence > b.confidence;
    });

    std::vector<bool> suppressed(dets.size(), false);
    std::vector<Detection> result;

    for (size_t i = 0; i < dets.size(); ++i) {
        if (suppressed[i]) continue;
        result.push_back(dets[i]);
        for (size_t j = i + 1; j < dets.size(); ++j) {
            if (!suppressed[j] && iou(dets[i], dets[j]) > iou_thresh)
                suppressed[j] = true;
        }
    }
    return result;
}

static std::vector<Detection> parse_yolox_output(
    const float* output, int num_anchors,
    float conf_thresh, int input_w, int input_h,
    int orig_w, int orig_h)
{
    std::vector<Detection> dets;
    float scale_x = static_cast<float>(orig_w) / input_w;
    float scale_y = static_cast<float>(orig_h) / input_h;

    for (int i = 0; i < num_anchors; ++i) {
        const float* row = output + i * (5 + NUM_CLASSES);
        float obj_conf = row[4];
        if (obj_conf < conf_thresh) continue;

        int   best_cls  = 0;
        float best_prob = 0.0f;
        for (int c = 0; c < NUM_CLASSES; ++c) {
            if (row[5 + c] > best_prob) {
                best_prob = row[5 + c];
                best_cls  = c;
            }
        }

        float score = obj_conf * best_prob;
        if (score < conf_thresh) continue;

        float cx = row[0], cy = row[1], bw = row[2], bh = row[3];
        Detection d;
        d.x1         = (cx - bw * 0.5f) * scale_x;
        d.y1         = (cy - bh * 0.5f) * scale_y;
        d.x2         = (cx + bw * 0.5f) * scale_x;
        d.y2         = (cy + bh * 0.5f) * scale_y;
        d.confidence = score;
        d.class_id   = best_cls;
        dets.push_back(d);
    }
    return dets;
}

// ── NV12 → RGB888 + 双线性/最近邻缩放到 640×640 ────────────────────────────────
static void preprocess_nv12_to_rgb(const uint8_t* src, int src_w, int src_h,
                                   uint8_t* dst, int dst_w, int dst_h) {
    const uint8_t* Y  = src;
    const uint8_t* UV = src + src_w * src_h;
    float sx = static_cast<float>(src_w) / dst_w;
    float sy = static_cast<float>(src_h) / dst_h;
    for (int dy = 0; dy < dst_h; ++dy) {
        for (int dx = 0; dx < dst_w; ++dx) {
            int sy_i = std::min(static_cast<int>(dy * sy), src_h - 1);
            int sx_i = std::min(static_cast<int>(dx * sx), src_w - 1);
            uint8_t y_val  = Y[sy_i * src_w + sx_i];
            int     uv_row = (sy_i / 2) * src_w + (sx_i & ~1);
            uint8_t u_val  = UV[uv_row];
            uint8_t v_val  = UV[uv_row + 1];
            int c = static_cast<int>(y_val) - 16;
            int d = static_cast<int>(u_val) - 128;
            int e = static_cast<int>(v_val) - 128;
            int r = (298 * c           + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d           + 128) >> 8;
            uint8_t* pix = dst + (dy * dst_w + dx) * 3;
            pix[0] = static_cast<uint8_t>(std::max(0, std::min(255, r)));
            pix[1] = static_cast<uint8_t>(std::max(0, std::min(255, g)));
            pix[2] = static_cast<uint8_t>(std::max(0, std::min(255, b)));
        }
    }
}

// Global shared state
std::mutex g_dets_mtx;
std::vector<Detection> g_latest_dets;
double g_latest_fps = 0.0;

std::mutex g_frame_mtx;
std::condition_variable g_frame_cv;
std::vector<uint8_t> g_latest_nv12;
bool g_frame_ready = false;

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    constexpr const char* MODEL_PATH = "/oem/usr/share/model/rknn/yolox_s.rknn";
    constexpr int   INPUT_W    = 640;
    constexpr int   INPUT_H    = 640;
    constexpr float CONF_THRESH = 0.60f; // 用户要求默认至少 0.60
    constexpr float IOU_THRESH  = 0.45f;

    std::printf("=== RKNN YOLOX-S 目标检测示例 ===\n");

    rv1126b::Camera::Config main_cam_cfg;
    main_cam_cfg.width        = 3840;
    main_cam_cfg.height       = 2160;
    main_cam_cfg.fps          = 30;
    main_cam_cfg.codec        = rv1126b::CodecType::H264;
    main_cam_cfg.raw_format   = rv1126b::PixelFormat::NV12;
    main_cam_cfg.device       = "/dev/video13";

    rv1126b::Camera camera_main;
    if (!camera_main.open(main_cam_cfg).ok()) {
        std::fprintf(stderr, "[错误] 打开主摄像头失败\n");
        return EXIT_FAILURE;
    }
    std::printf("主摄像头已打开 (%dx%d NV12) on %s\n", main_cam_cfg.width, main_cam_cfg.height, main_cam_cfg.device.c_str());

    UdpMjpegSender sender;
    if (!sender.init("192.168.4.100", 9090)) {
        std::fprintf(stderr, "[错误] UDP 发送器初始化失败\n");
    }
    
    rv1126b::Encoder enc;
    rv1126b::Encoder::Config enc_cfg;
    enc_cfg.width = main_cam_cfg.width;
    enc_cfg.height = main_cam_cfg.height;
    enc_cfg.codec = rv1126b::CodecType::MJPEG;
    enc_cfg.fps = main_cam_cfg.fps;
    enc_cfg.input_fmt = rv1126b::PixelFormat::NV12;
    if (!enc.init(enc_cfg).ok()) {
        std::fprintf(stderr, "[错误] 编码器初始化失败\n");
    }

    // 预分配全局帧缓冲区
    g_latest_nv12.resize(main_cam_cfg.width * main_cam_cfg.height * 3 / 2);

    // 启动 YOLO 工作线程
    std::thread yolo_thread([&]() {
        rv1126b::NPU npu;
        if (!npu.init().ok()) return;
        auto load_res = npu.load_model(MODEL_PATH);
        if (!load_res.ok()) return;
        rv1126b::ModelHandle model_handle = *load_res;
        
        std::vector<uint8_t> input_buf(INPUT_W * INPUT_H * 3);
        std::vector<uint8_t> local_nv12(main_cam_cfg.width * main_cam_cfg.height * 3 / 2);
        
        uint64_t infer_count = 0;
        auto fps_start = std::chrono::steady_clock::now();

        while (!g_stop.load()) {
            {
                std::unique_lock<std::mutex> lock(g_frame_mtx);
                g_frame_cv.wait(lock, []{ return g_frame_ready || g_stop.load(); });
                if (g_stop.load()) break;
                // Copy frame to local buffer
                memcpy(local_nv12.data(), g_latest_nv12.data(), local_nv12.size());
                g_frame_ready = false;
            }

            preprocess_nv12_to_rgb(local_nv12.data(), main_cam_cfg.width, main_cam_cfg.height,
                                   input_buf.data(), INPUT_W, INPUT_H);

            rv1126b::npu::TensorBuffer input_tensor;
            input_tensor.index = 0;
            input_tensor.data  = input_buf.data();
            input_tensor.size  = input_buf.size();
            input_tensor.dtype = rv1126b::npu::DataType::Uint8;

            std::vector<rv1126b::npu::TensorBuffer> inputs  = {input_tensor};
            std::vector<rv1126b::npu::TensorBuffer> outputs;

            if (npu.infer(model_handle, inputs, outputs).ok() && !outputs.empty() && outputs[0].data) {
                infer_count++;
                const float* out_data = reinterpret_cast<const float*>(outputs[0].data);
                int num_anchors = static_cast<int>(outputs[0].size / sizeof(float) / (5 + NUM_CLASSES));

                auto dets = parse_yolox_output(out_data, num_anchors, CONF_THRESH,
                                               INPUT_W, INPUT_H, main_cam_cfg.width, main_cam_cfg.height);
                auto final_dets = nms(dets, IOU_THRESH);
                npu.release_outputs(model_handle, outputs);

                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - fps_start).count();
                double fps = elapsed > 0.0 ? infer_count / elapsed : 0.0;

                std::lock_guard<std::mutex> lock(g_dets_mtx);
                g_latest_dets = std::move(final_dets);
                g_latest_fps = fps;
            }
        }
        npu.unload_model(model_handle);
        npu.deinit();
    });

    // 摄像头回调：画框、编码、发送
    camera_main.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
        if (g_stop.load()) return;
        if (!frame.data || frame.size < g_latest_nv12.size()) return;

        // Wake up YOLO thread with latest frame
        {
            std::lock_guard<std::mutex> lock(g_frame_mtx);
            memcpy(g_latest_nv12.data(), frame.data, g_latest_nv12.size());
            g_frame_ready = true;
        }
        g_frame_cv.notify_one();

        uint8_t* nv12_ptr = const_cast<uint8_t*>(frame.data);
        
        std::vector<Detection> current_dets;
        double current_fps = 0.0;
        {
            std::lock_guard<std::mutex> lock(g_dets_mtx);
            current_dets = g_latest_dets;
            current_fps = g_latest_fps;
        }

        char fps_text[64];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", current_fps);
        draw_text_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, fps_text, 20, 20, 76, 84, 255, 4);

        for (const auto& d : current_dets) {
            const char* cls_name = d.class_id < 20 ? COCO_CLASSES[d.class_id] : "unknown";
            int rx1 = static_cast<int>(d.x1);
            int ry1 = static_cast<int>(d.y1);
            int rx2 = static_cast<int>(d.x2);
            int ry2 = static_cast<int>(d.y2);

            for(int i=0; i<8; i++) {
                fill_rect_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, rx1, ry1+i, rx2, ry1+i, 76, 84, 255);
                fill_rect_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, rx1, ry2-i, rx2, ry2-i, 76, 84, 255);
                fill_rect_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, rx1+i, ry1, rx1+i, ry2, 76, 84, 255);
                fill_rect_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, rx2-i, ry1, rx2-i, ry2, 76, 84, 255);
            }

            char label[64];
            snprintf(label, sizeof(label), "%s", cls_name); // User requested specific item name, no conf%
            int text_scale = 4;
            int text_w = strlen(label) * 8 * text_scale;
            int text_h = 8 * text_scale;
            fill_rect_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, rx1, ry1 - text_h - 10, rx1 + text_w + 10, ry1, 76, 84, 255);
            draw_text_nv12(nv12_ptr, main_cam_cfg.width, main_cam_cfg.height, label, rx1 + 5, ry1 - text_h - 5, 29, 255, 107, text_scale); 
        }

        rv1126b::Encoder::InputFrame enc_in{};
        enc_in.data = nv12_ptr;
        enc_in.size = main_cam_cfg.width * main_cam_cfg.height * 3 / 2;
        enc_in.pts_us = rv1126b::now_us();
        if (enc.encode(enc_in).ok()) {
            rv1126b::Encoder::Packet pkt;
            while (enc.get_packet(pkt) && pkt.data) {
                sender.send_frame(pkt.data, pkt.size);
                enc.release_packet(pkt);
            }
        }
    });

    if (!camera_main.start().ok()) {
        std::fprintf(stderr, "[错误] 主摄像头启动失败\n");
    }

    std::printf("目标检测运行中，按 Ctrl+C 退出...\n\n");

    while (!g_stop.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    g_frame_cv.notify_all();
    yolo_thread.join();
    camera_main.stop();
    enc.deinit();
    sender.close_socket();
    std::printf("程序退出。\n");
    return EXIT_SUCCESS;
}
