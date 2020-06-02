cmake_minimum_required(VERSION 3.0.0)

project(ng VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(sdl2-image CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)

file(GLOB_RECURSE sources "src/*.cpp")
add_executable(ng ${sources})

target_include_directories(ng PUBLIC "include")
target_link_libraries(ng PRIVATE glm)
target_link_libraries(ng PRIVATE SDL2::SDL2main SDL2::SDL2-static)
target_link_libraries(ng PRIVATE SDL2::SDL2_image)
target_link_libraries(ng PRIVATE fmt::fmt fmt::fmt-header-only)
target_link_libraries(ng PRIVATE unofficial::gl3w::gl3w)
target_link_libraries(ng PRIVATE freetype)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)