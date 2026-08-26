#Requires -Version 5.1
<#
.SYNOPSIS
    Build the RXDK-Libs redistributable library distribution.

.DESCRIPTION
    Builds every shippable library twice -- once Debug, once ReleaseSmall -- via
    scripts\compile.ps1 (zig build), stages the .lib files into dist\lib\debug and
    dist\lib\release, archives the libcompat.lib comdat-fix objects, and copies the
    public headers into dist\include. dist\ is gitignored.

.PARAMETER Clean
    Remove the zig build cache + generated outputs first, forcing a full recompile
    (guaranteed-fresh build with no stale object leaking through).

.EXAMPLE
    .\build.ps1
        Build the dist (Debug + ReleaseSmall) into dist\lib\{debug,release} + dist\include.

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

# Remove the zig build cache AND all generated outputs so the next build recompiles
# every source from scratch -- the safe hammer that guarantees no stale object (e.g.
# an edited libxapi source that didn't get recompiled) leaks into the dist.
function Invoke-Clean {
    Write-Host ''
    Write-Host '==> clean: removing zig cache + generated outputs (forces full recompile)' -ForegroundColor Cyan
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

# libcompat.lib force-links picolibc's own fabs/sqrt/sin/... and the MSVC lldiv
# shim so an external `zig cc` title link can't lose a COMDAT tie-break to zig's
# bundled compiler-rt (whose ABI-mismatched fabs corrupts the x87 stack, etc.).
# memcpy/memmove are deliberately NOT here: picolibc's x86 asm versions are STRONG
# globals in libc.lib, so they already override compiler-rt's weak SSE2 memmove
# (which the real Pentium III can't execute) -- adding them to this whole-archive
# lib would duplicate a strong symbol and break the link. RXDK-Libs' own build
# passes these objects loose; this archive exists for external SDK-based title
# builds. Both object sets live under fixed zig-out\obj paths that the NEXT
# -Optimize build overwrites in place, so this must run once per variant, right
# after that variant's `compile.ps1 -Target libs` and before switching to the next.
function Copy-DistCompatLib {
    param(
        [Parameter(Mandatory)] [string]$DistLib,
        [Parameter(Mandatory)] [string]$Variant
    )
    $comdatFixObjs = @{
        'fabs.o'     = 'vendor_picolibc_libm_math_s_fabs_c.o'
        'fabsf.o'    = 'vendor_picolibc_libm_math_sf_fabs_c.o'
        'sqrt.o'     = 'vendor_picolibc_libm_math_s_sqrt_c.o'
        'sqrtf.o'    = 'vendor_picolibc_libm_math_sf_sqrt_c.o'
        'floor.o'    = 'vendor_picolibc_libm_math_s_floor_c.o'
        'floorf.o'   = 'vendor_picolibc_libm_math_sf_floor_c.o'
        'ceil.o'     = 'vendor_picolibc_libm_math_s_ceil_c.o'
        'ceilf.o'    = 'vendor_picolibc_libm_math_sf_ceil_c.o'
        'round.o'    = 'vendor_picolibc_libm_common_s_round_c.o'
        'roundf.o'   = 'vendor_picolibc_libm_common_sf_round_c.o'
        'trunc.o'    = 'vendor_picolibc_libm_common_s_trunc_c.o'
        'truncf.o'   = 'vendor_picolibc_libm_common_sf_trunc_c.o'
        'fmod.o'     = 'vendor_picolibc_libm_math_s_fmod_c.o'
        'fmodf.o'    = 'vendor_picolibc_libm_math_sf_fmod_c.o'
        'fmax.o'     = 'vendor_picolibc_libm_common_s_fmax_c.o'
        'fmaxf.o'    = 'vendor_picolibc_libm_common_sf_fmax_c.o'
        'fmin.o'     = 'vendor_picolibc_libm_common_s_fmin_c.o'
        'fminf.o'    = 'vendor_picolibc_libm_common_sf_fmin_c.o'
        'exp.o'      = 'vendor_picolibc_libm_math_s_exp_c.o'
        'expf.o'     = 'vendor_picolibc_libm_math_sf_exp_c.o'
        'log.o'      = 'vendor_picolibc_libm_math_s_log_c.o'
        'logf.o'     = 'vendor_picolibc_libm_math_sf_log_c.o'
        'tan.o'      = 'vendor_picolibc_libm_math_s_tan_c.o'
        'tanf.o'     = 'vendor_picolibc_libm_math_sf_tan_c.o'
        'cos.o'      = 'vendor_picolibc_libm_math_s_cos_c.o'
        'cosf.o'     = 'vendor_picolibc_libm_math_sf_cos_c.o'
        'sin.o'      = 'vendor_picolibc_libm_math_s_sin_c.o'
        'sinf.o'     = 'vendor_picolibc_libm_math_sf_sin_c.o'
        'rem_pio2.o'  = 'vendor_picolibc_libm_math_s_rem_pio2_c.o'
        'rem_pio2f.o' = 'vendor_picolibc_libm_math_sf_rem_pio2_c.o'
    }
    $comdatFixSrcs = @()
    foreach ($destName in $comdatFixObjs.Keys) {
        $src = Join-Path $root ('zig-out\obj\picolibc\{0}' -f $comdatFixObjs[$destName])
        if (Test-Path -LiteralPath $src) {
            $comdatFixSrcs += $src
        }
        else {
            Write-Warning "expected object not found: zig-out\obj\picolibc\$($comdatFixObjs[$destName]) ($Variant)"
        }
    }

    $msvcLldivObj = Join-Path $root 'zig-out\obj\compat\libs_libxapi_port_msvc_lldiv_c.o'
    if (Test-Path -LiteralPath $msvcLldivObj) {
        $comdatFixSrcs += $msvcLldivObj
    }
    else {
        Write-Warning "expected object not found: zig-out\obj\compat\libs_libxapi_port_msvc_lldiv_c.o ($Variant)"
    }

    $comdatFixLib = Join-Path $DistLib 'libcompat.lib'
    if (Test-Path -LiteralPath $comdatFixLib) { Remove-Item -LiteralPath $comdatFixLib -Force }
    & zig ar rcs $comdatFixLib @comdatFixSrcs
    if ($LASTEXITCODE -ne 0) { throw "Archiving $comdatFixLib failed (exit $LASTEXITCODE)" }
    Write-Host ('OK  {0}  libcompat.lib ({1} objs)' -f $DistLib, $comdatFixSrcs.Count) -ForegroundColor Green
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
        'libuix.lib'
    )

    # Two variants side by side: Debug (full symbols, -O0) and ReleaseSmall. Each
    # variant's libs + libcompat.lib must be copied/archived out of zig-out BEFORE
    # building the next, since zig-out\lib and zig-out\obj are fixed paths the next
    # -Optimize build overwrites in place.
    $variants = @(
        @{ Optimize = 'Debug'; Dir = 'debug' }
        @{ Optimize = 'ReleaseSmall'; Dir = 'release' }
    )
    foreach ($variant in $variants) {
        Write-Host ('==> {0}' -f $variant.Optimize) -ForegroundColor Cyan
        & $compile -Target libs -Optimize $variant.Optimize

        $distLib = Join-Path $distLibRoot $variant.Dir
        New-Item -ItemType Directory -Force -Path $distLib | Out-Null

        $copied = @()
        foreach ($name in $shipLibs) {
            $src = Join-Path $root ('zig-out\lib\{0}' -f $name)
            if (Test-Path -LiteralPath $src) {
                Copy-Item -LiteralPath $src -Destination $distLib -Force
                $copied += $name
            }
            else {
                Write-Warning "expected lib not found: zig-out\lib\$name ($($variant.Optimize))"
            }
        }

        Copy-DistCompatLib -DistLib $distLib -Variant $variant.Optimize

        Write-Host ('OK  dist\lib\{0}  {1} libs: {2}' -f $variant.Dir, $copied.Count, ($copied -join ', ')) -ForegroundColor Green
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
