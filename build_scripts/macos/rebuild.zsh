#!/bin/zsh

# Import all variables from _variables.zsh
source "$(dirname "$0")/_variables.zsh"

zsh "$(dirname "$0")/cleanbuild.zsh"
zsh "$(dirname "$0")/build_cmake.zsh"
