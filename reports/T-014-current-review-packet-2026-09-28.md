TASK: Let's work on the system-wide plugin and anything else to complete. Review exact current human-protected T-014 contract-checker candidate without editing it.
MODE: first-pass
FILES_TOUCHED: none by reviewer; inspect four protected paths only.
EXACT CURRENT SHA-256:
C:\Projects\aeDae\reports\webauthnplugin-abi-manifest.json 78859F16F2CFCF92E21452CFCE1ED837E1F91C6F14A3C667DC678944BBEFEC0E
C:\Projects\aeDae\scripts\ContractCheck.psm1 36EF711DE88BDFFAFDF6858C53EF603EA123F2E434BB69743E3E521C68B52192
C:\Projects\aeDae\scripts\verify-webauthnplugin-contract.ps1 829B4F0D7AB801127CFC5DC9D7544D19996E5D667C3F3BD0D305292065CDC3DF
C:\Projects\aeDae\scripts\test-webauthnplugin-contract-guard.ps1 FE4FD38A5E604360B31498A3759E7A88FEAC9E2433D9DCD8C8275A9D1FFF5AF3
DIFF AGAINST origin/main ef173ca:
```
diff --git a/scripts/ContractCheck.psm1 b/scripts/ContractCheck.psm1
index e70bdd9..3be1990 100644
--- a/scripts/ContractCheck.psm1
+++ b/scripts/ContractCheck.psm1
@@ -15,6 +15,31 @@ function Invoke-AeDaeHeaderCheck {
     $plugin=Get-Content (Join-Path $IncludeRoot 'webauthnplugin.h') -Raw; $auth=Get-Content (Join-Path $IncludeRoot 'pluginauthenticator.h') -Raw
     foreach($symbol in @('WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS','WebAuthNPluginAddAuthenticator(','EXPERIMENTAL_WebAuthNPluginAddAuthenticator2(','WebAuthNPluginPerformUserVerification(','EXPERIMENTAL_WebAuthNPluginPerformUserVerification2(')){if(-not $plugin.Contains($symbol)){throw "Missing required WebAuthn Plugin API symbol: $symbol"}}
     if(-not $auth.Contains('IPluginAuthenticator')){throw 'Missing IPluginAuthenticator token.'}; $iid='MIDL_INTERFACE("'+$manifest.stableInterface.iid+'")';$start=$auth.IndexOf($iid);if($start -lt 0){throw 'Unexpected IPluginAuthenticator IID declaration.'};$end=$auth.IndexOf('    };',$start);if($end -lt 0){throw 'Unexpected IPluginAuthenticator declaration terminator.'};$body=$auth.Substring($start,$end-$start);$last=-1;foreach($m in $manifest.stableInterface.methods){$p=$body.IndexOf("$m(");if($p -lt 0 -or $p -le $last){throw 'Unexpected IPluginAuthenticator operation method order.'};$last=$p}
     foreach($symbol in $manifest.forbiddenUnprefixedSymbols){if($plugin -match "(?<!EXPERIMENTAL_)$([regex]::Escape($symbol))\("){throw "Unprefixed experimental API detected: $symbol"}}
 }
-Export-ModuleMember -Function Invoke-AeDaeProjectSdkCheck, Invoke-AeDaeHeaderCheck
+function Invoke-AeDaeSourceCheck {
+    # ADR-001: EXPERIMENTAL_-prefixed APIs are compile-time prohibited. The header check proves the
+    # SDK still prefixes them; this proves our own sources never reference one. See
+    # reports/lifecycle-security-review.md F8 and tasks.md T-018 MED-05.
+    param([string]$Root)
+    $targets = @()
+    $srcRoot = Join-Path $Root 'src'
+    if (Test-Path $srcRoot) { $targets += Get-ChildItem $srcRoot -Recurse -File }
+    foreach ($dir in @('build', 'artifacts')) {
+        $path = Join-Path $Root $dir
+        if (Test-Path $path) { $targets += Get-ChildItem $path -Recurse -File -Include '*.map', '*.vcxproj', '*.props', '*.def' }
+    }
+    foreach ($file in $targets) {
+        if (Select-String -LiteralPath $file.FullName -Pattern 'EXPERIMENTAL_' -SimpleMatch -Quiet -ErrorAction SilentlyContinue) {
+            throw "Prohibited EXPERIMENTAL_ API reference (ADR-001) in: $($file.FullName)"
+        }
+        # ADR-002: SQLite is not an approved dependency. The amalgamation may still sit untracked
+        # under external/, so a prose ban is not enough — fail the build on any reference to it.
+        foreach ($token in @('sqlite3.h', 'sqlite3.c', 'sqlite3ext.h', 'sqlite-amalgamation')) {
+            if (Select-String -LiteralPath $file.FullName -Pattern $token -SimpleMatch -Quiet -ErrorAction SilentlyContinue) {
+                throw "Prohibited SQLite dependency reference (ADR-002) in: $($file.FullName)"
+            }
+        }
+    }
+}
+Export-ModuleMember -Function Invoke-AeDaeProjectSdkCheck, Invoke-AeDaeHeaderCheck, Invoke-AeDaeSourceCheck
diff --git a/scripts/test-webauthnplugin-contract-guard.ps1 b/scripts/test-webauthnplugin-contract-guard.ps1
index 14ba666..f6cad99 100644
--- a/scripts/test-webauthnplugin-contract-guard.ps1
+++ b/scripts/test-webauthnplugin-contract-guard.ps1
@@ -1,4 +1,7 @@
 $root=Split-Path $PSScriptRoot -Parent;$manifest=Join-Path $root 'reports\webauthnplugin-abi-manifest.json';Import-Module (Join-Path $PSScriptRoot 'ContractCheck.psm1') -Force;$sdk=(Get-Content $manifest -Raw|ConvertFrom-Json).sdkVersion;$live=Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include\$sdk\um"
 try{Invoke-AeDaeProjectSdkCheck @((Join-Path $root 'tests\fixtures\WindowsSdkVersionMismatch.vcxproj')) $manifest;throw 'Expected SDK mismatch'}catch{if($_.Exception.Message -notlike 'Pinned build project targets*'){throw}}
 foreach($header in @('webauthn.h','webauthnplugin.h','pluginauthenticator.h')){$temp=Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString('N'));New-Item -ItemType Directory $temp|Out-Null;try{foreach($copy in @('webauthn.h','webauthnplugin.h','pluginauthenticator.h')){Copy-Item (Join-Path $live $copy) (Join-Path $temp $copy)};Add-Content (Join-Path $temp $header) '// mutation';try{Invoke-AeDaeHeaderCheck $temp $manifest;throw 'Expected header mismatch'}catch{if($_.Exception.Message -notlike "$header hash mismatch*"){throw}}}finally{Remove-Item $temp -Recurse -Force}}
+$temp=Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString('N'));New-Item -ItemType Directory (Join-Path $temp 'src')|Out-Null;try{Set-Content (Join-Path $temp 'src\Prohibited.cpp') 'HRESULT h = EXPERIMENTAL_WebAuthNPluginAddAuthenticator2(nullptr, nullptr);';try{Invoke-AeDaeSourceCheck $temp;throw 'Expected prohibited EXPERIMENTAL_ reference'}catch{if($_.Exception.Message -notlike 'Prohibited EXPERIMENTAL_ API reference*'){throw}}}finally{Remove-Item $temp -Recurse -Force}
+$temp=Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString('N'));New-Item -ItemType Directory (Join-Path $temp 'src')|Out-Null;try{Set-Content (Join-Path $temp 'src\Vault.cpp') '#include "sqlite3.h"';try{Invoke-AeDaeSourceCheck $temp;throw 'Expected prohibited SQLite reference'}catch{if($_.Exception.Message -notlike 'Prohibited SQLite dependency reference*'){throw}}}finally{Remove-Item $temp -Recurse -Force}
+Invoke-AeDaeSourceCheck $root
 Write-Output 'WebAuthn contract guard negative tests passed'
diff --git a/scripts/verify-webauthnplugin-contract.ps1 b/scripts/verify-webauthnplugin-contract.ps1
index 7c80a8c..7255bb4 100644
--- a/scripts/verify-webauthnplugin-contract.ps1
+++ b/scripts/verify-webauthnplugin-contract.ps1
@@ -1,4 +1,4 @@
 $ErrorActionPreference = 'Stop'
 $root=Split-Path $PSScriptRoot -Parent;$manifest=Join-Path $root 'reports\webauthnplugin-abi-manifest.json';Import-Module (Join-Path $PSScriptRoot 'ContractCheck.psm1') -Force
 $projects=@((Join-Path $root 'build\AeDaePlugin.vcxproj'),(Join-Path $root 'build\AeDaeApp.vcxproj'),(Join-Path $root 'build\ComActivationHarness.vcxproj'));Invoke-AeDaeProjectSdkCheck $projects $manifest
-$sdk=(Get-Content $manifest -Raw|ConvertFrom-Json).sdkVersion;$include=Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include\$sdk\um";Invoke-AeDaeHeaderCheck $include $manifest;Write-Output "WebAuthn plugin contract verified: SDK $sdk"
+$sdk=(Get-Content $manifest -Raw|ConvertFrom-Json).sdkVersion;$include=Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include\$sdk\um";Invoke-AeDaeHeaderCheck $include $manifest;Invoke-AeDaeSourceCheck $root;Write-Output "WebAuthn plugin contract verified: SDK $sdk"
```
CURRENT ContractCheck.psm1:
```
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
        if (Select-String -LiteralPath $file.FullName -Pattern 'EXPERIMENTAL_' -SimpleMatch -Quiet -ErrorAction SilentlyContinue) {
            throw "Prohibited EXPERIMENTAL_ API reference (ADR-001) in: $($file.FullName)"
        }
        # ADR-002: SQLite is not an approved dependency. The amalgamation may still sit untracked
        # under external/, so a prose ban is not enough — fail the build on any reference to it.
        foreach ($token in @('sqlite3.h', 'sqlite3.c', 'sqlite3ext.h', 'sqlite-amalgamation')) {
            if (Select-String -LiteralPath $file.FullName -Pattern $token -SimpleMatch -Quiet -ErrorAction SilentlyContinue) {
                throw "Prohibited SQLite dependency reference (ADR-002) in: $($file.FullName)"
            }
        }
    }
}
Export-ModuleMember -Function Invoke-AeDaeProjectSdkCheck, Invoke-AeDaeHeaderCheck, Invoke-AeDaeSourceCheck
```
TESTS: 2026-09-28 local Debug scripts/build.ps1 passed contract verifier, negative guard, MSBuild (0 warnings/errors), COM/VaultStore/ManagementModel/KeyProtection tests. Hosted CI is not green; its mutable Windows SDK header hash differs from the manifest.
CONSTRAINTS: AGENTS.md forbids any agent edit of the four protected files. T-014 is only contract integrity plus bootstrap COM lifetime, not T-019 signing-envelope semantics or plugin operations. Evaluate five questions from reports/T-014-protected-diff-review-packet.md: fail-closed enumeration/read especially Select-String SilentlyContinue; literal-token scan sufficiency; SQLite-token scope/false negatives; production/negative guard parity; no weakened SDK/hash/IID/method-order checks. Return severity/file/line/exploit or failure condition/fix/verification, plus explicit APPROVE, APPROVE_WITH_REQUIRED_CHANGES, or BLOCKED for these exact hashes. Review-only: no tools or edits.

