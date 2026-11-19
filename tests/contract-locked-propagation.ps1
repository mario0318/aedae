$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$sourceRoot = Join-Path $root 'src'
$scripts = @(
    (Join-Path $root 'scripts\verify-webauthnplugin-contract.ps1'),
    (Join-Path $root 'scripts\build.ps1')
)
foreach ($script in $scripts) {
    if (-not (Test-Path -LiteralPath $script -PathType Leaf)) {
        throw "Required script is missing: $script"
    }
}

$fixture = Join-Path $sourceRoot ("ContractLockedFixture-$([guid]::NewGuid().ToString('N')).cpp")
$lock = $null
try {
    Set-Content -LiteralPath $fixture -Value '// disposable source read-failure fixture'
    $lock = [IO.File]::Open($fixture, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
    $powershell = (Get-Process -Id $PID).Path
    foreach ($script in $scripts) {
        $stdout = Join-Path ([IO.Path]::GetTempPath()) ("aedae-contract-$([guid]::NewGuid().ToString('N')).out")
        $stderr = Join-Path ([IO.Path]::GetTempPath()) ("aedae-contract-$([guid]::NewGuid().ToString('N')).err")
        try {
            $child = Start-Process -FilePath $powershell -ArgumentList "-NoProfile -File `"$script`"" `
                -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            $output = (Get-Content -LiteralPath $stdout -Raw) + (Get-Content -LiteralPath $stderr -Raw)
            if ($null -eq $child.ExitCode -or $child.ExitCode -eq 0 -or
                $output -match 'WebAuthn plugin contract verified' -or
                -not $output.Contains([IO.Path]::GetFileName($fixture))) {
                throw "Locked source was not propagated by $script (exit $($child.ExitCode)): $output"
            }
            Write-Output "Locked source rejected by $(Split-Path $script -Leaf) (exit $($child.ExitCode))"
        }
        finally {
            Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
        }
    }
}
finally {
    if ($lock) { $lock.Dispose() }
    Remove-Item -LiteralPath $fixture -Force -ErrorAction SilentlyContinue
}
if (Test-Path -LiteralPath $fixture) {
    throw "Disposable source fixture was not removed: $fixture"
}
