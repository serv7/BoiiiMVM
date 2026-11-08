param(
    [Parameter(Mandatory = $true)]
    [string]$GameDirectory,
    [switch]$Disable,
    [switch]$ClientUiOnly
)
$ErrorActionPreference = 'Stop'
if ($Disable -and $ClientUiOnly) { throw 'Choose either Disable or ClientUiOnly.' }
$gameRoot = (Resolve-Path -LiteralPath $GameDirectory).Path.TrimEnd('\')
if (!(Test-Path -LiteralPath "$gameRoot\BlackOps3.exe")) { throw 'This is not a BO3 game directory.' }
$pluginDirectory = Join-Path $gameRoot 'boiii\plugins'
$backupDirectory = Join-Path $PSScriptRoot 'backups'
New-Item -ItemType Directory -Force -Path $backupDirectory | Out-Null
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
$clientUiSource = Join-Path $PSScriptRoot 'assets\client-ui'
$clientUiDirectory = Join-Path $gameRoot 'boiii\ui_scripts'
Get-ChildItem -LiteralPath $clientUiSource -Recurse -File | ForEach-Object {
    $relative = $_.FullName.Substring($clientUiSource.Length).TrimStart('\')
    $uiDestination = Join-Path $clientUiDirectory $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $uiDestination -Parent) | Out-Null
    if (Test-Path -LiteralPath $uiDestination) {
        $uiBackupName = (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-ui-' + $relative.Replace('\','-')
        Copy-Item -LiteralPath $uiDestination -Destination (Join-Path $backupDirectory $uiBackupName)
    }
    Copy-Item -LiteralPath $_.FullName -Destination $uiDestination
}
if ($ClientUiOnly) {
    Write-Host "Installed client UI preferences in $clientUiDirectory. Restart BOIII to load them."
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
$scriptsDirectory=Join-Path $gameRoot 'boiii\custom_scripts'
New-Item -ItemType Directory -Force -Path $scriptsDirectory | Out-Null
foreach ($name in @('mvm_bot_tools.gsc','mvm_corpse.csc')) {
    $scriptDestination=Join-Path $scriptsDirectory $(if($name -eq 'mvm_corpse.csc'){$name}else{'mp\'+$name})
    New-Item -ItemType Directory -Force -Path (Split-Path $scriptDestination -Parent) | Out-Null
    if (Test-Path -LiteralPath $scriptDestination) {
        Copy-Item -LiteralPath $scriptDestination -Destination (Join-Path $backupDirectory ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$name))
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('botmod\'+$name)) -Destination $scriptDestination
}
$oldCorpseScript=Join-Path $scriptsDirectory 'mp\mvm_corpse.csc'
if(Test-Path -LiteralPath $oldCorpseScript) {
    Move-Item -LiteralPath $oldCorpseScript -Destination (Join-Path $backupDirectory ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-mp-mvm_corpse.csc'))
}
$previewSource=Join-Path $PSScriptRoot 'assets\death-previews'
if (Test-Path -LiteralPath $previewSource) {
    $previewDestination=Join-Path $gameRoot 'MVM\Theater\death-previews'
    New-Item -ItemType Directory -Force -Path $previewDestination | Out-Null
    Get-ChildItem -LiteralPath $previewSource -File | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $previewDestination $_.Name)}
}
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
