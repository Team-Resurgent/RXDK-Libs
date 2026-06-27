#Requires -Version 5.1
<#
.SYNOPSIS
    Interactive menu to build a sample ISO (or a redistributable lib bundle) for
    RXDK-LibsZig.

.DESCRIPTION
    Thin wrapper over scripts\compile.ps1.

    Pick a sample (and optimize mode) and it builds the zig target, patches/
    verifies the PE, converts it to an XBE (imagebld), and packs an XISO under
    zig-out\iso\.

    Or pick "dist" to build the libraries (libc, libcpp, libxapi) and stage the
    .lib files plus the public headers into dist\libs and dist\include
    (dist\ is gitignored).

    Run with no arguments for a looping menu: after each action it waits for
    Enter and returns to the menu. Pass -Sample or -Dist to run once and exit
    (scriptable).

.PARAMETER Sample
    Skip the menu and build this sample directly, then exit
    (xapi-smoke | libc-smoke | libcpp-smoke).

.PARAMETER Dist
    Skip the menu, build the lib distribution into dist\, then exit.

.PARAMETER Optimize
    Zig optimize mode. Defaults to Debug (prompts if not given interactively).

.PARAMETER NoHdd
    xapi-smoke only: build a boot-disc image without mounting/formatting the
    HDD utility drive.

.PARAMETER Mode
    Output mode. 'iso' builds an XISO (default). 'deploy' builds the XBE and
    copies it to the Xbox (e:\xbetest\default.xbe via xbcp) then launches it
    (xbox-launch). Persisted between runs in .rxdk-deploy.json.

.PARAMETER XboxIp
    Xbox hostname or IP for deploy mode. Persisted in .rxdk-deploy.json.

.EXAMPLE
    .\build-iso.ps1
        Show the looping menu.

.EXAMPLE
    .\build-iso.ps1 -Sample xapi-smoke -Optimize Debug
        Build the xapi-smoke ISO once, non-interactively.

.EXAMPLE
    .\build-iso.ps1 -Sample libcpp-smoke -Mode deploy -XboxIp 192.168.0.42
        Build the libcpp-smoke XBE, copy it to the kit, and launch it.

.EXAMPLE
    .\build-iso.ps1 -Dist -Optimize ReleaseFast
        Build libc/libcpp/libxapi and stage them into dist\.
