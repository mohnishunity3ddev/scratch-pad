#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

# Parse arguments
first_arg="$1"
second_arg="$2"
third_arg="$3"

# Validate arguments
if [[ -z "$first_arg" && -z "$second_arg" ]]; then
    echo "Error: At least one argument required." >&2
    exit 1
fi

# Handle predefined paths
case "$first_arg" in
    "scripts")
        cd "$BUILD_SCRIPTS_DIR_PATH" || exit 1
        exit 0
        ;;
    "root")
        cd "$PROJECT_ROOT_DIR_PATH" || exit 1
        exit 0
        ;;
    "bin")
        cd "$BIN_DIR_PATH" || exit 1
        exit 0
        ;;
    "extern")
        cd "$PROJECT_ROOT_DIR_PATH/external" || exit 1
        exit 0
        ;;
esac

# Handle chapter and project arguments
chapter_arg="$first_arg"
project_arg="$second_arg"

chapter_found=0
project_found=0

# Search for chapters
if [[ -d "$SRC_DIR_PATH" ]]; then
    for chapter in "$SRC_DIR_PATH"/*; do
        chapter_name=$(basename "$chapter")

        if [[ "$chapter_name" =~ "$chapter_arg" ]]; then
            chapter_found=1
            # echo "Chapter: $chapter_name matched with ChapterArg: $chapter_arg. Path: $chapter"

            # Search for projects in the chapter
            for chapter_subfolder in "$chapter"/*; do
                project_name=$(basename "$chapter_subfolder")

                if [[ "$project_name" =~ "$project_arg" ]]; then
                    project_found=1
                    # echo "Project: $project_name matched with ProjectArg: $project_arg. Path: $chapter_subfolder"

                    # Handle third argument for subdirectories
                    if [[ -n "$third_arg" ]]; then
                        subfolder_path="$chapter_subfolder/$third_arg"
                        if [[ -d "$subfolder_path" ]]; then
                            cd "$subfolder_path" || exit 1
                            exit 0
                        fi
                    fi
                
                    # Normalize path and check existence
                    chapter_subfolder=$(realpath "$chapter_subfolder")
                    if [[ -d "$chapter_subfolder" ]]; then
                        echo "Changing directory to: $chapter_subfolder"
                        # cd "$chapter_subfolder" || { echo "Error: Failed to change directory to $chapter_subfolder"; exit 1; }
                        pcd "$chapter_subfolder"
                        exit 0
                    else
                        echo "Error: Directory does not exist: $chapter_subfolder" >&2
                        exit 1
                    fi
                fi
            done
        fi
    done
fi

# Error handling for missing chapter or project
if [[ $chapter_found -eq 0 ]]; then
    echo "Error: Could not find Chapter [$chapter_arg] in the project" >&2
    exit 1
fi

if [[ $project_found -eq 0 ]]; then
    echo "Error: Could not find Project [$project_arg] in Chapter [$chapter_arg]" >&2
    exit 1
fi
