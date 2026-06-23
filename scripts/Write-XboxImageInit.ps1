# Emit per-sample zig-out/link/<name>_image_init.h from probe PE + XBE (build-time only).
param(
    [Parameter(Mandatory)]
    [string]$Name,
    [Parameter(Mandatory)]
    [string]$InputExe
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$outPath = Join-Path $root "zig-out\link\${Name}_image_init.h"
$inputFull = [IO.Path]::GetFullPath($InputExe)
$imagebld = Join-Path $root 'tools\rxdk-managed\win-x64\tools\imagebld.exe'
$xbePath = Join-Path $root "zig-out\link\${Name}_probe.xbe"

if (-not (Test-Path -LiteralPath $imagebld)) {
    throw "imagebld not found: $imagebld"
}

& $imagebld "/in:$inputFull" "/out:$xbePath" '/nologo' '/nolibwarn' '/stack:65536' | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "imagebld failed for probe PE: $inputFull"
}

$bytes = [IO.File]::ReadAllBytes($inputFull)
$e = [BitConverter]::ToInt32($bytes, 0x3C)
$opt = $e + 24
$anchorRva = [BitConverter]::ToUInt32($bytes, $opt + 16)
$coff = $e + 4
$nsec = [BitConverter]::ToUInt16($bytes, $coff + 2)
$optsz = [BitConverter]::ToUInt16($bytes, $coff + 16)
$sec = $opt + $optsz

$dataVa = 0
$dataVirt = 0
for ($i = 0; $i -lt $nsec; $i++) {
    $o = $sec + ($i * 40)
    $section = [Text.Encoding]::ASCII.GetString($bytes, $o, 8).Trim([char]0)
    if ($section -ne '.data') { continue }
    $dataVirt = [BitConverter]::ToUInt32($bytes, $o + 8)
    $dataVa = [BitConverter]::ToUInt32($bytes, $o + 12)
    break
}

if ($dataVa -eq 0) {
    throw "PE has no .data section: $inputFull"
}

$dump = & $imagebld /DUMP $xbePath 2>&1 | Out-String
if ($dump -notmatch '(?s)SECTION HEADER #\d+\s+\.data\s+[0-9A-Fa-fx]+\s+virtual address\s+([0-9A-Fa-fx]+)\s+virtual size[\s\S]*?([0-9A-Fa-fx]+)\s+size of raw data') {
    throw "Could not parse .data section from XBE dump: $xbePath"
}
$xbeVirt = [Convert]::ToUInt32($Matches[1].Replace('0x', ''), 16)
$xbeRaw = [Convert]::ToUInt32($Matches[2].Replace('0x', ''), 16)

# imagebld embeds less than the PE SizeOfRawData; clear from the XBE-loaded
# prefix through VirtualSize so picolibc globals are not garbage on kit boot.
$virt = [Math]::Max($dataVirt, $xbeVirt)
$bssStart = 0
$bssSize = 0
if ($virt -gt $xbeRaw) {
    $bssStart = [uint32]($dataVa + $xbeRaw)
    $bssSize = [uint32]($virt - $xbeRaw)
}

New-Item -ItemType Directory -Force -Path (Split-Path $outPath -Parent) | Out-Null
$header = @"
/* Generated at build time from $Name probe PE/XBE — do not edit. */
#pragma once

#define RXDK_IMAGE_ANCHOR_RVA 0x$("{0:X}" -f $anchorRva)u
#define RXDK_DATA_BSS_START_RVA 0x$("{0:X}" -f $bssStart)u
#define RXDK_DATA_BSS_SIZE 0x$("{0:X}" -f $bssSize)u
"@
[IO.File]::WriteAllText($outPath, $header, [Text.UTF8Encoding]::new($false))
Write-Host "Wrote $outPath (xbe .data raw=0x$("{0:X}" -f $xbeRaw) clear +0x$("{0:X}" -f $bssSize))"
