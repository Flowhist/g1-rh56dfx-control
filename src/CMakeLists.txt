cmake_minimum_required(VERSION 3.10)
project(rh56dfx_controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(hand_hold_position hand_hold_position.cpp)

add_executable(hand_clear_fault hand_clear_fault.cpp)

add_executable(hand_web_control hand_web_control.cpp)

add_executable(hand_compliant_teach hand_compliant_teach.cpp)
find_package(yaml-cpp REQUIRED)
target_include_directories(hand_compliant_teach PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/dfx_inspire_service/include)
target_link_libraries(hand_compliant_teach PRIVATE yaml-cpp)
