#ifndef RXDK_XNET_COMPAT_H
#define RXDK_XNET_COMPAT_H

/*
 * RXDK reconstruction header for libxnet.
 *
 * The leak's xnet SOURCE is an older API generation, but its matching private
 * winsock headers are missing from the leak. The public winsockx.h we have is a
 * SLIMMED winsock2.h (title-facing: raw sockets, full hostent/servent, several
 * XP1_/PFL_ protocol flags and the WSAPROTOCOL_INFOW struct were stripped). The
 * xnet IMPLEMENTATION (getxbyy.c / enumprot.c / sockinit.c / dhcp.c) was built
 * against the FULL winsock2.h plus an older private xnet config header.
 *
 * This header supplies exactly those omitted/older definitions. The standard
 * winsock pieces are copied verbatim from the leak's full winsock2.h
 * (public/wsdk/inc/winsock2.h); the XnetInit* config is reconstructed from the
 * source's usage, with the XNET_ADDR_* flags anchored to the public
 * XNET_GET_XNADDR_* values in winsockx.h.
 *
 * Included from winsockp.h, immediately after <winsockx.h> (so GUID / the base
 * winsock types are already defined).
 */

/* MinGW spelling some headers expect. */
#ifndef WINBOOL
#define WINBOOL BOOL
#endif

/* --- standard winsock1/2 bits the slimmed winsockx.h omits --------------- */

#ifndef SOCK_RAW
#define SOCK_RAW        3               /* raw-protocol interface */
#endif

#ifndef RXDK_XNET_HOSTENT_DEFINED
#define RXDK_XNET_HOSTENT_DEFINED
struct hostent {
    char    FAR * h_name;               /* official name of host */
    char    FAR * FAR * h_aliases;      /* alias list */
    short   h_addrtype;                 /* host address type */
    short   h_length;                   /* length of address */
    char    FAR * FAR * h_addr_list;    /* list of addresses */
#ifndef h_addr
#define h_addr  h_addr_list[0]          /* address, for backward compat */
#endif
};
struct servent {
    char    FAR * s_name;               /* official service name */
    char    FAR * FAR * s_aliases;      /* alias list */
    short   s_port;                     /* port # */
    char    FAR * s_proto;              /* protocol to use */
};
struct protoent {
    char    FAR * p_name;               /* official protocol name */
    char    FAR * FAR * p_aliases;      /* alias list */
    short   p_proto;                    /* protocol # */
};
#endif /* RXDK_XNET_HOSTENT_DEFINED */

/* WSAPROTOCOL_INFOW / chain (WSAEnumProtocols, enumprot.c) */
#ifndef MAX_PROTOCOL_CHAIN
#define MAX_PROTOCOL_CHAIN 7
#endif
#ifndef WSAPROTOCOL_LEN
#define WSAPROTOCOL_LEN  255
#endif
#ifndef RXDK_XNET_WSAPROTOCOL_DEFINED
#define RXDK_XNET_WSAPROTOCOL_DEFINED
typedef struct _WSAPROTOCOLCHAIN {
    int   ChainLen;
    DWORD ChainEntries[MAX_PROTOCOL_CHAIN];
} WSAPROTOCOLCHAIN, FAR * LPWSAPROTOCOLCHAIN;
typedef struct _WSAPROTOCOL_INFOW {
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID  ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int   iVersion;
    int   iAddressFamily;
    int   iMaxSockAddr;
    int   iMinSockAddr;
    int   iSocketType;
    int   iProtocol;
    int   iProtocolMaxOffset;
    int   iNetworkByteOrder;
    int   iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    WCHAR szProtocol[WSAPROTOCOL_LEN + 1];
} WSAPROTOCOL_INFOW, FAR * LPWSAPROTOCOL_INFOW;
#endif /* RXDK_XNET_WSAPROTOCOL_DEFINED */

/* dwProviderFlags (PFL_*) and dwServiceFlags1 (XP1_*) bits, byte order,
   security scheme, chain length sentinel -- from winsock2.h. */
