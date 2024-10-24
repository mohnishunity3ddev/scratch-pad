cmake_minimum_required(VERSION 3.22.0)

project(scratch_pad C)

set(EXTERN_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external)

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
