set(project GpuQueryDx)
set(folder "Examples")

file(GLOB sources "*.cpp" "*.h")

add_executable(${project} WIN32 ${sources})
add_dependencies(${project} Common Shaders)
set_target_properties(${project} PROPERTIES FOLDER ${folder})
target_link_libraries(${project} PRIVATE Common )