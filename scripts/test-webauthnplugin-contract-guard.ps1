[CmdletBinding()]
param()

$verifier = Join-Path $PSScriptRoot 'verify-webauthnplugin-contract.ps1'
$project = Join-Path $PSScriptRoot '..\build\AeDaePlugin.vcxproj'
$mismatchProject = Join-Path $PSScriptRoot '..\tests\fixtures\WindowsSdkVersionMismatch.vcxproj'

function Assert-FailsClosed([scriptblock]$Action, [string]$ExpectedMessage)
{
    try
    {
        & $Action
    }
    catch
    {
        if ($_.Exception.Message -like $ExpectedMessage) { return }
        throw "Unexpected failure: $($_.Exception.Message)"
    }
    throw "Expected verifier failure: $ExpectedMessage"
}

Assert-FailsClosed { & $verifier -ProjectFile $mismatchProject } 'Plugin project targets unapproved Windows SDK version*'

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("aedae-contract-test-" + [guid]::NewGuid().ToString('N'))
try
{
    New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null
    $liveRoot = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um'
    foreach ($header in @('webauthn.h', 'webauthnplugin.h', 'pluginauthenticator.h'))
    {
        Copy-Item -LiteralPath (Join-Path $liveRoot $header) -Destination (Join-Path $temporaryRoot $header)
    }
    Add-Content -LiteralPath (Join-Path $temporaryRoot 'webauthnplugin.h') -Value '// deliberate test mutation'
    Assert-FailsClosed { & $verifier -ProjectFile $project -ContractIncludeRoot $temporaryRoot } 'webauthnplugin.h hash mismatch*'
}
finally
{
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}

Write-Output 'WebAuthn contract guard negative tests passed'
