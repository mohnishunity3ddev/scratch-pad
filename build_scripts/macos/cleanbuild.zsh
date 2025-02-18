#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

# Function to log output
log_output() {
    local msg="$1"
    local fgcolor="$2"
    local bgcolor="$3"
    echo -e "\033[${bgcolor};${fgcolor}m${msg}\033[0m"
}

# Function to write alerts
write_alert() {
    log_output "$1" "31" "40"  # Red text on black background
}

# Function to write success messages
write_success() {
    log_output "$1" "32" "40"  # Green text on black background
}

# Function to write warnings
write_warning() {
    log_output "$1" "33" "40"  # Yellow text on black background
}

# Log cleaning start
write_alert "Cleaning ..."

# Remove all contents of the root bin directory
if [[ -d "$BIN_DIR_PATH" ]]; then
    rm -rf "$BIN_DIR_PATH"/*
else
    write_warning "$BIN_DIR_PATH does not exist. Skipping..."
fi

# Remove all build folder contents of chapter subfolders
if [[ -d "$SRC_DIR_PATH" ]]; then
    for chapter in "$SRC_DIR_PATH"/*; do
        if [[ -d "$chapter" ]]; then
            for chapter_subfolder in "$chapter"/*; do
                subfolder_build_path="$chapter_subfolder/$CHAPTER_SUBFOLDERS_BUILD_FOLDER_NAME"
                if [[ -d "$subfolder_build_path" ]]; then
                    rm -rf "$subfolder_build_path"/*
                else
                    write_warning "$subfolder_build_path does not exist. Skipping..."
                fi
            done
        fi
    done
else
    write_warning "$SRC_DIR_PATH does not exist. Skipping..."
fi

# Log cleaning completion
write_success "Cleaning Done!"
