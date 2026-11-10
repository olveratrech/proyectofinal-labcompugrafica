[CmdletBinding()]
param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$BuildDir = "build-windows"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Fail([string]$Message) {
    throw $Message
}

if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
    Fail "VCPKG_ROOT no está definida. Configúrala con la ruta de tu instalación de vcpkg."
}

if (-not (Test-Path -LiteralPath $VcpkgRoot -PathType Container)) {
    Fail "No existe VCPKG_ROOT: $VcpkgRoot"
}

$VcpkgRoot = (Resolve-Path -LiteralPath $VcpkgRoot).Path
$ToolchainFile = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$TripletRoot = Join-Path $VcpkgRoot "installed\x64-windows"

if (-not (Test-Path -LiteralPath $ToolchainFile -PathType Leaf)) {
    Fail "No se encontró el toolchain de vcpkg en $ToolchainFile"
}

if (-not (Test-Path -LiteralPath $TripletRoot -PathType Container)) {
    Fail "No se encontró el triplete x64-windows en $TripletRoot. Instala glew, glfw3, glm y assimp para x64-windows."
}

$CMakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($null -eq $CMakeCommand) {
    Fail "CMake no está disponible en PATH. Instala CMake 3.20 o posterior."
}

$VersionLine = (& $CMakeCommand.Source --version | Select-Object -First 1)
$VersionMatch = [regex]::Match($VersionLine, "\d+(\.\d+){1,2}")
if (-not $VersionMatch.Success -or ([version]$VersionMatch.Value -lt [version]"3.20")) {
    Fail "Se requiere CMake 3.20 o posterior. Versión detectada: $VersionLine"
}

$BuildPath = if ([IO.Path]::IsPathRooted($BuildDir)) {
    [IO.Path]::GetFullPath($BuildDir)
} else {
    [IO.Path]::GetFullPath((Join-Path $Root $BuildDir))
}

Write-Host "Configurando RenderLab para Windows x64..."
& $CMakeCommand.Source `
    "-S" $Root `
    "-B" $BuildPath `
    "-G" "Visual Studio 17 2022" `
    "-A" "x64" `
    "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile" `
    "-DVCPKG_TARGET_TRIPLET=x64-windows"
if ($LASTEXITCODE -ne 0) {
    Fail "La configuración de CMake falló. Verifica Visual Studio 2022 Build Tools y las dependencias de vcpkg."
}

Write-Host "Compilando Release x64..."
& $CMakeCommand.Source "--build" $BuildPath "--config" "Release" "--parallel"
if ($LASTEXITCODE -ne 0) {
    Fail "La compilación Release x64 falló."
}

$Executable = Join-Path $BuildPath "Release\laboratorio.exe"
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    Fail "No se encontró el ejecutable esperado: $Executable"
}

Write-Host "Ejecutable: $Executable"
