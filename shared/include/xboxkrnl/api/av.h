/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Av* kernel imports: the low-level audio/video interface. These program the
 * NV2A display controller and the TV encoder directly and are normally driven
 * by D3D; titles rarely call them, but they are the primitives behind display
 * mode selection and framebuffer hand-off across a relaunch.
 */

#ifndef XBOXKRNL_API_AV_H
#define XBOXKRNL_API_AV_H

/* Returns the framebuffer address stashed by AvSetSavedDataAddress (0 if none),
 * used to preserve a splash image across an image relaunch. */
PVOID STDCALL AvGetSavedDataAddress(void);

/* Sends a control option to the TV encoder. RegisterBase is the AV register
 * block; Option/Param are encoder-specific, and the encoder's reply (if any) is
 * written to *Result. */
VOID STDCALL AvSendTVEncoderOption(
    IN PVOID RegisterBase,
    IN ULONG Option,
    IN ULONG Param,
    OUT PULONG Result);

/* Programs the display controller for a new mode. RegisterBase is the AV
 * register block, Step selects the programming phase, DisplayMode encodes the
 * resolution/refresh, SourceColorFormat the framebuffer pixel format, Pitch the
 * scanline stride in bytes, and FrameBuffer the framebuffer physical address.
 * Returns the applied mode. */
ULONG STDCALL AvSetDisplayMode(
    IN PVOID RegisterBase,
    IN ULONG Step,
    IN ULONG DisplayMode,
    IN ULONG SourceColorFormat,
    IN ULONG Pitch,
    IN ULONG FrameBuffer);

/* Records a framebuffer address to be retained across a relaunch; retrieve it
 * with AvGetSavedDataAddress. */
VOID STDCALL AvSetSavedDataAddress(
    IN PVOID Address);

#endif
