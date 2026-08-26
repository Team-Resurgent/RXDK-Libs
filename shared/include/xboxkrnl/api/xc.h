/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Xc* and Xe* kernel imports: the cryptography and secure image-loading services the
 * kernel exposes to titles. Block ciphers (DES/CBC), RC4, SHA-1/HMAC, RSA
 * modular arithmetic and PKCS#1 signature checks, plus the Xe* helpers that load
 * and verify XBE sections. The pointer arguments are raw byte buffers whose
 * exact sizes are algorithm-defined; callers must size them per the cipher in
 * use.
 */

#ifndef XBOXKRNL_API_XC_H
#define XBOXKRNL_API_XC_H

/* Encrypts or decrypts one block. dwCipher selects the algorithm, pbKeyTable is
 * an expanded key from XcKeyTable, dwOp picks encrypt vs decrypt; one block of
 * pbInput is transformed into pbOutput. */
VOID STDCALL XcBlockCrypt(
    IN ULONG dwCipher,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp);

/* CBC-mode block cipher over dwInputLength bytes. pbFeedback holds the
 * IV/chaining block on entry and the updated feedback on exit. */
VOID STDCALL XcBlockCryptCBC(
    IN ULONG dwCipher,
    IN ULONG dwInputLength,
    OUT PUCHAR pbOutput,
    IN PUCHAR pbInput,
    IN PUCHAR pbKeyTable,
    IN ULONG dwOp,
    IN PUCHAR pbFeedback);

/* Dispatches a miscellaneous crypto operation selected by dwOp, with op-specific
 * arguments at pArgs. Returns an op-specific status. */
ULONG STDCALL XcCryptService(
    IN ULONG dwOp,
    IN PVOID pArgs);

/* Fixes the odd-parity bits of a DES key in place. dwKeyLength is in bytes. */
VOID STDCALL XcDESKeyParity(
    IN OUT PUCHAR pbKey,
    IN ULONG dwKeyLength);

/* Computes an HMAC-SHA1 over up to two concatenated input buffers, writing the
 * 20-byte digest to pbDigest. Pass length 0 for the second buffer if unused. */
VOID STDCALL XcHMAC(
    IN PUCHAR pbKey,
    IN ULONG dwKeyLength,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength,
    IN PUCHAR pbInput2,
    IN ULONG dwInputLength2,
    OUT PUCHAR pbDigest);

/* Expands the raw key pbKey into the scheduled key table pbKeyTable used by the
 * XcBlockCrypt* routines for the given dwCipher. */
VOID STDCALL XcKeyTable(
    IN ULONG dwCipher,
    OUT PUCHAR pbKeyTable,
    IN PUCHAR pbKey);

/* Big-integer modular exponentiation: pA = (pB ^ pC) mod pD, over dwN 32-bit
 * words. Used by the RSA primitives. Returns nonzero on success. */
ULONG STDCALL XcModExp(
    OUT PULONG pA,
    IN PULONG pB,
    IN PULONG pC,
    IN PULONG pD,
    IN ULONG dwN);

/* RSA private-key operation (decrypt/sign) of pbInput into pbOutput using the
 * private key pbPrvKey. Returns nonzero on success. */
ULONG STDCALL XcPKDecPrivate(
    IN PUCHAR pbPrvKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput);

/* RSA public-key operation (encrypt/verify) of pbInput into pbOutput using the
 * public key pbPubKey. Returns nonzero on success. */
ULONG STDCALL XcPKEncPublic(
    IN PUCHAR pbPubKey,
    IN PUCHAR pbInput,
    OUT PUCHAR pbOutput);

/* Returns the modulus length, in bytes, of the RSA public key pbPubKey. */
ULONG STDCALL XcPKGetKeyLen(
    IN PUCHAR pbPubKey);

/* RC4 keystream transform of dwInputLength bytes in place. pbKeyStruct is an
 * RC4 state initialized by XcRC4Key. Encrypt and decrypt are the same op. */
VOID STDCALL XcRC4Crypt(
    IN PUCHAR pbKeyStruct,
    IN ULONG dwInputLength,
    IN OUT PUCHAR pbInput);

/* Initializes RC4 state pbKeyStruct from the dwKeyLength-byte key pbKey. */
VOID STDCALL XcRC4Key(
    OUT PUCHAR pbKeyStruct,
    IN ULONG dwKeyLength,
    IN PUCHAR pbKey);

/* Finishes a SHA-1 digest, writing the 20-byte result to pbDigest and
 * consuming the context pbSHAContext. */
VOID STDCALL XcSHAFinal(
    IN PUCHAR pbSHAContext,
    OUT PUCHAR pbDigest);

/* Initializes a SHA-1 context in pbSHAContext prior to XcSHAUpdate. */
VOID STDCALL XcSHAInit(
    OUT PUCHAR pbSHAContext);

/* Feeds dwInputLength more bytes into the running SHA-1 context. */
VOID STDCALL XcSHAUpdate(
    IN OUT PUCHAR pbSHAContext,
    IN PUCHAR pbInput,
    IN ULONG dwInputLength);

#if !defined(RXDK_LIBXAPI_BUILD)
/* Re-keys the kernel's crypto vector with pNewVector; the prior ROM vector is
 * returned in pROMVector when supplied. Privileged; used by update flows. */
VOID STDCALL XcUpdateCrypto(
    IN PCRYPTO_VECTOR pNewVector,
    OUT PCRYPTO_VECTOR pROMVector OPTIONAL);
#endif

/* Verifies a PKCS#1 RSA signature pbSig over pbDigest against public key
 * pbPubKey. Returns TRUE if the signature is valid. */
BOOLEAN STDCALL XcVerifyPKCS1Signature(
    IN PUCHAR pbSig,
    IN PUCHAR pbPubKey,
    IN PUCHAR pbDigest);

#if !defined(RXDK_LIBXAPI_BUILD)
/* The running title's image file name, as an ANSI_STRING. */
XBAPI ANSI_STRING XeImageFileName[1];

/* Loads (maps and decrypts as needed) the XBE section described by Section.
 * Returns an NTSTATUS. */
NTSTATUS STDCALL XeLoadSection(
    IN PXBE_SECTION_HEADER Section);

/* The 284-byte public key blob used to verify XBE image signatures. */
XBAPI UCHAR XePublicKeyData[284];

/* Unloads a previously loaded XBE section, updating its reference state. */
NTSTATUS STDCALL XeUnloadSection(
    IN OUT PXBE_SECTION_HEADER Section);
#endif

#endif
