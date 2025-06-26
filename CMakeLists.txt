cmake_minimum_required(VERSION 3.15)
project(nienna CXX)

set(CMAKE_CXX_STANDARD 26)

find_package(volk REQUIRED CONFIG)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(slang REQUIRED CONFIG)

add_executable(nienna
	src/main.cpp
)

add_subdirectory(third_party/vk-bootstrap)

add_subdirectory(third_party/fastgltf)

target_link_libraries(nienna PRIVATE
	SDL3::SDL3
	volk::volk
	vk-bootstrap::vk-bootstrap
	slang::slang
	fastgltf::fastgltf
)
