<#
.SYNOPSIS
  Builds the compiler-rt builtins archive (libclang_rt.builtins-i386.a) that the LLVM title
  link needs, from the vendored compiler-rt source.

.DESCRIPTION
  When a title is linked with the RXDK LLVM toolchain (clang + lld) instead of `zig cc`, the
  link is `-nostdlib` and clang provides NO compiler-rt -- unlike `zig cc`, whose driver
  auto-links its bundled compiler-rt. The SDK libraries reference a handful of 64-bit integer
  builtins (__divdi3/__udivdi3/__moddi3/__umoddi3, pulled in by libcompat's MSVC __alldiv shim
  and picolibc's __ultoa_invert), plus the usual 64-bit shift/mul/compare and int<->float
  conversion helpers, so those symbols are undefined without this archive.

  This compiles the classic i386 helper subset (pure integer/fp math -- no OS, mem, or emutls
  builtins, which the RXDK libs already define and would otherwise collide) at
  --target=i686-pc-windows-gnu -march=pentium3, so it is PIII-clean, then archives it as
  libclang_rt.builtins-i386.a. The RXDK-Tools engine (Toolchain/LlvmRuntime) links it, on demand,
  after the SDK libs; libcompat's whole-archive fabs/memmove overrides still win the tie-break.

  End state: the xboxog LLVM CI should package this archive in the toolchain zip at
  lib/clang/<ver>/lib/windows/. Until then, run this after downloading the toolchain and it
  writes the archive into that path so the engine finds it.

.PARAMETER LlvmRoot
  Root of an unpacked xboxog-<os>-<arch> clang toolchain (the dir holding bin/clang). Defaults
  to $env:RXDK_LLVM.

.PARAMETER OutDir
  Where to write libclang_rt.builtins-i386.a. Defaults to <LlvmRoot>/lib/clang/<ver>/lib/windows
  (the canonical clang runtime path the engine searches first).
#>
[CmdletBinding()]
param(
    [string]$LlvmRoot = $env:RXDK_LLVM,
    [string]$OutDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot   # repo root (tools/ is under it)

if ([string]::IsNullOrWhiteSpace($LlvmRoot)) {
    throw "No LLVM toolchain: pass -LlvmRoot or set RXDK_LLVM to an unpacked xboxog-<os>-<arch> clang root."
}
$clang = Join-Path $LlvmRoot 'bin/clang.exe'
if (-not (Test-Path -LiteralPath $clang)) { $clang = Join-Path $LlvmRoot 'bin/clang' }
if (-not (Test-Path -LiteralPath $clang)) { throw "clang not found under $LlvmRoot/bin" }
$ar = Join-Path $LlvmRoot 'bin/llvm-ar.exe'
if (-not (Test-Path -LiteralPath $ar)) { $ar = Join-Path $LlvmRoot 'bin/llvm-ar' }

$bsrc = Join-Path $root 'vendor/llvm-project/compiler-rt/lib/builtins'
if (-not (Test-Path -LiteralPath $bsrc)) {
    throw "compiler-rt builtins source missing at $bsrc. Run scripts/init-submodules.ps1 (its sparse " +
          "cone includes compiler-rt/lib/builtins), or add it directly:`n" +
          "  git -C vendor/llvm-project sparse-checkout add compiler-rt/lib/builtins"
}

# Resolve the default OutDir to the toolchain's canonical clang runtime path.
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $clangLib = Join-Path $LlvmRoot 'lib/clang'
    $ver = Get-ChildItem -LiteralPath $clangLib -Directory | Select-Object -First 1
    if ($null -eq $ver) { throw "no lib/clang/<ver> under $LlvmRoot" }
    $OutDir = Join-Path $ver.FullName 'lib/windows'
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Classic i386 64-bit + int<->fp conversion helper set. Deliberately excludes emutls/mem/OS
# builtins (RXDK libs define those; including them would duplicate strong symbols).
$files = @(
    'divdi3', 'udivdi3', 'moddi3', 'umoddi3', 'udivmoddi4', 'divmoddi4',
    'ashldi3', 'ashrdi3', 'lshrdi3', 'muldi3', 'negdi2', 'cmpdi2', 'ucmpdi2',
    'floatdidf', 'floatdisf', 'floatundidf', 'floatundisf',
    'fixdfdi', 'fixsfdi', 'fixunsdfdi', 'fixunssfdi',
    'floatdixf', 'floatundixf', 'fixxfdi', 'fixunsxfdi',
    'mulodi4', 'muloti4', 'absvdi2', 'negvdi2'
)

# i386 stack-probe asm: _alloca (decorated __alloca -- what clang lowers alloca() to on
# i386 mingw), __chkstk (chkstk2.S) and __chkstk_ms (chkstk.S). zig's mingw runtime supplied
# these; a bare clang link does not. These define NEW symbols (no overlap with the generic .c
# divide/shift builtins above), so keep both alongside the .c set.
$asmFiles = @('i386/chkstk', 'i386/chkstk2')

$tmp = Join-Path $env:TEMP ('rxdk-rt-builtins-{0}' -f ([guid]::NewGuid().ToString('N')))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$objs = @()
try {
    foreach ($f in $files) {
        $src = Join-Path $bsrc "$f.c"
        if (-not (Test-Path -LiteralPath $src)) { Write-Warning "no source $f.c"; continue }
        $obj = Join-Path $tmp "$f.o"
        & $clang '--target=i686-pc-windows-gnu' '-march=pentium3' '-O2' '-ffreestanding' `
            '-fno-stack-protector' '-c' $src '-o' $obj
        if ($LASTEXITCODE -ne 0) { throw "compile failed: $f.c" }
        $objs += $obj
    }
    foreach ($f in $asmFiles) {
        $src = Join-Path $bsrc "$f.S"
        if (-not (Test-Path -LiteralPath $src)) { Write-Warning "no source $f.S"; continue }
        $obj = Join-Path $tmp (('{0}.o' -f ($f -replace '[\\/]', '_')))
        & $clang '--target=i686-pc-windows-gnu' '-march=pentium3' '-c' $src '-o' $obj
        if ($LASTEXITCODE -ne 0) { throw "assemble failed: $f.S" }
        $objs += $obj
    }
    $outLib = Join-Path $OutDir 'libclang_rt.builtins-i386.a'
    if (Test-Path -LiteralPath $outLib) { Remove-Item -LiteralPath $outLib -Force }
    & $ar rcs $outLib @objs
    if ($LASTEXITCODE -ne 0) { throw "archiving failed" }
    Write-Host ("OK  {0}  ({1} builtins)" -f $outLib, $objs.Count) -ForegroundColor Green
}
finally {
    Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
