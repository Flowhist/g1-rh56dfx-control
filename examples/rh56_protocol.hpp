#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace rh56 {

using Position = std::array<float, 6>;
using ReadRequest = std::array<uint8_t, 9>;
using ReadResponse = std::array<uint8_t, 20>;
using WriteRequest = std::array<uint8_t, 20>;

inline uint8_t Checksum(const uint8_t* data, std::size_t size)
{
    uint8_t sum = 0;
    for (std::size_t i = 2; i + 1 < size; ++i)
        sum = static_cast<uint8_t>(sum + data[i]);
    return sum;
}

inline ReadRequest MakeReadPosition(uint8_t id)
{
    ReadRequest request{0xEB, 0x90, id, 0x04, 0x11,
                        0x0A, 0x06, 0x0C, 0x00};
    request.back() = Checksum(request.data(), request.size());
    return request;
}

inline uint16_t EncodePosition(float normalized)
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    return static_cast<uint16_t>(
        std::lround((1.0f - normalized) * 1000.0f));
}

inline float DecodePosition(uint16_t raw)
{
    return 1.0f - std::clamp(raw, uint16_t{0}, uint16_t{1000}) / 1000.0f;
}

inline WriteRequest MakeWritePosition(uint8_t id, const Position& position)
{
    WriteRequest request{};
    request[0] = 0xEB;
    request[1] = 0x90;
    request[2] = id;
    request[3] = 0x0F;
    request[4] = 0x12;
    request[5] = 0xCE;
    request[6] = 0x05;
    for (std::size_t i = 0; i < position.size(); ++i) {
        const uint16_t raw = EncodePosition(position[i]);
        request[7 + 2 * i] = static_cast<uint8_t>(raw & 0xFF);
        request[8 + 2 * i] = static_cast<uint8_t>(raw >> 8);
    }
    request.back() = Checksum(request.data(), request.size());
    return request;
}

inline bool ParsePosition(const ReadResponse& response, uint8_t id,
                          Position& position)
{
    if (response[0] != 0x90 || response[1] != 0xEB ||
        response[2] != id || response[4] != 0x11 ||
        response[5] != 0x0A || response[6] != 0x06 ||
        response.back() != Checksum(response.data(), response.size()))
        return false;

    for (std::size_t i = 0; i < position.size(); ++i) {
        const uint16_t raw = static_cast<uint16_t>(response[7 + 2 * i]) |
                             (static_cast<uint16_t>(response[8 + 2 * i]) << 8);
        position[i] = DecodePosition(raw);
    }
    return true;
}

}  // namespace rh56
