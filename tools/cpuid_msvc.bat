@REM run from the cmd prompt. won't run in pwsh.
@echo off
if defined VSCMD_VER (
    echo Visual Studio Command Prompt is available.
) else (
    set vc_vars_path=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build
    pushd %vc_vars_path%
    call .\vcvarsall.bat x64
    popd
)

cl /O2 /MD /W4 /EHsc .\cpuid_msvc.cpp
rm .\cpuid_msvc.obj