# Deploy five independent aeDae Pages projects to temporary *.pages.dev URLs.
# Run from the repository root or any directory after `wrangler login`.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$webRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $webRoot
$surfaces = @('tech', 'world', 'life', 'live', 'online')
$npx = 'npx'
$wranglerArgs = @('--yes', 'wrangler@4')

function Invoke-Wrangler([string[]]$Arguments) {
    $output = & $npx @wranglerArgs @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Wrangler failed ($($Arguments -join ' ')):`n$($output -join "`n")"
    }
    return @($output)
}

Push-Location $repoRoot
try {
    Write-Host '-- Build and prepare --'
    & (Join-Path $PSScriptRoot 'prepare.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Preview preparation failed. No Pages deployment was attempted.' }

    Write-Host "`n-- Auth check --"
    Invoke-Wrangler @('whoami') | ForEach-Object { Write-Host $_ }

    $urls = @{}
    foreach ($surface in $surfaces) {
        $project = "aedae-$surface"
        $dist = Join-Path $webRoot "preview\pages\$surface"

        Write-Host "`n-- Ensuring $project exists --"
        $created = & $npx @wranglerArgs 'pages' 'project' 'create' $project '--production-branch' 'main' 2>&1
        if ($LASTEXITCODE -ne 0 -and (($created -join "`n") -notmatch '(?i)already exists')) {
            throw "Could not create or find Pages project ${project}:`n$($created -join "`n")"
        }
        $created | ForEach-Object { Write-Host $_ }

        Write-Host "`n-- Deploying $project --"
        $deployed = Invoke-Wrangler @('pages', 'deploy', $dist, '--project-name', $project)
        $deployed | ForEach-Object { Write-Host $_ }
        $match = [regex]::Match(($deployed -join "`n"), 'https://[A-Za-z0-9-]+\.pages\.dev')
        if ($match.Success) { $urls[$surface] = $match.Value }
    }

    Write-Host "`n-- Temporary URLs (DNS untouched) --"
    foreach ($surface in $surfaces) {
        $url = if ($urls.ContainsKey($surface)) { $urls[$surface] } else { '(see Wrangler output above)' }
        Write-Host "aedae.$surface -> $url"
    }
    Write-Host "`nVerify all five URLs before any custom-domain or DNS step."
}
finally {
    Pop-Location
}
