#include "xbox/kernel.h"
#include "xbox/xbox.h"

void xbox_trace(const char *stage)
{
    DbgPrint("RXDK trace: %s\n", stage);
}

void xbox_trace_ptr(const char *label, const void *ptr)
{
    DbgPrint("RXDK trace: %s %p\n", label, ptr);
}

void xbox_trace_enter_main(void)
{
    xbox_trace("crt0 -> main");
}
