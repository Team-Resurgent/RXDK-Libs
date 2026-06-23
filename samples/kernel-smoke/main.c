void DbgPrint(const char *fmt, ...);

void _start(void)
{
    DbgPrint("RXDK-LibsZig kernel-smoke OK\n");
    for (;;)
        ;
}
