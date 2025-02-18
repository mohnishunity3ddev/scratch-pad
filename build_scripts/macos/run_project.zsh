#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

exe="$BIN_DIR_PATH/src/$PROJECT_NAME"_Debug
$exe
