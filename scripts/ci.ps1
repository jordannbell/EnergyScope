# One step of the continuous-integration pipeline (called by the Jenkinsfile).
# Usage:  .\scripts\ci.ps1 -Stage Configure|Build|Test [-Config Release] [-BuildDir build]
#
# Each stage is a separate call so the CI server shows which one failed.
# The Test stage writes a JUnit XML report that Jenkins can display.
param(
    [Parameter(Mandatory)][ValidateSet("Configure", "Build", "Test")][string]$Stage,
    [string]$Config = "Release",
    [string]$BuildDir = "build"
)

. "$PSScriptRoot\common.ps1"
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root $BuildDir

switch ($Stage) {
    "Configure" {
        Invoke-InVsEnvironment @(
            "{cmake} -S `"$root`" -B `"$build`" -G Ninja -DCMAKE_MAKE_PROGRAM={ninja} -DCMAKE_BUILD_TYPE=$Config"
        )
    }
    "Build" {
        Invoke-InVsEnvironment @("{cmake} --build `"$build`"")
    }
    "Test" {
        # --output-junit takes a path relative to the build folder.
        Invoke-InVsEnvironment @(
            "ctest --test-dir `"$build`" --output-on-failure --output-junit test-results.xml"
        )
    }
}
