[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'

$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$powershell = (Get-Process -Id $PID).Path
$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('aedae-build-gates-' + [guid]::NewGuid().ToString('N'))

foreach ($failingGate in @('verify', 'negative')) {
    $scripts = Join-Path $fixtureRoot "$failingGate/scripts"
    New-Item -ItemType Directory -Path $scripts -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $repo 'scripts/build.ps1') -Destination $scripts
    $verify = if ($failingGate -eq 'verify') { 'gate-fail-23.ps1' } else { 'gate-success.ps1' }
    $negative = if ($failingGate -eq 'negative') { 'gate-fail-23.ps1' } else { 'gate-success.ps1' }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "fixtures/$verify") -Destination (Join-Path $scripts 'verify-webauthnplugin-contract.ps1')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "fixtures/$negative") -Destination (Join-Path $scripts 'test-webauthnplugin-contract-guard.ps1')
    $output = & $powershell -NoProfile -File (Join-Path $scripts 'build.ps1') -Configuration Debug 2>&1
    if ($LASTEXITCODE -ne 23) { throw "Gate $failingGate failed to propagate exit 23: $LASTEXITCODE" }
    if ($output -match 'MSBuild|Build started') { throw "Gate $failingGate allowed compilation" }
    Write-Output "PASS $failingGate rejection stops build and preserves exit code"
}

Write-Output "Synthetic build-gate fixtures retained at $fixtureRoot"
exit 0
