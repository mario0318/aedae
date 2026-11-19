$ErrorActionPreference = 'Stop'

function Invoke-AeDaeProjectSdkCheck {
    param([string[]]$ProjectFiles, [string]$ManifestPath)
    $manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    foreach ($project in $ProjectFiles) {
        $sdkMatches = [regex]::Matches((Get-Content $project -Raw), '<WindowsTargetPlatformVersion>(?<v>[^<]+)</WindowsTargetPlatformVersion>')
        if ($sdkMatches.Count -ne 1 -or $sdkMatches[0].Groups['v'].Value.Trim() -ne $manifest.sdkVersion) { throw "Pinned build project targets an unapproved Windows SDK: $project" }
    }
}
function Invoke-AeDaeHeaderCheck {
    param([string]$IncludeRoot, [string]$ManifestPath)
    $manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    foreach ($header in $manifest.headers.PSObject.Properties) { $path = Join-Path $IncludeRoot $header.Name; if (-not (Test-Path $path -PathType Leaf)) { throw "Required official contract header is missing: $path" }; $actual=(Get-FileHash $path -Algorithm SHA256).Hash; if($actual -ne $header.Value){throw "$($header.Name) hash mismatch. Expected $($header.Value), got $actual."} }
    $plugin=Get-Content (Join-Path $IncludeRoot 'webauthnplugin.h') -Raw; $auth=Get-Content (Join-Path $IncludeRoot 'pluginauthenticator.h') -Raw
    foreach($symbol in @('WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS','WebAuthNPluginAddAuthenticator(','EXPERIMENTAL_WebAuthNPluginAddAuthenticator2(','WebAuthNPluginPerformUserVerification(','EXPERIMENTAL_WebAuthNPluginPerformUserVerification2(')){if(-not $plugin.Contains($symbol)){throw "Missing required WebAuthn Plugin API symbol: $symbol"}}
    if(-not $auth.Contains('IPluginAuthenticator')){throw 'Missing IPluginAuthenticator token.'}; $iid='MIDL_INTERFACE("'+$manifest.stableInterface.iid+'")';$start=$auth.IndexOf($iid);if($start -lt 0){throw 'Unexpected IPluginAuthenticator IID declaration.'};$end=$auth.IndexOf('    };',$start);if($end -lt 0){throw 'Unexpected IPluginAuthenticator declaration terminator.'};$body=$auth.Substring($start,$end-$start);$last=-1;foreach($m in $manifest.stableInterface.methods){$p=$body.IndexOf("$m(");if($p -lt 0 -or $p -le $last){throw 'Unexpected IPluginAuthenticator operation method order.'};$last=$p}
    foreach($symbol in $manifest.forbiddenUnprefixedSymbols){if($plugin -match "(?<!EXPERIMENTAL_)$([regex]::Escape($symbol))\("){throw "Unprefixed experimental API detected: $symbol"}}
}
function Invoke-AeDaeSourceCheck {
    # ADR-001: EXPERIMENTAL_-prefixed APIs are compile-time prohibited. The header check proves the
    # SDK still prefixes them; this proves our own sources never reference one. See
    # reports/lifecycle-security-review.md F8 and tasks.md T-018 MED-05.
    param([string]$Root)
    $targets = @()
    $srcRoot = Join-Path $Root 'src'
    if (Test-Path $srcRoot) { $targets += Get-ChildItem $srcRoot -Recurse -File }
    foreach ($dir in @('build', 'artifacts')) {
        $path = Join-Path $Root $dir
        if (Test-Path $path) { $targets += Get-ChildItem $path -Recurse -File -Include '*.map', '*.vcxproj', '*.props', '*.def' }
    }
    foreach ($file in $targets) {
        if (Select-String -LiteralPath $file.FullName -Pattern 'EXPERIMENTAL_' -SimpleMatch -Quiet -ErrorAction Stop) {
            throw "Prohibited EXPERIMENTAL_ API reference (ADR-001) in: $($file.FullName)"
        }
        # ADR-002: SQLite is not an approved dependency. The amalgamation may still sit untracked
        # under external/, so a prose ban is not enough — fail the build on any reference to it.
        foreach ($token in @('sqlite3.h', 'sqlite3.c', 'sqlite3ext.h', 'sqlite-amalgamation')) {
            if (Select-String -LiteralPath $file.FullName -Pattern $token -SimpleMatch -Quiet -ErrorAction Stop) {
                throw "Prohibited SQLite dependency reference (ADR-002) in: $($file.FullName)"
            }
        }
    }
}
Export-ModuleMember -Function Invoke-AeDaeProjectSdkCheck, Invoke-AeDaeHeaderCheck, Invoke-AeDaeSourceCheck
