[CmdletBinding(SupportsShouldProcess = $true, DefaultParameterSetName = "Prepare")]
param(
    [Parameter(Mandatory = $true, ParameterSetName = "Prepare")]
    [ValidateNotNullOrEmpty()]
    [string]$SourceDir,

    [Parameter(Mandatory = $true, ParameterSetName = "Prepare")]
    [ValidateNotNullOrEmpty()]
    [string]$Ipa,

    [Parameter(Mandatory = $true, ParameterSetName = "Prepare")]
    [ValidateNotNullOrEmpty()]
    [string]$OutputDir,

    [Parameter(ParameterSetName = "Prepare")]
    [switch]$Force,

    [Parameter(Mandatory = $true, ParameterSetName = "SelfTest")]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ExpectedIpaSize = 8512047
$ExpectedIpaSha256 = "EF264BA977DC70586AA622B89A40528096B600A17732685E75A032D7AAB6FF2E"

function Get-NormalizedFullPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.Path]::GetFullPath($Path).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
}

function Test-ExcludedSourcePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$LeafName
    )

    $normalized = $RelativePath -replace "\\", "/"
    $segments = @($normalized -split "/")
    $excludedDirectories = @(
        "Debug",
        "Saved Games",
        "FinalSun",
        "Manuals",
        "HTML",
        "Internet",
        "images"
    )

    if ($segments.Count -gt 1) {
        foreach ($segment in $segments[0..($segments.Count - 2)]) {
            if ($excludedDirectories -contains $segment) {
                return $true
            }
        }
    }

    $extension = [System.IO.Path]::GetExtension($LeafName)
    if ($extension -match "(?i)^\.(exe|dll|dylib|pdb|map)$") {
        return $true
    }

    if ($LeafName -match "(?i)^(Game|GameD|SUN\.INI)$") {
        return $true
    }

    return $false
}

function Assert-SafeOutputPath {
    param(
        [Parameter(Mandatory = $true)][string]$SourceFull,
        [Parameter(Mandatory = $true)][string]$OutputFull
    )

    $root = [System.IO.Path]::GetPathRoot($OutputFull).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )

    if ($OutputFull -eq $root) {
        throw "Refusing to use a filesystem root as OutputDir: $OutputFull"
    }

    if ($OutputFull -eq $SourceFull) {
        throw "OutputDir must not be the source game directory."
    }

    $sourcePrefix = $SourceFull + [System.IO.Path]::DirectorySeparatorChar
    if ($OutputFull.StartsWith($sourcePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "OutputDir must not be inside the source game directory."
    }
}

