param(
    [ValidateSet('reconfigure', 'build', 'flash', 'monitor', 'erase', 'menuconfig')]
    [string]$Action = 'build',
    [string]$BuildDir = 'build.c6_companion',
    [string]$Port = ''
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    return Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

function Get-WorkspaceSettings([string]$RepoRoot) {
    $settingsPath = Join-Path $RepoRoot '.vscode\settings.json'
    if (-not (Test-Path $settingsPath)) {
        return $null
    }
    try {
        return Get-Content $settingsPath -Raw | ConvertFrom-Json
    }
    catch {
        Write-Warning "Failed to parse ${settingsPath}: $_"
        return $null
    }
}

function Resolve-IdfPath($Settings) {
    $candidates = @()
    if ($env:IDF_PATH) {
        $candidates += $env:IDF_PATH
    }
    if ($Settings -and $Settings.PSObject.Properties.Name -contains 'idf.currentSetup' -and $Settings.'idf.currentSetup') {
        $candidates += [string]$Settings.'idf.currentSetup'
    }
    $frameworkRoot = 'C:\ProgramData\Espressif\frameworks'
    if (Test-Path $frameworkRoot) {
        $candidates += (Get-ChildItem $frameworkRoot -Directory -Filter 'esp-idf-v*' | Sort-Object Name -Descending | ForEach-Object { $_.FullName })
    }
    foreach ($candidate in $candidates) {
        if (-not $candidate) {
            continue
        }
        $normalized = $candidate.TrimEnd('\', '/')
        if (Test-Path (Join-Path $normalized 'tools\idf.py')) {
            return $normalized
        }
    }
    throw 'Unable to resolve ESP-IDF path. Set IDF_PATH or update .vscode/settings.json.'
}

function Resolve-PythonExe($Settings) {
    $candidates = @()
    if ($env:IDF_PYTHON_ENV_PATH) {
        $candidates += (Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts\python.exe')
    }
    if ($Settings -and $Settings.PSObject.Properties.Name -contains 'idf.pythonBinPathWin' -and $Settings.'idf.pythonBinPathWin') {
        $candidates += [string]$Settings.'idf.pythonBinPathWin'
    }
    $pythonEnvRoot = 'C:\ProgramData\Espressif\python_env'
    if (Test-Path $pythonEnvRoot) {
        $candidates += (Get-ChildItem $pythonEnvRoot -Directory -Filter 'idf*_py*_env' | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'Scripts\python.exe' })
    }
    $candidates += 'python.exe'
    foreach ($candidate in $candidates) {
        try {
            $command = Get-Command $candidate -ErrorAction Stop
            return $command.Source
        }
        catch {
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }
    throw 'Unable to resolve Python executable for ESP-IDF.'
}

function Resolve-Port($Settings, [string]$RequestedPort) {
    if ($RequestedPort) {
        return $RequestedPort
    }
    if ($env:ESPPORT) {
        return $env:ESPPORT
    }
    if ($Settings) {
        if ($Settings.PSObject.Properties.Name -contains 'idf.portWin' -and $Settings.'idf.portWin') {
            return [string]$Settings.'idf.portWin'
        }
        if ($Settings.PSObject.Properties.Name -contains 'idf.port' -and $Settings.'idf.port') {
            return [string]$Settings.'idf.port'
        }
    }
    return 'COM6'
}

function Resolve-NinjaExe() {
    $candidates = @()
    $toolsRoot = 'C:\ProgramData\Espressif\tools\ninja'
    if (Test-Path $toolsRoot) {
        $candidates += (Get-ChildItem $toolsRoot -Directory | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'ninja.exe' })
    }
    $candidates += 'ninja.exe'
    foreach ($candidate in $candidates) {
        try {
            $command = Get-Command $candidate -ErrorAction Stop
            return $command.Source
        }
        catch {
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }
    return $null
}

function Resolve-RiscvToolchainBin() {
    $toolsRoot = 'C:\ProgramData\Espressif\tools\riscv32-esp-elf'
    if (-not (Test-Path $toolsRoot)) {
        return $null
    }
    $candidates = Get-ChildItem $toolsRoot -Directory |
        Sort-Object LastWriteTime -Descending |
        ForEach-Object { Join-Path $_.FullName 'riscv32-esp-elf\bin' }
    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate 'riscv32-esp-elf-gcc.exe')) {
            return $candidate
        }
    }
    return $null
}

$repoRoot = Get-RepoRoot
$settings = Get-WorkspaceSettings $repoRoot
$idfPath = Resolve-IdfPath $settings
$pythonExe = Resolve-PythonExe $settings
$ninjaExe = Resolve-NinjaExe
$riscvToolchainBin = Resolve-RiscvToolchainBin
$portValue = Resolve-Port $settings $Port
$idfPy = Join-Path $idfPath 'tools\idf.py'
$projectDir = Join-Path $repoRoot 'firmware\c6_companion'

$env:IDF_PATH = $idfPath
$env:IDF_PYTHON_ENV_PATH = Split-Path -Parent (Split-Path -Parent $pythonExe)
if ($riscvToolchainBin) {
    $env:PATH = "$riscvToolchainBin;$env:PATH"
}
if ($ninjaExe) {
    $env:PATH = "$(Split-Path -Parent $ninjaExe);$env:PATH"
}

$baseArgs = @($idfPy, '-B', $BuildDir, '-C', $projectDir)
if ($Action -in @('flash', 'monitor', 'erase') -and $portValue) {
    $baseArgs += @('-p', $portValue)
}

$actionArgs = switch ($Action) {
    'reconfigure' { ,@('set-target', 'esp32c6', 'reconfigure') }
    'build' { ,@('build') }
    'flash' { ,@('flash') }
    'monitor' { ,@('monitor') }
    'erase' { ,@('erase-flash') }
    'menuconfig' { ,@('menuconfig') }
}

Write-Host "[trail-mate-c6] RepoRoot : $repoRoot"
Write-Host "[trail-mate-c6] Project  : $projectDir"
Write-Host "[trail-mate-c6] ESP-IDF  : $idfPath"
Write-Host "[trail-mate-c6] Python   : $pythonExe"
Write-Host "[trail-mate-c6] BuildDir : $BuildDir"
if ($portValue) {
    Write-Host "[trail-mate-c6] Port     : $portValue"
}

Push-Location $repoRoot
try {
    & $pythonExe @baseArgs @actionArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}
