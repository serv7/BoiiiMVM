param(
    [Parameter(Mandatory = $true)]
    [string]$GameDirectory,
    [switch]$Disable
)
$ErrorActionPreference = 'Stop'
$gameRoot = (Resolve-Path -LiteralPath $GameDirectory).Path.TrimEnd('\')
if (!(Test-Path -LiteralPath "$gameRoot\BlackOps3.exe")) { throw 'This is not a BO3 game directory.' }
$pluginDirectory = Join-Path $gameRoot 'boiii\plugins'
$destination = Join-Path $pluginDirectory 'bo3_theater.dll'
if ($Disable) {
    if (Test-Path -LiteralPath $destination) {
        $backupDirectory = Join-Path $PSScriptRoot 'backups'
        New-Item -ItemType Directory -Force -Path $backupDirectory | Out-Null
        $disabled = Join-Path $backupDirectory ('bo3_theater-disabled-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.dll')
        Move-Item -LiteralPath $destination -Destination $disabled
        Write-Host "Disabled. Saved DLL at $disabled. Restart BOIII to unload it."
    }
    return
}
$source = Join-Path $PSScriptRoot 'build\bo3_theater.dll'
if (!(Test-Path -LiteralPath $source)) { throw 'Build the plugin first with build.ps1.' }
New-Item -ItemType Directory -Force -Path $pluginDirectory | Out-Null
if (Test-Path -LiteralPath $destination) {
    $backupDirectory = Join-Path $PSScriptRoot 'backups'
    New-Item -ItemType Directory -Force -Path $backupDirectory | Out-Null
    Copy-Item -LiteralPath $destination -Destination (Join-Path $backupDirectory ('bo3_theater-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.dll'))
}
Copy-Item -LiteralPath $source -Destination $destination
$presetDirectory=Join-Path $gameRoot 'MVM\Theater\fog-presets'
New-Item -ItemType Directory -Force -Path $presetDirectory | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'assets\fog-presets') -Filter '*.zfog' -File | ForEach-Object {
    $presetDestination=Join-Path $presetDirectory $_.Name
    if (!(Test-Path -LiteralPath $presetDestination)) { Copy-Item -LiteralPath $_.FullName -Destination $presetDestination }
}
$encoderSource=Join-Path $PSScriptRoot 'assets\ffmpeg'
if (Test-Path -LiteralPath $encoderSource) {
    $encoderDirectory=Join-Path $gameRoot 'MVM\Theater'
    New-Item -ItemType Directory -Force -Path $encoderDirectory | Out-Null
    Get-ChildItem -LiteralPath $encoderSource -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $encoderDirectory $_.Name) -Force
    }
} else { Write-Warning 'FFmpeg binaries were not found in assets/ffmpeg. Install a Windows FFmpeg build in MVM/Theater to enable video recording.' }
Write-Host "Installed $destination. Restart BOIII to load it."
