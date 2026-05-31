cmake_minimum_required(VERSION 3.13)
# NOTE: C가 함께 enable되어야 합니다. Ubuntu 22.04의 llvm-14 LLVMConfig.cmake가
# FindFFI.cmake / FindTerminfo.cmake에서 check_c_source_compiles()를 호출하기 때문입니다.
project(CovPass LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# We pin to LLVM 14 to match the container's clang/llvm packages.
find_package(LLVM 14 REQUIRED CONFIG)

message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM_VERSION = ${LLVM_PACKAGE_VERSION}")

# Build a loadable plugin (.so).  Note PREFIX="" so the file is named
# "CovPass.so" rather than "libCovPass.so" - matches the clang -fpass-plugin
# flag in the rest of the lab.
add_library(CovPass MODULE CovPass.cpp)
set_target_properties(CovPass PROPERTIES PREFIX "")

target_include_directories(CovPass PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(CovPass PRIVATE ${LLVM_DEFINITIONS})

# LLVM is built without RTTI by default; match that to avoid link issues.
target_compile_options(CovPass PRIVATE -fno-rtti -Wall -Wextra -Wno-unused-parameter)
