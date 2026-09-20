#Requires -Version 5.1
<#
.SYNOPSIS
    Build the RXDK-Libs redistributable library distribution.

.DESCRIPTION
    Builds every shippable library twice -- once Debug, once ReleaseSmall -- via
    scripts\compile.ps1 (the RXDK engine's build-sdk: clang/lld/llvm-lib), and
    stages the .lib files into one flat
    dist\lib\ (XDK-style: Release ships as libd3d8.lib, Debug as libd3d8d.lib --
    same folder, "d" suffix picks the variant), archives the libcompat[d].lib
    comdat-fix objects, and copies the public headers into dist\include. dist\ is
    gitignored.

.PARAMETER Clean
    Remove the generated build outputs first, forcing a full recompile
    (guaranteed-fresh build with no stale object leaking through).

.EXAMPLE
    .\build.ps1
        Build the dist (Debug + ReleaseSmall) into dist\lib (flat, "d"-suffixed Debug) + dist\include.

.EXAMPLE
    .\build.ps1 -Clean
        Clean, then build the dist from scratch.
#>
[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$compile = Join-Path $root 'scripts\compile.ps1'
if (-not (Test-Path -LiteralPath $compile)) {
    throw "scripts\compile.ps1 not found next to this script ($compile)."
}

# Remove all generated outputs so the next build recompiles
# every source from scratch -- the safe hammer that guarantees no stale object (e.g.
# an edited libxapi source that didn't get recompiled) leaks into the dist.
function Invoke-Clean {
    Write-Host ''
    Write-Host '==> clean: removing generated outputs (forces full recompile)' -ForegroundColor Cyan
    $targets = @(
        '.zig-cache',
        'zig-out\obj', 'zig-out\lib', 'zig-out\include',
        'zig-out\samples', 'zig-out\link', 'zig-out\xbe', 'zig-out\iso'
    )
    foreach ($rel in $targets) {
        $p = Join-Path $root $rel
        if (Test-Path -LiteralPath $p) {
            Remove-Item -LiteralPath $p -Recurse -Force
            Write-Host ('  removed  {0}' -f $rel) -ForegroundColor Green
        }
        else {
            Write-Host ('  (absent) {0}' -f $rel) -ForegroundColor DarkGray
        }
    }
}

function Invoke-DistBuild {
    Write-Host ''
    Write-Host '==> building lib distribution (Debug + ReleaseSmall)' -ForegroundColor Cyan

    $distLibRoot = Join-Path $root 'dist\lib'
    $distInc = Join-Path $root 'dist\include'
    foreach ($d in @($distLibRoot, $distInc)) {
        if (Test-Path -LiteralPath $d) { Remove-Item -LiteralPath $d -Recurse -Force }
    }
    New-Item -ItemType Directory -Force -Path $distInc | Out-Null

    # Ship every library by name (zig-out\lib can also hold stale artifacts from
    # earlier builds, so copy an explicit list rather than a wildcard).
    $shipLibs = @(
        'libkernel.lib', 'libxbdm.lib',
        'libc.lib', 'libcpp.lib', 'libxapi.lib',
        'libd3d8.lib', 'libd3dx8.lib', 'libxgraphics.lib',
        # The instrumented D3D (the XDK's d3d8i.lib): same driver with the
        # D3DPERF counters compiled in. A profiling title links it INSTEAD of libd3d8.
        'libd3d8i.lib',
        'libdsound.lib', 'libxmv.lib',
        # The XNet stack in the two variants the retail XDK ships. libxnet is the
        # plain sockets build; libxneto adds ONLINE/QoS/SG and pairs with libxonline.
        'libxnet.lib', 'libxneto.lib',
        # Middleware (opt-in via <RxdkLibraries>, but shipped).
        'libxact.lib', 'libxonline.lib', 'libxvoice.lib', 'libdmusic.lib',
        # The UIX drop-in Live UI (its own uix.lib in 5849; calls into libxonline).
        'libuix.lib',
        # The COMDAT-fix lib (picolibc math + msvc_lldiv), now assembled by the engine's
        # build-sdk (was build.ps1's Copy-DistCompatLib + `zig ar`), shipped like any other lib.
        'libcompat.lib'
    )

    # One flat lib\ dir, XDK-style: Release ships the bare name (libd3d8.lib), Debug
    # ships the same name with a "d" suffix before the extension (libd3d8d.lib) --
    # so a consumer selects the variant via "Additional Dependencies" filename, the
    # same $(Configuration)-conditioned d3d8$(D).lib flow the real XDK used, rather
    # than a separate library search path per config. Each variant's libs +
    # libcompat.lib must be copied/archived out of zig-out BEFORE building the next,
    # since zig-out\lib and zig-out\obj are fixed paths the next -Optimize build
    # overwrites in place.
    New-Item -ItemType Directory -Force -Path $distLibRoot | Out-Null
    $variants = @(
        @{ Optimize = 'Debug'; Suffix = 'd' }
        @{ Optimize = 'ReleaseSmall'; Suffix = '' }
    )
    foreach ($variant in $variants) {
        Write-Host ('==> {0}' -f $variant.Optimize) -ForegroundColor Cyan
        & $compile -Target libs -Optimize $variant.Optimize

        $copied = @()
        foreach ($name in $shipLibs) {
            $src = Join-Path $root ('zig-out\lib\{0}' -f $name)
            if (Test-Path -LiteralPath $src) {
                $destName = $name -replace '\.lib$', ($variant.Suffix + '.lib')
                Copy-Item -LiteralPath $src -Destination (Join-Path $distLibRoot $destName) -Force
                $copied += $destName
            }
            else {
                Write-Warning "expected lib not found: zig-out\lib\$name ($($variant.Optimize))"
            }
        }

        Write-Host ('OK  dist\lib  {0} libs ({1}): {2}' -f $copied.Count, $variant.Optimize, ($copied -join ', ')) -ForegroundColor Green
    }

    # Public headers, in three layers (optimize-independent -- copied once):
    #   1. zig-out\include - the staged libc/libc++/xapi set + xboxkrnl/ subdir.
    #   2. shared\include  - the device-library public headers + the Win32 base.
    #   3. dist-include    - the distribution-only master umbrella (xtl.h) and its
    #                        shims (xdk_compat.h, guiddef.h). Kept OUT of
    #                        shared\include so a public <xtl.h> doesn't shadow
    #                        libs\libxapi\internal\xtl.h in the in-tree library builds.
    $incSources = @(
        (Join-Path $root 'zig-out\include'),
        (Join-Path $root 'shared\include'),
        (Join-Path $root 'dist-include')
    )
    foreach ($incSrc in $incSources) {
        if (Test-Path -LiteralPath $incSrc) {
            Copy-Item -Path (Join-Path $incSrc '*') -Destination $distInc -Recurse -Force
        }
    }

    $hdrCount = @(Get-ChildItem -LiteralPath $distInc -Recurse -File -ErrorAction SilentlyContinue).Count
    Write-Host ''
    Write-Host ('OK  dist\include  {0} headers' -f $hdrCount) -ForegroundColor Green
}

if ($Clean) {
    Invoke-Clean
}
Invoke-DistBuild
