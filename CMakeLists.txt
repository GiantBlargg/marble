cmake_minimum_required(VERSION 3.17)

project(marble LANGUAGES CXX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
add_executable(marble ${SOURCES})
target_include_directories(marble PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)
install(TARGETS marble DESTINATION .)

set_property(TARGET marble PROPERTY CXX_STANDARD 20)
set_property(TARGET marble PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET marble PROPERTY CXX_EXTENSIONS OFF)

if (MSVC)
	target_compile_options(marble PUBLIC /W4)
else()
	target_compile_options(marble PUBLIC -Wall -Wextra -pedantic)
endif()

include(lib/lib.cmake)
include(shaders.cmake)
target_link_libraries(marble libs shaders)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/assets DESTINATION .)
