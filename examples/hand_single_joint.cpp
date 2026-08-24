/*
 * Minimal single-dimension test.
 *
 * Default behavior:
 *   keep all dimensions open, then move right index only.
 *
 * Canonical index:
 *   3 = right_index
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

int main(int argc, char** argv)
{
    std::string network_interface = argc > 1 ? argv[1] : "";

    unitree::robot::ChannelFactory::Instance()->Init(0, network_interface);

    auto pub =
        std::make_shared<unitree::robot::ChannelPublisher<MotorCmds>>(
            "rt/inspire/cmd");
    pub->InitChannel();

    MotorCmds cmd;
    cmd.cmds().resize(12);

    for (auto& motor : cmd.cmds())
        motor.q() = 1.0f;

    std::cout << "Opening all dimensions..." << std::endl;
    pub->Write(cmd);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Moving right index to 0.3..." << std::endl;
    cmd.cmds()[3].q() = 0.3f;
    pub->Write(cmd);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Returning right index to 1.0..." << std::endl;
    cmd.cmds()[3].q() = 1.0f;
    pub->Write(cmd);

    return 0;
}