#>
[CmdletBinding()]
param(
    [ValidateSet('xapi-smoke', 'libc-smoke', 'libcpp-smoke')]
    [string]$Sample,
    [switch]$Dist,
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize,
    [switch]$NoHdd,
    [ValidateSet('iso', 'deploy')]
    [string]$Mode,
    [string]$XboxIp
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$compile = Join-Path $root 'scripts\compile.ps1'
if (-not (Test-Path -LiteralPath $compile)) {
    throw "scripts\compile.ps1 not found next to this script ($compile)."
}

# ---- Deploy settings (Xbox IP + iso/deploy mode), persisted locally. -------
$deployConfigPath = Join-Path $root '.rxdk-deploy.json'

# Where a deployed XBE lands on the kit, and what xbox-launch runs. The RXDK
# tools address the Xbox E: drive as "xe:\" (xc:/xd: for C:/D:), so the kit-side
# path E:\xbetest is written xe:\xbetest for xbcp and xbox-launch.
$deployRemoteDir = 'xe:\xbetest'
$deployRemoteXbe = 'default.xbe'

# How long xbox-launch streams debug output / waits for the initial break before
# returning. Our smoke titles run to completion without a debugger break, so the
# launch "times out" (and the title keeps running on the kit) -- that is success,
# not failure. Keep it short enough not to stall the menu.
$deployLaunchTimeoutMs = 30000

function Get-DeployConfig {
    $cfg = [pscustomobject]@{ Mode = 'iso'; XboxIp = '' }
    if (Test-Path -LiteralPath $deployConfigPath) {
        try {
            $loaded = Get-Content -LiteralPath $deployConfigPath -Raw | ConvertFrom-Json
            if ($loaded.Mode -in @('iso', 'deploy')) { $cfg.Mode = $loaded.Mode }
            if ($loaded.XboxIp) { $cfg.XboxIp = [string]$loaded.XboxIp }
        }
        catch {
            Write-Warning "Could not read $deployConfigPath - using defaults."
        }
    }
    return $cfg
}

function Save-DeployConfig {
    param([Parameter(Mandatory)] [pscustomobject]$Config)
    $Config | ConvertTo-Json | Set-Content -LiteralPath $deployConfigPath -Encoding utf8
}

# Resolve a downloaded RXDK-Tools host binary (xbcp / xbox-launch), or fail with
# a pointer to the installer.
function Get-RxdkTool {
    param([Parameter(Mandatory)] [string]$Name)
    $path = Join-Path $root ('tools\rxdk-managed\win-x64\tools\{0}' -f $Name)
    if (-not (Test-Path -LiteralPath $path)) {
        throw "$Name not found ($path). Run scripts\install-rxdk-tools.ps1 first."
    }
    return $path
}

# Register the IP as the toolchain's default Xbox via xbset, so xbcp / xbox-launch
# / xbwatson can find it. Best-effort: warn (don't fail) if xbset is missing or
# errors -- the menu still passes the IP explicitly with -x / /x.
function Register-XboxIp {
    param([string]$XboxIp)
    if ([string]::IsNullOrWhiteSpace($XboxIp)) { return }
    try {
        $xbset = Get-RxdkTool 'xbset.exe'
        Write-Host ('==> xbset {0} (default Xbox)' -f $XboxIp) -ForegroundColor Cyan
        & $xbset $XboxIp
        if ($LASTEXITCODE -ne 0) {
            Write-Host ('  xbset returned exit {0}' -f $LASTEXITCODE) -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host ('  xbset unavailable: {0}' -f $_.Exception.Message) -ForegroundColor Yellow
    }
}

# Buildable sample ISO targets (the samples kept after the tree cleanup).
# Iso = the artifact filename compile.ps1 produces under zig-out\iso\.
$samples = @(
    [pscustomobject]@{ Target = 'xapi-smoke';   Iso = 'xapi-smoke.iso';   Desc = 'xAPI category smoke - 27 tests, kit hardware + HDD' }
    [pscustomobject]@{ Target = 'libc-smoke';   Iso = 'libc-smoke.iso';   Desc = 'libc / C23 runtime conformance matrix' }
    [pscustomobject]@{ Target = 'libcpp-smoke'; Iso = 'libcpp-smoke.iso'; Desc = 'libc++ / C++23 smoke (expected, string_view, iostream)' }
)

function Select-Action {
    param([Parameter(Mandatory)] [pscustomobject]$Config)

    $ipText = if ([string]::IsNullOrWhiteSpace($Config.XboxIp)) { '(not set)' } else { $Config.XboxIp }
    $verb = if ($Config.Mode -eq 'deploy') { 'DPLY' } else { 'ISO ' }

    Write-Host ''
    Write-Host '  RXDK-LibsZig - build menu' -ForegroundColor Cyan
    Write-Host '  -------------------------'
    Write-Host ('   mode: {0,-7}  xbox: {1}' -f $Config.Mode, $ipText) -ForegroundColor DarkGray
    for ($i = 0; $i -lt $samples.Count; $i++) {
        Write-Host ('   {0}. {1,-18} {2}  {3}' -f ($i + 1), $samples[$i].Target, $verb, $samples[$i].Desc)
    }
    Write-Host '   d. dist               libs libc/libcpp/libxapi + headers -> dist\'
    Write-Host '   m. mode               toggle iso <-> deploy'
    Write-Host '   i. ip                 set Xbox hostname / IP (deploy)'
    Write-Host '   w. watson             launch xbWatson debug monitor (/x <ip>)'
    Write-Host '   q. quit'
    Write-Host ''
    while ($true) {
        $sel = Read-Host ('  Select [1-{0}, d, m, i, w, q]' -f $samples.Count)
        if ($sel -match '^\s*(q|quit)\s*$') { return $null }
        if ($sel -match '^\s*d(ist)?\s*$') { return 'dist' }
        if ($sel -match '^\s*m(ode)?\s*$') { return 'toggle-mode' }
        if ($sel -match '^\s*i(p)?\s*$') { return 'set-ip' }
        if ($sel -match '^\s*w(atson)?\s*$') { return 'watson' }
        $n = 0
        if ([int]::TryParse($sel, [ref]$n) -and $n -ge 1 -and $n -le $samples.Count) {
            return $samples[$n - 1]
        }
        Write-Host '  Invalid selection - enter a number, d/m/i/w, or q to quit.' -ForegroundColor Yellow
    }
}

# Prompt for and validate an Xbox hostname/IP (accepts dotted-quad or a hostname).
function Read-XboxIp {
    param([string]$Current)
    $prompt = if ([string]::IsNullOrWhiteSpace($Current)) { '  Xbox hostname or IP' } else { ('  Xbox hostname or IP [{0}]' -f $Current) }
    $val = Read-Host $prompt
    if ([string]::IsNullOrWhiteSpace($val)) { return $Current }
    return $val.Trim()
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

function Invoke-SampleDeploy {
    param(
        [Parameter(Mandatory)] [pscustomobject]$Chosen,
        [Parameter(Mandatory)] [string]$Opt,
        [bool]$UseNoHdd,
        [Parameter(Mandatory)] [string]$XboxIp
    )
    if ([string]::IsNullOrWhiteSpace($XboxIp)) {
        throw "No Xbox IP set. Pick 'i' in the menu (or pass -XboxIp) before deploying."
    }

    $xbcp = Get-RxdkTool 'xbcp.exe'
    $launch = Get-RxdkTool 'xbox-launch.exe'

    Write-Host ''
    Write-Host ('==> building {0} XBE ({1}) for deploy' -f $Chosen.Target, $Opt) -ForegroundColor Cyan

    # Deploy only needs the XBE, not an XISO.
    $compileArgs = @{
        Target   = $Chosen.Target
        Optimize = $Opt
        Xbe      = $true
    }
    if ($Chosen.Target -eq 'xapi-smoke' -and $UseNoHdd) { $compileArgs['NoHdd'] = $true }

    & $compile @compileArgs

    $xbe = Join-Path $root ('zig-out\xbe\{0}.xbe' -f $Chosen.Target)
    if (-not (Test-Path -LiteralPath $xbe)) {
        throw "Build reported success but XBE not found at $xbe"
    }

    $remoteXbe = '{0}\{1}' -f $deployRemoteDir, $deployRemoteXbe

    Write-Host ''
    Write-Host ('==> copying {0} -> {1} on {2}' -f $Chosen.Target, $remoteXbe, $XboxIp) -ForegroundColor Cyan
    # -t create destination dir, -y overwrite without prompting.
    & $xbcp $xbe $remoteXbe -x $XboxIp -y -t
    if ($LASTEXITCODE -ne 0) { throw "xbcp failed (exit $LASTEXITCODE)." }

    Write-Host ''
    Write-Host ('==> launching {0} on {1}' -f $remoteXbe, $XboxIp) -ForegroundColor Cyan
    & $launch /dir $deployRemoteDir /title $deployRemoteXbe /x $XboxIp /timeout $deployLaunchTimeoutMs
    $code = $LASTEXITCODE

    Write-Host ''
    if ($code -eq 2) {
        # xbox-launch's "no console configured" code.
        Write-Warning "xbox-launch: no Xbox console configured (set one with 'i' / xbset)."
    }
    elseif ($code -eq 0) {
        Write-Host ('OK  deployed + launched {0} on {1} (hit initial break)' -f $Chosen.Target, $XboxIp) -ForegroundColor Green
    }
    else {
        # Non-zero here means no initial debug break arrived within the timeout.
        # That is expected for our run-to-completion smoke titles: the title was
        # launched and ran on the kit (watch full output with 'w' / xbWatson).
        Write-Host ('OK  deployed + launched {0} on {1} (ran to completion; no debug break)' -f $Chosen.Target, $XboxIp) -ForegroundColor Green
    }

    # The deploy + launch succeeded; don't let xbox-launch's break-timeout exit
    # code leak out as a script-level failure (matters for scripted -Sample runs).
    $global:LASTEXITCODE = 0
}

# Launch the xbWatson debug-output monitor against the configured kit. It runs
# in its own window so the menu stays usable.
function Invoke-Watson {
    param([Parameter(Mandatory)] [string]$XboxIp)
    if ([string]::IsNullOrWhiteSpace($XboxIp)) {
        throw "No Xbox IP set. Pick 'i' in the menu (or pass -XboxIp) first."
    }
    $watson = Get-RxdkTool 'xbwatson.exe'
    Write-Host ('==> launching xbWatson on {0}' -f $XboxIp) -ForegroundColor Cyan
    Start-Process -FilePath $watson -ArgumentList '/x', $XboxIp | Out-Null
    Write-Host ('OK  xbWatson started for {0}' -f $XboxIp) -ForegroundColor Green
}

function Invoke-DistBuild {
    param([Parameter(Mandatory)] [string]$Opt)
    Write-Host ''
    Write-Host ('==> building lib distribution (libc/libcpp/libxapi, {0})' -f $Opt) -ForegroundColor Cyan

    # compile.ps1 -Target libs runs the default zig install step, which builds
    # libc/libcpp/libxapi and stages the .lib files + public headers into
    # zig-out\lib and zig-out\include.
    & $compile -Target libs -Optimize $Opt

    $distLibs = Join-Path $root 'dist\libs'
    $distInc = Join-Path $root 'dist\include'
    foreach ($d in @($distLibs, $distInc)) {
        if (Test-Path -LiteralPath $d) { Remove-Item -LiteralPath $d -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $d | Out-Null
    }

    # Ship only the canonical libraries (zig-out\lib can hold stale artifacts
    # from earlier/explicit builds).
    $shipLibs = @('libc.lib', 'libcpp.lib', 'libxapi.lib')
    $copied = @()
    foreach ($name in $shipLibs) {
        $src = Join-Path $root ('zig-out\lib\{0}' -f $name)
        if (Test-Path -LiteralPath $src) {
            Copy-Item -LiteralPath $src -Destination $distLibs -Force
            $copied += $name
        }
        else {
            Write-Warning "expected lib not found: zig-out\lib\$name"
        }
    }

    # -Path (not -LiteralPath) so the '*' wildcard is expanded.
    $incSrc = Join-Path $root 'zig-out\include'
    if (Test-Path -LiteralPath $incSrc) {
        Copy-Item -Path (Join-Path $incSrc '*') -Destination $distInc -Recurse -Force
    }

    $hdrCount = @(Get-ChildItem -LiteralPath $distInc -Recurse -File -ErrorAction SilentlyContinue).Count
    Write-Host ''
    Write-Host ('OK  dist\libs     {0} libs: {1}' -f $copied.Count, ($copied -join ', ')) -ForegroundColor Green
    Write-Host ('OK  dist\include  {0} headers' -f $hdrCount) -ForegroundColor Green
}

# Resolve optimize mode (prompt with a Debug default when interactive).
function Resolve-Optimize {
    if ($Optimize) { return $Optimize }
    $opt = Read-Host '  Optimize [Debug] (Debug/ReleaseSafe/ReleaseFast/ReleaseSmall)'
    if ([string]::IsNullOrWhiteSpace($opt)) { return 'Debug' }
    if ($opt -notin @('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')) {
        throw "Invalid optimize mode: $opt"
    }
    return $opt
}

# Build a sample as an ISO or deploy it to the kit, per the active mode.
function Invoke-SampleAction {
    param(
        [Parameter(Mandatory)] [pscustomobject]$Chosen,
        [Parameter(Mandatory)] [string]$Opt,
        [bool]$UseNoHdd,
        [Parameter(Mandatory)] [pscustomobject]$Config
    )
    if ($Config.Mode -eq 'deploy') {
        Invoke-SampleDeploy -Chosen $Chosen -Opt $Opt -UseNoHdd:$UseNoHdd -XboxIp $Config.XboxIp
    }
    else {
        Invoke-SampleIso -Chosen $Chosen -Opt $Opt -UseNoHdd:$UseNoHdd
    }
}

# Load persisted settings, then let -Mode / -XboxIp override (and persist) them.
$deployCfg = Get-DeployConfig
$cfgDirty = $false
if ($Mode) { $deployCfg.Mode = $Mode; $cfgDirty = $true }
if ($PSBoundParameters.ContainsKey('XboxIp')) {
    $deployCfg.XboxIp = $XboxIp
    $cfgDirty = $true
    Register-XboxIp -XboxIp $XboxIp
}
if ($cfgDirty) { Save-DeployConfig -Config $deployCfg }

# Config-only invocation (-Mode/-XboxIp with no build target): persist and exit.
if ($cfgDirty -and -not $Sample -and -not $Dist) {
    $shown = if ([string]::IsNullOrWhiteSpace($deployCfg.XboxIp)) { '(not set)' } else { $deployCfg.XboxIp }
    Write-Host ('  saved: mode={0}, xbox={1}' -f $deployCfg.Mode, $shown) -ForegroundColor Green
    return
}

# ---- Non-interactive: run once and exit (errors propagate). ----------------
if ($Dist) {
    Invoke-DistBuild -Opt $(if ($Optimize) { $Optimize } else { 'Debug' })
    return
}
if ($Sample) {
    $chosen = $samples | Where-Object { $_.Target -eq $Sample } | Select-Object -First 1
    $opt = if ($Optimize) { $Optimize } else { 'Debug' }
    Invoke-SampleAction -Chosen $chosen -Opt $opt -UseNoHdd:([bool]$NoHdd) -Config $deployCfg
    return
}

# ---- Interactive: loop the menu until the user quits. ----------------------
while ($true) {
    $chosen = Select-Action -Config $deployCfg
    if (-not $chosen) { Write-Host '  Bye.'; break }

    # Settings actions: update config and loop without the "press Enter" pause.
    if ($chosen -eq 'toggle-mode') {
        $deployCfg.Mode = if ($deployCfg.Mode -eq 'deploy') { 'iso' } else { 'deploy' }
        Save-DeployConfig -Config $deployCfg
        Write-Host ('  mode -> {0}' -f $deployCfg.Mode) -ForegroundColor Green
        continue
    }
    if ($chosen -eq 'set-ip') {
        $deployCfg.XboxIp = Read-XboxIp -Current $deployCfg.XboxIp
        Save-DeployConfig -Config $deployCfg
        Register-XboxIp -XboxIp $deployCfg.XboxIp
        $shown = if ([string]::IsNullOrWhiteSpace($deployCfg.XboxIp)) { '(not set)' } else { $deployCfg.XboxIp }
        Write-Host ('  xbox -> {0}' -f $shown) -ForegroundColor Green
        continue
    }
    if ($chosen -eq 'watson') {
        try { Invoke-Watson -XboxIp $deployCfg.XboxIp }
        catch { Write-Host ('  {0}' -f $_.Exception.Message) -ForegroundColor Red }
        continue
    }

    try {
        if ($chosen -eq 'dist') {
            Invoke-DistBuild -Opt (Resolve-Optimize)
        }
        else {
            $opt = Resolve-Optimize

            # xapi-smoke can target the HDD utility drive (mount + format) or a
            # plain boot disc. Other samples ignore HDD flags.
            $useNoHdd = [bool]$NoHdd
            if ($chosen.Target -eq 'xapi-smoke' -and -not $NoHdd) {
                $ans = Read-Host '  Mount + format HDD utility drive? (recommended for kit) [Y/n]'
                if ($ans -match '^\s*[nN]') { $useNoHdd = $true }
            }

            Invoke-SampleAction -Chosen $chosen -Opt $opt -UseNoHdd:$useNoHdd -Config $deployCfg
        }
    }
    catch {
        Write-Host ''
        Write-Host ('BUILD FAILED: {0}' -f $_.Exception.Message) -ForegroundColor Red
    }

    Write-Host ''
    [void](Read-Host 'Press Enter to return to the menu')
}
