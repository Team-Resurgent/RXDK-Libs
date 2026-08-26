/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Public definitions for the in-memory MSZIP (inflate) decompressor implemented
 * in nfmdeco.c. Declares the NFM_Prepare and NFM_Decompress entry points along
 * with the types and constants callers share with the decompressor.
 */

/* --- types -------------------------------------------------------------- */

#ifndef DIAMONDAPI
#define DIAMONDAPI __cdecl
#endif

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char  BYTE;
#endif

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int UINT;
#endif

#ifndef _ULONG_DEFINED
#define _ULONG_DEFINED
typedef unsigned long  ULONG;
#endif

#ifndef FAR
#ifdef BIT16
#define FAR far
#else
#define FAR
#endif
#endif

#ifndef HUGE
#ifdef BIT16
#define HUGE huge
#else
#define HUGE
#endif
#endif

#ifndef UNALIGNED
#ifdef  NEEDS_ALIGNMENT
#define UNALIGNED __unaligned
#else   /* !NEEDS_ALIGNMENT */
#define UNALIGNED
#endif  /* NEEDS_ALIGNMENT */
#endif  /* UNALIGNED */

#ifndef _MI_MEMORY_DEFINED
#define _MI_MEMORY_DEFINED
typedef void HUGE *  MI_MEMORY;
#endif

/***    PFNALLOC - Memory allocation function for MCI
 *
 *  Entry:
 *      cb - Size in bytes of memory block to allocate
 *
 *  Exit-Success:
 *      Returns !NULL pointer to memory block
 *
 *  Exit-Failure:
 *      Returns NULL; insufficient memory
 */
#ifndef _PFNALLOC_DEFINED
#define _PFNALLOC_DEFINED
typedef MI_MEMORY (_cdecl FAR DIAMONDAPI *PFNALLOC)(ULONG cb);       /* pfnma */
#endif


/***    PFNFREE - Free memory function for MCI
 *
 *  Entry:
 *      pv - Memory block allocated by matching PFNALLOC function
 *
 *  Exit:
 *      Memory block freed.
 */
#ifndef _PFNFREE_DEFINED
#define _PFNFREE_DEFINED
typedef void (_cdecl FAR DIAMONDAPI *PFNFREE)(MI_MEMORY pv);          /* pfnmf */
#endif

/* --- prototypes --------------------------------------------------------- */

extern int NFM_Prepare(void FAR *ctx, BYTE FAR *bfSrc, UINT cbSrc,
        BYTE FAR *bfDest, UINT cbDest);

extern int NFM_Decompress(void FAR *ctx, UINT FAR *pcbDestCount);

extern void *NFMdeco_create(PFNALLOC NFMalloc);

extern void NFMdeco_destroy(void FAR *ctx, PFNFREE NFMfree);

/* --- constants ---------------------------------------------------------- */

/* return codes */

#define     NFMsuccess      0       /* successful completion */
#define     NFMdestsmall    1       /* destination buffer is too small */
#define     NFMoutofmem     2       /* alloc returned an error (NULL) */
#define     NFMinvalid      3       /* source buffer contains bad data */


#ifdef LARGE_STORED_BLOCKS
#define     MAX_GROWTH      12      /* maximum growth of a block */
#else
#define     MAX_GROWTH      8       /* maximum growth of a block */
#endif

/* ----------------------------------------------------------------------- */
