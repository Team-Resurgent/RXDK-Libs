#Requires -Version 5.1
<#
.SYNOPSIS
    Interactive menu to build a sample ISO for RXDK-LibsZig.

.DESCRIPTION
    Thin wrapper over scripts\compile.ps1. Pick a sample (and optimize mode),
    and it builds the zig target, patches/verifies the PE, converts it to an XBE
    (imagebld), and packs an XISO. The ISO lands in zig-out\iso\.

    Run with no arguments for a looping menu: after each build it waits for Enter
    and returns to the menu. Pass -Sample to build once and exit (scriptable).

.PARAMETER Sample
    Skip the menu and build this sample directly, then exit
    (xapi-smoke | libc-smoke | libcpp-smoke).

.PARAMETER Optimize
    Zig optimize mode. Defaults to Debug (prompts if not given interactively).

.PARAMETER NoHdd
    xapi-smoke only: build a boot-disc image without mounting/formatting the
    HDD utility drive.

.EXAMPLE
    .\build-iso.ps1
        Show the looping menu.

.EXAMPLE
    .\build-iso.ps1 -Sample xapi-smoke -Optimize Debug
        Build the xapi-smoke ISO once, non-interactively.
#>
[CmdletBinding()]
param(
    [ValidateSet('xapi-smoke', 'libc-smoke', 'libcpp-smoke')]
    [string]$Sample,
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize,
    [switch]$NoHdd
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$compile = Join-Path $root 'scripts\compile.ps1'
if (-not (Test-Path -LiteralPath $compile)) {
    throw "scripts\compile.ps1 not found next to this script ($compile)."
}

# Buildable sample ISO targets (the samples kept after the tree cleanup).
# Iso = the artifact filename compile.ps1 produces under zig-out\iso\.
$samples = @(
    [pscustomobject]@{ Target = 'xapi-smoke';   Iso = 'xapi-smoke.iso';   Desc = 'xAPI category smoke - 27 tests, kit hardware + HDD' }
    [pscustomobject]@{ Target = 'libc-smoke';   Iso = 'libc-smoke.iso';   Desc = 'libc / C23 runtime conformance matrix' }
    [pscustomobject]@{ Target = 'libcpp-smoke'; Iso = 'libcpp-smoke.iso'; Desc = 'libc++ / C++23 smoke (expected, string_view, iostream)' }
)

function Select-Sample {
    Write-Host ''
    Write-Host '  RXDK-LibsZig - build a sample ISO' -ForegroundColor Cyan
    Write-Host '  ---------------------------------'
    for ($i = 0; $i -lt $samples.Count; $i++) {
        Write-Host ('   {0}. {1,-18} {2}' -f ($i + 1), $samples[$i].Target, $samples[$i].Desc)
    }
    Write-Host '   q. quit'
    Write-Host ''
    while ($true) {
        $sel = Read-Host ('  Select a sample [1-{0}, q]' -f $samples.Count)
        if ($sel -match '^\s*(q|quit)\s*$') { return $null }
        $n = 0
        if ([int]::TryParse($sel, [ref]$n) -and $n -ge 1 -and $n -le $samples.Count) {
            return $samples[$n - 1]
        }
        Write-Host '  Invalid selection - enter a number from the list, or q to quit.' -ForegroundColor Yellow
    }
}

function Invoke-SampleIso {
    param(
        [Parameter(Mandatory)] [pscustomobject]$Chosen,
        [Parameter(Mandatory)] [string]$Opt,
        [bool]$UseNoHdd
    )
    Write-Host ''
    Write-Host ('==> building {0} ISO ({1})' -f $Chosen.Target, $Opt) -ForegroundColor Cyan

    $compileArgs = @{
        Target   = $Chosen.Target
        Optimize = $Opt
        Iso      = $true
    }
    if ($Chosen.Target -eq 'xapi-smoke' -and $UseNoHdd) { $compileArgs['NoHdd'] = $true }

    & $compile @compileArgs

    $isoPath = Join-Path $root ('zig-out\iso\{0}' -f $Chosen.Iso)
    Write-Host ''
    if (Test-Path -LiteralPath $isoPath) {
        $size = (Get-Item -LiteralPath $isoPath).Length
        Write-Host ('OK  {0}  ({1:N0} bytes)' -f $isoPath, $size) -ForegroundColor Green
    }
    else {
        Write-Warning "Build reported success but ISO not found at $isoPath"
    }
}

# Resolve optimize mode for the non-interactive defaults.
function Resolve-Optimize {
    if ($Optimize) { return $Optimize }
    $opt = Read-Host '  Optimize [Debug] (Debug/ReleaseSafe/ReleaseFast/ReleaseSmall)'
    if ([string]::IsNullOrWhiteSpace($opt)) { return 'Debug' }
    if ($opt -notin @('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')) {
        throw "Invalid optimize mode: $opt"
    }
    return $opt
}

# ---- Non-interactive: build once and exit (errors propagate). --------------
if ($Sample) {
    $chosen = $samples | Where-Object { $_.Target -eq $Sample } | Select-Object -First 1
    $opt = if ($Optimize) { $Optimize } else { 'Debug' }
    Invoke-SampleIso -Chosen $chosen -Opt $opt -UseNoHdd:([bool]$NoHdd)
    return
}

# ---- Interactive: loop the menu until the user quits. ----------------------
while ($true) {
    $chosen = Select-Sample
    if (-not $chosen) { Write-Host '  Bye.'; break }

    $opt = Resolve-Optimize

    # xapi-smoke can target the HDD utility drive (mount + format) or a plain
    # boot disc. Other samples ignore HDD flags.
    $useNoHdd = [bool]$NoHdd
    if ($chosen.Target -eq 'xapi-smoke' -and -not $NoHdd) {
        $ans = Read-Host '  Mount + format HDD utility drive? (recommended for kit) [Y/n]'
        if ($ans -match '^\s*[nN]') { $useNoHdd = $true }
    }

    try {
        Invoke-SampleIso -Chosen $chosen -Opt $opt -UseNoHdd:$useNoHdd
    }
    catch {
        Write-Host ''
        Write-Host ('BUILD FAILED: {0}' -f $_.Exception.Message) -ForegroundColor Red
    }

    Write-Host ''
    [void](Read-Host 'Press Enter to return to the menu')
}
