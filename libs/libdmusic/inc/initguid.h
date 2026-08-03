#ifndef _RXDK_DMUSIC_INITGUID_H_
#define _RXDK_DMUSIC_INITGUID_H_
/*
 * initguid.h shim for the Xbox DirectMusic port. Matches the MS pattern: define
 * INITGUID and re-include guiddef.h so DEFINE_GUID flips from an extern
 * declaration to an allocating definition (libxapi/nt/guiddef.h keys off
 * INITGUID). The DirectMusic guids.cpp TUs pull this so the class/interface GUIDs
 * get real storage in this library rather than dangling as externals.
 */
#ifndef INITGUID
#define INITGUID
#endif
#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif
#include <guiddef.h>

#endif /* _RXDK_DMUSIC_INITGUID_H_ */
