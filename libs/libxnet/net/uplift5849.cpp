// RXDK 5849 uplift: XNet connect + QoS-lookup entry points absent from the Jan-2002 leak net stack.
//
// XNetConnect/XNetGetConnectStatus are REAL under the leak's model: 5849 split the security-
// association handshake into an explicit start/poll pair, but the leak's stack establishes the SA
// implicitly on first send to a secure address and blocks/retries internally -- the API contract
// ("you may now send to this peer") is already satisfied, so Connect succeeds as a no-op and the
// status reports CONNECTED.
//
// The QoS lookups are a PROTOCOL BOUNDARY, not unfinished work: the 5849 peer-probe wire format is
// documented nowhere in the leak, so a probe engine could not interoperate with anything. They
// report "no result" (the same shape titles see when every probe times out); revisit only if the
// 5849 QoS protocol is ever documented (or Insignia defines a replacement).

#include "xnp.h"

extern "C" {

INT WSAAPI XNetConnect(const IN_ADDR)
{
    // the leak's secure stack connects lazily on first send; nothing to start
    return 0;
}

DWORD WSAAPI XNetGetConnectStatus(const IN_ADDR)
{
    return 0x0002; // XNET_CONNECT_STATUS_CONNECTED (sends to the peer are permitted now)
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

// 5849 broadcast title-version mismatch detection. The leak's stack never tracks
// broadcast version mismatches, so there is never anything to report.
DWORD WSAAPI XNetGetBroadcastVersionStatus(BOOL /*fReset*/)
{
    return 0;
}

}
