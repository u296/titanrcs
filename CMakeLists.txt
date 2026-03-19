cmake_minimum_required(VERSION 4.0.0)
project(titanrcs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)




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

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG master
)
FetchContent_MakeAvailable(vma)



add_executable(titanrcs 
    src/main.c
    src/cleanup.c
    src/backend/instance.c
    src/backend/device.c
    src/backend/allocator.c
    src/backend/backend.c
    src/swapchaincontext/swapchain.c
    src/swapchaincontext/renderpass.c
    src/swapchaincontext/framebuffers.c
    src/swapchaincontext/swapchaincontext.c
    src/resources/commandpool.c
    src/resources/sync.c
    src/resources/renderresources.c
    src/pipeline.c
    src/buffers.c
    src/linalg.c
    src/descriptors.c
    src/vma_impl.cpp
    src/render.c
    )

set_source_files_properties(src/vma_impl.cpp PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/vma_impl.cpp PROPERTIES COMPILE_FLAGS "-Wno-all")


set_target_properties(titanrcs PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
    COMPILE_FLAGS "-Wall"
    C_STANDARD 23
)

set_source_files_properties(src/vma_impl.cpp PROPERTIES COMPILE_FLAGS "-w")


target_link_libraries(titanrcs PRIVATE volk::volk glfw VulkanMemoryAllocator)


if(APPLE)
    # Get the SDK path automatically
    execute_process(COMMAND xcrun --show-sdk-path 
                    OUTPUT_VARIABLE APPLE_SDK_PATH 
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    
    
    message("SDK PATH IS ${APPLE_SDK_PATH}")
    target_include_directories(titanrcs PRIVATE "${APPLE_SDK_PATH}/usr/include/c++/v1")
endif()


target_include_directories(titanrcs PRIVATE "${CMAKE_SOURCE_DIR}/include")
