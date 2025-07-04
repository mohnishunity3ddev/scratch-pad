cmake_minimum_required(VERSION 3.22.0)

project(scratch_pad C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(../cmake_macros/prac.cmake)

SETUP_APP(scratch_pad "scratch_pad")

if(TARGET SharedUtils)
    target_link_libraries(scratch_pad SharedUtils)
endif()

target_compile_definitions(scratch_pad PRIVATE _AMD64_)
