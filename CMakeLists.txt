cmake_minimum_required(VERSION 3.24)

project(UniversalAppearance VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static triplet to avoid spdlog.dll dependency errors
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
endif()

include(FetchContent)

# Fetch CommonLibF4
FetchContent_Declare(
    CommonLibF4
    GIT_REPOSITORY https://github.com/Ryan-rsm-McKenzie/CommonLibF4.git
    SOURCE_SUBDIR CommonLibF4
)
FetchContent_MakeAvailable(CommonLibF4)

# Find Dependencies
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

# Define Plugin Name
set(PLUGIN_NAME "UniversalAppearance")

# Source Files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_library(${PLUGIN_NAME} SHARED ${SOURCES} ${HEADERS})

# Precompiled Header
target_precompile_headers(${PLUGIN_NAME} PRIVATE src/PCH.h)

# Include Directories
target_include_directories(${PLUGIN_NAME} PRIVATE src)

# Link Libraries
target_link_libraries(${PLUGIN_NAME} 
    PRIVATE 
        CommonLibF4::CommonLibF4
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# F4SE Plugin Definitions
target_compile_definitions(${PLUGIN_NAME} PRIVATE 
    F4SE_SUPPORT_XSE
    UNICODE
    _UNICODE
)

# Output properties
set_target_properties(${PLUGIN_NAME} PROPERTIES 
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

if(MSVC)
    target_compile_options(${PLUGIN_NAME} PRIVATE /MP /Zm200)
endif()
