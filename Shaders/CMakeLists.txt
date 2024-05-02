set(project Shaders)

file(GLOB ShaderSrc  "*.hlsl")
file(GLOB ShaderInc "include/*.hlsli")
file(GLOB ShaderPass "Passes/*.hlsl")

set_source_files_properties(${ShaderSrc} PROPERTIES VS_TOOL_OVERRIDE "None") 
set_source_files_properties(${ShaderInc} PROPERTIES VS_TOOL_OVERRIDE "None") 
set_source_files_properties(${ShaderPass} PROPERTIES VS_TOOL_OVERRIDE "None") 

add_custom_target(${project} 
    DEPENDS dxc
    SOURCES ${ShaderSrc} ${ShaderInc} ${ShaderPass})

# On Windows - DXC are part of WindowsSDK and there's also DXC in VulkanSDK which supports SPIR-V
if (DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set (WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
elseif (DEFINED ENV{WindowsSDKLibVersion})
    string (REGEX REPLACE "\\\\$" "" WINDOWS_SDK_VERSION "$ENV{WindowsSDKLibVersion}")
else ()
    message (FATAL_ERROR "WindowsSDK is not installed (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is not defined; WindowsSDKLibVersion is '$ENV{WindowsSDKLibVersion}')!")
endif ()

get_filename_component (WINDOWS_SDK_ROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
set (WINDOWS_SDK_BIN "${WINDOWS_SDK_ROOT}/bin/${WINDOWS_SDK_VERSION}/x64")
find_program (dxc_path "${WINDOWS_SDK_BIN}/dxc")
if(NOT dxc_path)
    message(FATAL_ERROR "dxc not found")
else()
    message(STATUS "dxc found at ${dxc_path}")
endif()

find_program (dxc_spirv_path "$ENV{VULKAN_SDK}/Bin/dxc")
if(NOT dxc_spirv_path AND vulkan_lib)
    message(FATAL_ERROR "dxc_spirv not found")
else()
    message(STATUS "dxc_spirv found at ${dxc_spirv_path}")
endif()


foreach(shader ${ShaderSrc})
    # message(STATUS "shader: ${shader}")
    get_filename_component(shader_name ${shader} NAME_WLE)  # 获取shader的名
    get_filename_component(shader_ext ${shader} EXT)        # 获取shader的拓展名
    get_filename_component(shader_dir ${shader} DIRECTORY)   # 获取shader的目录
    # message(STATUS "shader_ext: ${shader_ext}")
    string(REPLACE "." ";" shader_ext_list ${shader_ext})
    list(GET shader_ext_list 1 shader_type)
    # message(STATUS "shader_type: ${shader_type}")

    set(shader_output "${SHADER_OUTPUT_DIRECTORY}/${shader_name}.bin")
    # compile shader to dxil
    if("${shader_type}" STREQUAL "lib")
        set(shader_command ${dxc_path} -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -Qembed_debug) 
    elseif("${shader_type}" STREQUAL "rgen")
        set(shader_command ${dxc_path} -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -Qembed_debug)
    elseif("${shader_type}" STREQUAL "rmis")
        set(shader_command ${dxc_path} -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -Qembed_debug)
    elseif("${shader_type}" STREQUAL "rhit")
        set(shader_command ${dxc_path} -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -Qembed_debug)
    else()
        set(shader_command ${dxc_path} -T ${shader_type}_6_5 -Fo ${shader_output} -E main -Od -Zi ${shader} -Qembed_debug)
    endif()

    add_custom_command(
        TARGET ${project}
        PRE_BUILD COMMAND ${shader_command}
    )

    if(vulkan_lib)
        set(shader_output "${SHADER_OUTPUT_DIRECTORY}/${shader_name}.spv")
        # compile shader to spirv
        if("${shader_type}" STREQUAL "lib")
            set(shader_command ${dxc_spirv_path} -spirv  -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        elseif("${shader_type}" STREQUAL "rgen")
            set(shader_command ${dxc_spirv_path} -spirv -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3 -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_ray_query 
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        elseif("${shader_type}" STREQUAL "rmis")
            set(shader_command ${dxc_spirv_path} -spirv -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3 -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_ray_query 
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        elseif("${shader_type}" STREQUAL "rhit")
            set(shader_command ${dxc_spirv_path} -spirv -T lib_6_5 -Fo ${shader_output} ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3 -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_ray_query 
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all 
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        elseif("${shader_type}" STREQUAL "ms")
            set(shader_command ${dxc_spirv_path} -spirv  -T ms_6_5 -Fo ${shader_output} -E main ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader 
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        else()
            set(shader_command  ${dxc_spirv_path} -spirv -T ${shader_type}_6_5 -Fo ${shader_output} -E main ${shader} -Od -Zi -D SPIRV
                -fspv-target-env=vulkan1.3 
                -fvk-t-shift ${srvOffset} all 
                -fvk-s-shift ${samplerOffset} all
                -fvk-b-shift ${cbvOffset} all 
                -fvk-u-shift ${uavOffset} all)
        endif()

        add_custom_command(
            TARGET ${project}
            PRE_BUILD COMMAND ${shader_command}
        )
    endif()
endforeach()