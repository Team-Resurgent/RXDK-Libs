# Build the RXDK-Libs distribution libraries with Zig.
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet('libs', 'verify-no-vs')]
    [string]$Target = 'libs',
    [ValidateSet('Debug', 'ReleaseSafe', 'ReleaseFast', 'ReleaseSmall')]
    [string]$Optimize = 'Debug',
    [switch]$SkipSubmoduleCheck
)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
Set-Location -LiteralPath $Root

function Require-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name not found on PATH. Install Zig 0.16+ from https://ziglang.org/download/"
    }
}

function Require-Submodules {
    param([string]$RepoRoot)
    $required = @(
        (Join-Path $RepoRoot 'vendor\picolibc\libc\include\stdio.h')
        (Join-Path $RepoRoot 'vendor\llvm-project\libcxx\include\iostream')
    )
    foreach ($path in $required) {
        if (-not (Test-Path -LiteralPath $path)) {
            throw @"
Missing vendor sources: $path
Run: git submodule update --init --recursive
"@
        }
    }
}

function Invoke-ZigBuild {
    param(
        [string[]]$Step,
        [string]$Opt
    )
    $args = @('build', "-Doptimize=$Opt")
    if ($Step) {
        $args += $Step
    }
    Write-Host "==> zig $($args -join ' ')" -ForegroundColor Cyan
    & zig @args
    if ($LASTEXITCODE -ne 0) {
        throw "zig build failed (exit $LASTEXITCODE)"
    }
}

Require-Command -Name 'zig'
if (-not $SkipSubmoduleCheck) {
    Require-Submodules -RepoRoot $Root
}

switch ($Target) {
    'verify-no-vs' {
        Invoke-ZigBuild -Step @('verify-no-vs') -Opt $Optimize
    }
    'libs' {
        # Default install stages libc/libcpp/libxapi (+ their public headers);
        # the device libs have their own named steps -> build them all so "libs"
        # means every shippable .lib.
        Invoke-ZigBuild -Step @() -Opt $Optimize
        Invoke-ZigBuild -Step @('libd3d8', 'libd3d8i', 'libd3dx8', 'libxgraphics', 'libdsound', 'libxnet', 'libxneto', 'libxmv') -Opt $Optimize
        # Middleware: XACT audio, the Xbox Live client (+ the UIX drop-in UI),
        # XHV voice and DirectMusic. Titles opt in by naming them in
        # <RxdkLibraries>, but they must still ship in the SDK.
        Invoke-ZigBuild -Step @('libxact', 'libxonline', 'libuix', 'libxvoice', 'libdmusic') -Opt $Optimize
    }
}

Write-Host "Done ($Target)." -ForegroundColor Green
