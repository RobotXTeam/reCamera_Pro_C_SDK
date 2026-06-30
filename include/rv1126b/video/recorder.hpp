/**
 * @file recorder.hpp
 * @brief RV1126B SDK — 文件录像模块
 *
 * 将编码后的视频帧写入 MP4/TS 文件。
 * 使用原始 MPEG-TS 封装（无需外部 muxer 库），支持 H.264/H.265。
 * 支持按时长或文件大小自动分片。
 *
 * @code
 * Recorder rec;
 * Recorder::Config cfg;
 * cfg.output_path  = "/sdcard/record_%Y%m%d_%H%M%S.ts";  // strftime 格式
 * cfg.duration_sec = 300;   // 5 分钟一个文件
 * cfg.max_size_mb  = 512;
 * cfg.codec        = CodecType::H264;
 * rec.start(cfg);
 *
 * // 推送编码帧（来自 Camera 或 Encoder）
 * rec.push_frame(pkt);
 *
 * rec.stop();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include "rv1126b/core/utils.hpp"
#include "rv1126b/video/encoder.hpp"
#include <cstdint>
#include <string>
#include <functional>

namespace rv1126b {

class Recorder {
public:
    struct Config {
        std::string output_path     {"/tmp/record_%Y%m%d_%H%M%S.ts"};
        int         duration_sec    {0};     ///< 0 = 不限时长
        int         max_size_mb     {0};     ///< 0 = 不限大小
        CodecType   codec           {CodecType::H264};
        int         width           {1920};
        int         height          {1080};
        int         fps             {30};
    };

    /// 分片完成回调（参数为刚关闭的文件路径）
    using SegmentCallback = std::function<void(const std::string& path)>;

    Recorder();
    ~Recorder();

    Recorder(const Recorder&)            = delete;
    Recorder& operator=(const Recorder&) = delete;

    /**
     * @brief 开始录像
     * @param config 录像配置
     */
    Result<void> start(const Config& config);

    /// 停止录像（等待当前文件关闭）
    Result<void> stop();

    bool is_recording() const noexcept;

    /**
     * @brief 推送一个编码包（线程安全）
     * @param pkt 来自 Encoder::get_packet() 的包
     */
    Result<void> push_frame(const Encoder::Packet& pkt);

    /**
     * @brief 强制开始新的分片文件
     */
    Result<void> split();

    /// 设置分片完成回调
    void set_segment_callback(SegmentCallback cb);

    /// 当前文件已写入字节数
    size_t current_file_size() const noexcept;

    /// 当前文件已录制时长（秒）
    int current_duration_sec() const noexcept;

    /// 当前输出文件路径
    std::string current_file_path() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
