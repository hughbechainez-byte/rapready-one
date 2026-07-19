$ErrorActionPreference = 'Stop'
$packageRoot = $PSScriptRoot
$source = Get-ChildItem -LiteralPath $packageRoot -Directory -Filter '*.vst3' | Select-Object -First 1
if ($null -eq $source) {
    $packageRoot = Split-Path -Parent $PSScriptRoot
    $source = Get-ChildItem -LiteralPath $packageRoot -Directory -Filter '*.vst3' | Select-Object -First 1
}
if ($null -eq $source) {
    throw 'No .vst3 bundle was found beside the tools folder.'
}

$destinationRoot = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
$destination = Join-Path $destinationRoot $source.Name
New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null
if (Test-Path -LiteralPath $destination) {
    Remove-Item -LiteralPath $destination -Recurse -Force
}
Copy-Item -LiteralPath $source.FullName -Destination $destination -Recurse
Write-Host "Installed $($source.Name) to $destination"
