/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Thread-control-block hooks the toolchain references but that the emulated-TLS
 * model (see emutls.c) does not use: __x86_tls_tcb_offset stays zero and
 * __set_tcb is a no-op.
 */

#include <stdint.h>

uintptr_t __x86_tls_tcb_offset;

void __set_tcb(void *tcb)
{
    (void)tcb;
}
