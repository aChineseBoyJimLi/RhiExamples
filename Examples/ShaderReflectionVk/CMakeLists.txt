set(project ShaderReflectionVk)
set(folder "Examples")

file(GLOB sources "*.cpp" "*.h")
set(base_sources ${CMAKE_CURRENT_SOURCE_DIR}/../AppBaseVk.h
	            ${CMAKE_CURRENT_SOURCE_DIR}/../AppBaseVk.cpp)

add_executable(${project} WIN32 ${sources} ${base_sources})
add_dependencies(${project} Common Shaders)
set_target_properties(${project} PROPERTIES FOLDER ${folder})
set_target_properties(${project} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
target_link_libraries(${project} PRIVATE Common )