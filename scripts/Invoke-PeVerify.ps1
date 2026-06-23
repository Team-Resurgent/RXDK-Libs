# PE checks before imagebld (import descriptors + .XBLD for kernel-linked samples).
param(
    [Parameter(Mandatory)]
    [string]$InputExe
)

$ErrorActionPreference = 'Stop'
$path = [IO.Path]::GetFullPath($InputExe)
if (-not (Test-Path -LiteralPath $path)) {
    throw "PE not found: $path"
}

$bytes = [IO.File]::ReadAllBytes($path)
$e = [BitConverter]::ToInt32($bytes, 0x3C)
$opt = $e + 24
$coff = $e + 4
$nsec = [BitConverter]::ToUInt16($bytes, $coff + 2)
$optsz = [BitConverter]::ToUInt16($bytes, $coff + 16)
$sec = $opt + $optsz

$names = @()
for ($i = 0; $i -lt $nsec; $i++) {
    $o = $sec + ($i * 40)
    $names += [Text.Encoding]::ASCII.GetString($bytes, $o, 8).Trim([char]0)
}

$subsystem = [BitConverter]::ToUInt16($bytes, $opt + 68)
if ($subsystem -ne 14) {
    throw "PE subsystem is $subsystem (expected 14 Xbox). Run Patch-PeXbox.ps1."
}

$descCount = 0
$dd = $opt + 96
$imp = [BitConverter]::ToUInt32($bytes, $dd + 8)
if ($imp -ne 0) {
    $rvaToOff = {
        param([uint32]$rva)
        for ($i = 0; $i -lt $nsec; $i++) {
            $o = $sec + ($i * 40)
            $va = [BitConverter]::ToUInt32($bytes, $o + 12)
            $raw = [BitConverter]::ToUInt32($bytes, $o + 20)
            $rsz = [BitConverter]::ToUInt32($bytes, $o + 16)
            if ($rva -ge $va -and $rva -lt ($va + [Math]::Max($rsz, 1))) {
                return [int]($raw + ($rva - $va))
            }
        }
        return -1
    }
    $off = & $rvaToOff $imp
    for ($i = 0; $i -lt 32; $i++) {
        $d = $off + ($i * 20)
        $oft = [BitConverter]::ToUInt32($bytes, $d)
        $iat = [BitConverter]::ToUInt32($bytes, $d + 16)
        if ($oft -eq 0 -and $iat -eq 0) { break }
        $descCount++
    }
}

if ($descCount -ne 1) {
    throw "PE has $descCount xboxkrnl import descriptors (expected 1)."
}

if ($names -notcontains '.XBLD') {
    throw "PE is missing .XBLD section (link prebuilt/xboxkrnl_xbld.obj)."
}

Write-Host "PE OK: subsystem=14, import descriptors=$descCount, sections=$($names -join ', ')" -ForegroundColor Green
