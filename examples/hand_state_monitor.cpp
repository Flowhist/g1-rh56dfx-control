/*
 * Minimal dual-hand state monitor.
 *
 * Usage:
 *   ./hand_state_monitor [network-interface]
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <unitree/idl/go2/MotorStates_.hpp>
#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

using MotorStates = unitree_go::msg::dds_::MotorStates_;

class Monitor
{
public:
    void OnMessage(const void* ptr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = *static_cast<const MotorStates*>(ptr);
        received_ = true;
    }

    void Print()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!received_ || state_.states().size() < 12)
        {
            std::cout << "Waiting for rt/inspire/state..." << std::endl;
            return;
        }

        std::cout << std::fixed << std::setprecision(3);

        std::cout << "Right: ";
        for (int i = 0; i < 6; ++i)
            std::cout << state_.states()[i].q() << ' ';

        std::cout << "\nLeft : ";
        for (int i = 6; i < 12; ++i)
            std::cout << state_.states()[i].q() << ' ';

        std::cout << "\n" << std::endl;
    }

private:
    std::mutex mutex_;
    MotorStates state_;
    bool received_{false};
};

int main(int argc, char** argv)
{
    std::string network_interface = argc > 1 ? argv[1] : "";

    unitree::robot::ChannelFactory::Instance()->Init(0, network_interface);

    Monitor monitor;

    auto sub =
        std::make_shared<unitree::robot::ChannelSubscriber<MotorStates>>(
            "rt/inspire/state");

    sub->InitChannel(
        [&monitor](const void* ptr)
        {
            monitor.OnMessage(ptr);
        });

    while (true)
    {
        monitor.Print();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
