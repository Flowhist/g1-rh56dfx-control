// Dual-port RH56DFX state publisher. This process contains no motion command path.

#include "rh56_readonly.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>

#include <unitree/idl/go2/MotorStates_.hpp>
#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>

using MotorStates = unitree_go::msg::dds_::MotorStates_;

namespace {

constexpr const char* kRightDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0";
constexpr const char* kLeftDevice =
    "/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0";

std::atomic_bool running{true};

void Stop(int) { running = false; }

}  // namespace

int main(int argc, char** argv)
{
    const std::string network = argc > 1 ? argv[1] : "eth0";
    const std::string right_device = argc > 2 ? argv[2] : kRightDevice;
    const std::string left_device = argc > 3 ? argv[3] : kLeftDevice;

    try {
        Rh56Readonly right(right_device);
        Rh56Readonly left(left_device);

        unitree::robot::ChannelFactory::Instance()->Init(0, network);
        auto publisher =
            std::make_shared<unitree::robot::ChannelPublisher<MotorStates>>(
                "rt/inspire/state");
        publisher->InitChannel();

        MotorStates state;
        state.states().resize(12);
        const float missing = std::numeric_limits<float>::quiet_NaN();
        for (auto& motor : state.states())
            motor.q() = missing;

        std::signal(SIGINT, Stop);
        std::signal(SIGTERM, Stop);
        std::cout << "Publishing read-only hand state on rt/inspire/state\n"
                  << "right=" << right_device << "\nleft=" << left_device
                  << "\n";

        std::array<float, 6> position{};
        while (running) {
            if (right.ReadPosition(position)) {
                for (std::size_t i = 0; i < position.size(); ++i)
                    state.states()[i].q() = position[i];
            } else {
                for (std::size_t i = 0; i < position.size(); ++i)
                    ++state.states()[i].lost();
            }

            if (left.ReadPosition(position)) {
                for (std::size_t i = 0; i < position.size(); ++i)
                    state.states()[i + 6].q() = position[i];
            } else {
                for (std::size_t i = 0; i < position.size(); ++i)
                    ++state.states()[i + 6].lost();
            }

            publisher->Write(state);
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
