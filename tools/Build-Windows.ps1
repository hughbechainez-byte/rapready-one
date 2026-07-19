param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $cmakePath = 'C:\Program Files\CMake\bin\cmake.exe'
    if (Test-Path -LiteralPath $cmakePath) {
        Set-Alias cmake $cmakePath
    } else {
        throw 'CMake 3.24 or newer is required.'
    }
}

$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere) -or
    -not (& $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)) {
    throw 'Install Visual Studio 2022 Desktop development with C++ (MSVC v143 and Windows 11 SDK), then rerun.'
}

cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $build --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir $build -C $Configuration --output-on-failure
exit $LASTEXITCODE
