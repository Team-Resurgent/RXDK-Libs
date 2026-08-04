// RXDK 5849 uplift: XNet connect + QoS-lookup entry points that the XDK-5849 matchmaking/connect
// samples call but that are absent from the Jan-2002 leak net stack. Documented STUBS so the Live
// samples link and boot; they report idle/no-result. Must be implemented for real Xbox Live
// (e.g. Insignia) matchmaking. TODO(5849-xnet): implement over the secure-gateway / QoS engine.

#include "xnp.h"

extern "C" {

INT WSAAPI XNetConnect(const IN_ADDR)
{
    return 0;   // treat as "connection started"
}

DWORD WSAAPI XNetGetConnectStatus(const IN_ADDR)
{
    return 0;   // XNET_CONNECT_STATUS_IDLE
}

INT WSAAPI XNetQosLookup(UINT, const XNADDR *[], const XNKID *[], const XNKEY *[], UINT,
                         const IN_ADDR[], const DWORD[], UINT, DWORD, DWORD, WSAEVENT, XNQOS **ppxnqos)
{
    if (ppxnqos) *ppxnqos = NULL;
    return -1;  // SOCKET_ERROR
}

INT WSAAPI XNetQosServiceLookup(DWORD, WSAEVENT, XNQOS **ppxnqos)
{
    if (ppxnqos) *ppxnqos = NULL;
    return -1;
}

INT WSAAPI XNetTsAddrToInAddr(const XNADDR *, DWORD, const XNKID *, IN_ADDR *)
{
    return -1;
}

INT WSAAPI XNetXnAddrToMachineId(const XNADDR *, ULONGLONG *pqwMachineId)
{
    if (pqwMachineId) *pqwMachineId = 0;
    return -1;
}

INT WSAAPI XNetInAddrToServer(const IN_ADDR, IN_ADDR *)
{
    return -1;
}

}
