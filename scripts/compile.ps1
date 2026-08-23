# Build RXDK-Libs (Zig-only runtime, samples, optional XBE conversion).
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet(
        'all', 'libs', 'samples', 'verify-no-vs',
        'libc-smoke', 'libcpp-smoke', 'xapi-smoke', 'xapi-input', 'd3d8-triangle', 'd3dmath-smoke', 'd3d8-textures', 'dsound-music', 'xnet-net', 'xmv-play', 'xfont-smoke', 'dxt-fps'
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
        # 128 KiB (XDK default 64 KiB): generous headroom only. The earlier
        # "DmEnetFunc overflows the stack" rationale was wrong; real fix was the
        # CXbdmClient bridge. Can likely return to 65536 after HW validation.
        [int]$StackSize = 131072
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
        # Stage the sample's media\ (if any) into the image so D:\media\ assets
        # (dsound-music's OGG, d3d8-textures' bitmaps) load from a self-contained
        # ISO -- matching what a deploy xbcp's to the console.
        $mediaDir = Join-Path $Root "samples\$SampleName\media"
        & (Join-Path $PSScriptRoot 'Invoke-XbeIsoBuild.ps1') -InputXbe $xbe -MediaDir $mediaDir
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
    'libc-smoke', 'libcpp-smoke', 'xapi-smoke', 'xapi-input', 'd3d8-triangle', 'd3dmath-smoke', 'd3d8-textures', 'dsound-music', 'xnet-net', 'xmv-play', 'xfont-smoke'
)

switch ($Target) {
    'verify-no-vs' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
    }
    'libs' {
        # Default install stages libc/libcpp/libxapi (+ their public headers);
        # the device libs have their own named steps -> build them all so "libs"
        # means every shippable .lib.
        Invoke-ZigBuild -Step @() -Opt $Optimize
        Invoke-ZigBuild -Step @('libd3d8', 'libd3d8i', 'libd3dx8', 'libxgraphics', 'libdsound', 'libxnet', 'libxneto', 'libxmv') -Opt $Optimize
        # Middleware: XACT audio, the Xbox Live client (+ the UIX drop-in UI),
        # XHV voice and DirectMusic. Titles opt in by naming them in
        # <RxdkLibraries>, but they must still ship in the SDK -- the imported
        # XDK sample suite links them.
        Invoke-ZigBuild -Step @('libxact', 'libxonline', 'libuix', 'libxvoice', 'libdmusic') -Opt $Optimize
    }
    'samples' {
        Build-AllSamples -Opt $Optimize -Xbe:$Xbe -Iso:$Iso
    }
    'dxt-fps' {
        # A DXT (debug-monitor extension) is a raw PE, not an XBE: no XBE wrap, no
        # ISO. Build zig-out\samples\dxt-fps\dxt-fps.dxt, then patch its PE
        # subsystem to Xbox(14) via imagebld /DXT so it matches a retail .dxt.
        # xbdm ignores the subsystem, so an older imagebld without /DXT is a
        # non-fatal warning. Deploy is a copy to the console's E:\dxt + a warm
        # reboot (handled by the VS Code flow).
        Invoke-ZigBuild -Step @('dxt-fps') -Opt $Optimize
        $dxt = Join-Path $Root 'zig-out\samples\dxt-fps\dxt-fps.dxt'
        if (Test-Path -LiteralPath $dxt) {
            try {
                & (Join-Path $PSScriptRoot 'Invoke-DxtPatch.ps1') -InputDxt $dxt
            }
            catch {
                Write-Warning ("imagebld /DXT subsystem patch skipped: {0}" -f $_.Exception.Message)
            }
        }
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
