cmake_minimum_required(VERSION 3.22.0)

project(scratch_pad C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXTERN_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Include the module to check compiler flags
include(CheckCCompilerFlag)
# Path to the cpuid tool
set(CPUID_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpuid.exe")
if (NOT EXISTS ${CPUID_TOOL})
    message(FATAL_ERROR "cpuid tool is not present!")
endif()

function(check_and_define flag define_name message_text)
    # check for cpu support.
    execute_process(
        COMMAND "${CPUID_TOOL}" "${message_text}"
        OUTPUT_VARIABLE CPUID_OUTPUT
        ERROR_VARIABLE CPUID_ERROR
        RESULT_VARIABLE CPUID_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (CPUID_RESULT EQUAL 0)
        # check for compiler support
        check_c_compiler_flag("${flag}" HAS_FLAG)
        if (HAS_FLAG)
            # define for the detected instructtion sets.
            add_compile_options("${flag}")
            add_definitions("-D${define_name}")
            message(STATUS "${message_text} detected.")
        else()
            message(STATUS "${message_text} not supported.")
        endif()
    else()
        message(STATUS "${message_text} not supported.")
    endif()
endfunction()

check_and_define("-msse" HAS_SSE "sse")
check_and_define("-msse2" HAS_SSE2 "sse2")
check_and_define("-msse3" HAS_SSE3 "sse3")
check_and_define("-msse4.1" HAS_SSE4_1 "sse41")
check_and_define("-msse4.2" HAS_SSE4_2 "sse42")
check_and_define("-mavx" HAS_AVX "avx")
check_and_define("-mavx2" HAS_AVX2 "avx2")
check_and_define("-mavx512f" HAS_AVX512F "avx512f")
check_and_define("-mavx512dq" HAS_AVX512DQ "avx512dq")
check_and_define("-mavx512bw" HAS_AVX512BW "avx512bw")
check_and_define("-mavx512vl" HAS_AVX512VL "avx512vl")

# set(CMAKE_CXX_STANDARD 20) # this does nothing for MSVC, use target_compile_options below
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS ON)

# Definitions
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()
option(USE_DEBUG "Enter debug mode" OFF)
if (USE_DEBUG)
  add_definitions(-DDEBUG)
endif()

###### Find OpenGL
# find_package(OpenGL REQUIRED)

###### GLFW Stuff
# set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "")
# set(GLFW_BUILD_TESTS        OFF CACHE BOOL "")
# set(GLFW_BUILD_DOCS         OFF CACHE BOOL "")
# set(GLFW_INSTALL            OFF CACHE BOOL "")
# add_subdirectory(external/glfw)

###### Assimp Stuff
# set(ASSIMP_NO_EXPORT            ON CACHE BOOL "")
# set(ASSIMP_BUILD_DRACO          OFF CACHE BOOL "")
# set(ASSIMP_BUILD_ASSIMP_TOOLS   OFF CACHE BOOL "")
# set(ASSIMP_BUILD_TESTS          OFF CACHE BOOL "")
# set(ASSIMP_INSTALL_PDB          OFF CACHE BOOL "")
# set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
# set(ASSIMP_BUILD_OBJ_IMPORTER   ON CACHE BOOL "")
# set(ASSIMP_BUILD_GLTF_IMPORTER  ON CACHE BOOL "")
# if(WIN32)
#     set(ASSIMP_BUILD_ZLIB       ON CACHE BOOL "")
# endif()
# add_subdirectory(external/assimp)

# if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
#     set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes -Wno-defaulted-function-deleted -Wno-braced-scalar-init -Wno-writable-strings -Wno-format -Wno-deprecated-declarations -Wno-macro-redefined")
# endif ()

###### GLAD Stuff
# set(GLAD_SOURCES external/glad/src/glad.c)
# add_library(glad ${GLAD_SOURCES})

###### Include Directories ...
# include_directories(external/glfw/include)
# include_directories(external/glm)
# include_directories(external/glad/include)
# include_directories(external/stb)
# include_directories(external/assimp/include)
# include_directories($<TARGET_FILE_DIR:assimp>/../include) # there is one config.h file which gets build in assimp's build directory by cmake.
# include_directories(external/imgui)
include_directories(common)

# if(WIN32)
#     set_property(TARGET zlibstatic    PROPERTY FOLDER "ThirdPartyLibraries")
# endif()
# set_property(TARGET glfw            PROPERTY FOLDER "ThirdPartyLibraries")
# set_property(TARGET glad            PROPERTY FOLDER "ThirdPartyLibraries")
# set_property(TARGET assimp          PROPERTY FOLDER "ThirdPartyLibraries")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main_expanded.cpp
    COMMAND ${CMAKE_CXX_COMPILER} /EP ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp > ${CMAKE_CURRENT_BINARY_DIR}/main_expanded.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)


# Include Common SharedUtils
add_subdirectory(common)
add_subdirectory(src)