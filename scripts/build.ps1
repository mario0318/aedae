[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')

& (Join-Path $PSScriptRoot 'verify-webauthnplugin-contract.ps1')
if (-not $?) { exit 1 }
& (Join-Path $PSScriptRoot 'test-webauthnplugin-contract-guard.ps1')
if (-not $?) { exit 1 }

$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Build Tools with C++ support is required.' }
$msbuild = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild with C++ support was not found.' }
& $msbuild (Join-Path $PSScriptRoot '..\AeDae.sln') "/p:Configuration=$Configuration" '/p:Platform=x64' /m
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot "..\artifacts\$Configuration\ComActivationHarness.exe") (Join-Path $PSScriptRoot "..\artifacts\$Configuration\aeDaePlugin.dll")
exit $LASTEXITCODE
