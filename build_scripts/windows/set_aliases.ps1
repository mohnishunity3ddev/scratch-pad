. $PSScriptRoot\_variables.ps1

Set-Alias -Name rebuild     -Value "undefined"
Set-Alias -Name rb          -Value "undefined"
Set-Alias -Name ms          -Value "undefined"
Set-Alias -Name msvc        -Value "undefined"
Set-Alias -Name build       -Value "undefined"
Set-Alias -Name cleanall    -Value "undefined"
Set-Alias -Name goto        -Value "undefined"
Set-Alias -Name run         -Value "undefined"
Set-Alias -Name push        -Value "undefined"
Set-Alias -Name pull        -Value "undefined"

if(Test-Path alias:rebuild*)    { Remove-Alias rebuild }
if(Test-Path alias:rb*)         { Remove-Alias rb }
if(Test-Path alias:build*)      { Remove-Alias build }
if(Test-Path alias:cleanall*)   { Remove-Alias cleanall }
if(Test-Path alias:goto*)       { Remove-Alias goto }
if(Test-Path alias:run*)        { Remove-Alias run }
if(Test-Path alias:msvc*)       { Remove-Alias msvc }
if(Test-Path alias:ms*)         { Remove-Alias ms }
if(Test-Path alias:push*)       { Remove-Alias push }
if(Test-Path alias:pull*)       { Remove-Alias pull }

Set-Alias -Name rebuild -Value $PSScriptRoot\rebuild.ps1 -Scope Global
Set-Alias -Name rb -Value $PSScriptRoot\rebuild.ps1 -Scope Global

Set-Alias -Name ms -Value $PSScriptRoot\build_msvc.ps1 -Scope Global
Set-Alias -Name msvc -Value $PSScriptRoot\build_msvc.ps1 -Scope Global

Set-Alias -Name build -Value $PSScriptRoot\build_cmake.ps1 -Scope Global

Set-Alias -Name cleanall -Value $PSScriptRoot\cleanbuild.ps1 -Scope Global

Set-Alias -Name goto -Value $PSScriptRoot\goto.ps1 -Scope Global
Set-Alias -Name run -Value $PSScriptRoot\run_project.ps1 -Scope Global

Set-Alias -Name push -Value $PSScriptRoot\git_push.ps1 -Scope Global
Set-Alias -Name pull -Value $PSScriptRoot\git_pull.ps1 -Scope Global

Write-Yellow "All Command Aliases are set. You can use build/rebuild/msvc/cleanall/goto/run/push/pull ..."