param(
    [string]$BlenderVersion = "4.4",
    [string]$DazLibrary = "$env:USERPROFILE\Documents\DAZ 3D\Studio\My Library",
    [string]$Tag = "version_5_1_0",
    [switch]$DryRun,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$cacheRoot = Join-Path $repoRoot "Tools\.cache\diffeomorphic"
$zipPath = Join-Path $cacheRoot "$Tag.zip"
$extractRoot = Join-Path $cacheRoot $Tag
$url = "https://github.com/Diffeomorphic/import_daz/archive/refs/tags/$Tag.zip"

$blenderAddonRoot = Join-Path $env:APPDATA "Blender Foundation\Blender\$BlenderVersion\scripts\addons"
$addonTarget = Join-Path $blenderAddonRoot "import_daz"
$dazScriptTarget = Join-Path $DazLibrary "Scripts\Diffeomorphic"

Write-Host "Diffeomorphic tag: $Tag"
Write-Host "Download URL: $url"
Write-Host "Blender add-on target: $addonTarget"
Write-Host "Daz script target: $dazScriptTarget"
Write-Host "Cache root: $cacheRoot"

if ($DryRun) {
    Write-Host "Dry run only. No files were downloaded or installed."
    exit 0
}

New-Item -ItemType Directory -Force -Path $cacheRoot | Out-Null

if ($Force -and (Test-Path $zipPath)) {
    Remove-Item -LiteralPath $zipPath -Force
}

if (!(Test-Path $zipPath)) {
    Write-Host "Downloading Diffeomorphic..."
    Invoke-WebRequest -Uri $url -OutFile $zipPath
}

if (Test-Path $extractRoot) {
    Remove-Item -LiteralPath $extractRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $extractRoot | Out-Null

Write-Host "Extracting archive..."
Expand-Archive -LiteralPath $zipPath -DestinationPath $extractRoot -Force

$sourceRoot = Get-ChildItem -LiteralPath $extractRoot -Directory | Select-Object -First 1
if ($null -eq $sourceRoot -or !(Test-Path (Join-Path $sourceRoot.FullName "__init__.py"))) {
    throw "Could not locate Diffeomorphic add-on root after extracting $zipPath"
}

New-Item -ItemType Directory -Force -Path $blenderAddonRoot | Out-Null
if (Test-Path $addonTarget) {
    Remove-Item -LiteralPath $addonTarget -Recurse -Force
}
Copy-Item -LiteralPath $sourceRoot.FullName -Destination $addonTarget -Recurse -Force

$dazScriptSource = Join-Path $sourceRoot.FullName "to_daz_studio\Scripts\Diffeomorphic"
if (!(Test-Path $dazScriptSource)) {
    throw "Could not locate Diffeomorphic Daz Studio scripts in $dazScriptSource"
}

New-Item -ItemType Directory -Force -Path $dazScriptTarget | Out-Null
Copy-Item -Path (Join-Path $dazScriptSource "*") -Destination $dazScriptTarget -Recurse -Force

Write-Host "Installed Blender add-on to: $addonTarget"
Write-Host "Installed Daz scripts to: $dazScriptTarget"
Write-Host "The bridge bake script enables the add-on during each background Blender run."
