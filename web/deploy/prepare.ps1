# Build five self-contained preview bundles without network access or deployment.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$webRoot = Split-Path -Parent $PSScriptRoot
$distRoot = Join-Path $webRoot 'dist'
$previewRoot = [IO.Path]::GetFullPath((Join-Path $webRoot 'preview'))
$pagesRoot = [IO.Path]::GetFullPath((Join-Path $previewRoot 'pages'))
$expectedPrefix = $previewRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $pagesRoot.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to prepare outside the web preview directory: $pagesRoot"
}
foreach ($path in @($previewRoot, $pagesRoot)) {
    if (Test-Path -LiteralPath $path) {
        $item = Get-Item -LiteralPath $path -Force
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing to use reparse-point preview path: $path"
        }
    }
}

& node (Join-Path $webRoot 'build.mjs')
if ($LASTEXITCODE -ne 0) { throw 'Build failed. Preview bundles were not prepared.' }

if (Test-Path -LiteralPath $pagesRoot) {
    Remove-Item -LiteralPath $pagesRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $pagesRoot | Out-Null

$headerText = [IO.File]::ReadAllText((Join-Path $PSScriptRoot '_headers.template'))
$surfaces = @('tech', 'world', 'life', 'live', 'online')
foreach ($surface in $surfaces) {
    $bundle = Join-Path $pagesRoot $surface
    New-Item -ItemType Directory -Path $bundle | Out-Null
    Copy-Item -Path (Join-Path $distRoot '*') -Destination $bundle -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $distRoot "$surface\index.html") -Destination (Join-Path $bundle 'index.html') -Force
    [IO.File]::WriteAllText((Join-Path $bundle '_headers'), $headerText, [Text.UTF8Encoding]::new($false))

    foreach ($required in @('index.html', 'assets\styles.css', 'tech\index.html', 'world\index.html', 'life\index.html', 'live\index.html', 'online\index.html', '_headers')) {
        if (-not (Test-Path -LiteralPath (Join-Path $bundle $required))) {
            throw "Preview bundle aedae-$surface is missing $required"
        }
    }
    Write-Host "Prepared aedae-$surface at $bundle"
}
