cmake_minimum_required(VERSION 3.23)

project(VkTestBed)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_subdirectory(third_party)

set(PROJECT_WORK_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data)

set(SOURCE_FILES
        src/main.cpp
        src/Instance.cpp
        src/Instance.hpp
        src/VkCommon.hpp
        src/PhysicalDevice.cpp
        src/PhysicalDevice.hpp
        src/Debug.cpp
        src/Debug.hpp
        src/VkUnit.hpp
        src/Device.cpp
        src/Device.hpp
        src/Queue.cpp
        src/Queue.hpp
        src/SwapChain.cpp
        src/SwapChain.hpp
        src/RenderContext.cpp
        src/RenderContext.hpp
        src/Image.cpp
        src/Image.hpp
        src/ImageView.cpp
        src/ImageView.hpp
        src/RenderTarget.cpp
        src/RenderTarget.hpp
        src/RenderFrame.cpp
        src/RenderFrame.hpp
        src/FencePool.cpp
        src/FencePool.hpp
        src/SemaphorePool.cpp
        src/SemaphorePool.hpp
        src/CommandBuffer.cpp
        src/CommandBuffer.hpp
        src/CommandBufferPool.cpp
        src/CommandBufferPool.hpp
        src/Buffer.cpp
        src/Buffer.hpp
        src/BufferPool.cpp
        src/BufferPool.hpp
        src/Helpers.hpp
        src/DescriptorSetLayout.cpp
        src/DescriptorSetLayout.hpp
        src/ShaderModule.cpp
        src/ShaderModule.hpp
        src/ShaderUtils.hpp
        src/ShaderUtils.cpp
        src/DescriptorPool.cpp
        src/DescriptorPool.hpp
        src/DescriptorSet.cpp
        src/DescriptorSet.hpp
        src/ResourceCaching.hpp
)

add_executable(Vk ${SOURCE_FILES})
set_target_properties(Vk
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
        CXX_STANDARD 17
)
target_compile_definitions(Vk
        PUBLIC
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        # Windows.
        $<$<PLATFORM_ID:Windows>:NOMINMAX>  # do not define min/max macros
        $<$<PLATFORM_ID:Windows>:UNICODE>   # force character map to unicode
        # MSVC C++ library.
        $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>
        $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<CXX_COMPILER_ID:MSVC>:_ENABLE_EXTENDED_ALIGNED_STORAGE>
        $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
        # Clang.
        $<$<CXX_COMPILER_ID:Clang>:_MSC_EXTENSIONS> # enable MS extensions
        PRIVATE
        #$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        _PROJECT_DIR_= "${CMAKE_CURRENT_SOURCE_DIR}/"
)

target_link_libraries(Vk 
#        Vulkan::Vulkan 
        glfw 
        glm::glm 
        tinyobjloader::tinyobjloader 
        spdlog::spdlog 
        GPUOpen::VulkanMemoryAllocator
        SPIRV
        glslang 
        glslang-default-resource-limits
        spirv-cross-glsl
)

#message(STATUS ${Vulkan_INCLUDE_DIR})
target_include_directories(Vk 
        PUBLIC 
        ${Vulkan_INCLUDE_DIR}
)