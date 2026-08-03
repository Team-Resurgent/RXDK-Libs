// RXDK-added: minimal in-lib stub for the OLE32 per-thread exception-context
// accessor. except.hxx (pulled via sem.hxx/memstm by pchcompos, force-included
// by the dmcompos slice) declares `_ExceptionContext()` and its CTry/CException
// machinery calls it, but the real implementation lived in OLE32. Xbox has no
// OLE32 exception runtime and these paths are dead here, so return a zero-init
// context (a raw buffer, which also avoids CExceptionContext's ctor/dtor -- those
// are OLE32 externals too). Types come from the force-included pchcompos.h.
CExceptionContext& APINOT _ExceptionContext(void)
{
    static char s_ExceptionContextBuf[sizeof(CExceptionContext)] = { 0 };
    return *reinterpret_cast<CExceptionContext*>(s_ExceptionContextBuf);
}
