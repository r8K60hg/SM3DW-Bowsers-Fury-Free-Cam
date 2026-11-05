# Builds BF Freecam on Windows using Docker Desktop (needs Git for Windows + Docker Desktop).
# Run from PowerShell:  powershell -ExecutionPolicy Bypass -File .\build.ps1
$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot
$ExlaunchRepo = "https://github.com/shadowninja108/exlaunch.git"
$ExlaunchCommit = "f698816d6e198afb0029ad5c07d55e7017a620fe"
$PinnedImage = "devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282"
$FallbackImage = "devkitpro/devkita64:latest"
$TitleId = "010028600EBDA000"
$BuildRoot = Join-Path $Root ".build"
$Work = Join-Path $BuildRoot "exlaunch"
$Dist = Join-Path $Root "dist"

function Invoke-Native([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

Write-Host "==> Fetching ExLaunch @ $($ExlaunchCommit.Substring(0,7))"
if (Test-Path $BuildRoot) { Remove-Item $BuildRoot -Recurse -Force }
New-Item -ItemType Directory -Path $BuildRoot | Out-Null
Invoke-Native "git" @("-c", "core.autocrlf=false", "clone", "--quiet", $ExlaunchRepo, $Work)
Invoke-Native "git" @("-C", $Work, "checkout", "--quiet", $ExlaunchCommit)

Write-Host "==> Applying freecam sources"
Copy-Item -Path (Join-Path $Root "src\*") -Destination $Work -Recurse -Force

Write-Host "==> Building in Docker"
$Image = $PinnedImage
& docker pull --quiet $Image *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    pinned image unavailable, using $FallbackImage"
    $Image = $FallbackImage
}
Invoke-Native "docker" @("run", "--rm", "-v", "${Root}:/work", "-w", "/work/.build/exlaunch", $Image, "bash", "-lc", 'make -j$(nproc)')

foreach ($f in @("subsdk9", "main.npdm")) {
    if (-not (Test-Path (Join-Path $Work "deploy\$f"))) { throw "Build output missing: deploy\$f" }
}

Write-Host "==> Packaging"
if (Test-Path $Dist) { Remove-Item $Dist -Recurse -Force }
$Emu = Join-Path $Dist "emulator\BFFreecam\exefs"
$Atmo = Join-Path $Dist "atmosphere\atmosphere\contents\$TitleId\exefs"
New-Item -ItemType Directory -Path $Emu, $Atmo -Force | Out-Null
foreach ($f in @("subsdk9", "main.npdm")) {
    Copy-Item (Join-Path $Work "deploy\$f") (Join-Path $Emu $f)
    Copy-Item (Join-Path $Work "deploy\$f") (Join-Path $Atmo $f)
}
Compress-Archive -Path (Join-Path $Dist "emulator\BFFreecam") -DestinationPath (Join-Path $Dist "BFFreecam-emulator.zip") -Force
Compress-Archive -Path (Join-Path $Dist "atmosphere\atmosphere") -DestinationPath (Join-Path $Dist "BFFreecam-atmosphere.zip") -Force

Write-Host "==> Done. Files are in: $Dist"
