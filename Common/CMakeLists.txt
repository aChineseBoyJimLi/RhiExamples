set(project Common)

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

add_library(${project} STATIC ${SOURCES})
add_dependencies(${project} DirectXMesh)

target_include_directories(${project} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${project} PUBLIC
    d3d
    glm
    stb
    vulkan
    assimp
    DirectXMesh)

target_link_libraries(${project} PRIVATE
    d3dcompiler.lib
    d3d12.lib
    dxgi.lib
    dxguid.lib
    assimp-vc143-mt.lib)

if(vulkan_lib)
    target_link_libraries(${project} PRIVATE ${vulkan_lib})
endif()
