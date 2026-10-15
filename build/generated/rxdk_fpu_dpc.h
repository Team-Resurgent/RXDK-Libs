/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * RXDK_FPU_SAFE_DPC(Routine): a DPC routine that runs Routine with the FPU/SSE
 * state saved around it. Every DPC an SDK library registers uses it:
 *
 *     KeInitializeDpc(&m_dpc, RXDK_FPU_SAFE_DPC(CFoo::DpcRoutine), this);
 *
 * A DPC runs on top of whatever thread it interrupted, and the kernel does not
 * save that thread's floating-point state for it. x87 or SSE use in the routine
 * -- its own math, an xmm copy the compiler chose, a title callback it calls --
 * would overwrite the interrupted thread's registers (a title's SSE float math)
 * or bugcheck with ATTEMPTED_FPU_USE_FROM_DPC. RxdkRunDpcFpuSafe (libxapi,
 * k32/fpudpc.c) brackets the call with KeSaveFloatingPointState and
 * KeRestoreFloatingPointState.
 *
 * C++: the macro names the routine where it is used, so a private static member
 * works from inside its class. C: define the wrapper once at file scope with
 * RXDK_FPU_SAFE_DPC_DEFINE(Routine), after Routine is declared.
 *
 * The wrapper only has void * parameters, so this header needs no kernel
 * headers and cannot clash with a library's own declarations. It is placed in
 * .text whatever #pragma code_seg is in force where it is defined, since a DPC
 * routine must stay resident.
 */
#ifndef RXDK_FPU_SAFE_DPC_H
#define RXDK_FPU_SAFE_DPC_H

#ifdef __cplusplus
extern "C" {
#endif
void __attribute__((__stdcall__)) RxdkRunDpcFpuSafe(void *Routine, void *Dpc, void *DeferredContext,
                                 void *SystemArgument1, void *SystemArgument2);
#ifdef __cplusplus
}

/* A DPC routine must be __stdcall: the kernel and RxdkRunDpcFpuSafe both call it that
   way and it must pop its own four arguments. A cdecl one returns with the stack 16
   bytes off; the kernel's DPC loop happens to tolerate that, a caller in C does not
   (it hung titles on iND-BiOS, where those DPCs fire), so it is rejected here. */
/* The parameter types are the library's own (its KDPC may be declared in its own
   namespace), so only the convention and the argument count are checked. */
/* Spelled out both ways, as attributes: some libraries compile with stdcall as the
   default convention (an unannotated function pointer is then the stdcall one), and
   some headers #define __cdecl to nothing. */
template <class... A> constexpr bool RxdkIsStdcallDpc(void (__attribute__((__stdcall__)) *)(A...)) { return sizeof...(A) == 4; }
template <class... A> constexpr bool RxdkIsStdcallDpc(void (__attribute__((__cdecl__)) *)(A...)) { return false; }
template <class T> constexpr bool RxdkIsStdcallDpc(T) { return false; }

template <auto Routine>
__attribute__((section(".text")))
static void __attribute__((__stdcall__)) RxdkFpuSafeDpc(void *Dpc, void *DeferredContext,
                                     void *SystemArgument1, void *SystemArgument2)
{
    static_assert(RxdkIsStdcallDpc(Routine),
                  "RXDK_FPU_SAFE_DPC: a DPC routine must be void __stdcall (PKDPC, PVOID, PVOID, PVOID)");
    RxdkRunDpcFpuSafe((void *)Routine, Dpc, DeferredContext, SystemArgument1, SystemArgument2);
}

#define RXDK_FPU_SAFE_DPC(Routine) ((PKDEFERRED_ROUTINE)RxdkFpuSafeDpc<Routine>)

#else

/* As in C++: the routine must be __stdcall (see above). */
#define RXDK_FPU_SAFE_DPC_DEFINE(Routine)                                              \
    _Static_assert(__builtin_types_compatible_p(__typeof__(&Routine),                  \
                       void (__attribute__((__stdcall__)) *)(struct _KDPC *, void *, void *, void *)),    \
                   "RXDK_FPU_SAFE_DPC: " #Routine " must be void __stdcall "           \
                   "(PKDPC, PVOID, PVOID, PVOID)");                                    \
    __attribute__((section(".text")))                                                  \
    static void __attribute__((__stdcall__)) Routine##_RxdkFpuSafe(void *Dpc, void *DeferredContext,      \
                                                void *SystemArgument1,                 \
                                                void *SystemArgument2)                 \
    {                                                                                  \
        RxdkRunDpcFpuSafe((void *)Routine, Dpc, DeferredContext, SystemArgument1,      \
                          SystemArgument2);                                            \
    }

#define RXDK_FPU_SAFE_DPC(Routine) ((PKDEFERRED_ROUTINE)Routine##_RxdkFpuSafe)

#endif

#endif /* RXDK_FPU_SAFE_DPC_H */
