cmake_minimum_required(VERSION 3.25)
project(nienna LANGUAGES CXX)

if(NOT DEFINED ENV{VULKAN_SDK})
  message(FATAL_ERROR "VULKAN_SDK is not set; source the Vulkan SDK's setup-env.sh first")
endif()

find_package(Vulkan 1.4 REQUIRED)
find_package(slang REQUIRED)
find_package(SDL3 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
add_subdirectory(third_party/fastgltf)

file(GLOB IMGUI_SOURCES third_party/imgui/*.cpp)

add_executable(nienna
    src/AABB.cpp
    src/Allocator.cpp
    src/Camera.cpp
    src/Command.cpp
    src/Device.cpp
    src/FrameContext.cpp
    src/GltfLoader.cpp
    src/ImGuiLayer.cpp
    src/ImageLayoutState.cpp
    src/Instance.cpp
    src/MaterialPacking.cpp
    src/PhysicalDevice.cpp
    src/Pipeline.cpp
    src/PipelineLayout.cpp
    src/RenderableResources.cpp
    src/RenderContext.cpp
    src/RenderTargets.cpp
    src/Renderer.cpp
    src/SceneView.cpp
    src/Shader.cpp
    src/ShaderInterface.cpp
    src/Surface.cpp
    src/Swapchain.cpp
    src/Sync.cpp
    src/UniqueImage.cpp
    src/Utility.cpp
    src/Window.cpp
    src/main.cpp
    src/stb_image.cpp
    src/vma.cpp
    ${IMGUI_SOURCES}
    third_party/imgui/backends/imgui_impl_vulkan.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
)

target_compile_features(nienna PRIVATE cxx_std_26)

target_compile_definitions(nienna PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

target_include_directories(nienna PRIVATE
    include
    third_party/imgui
    third_party/imgui/backends
)

target_link_libraries(nienna PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    fastgltf::fastgltf
    fmt::fmt
    glm::glm
    $ENV{VULKAN_SDK}/lib/libspirv-reflect-static.a
)

file(GLOB SHADER_SOURCES shaders/*.slang)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_autogen)
foreach(SHADER ${SHADER_SOURCES})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  set(OUTPUT_FILE ${CMAKE_BINARY_DIR}/_autogen/${SHADER_NAME}.spv)
  add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SLANG_EXECUTABLE}
            -I ${CMAKE_SOURCE_DIR}/include
            -profile sm_6_6+spirv_1_6
            -target spirv
            -force-glsl-scalar-layout
            -fvk-use-entrypoint-name
            -g3 -O0
            -o ${OUTPUT_FILE} ${SHADER}
        MAIN_DEPENDENCY ${SHADER}
        VERBATIM
    )
  target_sources(nienna PRIVATE ${OUTPUT_FILE})
endforeach()
