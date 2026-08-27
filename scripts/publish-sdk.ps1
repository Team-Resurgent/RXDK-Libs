#Requires -Version 5.1
<#
.SYNOPSIS
    Publish the built RXDK-Libs distribution into the RXDK-SDK submodule and push it.

.DESCRIPTION
    The consumer SDK (headers + static libs) lives in its own repo, RXDK-SDK,
    vendored here as the `sdk/` submodule. This script takes the dist that
    build.ps1 produced (dist\include, dist\lib) and publishes it:

        1. fast-forwards the sdk\ submodule to the tip of its default branch
           (so the release is always cut from the latest head),
        2. mirrors dist\include -> sdk\include and dist\lib -> sdk\lib
           (replacing, so files deleted from the dist do not linger),
        3. stamps sdk\VERSION with the release version,
        4. commits "Release <version>" in the submodule and pushes it,
        5. stages the updated gitlink in RXDK-Libs so the parent records the
           exact SDK commit this build produced.

    intellisense\ in the SDK is left untouched -- it is maintained separately.

.PARAMETER Version
    Release version string written to sdk\VERSION, e.g. v1.1.5.

.PARAMETER Build
    Run build.ps1 -Clean first to guarantee a fresh dist before publishing.

.PARAMETER NoPush
    Do everything except the submodule 'git push' (local dry run).

.EXAMPLE
    .\scripts\publish-sdk.ps1 -Version v1.1.5
        Publish the current dist\ as SDK v1.1.5 and push it.

.EXAMPLE
    .\scripts\publish-sdk.ps1 -Version v1.1.5 -Build
        Rebuild the dist from scratch, then publish it as v1.1.5.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Version,
    [switch]$Build,
    [switch]$NoPush
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot   # scripts\ -> repo root
$dist = Join-Path $root 'dist'
$sdk  = Join-Path $root 'sdk'

function Invoke-Git {
    param([string]$Dir, [Parameter(ValueFromRemainingArguments = $true)][string[]]$GitArgs)
    # Capture both streams into a variable: git writes normal progress ("From
    # https://...", "Everything up-to-date") to stderr, and in PowerShell 5.1 an
    # uncaptured native stderr line becomes a terminating error under
    # ErrorActionPreference=Stop. Capturing defuses that; we gate on the real exit
    # code, not $?.
    # ErrorActionPreference=Stop (set script-scope) turns any native stderr line
    # into a terminating error even when captured, so drop to Continue just for the
    # git call and gate on the exit code instead.
    $eap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & git -C $Dir @GitArgs 2>&1
    } finally {
        $ErrorActionPreference = $eap
    }
    if ($LASTEXITCODE -ne 0) {
        throw "git $($GitArgs -join ' ') failed in $Dir ($LASTEXITCODE):`n$(( $out | ForEach-Object { "$_" } ) -join "`n")"
    }
    $out | ForEach-Object { Write-Host "$_" }
}

if ($Build) {
    Write-Host "==> building dist first (build.ps1 -Clean)"
    & (Join-Path $root 'build.ps1') -Clean
    if ($LASTEXITCODE -ne 0) { throw "build.ps1 failed ($LASTEXITCODE)" }
}

if (-not (Test-Path (Join-Path $dist 'include')) -or -not (Test-Path (Join-Path $dist 'lib'))) {
    throw "dist\ is missing or incomplete at $dist -- run .\build.ps1 first (or pass -Build)."
}
if (-not (Test-Path (Join-Path $sdk '.git'))) {
    throw "sdk\ submodule is not initialized -- run: git submodule update --init sdk"
}

# 1. Fast-forward the submodule to the tip of its default branch (latest head).
Write-Host "==> updating sdk\ submodule to latest head"
$eapSaved = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
$remoteShow = (& git -C $sdk remote show origin 2>&1 | ForEach-Object { "$_" })
$ErrorActionPreference = $eapSaved
$headLine = $remoteShow | Select-String 'HEAD branch:' | Select-Object -First 1
$defaultBranch = if ($headLine) { $headLine.ToString().Split(':')[-1].Trim() } else { 'main' }
if (-not $defaultBranch) { $defaultBranch = 'main' }
Invoke-Git $sdk fetch origin $defaultBranch
Invoke-Git $sdk checkout $defaultBranch
Invoke-Git $sdk pull --ff-only origin $defaultBranch

# 2. Mirror the dist include/ and lib/ into the submodule.
Write-Host "==> copying dist -> sdk (include, lib)"
foreach ($sub in @('include', 'lib')) {
    $src = Join-Path $dist $sub
    $dst = Join-Path $sdk  $sub
    if (Test-Path $dst) { Remove-Item -Recurse -Force $dst }
    Copy-Item -Recurse -Force $src $dst
}

# 3. Stamp VERSION.
Write-Host "==> stamping sdk\VERSION = $Version"
Set-Content -Path (Join-Path $sdk 'VERSION') -Value $Version -Encoding ascii

# 4. Commit + push the submodule.
Invoke-Git $sdk add -A
$dirty = (& git -C $sdk status --porcelain)
if ($dirty) {
    Invoke-Git $sdk commit -m "Release $Version"
    if ($NoPush) {
        Write-Host "==> -NoPush: committed sdk locally, not pushed"
    } else {
        Write-Host "==> pushing sdk to origin/$defaultBranch"
        Invoke-Git $sdk push origin "HEAD:$defaultBranch"
    }
} else {
    Write-Host "==> sdk: no changes to commit (dist identical to published SDK)"
}

# 5. Stage the updated gitlink in the parent so RXDK-Libs records this SDK commit.
Invoke-Git $root add sdk
Write-Host ""
Write-Host "Published RXDK-SDK $Version."
Write-Host "The updated sdk gitlink is staged in RXDK-Libs -- commit + push the parent to record it."
