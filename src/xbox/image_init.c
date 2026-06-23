#include <stddef.h>
#include <string.h>

#include "xbox/xbox.h"

/* RXDK_* defines come from -include zig-out/link/<sample>_image_init.h at build time. */
#ifndef RXDK_IMAGE_ANCHOR_RVA
#define RXDK_IMAGE_ANCHOR_RVA 0u
#define RXDK_DATA_BSS_START_RVA 0u
#define RXDK_DATA_BSS_SIZE 0u
#endif

extern char xbox_image_anchor;

void xbox_zero_uninitialized_data(void)
{
#if RXDK_DATA_BSS_SIZE > 0
    char *const anchor = &xbox_image_anchor;
    char *const start = anchor + (ptrdiff_t)(RXDK_DATA_BSS_START_RVA - RXDK_IMAGE_ANCHOR_RVA);
    memset(start, 0, (size_t)RXDK_DATA_BSS_SIZE);
#endif
}
