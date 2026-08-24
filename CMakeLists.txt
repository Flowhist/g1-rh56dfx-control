cmake_minimum_required(VERSION 3.10)
project(g1_rh56dfx_control LANGUAGES C CXX)

include(CTest)
add_subdirectory(third_party/dfx_inspire_service)
add_subdirectory(examples)
