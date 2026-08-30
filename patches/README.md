# Vendor patches

Small, targeted fixes applied on top of the pinned `vendor/llvm-project` submodule
(upstream `llvm/llvm-project`), which we can't commit into directly. Each `*.patch`
is a `git diff` rooted at the llvm-project tree and is applied by
`scripts/init-submodules.ps1` after the submodule is checked out and cleaned. The
apply is idempotent (already-applied patches are skipped), so re-running init is safe.

## Patches

- **`libunwind-eh-frame-pe-discovery.patch`** — libunwind's baremetal DWARF frame
  discovery reads the `__eh_frame_start`/`__eh_frame_end` bracket markers to find the
  merged `.eh_frame`. Under lld-link (PE/COFF) those markers only enclose the directly
  linked object's `.eh_frame`; every archive (libc++/libunwind) FDE lands outside, so
  unwinding finds almost nothing and **every C++ throw terminates**. `.eh_frame` is one
  contiguous PE section, so this patches `findUnwindSections` to recover the section's
  true length from the PE section table at runtime (imagebld keeps the PE headers in the
  XBE at a fixed base). The start marker is still correct; only the length is recomputed.
  Verified on xemu via `tests/RxdkTests` (`_Unwind_Backtrace` walks frames; throw/catch/
  unwind/rethrow all pass).
