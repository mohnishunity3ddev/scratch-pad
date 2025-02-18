#!/bin/zsh

# Variables
PROJECT_NAME="scratch_pad"

BUILD_SCRIPTS_DIR_PATH="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT_PATH_RELATIVE_FROM_SCRIPT="../../"
BUILD_FOLDER_NAME="bin"
SRC_FOLDER_NAME="src"
BUILD_SCRIPTS_FOLDER_NAME="build_scripts"
CHAPTER_SUBFOLDERS_BUILD_FOLDER_NAME="bin"

PROJECT_ROOT_DIR_PATH="$(cd "$BUILD_SCRIPTS_DIR_PATH/$PROJECT_ROOT_PATH_RELATIVE_FROM_SCRIPT" && pwd)"
BIN_DIR_PATH="$PROJECT_ROOT_DIR_PATH/$BUILD_FOLDER_NAME"
SRC_DIR_PATH="$PROJECT_ROOT_DIR_PATH/$SRC_FOLDER_NAME"
BUILD_SCRIPTS_DIR_PATH="$PROJECT_ROOT_DIR_PATH/$BUILD_SCRIPTS_FOLDER_NAME"

# Functions

# Get the full path of a relative path
get_full_path() {
    local relative_path="$1"
    if [[ -z "$relative_path" ]]; then
        echo "Error: No path provided" >&2
        return 1
    fi
    realpath "$relative_path"
}

# Navigate to a parent directory containing the specified name
function pcd {
    target="$1"
    if [[ -z "$target" ]]; then
        echo "Usage: pcd <directory-name>" >&2
        return 1
    fi
    cd "$target"
}
