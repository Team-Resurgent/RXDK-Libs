#Requires -Version 5.1
<#
.SYNOPSIS
    Fail if an SDK library registers a DPC whose routine is not wrapped in RXDK_FPU_SAFE_DPC.
.DESCRIPTION
    A DPC runs on top of whatever thread it interrupted, and the kernel does not save that
    thread's floating-point state for it. Any x87/SSE use in the routine -- its own math, an xmm
    copy the compiler chose, a title callback -- overwrites the interrupted thread's registers
    (a title's SSE float math) or bugchecks with ATTEMPTED_FPU_USE_FROM_DPC.
    RXDK_FPU_SAFE_DPC (build\generated\rxdk_fpu_dpc.h) runs the routine with the FPU state saved,
    so every KeInitializeDpc in libs\ must pass its routine through it. Forwarders that take the
    routine as a parameter named DeferredRoutine pass through whatever their callers wrapped.
    build.ps1 runs this before compiling.
.EXAMPLE
    .\scripts\check-dpc-fpu.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Libs = Join-Path $Root 'libs'

# Blank out comments, keeping line breaks, so commented-out calls are ignored.
function Remove-Comments([string]$src) {
    [regex]::Replace($src, '//[^\n]*|/\*[\s\S]*?\*/', { param($m) [regex]::Replace($m.Value, '[^\n]', ' ') })
}

# The top-level arguments of the call whose '(' is just before $start.
function Get-CallArgs([string]$src, [int]$start) {
    $depth = 1; $parts = New-Object System.Collections.Generic.List[string]; $arg = New-Object System.Text.StringBuilder
    for ($i = $start; $i -lt $src.Length -and $depth -gt 0; $i++) {
        $c = $src[$i]
        if ('([{'.Contains($c)) { $depth++ }
        elseif (')]}'.Contains($c)) { $depth--; if ($depth -eq 0) { break } }
        if ($c -eq ',' -and $depth -eq 1) { $parts.Add($arg.ToString().Trim()); [void]$arg.Clear() }
        else { [void]$arg.Append($c) }
    }
    $parts.Add($arg.ToString().Trim())
    return , $parts
}

$call = [regex]'(?<![\w:])(?:::)?KeInitializeDpc\s*\('
$bad = @()
Get-ChildItem -LiteralPath $Libs -Recurse -File -Include *.c, *.cpp, *.cc | ForEach-Object {
    $path = $_.FullName
    $src = Remove-Comments ([System.IO.File]::ReadAllText($path))
    foreach ($m in $call.Matches($src)) {
        $a = Get-CallArgs $src ($m.Index + $m.Length)
        if ($a.Count -ne 3 -or ($a[0] + $a[1]) -match '\bP?R?KDPC\b|PKDEFERRED_ROUTINE\s+\w') { continue }  # declaration/definition
        $routine = ($a[1] -replace '\s+', ' ')
        if ($routine.StartsWith('RXDK_FPU_SAFE_DPC(') -or $routine -eq 'DeferredRoutine') { continue }
        $line = ($src.Substring(0, $m.Index) -split "`n").Count
        $bad += ('{0}:{1}: {2}' -f $path.Substring($Root.Length + 1), $line, $routine)
    }
}

if ($bad.Count) {
    Write-Host 'check-dpc-fpu: DPC routines not wrapped in RXDK_FPU_SAFE_DPC (see build\generated\rxdk_fpu_dpc.h):' -ForegroundColor Red
    $bad | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}
Write-Host 'OK  every KeInitializeDpc in libs\ runs its routine with the FPU state saved' -ForegroundColor Green
