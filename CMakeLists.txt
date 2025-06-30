cmake_minimum_required(VERSION 3.15)
project(nienna LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_PREFIX_PATH $ENV{VULKAN_SDK})
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(nienna src/main.cpp)

add_library(spirv_reflect STATIC spirv_reflect.c)

target_include_directories(nienna PRIVATE
    include
    ${Vulkan_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}/SPIRV-Reflect
    ${Vulkan_INCLUDE_DIRS}/SPIRV-Reflect/include
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
    spirv_reflect
)

if(NOT Vulkan_SLANGC_EXECUTABLE)
    get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
    find_program(Vulkan_SLANGC_EXECUTABLE
        NAMES slangc
        HINTS ${_Vulkan_LIB_DIR}/../Bin
    )
endif()

if( Vulkan_SLANGC_EXECUTABLE)

    set(SLANG_FLAGS
        -profile sm_6_6+spirv_1_6
        -capability spvInt64Atomics+spvShaderInvocationReorderNV+spvShaderClockKHR+spvRayTracingMotionBlurNV
        -target spirv
        -emit-spirv-directly
        -force-glsl-scalar-layout
        -fvk-use-entrypoint-name
        -g3  # Enable debug information
        -O0  # Disable optimization (spirv-opt)
        -source-embed-style text
    )

    # Compile Slang shaders using slangc
    file(GLOB SHADER_SLANG_FILES "shaders/*.slang")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_autogen)
    foreach(SHADER ${SHADER_SLANG_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        string(REPLACE "." "_" VN_SHADER_NAME ${SHADER_NAME})
        set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/_autogen/${SHADER_NAME}.spv")
        set(_COMMAND ${Vulkan_SLANGC_EXECUTABLE}
        ${_SLANG_FLAGS}
        -source-embed-name ${VN_SHADER_NAME}
        -o ${OUTPUT_FILE} ${SHADER})

        add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${_COMMAND}
        MAIN_DEPENDENCY ${SHADER}
        COMMENT "Compiling Slang shader ${SHADER_NAME} with slangc"
        VERBATIM
        )
        list(APPEND SHADER_HEADERS ${OUTPUT_FILE})
    endforeach()
else()
    message(ERROR "slangc executable not found")
endif()

# Add shader files to the project
file(GLOB SHADER_H_FILES "shaders/*.spv")
source_group("Shaders" FILES ${SHADER_SLANG_FILES} ${SHADER_H_FILES})
target_sources(nienna PRIVATE ${SHADER_H_FILES})

# Add the shader headers directly to the executable target
target_sources(nienna PRIVATE ${SHADER_HEADERS})


# Include the shader and _autogen directory
target_include_directories(nienna PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(nienna PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")


