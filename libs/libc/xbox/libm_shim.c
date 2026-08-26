/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Small libm gap-fillers.
 *
 * picolibc's __issignalingl (long double) is compiled only under
 * _NEED_FLOAT_HUGE && __HAVE_BUILTIN_ISSIGNALINGL, which our profile doesn't
 * set, yet nexttoward()/nexttowardf() reference it. Provide it here.
 */

int __issignalingl(long double x)
{
#if defined(__has_builtin)
#if __has_builtin(__builtin_issignalingl)
    return __builtin_issignalingl(x);
#else
    (void)x;
    return 0;
#endif
#else
    (void)x;
    return 0;
#endif
}
