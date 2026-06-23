# Build RXDK-LibsZig (Zig-only runtime, samples, optional XBE conversion).
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet('all', 'libs', 'samples', 'kernel-smoke', 'hello-c', 'hello-cpp', 'conformance-c23', 'conformance-cpp23', 'verify-no-vs')]
    [string]$Target = 'all',
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize = 'Debug',
    [switch]$Xbe,
    [switch]$Iso,
    [switch]$SkipSubmoduleCheck
)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
Set-Location -LiteralPath $Root

function Require-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name not found on PATH. Install Zig 0.16+ from https://ziglang.org/download/"
    }
}

function Require-Submodules {
    param([string]$RepoRoot)
    $required = @(
        (Join-Path $RepoRoot 'vendor\picolibc\libc\include\stdio.h')
        (Join-Path $RepoRoot 'vendor\llvm-project\libcxx\include\iostream')
    )
    foreach ($path in $required) {
        if (-not (Test-Path -LiteralPath $path)) {
            throw @"
Missing vendor sources: $path
Run: git submodule update --init --recursive
"@
        }
    }
}

function Invoke-ZigBuild {
    param(
        [string[]]$Step,
        [string]$Opt
    )
    $args = @('build', "-Doptimize=$Opt")
    if ($Step) {
        $args += $Step
    }
    Write-Host "==> zig $($args -join ' ')" -ForegroundColor Cyan
    & zig @args
    if ($LASTEXITCODE -ne 0) {
        throw "zig build failed (exit $LASTEXITCODE)"
    }
}

function Convert-SampleXbe {
    param(
        [string]$SampleName,
        [switch]$Iso
    )
    $pe = Join-Path $Root "zig-out\samples\$SampleName\$SampleName.exe"
    if (-not (Test-Path -LiteralPath $pe)) {
        Write-Warning "Skip XBE: PE not found: $pe"
        return
    }
    $xbe = & (Join-Path $PSScriptRoot 'Invoke-ImageBuild.ps1') -InputExe $pe -XbeDebug -NoLibWarn -BootDisc:$Iso
    if ($Iso) {
        & (Join-Path $PSScriptRoot 'Invoke-XbeIsoBuild.ps1') -InputXbe $xbe
    }
}

if ($Iso -and -not $Xbe) {
    $Xbe = $true
}

Require-Command -Name 'zig'
if (-not $SkipSubmoduleCheck) {
    Require-Submodules -RepoRoot $Root
}

$sampleTargets = @(
    'kernel-smoke'
    'hello-c'
    'hello-cpp'
    'conformance-c23'
    'conformance-cpp23'
)

switch ($Target) {
    'verify-no-vs' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
    }
    'libs' {
        Invoke-ZigBuild -Step @() -Opt $Optimize
    }
    'samples' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
        foreach ($name in $sampleTargets) {
            Invoke-ZigBuild -Step @($name) -Opt $Optimize
            if ($Xbe) { Convert-SampleXbe -SampleName $name -Iso:$Iso }
        }
    }
    { $_ -in $sampleTargets } {
        Invoke-ZigBuild -Step @('verify-no-vs', $Target) -Opt $Optimize
        if ($Xbe) { Convert-SampleXbe -SampleName $Target -Iso:$Iso }
    }
    'all' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
        Invoke-ZigBuild -Step @() -Opt $Optimize
        foreach ($name in $sampleTargets) {
            Invoke-ZigBuild -Step @($name) -Opt $Optimize
            if ($Xbe) { Convert-SampleXbe -SampleName $name -Iso:$Iso }
        }
    }
}

Write-Host "Done ($Target)." -ForegroundColor Green
