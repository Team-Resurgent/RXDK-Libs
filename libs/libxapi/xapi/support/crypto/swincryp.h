//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       wincrypt.h
//
//  Contents:   Cryptographic API Prototypes and Definitions
//
//----------------------------------------------------------------------------

#pragma once
#define __SWINCRYP_H__


#ifdef __cplusplus
extern "C" {
#endif


BOOL
__attribute__((__stdcall__))
SCryptAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
BOOL
__attribute__((__stdcall__))
SCryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR pszContainer,
    LPCWSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags);
#ifdef UNICODE
#define SCryptAcquireContext  SCryptAcquireContextW
#else
#define SCryptAcquireContext  SCryptAcquireContextA
#endif // !UNICODE


BOOL
__attribute__((__stdcall__))
SCryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags);


BOOL
__attribute__((__stdcall__))
SCryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

BOOL
__attribute__((__stdcall__))
SCryptDuplicateKey(
    HCRYPTKEY hKey,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTKEY * phKey);

BOOL
__attribute__((__stdcall__))
SCryptDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey);


BOOL
__attribute__((__stdcall__))
SCryptDestroyKey(
    HCRYPTKEY hKey);

BOOL
__attribute__((__stdcall__))
SCryptSetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptGetKeyParam(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptSetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptGenRandom(
    HCRYPTPROV hProv,
    DWORD dwLen,
    BYTE *pbBuffer);

BOOL
__attribute__((__stdcall__))
SCryptGetUserKey(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey);

BOOL
__attribute__((__stdcall__))
SCryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

BOOL
__attribute__((__stdcall__))
SCryptImportKey(
    HCRYPTPROV hProv,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey);

BOOL
__attribute__((__stdcall__))
SCryptEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen);

BOOL
__attribute__((__stdcall__))
SCryptDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen);

BOOL
__attribute__((__stdcall__))
SCryptCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash);

BOOL
__attribute__((__stdcall__))
SCryptDuplicateHash(
    HCRYPTHASH hHash,
    DWORD *pdwReserved,
    DWORD dwFlags,
    HCRYPTHASH * phHash);

BOOL
__attribute__((__stdcall__))
SCryptHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptHashSessionKey(
    HCRYPTHASH hHash,
    HCRYPTKEY hKey,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptGetHashValue(
    HCRYPTHASH hHash,
    DWORD dwFlags,
    BYTE *pbHash,
    DWORD *pdwHashLen);

BOOL
__attribute__((__stdcall__))
SCryptDestroyHash(
    HCRYPTHASH hHash);

BOOL
__attribute__((__stdcall__))
SCryptSignHashA(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);

BOOL
__attribute__((__stdcall__))
SCryptSignHashW(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR sDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen);

#ifdef UNICODE
#define SCryptSignHash  SCryptSignHashW
#else
#define SCryptSignHash  SCryptSignHashA
#endif // !UNICODE

BOOL
__attribute__((__stdcall__))
SCryptVerifySignatureA(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCSTR sDescription,
    DWORD dwFlags);

BOOL
__attribute__((__stdcall__))
SCryptVerifySignatureW(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR sDescription,
    DWORD dwFlags);

#ifdef UNICODE
#define SCryptVerifySignature  SCryptVerifySignatureW
#else
#define SCryptVerifySignature  SCryptVerifySignatureA
#endif // !UNICODE

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

