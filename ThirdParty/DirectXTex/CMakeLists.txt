set(project DirectXTex)
set(folder "ThirdParty")

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(${project} STATIC ${src})
set_target_properties(${project} PROPERTIES FOLDER ${folder})
target_include_directories(${project} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Compiled)
target_include_directories(${project} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})