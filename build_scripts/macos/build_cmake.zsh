#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

# Parse arguments
arg="$1"

# Code Gen Specific
code_gen=""
is_xcode=0
enable_tsan=0
# Determine build type
is_release=0
if [[ "$arg" == "rel" || "$arg" == "release" || "$arg" == "r" ]]; then
    is_release=1
elif [[ "$arg" == "xcode" ]]; then
    code_gen="-GXcode"
    is_xcode=1
elif [[ "$arg" == "tsan" ]]; then
    enable_tsan=1
fi

# Adjust project root path
TMP_PROJECT_ROOT="$PROJECT_ROOT_DIR_PATH"
TMP_PROJECT_ROOT="${TMP_PROJECT_ROOT//\/../}"
TMP_PROJECT_ROOT=$(dirname "$TMP_PROJECT_ROOT")
cwd=$(pwd)
t1="$TMP_PROJECT_ROOT/$BUILD_FOLDER_NAME"
InsideBin=0
if [[ "$t1" == "$cwd" ]]; then
    InsideBin=1
fi

# Build using CMake
echo "PROJECT_ROOT_DIR_PATH=$PROJECT_ROOT_DIR_PATH; BIN_DIR_PATH=$BIN_DIR_PATH"
# cd "$BIN_DIR_PATH" || exit 1
if [[ $is_release -eq 0 ]]; then
    if [[ enable_tsan -eq 1 ]]; then
        echo "TSan is ON"
        cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
              -DENABLE_TSAN=ON \
              -DCMAKE_BUILD_TYPE=Debug \
              -S "$PROJECT_ROOT_DIR_PATH" \
              -B "$BIN_DIR_PATH"
    else
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -S "$PROJECT_ROOT_DIR_PATH" \
              -B "$BIN_DIR_PATH" $code_gen
    fi
else
    cmake -DCMAKE_BUILD_TYPE=Release \
          -S "$PROJECT_ROOT_DIR_PATH" \
          -B "$BIN_DIR_PATH" $code_gen
fi

cd "$BIN_DIR_PATH" || exit 1

# Build CMake files with Make
if [[ is_xcode -eq 0 ]]; then
    make
    # for vscode clangd linter. only works with make(?)
    mv $BIN_DIR_PATH/compile_commands.json $PROJECT_ROOT_DIR_PATH/compile_commands.json
else
    xcodebuild -project $BIN_DIR_PATH/$PROJECT_NAME.xcodeproj
    open $BIN_DIR_PATH/$PROJECT_NAME.xcodeproj
fi

# Return to the previous directory if not inside the bin directory
if [[ $InsideBin -eq 0 ]]; then
    cd -
fi