function Invoke-Prepare {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDirValue,
        [Parameter(Mandatory = $true)][string]$IpaValue,
        [Parameter(Mandatory = $true)][string]$OutputDirValue,
        [switch]$ForceValue,
        [switch]$SkipReleaseHashValidation
    )

    if (-not (Test-Path -LiteralPath $SourceDirValue -PathType Container)) {
        throw "SourceDir does not exist or is not a directory: $SourceDirValue"
    }
    if (-not (Test-Path -LiteralPath $IpaValue -PathType Leaf)) {
        throw "IPA does not exist: $IpaValue"
    }

    $sourceFull = (Resolve-Path -LiteralPath $SourceDirValue).Path.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
    $ipaFile = Get-Item -LiteralPath $IpaValue
    $outputFull = Get-NormalizedFullPath -Path $OutputDirValue

    Assert-SafeOutputPath -SourceFull $sourceFull -OutputFull $outputFull

    if (-not $SkipReleaseHashValidation) {
        if ($ipaFile.Length -ne $ExpectedIpaSize) {
            throw "IPA size mismatch. Expected $ExpectedIpaSize bytes, got $($ipaFile.Length)."
        }

        $ipaHash = (Get-FileHash -LiteralPath $ipaFile.FullName -Algorithm SHA256).Hash.ToUpperInvariant()
        if ($ipaHash -ne $ExpectedIpaSha256) {
            throw "IPA SHA-256 mismatch. Expected $ExpectedIpaSha256, got $ipaHash."
        }
    }

    $temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("cnc-ts-ios-" + [guid]::NewGuid().ToString("N"))
    $zipPath = Join-Path $temporaryRoot "OpenTS.zip"
    $expandedPath = Join-Path $temporaryRoot "expanded"

    try {
        New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
        Copy-Item -LiteralPath $ipaFile.FullName -Destination $zipPath
        Expand-Archive -LiteralPath $zipPath -DestinationPath $expandedPath

        $appRoot = Join-Path $expandedPath "Payload\OpenTS.app"
        $appUi = Join-Path $appRoot "ui"
        $appLanguage = Join-Path $appRoot "Language.dat"

        if (-not (Test-Path -LiteralPath $appRoot -PathType Container)) {
            throw "IPA does not contain Payload/OpenTS.app."
        }
        if (-not (Test-Path -LiteralPath $appUi -PathType Container)) {
            throw "IPA does not contain Payload/OpenTS.app/ui."
        }
        if (-not (Test-Path -LiteralPath $appLanguage -PathType Leaf)) {
            throw "IPA does not contain Payload/OpenTS.app/Language.dat."
        }

        if (-not $PSCmdlet.ShouldProcess($outputFull, "Create transfer-ready CnC TS OpenTS data folder")) {
            Write-Host "Validated inputs. No output written because -WhatIf was used."
            return
        }

        if (Test-Path -LiteralPath $outputFull) {
            $existingItems = @(Get-ChildItem -LiteralPath $outputFull -Force -ErrorAction Stop)
            if ($existingItems.Count -gt 0 -and -not $ForceValue) {
                throw "OutputDir already exists and is not empty. Choose another path or pass -Force."
            }
            if ($ForceValue) {
                Remove-Item -LiteralPath $outputFull -Recurse -Force
            }
        }

        $openTsRoot = Join-Path $outputFull "OpenTS"
        $dataRoot = Join-Path $openTsRoot "Data"
        $userRoot = Join-Path $openTsRoot "User"

        New-Item -ItemType Directory -Path $dataRoot -Force | Out-Null
        New-Item -ItemType Directory -Path $userRoot -Force | Out-Null

        $sourcePrefix = $sourceFull + [System.IO.Path]::DirectorySeparatorChar
        $copiedOwnerFiles = 0

        foreach ($file in Get-ChildItem -LiteralPath $sourceFull -Recurse -File -Force) {
            if (-not $file.FullName.StartsWith($sourcePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                throw "Source traversal escaped SourceDir: $($file.FullName)"
            }

            $relative = $file.FullName.Substring($sourcePrefix.Length)
            if (Test-ExcludedSourcePath -RelativePath $relative -LeafName $file.Name) {
                continue
            }

            $destination = Join-Path $dataRoot $relative
            $destinationParent = Split-Path -Parent $destination
            New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
            $copiedOwnerFiles++
        }

        $destinationUi = Join-Path $dataRoot "ui"
        if (Test-Path -LiteralPath $destinationUi) {
            Remove-Item -LiteralPath $destinationUi -Recurse -Force
        }
        Copy-Item -LiteralPath $appUi -Destination $destinationUi -Recurse -Force
        Copy-Item -LiteralPath $appLanguage -Destination (Join-Path $dataRoot "Language.dat") -Force

        $dataFiles = @(Get-ChildItem -LiteralPath $dataRoot -Recurse -File -Force)
        $totalBytes = ($dataFiles | Measure-Object -Property Length -Sum).Sum
        if ($null -eq $totalBytes) {
            $totalBytes = 0
        }

        Write-Host ""
        Write-Host "CnC TS iOS data preparation complete."
        Write-Host "Output:       $openTsRoot"
        Write-Host "Game data:    $dataRoot"
        Write-Host "User state:   $userRoot"
        Write-Host "Owner files:  $copiedOwnerFiles copied from the source installation"
        Write-Host "Data files:   $($dataFiles.Count) total"
        Write-Host "Data bytes:   $totalBytes"
        Write-Host ""
        Write-Host "Copy the generated OpenTS folder into the CnC TS app's Files-visible Documents area."
        Write-Host "The final on-device paths must be Documents/OpenTS/Data and Documents/OpenTS/User."
    }
    finally {
        if (Test-Path -LiteralPath $temporaryRoot) {
            Remove-Item -LiteralPath $temporaryRoot -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

function Invoke-SelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("cnc-ts-ios-selftest-" + [guid]::NewGuid().ToString("N"))
    $source = Join-Path $root "source"
    $ipaTree = Join-Path $root "ipa-tree"
    $output = Join-Path $root "output"
    $fakeIpa = Join-Path $root "fixture.ipa"
    $fakeZip = Join-Path $root "fixture.zip"

    try {
        New-Item -ItemType Directory -Path $source -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $source "Manuals") -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $source "sub") -Force | Out-Null

        Set-Content -LiteralPath (Join-Path $source "TIBSUN.MIX") -Value "synthetic-runtime-data"
        Set-Content -LiteralPath (Join-Path $source "MOVIES01.MIX") -Value "synthetic-movie-data"
        Set-Content -LiteralPath (Join-Path $source "game.exe") -Value "exclude-me"
        Set-Content -LiteralPath (Join-Path $source "SUN.INI") -Value "exclude-me"
        Set-Content -LiteralPath (Join-Path $source "Manuals\manual.txt") -Value "exclude-me"
        Set-Content -LiteralPath (Join-Path $source "sub\keep.dat") -Value "keep-me"

        $appRoot = Join-Path $ipaTree "Payload\OpenTS.app"
        $appUi = Join-Path $appRoot "ui"
        New-Item -ItemType Directory -Path $appUi -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $appUi "mainmenu.rml") -Value "synthetic-engine-ui"
        Set-Content -LiteralPath (Join-Path $appRoot "Language.dat") -Value "synthetic-language"

        Compress-Archive -Path (Join-Path $ipaTree "*") -DestinationPath $fakeZip
        Move-Item -LiteralPath $fakeZip -Destination $fakeIpa

        Invoke-Prepare -SourceDirValue $source -IpaValue $fakeIpa -OutputDirValue $output -ForceValue -SkipReleaseHashValidation

        $checks = @(
            @{ Path = "OpenTS\Data\TIBSUN.MIX"; ShouldExist = $true },
            @{ Path = "OpenTS\Data\MOVIES01.MIX"; ShouldExist = $true },
            @{ Path = "OpenTS\Data\sub\keep.dat"; ShouldExist = $true },
            @{ Path = "OpenTS\Data\ui\mainmenu.rml"; ShouldExist = $true },
            @{ Path = "OpenTS\Data\Language.dat"; ShouldExist = $true },
            @{ Path = "OpenTS\User"; ShouldExist = $true },
            @{ Path = "OpenTS\Data\game.exe"; ShouldExist = $false },
            @{ Path = "OpenTS\Data\SUN.INI"; ShouldExist = $false },
            @{ Path = "OpenTS\Data\Manuals\manual.txt"; ShouldExist = $false }
        )

        foreach ($check in $checks) {
            $exists = Test-Path -LiteralPath (Join-Path $output $check.Path)
            if ($exists -ne $check.ShouldExist) {
                throw "Self-test failed for $($check.Path). Expected existence=$($check.ShouldExist), actual=$exists."
            }
        }

        Write-Host "SELF-TEST PASS"
    }
    finally {
        if (Test-Path -LiteralPath $root) {
            Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

if ($SelfTest) {
    Invoke-SelfTest
    exit 0
}

Invoke-Prepare -SourceDirValue $SourceDir -IpaValue $Ipa -OutputDirValue $OutputDir -ForceValue:$Force
