cmake_minimum_required(VERSION 4.0.0)
project(titanrcs VERSION 0.1.0 LANGUAGES C)

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(volk)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glfw)



add_executable(titanrcs 
    src/main.c
    src/cleanup.c
    src/instance.c
    src/device.c
    src/swapchain.c
    src/renderpass.c
    src/pipeline.c
    src/framebuffers.c
    src/commandpool.c
    src/sync.c
    src/buffers.c
    src/linalg.c
    src/descriptors.c
    )

set_target_properties(titanrcs PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
    COMPILE_FLAGS "-Wall"
    C_STANDARD 23
)

target_link_libraries(titanrcs PRIVATE volk::volk glfw)