#ifndef BASE_PROTOCOL
#define BASE_PROTOCOL                       1
#endif
#ifndef PFL_MULTIPLE_PROTO_ENTRIES
#define PFL_MULTIPLE_PROTO_ENTRIES          0x00000001
#define PFL_RECOMMENDED_PROTO_ENTRY         0x00000002
#define PFL_HIDDEN                          0x00000004
#define PFL_MATCHES_PROTOCOL_ZERO           0x00000008
#endif
#ifndef XP1_CONNECTIONLESS
#define XP1_CONNECTIONLESS                  0x00000001
#define XP1_GUARANTEED_DELIVERY             0x00000002
#define XP1_GUARANTEED_ORDER                0x00000004
#define XP1_MESSAGE_ORIENTED                0x00000008
#define XP1_PSEUDO_STREAM                   0x00000010
#define XP1_GRACEFUL_CLOSE                  0x00000020
#define XP1_EXPEDITED_DATA                  0x00000040
#define XP1_CONNECT_DATA                    0x00000080
#define XP1_DISCONNECT_DATA                 0x00000100
#define XP1_SUPPORT_BROADCAST               0x00000200
#define XP1_SUPPORT_MULTIPOINT              0x00000400
#define XP1_MULTIPOINT_CONTROL_PLANE        0x00000800
#define XP1_MULTIPOINT_DATA_PLANE           0x00001000
#define XP1_QOS_SUPPORTED                   0x00002000
#define XP1_INTERRUPT                       0x00004000
#define XP1_UNI_SEND                        0x00008000
#define XP1_UNI_RECV                        0x00010000
#define XP1_IFS_HANDLES                     0x00020000
#define XP1_PARTIAL_MESSAGE                 0x00040000
#endif
/* IP-level socket options + multicast request (ws2tcpip.h; sockopt.c, mcast.c).
   The slimmed winsockx.h drops these. Standard winsock2 values. */
#ifndef IP_OPTIONS
#define IP_OPTIONS          1
#define IP_HDRINCL          2
#define IP_TOS              3
#define IP_TTL              4
#define IP_MULTICAST_IF     9
#define IP_MULTICAST_TTL    10
#define IP_MULTICAST_LOOP   11
#define IP_ADD_MEMBERSHIP   12
#define IP_DROP_MEMBERSHIP  13
#define IP_DONTFRAGMENT     14
#endif
#ifndef RXDK_XNET_IP_MREQ_DEFINED
#define RXDK_XNET_IP_MREQ_DEFINED
struct ip_mreq {
    struct in_addr imr_multiaddr;   /* IP multicast address of group */
    struct in_addr imr_interface;   /* local IP address of interface */
};
#endif

#ifndef BIGENDIAN
#define BIGENDIAN                           0x0000
#endif
#ifndef SECURITY_PROTOCOL_NONE
#define SECURITY_PROTOCOL_NONE              0x0000
#endif

/* --- older private xnet config (reconstructed from source usage) --------- */

/* XnetInitialize() optional parameters. Fields per sockinit.c usage; a title
   that passes this sets structSize = sizeof(XnetInitParams). Our sample passes
   NULL (defaults), so only the field set/names matter for the build. */
#ifndef RXDK_XNET_INITPARAMS_DEFINED
#define RXDK_XNET_INITPARAMS_DEFINED
typedef struct _XnetInitParams {
    DWORD structSize;
    DWORD privatePoolSize;
    DWORD enetRecvQLength;
    DWORD maxIPReassemblySize;
    DWORD maxIPReassemblyDgrams;
    DWORD defaultSocketRecvBufSize;
    DWORD defaultSocketSendBufSize;
    DWORD defaultIpTTL;
    DWORD defaultIpTOS;
    DWORD maxSockets;
    DWORD configFlags;
} XnetInitParams;
#endif

/* configFlags bits */
#ifndef XNET_INITFLAG_FORCE_AUTONET
#define XNET_INITFLAG_FORCE_AUTONET     0x0001
#endif

/* netutil.h defines MAlloc/MAlloc0/Free only for the user-mode _XNET_SERVICE
   build (LocalAlloc). Our kernel-runtime build routes them to the pooled
   allocator (SysAlloc/SysFree, from nettypes.h -- in scope at every call site
   via the precomp). 'tenX' = pool tag. */
#if !defined(_XNET_SERVICE)
#ifndef MAlloc
#define MAlloc(size)   SysAlloc((size), 'tenX')
#define MAlloc0(size)  SysAlloc0((size), 'tenX')
#define Free(ptr)      SysFree(ptr)
#endif
#endif

/* Address-configuration state (returned by IpGetBestAddress / reported to the
   title). Values anchored to the public XNET_GET_XNADDR_* flags in winsockx.h
   so the title-visible result is correct; XNET_HAS_GATEWAY is an OR-able bit. */
#ifndef XNET_ADDR_NONE
#define XNET_ADDR_NONE      0x01    /* == XNET_GET_XNADDR_NONE     */
#define XNET_ADDR_LOOPBACK  0x02    /* == XNET_GET_XNADDR_ETHERNET */
#define XNET_ADDR_STATIC    0x04    /* == XNET_GET_XNADDR_STATIC   */
#define XNET_ADDR_DHCP      0x08    /* == XNET_GET_XNADDR_DHCP     */
#define XNET_ADDR_AUTOIP    0x10    /* == XNET_GET_XNADDR_AUTO     */
#define XNET_HAS_GATEWAY    0x20    /* == XNET_GET_XNADDR_GATEWAY  */
#endif

#endif /* RXDK_XNET_COMPAT_H */
