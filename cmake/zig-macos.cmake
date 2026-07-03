# CMake toolchain file for Zig cross-compilation (Linux → macOS)
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use zig as the C/C++ compiler
find_program(CMAKE_C_COMPILER NAMES zig)
find_program(CMAKE_CXX_COMPILER NAMES zig)

# Zig acts as a compiler wrapper with -target flag
set(CMAKE_C_COMPILER_ARG1 "cc -target x86_64-macos")
set(CMAKE_CXX_COMPILER_ARG1 "c++ -target x86_64-macos")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# macOS-specific settings
set(APPLE TRUE)
set(DARWIN TRUE)
