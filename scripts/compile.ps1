# Build RXDK-LibsZig (Zig-only runtime, samples, optional XBE conversion).
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet(
        'all', 'libs', 'samples', 'verify-no-vs',
        'libc-smoke', 'libcpp-smoke', 'xapi-smoke', 'xapi-input', 'd3d8-triangle', 'd3d8-textures', 'dsound-music'
    )]
    [string]$Target = 'all',
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize = 'Debug',
    [switch]$Xbe,
    [switch]$Iso,
    [switch]$Deploy,
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
        [switch]$Deploy,
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
    # A deployed XBE (run from the kit's E:) needs the same boot init flags as a
    # boot disc -- INITFLAGS:24 /DONTMOUNTUD /DONTMODIFYHD (NO_SETUP_HARD_DISK).
    # Without them the title tries to set up the utility drive and never reaches
    # its app thread under xbox-launch.
    $bootDisc = $Iso -or $Deploy
    $xbe = & (Join-Path $PSScriptRoot 'Invoke-ImageBuild.ps1') -InputExe $pe -XbeDebug -NoLibWarn -BootDisc:$bootDisc -MountHdd:$MountHdd -FormatHdd:$FormatHdd -MaxImportThunks $MaxImportThunks -StackSize $StackSize
    if ($Iso) {
        & (Join-Path $PSScriptRoot 'Invoke-XbeIsoBuild.ps1') -InputXbe $xbe
    }
}

if (($Iso -or $Deploy) -and -not $Xbe) {
    $Xbe = $true
}

Require-Command -Name 'zig'
if (-not $SkipSubmoduleCheck) {
    Require-Submodules -RepoRoot $Root
}

function Build-Sample {
    param(
        [string]$Target,
        [string]$Opt,
        [switch]$Xbe,
        [switch]$Iso,
        [switch]$Deploy,
        [switch]$NoHdd,
        [switch]$FormatHdd
    )
    # Build step, artifact, and PE name are all $Target now.
    Invoke-ZigBuild -Step @('verify-no-vs', $Target) -Opt $Opt
    if ($Xbe) {
        $mountHdd = ($Target -eq 'xapi-smoke') -and -not $NoHdd
        $formatHdd = $mountHdd -and (($Target -eq 'xapi-smoke' -and $Iso) -or $FormatHdd)
        Convert-SampleXbe -SampleName $Target -Iso:$Iso -Deploy:$Deploy -MountHdd:$mountHdd -FormatHdd:$formatHdd
    }
}

function Build-AllSamples {
    param(
        [string]$Opt,
        [switch]$Xbe,
        [switch]$Iso
    )
    Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Opt
    foreach ($name in @('libc-smoke', 'libcpp-smoke', 'xapi-smoke')) {
        Invoke-ZigBuild -Step @($name) -Opt $Opt
        if ($Xbe) {
            Build-Sample -Target $name -Opt $Opt -Xbe:$Xbe -Iso:$Iso
        }
    }
}

$singleSampleTargets = @(
    'libc-smoke', 'libcpp-smoke', 'xapi-smoke', 'xapi-input', 'd3d8-triangle', 'd3d8-textures', 'dsound-music'
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
        Build-Sample -Target $Target -Opt $Optimize -Xbe:$Xbe -Iso:$Iso -Deploy:$Deploy -NoHdd:$NoHdd -FormatHdd:$FormatHdd
    }
    'all' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
        Invoke-ZigBuild -Step @() -Opt $Optimize
        Build-AllSamples -Opt $Optimize -Xbe:$Xbe -Iso:$Iso
    }
}

Write-Host "Done ($Target)." -ForegroundColor Green
