cmake_minimum_required(VERSION 3.15)
project(nienna CXX)

set(CMAKE_CXX_STANDARD 26)

find_package(Vulkan REQUIRED)
find_package(volk REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(nienna
	src/main.cpp
)

add_subdirectory(third_party/vk-bootstrap)

set(TINYGLTF_HEADER_ONLY ON)
set(TINYGLTF_INSTALL OFF)
add_subdirectory(third_party/tinygltf)

target_include_directories(nienna PRIVATE third_party/tinygltf)
target_link_libraries(nienna PRIVATE SDL3::SDL3 volk::volk vk-bootstrap::vk-bootstrap)
