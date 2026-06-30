#ifndef XBOXKRNL_API_XC_H
#define XBOXKRNL_API_XC_H

XBAPI VOID STDCALL XcBlockCrypt
(
    IN ULONG dwCipher,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp
);

XBAPI VOID STDCALL XcBlockCryptCBC
(
    IN ULONG dwCipher,
    IN ULONG dwInputLength,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp,
    IN PUCHAR pbFeedback
);

XBAPI ULONG STDCALL XcCryptService
(
    IN ULONG dwOp,
    IN PVOID pArgs
);

XBAPI VOID STDCALL XcDESKeyParity
(
    IN OUT PUCHAR pbKey,
    IN ULONG dwKeyLength
);

XBAPI VOID STDCALL XcHMAC
(
    IN PUCHAR pbKey,
    IN ULONG dwKeyLength,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength,
    IN PUCHAR pbInput2,
    IN ULONG dwInputLength2,
    OUT PUCHAR pbDigest
);

XBAPI VOID STDCALL XcKeyTable
(
    IN ULONG dwCipher,
    OUT PUCHAR pbKeyTable,
    IN PUCHAR pbKey
);

XBAPI ULONG STDCALL XcModExp
(
    OUT PULONG pA,
    IN PULONG pB,
    IN PULONG pC,
    IN PULONG pD,
    IN ULONG dwN
);

XBAPI ULONG STDCALL XcPKDecPrivate
(
    IN PUCHAR pbPrvKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

XBAPI ULONG STDCALL XcPKEncPublic
(
    IN PUCHAR pbPubKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

XBAPI ULONG STDCALL XcPKGetKeyLen
(
    IN PUCHAR pbPubKey
);

XBAPI VOID STDCALL XcRC4Crypt
(
    IN PUCHAR pbKeyStruct,
    IN ULONG dwInputLength,
    IN OUT PUCHAR pbInput
);

XBAPI VOID STDCALL XcRC4Key
(
    OUT PUCHAR pbKeyStruct,
    IN ULONG dwKeyLength,
    IN PUCHAR pbKey
);

XBAPI VOID STDCALL XcSHAFinal
(
    IN PUCHAR pbSHAContext,
    OUT PUCHAR pbDigest
);

XBAPI VOID STDCALL XcSHAInit
(
    OUT PUCHAR pbSHAContext
);

XBAPI VOID STDCALL XcSHAUpdate
(
    IN OUT PUCHAR pbSHAContext,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength
);

#if !defined(RXDK_LIBXAPI_BUILD)
XBAPI VOID STDCALL XcUpdateCrypto
(
    IN PCRYPTO_VECTOR pNewVector,
    OUT PCRYPTO_VECTOR pROMVector OPTIONAL
);
#endif

XBAPI BOOLEAN STDCALL XcVerifyPKCS1Signature
(
    IN PUCHAR pbSig,
    IN PUCHAR pbPubKey,
    IN PUCHAR pbDigest
);

#if !defined(RXDK_LIBXAPI_BUILD)
XBAPI ANSI_STRING XeImageFileName[1];

XBAPI NTSTATUS STDCALL XeLoadSection
(
    IN PXBE_SECTION_HEADER Section
);

XBAPI UCHAR XePublicKeyData[284];

XBAPI NTSTATUS STDCALL XeUnloadSection
(
    IN OUT PXBE_SECTION_HEADER Section
);
#endif

#endif
