$ErrorActionPreference = 'Stop'
$taskBuildPath = $env:Path
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $taskBuildPath, 'Process')
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'Visual Studio C++ build tools were not found.' }
$taskRoot = Split-Path $PSScriptRoot -Parent
Push-Location $taskRoot
try {
    $checks = @(
        @('tests', 'theater_logic_tests'),
        @('preferences', 'preferences'),
        @('state_exchange', 'state_exchange'),
        @('playback_helper', 'playback_helper'),
        @('animations_runtime', 'animations_runtime'),
        @('native_calls', 'native_calls'),
        @('editor_runtime', 'editor_runtime'),
        @('cursor_hooks', 'cursor_hooks'),
        @('ui_preview', 'ui_preview')
    )
    foreach ($check in $checks) {
        & $msbuild "tests\$($check[0]).vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Test build failed: $($check[0])" }
        & ".\build\tests\$($check[1]).exe"
        if ($LASTEXITCODE -ne 0) { throw "Check failed: $($check[1]) ($LASTEXITCODE)" }
    }
} finally { Pop-Location }
