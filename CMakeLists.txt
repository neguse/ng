﻿cmake_minimum_required(VERSION 3.10.0)

project(ng VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(sdl2_image CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(Freetype REQUIRED)

file(GLOB_RECURSE sources "src/*.cpp")
add_executable(ng ${sources})

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 --preload-file ${CMAKE_SOURCE_DIR}/asset@asset -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy -sGL_ENABLE_GET_PROC_ADDRESS")
else()
    target_link_libraries(ng PRIVATE unofficial::gl3w::gl3w)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
endif()

target_include_directories(ng PUBLIC "include" "src")
target_link_libraries(ng PRIVATE glm::glm)
target_link_libraries(ng PRIVATE SDL2::SDL2main SDL2::SDL2)
target_link_libraries(ng PRIVATE SDL2_image::SDL2_image-static)
target_link_libraries(ng PRIVATE fmt::fmt-header-only)
target_link_libraries(ng PRIVATE Freetype::Freetype)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
