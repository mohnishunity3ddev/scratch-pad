cmake_minimum_required(VERSION 3.22.0)

project(scratch_pad C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXTERN_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Include the module to check compiler flags
include(CheckCCompilerFlag)
if(WIN32)
    set(CPUID_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpuid_msvc.exe")
else()
    set(CPUID_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpuid.exe")
endif()

# Path to the cpuid tool
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
        if (MSVC)
            # Map GCC/Clang flags to MSVC equivalents
            if ("${flag}" STREQUAL "-msse")
                set(flag "/arch:SSE")
            elseif ("${flag}" STREQUAL "-msse2")
                set(flag "/arch:SSE2")
            elseif ("${flag}" STREQUAL "-msse3")
                set(flag "/arch:SSE2")
            elseif ("${flag}" STREQUAL "-msse4.1")
                set(flag "/arch:SSE2")
            elseif ("${flag}" STREQUAL "-msse4.2")
                set(flag "/arch:SSE2")
            elseif ("${flag}" STREQUAL "-mavx")
                set(flag "/arch:AVX")
            elseif ("${flag}" STREQUAL "-mavx2")
                set(flag "/arch:AVX2")
            endif()
            # there are no specific MSVC cl flags for avx512 variants apparently. just use intrinsics.
            if(NOT flag STREQUAL "-mavx512f" AND
               NOT flag STREQUAL "-mavx512dq" AND
               NOT flag STREQUAL "-mavx512bw" AND
               NOT flag STREQUAL "-mavx512vl")
                add_compile_options("${flag}")
            endif()
            add_definitions("-D${define_name}")
            message(STATUS "[CPUID]: ${message_text} detected.")
        else()
            # check for compiler support
            check_c_compiler_flag("${flag}" HAS_FLAG)
            if (HAS_FLAG)
                # define for the detected instructtion sets.
                add_compile_options("${flag}")
                add_definitions("-D${define_name}")
                message(STATUS "[CPUID]: ${message_text} detected.")
            else()
                message(STATUS "[CPUID]: ${message_text}'s compiler flag ${flag} not supported by compiler.")
            endif()
        endif()
    else()
        message(STATUS "[CPUID]: ${message_text} not supported.")
    endif()
endfunction()

# clang-intrin-flag --- custom-define ---  intrin-message-text
check_and_define("-msse"     HAS_SSE        "sse")
check_and_define("-msse2"    HAS_SSE2       "sse2")
check_and_define("-msse3"    HAS_SSE3       "sse3")
check_and_define("-msse4.1"  HAS_SSE4_1     "sse41")
check_and_define("-msse4.2"  HAS_SSE4_2     "sse42")
check_and_define("-mavx"     HAS_AVX        "avx")
check_and_define("-mavx2"    HAS_AVX2       "avx2")
check_and_define("-mavx512f" HAS_AVX512F    "avx512f")
# check_and_define("-mavx512dq" HAS_AVX512DQ "avx512dq")
# check_and_define("-mavx512bw" HAS_AVX512BW "avx512bw")
# check_and_define("-mavx512vl" HAS_AVX512VL "avx512vl")

# set(CMAKE_CXX_STANDARD 20) # this does nothing for MSVC, use target_compile_options below
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS ON)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Definitions
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

include_directories(common)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main_expanded.cpp
    COMMAND ${CMAKE_CXX_COMPILER} /EP ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp > ${CMAKE_CURRENT_BINARY_DIR}/main_expanded.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# Include Common SharedUtils
add_subdirectory(common)
add_subdirectory(src)