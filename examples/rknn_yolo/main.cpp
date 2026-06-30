// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// rknn_yolo — RKNN YOLOX-S 目标检测示例
// 加载 RKNN 模型，对摄像头每一帧做推理，
// 执行简单的 NMS 后处理，实时打印检测结果和 FPS。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>
#include <rv1126b/npu.hpp>

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

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// ---------------------------------------------------------------
// COCO 类别名（仅列出 80 类中前 20 类，其余用 "class_N" 填充）
// ---------------------------------------------------------------
static const char* COCO_CLASSES[] = {
    "person","bicycle","car","motorcycle","airplane","bus","train","truck",
    "boat","traffic light","fire hydrant","stop sign","parking meter","bench",
    "bird","cat","dog","horse","sheep","cow"
    // ... 实际使用时请补全 80 类
};
static constexpr int NUM_CLASSES = 80;

// ---------------------------------------------------------------
// 检测框
// ---------------------------------------------------------------
struct Detection {
    float x1, y1, x2, y2;
    float confidence;
    int   class_id;
};

// ---------------------------------------------------------------
// IoU 计算
// ---------------------------------------------------------------
static float iou(const Detection& a, const Detection& b)
{
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

// ---------------------------------------------------------------
// 简单 NMS
// ---------------------------------------------------------------
static std::vector<Detection> nms(std::vector<Detection>& dets, float iou_thresh)
{
    std::sort(dets.begin(), dets.end(),
              [](const Detection& a, const Detection& b) {
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

// ---------------------------------------------------------------
// 解析 YOLOX 输出张量（简化版，假设单尺度 640x640 输出）
// YOLOX 输出格式: [batch, num_anchors, 5 + num_classes]
// ---------------------------------------------------------------
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

        // 找最高类别置信度
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

        // cx, cy, w, h -> x1, y1, x2, y2
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

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    constexpr const char* MODEL_PATH = "/oem/usr/share/model/rknn/yolox_s.rknn";
    constexpr int   INPUT_W    = 640;
    constexpr int   INPUT_H    = 640;
    constexpr float CONF_THRESH = 0.45f;
    constexpr float IOU_THRESH  = 0.45f;

    std::printf("=== RKNN YOLOX-S 目标检测示例 ===\n");
    std::printf("模型: %s\n", MODEL_PATH);
    std::printf("输入尺寸: %dx%d  置信度阈值: %.2f  NMS IoU: %.2f\n\n",
                INPUT_W, INPUT_H, CONF_THRESH, IOU_THRESH);

    // ---------------------------------------------------------------
    // 1. 初始化 NPU 并加载模型
    // ---------------------------------------------------------------
    rv1126b::NPU npu;
    auto init_res = npu.init();
    if (!init_res.ok()) {
        std::fprintf(stderr, "[错误] NPU 初始化失败: %s\n",
                     std::string(rv1126b::to_string(init_res.error())).c_str());
        return EXIT_FAILURE;
    }

    auto load_res = npu.load_model(MODEL_PATH);
    if (!load_res.ok()) {
        std::fprintf(stderr, "[错误] 加载模型失败: %s\n"
                     "       请确认文件存在: %s\n",
                     std::string(rv1126b::to_string(load_res.error())).c_str(),
                     MODEL_PATH);
        npu.deinit();
        return EXIT_FAILURE;
    }
    rv1126b::ModelHandle model_handle = *load_res;
    std::printf("RKNN 模型加载成功\n");

    // ---------------------------------------------------------------
    // 2. 打开摄像头
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cam_cfg;
    cam_cfg.width        = 1280;
    cam_cfg.height       = 720;
    cam_cfg.fps          = 25;
    cam_cfg.codec        = rv1126b::CodecType::H264;
    cam_cfg.bitrate_kbps = 4000;

    rv1126b::Camera camera;
    if (!camera.open(cam_cfg).ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败\n");
        npu.unload_model(model_handle);
        npu.deinit();
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开 (%dx%d)\n\n", cam_cfg.width, cam_cfg.height);

    // ---------------------------------------------------------------
    // 3. 帧处理循环
    // ---------------------------------------------------------------
    std::mutex        infer_mtx;
    uint64_t          frame_count  = 0;
    uint64_t          infer_count  = 0;
    auto              fps_start    = std::chrono::steady_clock::now();

    // 输入缓冲区（RGB888，640×640×3）
    std::vector<uint8_t> input_buf(INPUT_W * INPUT_H * 3);

    // ── NV12 → RGB888 + 双线性缩放到 640×640 ────────────────────────────────
    // 原始帧: NV12, src_w×src_h
    // 输出:   RGB888, INPUT_W×INPUT_H
    auto preprocess_nv12_to_rgb = [&](const uint8_t* src,
                                       int src_w, int src_h,
                                       uint8_t* dst,
                                       int dst_w, int dst_h) {
        const uint8_t* Y  = src;
        const uint8_t* UV = src + src_w * src_h;

        float sx = static_cast<float>(src_w) / dst_w;
        float sy = static_cast<float>(src_h) / dst_h;

        for (int dy = 0; dy < dst_h; ++dy) {
            for (int dx = 0; dx < dst_w; ++dx) {
                // 最近邻采样（快速）
                int sy_i = std::min(static_cast<int>(dy * sy), src_h - 1);
                int sx_i = std::min(static_cast<int>(dx * sx), src_w - 1);

                uint8_t y_val  = Y[sy_i * src_w + sx_i];
                int     uv_row = (sy_i / 2) * src_w + (sx_i & ~1);
                uint8_t u_val  = UV[uv_row];
                uint8_t v_val  = UV[uv_row + 1];

                // YUV → RGB (BT.601 full-range)
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
    };

    camera.set_frame_callback(
        [&](const rv1126b::VideoFrame& frame) {
            if (g_stop.load()) return;

            std::lock_guard<std::mutex> lk(infer_mtx);
            frame_count++;

            // 每 2 帧推理一次，降低 CPU 占用
            if (frame_count % 2 != 0) return;

            // NV12 → RGB888 + 缩放到 640×640
            if (frame.data && frame.size >= (size_t)(frame.width * frame.height * 3 / 2)) {
                preprocess_nv12_to_rgb(frame.data,
                                       frame.width, frame.height,
                                       input_buf.data(),
                                       INPUT_W, INPUT_H);
            } else {
                return; // 帧数据不完整
            }
            rv1126b::npu::TensorBuffer input_tensor;
            input_tensor.index = 0;
            input_tensor.data  = input_buf.data();
            input_tensor.size  = (size_t)(INPUT_W * INPUT_H * 3);
            input_tensor.dtype = rv1126b::npu::DataType::Uint8;

            std::vector<rv1126b::npu::TensorBuffer> inputs  = {input_tensor};
            std::vector<rv1126b::npu::TensorBuffer> outputs;

            auto infer_t0 = std::chrono::steady_clock::now();
            auto infer_res = npu.infer(model_handle, inputs, outputs);
            auto infer_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - infer_t0).count();

            if (!infer_res.ok()) {
                std::fprintf(stderr, "[警告] 推理失败: %s\n",
                             std::string(rv1126b::to_string(infer_res.error())).c_str());
                return;
            }
            infer_count++;

            // 解析输出（YOLOX-S: 3549 anchors for 640x640）
            if (outputs.empty() || !outputs[0].data) return;
            const float* out_data    = reinterpret_cast<const float*>(outputs[0].data);
            int          num_anchors = static_cast<int>(outputs[0].size / sizeof(float) / (5 + NUM_CLASSES));

            auto dets = parse_yolox_output(
                out_data, num_anchors, CONF_THRESH,
                INPUT_W, INPUT_H, cam_cfg.width, cam_cfg.height);
            auto final_dets = nms(dets, IOU_THRESH);

            npu.release_outputs(model_handle, outputs);

            // FPS 统计
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - fps_start).count();
            double fps = elapsed > 0.0 ? infer_count / elapsed : 0.0;

            // 打印本帧检测结果
            std::printf("[帧%5llu | 推理%4lld ms | FPS %.1f] 检测到 %zu 个目标\n",
                        (unsigned long long)frame_count, infer_ms, fps,
                        final_dets.size());

            for (const auto& d : final_dets) {
                const char* cls_name = d.class_id < 20
                    ? COCO_CLASSES[d.class_id]
                    : "unknown";
                std::printf("  %-16s conf=%.2f  bbox=[%.0f,%.0f,%.0f,%.0f]\n",
                            cls_name, d.confidence,
                            d.x1, d.y1, d.x2, d.y2);
            }
        }
    );

    // 启动摄像头
    if (!camera.start().ok()) {
        std::fprintf(stderr, "[错误] 摄像头启动失败\n");
        npu.unload_model(model_handle);
        npu.deinit();
        return EXIT_FAILURE;
    }

    std::printf("目标检测运行中，按 Ctrl+C 退出...\n\n");

    // 等待退出信号
    while (!g_stop.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ---------------------------------------------------------------
    // 4. 清理资源
    // ---------------------------------------------------------------
    camera.stop();
    npu.unload_model(model_handle);
    npu.deinit();

    std::printf("\n总帧数: %llu  总推理次数: %llu\n",
                (unsigned long long)frame_count,
                (unsigned long long)infer_count);
    std::printf("程序退出。\n");
    return EXIT_SUCCESS;
}
