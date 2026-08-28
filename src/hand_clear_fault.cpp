#include "rh56/readonly.hpp"
#include "rh56/writer.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {

constexpr const char* kRightDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";
constexpr std::array<const char*, 6> kJointNames{
    "pinky", "ring", "middle", "index", "thumb-bend", "thumb-rotation"};
constexpr uint8_t kMaximumTemperatureC = 60;

std::filesystem::path ProjectRoot(const char* executable)
{
    return std::filesystem::weakly_canonical(executable)
        .parent_path().parent_path().parent_path();
}

bool ControllerActive(const std::filesystem::path& root)
{
    std::ifstream input(root / "run" / "hand_controller.pid");
    int pid = 0;
    return (input >> pid) && pid > 0 &&
           std::filesystem::exists("/proc/" + std::to_string(pid));
}

std::optional<std::size_t> JointIndex(const std::string& name)
{
    for (std::size_t i = 0; i < kJointNames.size(); ++i)
        if (name == kJointNames[i])
            return i;
    return std::nullopt;
}

void PrintTelemetry(const char* prefix, const rh56::ByteValues& error,
                    const rh56::ByteValues& status,
                    const rh56::ByteValues& temperature)
{
    for (std::size_t i = 0; i < kJointNames.size(); ++i)
        std::cout << prefix << ' ' << kJointNames[i]
                  << " error=0x" << std::hex << +error[i] << std::dec
                  << " status=" << +status[i]
                  << " temperature=" << +temperature[i] << "C\n";
}

}  // namespace

int main(int argc, char** argv)
{
    std::string hand;
    std::string joint;
    bool execute = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--execute")
            execute = true;
        else if ((argument == "--hand" || argument == "--joint") && i + 1 < argc) {
            const std::string value = argv[++i];
            if (argument == "--hand")
                hand = value;
            else
                joint = value;
        } else {
            std::cerr << "Usage: " << argv[0]
                      << " --hand left|right --joint NAME --execute\n";
            return 2;
        }
    }
    const auto joint_index = JointIndex(joint);
    if (!execute || (hand != "left" && hand != "right") || !joint_index) {
        std::cerr << "Usage: " << argv[0]
                  << " --hand left|right --joint pinky|ring|middle|index|"
                     "thumb-bend|thumb-rotation --execute\n";
        return 2;
    }

    const auto root = ProjectRoot(argv[0]);
    if (ControllerActive(root)) {
        std::cerr << "Refusing to clear faults while a hand controller is active.\n";
        return 1;
    }
    const char* device = hand == "right" ? kRightDevice : kLeftDevice;

    try {
        rh56::Position position{};
        rh56::ByteValues error{};
        rh56::ByteValues status{};
        rh56::ByteValues temperature{};
        {
            Rh56Readonly reader(device);
            if (!reader.ReadPosition(position) ||
                !reader.ReadBytes(0x0646, error) ||
                !reader.ReadBytes(0x064C, status) ||
                !reader.ReadBytes(0x0652, temperature))
                throw std::runtime_error("pre-clear telemetry read failed");
        }
        PrintTelemetry("before", error, status, temperature);
        if (error[*joint_index] == 0 && status[*joint_index] != 7) {
            std::cout << hand << ' ' << joint << ": no fault is present\n";
            return 0;
        }
        for (std::size_t i = 0; i < temperature.size(); ++i)
            if (error[i] != 0 && temperature[i] >= kMaximumTemperatureC)
                throw std::runtime_error(
                    std::string(kJointNames[i]) +
                    " is too hot; overtemperature faults must cool and auto-clear");

        rh56::JointMask recover{};
        for (std::size_t i = 0; i < error.size(); ++i)
            recover[i] = error[i] != 0 || status[i] == 7;
        {
            Rh56Writer writer(device);
            if (!writer.WritePosition(position, recover))
                throw std::runtime_error("failed to hold faulted joints at current feedback");
            if (error[*joint_index] != 0 && !writer.ClearErrors())
                throw std::runtime_error("clear-error register write was not acknowledged");
            if (!writer.WritePosition(position, recover))
                throw std::runtime_error("failed to re-enable current-position hold");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        {
            Rh56Readonly reader(device);
            if (!reader.ReadBytes(0x0646, error) ||
                !reader.ReadBytes(0x064C, status) ||
                !reader.ReadBytes(0x0652, temperature))
                throw std::runtime_error("post-clear telemetry read failed");
        }
        PrintTelemetry("after", error, status, temperature);
        if (error[*joint_index] != 0 || status[*joint_index] == 7) {
            std::cerr << hand << ' ' << joint
                      << ": fault-stop state remains after recovery\n";
            return 1;
        }
        std::cout << hand << ' ' << joint << ": fault cleared; no travel commanded\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
