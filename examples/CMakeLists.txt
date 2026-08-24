cmake_minimum_required(VERSION 3.10)
project(rh56dfx_minimal_examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

add_executable(hand_command_safety_test hand_command_safety_test.cpp)
add_test(NAME hand_command_safety COMMAND hand_command_safety_test)

add_executable(rh56_protocol_test rh56_protocol_test.cpp)
add_test(NAME rh56_protocol COMMAND rh56_protocol_test)

add_executable(hand_single_joint_safe hand_single_joint_safe.cpp)

add_executable(hand_hold_position hand_hold_position.cpp)
