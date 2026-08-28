[CmdletBinding()]
param(
    [string]$SdkVersion = '10.0.26100.0'
)

$approvedSdkVersion = '10.0.26100.0'
$approvedWebAuthnHash = 'FB575592CE15D8F672386AAB5E6B3D3AAC101F4EC11A8C52C8085CD6D59A9665'
$approvedPluginHash = '8B8897A5FE7D4575B5DE8287C7F0E79CED3D96CAF6D273BC7E473E225AC873B8'
$approvedAuthenticatorHash = '3B5C60E4972AA9FCD3BD6499335F5BE241A757DD628E231FF0AE7ADBC7F859E7'
if ($SdkVersion -ne $approvedSdkVersion) {
    throw "Unapproved Windows SDK version: $SdkVersion. Expected $approvedSdkVersion; re-pinning requires security review."
}

$includeRoot = "C:\Program Files (x86)\Windows Kits\10\Include\$SdkVersion\um"
$pluginHeader = Join-Path $includeRoot 'webauthnplugin.h'
$authenticatorHeader = Join-Path $includeRoot 'pluginauthenticator.h'
$webAuthnHeader = Join-Path $includeRoot 'webauthn.h'
foreach ($path in @($webAuthnHeader, $pluginHeader, $authenticatorHeader)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required official WebAuthn contract header is missing: $path"
    }
}

$pluginText = Get-Content -LiteralPath $pluginHeader -Raw
$authenticatorText = Get-Content -LiteralPath $authenticatorHeader -Raw
$requiredPluginSymbols = @(
    'WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS',
    'WebAuthNPluginAddAuthenticator(',
    'EXPERIMENTAL_WebAuthNPluginAddAuthenticator2(',
    'WebAuthNPluginPerformUserVerification(',
    'EXPERIMENTAL_WebAuthNPluginPerformUserVerification2('
)
$requiredAuthenticatorSymbols = @('IPluginAuthenticator', 'MakeCredential(', 'GetAssertion(', 'CancelOperation(', 'GetLockStatus(')
foreach ($symbol in $requiredPluginSymbols) {
    if (-not $pluginText.Contains($symbol)) { throw "Missing required WebAuthn Plugin API symbol: $symbol" }
}
foreach ($symbol in $requiredAuthenticatorSymbols) {
    if (-not $authenticatorText.Contains($symbol)) { throw "Missing required plugin authenticator contract symbol: $symbol" }
}

$pluginHash = (Get-FileHash -LiteralPath $pluginHeader -Algorithm SHA256).Hash
$authenticatorHash = (Get-FileHash -LiteralPath $authenticatorHeader -Algorithm SHA256).Hash
$webAuthnHash = (Get-FileHash -LiteralPath $webAuthnHeader -Algorithm SHA256).Hash
if ($webAuthnHash -ne $approvedWebAuthnHash) {
    throw "webauthn.h hash mismatch. Expected $approvedWebAuthnHash, got $webAuthnHash."
}
if ($pluginHash -ne $approvedPluginHash) {
    throw "webauthnplugin.h hash mismatch. Expected $approvedPluginHash, got $pluginHash."
}
if ($authenticatorHash -ne $approvedAuthenticatorHash) {
    throw "pluginauthenticator.h hash mismatch. Expected $approvedAuthenticatorHash, got $authenticatorHash."
}

$stableInterface = 'MIDL_INTERFACE("d26bcf6f-b54c-43ff-9f06-d5bf148625f7")'
${stableInterfaceStart} = $authenticatorText.IndexOf($stableInterface)
if ($stableInterfaceStart -lt 0) { throw 'Unexpected IPluginAuthenticator IID declaration.' }
${stableInterfaceEnd} = $authenticatorText.IndexOf('    };', ${stableInterfaceStart})
if (${stableInterfaceEnd} -lt 0) { throw 'Unexpected IPluginAuthenticator declaration terminator.' }
${stableInterfaceText} = $authenticatorText.Substring(${stableInterfaceStart}, ${stableInterfaceEnd} - ${stableInterfaceStart})
$methodPositions = @('MakeCredential(', 'GetAssertion(', 'CancelOperation(', 'GetLockStatus(') | ForEach-Object { ${stableInterfaceText}.IndexOf($_) }
$previousPosition = -1
foreach ($position in $methodPositions) {
    if ($position -lt 0 -or $position -le $previousPosition) {
        throw 'Unexpected IPluginAuthenticator operation method order.'
    }
    $previousPosition = $position
}
if ($methodPositions.Count -ne 4) {
    throw 'Unexpected IPluginAuthenticator operation method order.'
}
if ($pluginText -match '(?<!EXPERIMENTAL_)WebAuthNPluginAddAuthenticator2\(' -or
    $pluginText -match '(?<!EXPERIMENTAL_)WebAuthNPluginPerformUserVerification2\(') {
    throw 'Unprefixed v2 plugin API detected; experimental APIs must retain their EXPERIMENTAL_ prefix.'
}
Write-Output "WebAuthn plugin contract verified: SDK $SdkVersion"
Write-Output "webauthn.h SHA256: $webAuthnHash"
Write-Output "webauthnplugin.h SHA256: $pluginHash"
Write-Output "pluginauthenticator.h SHA256: $authenticatorHash"
