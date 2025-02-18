#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

# Arguments
chapter_arg="$1"
project_arg="$2"

# Validate arguments
if [[ -z "$chapter_arg" || -z "$project_arg" ]]; then
    echo "Error: Two arguments are required. arg1: Chapter Name; arg2: Project Name" >&2
    exit 1
fi

# Flags for chapter and project found
chapter_found=0
project_found=0

# Iterate through chapters
if [[ -d "$SRC_DIR_PATH" ]]; then
    for chapter in "$SRC_DIR_PATH"/*; do
        if [[ -d "$chapter" ]]; then
            chapter_name=$(basename "$chapter")

            if [[ "$chapter_name" =~ "$chapter_arg" ]]; then
                chapter_found=1
                # echo "Chapter: $chapter_name matched with ChapterArg: $chapter_arg. Path: $chapter"

                # Iterate through projects in the chapter
                for chapter_subfolder in "$chapter"/*; do
                    if [[ -d "$chapter_subfolder" ]]; then
                        project_name=$(basename "$chapter_subfolder")

                        if [[ "$project_name" =~ "$project_arg" ]]; then
                            project_found=1
                            # echo "Project: $project_name matched with ProjectArg: $project_arg. Path: $chapter_subfolder"

                            subfolder_build_path="$chapter_subfolder/$CHAPTER_SUBFOLDERS_BUILD_FOLDER_NAME"

                            if [[ -d "$subfolder_build_path" ]]; then
                                cd "$subfolder_build_path" || exit 1

                                # Find the first executable file (excluding directories)
                                exe=$(find "$subfolder_build_path" -type f -perm +111 | head -n 1)

                                if [[ -n "$exe" ]]; then
                                    echo "Executing: $exe"
                                    "$exe"
                                    exit 0
                                else
                                    echo "Error: No executable found in $subfolder_build_path" >&2
                                    exit 1
                                fi
                            else
                                echo "Error: No bin folder present in $subfolder_build_path" >&2
                                exit 1
                            fi
                        fi
                    fi
                done
            fi
        fi
    done
else
    echo "Error: Source directory $SRC_DIR_PATH does not exist" >&2
    exit 1
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
