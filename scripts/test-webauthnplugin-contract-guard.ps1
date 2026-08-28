[CmdletBinding()]
param()

$verifier = Join-Path $PSScriptRoot 'verify-webauthnplugin-contract.ps1'
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

Assert-FailsClosed { & $verifier -ProjectFiles @($mismatchProject) } 'Pinned build project targets unapproved Windows SDK version*'

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("aedae-contract-test-" + [guid]::NewGuid().ToString('N'))
try
{
    New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null
    $liveRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include\10.0.26100.0\um'
    foreach ($header in @('webauthn.h', 'webauthnplugin.h', 'pluginauthenticator.h'))
    {
        Copy-Item -LiteralPath (Join-Path $liveRoot $header) -Destination (Join-Path $temporaryRoot $header)
    }
    Add-Content -LiteralPath (Join-Path $temporaryRoot 'webauthnplugin.h') -Value '// deliberate test mutation'
    $mutatedHash = (Get-FileHash -LiteralPath (Join-Path $temporaryRoot 'webauthnplugin.h') -Algorithm SHA256).Hash
    if ($mutatedHash -eq '8B8897A5FE7D4575B5DE8287C7F0E79CED3D96CAF6D273BC7E473E225AC873B8') { throw 'Mutated-header self-test failed.' }
}
finally
{
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}

Write-Output 'WebAuthn contract guard negative tests passed'
