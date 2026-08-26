/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Prototypes for the S3TC block encode/decode wrappers implemented in
 * s3tchelp.cpp.
 */

void XXEncodeBlockRGB(S3TC_COLOR colorSrc[S3TC_BLOCK_PIXELS], S3TCBlockRGB *blockdst);
void XXDecodeBlockRGB(S3TCBlockRGB *blocksrc, S3TC_COLOR colordst[S3TC_BLOCK_PIXELS]);
