#!/bin/zsh

# Source variables (if any)
if [[ -f "$(dirname $0)/_variables.zsh" ]]; then
    source "$(dirname $0)/_variables.zsh"
fi

# Remove existing aliases if they already exist
unalias rebuild 2>/dev/null
unalias rb 2>/dev/null
unalias ms 2>/dev/null
unalias msvc 2>/dev/null
unalias build 2>/dev/null
unalias cleanall 2>/dev/null
unalias clean 2>/dev/null
unalias goto 2>/dev/null
unalias run 2>/dev/null
unalias opt 2>/dev/null
unalias prof 2>/dev/null
unalias create 2>/dev/null
unalias push 2>/dev/null
unalias pull 2>/dev/null

# Define aliases
alias rebuild="zsh $(dirname $0)/rebuild.zsh"
alias rb="zsh $(dirname $0)/rebuild.zsh"
alias ms="zsh $(dirname $0)/build_msvc.zsh"
alias msvc="zsh $(dirname $0)/build_msvc.zsh"
alias build="zsh $(dirname $0)/build_cmake.zsh"
alias cleanall="zsh $(dirname $0)/cleanbuild.zsh"
alias clean="zsh $(dirname $0)/cleanbuild.zsh"
alias goto="zsh $(dirname $0)/goto.zsh"
alias run="zsh $(dirname $0)/run_project.zsh"
alias opt="zsh $(dirname $0)/optimize.zsh"
alias prof="zsh $(dirname $0)/profile.zsh"
alias create="zsh $(dirname $0)/create_project.zsh"
alias push="zsh $(dirname $0)/git_push.zsh"
alias pull="zsh $(dirname $0)/git_pull.zsh"

# Print success message
echo -e "\033[33mAll command aliases are set. You can use rebuild/rb/msvc/build/clean/goto/run/create/push/pull...\033[0m"
