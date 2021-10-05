cmake_minimum_required(VERSION 3.20)

project(marble LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
add_executable(marble ${SOURCES})
target_include_directories(marble PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)
install(TARGETS marble DESTINATION .)

if (MSVC)
	target_compile_options(marble PUBLIC /W4)
else()
	target_compile_options(marble PUBLIC -Wall -Wextra -pedantic)
endif()

include(lib/lib.cmake)
include(shaders.cmake)
target_link_libraries(marble libs shaders)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/assets DESTINATION .)
