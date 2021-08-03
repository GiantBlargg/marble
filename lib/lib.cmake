include(FetchContent)

add_library(libs INTERFACE)

if(UNIX)
	find_package(glfw3 3.3 REQUIRED CONFIG)
else()
	FetchContent_Declare(GLFW
		GIT_REPOSITORY https://github.com/glfw/glfw.git
		GIT_TAG 3.3.4
	)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(GLFW)
endif()
target_compile_definitions(libs INTERFACE GLFW_INCLUDE_NONE)
target_link_libraries(libs INTERFACE glfw)

include(${CMAKE_CURRENT_LIST_DIR}/gl/gl.cmake)
target_link_libraries(libs INTERFACE gl)

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

FetchContent_Declare(json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.9.1)
set(JSON_BuildTests OFF)
set(JSON_Install OFF)
set(JSON_MultipleHeaders ON)
set(JSON_ImplicitConversions OFF)
FetchContent_MakeAvailable(json)
target_link_libraries(libs INTERFACE nlohmann_json::nlohmann_json)

# find_package(OpenAL REQUIRED CONFIG)
# target_link_libraries(libs INTERFACE OpenAL::OpenAL)
