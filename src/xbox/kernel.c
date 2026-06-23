#include <stdarg.h>
#include <stddef.h>

#include "xbox/kernel.h"

void OutputDebugStringA(const char *lpOutputString)
{
    if (lpOutputString)
        DbgPrint("%s", lpOutputString);
}
