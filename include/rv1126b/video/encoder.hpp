/**
 * @file encoder.hpp
 * @brief RV1126B SDK — 视频编码器模块
 *
 * 使用 Rockchip MPP (Media Process Platform) 进行硬件编码。
 * 支持 H.264 / H.265 / MJPEG，输入 NV12 原始帧，输出编码包。
 *
 * @code
 * Encoder enc;
 * Encoder::Config cfg;
 * cfg.width   = 1920;
 * cfg.height  = 1080;
 * cfg.codec   = CodecType::H264;
 * cfg.bitrate_kbps = 4000;
 * cfg.fps     = 30;
 * cfg.gop     = 60;
 * enc.init(cfg);
 *
 * // 每帧编码
 * Encoder::InputFrame frame{};
 * frame.data    = nv12_data;
 * frame.size    = w * h * 3 / 2;
 * frame.pts_us  = pts;
 * enc.encode(frame);
 *
 * // 获取编码包
 * Encoder::Packet pkt;
 * while (enc.get_packet(pkt) && pkt.data) {
 *     // 使用 pkt.data, pkt.size ...
 *     enc.release_packet(pkt);
 * }
 * enc.deinit();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include "rv1126b/core/utils.hpp"
#include <cstdint>

namespace rv1126b {

class Encoder {
public:
    struct Config {
        int       width         {1920};
        int       height        {1080};
        CodecType codec         {CodecType::H264};
        int       bitrate_kbps  {4000};   ///< 目标码率（kbps）
        int       fps           {30};
        int       gop           {60};     ///< I 帧间隔（帧数）
        PixelFormat input_fmt   {PixelFormat::NV12};

        /// H.264 Profile: 66=Baseline, 77=Main, 100=High
        int       h264_profile  {100};
        /// H.264 Level × 10：如 41 = Level 4.1
        int       h264_level    {41};

        /// Rate control: 0=VBR, 1=CBR, 2=CQP
        int       rc_mode       {1};
        /// CQP 模式质量（0-51，越小质量越高）
        int       qp            {26};
    };

    struct InputFrame {
        const uint8_t* data     {nullptr};
        size_t         size     {0};
        int64_t        pts_us   {0};
        bool           force_idr{false};   ///< 强制本帧为 IDR
        PixelFormat    fmt      {PixelFormat::NV12};
        int            fd       {-1};      ///< DMA-buf fd（优先于 data 指针）
    };

    struct Packet {
        uint8_t* data       {nullptr};  ///< 编码数据（需调用 release_packet 释放）
        size_t   size       {0};
        int64_t  pts_us     {0};
        int64_t  dts_us     {0};
        bool     is_keyframe{false};
        CodecType codec     {CodecType::H264};
    };

    Encoder();
    ~Encoder();

    Encoder(const Encoder&)            = delete;
    Encoder& operator=(const Encoder&) = delete;

    /// 初始化 MPP 编码器
    Result<void> init(const Config& config);

    /// 释放所有资源
    void deinit();

    bool is_initialized() const noexcept;

    /**
     * @brief 提交一帧原始数据进行编码（非阻塞，MPP 内部缓冲）
     * @param frame 输入帧
     */
    Result<void> encode(const InputFrame& frame);

    /**
     * @brief 从 MPP 取出一个编码包
     * @param pkt 输出包，pkt.data==nullptr 表示暂无数据
     * @return Error::Timeout 表示当前无可用包，其他错误为编码失败
     */
    Result<void> get_packet(Packet& pkt, int timeout_ms = 40);

    /// 释放 get_packet 返回的包缓冲区
    void release_packet(Packet& pkt);

    // ─── 运行时参数调整 ────────────────────────────────────────────────────────

    Result<void> set_bitrate(int kbps);
    Result<void> set_fps(int fps);
    Result<void> set_gop(int gop);

    /// 请求下一帧为 IDR 关键帧
    Result<void> request_idr();

    const Config& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
