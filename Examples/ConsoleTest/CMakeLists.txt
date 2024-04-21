set(project ConsoleTest)
set(folder "Examples")

file(GLOB sources "*.cpp" "*.h")

add_executable(${project} ${sources})
add_dependencies(${project} Common Shaders)
set_target_properties(${project} PROPERTIES FOLDER ${folder})
set_target_properties(${project} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
target_link_libraries(${project} PRIVATE Common )