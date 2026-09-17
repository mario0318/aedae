[CmdletBinding()]
param(
    [ValidatePattern('^\d+\.\d+\.\d+\.\d+$')][string]$Version = '0.1.0.0',
    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$artifactsRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'artifacts'))
$stage = [IO.Path]::GetFullPath((Join-Path $artifactsRoot 'package-stage'))
$packages = [IO.Path]::GetFullPath((Join-Path $artifactsRoot 'packages'))
$expectedPrefix = $artifactsRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
foreach ($path in @($stage, $packages)) {
    if (-not $path.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to write package material outside artifacts: $path"
    }
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $packages "aedae-$Version-x64.msix"
}
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
if (-not $OutputPath.StartsWith(
        $packages.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputPath must remain inside $packages"
}

$minimumMakeAppx = [version]'10.0.26100.7175'
$makeappx = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter MakeAppx.exe -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\x64\\MakeAppx\.exe$' -and $_.VersionInfo.FileVersionRaw -ge $minimumMakeAppx } |
    Sort-Object { $_.VersionInfo.FileVersionRaw } -Descending |
    Select-Object -First 1
if (-not $makeappx) {
    throw "x64 MakeAppx.exe $minimumMakeAppx or later is required. No package was created."
}

& (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

function New-DeveloperAsset([string]$Path, [int]$Size) {
    Add-Type -AssemblyName System.Drawing
    $bitmap = [Drawing.Bitmap]::new($Size, $Size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $background = [Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#0a0a0f'))
    $accent = [Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#d7ff64'))
    try {
        $graphics.FillRectangle($background, 0, 0, $Size, $Size)
        $unit = [Math]::Max(2, [Math]::Floor($Size / 11))
        $center = [Math]::Floor($Size / 2)
        $graphics.FillRectangle($accent, $center - $unit, $unit * 2, $unit * 2, $Size - ($unit * 4))
        $graphics.FillRectangle($accent, $unit * 2, $center - $unit, $Size - ($unit * 4), $unit * 2)
        $graphics.FillRectangle($background, $center - [Math]::Floor($unit / 2), $center - [Math]::Floor($unit / 2), $unit, $unit)
        $bitmap.SetResolution(96, 96)
        $bitmap.Save($Path, [Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $accent.Dispose()
        $background.Dispose()
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path (Join-Path $stage 'Assets') -Force | Out-Null
New-Item -ItemType Directory -Path $packages -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'packaging\Package.appxmanifest') -Destination (Join-Path $stage 'AppxManifest.xml')
Copy-Item -LiteralPath (Join-Path $artifactsRoot 'Release\aeDaeApp.exe') -Destination $stage
New-DeveloperAsset (Join-Path $stage 'Assets\StoreLogo.png') 50
New-DeveloperAsset (Join-Path $stage 'Assets\Square150x150Logo.png') 150
New-DeveloperAsset (Join-Path $stage 'Assets\Square44x44Logo.png') 44

$manifestPath = Join-Path $stage 'AppxManifest.xml'
[xml]$manifest = Get-Content -Raw -LiteralPath $manifestPath
$namespaces = [Xml.XmlNamespaceManager]::new($manifest.NameTable)
$namespaces.AddNamespace('f', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10')
$namespaces.AddNamespace('rescap', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities')
$identity = $manifest.SelectSingleNode('/f:Package/f:Identity', $namespaces)
$application = $manifest.SelectSingleNode('/f:Package/f:Applications/f:Application', $namespaces)
$runFullTrust = $manifest.SelectSingleNode('/f:Package/f:Capabilities/rescap:Capability[@Name="runFullTrust"]', $namespaces)
if (-not $identity -or $identity.ProcessorArchitecture -ne 'x64') { throw 'Manifest must declare one x64 package identity.' }
$identity.Version = $Version
if (-not $application -or $application.Executable -ne 'aeDaeApp.exe' -or
    $application.EntryPoint -ne 'Windows.FullTrustApplication' -or -not $runFullTrust) {
    throw 'Manifest full-trust application contract is incomplete.'
}
if ($manifest.SelectSingleNode('/f:Package/f:Capabilities/f:Capability[@Name="internetClient"]', $namespaces)) {
    throw 'Bootstrap package must not request internetClient.'
}
$manifest.Save($manifestPath)

$fixedTime = [datetime]::SpecifyKind([datetime]'2000-01-01T00:00:00', [DateTimeKind]::Utc)
Get-ChildItem -LiteralPath $stage -Recurse -Force | ForEach-Object { $_.LastWriteTimeUtc = $fixedTime }
(Get-Item -LiteralPath $stage).LastWriteTimeUtc = $fixedTime
if (Test-Path -LiteralPath $OutputPath) { Remove-Item -LiteralPath $OutputPath -Force }
& $makeappx.FullName pack /d $stage /p $OutputPath /o
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Add-Type -AssemblyName System.IO.Compression.FileSystem
# MakeAppx stamps every ZIP entry with the packaging time even when all staged
# inputs have fixed timestamps. Timestamps are not covered by AppxBlockMap.xml.
# Patch only the DOS time/date fields in the existing local and central headers;
# rewriting the archive through ZipArchive changes MakeAppx's data-descriptor
# layout and produces a container that MakeAppx itself refuses to unpack.
function Normalize-AppxTimestamps([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $eocd = -1
    $searchFloor = [Math]::Max(0, $bytes.Length - 65557)
    for ($index = $bytes.Length - 22; $index -ge $searchFloor; --$index) {
        if ([BitConverter]::ToUInt32($bytes, $index) -eq 0x06054b50 -and
            $index + 22 + [BitConverter]::ToUInt16($bytes, $index + 20) -eq $bytes.Length) {
            $eocd = $index
            break
        }
    }
    if ($eocd -lt 0) { throw 'Package has no ZIP end-of-central-directory record.' }
    $entryCount16 = [BitConverter]::ToUInt16($bytes, $eocd + 10)
    $centralSize32 = [BitConverter]::ToUInt32($bytes, $eocd + 12)
    $centralStart32 = [BitConverter]::ToUInt32($bytes, $eocd + 16)
    $usesZip64 = $entryCount16 -eq 0xffff -or $centralSize32 -eq [uint32]::MaxValue -or
        $centralStart32 -eq [uint32]::MaxValue
    if ($usesZip64) {
        $locator = $eocd - 20
        if ($locator -lt 0 -or [BitConverter]::ToUInt32($bytes, $locator) -ne 0x07064b50) {
            throw 'ZIP64 package has no locator immediately before the ordinary end record.'
        }
        if ([BitConverter]::ToUInt32($bytes, $locator + 4) -ne 0 -or
            [BitConverter]::ToUInt32($bytes, $locator + 16) -ne 1) {
            throw 'Multi-disk ZIP64 packages are not supported.'
        }
        $zip64Offset64 = [BitConverter]::ToUInt64($bytes, $locator + 8)
        if ($zip64Offset64 -gt [int]::MaxValue -or $zip64Offset64 + 56 -gt $locator) {
            throw 'ZIP64 end record is outside the bounded package.'
        }
        $zip64 = [int]$zip64Offset64
        if ([BitConverter]::ToUInt32($bytes, $zip64) -ne 0x06064b50 -or
            [BitConverter]::ToUInt64($bytes, $zip64 + 4) -lt 44) {
            throw 'ZIP64 end record is malformed.'
        }
        if ([BitConverter]::ToUInt32($bytes, $zip64 + 16) -ne 0 -or
            [BitConverter]::ToUInt32($bytes, $zip64 + 20) -ne 0) {
            throw 'Multi-disk ZIP64 packages are not supported.'
        }
        $entriesOnDisk64 = [BitConverter]::ToUInt64($bytes, $zip64 + 24)
        $entryCount64 = [BitConverter]::ToUInt64($bytes, $zip64 + 32)
        $centralSize64 = [BitConverter]::ToUInt64($bytes, $zip64 + 40)
        $centralStart64 = [BitConverter]::ToUInt64($bytes, $zip64 + 48)
        if ($entriesOnDisk64 -ne $entryCount64 -or $entryCount64 -gt [int]::MaxValue -or
            $centralSize64 -gt [int]::MaxValue -or $centralStart64 -gt [int]::MaxValue -or
            $centralStart64 + $centralSize64 -ne $zip64Offset64) {
            throw 'ZIP64 central-directory bounds or entry counts are inconsistent.'
        }
        $entryCount = [int]$entryCount64
        $centralSize = [int]$centralSize64
        $centralStart = [int]$centralStart64
    }
    else {
        if ([BitConverter]::ToUInt16($bytes, $eocd + 4) -ne 0 -or
            [BitConverter]::ToUInt16($bytes, $eocd + 6) -ne 0 -or
            [BitConverter]::ToUInt16($bytes, $eocd + 8) -ne $entryCount16) {
            throw 'Multi-disk packages are not supported.'
        }
        $entryCount = [int]$entryCount16
        $centralSize = [int]$centralSize32
        $centralStart = [int]$centralStart32
        if ($centralStart + $centralSize -ne $eocd) {
            throw 'Central-directory bounds are inconsistent.'
        }
    }
    $dosDate = 0x2821 # 2000-01-01; DOS time is 00:00:00.
    $central = $centralStart
    for ($entryIndex = 0; $entryIndex -lt $entryCount; ++$entryIndex) {
        if ($central + 46 -gt $bytes.Length -or
            [BitConverter]::ToUInt32($bytes, $central) -ne 0x02014b50) {
            throw "Malformed central directory at entry $entryIndex."
        }
        $nameLength = [BitConverter]::ToUInt16($bytes, $central + 28)
        $extraLength = [BitConverter]::ToUInt16($bytes, $central + 30)
        $commentLength = [BitConverter]::ToUInt16($bytes, $central + 32)
        $entryEnd = $central + 46 + $nameLength + $extraLength + $commentLength
        if ($entryEnd -gt $centralStart + $centralSize) {
            throw "Central entry $entryIndex exceeds the declared directory."
        }
        $local32 = [BitConverter]::ToUInt32($bytes, $central + 42)
        if ($local32 -eq [uint32]::MaxValue) {
            $extra = $central + 46 + $nameLength
            $extraEnd = $extra + $extraLength
            $local64 = $null
            while ($extra + 4 -le $extraEnd) {
                $extraId = [BitConverter]::ToUInt16($bytes, $extra)
                $fieldSize = [BitConverter]::ToUInt16($bytes, $extra + 2)
                $field = $extra + 4
                $fieldEnd = $field + $fieldSize
                if ($fieldEnd -gt $extraEnd) { throw "Malformed extra field at entry $entryIndex." }
                if ($extraId -eq 1) {
                    if ([BitConverter]::ToUInt32($bytes, $central + 24) -eq [uint32]::MaxValue) { $field += 8 }
                    if ([BitConverter]::ToUInt32($bytes, $central + 20) -eq [uint32]::MaxValue) { $field += 8 }
                    if ($field + 8 -gt $fieldEnd) {
                        throw "ZIP64 local-header offset is missing at entry $entryIndex."
                    }
                    $local64 = [BitConverter]::ToUInt64($bytes, $field)
                    break
                }
                $extra = $fieldEnd
            }
            if ($null -eq $local64 -or $local64 -gt [int]::MaxValue) {
                throw "ZIP64 local-header offset is invalid at entry $entryIndex."
            }
            $local = [int]$local64
        }
        else {
            $local = [int]$local32
        }
        if ($local + 30 -gt $bytes.Length -or
            [BitConverter]::ToUInt32($bytes, $local) -ne 0x04034b50) {
            throw "Malformed local header at entry $entryIndex."
        }
        foreach ($timeOffset in @(($central + 12), ($local + 10))) {
            $bytes[$timeOffset] = 0
            $bytes[$timeOffset + 1] = 0
            $bytes[$timeOffset + 2] = $dosDate -band 0xff
            $bytes[$timeOffset + 3] = ($dosDate -shr 8) -band 0xff
        }
        $central = $entryEnd
    }
    if ($central -ne $centralStart + $centralSize) {
        throw 'Central-directory size does not match parsed entries.'
    }
    $normalizedPath = "$Path.normalizing"
    $backupPath = "$Path.pre-normalization"
    try {
        if ([IO.File]::Exists($normalizedPath)) { [IO.File]::Delete($normalizedPath) }
        if ([IO.File]::Exists($backupPath)) { [IO.File]::Delete($backupPath) }
        [IO.File]::WriteAllBytes($normalizedPath, $bytes)
        if (([IO.FileInfo]::new($normalizedPath)).Length -ne $bytes.Length) {
            throw 'Normalized package length does not match the source package.'
        }
        [IO.File]::Replace($normalizedPath, $Path, $backupPath)
        [IO.File]::Delete($backupPath)
    }
    finally {
        if ([IO.File]::Exists($normalizedPath)) { [IO.File]::Delete($normalizedPath) }
    }
}

Normalize-AppxTimestamps $OutputPath

$archive = [IO.Compression.ZipFile]::OpenRead($OutputPath)
try {
    if ($archive.Entries.FullName -contains 'AppxSignature.p7x') {
        throw 'Developer package unexpectedly contains a signature.'
    }
    foreach ($required in @('AppxManifest.xml', 'AppxBlockMap.xml', 'aeDaeApp.exe',
            'Assets/StoreLogo.png', 'Assets/Square150x150Logo.png', 'Assets/Square44x44Logo.png')) {
        if ($archive.Entries.FullName -notcontains $required) { throw "Package is missing $required" }
    }
}
finally {
    $archive.Dispose()
}

$hash = (Get-FileHash -LiteralPath $OutputPath -Algorithm SHA256).Hash.ToLowerInvariant()
$hashPath = "$OutputPath.sha256"
[IO.File]::WriteAllText($hashPath, "$hash *$([IO.Path]::GetFileName($OutputPath))" + [Environment]::NewLine,
    [Text.UTF8Encoding]::new($false))
Write-Host "Unsigned developer package: $OutputPath"
Write-Host "SHA-256: $hash"
Write-Host "MakeAppx: $($makeappx.FullName) ($($makeappx.VersionInfo.FileVersionRaw))"
Write-Host 'Signing and publishing were not attempted.'
