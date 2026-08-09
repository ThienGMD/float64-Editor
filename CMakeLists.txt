cmake_minimum_required(VERSION 3.21)

project(float64Editor VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Cấu hình kiến trúc riêng tùy theo nền tảng mục tiêu
if (NOT ANDROID)
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin" OR APPLE)
        # Hỗ trợ cả Mac Intel lẫn Apple Silicon (M1/M2/M3/M4)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    endif()
endif()

# Kiểm tra Geode SDK
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

# Thêm tất cả tệp nguồn trong thư mục src
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Thiết lập thư viện chia sẻ cho mod
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Thêm Geode SDK subdirectory
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Thiết lập dependencies, resources và liên kết với Geode
setup_geode_mod(${PROJECT_NAME})
