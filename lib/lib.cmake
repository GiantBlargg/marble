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
target_compile_definitions(stb PUBLIC STBI_ONLY_JPEG STBI_ONLY_PNG STBI_ONLY_HDR)
target_link_libraries(libs INTERFACE stb)

FetchContent_Declare(IMGUI URL https://github.com/ocornut/imgui/archive/refs/tags/v1.84.2.zip)
if(NOT imgui_POPULATED)
	FetchContent_Populate(IMGUI)
endif()
add_library(imgui
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui glfw ${CMAKE_DL_LIBS})
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DISABLE_DEMO_WINDOWS)
target_link_libraries(libs INTERFACE imgui)

FetchContent_Declare(json
	URL https://github.com/nlohmann/json/releases/download/v3.9.1/include.zip
)
if(NOT json_POPULATED)
	FetchContent_Populate(json)
endif()
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${json_SOURCE_DIR}/include)
target_link_libraries(libs INTERFACE nlohmann_json)

FetchContent_Declare(mikktspace
	GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git
	GIT_TAG 3e895b4
)
if(NOT mikktspace_POPULATED)
	FetchContent_Populate(mikktspace)
endif()
add_library(mikktspace ${mikktspace_SOURCE_DIR}/mikktspace.c)
target_include_directories(mikktspace INTERFACE ${mikktspace_SOURCE_DIR})
target_link_libraries(libs INTERFACE mikktspace)

# find_package(OpenAL REQUIRED CONFIG)
# target_link_libraries(libs INTERFACE OpenAL::OpenAL)
