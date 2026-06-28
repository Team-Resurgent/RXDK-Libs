#include "bridge_k32.h"
/* RXDK debug RIP stubs for retail libxapi (vendor xdbg.h macros). */

#include <xboxkrnl/xboxdef.h>

void RIP(void)
{
}

void RIP_ON_NOT_TRUE(const char *api, int expr)
{
    (void)api;
    (void)expr;
}

void RIP_ON_NOT_TRUE_WITH_MESSAGE(int expr, const char *msg)
{
    (void)expr;
    (void)msg;
}
