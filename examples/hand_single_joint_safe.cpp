// Isolated one-axis RH56 test. Uses the official Unitree q convention.

#include "hand_command_safety.hpp"
#include "rh56_readonly.hpp"
#include "rh56_writer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace {

constexpr const char* kRightDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";

bool ReadHand(const std::string& device, rh56::Position& position)
{
    Rh56Readonly reader(device);
    return reader.ReadPosition(position);
}

bool WriteJoint(const std::string& device, std::size_t joint, float value)
{
    rh56::Position position{};
    position[joint] = value;
    rh56::JointMask mask{};
    mask[joint] = true;
    Rh56Writer writer(device);
    return writer.WritePosition(position, mask);
}

class PidFile
{
public:
    explicit PidFile(const char* executable)
    {
        const auto root = std::filesystem::canonical(executable)
                              .parent_path().parent_path().parent_path();
        path_ = root / "run" / "hand_controller.pid";
        std::filesystem::create_directories(path_.parent_path());
        std::ofstream(path_) << getpid() << '\n';
    }

    ~PidFile()
    {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

private:
    std::filesystem::path path_;
};

bool MoveSlowly(const std::string& device, std::size_t joint, float from,
                float to, float step, std::chrono::milliseconds step_delay)
{
    const float direction = to >= from ? 1.0f : -1.0f;
    float command = from;
    while (std::fabs(to - command) > 0.0001f) {
        command += direction * std::min(step, std::fabs(to - command));
        if (!WriteJoint(device, joint, command))
            return false;
        std::this_thread::sleep_for(step_delay);
        rh56::Position feedback{};
        if (!ReadHand(device, feedback))
            return false;
        std::cout << "feedback=" << feedback[joint] << '\n';
    }
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    bool execute = false;
    std::string hand = "left";
    std::string joint_name = "index";
    float delta = -0.02f;
    int hold_ms = 1000;
    float step = 0.005f;
    int step_delay_ms = 500;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--execute")
            execute = true;
        else if (argument == "--hand" && i + 1 < argc)
            hand = argv[++i];
        else if (argument == "--joint" && i + 1 < argc)
            joint_name = argv[++i];
        else if (argument == "--delta" && i + 1 < argc)
            delta = std::stof(argv[++i]);
        else if (argument == "--hold-ms" && i + 1 < argc)
            hold_ms = std::stoi(argv[++i]);
        else if (argument == "--step" && i + 1 < argc)
            step = std::stof(argv[++i]);
        else if (argument == "--step-delay-ms" && i + 1 < argc)
            step_delay_ms = std::stoi(argv[++i]);
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--hand left|right]"
                         " [--joint pinky|ring|middle|index|thumb-bend|thumb-rotation]"
                         " [--delta signed-value] [--hold-ms nonnegative]"
                         " [--step positive] [--step-delay-ms nonnegative]"
                         " [--execute]\n";
            return 2;
        }
    }
    if (hand != "left" && hand != "right") {
        std::cerr << "--hand must be left or right\n";
        return 2;
    }
    const std::array<std::string, 6> joint_names{
        "pinky", "ring", "middle", "index", "thumb-bend", "thumb-rotation"};
    const auto joint_it =
        std::find(joint_names.begin(), joint_names.end(), joint_name);
    if (joint_it == joint_names.end()) {
        std::cerr << "Unknown joint: " << joint_name << '\n';
        return 2;
    }
    if (!std::isfinite(delta) || delta == 0.0f) {
        std::cerr << "--delta must be finite and non-zero\n";
        return 2;
    }
    if (hold_ms < 0) {
        std::cerr << "--hold-ms must be nonnegative\n";
        return 2;
    }
    if (!std::isfinite(step) || step <= 0.0f) {
        std::cerr << "--step must be finite and positive\n";
        return 2;
    }
    if (step_delay_ms < 0) {
        std::cerr << "--step-delay-ms must be nonnegative\n";
        return 2;
    }
    const auto step_delay = std::chrono::milliseconds(step_delay_ms);
    const std::size_t local_joint =
        static_cast<std::size_t>(joint_it - joint_names.begin());
    const bool left = hand == "left";
    const std::string device = left ? kLeftDevice : kRightDevice;
    const std::size_t offset = left ? 6 : 0;
    const std::size_t global_index = offset + local_joint;

    try {
        PidFile pid_file(argv[0]);
        rh56::Position current{};
        if (!ReadHand(device, current)) {
            std::cerr << "Failed to read " << hand << "-hand position\n";
            return 1;
        }

        HandCommandSafety::Config safety_config;
        safety_config.max_step = 1.0f;
        HandCommandSafety safety(safety_config);
        HandCommandSafety::Command measured{};
        measured.fill(1.0f);
        std::copy(current.begin(), current.end(), measured.begin() + offset);
        if (!safety.Reset(measured))
            return 1;

        std::vector<float> target(measured.begin(), measured.end());
        target[global_index] += delta;
        if (target[global_index] < 0.0f || target[global_index] > 1.0f) {
            std::cerr << "Requested target " << target[global_index]
                      << " is outside the RH56 protocol range [0, 1]\n";
            return 2;
        }
        const auto now = HandCommandSafety::Clock::now();
        if (!safety.Accept(target, now))
            return 1;

        HandCommandSafety::Command bounded{};
        if (!safety.Next(now, bounded))
            return 1;

        const float original = measured[global_index];
        const float requested = bounded[global_index];
        std::cout << std::fixed << std::setprecision(3)
                  << hand << '_' << joint_name << " current=" << original
                  << " target=" << requested << " delta="
                  << requested - original << '\n';

        rh56::Position frame_position{};
        frame_position[local_joint] = requested;
        rh56::JointMask frame_mask{};
        frame_mask[local_joint] = true;
        const auto frame =
            rh56::MakeWritePosition(1, frame_position, frame_mask);
        std::cout << "frame:";
        for (uint8_t byte : frame)
            std::cout << ' ' << std::hex << std::setw(2)
                      << std::setfill('0') << static_cast<int>(byte);
        std::cout << std::dec << '\n';

        if (!execute) {
            std::cout << "DRY RUN: no position command sent\n";
            return 0;
        }

        if (!MoveSlowly(device, local_joint, original, requested, step,
                        step_delay)) {
            std::cerr << "Target sequence failed; attempting return\n";
            MoveSlowly(device, local_joint, requested, original, step,
                       step_delay);
            return 1;
        }
        std::cout << "holding target for " << hold_ms << " ms\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));

        if (!MoveSlowly(device, local_joint, requested, original, step,
                        step_delay)) {
            std::cerr << "CRITICAL: return sequence failed\n";
            return 2;
        }

        rh56::Position returned{};
        if (!ReadHand(device, returned)) {
            std::cerr << "Failed to verify returned position\n";
            return 2;
        }
        std::cout << "returned=" << returned[local_joint] << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
