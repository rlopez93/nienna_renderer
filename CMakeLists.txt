cmake_minimum_required(VERSION 3.15)
project(nienna LANGUAGES C CXX)

add_subdirectory(third_party/fastgltf)


set(CMAKE_CXX_STANDARD 26)
set(CMAKE_PREFIX_PATH $ENV{VULKAN_SDK})
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)

add_executable(nienna)

target_sources(nienna PRIVATE
    src/Allocator.cpp
    src/Camera.cpp
    src/Command.cpp
    src/Device.cpp
    src/FrameContext.cpp
    src/Instance.cpp
    src/PhysicalDevice.cpp
    src/Pipeline.cpp
    src/PipelineLayout.cpp
    src/Renderer.cpp
    src/ResourceAllocator.cpp
    src/ResourceBindings.cpp
    src/ResourceLayout.cpp
    src/ResourceUpdate.cpp
    src/Scene.cpp
    src/SceneResources.cpp
    src/Shader.cpp
    src/Surface.cpp
    src/Swapchain.cpp
    src/Utility.cpp
    src/Window.cpp
    src/gltfLoader.cpp
    src/main.cpp
    src/spirv_reflect.c
    src/stb_image.cpp
    src/vma.cpp
)

file(GLOB IMGUI_SOURCES "third_party/imgui/*.cpp")

target_sources(nienna PRIVATE
${IMGUI_SOURCES}
third_party/imgui/backends/imgui_impl_vulkan.cpp
third_party/imgui/backends/imgui_impl_sdl3.cpp)

target_include_directories(nienna PRIVATE
    include
    third_party/imgui
    third_party/imgui/backends
    ${Vulkan_INCLUDE_DIRS}
)

target_link_directories(nienna PRIVATE
    ${Vulkan_LIBRARIES}
)

target_link_libraries(nienna PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
    fastgltf::fastgltf
    fmt::fmt
    glm::glm
)

set(Vulkan_SLANGC_EXECUTABLE "slangc")

if(NOT Vulkan_SLANGC_EXECUTABLE)
    get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
    find_program(Vulkan_SLANGC_EXECUTABLE
        NAMES slangc
        HINTS ${_Vulkan_LIB_DIR}/../Bin
    )
endif()

if(Vulkan_SLANGC_EXECUTABLE)

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
        # DEPENDS slangc
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


