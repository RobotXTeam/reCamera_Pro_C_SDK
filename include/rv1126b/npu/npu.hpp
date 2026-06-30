/**
 * @file npu.hpp
 * @brief RV1126B SDK — NPU (RKNN) module
 *
 * Dynamically loads librknnrt.so via dlopen/dlsym — no compile-time dependency.
 * Supports concurrent multi-model loading and multi-core inference (RKNN_FLAG_PRIOR_*).
 *
 * @code
 * NPU npu;
 * npu.init("/oem/usr/lib/librknnrt.so");
 *
 * auto handle = npu.load_model("/oem/models/yolov5.rknn");
 * if (!handle) { ... }
 *
 * npu::TensorBuffer input;
 * input.index = 0;
 * input.data  = image_data;
 * input.size  = image_size;
 * input.dtype = npu::DataType::Uint8;
 *
 * std::vector<npu::TensorBuffer> outputs;
 * npu.infer(*handle, {input}, outputs);
 *
 * npu.unload_model(*handle);
 * npu.deinit();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include "rv1126b/npu/tensor.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace rv1126b {

using ModelHandle = uint32_t;
static constexpr ModelHandle INVALID_MODEL_HANDLE = 0;

namespace npu {

struct DetectionResult {
    float x1, y1, x2, y2;   ///< Bounding box (pixel coordinates, original size)
    float score;             ///< Confidence score [0, 1]
    int   class_id;
    std::string class_name;
};

struct ModelInfo {
    std::string            name;
    uint32_t               input_count;
    uint32_t               output_count;
    std::vector<TensorInfo> inputs;
    std::vector<TensorInfo> outputs;
    int                    sdk_version_major;
    int                    sdk_version_minor;
};

} // namespace npu

class NPU {
public:
    static constexpr const char* DEFAULT_LIB_PATH = "/oem/usr/lib/librknnrt.so";

    NPU();
    ~NPU();

    NPU(const NPU&)            = delete;
    NPU& operator=(const NPU&) = delete;

    /**
     * @brief Initialize the NPU, dlopen librknnrt.so
     * @param lib_path  Path to librknnrt.so, defaults to /oem/usr/lib/librknnrt.so
     */
    Result<void> init(const std::string& lib_path = DEFAULT_LIB_PATH);

    /// Release all resources
    void deinit();

    bool is_initialized() const noexcept;

    // ─── Model management ────────────────────────────────────────────────────────

    /**
     * @brief Load an RKNN model from file
     * @param path       Path to .rknn model file
     * @param core_mask  NPU core mask (0=auto, 1=core0, 2=core1, 3=both)
     * @return ModelHandle
     */
    Result<ModelHandle> load_model(const std::string& path,
                                   uint32_t core_mask = 0);

    /**
     * @brief Load an RKNN model from a memory buffer
     */
    Result<ModelHandle> load_model_from_buffer(const uint8_t* data, size_t size,
                                               uint32_t core_mask = 0);

    /// Unload a model and release its RKNN context
    Result<void> unload_model(ModelHandle handle);

    /// Query model info (input/output tensor descriptions)
    Result<npu::ModelInfo> get_model_info(ModelHandle handle);

    // ─── Inference ────────────────────────────────────────────────────────────────

    /**
     * @brief Synchronous inference
     * @param handle   Model handle
     * @param inputs   Input tensor list (count must match the model)
     * @param outputs  Output tensor list (allocated by SDK, auto-filled after inference)
     *                 Caller must call release_outputs() after consuming the output data
     */
    Result<void> infer(ModelHandle handle,
                       const std::vector<npu::TensorBuffer>& inputs,
                       std::vector<npu::TensorBuffer>& outputs);

    /**
     * @brief Release output buffers allocated by infer()
     */
    Result<void> release_outputs(ModelHandle handle,
                                 std::vector<npu::TensorBuffer>& outputs);

    // ─── Performance query ────────────────────────────────────────────────────────

    /**
     * @brief Get the duration of the last inference (microseconds)
     */
    Result<uint64_t> get_last_infer_time_us(ModelHandle handle);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
