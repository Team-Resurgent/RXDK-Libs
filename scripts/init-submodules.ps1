#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $RepoRoot

# core.longpaths must be passed on the command line -- a repo-level setting does NOT
# propagate to the child git processes that check out submodules. Without it the
# xbox_leak_may_2020 submodule, whose tree holds very deep installer paths, fails to
# check out under a deep worktree path (e.g. .claude/worktrees/<name>/vendor/...) with
# "Filename too long" on Windows.
$lp = @('-c', 'core.longpaths=true')

Write-Host 'Initializing picolibc + xbox_leak_may_2020 (full checkout)...'
git @lp submodule update --init vendor/picolibc vendor/xbox_leak_may_2020
if ($LASTEXITCODE -ne 0) { throw "submodule update (picolibc/xbox_leak_may_2020) failed ($LASTEXITCODE)" }

# llvm-project is only used to compile the C++ runtime (libcpp) from three source trees:
# libcxx, libcxxabi and libunwind. The rest of the monorepo (clang, llvm, mlir, ...) is
# ~100k files we never build and that can exhaust the disk. Configure the sparse cone in
# the module store BEFORE the working tree is materialised so a full checkout never runs.
$llvm = 'vendor/llvm-project'
$cone = @('libcxx', 'libcxxabi', 'libunwind')

Write-Host "Configuring llvm-project sparse checkout ($($cone -join ' '))..."
git submodule init $llvm

$moduleDir = (git rev-parse --git-path "modules/$llvm").Trim()
if (Test-Path $moduleDir) {
    # Pre-seed the sparse cone so the very first checkout is already trimmed.
    git -C $moduleDir config core.sparseCheckout true
    git -C $moduleDir config core.sparseCheckoutCone true
    $info = Join-Path $moduleDir 'info'
    if (-not (Test-Path $info)) { New-Item -ItemType Directory -Force $info | Out-Null }
    $patterns = @('/*', '!/*/') + ($cone | ForEach-Object { "/$_/" })
    Set-Content -Encoding ascii -Path (Join-Path $info 'sparse-checkout') -Value $patterns
}

git @lp submodule update --init $llvm
if ($LASTEXITCODE -ne 0) { throw "submodule update (llvm-project) failed ($LASTEXITCODE)" }

# Enforce the cone (trims anything that slipped in) and pin to the recorded commit --
# a resumed or partially-failed checkout can otherwise leave the wrong libcxx revision,
# which then mismatches shared/libcxx/include and fails the libcpp build.
git -C $llvm sparse-checkout init --cone
git -C $llvm sparse-checkout set @cone
$pin = ((git ls-tree HEAD $llvm) -split '\s+')[2]
git -C $llvm @lp checkout -f $pin
git -C $llvm clean -ffdq | Out-Null

Write-Host 'Submodule status:'
git submodule status

Write-Host 'llvm-project tree:'
Get-ChildItem (Join-Path $RepoRoot $llvm) -Name | Where-Object { $_ -notmatch '^\.' }
