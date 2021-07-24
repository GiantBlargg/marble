include(FetchContent)

add_library(libs INTERFACE)

find_package(glfw3 3.3 REQUIRED CONFIG)
target_compile_definitions(libs INTERFACE GLFW_INCLUDE_NONE)
target_link_libraries(libs INTERFACE glfw)

include(${CMAKE_CURRENT_LIST_DIR}/gl/gl.cmake)
target_link_libraries(libs INTERFACE gl)

find_package(assimp 5.0.1 REQUIRED CONFIG)
target_link_libraries(libs INTERFACE ${ASSIMP_LIBRARIES})

FetchContent_Declare(GLM
	URL https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
)
FetchContent_MakeAvailable(GLM)
target_link_libraries(libs INTERFACE glm)

add_library(stb ${CMAKE_CURRENT_LIST_DIR}/stb/stb.cpp)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb/)
target_link_libraries(libs INTERFACE stb)

file(GLOB_RECURSE IMGUI_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/imgui/*.cpp)
add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imgui/)
target_link_libraries(imgui gl glfw)
target_link_libraries(libs INTERFACE imgui)

# find_package(OpenAL REQUIRED CONFIG)
# target_link_libraries(libs INTERFACE OpenAL::OpenAL)
