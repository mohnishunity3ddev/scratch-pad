cmake_minimum_required(VERSION 3.22.0)

project(scratch_pad)

include(../cmake_macros/prac.cmake)

SETUP_APP(scratch_pad "scratch_pad")

if(TARGET SharedUtils)
    target_link_libraries(scratch_pad SharedUtils)
endif()
