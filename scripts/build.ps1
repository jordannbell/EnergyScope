# Builds EnergyScope with Visual Studio's tools and runs the tests.
# Usage (PowerShell, from anywhere):  .\scripts\build.ps1 [-Config Release]
param([string]$Config = "Debug")

. "$PSScriptRoot\common.ps1"
$root = Split-Path $PSScriptRoot -Parent

Invoke-InVsEnvironment @(
    "{cmake} -S `"$root`" -B `"$root\build`" -G Ninja -DCMAKE_MAKE_PROGRAM={ninja} -DCMAKE_BUILD_TYPE=$Config",
    "{cmake} --build `"$root\build`"",
    "ctest --test-dir `"$root\build`" --output-on-failure"
)
