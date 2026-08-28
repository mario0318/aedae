function Invoke-AeDaeHeaderCheck { param([string]$IncludeRoot,[string]$ManifestPath)
 $manifest=Get-Content $ManifestPath -Raw|ConvertFrom-Json
 foreach($p in $manifest.headers.PSObject.Properties){$f=Join-Path $IncludeRoot $p.Name;if(-not(Test-Path $f)){throw "Required header missing: $f"};$h=(Get-FileHash $f -Algorithm SHA256).Hash;if($h -ne $p.Value){throw "$($p.Name) hash mismatch. Expected $($p.Value), got $h."}}
 $plugin=Get-Content (Join-Path $IncludeRoot 'webauthnplugin.h') -Raw;$auth=Get-Content (Join-Path $IncludeRoot 'pluginauthenticator.h') -Raw
 foreach($s in @('WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS','WebAuthNPluginAddAuthenticator(','EXPERIMENTAL_WebAuthNPluginAddAuthenticator2(','WebAuthNPluginPerformUserVerification(','EXPERIMENTAL_WebAuthNPluginPerformUserVerification2(')){if(-not $plugin.Contains($s)){throw "Missing required WebAuthn Plugin API symbol: $s"}}
 $iid='MIDL_INTERFACE("' + $manifest.stableInterface.iid + '")';$start=$auth.IndexOf($iid);if($start -lt 0){throw 'Unexpected IPluginAuthenticator IID declaration.'};$end=$auth.IndexOf('    };',$start);$body=$auth.Substring($start,$end-$start);$last=-1;foreach($m in $manifest.stableInterface.methods){$pos=$body.IndexOf("$m(");if($pos -lt 0 -or $pos -le $last){throw 'Unexpected IPluginAuthenticator operation method order.'};$last=$pos}
 foreach($s in $manifest.forbiddenUnprefixedSymbols){if($plugin -match "(?<!EXPERIMENTAL_)$([regex]::Escape($s))\("){throw "Unprefixed experimental API detected: $s"}}
}
Export-ModuleMember -Function Invoke-AeDaeHeaderCheck
