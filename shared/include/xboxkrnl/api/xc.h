#ifndef XBOXKRNL_API_XC_H
#define XBOXKRNL_API_XC_H

VOID STDCALL XcBlockCrypt
(
    IN ULONG dwCipher,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp
);

VOID STDCALL XcBlockCryptCBC
(
    IN ULONG dwCipher,
    IN ULONG dwInputLength,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp,
    IN PUCHAR pbFeedback
);

ULONG STDCALL XcCryptService
(
    IN ULONG dwOp,
    IN PVOID pArgs
);

VOID STDCALL XcDESKeyParity
(
    IN OUT PUCHAR pbKey,
    IN ULONG dwKeyLength
);

VOID STDCALL XcHMAC
(
    IN PUCHAR pbKey,
    IN ULONG dwKeyLength,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength,
    IN PUCHAR pbInput2,
    IN ULONG dwInputLength2,
    OUT PUCHAR pbDigest
);

VOID STDCALL XcKeyTable
(
    IN ULONG dwCipher,
    OUT PUCHAR pbKeyTable,
    IN PUCHAR pbKey
);

ULONG STDCALL XcModExp
(
    OUT PULONG pA,
    IN PULONG pB,
    IN PULONG pC,
    IN PULONG pD,
    IN ULONG dwN
);

ULONG STDCALL XcPKDecPrivate
(
    IN PUCHAR pbPrvKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

ULONG STDCALL XcPKEncPublic
(
    IN PUCHAR pbPubKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

ULONG STDCALL XcPKGetKeyLen
(
    IN PUCHAR pbPubKey
);

VOID STDCALL XcRC4Crypt
(
    IN PUCHAR pbKeyStruct,
    IN ULONG dwInputLength,
    IN OUT PUCHAR pbInput
);

VOID STDCALL XcRC4Key
(
    OUT PUCHAR pbKeyStruct,
    IN ULONG dwKeyLength,
    IN PUCHAR pbKey
);

VOID STDCALL XcSHAFinal
(
    IN PUCHAR pbSHAContext,
    OUT PUCHAR pbDigest
);

VOID STDCALL XcSHAInit
(
    OUT PUCHAR pbSHAContext
);

VOID STDCALL XcSHAUpdate
(
    IN OUT PUCHAR pbSHAContext,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength
);

#if !defined(RXDK_LIBXAPI_BUILD)
VOID STDCALL XcUpdateCrypto
(
    IN PCRYPTO_VECTOR pNewVector,
    OUT PCRYPTO_VECTOR pROMVector OPTIONAL
);
#endif

BOOLEAN STDCALL XcVerifyPKCS1Signature
(
    IN PUCHAR pbSig,
    IN PUCHAR pbPubKey,
    IN PUCHAR pbDigest
);

#if !defined(RXDK_LIBXAPI_BUILD)
XBAPI ANSI_STRING XeImageFileName[1];

NTSTATUS STDCALL XeLoadSection
(
    IN PXBE_SECTION_HEADER Section
);

XBAPI UCHAR XePublicKeyData[284];

NTSTATUS STDCALL XeUnloadSection
(
    IN OUT PXBE_SECTION_HEADER Section
);
#endif

#endif
