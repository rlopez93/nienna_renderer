cmake_minimum_required(VERSION 3.15)
project(nienna CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_PREFIX_PATH $ENV{VULKAN_SDK})
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(nienna src/main.cpp)

target_include_directories(nienna PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_link_directories(nienna PRIVATE
    ${Vulkan_LIBRARIES}
)

add_subdirectory(third_party/vk-bootstrap)
add_subdirectory(third_party/fastgltf)

target_link_libraries(nienna PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
    fastgltf::fastgltf
)
