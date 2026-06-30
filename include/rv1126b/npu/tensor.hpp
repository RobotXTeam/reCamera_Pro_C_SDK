/**
 * @file tensor.hpp
 * @brief RV1126B SDK — NPU tensor type definitions
 *
 * Defines tensor enums and structs aligned with the RKNN API,
 * so external code does not depend directly on rknn_api.h.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

namespace rv1126b {
namespace npu {

// ─── Data types ──────────────────────────────────────────────────────────────────

enum class DataType : int {
    Float32 = 0,
    Float16 = 1,
    Int8    = 2,
    Uint8   = 3,
    Int16   = 4,
    Uint16  = 5,
    Int32   = 6,
    Uint32  = 7,
    Int64   = 8,
    Bool    = 9,
};

inline const char* to_string(DataType dt) noexcept {
    switch (dt) {
        case DataType::Float32: return "float32";
        case DataType::Float16: return "float16";
        case DataType::Int8:    return "int8";
        case DataType::Uint8:   return "uint8";
        case DataType::Int16:   return "int16";
        case DataType::Uint16:  return "uint16";
        case DataType::Int32:   return "int32";
        case DataType::Uint32:  return "uint32";
        case DataType::Int64:   return "int64";
        case DataType::Bool:    return "bool";
        default:                return "unknown";
    }
}

inline size_t dtype_size(DataType dt) noexcept {
    switch (dt) {
        case DataType::Float32:
        case DataType::Int32:
        case DataType::Uint32:  return 4;
        case DataType::Float16:
        case DataType::Int16:
        case DataType::Uint16:  return 2;
        case DataType::Int8:
        case DataType::Uint8:
        case DataType::Bool:    return 1;
        case DataType::Int64:   return 8;
        default:                return 0;
    }
}

// ─── Tensor memory layout ───────────────────────────────────────────────────────

enum class TensorLayout : int {
    Undefined = 0,
    NCHW      = 1,
    NHWC      = 2,
    NC1HWC2   = 3,   ///< RKNN internal format
};

inline const char* to_string(TensorLayout layout) noexcept {
    switch (layout) {
        case TensorLayout::NCHW:     return "NCHW";
        case TensorLayout::NHWC:     return "NHWC";
        case TensorLayout::NC1HWC2:  return "NC1HWC2";
        default:                     return "undefined";
    }
}

// ─── Quantization parameters ────────────────────────────────────────────────────

enum class QuantType : int {
    None   = 0,
    Affine = 1,   ///< zero_point + scale
    DFP    = 2,   ///< dynamic fixed-point
};

struct QuantParam {
    QuantType type       {QuantType::None};
    int       zero_point {0};
    float     scale      {1.0f};
    int       fl         {0};      ///< DFP fraction length
};

// ─── Tensor info ─────────────────────────────────────────────────────────────────

static constexpr int TENSOR_MAX_DIMS = 8;

struct TensorInfo {
    std::string  name;
    DataType     dtype    {DataType::Uint8};
    TensorLayout layout   {TensorLayout::NHWC};
    int          n_dims   {0};
    std::array<uint32_t, TENSOR_MAX_DIMS> dims{};

    QuantParam   qnt;

    /// Total element count
    size_t element_count() const noexcept {
        if (n_dims == 0) return 0;
        size_t count = 1;
        for (int i = 0; i < n_dims; ++i) count *= dims[i];
        return count;
    }

    /// Total byte size
    size_t byte_size() const noexcept {
        return element_count() * dtype_size(dtype);
    }
};

// ─── Tensor buffer (inference input/output) ─────────────────────────────────────

struct TensorBuffer {
    int      index  {0};         ///< Input/output index
    void*    data   {nullptr};   ///< Data pointer (caller manages lifetime)
    size_t   size   {0};         ///< Data byte count
    DataType dtype  {DataType::Uint8};
    bool     pass_through {false}; ///< true = bypass quantization, pass raw data
};

} // namespace npu
} // namespace rv1126b
