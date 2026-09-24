/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * Replaceable global operator new/delete for the OG Xbox libc++.
 *
 * libc++abi's stdlib_new_delete.cpp declares the base `operator new(size_t)` /
 * `operator new[](size_t)` with `_LIBCPP_OVERRIDABLE_FUNCTION`. clang 23 lowers
 * that override-detection mechanism to a section/alias scheme whose base symbol
 * (`__Znwj` / `__ZdlPv` on i686-windows-gnu) does not survive on this freestanding
 * target -- only the aligned (align_val_t) and nothrow overloads emit, so a plain
 * `new int` / `delete p` in title code links undefined. (zig's older clang did
 * emit them, which is why v1.2.3 linked.) Provide the standard malloc/free-backed
 * base set here as strong definitions; they override libc++abi's weak overloads
 * and satisfy the missing symbols. The aligned/nothrow variants stay libc++abi's.
 *
 * See docs/llvm-toolchain-plan.md (the LLVM migration surfaced this).
 */
#include <new>
#include <cstddef>
#include <cstdlib>

namespace {
void *rxdk_allocate(std::size_t size) {
    if (size == 0)
        size = 1;
    for (;;) {
        if (void *p = std::malloc(size))
            return p;
        // Out of memory: run the installed new-handler, or fail per the standard.
        std::new_handler h = std::get_new_handler();
        if (!h) {
#if __cpp_exceptions
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }
        h();
    }
}
} // namespace

void *operator new(std::size_t size) { return rxdk_allocate(size); }
void *operator new[](std::size_t size) { return rxdk_allocate(size); }

void operator delete(void *ptr) noexcept { std::free(ptr); }
void operator delete[](void *ptr) noexcept { std::free(ptr); }
void operator delete(void *ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete[](void *ptr, std::size_t) noexcept { std::free(ptr); }

// MSVC C++ ABI aliases: cl.exe-built XDK libs (and the mscompat test) reference
// the MSVC-mangled operator new/delete. Forward to the Itanium ones above; this
// is name aliasing only (the asm() labels are the MSVC-mangled names, not valid
// C++ identifiers) -- no MSVC EH/RTTI/STL ABI is involved. Matches RXDK-360's
// runtime/xbox/cxxrt.cpp.
extern "C++" {
__attribute__((weak)) void *rxdk_msvc_new(std::size_t n) asm("??2@YAPAXI@Z");
void *rxdk_msvc_new(std::size_t n) { return ::operator new(n); }

__attribute__((weak)) void *rxdk_msvc_new_nothrow(std::size_t n, const void *) asm("??2@YAPAXIABUnothrow_t@std@@@Z");
void *rxdk_msvc_new_nothrow(std::size_t n, const void *) { return std::malloc(n ? n : 1); }

__attribute__((weak)) void *rxdk_msvc_newa(std::size_t n) asm("??_U@YAPAXI@Z");
void *rxdk_msvc_newa(std::size_t n) { return ::operator new[](n); }

void rxdk_msvc_del(void *p) noexcept asm("??3@YAXPAX@Z");
void rxdk_msvc_del(void *p) noexcept { ::operator delete(p); }

void rxdk_msvc_dela(void *p) noexcept asm("??_V@YAXPAX@Z");
void rxdk_msvc_dela(void *p) noexcept { ::operator delete[](p); }
}
