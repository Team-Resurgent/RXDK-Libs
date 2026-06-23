#ifndef XBOXKRNL_API_XC_H
#define XBOXKRNL_API_XC_H

XBAPI VOID NTAPI XcBlockCrypt
(
    IN ULONG dwCipher,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp
);

XBAPI VOID NTAPI XcBlockCryptCBC
(
    IN ULONG dwCipher,
    IN ULONG dwInputLength,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp,
    IN PUCHAR pbFeedback
);

XBAPI ULONG NTAPI XcCryptService
(
    IN ULONG dwOp,
    IN PVOID pArgs
);

XBAPI VOID NTAPI XcDESKeyParity
(
    IN OUT PUCHAR pbKey,
    IN ULONG dwKeyLength
);

XBAPI VOID NTAPI XcHMAC
(
    IN PUCHAR pbKey,
    IN ULONG dwKeyLength,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength,
    IN PUCHAR pbInput2,
    IN ULONG dwInputLength2,
    OUT PUCHAR pbDigest
);

XBAPI VOID NTAPI XcKeyTable
(
    IN ULONG dwCipher,
    OUT PUCHAR pbKeyTable,
    IN PUCHAR pbKey
);

XBAPI ULONG NTAPI XcModExp
(
    OUT PULONG pA,
    IN PULONG pB,
    IN PULONG pC,
    IN PULONG pD,
    IN ULONG dwN
);

XBAPI ULONG NTAPI XcPKDecPrivate
(
    IN PUCHAR pbPrvKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

XBAPI ULONG NTAPI XcPKEncPublic
(
    IN PUCHAR pbPubKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput
);

XBAPI ULONG NTAPI XcPKGetKeyLen
(
    IN PUCHAR pbPubKey
);

XBAPI VOID NTAPI XcRC4Crypt
(
    IN PUCHAR pbKeyStruct,
    IN ULONG dwInputLength,
    IN OUT PUCHAR pbInput
);

XBAPI VOID NTAPI XcRC4Key
(
    OUT PUCHAR pbKeyStruct,
    IN ULONG dwKeyLength,
    IN PUCHAR pbKey
);

XBAPI VOID NTAPI XcSHAFinal
(
    IN PUCHAR pbSHAContext,
    OUT PUCHAR pbDigest
);

XBAPI VOID NTAPI XcSHAInit
(
    OUT PUCHAR pbSHAContext
);

XBAPI VOID NTAPI XcSHAUpdate
(
    IN OUT PUCHAR pbSHAContext,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength
);

XBAPI VOID NTAPI XcUpdateCrypto
(
    IN PCRYPTO_VECTOR pNewVector,
    OUT PCRYPTO_VECTOR pROMVector OPTIONAL
);

XBAPI BOOLEAN NTAPI XcVerifyPKCS1Signature
(
    IN PUCHAR pbSig,
    IN PUCHAR pbPubKey,
    IN PUCHAR pbDigest
);

XBAPI ANSI_STRING XeImageFileName[1];

XBAPI NTSTATUS NTAPI XeLoadSection
(
    IN PXBE_SECTION_HEADER Section
);

XBAPI UCHAR XePublicKeyData[284];

XBAPI NTSTATUS NTAPI XeUnloadSection
(
    IN OUT PXBE_SECTION_HEADER Section
);

#endif
