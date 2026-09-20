# Build the RXDK-Libs distribution libraries with the RXDK LLVM toolchain, via the
# .NET engine's `build-sdk` (RXDK-Tools). Replaces the old `zig build` path: the
# engine compiles + archives every SDK lib (byte-identical to the previous zig-LLVM
# build) from build/sdk/*.json, assembles libcompat, and stages the public headers.
param(
    [string]$Root = (Join-Path $PSScriptRoot '..'),
    [ValidateSet('libs')]
    [string]$Target = 'libs',
    [ValidateSet('Debug', 'ReleaseSmall')]
    [string]$Optimize = 'Debug',
    [switch]$SkipSubmoduleCheck,
    [switch]$NoHeaders
)

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
Set-Location -LiteralPath $Root

function Require-Submodules {
    param([string]$RepoRoot)
    $required = @(
        (Join-Path $RepoRoot 'vendor\picolibc\libc\include\stdio.h')
        (Join-Path $RepoRoot 'vendor\llvm-project\libcxx\include\iostream')
    )
    foreach ($path in $required) {
        if (-not (Test-Path -LiteralPath $path)) {
            throw "Missing vendor sources: $path`nRun: git submodule update --init --recursive"
        }
    }
}

# Locate the RXDK engine CLI: RXDK_CLI (a built Rxdk.Cli.dll) wins; otherwise a
# sibling RXDK-Tools checkout's built dll, otherwise `dotnet run` against it.
function Resolve-EngineCli {
    param([string]$RepoRoot)
    if ($env:RXDK_CLI -and (Test-Path -LiteralPath $env:RXDK_CLI)) {
        return @{ Kind = 'dll'; Path = $env:RXDK_CLI }
    }
    $toolsRoot = Join-Path (Split-Path $RepoRoot -Parent) 'RXDK-Tools'
    $proj = Join-Path $toolsRoot 'src\Rxdk.Cli'
    if (Test-Path -LiteralPath $proj) {
        $dll = Get-ChildItem -LiteralPath $proj -Recurse -Filter 'Rxdk.Cli.dll' -ErrorAction SilentlyContinue |
               Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($dll) { return @{ Kind = 'dll'; Path = $dll.FullName } }
        return @{ Kind = 'project'; Path = $proj }
    }
    throw "RXDK engine CLI not found. Set RXDK_CLI to a built Rxdk.Cli.dll, or check out RXDK-Tools next to RXDK-Libs."
}

if (-not $SkipSubmoduleCheck) {
    Require-Submodules -RepoRoot $Root
}

$cfg = if ($Optimize -eq 'ReleaseSmall') { 'Release' } else { 'Debug' }
$cli = Resolve-EngineCli -RepoRoot $Root
$sdkArgs = @('build-sdk', '--repo-root', $Root, '--config', $cfg)
if ($NoHeaders) { $sdkArgs += '--no-headers' }

Write-Host "==> engine build-sdk --config $cfg" -ForegroundColor Cyan
if ($cli.Kind -eq 'dll') {
    & dotnet $cli.Path @sdkArgs
}
else {
    & dotnet run --project $cli.Path -- @sdkArgs
}
if ($LASTEXITCODE -ne 0) { throw "engine build-sdk failed (exit $LASTEXITCODE)" }

Write-Host "Done ($Target, $Optimize)." -ForegroundColor Green
