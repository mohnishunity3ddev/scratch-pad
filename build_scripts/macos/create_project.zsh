#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

# Required arguments
proj_name_arg="$1"
chapter_number_arg="$2"

# Default project name if none provided
if [[ -z "$proj_name_arg" ]]; then
    echo "Warning: No project name given. Creating template 'tempproj'"
    proj_name_arg="tempproj"
fi

found_chapter_dir=""
found_chapter_folder_name=""

# Iterate through chapters to find the matching one
if [[ -d "$SRC_DIR_PATH" ]]; then
    for chapter in "$SRC_DIR_PATH"/*; do
        chapter_name=$(basename "$chapter")
        if [[ "$chapter_name" =~ "$chapter_number_arg" ]]; then
            found_chapter_dir="$chapter"
            found_chapter_folder_name="$chapter_name"
            break
        fi
    done
fi

if [[ -n "$found_chapter_dir" ]]; then
    echo "Found Chapter: $found_chapter_dir"

    # Navigate to the chapter directory and create project structure
    cd "$found_chapter_dir" || exit 1
    mkdir -p "$proj_name_arg/bin" "$proj_name_arg/src"
    touch "$proj_name_arg/src/$proj_name_arg.cpp"
    touch "$proj_name_arg/CMakeLists.txt"

    # Determine chapter number
    chapter_number="##"
    if [[ -n "$chapter_number_arg" ]]; then
        chapter_number="$chapter_number_arg"
    fi

    # Add content to CMakeLists.txt
    cat << EOF > "$proj_name_arg/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22.0)

project($proj_name_arg)

include(../../../cmake_macros/prac.cmake)

SETUP_APP($proj_name_arg "Chapter$chapter_number")

if(TARGET SharedUtils)
    target_link_libraries($proj_name_arg SharedUtils)
endif()
EOF

    # Add content to the C++ source file
    cat << EOF > "$proj_name_arg/src/$proj_name_arg.cpp"
#include <iostream>

int main() {
    std::cout << "Hello, World! from $proj_name_arg\\n";
    return 0;
}
EOF

    # Update root CMakeLists.txt
    echo "add_subdirectory(src/$found_chapter_folder_name/$proj_name_arg)" >> "$PROJECT_ROOT_DIR_PATH/CMakeLists.txt"

else
    echo "Error: Could not find Chapter associated with given arg: $chapter_number_arg" >&2
    exit 1
fi
