#Requires -Version 5.1
<#
.SYNOPSIS
    Fail if an SDK library calls a C library function with the wrong calling convention.
.DESCRIPTION
    Several libraries (libd3d8, libdsound, ...) are built with -fdefault-calling-conv=stdcall and
    -ffreestanding, so a libc prototype without an explicit convention compiles as __stdcall: the
    call site references _memcpy@12 (callee pops its arguments) while libc defines _memcpy
    (__cdecl, caller pops). lld resolves the decorated name to the plain one with only a warning,
    and every such call then leaves the stack unbalanced. Each library's site\cdecl_libc.h
    declares the libc functions it calls __cdecl to prevent this; this script catches one it
    missed.

    A library fails if it leaves undefined a symbol _name@N that no library defines, while libc
    defines _name. (A real __stdcall function that some library defines -- winsock's select in
    libxnet, say -- is fine.) build.ps1 runs this on dist\lib after building.
.PARAMETER LibDir
    The directory of .lib files to check (default: dist\lib).
.EXAMPLE
    .\scripts\check-cdecl-libc.ps1
.EXAMPLE
    .\scripts\check-cdecl-libc.ps1 -LibDir C:\ProgramData\RXDK\sdk\lib
#>
[CmdletBinding()]
param(
    [string]$LibDir = (Join-Path (Split-Path -Parent $PSScriptRoot) 'dist\lib')
)

$ErrorActionPreference = 'Stop'

# The external symbols of every COFF object in an ar archive: @{ Defined; Undefined }.
function Get-ArchiveSymbols([string]$path) {
    $b = [System.IO.File]::ReadAllBytes($path)
    $defined = New-Object System.Collections.Generic.HashSet[string]
    $undefined = New-Object System.Collections.Generic.HashSet[string]
    if ($b.Length -lt 8 -or [System.Text.Encoding]::ASCII.GetString($b, 0, 8) -ne "!<arch>`n") {
        throw "$path is not an ar archive"
    }
    $pos = 8
    while ($pos + 60 -le $b.Length) {
        $name = [System.Text.Encoding]::ASCII.GetString($b, $pos, 16).Trim()
        $size = [int64]([System.Text.Encoding]::ASCII.GetString($b, $pos + 48, 10).Trim())
        $data = $pos + 60
        if ($name -ne '/' -and $name -ne '//' -and $name -ne '/SYM64/' -and $size -ge 20) {
            Add-CoffSymbols $b $data $size $defined $undefined
        }
        $pos = $data + $size
        if ($pos % 2) { $pos++ }
    }
    return @{ Defined = $defined; Undefined = $undefined }
}

function Add-CoffSymbols([byte[]]$b, [int64]$o, [int64]$size, $defined, $undefined) {
    $machine = [BitConverter]::ToUInt16($b, $o)
    # Only i386 objects (short import members and anything else are skipped).
    if ($machine -ne 0x14C) { return }
    $symPtr = [BitConverter]::ToUInt32($b, $o + 8)
    $count = [BitConverter]::ToUInt32($b, $o + 12)
    if ($symPtr -eq 0 -or $symPtr + 18 * $count -gt $size) { return }
    $sym = $o + $symPtr
    $strtab = $sym + 18 * $count
    $i = 0
    while ($i -lt $count) {
        $e = $sym + 18 * $i
        $section = [BitConverter]::ToInt16($b, $e + 12)
        $class = $b[$e + 16]
        $aux = $b[$e + 17]
        if ($class -eq 2) {  # IMAGE_SYM_CLASS_EXTERNAL
            if ([BitConverter]::ToUInt32($b, $e) -eq 0) {
                $off = [BitConverter]::ToUInt32($b, $e + 4)
                $start = $strtab + $off; $end = $start
                while ($end -lt $b.Length -and $b[$end] -ne 0) { $end++ }
                $n = [System.Text.Encoding]::ASCII.GetString($b, $start, $end - $start)
            } else {
                $n = [System.Text.Encoding]::ASCII.GetString($b, $e, 8).TrimEnd([char]0)
            }
            if ($section -gt 0) { [void]$defined.Add($n) }
            elseif ($section -eq 0 -and [BitConverter]::ToUInt32($b, $e + 8) -eq 0) { [void]$undefined.Add($n) }
        }
        $i += 1 + $aux
    }
}

$libs = @(Get-ChildItem -LiteralPath $LibDir -Filter *.lib -File)
if (-not $libs) { throw "no .lib files in $LibDir" }

$all = @{}
$everyDefined = New-Object System.Collections.Generic.HashSet[string]
foreach ($lib in $libs) {
    $s = Get-ArchiveSymbols $lib.FullName
    $all[$lib.Name] = $s
    $everyDefined.UnionWith($s.Defined)
}
$libc = New-Object System.Collections.Generic.HashSet[string]
foreach ($name in @('libc.lib', 'libcd.lib')) {
    if ($all.ContainsKey($name)) { $libc.UnionWith($all[$name].Defined) }
}
if ($libc.Count -eq 0) { throw "libc.lib not found in $LibDir" }

$bad = @()
foreach ($lib in $libs) {
    foreach ($u in $all[$lib.Name].Undefined) {
        if ($u -match '^(_[A-Za-z0-9_]+)@\d+$' -and -not $everyDefined.Contains($u) -and $libc.Contains($Matches[1])) {
            $bad += "  $($lib.Name): calls $($Matches[1].Substring(1)) as __stdcall ($u); libc defines it __cdecl"
        }
    }
}

if ($bad) {
    Write-Host 'check-cdecl-libc: libc functions called with the wrong calling convention.' -ForegroundColor Red
    Write-Host 'Declare them __cdecl in the library''s site\cdecl_libc.h:' -ForegroundColor Red
    $bad | Sort-Object -Unique | ForEach-Object { Write-Host $_ -ForegroundColor Red }
    exit 1
}
Write-Host ("OK  check-cdecl-libc  {0} libraries" -f $libs.Count) -ForegroundColor Green
exit 0
