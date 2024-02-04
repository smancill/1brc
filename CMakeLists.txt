# SPDX-FileCopyrightText: © Sebastián Mancilla <smancill@smancill.dev>
#
# SPDX-License-Identifier: MIT-0

cmake_minimum_required(VERSION 3.16.3)
project(1BRC LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release MinSizeRel RelWithDebInfo)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

add_executable(baseline src/baseline.cpp)
add_executable(main src/main.cpp)
