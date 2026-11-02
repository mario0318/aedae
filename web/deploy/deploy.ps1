# Deploy aeDae Pages projects as PREVIEW deployments (branch alias on *.pages.dev).
# Never deploys to a project's production branch, so a bound custom domain is never touched.
# Defaults to the life surface only; pass -Surface tech,world,life,live,online for all five.
# Run only after operator authentication and explicit deployment authorization.
param(
    [ValidateSet('tech', 'world', 'life', 'live', 'online')]
    [string[]]$Surface = @('life'),
    [ValidatePattern('^[a-z0-9-]+$')]
    [ValidateScript({ $_ -ne 'main' })]
    [string]$Branch = 'preview'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$webRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $webRoot
$surfaces = $Surface
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
        $config = Join-Path $PSScriptRoot "wrangler.$surface.toml"
        if (-not (Test-Path -LiteralPath $config)) {
            throw "Missing Wrangler configuration for ${project}: $config"
        }

        Write-Host "`n-- Ensuring $project exists --"
        $created = & $npx @wranglerArgs 'pages' 'project' 'create' $project '--production-branch' 'main' 2>&1
        $creationText = $created -join "`n"
        $existingProject = [regex]::IsMatch(
            $creationText,
            "(?is)(?=.*\b$([regex]::Escape($project))\b)(?=.*\balready exists\b)"
        )
        if ($LASTEXITCODE -ne 0 -and -not $existingProject) {
            throw "Could not create or find Pages project ${project}:`n$creationText"
        }
        $created | ForEach-Object { Write-Host $_ }

        Write-Host "`n-- Deploying $project --"
        $deployed = Invoke-Wrangler @('pages', 'deploy', $dist, '--project-name', $project, '--branch', $Branch, '--config', $config)
        $deployed | ForEach-Object { Write-Host $_ }
        $match = [regex]::Match(($deployed -join "`n"), 'https://[A-Za-z0-9.-]+\.pages\.dev')
        if ($match.Success) { $urls[$surface] = $match.Value }
    }

    Write-Host "`n-- Temporary URLs (DNS untouched) --"
    foreach ($surface in $surfaces) {
        $url = if ($urls.ContainsKey($surface)) { $urls[$surface] } else { '(see Wrangler output above)' }
        Write-Host "aedae.$surface (preview branch $Branch) -> $url"
    }
    Write-Host "`nVerify each URL (node web/deploy/verify-preview.mjs <url>) before any custom-domain or DNS step."
}
finally {
    Pop-Location
}
