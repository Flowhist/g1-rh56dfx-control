#include "rh56_protocol.hpp"

#include <cmath>
#include <iostream>

namespace {

int failures = 0;

void Expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool Near(float actual, float expected)
{
    return std::fabs(actual - expected) < 1e-6f;
}

}  // namespace

int main()
{
    const auto read = rh56::MakeReadPosition(1);
    Expect(read == rh56::ReadRequest{
                       0xEB, 0x90, 0x01, 0x04, 0x11,
                       0x0A, 0x06, 0x0C, 0x32},
           "read-position frame is exact");

    Expect(rh56::EncodePosition(0.0f) == 0, "closed encodes as raw zero");
    Expect(rh56::EncodePosition(1.0f) == 1000,
           "open encodes as raw one thousand");
    Expect(rh56::EncodePosition(0.75f) == 750,
           "normalized command follows the official raw direction");
    Expect(Near(rh56::DecodePosition(250), 0.25f),
           "raw state decodes to the official Unitree convention");

    rh56::Position command{1.0f, 0.0f, 0.75f, 0.5f, -1.0f, 2.0f};
    const auto write = rh56::MakeWritePosition(1, command);
    Expect(write[0] == 0xEB && write[1] == 0x90 && write[4] == 0x12,
           "write-position frame header is exact");
    Expect(write[5] == 0xCE && write[6] == 0x05,
           "write-position register is ANGLE_SET");
    Expect(write[7] == 0xE8 && write[8] == 0x03,
           "open command bytes are 1000 little-endian");
    Expect(write[9] == 0x00 && write[10] == 0x00,
           "closed command bytes are zero");
    Expect(write.back() == rh56::Checksum(write.data(), write.size()),
           "write-position checksum is exact");

    rh56::JointMask index_only{};
    index_only[3] = true;
    const auto masked = rh56::MakeWritePosition(1, command, index_only);
    Expect(masked[7] == 0xFF && masked[8] == 0xFF,
           "unselected joints encode as no-change");
    Expect(masked[13] == 0xF4 && masked[14] == 0x01,
           "selected joint encodes its target");

    rh56::ReadResponse response{
        0x90, 0xEB, 0x01, 0x0F, 0x11, 0x0A, 0x06,
        0xFA, 0x00, 0xF4, 0x01, 0xE8, 0x03,
        0x00, 0x00, 0x7D, 0x00, 0xEE, 0x02, 0x00};
    response.back() = rh56::Checksum(response.data(), response.size());
    rh56::Position decoded{};
    Expect(rh56::ParsePosition(response, 1, decoded),
           "valid position response is accepted");
    Expect(Near(decoded[0], 0.25f) && Near(decoded[1], 0.5f) &&
               Near(decoded[2], 1.0f) && Near(decoded[3], 0.0f),
           "position response uses the official Unitree direction");

    response.back() ^= 0x01;
    Expect(!rh56::ParsePosition(response, 1, decoded),
           "bad checksum is rejected");

    const auto current_read = rh56::MakeRead(1, 0x063A, 0x0C);
    Expect(current_read[5] == 0x3A && current_read[6] == 0x06 &&
               current_read[7] == 0x0C,
           "current read uses CURRENT register and twelve-byte payload");
    const auto status_read = rh56::MakeRead(1, 0x064C, 0x06);
    Expect(status_read[5] == 0x4C && status_read[6] == 0x06 &&
               status_read[7] == 0x06,
           "status read uses STATUS register and six-byte payload");
    const auto error_read = rh56::MakeRead(1, 0x0646, 0x06);
    Expect(error_read[5] == 0x46 && error_read[6] == 0x06 &&
               error_read[7] == 0x06,
           "error read uses ERROR register and six-byte payload");
    const auto temperature_read = rh56::MakeRead(1, 0x0652, 0x06);
    Expect(temperature_read[5] == 0x52 && temperature_read[6] == 0x06,
           "temperature read uses TEMP register");

    const rh56::RawValues current_limits{0, 250, 500, 750, 1000, 1500};
    const auto limit_write = rh56::MakeWriteWords(1, 0x03FC, current_limits);
    Expect(limit_write[5] == 0xFC && limit_write[6] == 0x03,
           "current limit write uses CURRENT_LIMIT register");
    Expect(limit_write[17] == 0xDC && limit_write[18] == 0x05,
           "current limit values are encoded little-endian");
    Expect(limit_write.back() ==
               rh56::Checksum(limit_write.data(), limit_write.size()),
           "current limit write checksum is exact");

    rh56::ReadResponse current_response{
        0x90, 0xEB, 0x01, 0x0F, 0x11, 0x3A, 0x06,
        0x64, 0x00, 0xC8, 0x00, 0x2C, 0x01,
        0x90, 0x01, 0xF4, 0x01, 0x58, 0x02, 0x00};
    current_response.back() =
        rh56::Checksum(current_response.data(), current_response.size());
    rh56::RawValues currents{};
    Expect(rh56::ParseWords(current_response, 1, 0x063A, currents),
           "valid current response is accepted");
    Expect(currents == rh56::RawValues{100, 200, 300, 400, 500, 600},
           "current response preserves raw mA values");
    Expect(!rh56::ParseWords(current_response, 1, 0x062E, currents),
           "response from the wrong register is rejected");

    rh56::ByteReadResponse status_response{
        0x90, 0xEB, 0x01, 0x09, 0x11, 0x4C, 0x06,
        0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x00};
    status_response.back() =
        rh56::Checksum(status_response.data(), status_response.size());
    rh56::ByteValues statuses{};
    Expect(rh56::ParseBytes(status_response, 1, 0x064C, statuses),
           "valid status response is accepted");
    Expect(statuses == rh56::ByteValues{0, 1, 2, 3, 5, 7},
           "all six status bytes are decoded");

    if (failures == 0)
        std::cout << "All RH56 protocol tests passed\n";
    return failures == 0 ? 0 : 1;
}
