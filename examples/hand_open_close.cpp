/*
 * Minimal dual-hand open/close test.
 *
 * Run on the G1 Jetson after the official RH56DFX service is working.
 *
 * Usage:
 *   ./hand_open_close [network-interface]
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>

using MotorCmds = unitree_go::msg::dds_::MotorCmds_;

static constexpr const char* kCmdTopic = "rt/inspire/cmd";

static void Fill(MotorCmds& cmd, float q)
{
    q = std::clamp(q, 0.0f, 1.0f);
    for (auto& motor : cmd.cmds())
        motor.q() = q;
}

int main(int argc, char** argv)
{
    std::string network_interface = argc > 1 ? argv[1] : "";

    unitree::robot::ChannelFactory::Instance()->Init(0, network_interface);

    auto pub =
        std::make_shared<unitree::robot::ChannelPublisher<MotorCmds>>(kCmdTopic);
    pub->InitChannel();

    MotorCmds cmd;
    cmd.cmds().resize(12);

    std::cout << "Open" << std::endl;
    Fill(cmd, 1.0f);
    pub->Write(cmd);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Half" << std::endl;
    Fill(cmd, 0.5f);
    pub->Write(cmd);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Close" << std::endl;
    Fill(cmd, 0.0f);
    pub->Write(cmd);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Open" << std::endl;
    Fill(cmd, 1.0f);
    pub->Write(cmd);

    return 0;
}
