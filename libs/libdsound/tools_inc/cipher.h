/*
 * Portions Copyright (c) Microsoft Corporation - Xbox XDK.
 * Reworked / modified 2026 - Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Team Resurgent's modifications are licensed GPL-3.0-or-later (see LICENSE.md);
 * original Microsoft-authored portions remain (c) Microsoft Corporation.
 */

/*
 * XAudio cipher interface: the LFSR-based encode/decode and key generation
 * used to obfuscate DSP microcode (XAudiopUtility_Encode/Decode/GenerateKey).
 */

#define KEY_SIZE 8
#define PRIVATE_KEY_SEED ((ULONGLONG) 0x7fa49bca49de12ba)

int XAudiopUtility_Encode(
						  PUCHAR pKey,
						  PUCHAR pSrc, 
						  DWORD dwSize, 
						  PUCHAR pDst,
						  BOOL fEmbeddedKey);

int XAudiopUtility_Decode(PUCHAR pKey,
						  PUCHAR pSrc, 
						  DWORD dwSize, 
						  PUCHAR pDst,
						  BOOL fEmbeddedKey);

void XAudiopUtility_GenerateKey(PUCHAR pKey);


