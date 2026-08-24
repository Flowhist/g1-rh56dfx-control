#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

class HandCommandSafety
{
public:
    static constexpr std::size_t kSize = 12;
    using Command = std::array<float, kSize>;
    using Clock = std::chrono::steady_clock;

    struct Config
    {
        float minimum{0.0f};
        float maximum{1.0f};
        float max_step{0.02f};
        std::chrono::milliseconds timeout{500};
    };

    HandCommandSafety() = default;
    explicit HandCommandSafety(Config config) : config_(config) {}

    bool Reset(const Command& measured)
    {
        if (!AllFinite(measured))
            return false;
        output_ = measured;
        for (float& value : output_)
            value = std::clamp(value, config_.minimum, config_.maximum);
        target_ = output_;
        has_target_ = false;
        return true;
    }

    bool Accept(const std::vector<float>& input, Clock::time_point now)
    {
        if (input.size() != kSize)
            return false;

        Command candidate{};
        std::copy(input.begin(), input.end(), candidate.begin());
        if (!AllFinite(candidate))
            return false;

        for (float& value : candidate)
            value = std::clamp(value, config_.minimum, config_.maximum);
        target_ = candidate;
        last_command_ = now;
        has_target_ = true;
        return true;
    }

    bool Next(Clock::time_point now, Command& output)
    {
        if (!has_target_ || now - last_command_ > config_.timeout)
            return false;

        for (std::size_t i = 0; i < kSize; ++i) {
            const float delta = target_[i] - output_[i];
            output_[i] +=
                std::clamp(delta, -config_.max_step, config_.max_step);
        }
        output = output_;
        return true;
    }

    const Command& output() const { return output_; }

private:
    static bool AllFinite(const Command& command)
    {
        return std::all_of(command.begin(), command.end(),
                           [](float value) { return std::isfinite(value); });
    }

    Config config_{};
    Command output_{};
    Command target_{};
    Clock::time_point last_command_{};
    bool has_target_{false};
};
