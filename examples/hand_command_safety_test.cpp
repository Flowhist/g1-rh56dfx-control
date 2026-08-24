#include "hand_command_safety.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

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
    using Clock = HandCommandSafety::Clock;
    const auto now = Clock::now();

    HandCommandSafety safety;
    HandCommandSafety::Command measured{};
    measured.fill(0.5f);
    Expect(safety.Reset(measured), "valid measured state is accepted");

    Expect(!safety.Accept(std::vector<float>(11, 0.5f), now),
           "wrong command size is rejected");

    std::vector<float> invalid(12, 0.5f);
    invalid[3] = std::numeric_limits<float>::quiet_NaN();
    Expect(!safety.Accept(invalid, now), "NaN is rejected");

    std::vector<float> target(12, 0.9f);
    target[0] = -1.0f;
    target[1] = 2.0f;
    Expect(safety.Accept(target, now), "finite command is accepted");

    HandCommandSafety::Command output{};
    Expect(safety.Next(now, output), "fresh command produces output");
    Expect(Near(output[0], 0.48f), "lower target is rate-limited");
    Expect(Near(output[1], 0.52f), "upper target is clamped and rate-limited");
    Expect(Near(output[2], 0.52f), "ordinary target is rate-limited");

    Expect(safety.Next(now + std::chrono::milliseconds(100), output),
           "command remains active before timeout");
    Expect(Near(output[2], 0.54f), "successive step remains bounded");

    Expect(!safety.Next(now + std::chrono::milliseconds(501), output),
           "stale command stops producing writes");

    measured[4] = std::numeric_limits<float>::infinity();
    Expect(!safety.Reset(measured), "invalid measured state is rejected");

    if (failures == 0)
        std::cout << "All hand command safety tests passed\n";
    return failures == 0 ? 0 : 1;
}
