<#
.SYNOPSIS
Disassembles one function out of a prebuilt XDK static library.

.DESCRIPTION
The retail XDK libs put every function in its own COMDAT section, all named
'D3D' and all based at zero, so llvm-objdump labels the blocks by section name
and mangled C++ names never appear. Recovering a single function therefore means
resolving the symbol to a section number, counting where that section falls
among the code sections, and slicing the corresponding block out of the dump.

Used to compare RXDK's source-built libs against the shipped 5849 binaries.

.EXAMPLE
Get-XdkFunction -Lib "...\d3d8.lib" -Member pusher.obj -Symbol '?KickOff@CDevice@D3D@@QAEXXZ'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Lib,
    [Parameter(Mandatory)] [string] $Member,
    [Parameter(Mandatory)] [string] $Symbol,
    [string] $LlvmBin = "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\Llvm\bin",
    [string] $WorkDir = (Join-Path $env:TEMP 'xdkfunc')
)

$ErrorActionPreference = 'Stop'

$ar = Join-Path $LlvmBin 'llvm-ar.exe'
$od = Join-Path $LlvmBin 'llvm-objdump.exe'
$ro = Join-Path $LlvmBin 'llvm-readobj.exe'
foreach ($t in @($ar, $od, $ro)) {
    if (-not (Test-Path $t)) { throw "not found: $t" }
}

New-Item $WorkDir -ItemType Directory -Force | Out-Null
$obj = Join-Path $WorkDir $Member

if (-not (Test-Path $obj)) {
    Push-Location $WorkDir
    try {
        # Archive members are stored with their original 'obj\i386\' prefix.
        & $ar x $Lib "obj\i386\$Member" 2>&1 | Out-Null
    } finally { Pop-Location }
}
if (-not (Test-Path $obj)) { throw "could not extract $Member from $Lib" }

# Symbol -> section number.

$symText = & $ro --symbols $obj 2>$null
$section = $null
for ($i = 0; $i -lt $symText.Count; $i++) {
    if ($symText[$i] -match [regex]::Escape($Symbol)) {
        for ($j = $i; $j -lt [Math]::Min($symText.Count, $i + 8); $j++) {
            if ($symText[$j] -match '^\s*Section:\s*\S+\s*\((\d+)\)') { $section = [int]$matches[1]; break }
        }
        if ($section) { break }
    }
}
if (-not $section) { throw "symbol not found in ${Member}: $Symbol" }

# Section number -> ordinal among code sections, and its size.

$secText = & $ro --section-headers $obj 2>$null
$secs = @()
for ($i = 0; $i -lt $secText.Count; $i++) {
    if ($secText[$i] -match '^\s*Number:\s*(\d+)') {
        $num = [int]$matches[1]; $name = ''; $size = 0
        for ($j = $i; $j -lt [Math]::Min($secText.Count, $i + 12); $j++) {
            if ($secText[$j] -match '^\s*Name:\s*(\S+)') { $name = $matches[1] }
            if ($secText[$j] -match '^\s*RawDataSize:\s*(\d+)') { $size = [int]$matches[1]; break }
        }
        $secs += [pscustomobject]@{ Number = $num; Name = $name; Size = $size }
    }
}

$target = $secs | Where-Object { $_.Number -eq $section }
$code = @($secs | Where-Object { $_.Name -eq $target.Name })
$ordinal = -1
for ($k = 0; $k -lt $code.Count; $k++) { if ($code[$k].Number -eq $section) { $ordinal = $k; break } }
if ($ordinal -lt 0) { throw "section $section not found among '$($target.Name)' sections" }

# Slice the matching block out of the full disassembly.

$asm = & $od -d --no-show-raw-insn $obj 2>$null
$hdr = @()
for ($i = 0; $i -lt $asm.Count; $i++) {
    if ($asm[$i] -match "^Disassembly of section $([regex]::Escape($target.Name)):") { $hdr += $i }
}
if ($ordinal -ge $hdr.Count) { throw "only $($hdr.Count) code blocks in the dump, need #$($ordinal + 1)" }

$from = $hdr[$ordinal]
$to = if ($ordinal + 1 -lt $hdr.Count) { $hdr[$ordinal + 1] } else { $asm.Count }

Write-Output "// $Symbol"
Write-Output "// $Member section $section, $($target.Size) bytes, code block $($ordinal + 1)/$($hdr.Count)"
for ($i = $from; $i -lt $to; $i++) { if ($asm[$i].Trim()) { Write-Output $asm[$i] } }
