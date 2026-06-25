# Build RXDK-LibsZig (Zig-only runtime, samples, optional XBE conversion).
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet(
        'all', 'libs', 'samples', 'verify-no-vs',
        'kernel-smoke', 'kernel-api-smoke', 'kernel-api-link',
        'kernel-api-probe', 'kernel-api-probe-debug',
        'hello-c', 'hello-cpp',
        'conformance-c', 'conformance-c23', 'conformance-cpp23',
        'c23-stdbit-smoke', 'cpp23-expected-smoke',
        'xapi-smoke', 'xapi-link'
    )]
    [string]$Target = 'all',
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize = 'Debug',
    [switch]$Xbe,
    [switch]$Iso,
    [switch]$NoHdd,
    [switch]$FormatHdd,
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
        (Join-Path $RepoRoot 'vendor\xbox_leak_may_2020\xbox_leak_may_2020\xbox trunk\xbox\private\ntos\init\console\obj\i386\xboxkrnl.def')
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
        [switch]$Iso,
        [switch]$MountHdd,
        [switch]$FormatHdd,
        [int]$MaxImportThunks = 0,
        [int]$StackSize = 65536
    )
    $pe = Join-Path $Root "zig-out\samples\$SampleName\$SampleName.exe"
    if (-not (Test-Path -LiteralPath $pe)) {
        Write-Warning "Skip XBE: PE not found: $pe"
        return
    }
    $xbe = & (Join-Path $PSScriptRoot 'Invoke-ImageBuild.ps1') -InputExe $pe -XbeDebug -NoLibWarn -BootDisc:$Iso -MountHdd:$MountHdd -FormatHdd:$FormatHdd -MaxImportThunks $MaxImportThunks -StackSize $StackSize
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

function Resolve-Sample {
    param([string]$Target)
    switch ($Target) {
        'conformance-c23' { return @{ BuildStep = 'conformance-c23'; Artifact = 'c23-stdbit-smoke' } }
        'c23-stdbit-smoke' { return @{ BuildStep = 'conformance-c23'; Artifact = 'c23-stdbit-smoke' } }
        'conformance-cpp23' { return @{ BuildStep = 'conformance-cpp23'; Artifact = 'cpp23-expected-smoke' } }
        'cpp23-expected-smoke' { return @{ BuildStep = 'conformance-cpp23'; Artifact = 'cpp23-expected-smoke' } }
        default { return @{ BuildStep = $Target; Artifact = $Target } }
    }
}

function Build-Sample {
    param(
        [string]$Target,
        [string]$Opt,
        [switch]$Xbe,
        [switch]$Iso,
        [switch]$NoHdd,
        [switch]$FormatHdd
    )
    if ($Iso -and $Target -eq 'kernel-api-link') {
        throw @"
kernel-api-link is a host-only link test (367 xboxkrnl imports) — do not deploy to kit.
Use: .\scripts\compile.ps1 -Target kernel-api-probe -Iso
"@
    }
    if ($Iso -and $Target -eq 'xapi-link') {
        throw @"
xapi-link is a host-only link smoke — do not deploy to kit.
Use: .\scripts\compile.ps1 -Target xapi-smoke -Iso
"@
    }
    $sample = Resolve-Sample -Target $Target
    Invoke-ZigBuild -Step @('verify-no-vs', $sample.BuildStep) -Opt $Opt
    if ($Xbe) {
        $maxThunks = 0
        $stackSize = 65536
        if ($sample.Artifact -eq 'kernel-api-probe' -or $sample.Artifact -eq 'kernel-api-probe-debug') {
            $stackSize = 1048576
        }
        $mountHdd = ($sample.Artifact -eq 'xapi-smoke') -and -not $NoHdd
        $formatHdd = $mountHdd -and (($sample.Artifact -eq 'xapi-smoke' -and $Iso) -or $FormatHdd)
        Convert-SampleXbe -SampleName $sample.Artifact -Iso:$Iso -MountHdd:$mountHdd -FormatHdd:$formatHdd -MaxImportThunks $maxThunks -StackSize $stackSize
    }
}

function Build-AllSamples {
    param(
        [string]$Opt,
        [switch]$Xbe,
        [switch]$Iso
    )
    Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Opt
    foreach ($name in @('kernel-smoke', 'kernel-api-smoke', 'kernel-api-link', 'kernel-api-probe', 'kernel-api-probe-debug', 'hello-c', 'hello-cpp', 'conformance-c', 'c23-stdbit-smoke', 'cpp23-expected-smoke')) {
        $sample = Resolve-Sample -Target $name
        Invoke-ZigBuild -Step @($sample.BuildStep) -Opt $Opt
        if ($Xbe) {
            Build-Sample -Target $name -Opt $Opt -Xbe:$Xbe -Iso:$Iso
        }
    }
}

$singleSampleTargets = @(
    'kernel-smoke', 'kernel-api-smoke', 'kernel-api-link',
    'kernel-api-probe', 'kernel-api-probe-debug',
    'hello-c', 'hello-cpp',
    'conformance-c', 'conformance-c23', 'conformance-cpp23',
    'c23-stdbit-smoke', 'cpp23-expected-smoke',
    'xapi-smoke', 'xapi-link'
)

switch ($Target) {
    'verify-no-vs' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
    }
    'libs' {
        Invoke-ZigBuild -Step @() -Opt $Optimize
    }
    'samples' {
        Build-AllSamples -Opt $Optimize -Xbe:$Xbe -Iso:$Iso
    }
    { $_ -in $singleSampleTargets } {
        Build-Sample -Target $Target -Opt $Optimize -Xbe:$Xbe -Iso:$Iso -NoHdd:$NoHdd -FormatHdd:$FormatHdd
    }
    'all' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
        Invoke-ZigBuild -Step @() -Opt $Optimize
        Build-AllSamples -Opt $Optimize -Xbe:$Xbe -Iso:$Iso
    }
}

Write-Host "Done ($Target)." -ForegroundColor Green
