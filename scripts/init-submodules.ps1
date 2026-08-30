#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $RepoRoot

# core.longpaths must be passed on the command line -- a repo-level setting does NOT
# propagate to the child git processes that check out submodules, and llvm-project's
# tree holds deep paths that can otherwise fail with "Filename too long" on Windows.
$lp = @('-c', 'core.longpaths=true')

Write-Host 'Initializing picolibc (full checkout)...'
git @lp submodule update --init vendor/picolibc
if ($LASTEXITCODE -ne 0) { throw "submodule update (picolibc) failed ($LASTEXITCODE)" }

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

# Apply RXDK's tracked patches to the pinned vendor tree. Must run after the checkout -f +
# clean above (they reset the tree). Idempotent: skips a patch that is already applied, so
# re-running init doesn't fail. See patches/README.md.
$patchDir = Join-Path $RepoRoot 'patches'
if (Test-Path $patchDir) {
    Get-ChildItem $patchDir -Filter '*.patch' | Sort-Object Name | ForEach-Object {
        git -C $llvm apply --reverse --check $_.FullName 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  patch already applied: $($_.Name)"
        }
        else {
            git -C $llvm apply $_.FullName
            if ($LASTEXITCODE -ne 0) { throw "failed to apply patch $($_.Name)" }
            Write-Host "  applied patch: $($_.Name)"
        }
    }
}

Write-Host 'Submodule status:'
git submodule status

Write-Host 'llvm-project tree:'
Get-ChildItem (Join-Path $RepoRoot $llvm) -Name | Where-Object { $_ -notmatch '^\.' }
