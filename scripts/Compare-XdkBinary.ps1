# Compare RXDK-Libs PE/XBE headers against a known-good XDK build.
param(
    [string]$ReferenceExe = 'D:\Git\TriangleXDK\Debug\TriangleXDK.exe',
    [string]$ReferenceXbe = 'D:\Git\TriangleXDK\Debug\TriangleXDK.xbe',
    [string]$CandidateExe = 'zig-out\samples\libc-smoke\libc-smoke.exe',
    [string]$CandidateXbe = 'zig-out\xbe\libc-smoke.xbe'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$imagebld = Join-Path $root 'tools\rxdk-managed\win-x64\tools\imagebld.exe'

function Resolve-RepoPath([string]$p) {
    if ([IO.Path]::IsPathRooted($p)) { return $p }
    return Join-Path $root $p
}

function Get-PeSummary([string]$path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    $e = [BitConverter]::ToInt32($bytes, 0x3C)
    $coff = $e + 4
    $opt = $e + 24
    $optSize = [BitConverter]::ToUInt16($bytes, $coff + 16)
    $sec = $opt + $optSize
    $sections = @()
    $n = [BitConverter]::ToUInt16($bytes, $coff + 2)
    for ($i = 0; $i -lt $n; $i++) {
        $o = $sec + ($i * 40)
        $sections += [Text.Encoding]::ASCII.GetString($bytes, $o, 8).Trim([char]0)
    }
    [pscustomobject]@{
        Path = $path
        Size = $bytes.Length
        Subsystem = [BitConverter]::ToUInt16($bytes, $opt + 68)
        StackReserve = ('0x{0:X}' -f [BitConverter]::ToUInt32($bytes, $opt + 72))
        StackCommit = ('0x{0:X}' -f [BitConverter]::ToUInt32($bytes, $opt + 76))
        EntryRVA = ('0x{0:X}' -f [BitConverter]::ToUInt32($bytes, $opt + 16))
        Sections = ($sections -join ', ')
    }
}

function Get-XbeSummary([string]$path) {
    if (-not (Test-Path -LiteralPath $imagebld)) {
        return [pscustomobject]@{ Path = $path; Error = 'imagebld not installed' }
    }
    $dump = & $imagebld /DUMP $path 2>&1 | Out-String
    $init = if ($dump -match '(\d+) initialization flags') { $Matches[1] } else { '?' }
    $stack = if ($dump -match '([0-9A-Fa-fx]+) size of stack commit') { $Matches[1] } else { '?' }
    $media = if ($dump -match '([0-9A-Fa-fx]+) allowed media types') { $Matches[1] } else { '?' }
    $libs = ([regex]::Matches($dump, '^\s+([A-Z0-9]+)\s+[0-9.]+\s', 'Multiline') | ForEach-Object { $_.Groups[1].Value }) -join ', '
    [pscustomobject]@{
        Path = $path
        InitFlags = $init
        StackCommit = $stack
        MediaTypes = $media
        Libraries = $libs
    }
}

$refExe = Resolve-RepoPath $ReferenceExe
$refXbe = Resolve-RepoPath $ReferenceXbe
$candExe = Resolve-RepoPath $CandidateExe
$candXbe = Resolve-RepoPath $CandidateXbe

Write-Host '=== PE comparison ===' -ForegroundColor Cyan
Get-PeSummary $refExe | Format-List
Get-PeSummary $candExe | Format-List

Write-Host '=== XBE comparison ===' -ForegroundColor Cyan
Get-XbeSummary $refXbe | Format-List
Get-XbeSummary $candXbe | Format-List

Write-Host 'TriangleXDK imagebld (from BuildLog.htm): /stack:65536 /debug /nolibwarn' -ForegroundColor Yellow
Write-Host 'TriangleXDK deploy: xbecopy to devkit E:\ (not ISO boot)' -ForegroundColor Yellow
