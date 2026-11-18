# Shared helpers for the build scripts. Dot-source it:  . "$PSScriptRoot\common.ps1"

$ErrorActionPreference = "Stop"

# Runs shell commands inside the Visual Studio C++ environment (compiler,
# include paths...). `vcvars64.bat` prepares that environment; Ninja and
# CMake are the ones shipped with Visual Studio.
function Invoke-InVsEnvironment {
    param([Parameter(Mandatory)][string[]]$Commands)

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found: is Visual Studio installed?" }
    $vs = & $vswhere -latest -products * -property installationPath
    if (-not $vs) { throw "Visual Studio not found. Install it with the C++ and MFC components." }

    $vcvars = "$vs\VC\Auxiliary\Build\vcvars64.bat"
    $cmakeDir = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake"

    # Expand the {cmake} and {ninja} placeholders, then chain with && so the
    # first failing command stops the rest.
    $expanded = $Commands | ForEach-Object {
        $_.Replace("{cmake}", "`"$cmakeDir\CMake\bin\cmake.exe`"").
           Replace("{ninja}", "`"$cmakeDir\Ninja\ninja.exe`"")
    }
    cmd /c "`"$vcvars`" >nul && $($expanded -join ' && ')"
    if ($LASTEXITCODE -ne 0) { throw "Command failed (exit code $LASTEXITCODE)." }
}
