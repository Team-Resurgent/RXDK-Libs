//===----------------------------------------------------------------------===//
// RXDK-Libs wrapper for libc++abi's <__cxxabi_config.h>.
//
// build/generated/libcxx is first on the libcpp include path, and libc++abi's
// cxxabi.h pulls this header in with angle brackets (#include <__cxxabi_config.h>),
// so this wrapper is found first. We include the real vendored header via
// #include_next, then correct one target-ABI detail it gets wrong for our build.
//
// We compile libc++/libc++abi with -U_WIN32 to take the Itanium/POSIX code
// paths, but clang still emits i386 C++ member functions -- including exception
// object destructors -- as __thiscall (this in ECX). The vendored header keys
// _LIBCXXABI_DTOR_FUNC purely off _WIN32, so with -U_WIN32 it degrades to cdecl
// and libc++abi then calls ~T() passing thrown_object on the stack while the
// real destructor reads `this` from a stale ECX -> wild pointer (e.g. the
// reference-counted message string of a thrown std::runtime_error).
//
// Forcing the qualifier here keeps the fix in the owned tree -- no patch to the
// vendored llvm-project submodule, so it survives a fresh clone / submodule
// update. Gated on _LIBCXXABI_FORCE_THISCALL_DTOR (set in libs/libcpp/build.zig),
// which the libc++ headers (shared/libcxx/.../exception_ptr.h) also honor so the
// two declarations of __cxa_init_primary_exception agree.
//===----------------------------------------------------------------------===//

#include_next <__cxxabi_config.h>

#if defined(_LIBCXXABI_FORCE_THISCALL_DTOR)
#undef _LIBCXXABI_DTOR_FUNC
#define _LIBCXXABI_DTOR_FUNC __thiscall
#endif
