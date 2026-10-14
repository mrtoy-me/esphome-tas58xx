#pragma once
#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <cmath>
#include <climits>
#include <numbers>
namespace esphome {
inline int32_t byteswap(int32_t v) { return static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(v))); }
inline uint32_t byteswap(uint32_t v) { return __builtin_bswap32(v); }
}
