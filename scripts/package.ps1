[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

$makeappx = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter MakeAppx.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $makeappx) { throw 'MakeAppx.exe from Windows SDK 10.0.26100.7175 or later is required. No package was created.' }
& (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$stage = Join-Path $PSScriptRoot '..\artifacts\package-stage'
New-Item -ItemType Directory -Force -Path $stage, (Join-Path $stage 'Assets') | Out-Null
Copy-Item (Join-Path $PSScriptRoot '..\packaging\Package.appxmanifest') $stage -Force
Copy-Item (Join-Path $PSScriptRoot "..\artifacts\$Configuration\aeDaeApp.exe") $stage -Force
throw 'Packaging payload assets and full-trust extension validation are intentionally incomplete. No package was created.'

