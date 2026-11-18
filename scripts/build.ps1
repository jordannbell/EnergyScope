# Builds EnergyScope with Visual Studio's tools and runs the tests.
# Usage (PowerShell, from anywhere):  .\scripts\build.ps1 [-Config Release]
param([string]$Config = "Debug")

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -property installationPath
if (-not $vs) { throw "Visual Studio not found. Install it with the C++ and MFC components." }

$vcvars = "$vs\VC\Auxiliary\Build\vcvars64.bat"
$cmakeDir = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake"
$cmake = "$cmakeDir\CMake\bin\cmake.exe"
$ninja = "$cmakeDir\Ninja\ninja.exe"

# vcvars64.bat prepares the compiler environment (cl.exe, include paths...).
$steps = @(
    "`"$cmake`" -S `"$root`" -B `"$root\build`" -G Ninja -DCMAKE_MAKE_PROGRAM=`"$ninja`" -DCMAKE_BUILD_TYPE=$Config",
    "`"$cmake`" --build `"$root\build`"",
    "ctest --test-dir `"$root\build`" --output-on-failure"
) -join " && "

cmd /c "`"$vcvars`" >nul && $steps"
if ($LASTEXITCODE -ne 0) { throw "Build or tests failed (exit code $LASTEXITCODE)." }
