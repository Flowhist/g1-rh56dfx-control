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

    if (failures == 0)
        std::cout << "All RH56 protocol tests passed\n";
    return failures == 0 ? 0 : 1;
}
