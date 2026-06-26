/*
 * C synchronous signals for Xbox (ISO C <signal.h> + the POSIX mask helpers).
 *
 * There is no asynchronous signal delivery on Xbox (no OS signal source), but
 * signal()/raise() are well-defined synchronously: signal() registers a
 * handler, raise() invokes it on the calling thread. sigprocmask() blocks
 * signals; a raise of a blocked signal becomes pending and is delivered when
 * unblocked. The default disposition is a no-op (we never terminate the title
 * from a stray raise; abort() does its own _exit after raising SIGABRT).
 */

/* sigset_t/sigprocmask/SIG_BLOCK are POSIX (gated behind __POSIX_VISIBLE,
   off under -std=c23). signal()/raise() are ISO C and always visible. */
#define _GNU_SOURCE 1

#include <errno.h>
#include <signal.h>
#include <stddef.h>

#ifndef NSIG
#define NSIG 32
#endif

static _sig_func_ptr g_handlers[NSIG]; /* 0 == SIG_DFL */
static sigset_t g_blocked;
static sigset_t g_pending;

static int valid_sig(int sig)
{
    return sig > 0 && sig < NSIG;
}

static void deliver(int sig)
{
    _sig_func_ptr h = g_handlers[sig];
    if (h == SIG_DFL || h == SIG_IGN)
        return;
    g_handlers[sig] = SIG_DFL; /* C: reset to default before the handler runs */
    h(sig);
}

_sig_func_ptr signal(int sig, _sig_func_ptr handler)
{
    _sig_func_ptr prev;
    if (!valid_sig(sig)) {
        errno = EINVAL;
        return SIG_ERR;
    }
    prev = g_handlers[sig];
    g_handlers[sig] = handler;
    return prev;
}

int raise(int sig)
{
    sigset_t bit;
    if (!valid_sig(sig)) {
        errno = EINVAL;
        return -1;
    }
    bit = (sigset_t)1 << sig;
    if (g_blocked & bit) {
        g_pending |= bit; /* deferred until unblocked */
        return 0;
    }
    deliver(sig);
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (oldset)
        *oldset = g_blocked;

    if (set) {
        switch (how) {
        case SIG_BLOCK:   g_blocked |= *set; break;
        case SIG_UNBLOCK: g_blocked &= ~*set; break;
        case SIG_SETMASK: g_blocked = *set; break;
        default: errno = EINVAL; return -1;
        }
        /* deliver any pending signals that just became unblocked */
        for (int s = 1; s < NSIG; ++s) {
            sigset_t bit = (sigset_t)1 << s;
            if ((g_pending & bit) && !(g_blocked & bit)) {
                g_pending &= ~bit;
                deliver(s);
            }
        }
    }
    return 0;
}

int sigpending(sigset_t *set)
{
    if (set)
        *set = g_pending;
    return 0;
}
