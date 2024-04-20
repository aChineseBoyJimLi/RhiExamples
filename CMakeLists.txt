cmake_minimum_required(VERSION 3.2)

project(RhiExamples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
set(SHADER_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders")

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIRECTORY})

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

find_program(vulkan_lib "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
if (vulkan_lib)
    message(STATUS "vulkan lib found at: ${vulkan_lib}")
else()
    message(WARNING "Not find vulkan lib")
endif()

if(vulkan_lib)
    set(srvOffset 0)
    set(samplerOffset 128)
    set(cbvOffset 256)
    set(uavOffset 384)
    
    add_compile_definitions(SPIRV_SRV_BINDING_OFFSET=${srvOffset})
    add_compile_definitions(SPIRV_SAMPLER_BINDING_OFFSET=${samplerOffset})
    add_compile_definitions(SPIRV_CBV_BINDING_OFFSET=${cbvOffset})
    add_compile_definitions(SPIRV_UAV_BINDING_OFFSET=${uavOffset})
endif()

add_subdirectory(ThirdParty)
add_subdirectory(Shaders)
add_subdirectory(Common)
add_subdirectory(Examples)