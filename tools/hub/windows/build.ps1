[CmdletBinding()]
param(
    [string]$Msys = 'C:\msys64',
    [switch]$InstallToolchain,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$EnetCommit = '5a9c537fd464b3c6d3c55e1d3bd47588faf71b42'
$EnetUrl = "https://github.com/lsalzman/enet/archive/$EnetCommit.zip"

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$Work = Join-Path $PSScriptRoot 'build'
$Output = Join-Path $PSScriptRoot 'NightCoopHub.exe'
$msysWasThere = Test-Path (Join-Path $Msys 'usr\bin\bash.exe')
$installedMsys = $false

function Find-Toolchain {
    $dirs = @()
    $found = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($found) { $dirs += (Split-Path $found.Source) }
    $dirs += (Join-Path $Msys 'ucrt64\bin')
    foreach ($dir in $dirs) {
        $gxx = Join-Path $dir 'g++.exe'
        $gcc = Join-Path $dir 'gcc.exe'
        if (-not ((Test-Path $gxx) -and (Test-Path $gcc))) { continue }
        $env:PATH = "$dir;$env:PATH"
        try {
            $machine = (& $gxx -dumpmachine | Out-String).Trim()
            $major = [int](((& $gxx -dumpversion | Out-String).Trim()) -split '\.')[0]
        } catch { continue }
        if ($machine -notmatch 'mingw') { continue }
        if ($major -lt 13) {
            Write-Host "Skipping $gxx : GCC $major is too old, the hub needs GCC 13+."
            continue
        }
        if ($dir -notmatch 'ucrt') {
            Write-Warning "$dir is not an UCRT64 toolchain."
        }
        return @{ Dir = $dir; Gcc = $gcc; Gxx = $gxx; Version = $major }
    }
    return $null
}

function Install-Toolchain {
    $installer = Join-Path $env:TEMP 'msys2-x86_64-latest.exe'
    Write-Host 'Downloading the MSYS2 installer ...'
    Invoke-WebRequest 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe' -OutFile $installer
    Write-Host "Installing MSYS2 into $Msys ..."
    $root = $Msys -replace '\\', '/'
    $proc = Start-Process -FilePath $installer -ArgumentList @('in', '--confirm-command', '--accept-messages', '--root', $root) -Wait -PassThru
    if ($proc.ExitCode -ne 0) { throw "The MSYS2 installer failed (exit code $($proc.ExitCode))." }
    $bash = Join-Path $Msys 'usr\bin\bash.exe'
    if (-not (Test-Path $bash)) { throw "MSYS2 was not found in $Msys after the installation." }
    Write-Host 'Updating MSYS2 and installing GCC ...'
    & $bash -lc 'pacman -Syu --noconfirm'
    & $bash -lc 'pacman -Syu --noconfirm'
    & $bash -lc 'pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc'
    if ($LASTEXITCODE -ne 0) { throw 'pacman could not install mingw-w64-ucrt-x86_64-gcc.' }
}

function Invoke-Tool([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$(Split-Path $Exe -Leaf) failed (exit code $LASTEXITCODE)." }
}

function Get-Sources([string]$Url, [string]$Pattern, [string]$Check) {
    $zip = Join-Path $Work 'download.zip'
    Invoke-WebRequest $Url -OutFile $zip
    Expand-Archive -Path $zip -DestinationPath $Work -Force
    Remove-Item $zip
    $dir = Get-ChildItem $Work -Directory -Filter $Pattern | Select-Object -First 1
    if (-not $dir -or -not (Test-Path (Join-Path $dir.FullName $Check))) { throw "Unexpected contents of $Url" }
    return $dir.FullName
}

$tc = Find-Toolchain
if (-not $tc -and $InstallToolchain) {
    Install-Toolchain
    $installedMsys = -not $msysWasThere
    $tc = Find-Toolchain
}
if (-not $tc) {
    Write-Host 'No MinGW-w64 g++ 13+ found. Run this script with -InstallToolchain.' -ForegroundColor Yellow
    exit 1
}
Write-Host "Toolchain: $($tc.Gxx) (GCC $($tc.Version))"

New-Item -ItemType Directory -Force $Work | Out-Null

$enetDir = Join-Path $Work "enet-$EnetCommit"
if (-not (Test-Path (Join-Path $enetDir 'win32.c'))) {
    Write-Host "Downloading ENet $EnetCommit ..."
    $enetDir = Get-Sources $EnetUrl "enet-$EnetCommit" 'win32.c'
}

$obj = Join-Path $Work 'obj'
if (Test-Path $obj) { Remove-Item $obj -Recurse -Force }
New-Item -ItemType Directory -Force $obj | Out-Null

Write-Host 'Compiling ENet ...'
foreach ($f in 'callbacks', 'compress', 'host', 'list', 'packet', 'peer', 'protocol', 'win32') {
    Invoke-Tool $tc.Gcc @('-c', '-O2', "-I$enetDir\include", "$enetDir\$f.c", '-o', "$obj\enet_$f.o")
}

Write-Host 'Compiling Monocypher ...'
Invoke-Tool $tc.Gcc @('-c', '-O2', "$Root\vendor\monocypher\monocypher.c", '-o', "$obj\monocypher.o")

Write-Host 'Compiling the hub ...'
foreach ($src in 'src\Log.cpp', 'src\NetCrypto.cpp', 'src\HubServer.cpp', 'tools\hub\main.cpp') {
    $name = [IO.Path]::GetFileNameWithoutExtension($src)
    Invoke-Tool $tc.Gxx @('-c', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wno-unused-parameter',
        "-I$enetDir\include", "$Root\$src", '-o', "$obj\$name.o")
}

$hubExe = Join-Path $Work 'nightcoop_hub.exe'
Write-Host 'Linking ...'
$objects = @(Get-ChildItem $obj -Filter *.o | ForEach-Object { $_.FullName })
Invoke-Tool $tc.Gxx (@('-o', $hubExe) + $objects + @('-pthread', '-static', '-lws2_32', '-lwinmm', '-lbcrypt'))

$key = Join-Path $Work 'smoke.key'
if (Test-Path $key) { Remove-Item $key -Force }
Invoke-Tool $hubExe @('--gen-sign-key', $key) | Out-Null
$pub = & $hubExe --hub-public $key
Remove-Item $key -Force
if (-not ($pub | Select-String -Pattern '^nightcoop-hub-public 1 ' -Quiet)) {
    throw 'Smoke test failed: --hub-public did not print the expected line.'
}

Write-Host 'Building NightCoopHub.exe ...'
Copy-Item $hubExe (Join-Path $Work 'hub_payload.bin') -Force
Push-Location $Work
try {
    Invoke-Tool $tc.Gxx @('-std=c++20', '-O2', '-Wall', '-Wextra', "$PSScriptRoot\panel\NightCoopHub.cpp",
        '-o', $Output, '-static', '-lshell32', '-ladvapi32', '-lwinhttp')
} finally {
    Pop-Location
}

if ($Clean) {
    Write-Host 'Removing the files used for the build ...'
    Remove-Item $Work -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $env:TEMP 'msys2-x86_64-latest.exe') -Force -ErrorAction SilentlyContinue
    if ($installedMsys) {
        Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path -like "$Msys\*" } | Stop-Process -Force -ErrorAction SilentlyContinue
        Remove-Item $Msys -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $env:ProgramData 'Microsoft\Windows\Start Menu\Programs\MSYS2') -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ''
Write-Host "Done: $Output" -ForegroundColor Green
