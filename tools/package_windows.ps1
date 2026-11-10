[CmdletBinding()]
param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$BuildDir = "build-windows",
    [string]$Generator = "Visual Studio 17 2022"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildScript = Join-Path $PSScriptRoot "build_windows.ps1"

function Fail([string]$Message) {
    throw $Message
}

function Assert-UnderRoot([string]$Path) {
    $RootPrefix = ([IO.Path]::GetFullPath($Root)).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    $FullPath = [IO.Path]::GetFullPath($Path)
    if (-not $FullPath.StartsWith($RootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        Fail "La ruta de salida está fuera del repositorio: $FullPath"
    }
}

function Get-DumpbinPath {
    $Command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($null -ne $Command) {
        return $Command.Source
    }

    $VsWhereCandidates = @(
        (Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\Installer\vswhere.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe")
    )
    foreach ($VsWhere in $VsWhereCandidates) {
        if (-not (Test-Path -LiteralPath $VsWhere -PathType Leaf)) {
            continue
        }
        $InstallationPath = (& $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
        if ([string]::IsNullOrWhiteSpace($InstallationPath)) {
            continue
        }
        $Dumpbin = Get-ChildItem -LiteralPath (Join-Path $InstallationPath "VC\Tools\MSVC") -Filter "dumpbin.exe" -File -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($null -ne $Dumpbin) {
            return $Dumpbin.FullName
        }
    }

    $VsRoots = @(
        (Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2026"),
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2026"),
        (Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2022"),
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022")
    )
    foreach ($VsRoot in $VsRoots) {
        if (-not (Test-Path -LiteralPath $VsRoot -PathType Container)) {
            continue
        }
        $Dumpbin = Get-ChildItem -LiteralPath $VsRoot -Filter "dumpbin.exe" -File -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($null -ne $Dumpbin) {
            return $Dumpbin.FullName
        }
    }

    Fail "No se encontró dumpbin.exe. Instala las herramientas de C++ de Visual Studio 2026/2022 (Desktop development with C++)."
}

function Get-DllDependencies([string]$Binary, [string]$DumpbinPath) {
    $Output = & $DumpbinPath /nologo /dependents $Binary 2>&1
    if ($LASTEXITCODE -ne 0) {
        Fail "dumpbin no pudo inspeccionar $Binary"
    }

    $Dependencies = New-Object System.Collections.Generic.List[string]
    foreach ($Line in $Output) {
        $Name = ([string]$Line).Trim()
        if ($Name -match "^[A-Za-z0-9_.-]+\.dll$") {
            if (-not $Dependencies.Contains($Name.ToLowerInvariant())) {
                $Dependencies.Add($Name.ToLowerInvariant())
            }
        }
    }
    return $Dependencies
}

function Test-SystemPath([string]$Path) {
    $FullPath = [IO.Path]::GetFullPath($Path)
    $WindowsRoot = if ($env:WINDIR) { [IO.Path]::GetFullPath($env:WINDIR) } else { "C:\Windows" }
    $SystemPrefixes = @(
        (Join-Path $WindowsRoot "System32") + [IO.Path]::DirectorySeparatorChar,
        (Join-Path $WindowsRoot "SysWOW64") + [IO.Path]::DirectorySeparatorChar,
        (Join-Path $WindowsRoot "WinSxS") + [IO.Path]::DirectorySeparatorChar
    )
    foreach ($Prefix in $SystemPrefixes) {
        if ($FullPath.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }
    return $false
}

function Resolve-RuntimeDll([string]$Name, [string]$PackageDir, [string]$TripletBin, [string[]]$RuntimeRoots) {
    $Candidates = New-Object System.Collections.Generic.List[string]
    $Candidates.Add((Join-Path $PackageDir $Name))
    $Candidates.Add((Join-Path $TripletBin $Name))

    foreach ($RuntimeRoot in $RuntimeRoots) {
        if (-not [string]::IsNullOrWhiteSpace($RuntimeRoot)) {
            $Candidates.Add((Join-Path $RuntimeRoot $Name))
        }
    }

    foreach ($PathEntry in ($env:PATH -split [IO.Path]::PathSeparator)) {
        if (-not [string]::IsNullOrWhiteSpace($PathEntry)) {
            $Candidates.Add((Join-Path $PathEntry $Name))
        }
    }

    $FoundSystemDll = $false
    foreach ($Candidate in $Candidates) {
        if (-not (Test-Path -LiteralPath $Candidate -PathType Leaf)) {
            continue
        }
        $Resolved = (Resolve-Path -LiteralPath $Candidate).Path
        if (Test-SystemPath $Resolved) {
            $FoundSystemDll = $true
            continue
        }
        return $Resolved
    }

    if ($FoundSystemDll -and $Name -match "^(msvcp|vcruntime|concrt)[0-9_].*\.dll$") {
        return $false
    }
    if ($FoundSystemDll) {
        return $null
    }
    return $false
}

function Test-SystemDllName([string]$Name) {
    if ($Name -like "api-ms-win-*.dll" -or $Name -like "ext-ms-*.dll") {
        return $true
    }
    $SystemNames = @(
        "advapi32.dll", "bcrypt.dll", "combase.dll", "comdlg32.dll", "crypt32.dll", "d3d11.dll",
        "dwmapi.dll", "gdi32.dll", "imm32.dll", "iphlpapi.dll", "kernel32.dll", "ntdll.dll",
        "ole32.dll", "oleaut32.dll", "opengl32.dll", "psapi.dll", "rpcrt4.dll", "sechost.dll",
        "setupapi.dll", "shell32.dll", "shlwapi.dll", "user32.dll", "uxtheme.dll", "version.dll",
        "winmm.dll", "ws2_32.dll", "xinput1_4.dll", "ucrtbase.dll"
    )
    return $SystemNames -contains $Name.ToLowerInvariant()
}

if (-not (Test-Path -LiteralPath $BuildScript -PathType Leaf)) {
    Fail "No se encontró $BuildScript"
}

& $BuildScript -VcpkgRoot $VcpkgRoot -BuildDir $BuildDir -Generator $Generator
if ($LASTEXITCODE -ne 0) {
    Fail "La compilación de Windows falló."
}

if ([string]::IsNullOrWhiteSpace($VcpkgRoot) -or -not (Test-Path -LiteralPath $VcpkgRoot -PathType Container)) {
    Fail "VCPKG_ROOT no está definida o no existe."
}
$VcpkgRoot = (Resolve-Path -LiteralPath $VcpkgRoot).Path

$BuildPath = if ([IO.Path]::IsPathRooted($BuildDir)) {
    [IO.Path]::GetFullPath($BuildDir)
} else {
    [IO.Path]::GetFullPath((Join-Path $Root $BuildDir))
}
$Executable = Join-Path $BuildPath "Release\laboratorio.exe"
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    Fail "No se encontró el ejecutable compilado: $Executable"
}

$DistRoot = Join-Path $Root "dist"
$PackageDir = Join-Path $DistRoot "RenderLab-windows-x64"
$ArchivePath = Join-Path $DistRoot "RenderLab-windows-x64.zip"
Assert-UnderRoot $PackageDir
Assert-UnderRoot $ArchivePath

if (Test-Path -LiteralPath $PackageDir) {
    Remove-Item -LiteralPath $PackageDir -Recurse -Force
}
if (Test-Path -LiteralPath $ArchivePath) {
    Remove-Item -LiteralPath $ArchivePath -Force
}
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

Copy-Item -LiteralPath $Executable -Destination (Join-Path $PackageDir "laboratorio.exe")
Copy-Item -LiteralPath (Join-Path $Root "assets") -Destination (Join-Path $PackageDir "assets") -Recurse
Copy-Item -LiteralPath (Join-Path $Root "shaders") -Destination (Join-Path $PackageDir "shaders") -Recurse
New-Item -ItemType Directory -Path (Join-Path $PackageDir "licenses") -Force | Out-Null

$TripletBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"
if (-not (Test-Path -LiteralPath $TripletBin -PathType Container)) {
    Fail "No se encontró el directorio de DLL de vcpkg: $TripletBin"
}

$RuntimeRoots = New-Object System.Collections.Generic.List[string]
if ($env:VCToolsRedistDir) {
    $RuntimeRoots.Add($env:VCToolsRedistDir)
    if (Test-Path -LiteralPath $env:VCToolsRedistDir -PathType Container) {
        $RuntimeDirectories = Get-ChildItem -LiteralPath $env:VCToolsRedistDir -Directory -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\x64(\\[^\\]+)?$" }
        foreach ($Directory in $RuntimeDirectories) {
            $RuntimeRoots.Add($Directory.FullName)
        }
    }
}
foreach ($VsRoot in @(
    (Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2026"),
    (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2026"),
    (Join-Path ${env:ProgramFiles} "Microsoft Visual Studio\2022"),
    (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022")
)) {
    if (-not (Test-Path -LiteralPath $VsRoot -PathType Container)) {
        continue
    }
    $RuntimeDirectories = Get-ChildItem -LiteralPath $VsRoot -Directory -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\VC\\Redist\\MSVC\\[^\\]+\\x64(\\[^\\]+)?$" }
    foreach ($Directory in $RuntimeDirectories) {
        $RuntimeRoots.Add($Directory.FullName)
    }
}

$DumpbinPath = Get-DumpbinPath
$Queue = New-Object System.Collections.Generic.Queue[string]
$Visited = New-Object System.Collections.Generic.HashSet[string]
$Bundled = New-Object System.Collections.Generic.HashSet[string]
$Queue.Enqueue((Join-Path $PackageDir "laboratorio.exe"))

while ($Queue.Count -gt 0) {
    $Current = $Queue.Dequeue()
    $CurrentKey = ([IO.Path]::GetFullPath($Current)).ToLowerInvariant()
    if (-not $Visited.Add($CurrentKey)) {
        continue
    }

    foreach ($DependencyName in (Get-DllDependencies $Current $DumpbinPath)) {
        if (Test-SystemDllName $DependencyName) {
            continue
        }

        $Resolved = Resolve-RuntimeDll $DependencyName $PackageDir $TripletBin $RuntimeRoots.ToArray()
        if ($null -eq $Resolved) {
            continue
        }
        if ($false -eq $Resolved) {
            Fail "No se encontró la DLL no perteneciente al sistema '$DependencyName'. Se buscó en vcpkg x64-windows, el runtime de Visual Studio y PATH."
        }

        $Target = Join-Path $PackageDir ([IO.Path]::GetFileName($Resolved))
        if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) {
            Copy-Item -LiteralPath $Resolved -Destination $Target
        }
        if ($Bundled.Add(([IO.Path]::GetFileName($Target)).ToLowerInvariant())) {
            Write-Host "DLL incluida: $([IO.Path]::GetFileName($Target))"
        }
        $Queue.Enqueue($Target)
    }
}

Copy-Item -LiteralPath (Join-Path $Root "LICENSE") -Destination (Join-Path $PackageDir "licenses\proyecto.txt")
$ShareRoot = Join-Path $VcpkgRoot "installed\x64-windows\share"
if (Test-Path -LiteralPath $ShareRoot -PathType Container) {
    foreach ($PortDirectory in (Get-ChildItem -LiteralPath $ShareRoot -Directory | Sort-Object Name)) {
        $Copyright = Join-Path $PortDirectory.FullName "copyright"
        if (Test-Path -LiteralPath $Copyright -PathType Leaf) {
            $TargetName = "vcpkg-$($PortDirectory.Name)-copyright.txt"
            Copy-Item -LiteralPath $Copyright -Destination (Join-Path $PackageDir "licenses\$TargetName")
        }
    }
}

$Readme = @"
RenderLab — Laboratorio de materia, fuerzas y movimiento
=========================================================

Requisitos:
- Windows 10 u 11 de 64 bits.
- GPU y controlador compatibles con OpenGL 3.3.
- No se requiere instalar un compilador ni copiar DLL manualmente.

Ejecución:
1. Descomprima el archivo completo.
2. Abra esta carpeta y ejecute laboratorio.exe.
3. No separe el ejecutable de assets, shaders ni las DLL incluidas.

Controles:
W, A, S, D       Caminar
Ratón derecho   Mirar mientras se mantiene presionado
1 a 7            Viajar a una estación
E                Abrir o cerrar la ficha cercana
R                Reiniciar las simulaciones
P                Activar o detener la demostración seleccionada
L                Encender o apagar la luz del microscopio
O                Girar 120 grados los objetivos del microscopio
F                Subir o bajar el enfoque del microscopio
Esc              Cerrar la ficha o salir
"@
Set-Content -LiteralPath (Join-Path $PackageDir "LEEME.txt") -Value $Readme -Encoding UTF8

if (-not (Test-Path -LiteralPath (Join-Path $PackageDir "assets\models\parts.txt") -PathType Leaf)) {
    Fail "El paquete no contiene assets/models/parts.txt"
}
foreach ($RequiredDirectory in @("assets\ui", "assets\textures", "shaders")) {
    if (-not (Test-Path -LiteralPath (Join-Path $PackageDir $RequiredDirectory) -PathType Container)) {
        Fail "El paquete no contiene $RequiredDirectory"
    }
}
if ((Get-ChildItem -LiteralPath $PackageDir -Filter "*.dll" -File).Count -eq 0) {
    Fail "El paquete no contiene DLL de terceros."
}

Compress-Archive -LiteralPath $PackageDir -DestinationPath $ArchivePath -CompressionLevel Optimal

Add-Type -AssemblyName System.IO.Compression.FileSystem
$Zip = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
try {
    $EntryNames = @($Zip.Entries | ForEach-Object { $_.FullName.Replace("\\", "/") })
} finally {
    $Zip.Dispose()
}

$RequiredEntries = @(
    "RenderLab-windows-x64/laboratorio.exe",
    "RenderLab-windows-x64/assets/models/parts.txt"
)
foreach ($Entry in $RequiredEntries) {
    if ($EntryNames -notcontains $Entry) {
        Fail "El ZIP no contiene $Entry"
    }
}
foreach ($DirectoryEntry in @("assets/ui/", "assets/textures/", "shaders/")) {
    if (-not ($EntryNames | Where-Object { $_ -like "RenderLab-windows-x64/$DirectoryEntry*" })) {
        Fail "El ZIP no contiene $DirectoryEntry"
    }
}
if (-not ($EntryNames | Where-Object { $_ -like "RenderLab-windows-x64/*.dll" })) {
    Fail "El ZIP no contiene DLL de terceros."
}

Write-Host "Carpeta: $PackageDir"
Write-Host "Archivo: $ArchivePath"
