cmake_minimum_required(VERSION 3.22.1)

project(AgIsoVirtualTerminalNative LANGUAGES CXX)

add_library(agisovirtualterminal_native SHARED native_bridge.cpp)

target_compile_features(agisovirtualterminal_native PRIVATE cxx_std_17)
target_compile_options(agisovirtualterminal_native PRIVATE -Wall -Wextra -Wpedantic)

find_library(log-lib log)
target_link_libraries(agisovirtualterminal_native PRIVATE ${log-lib})
