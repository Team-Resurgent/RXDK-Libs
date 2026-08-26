/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Buffer base class: logic shared between the Index, Vertex, and Command
 * buffer types.
 */

#ifdef STARTUPANIMATION
namespace D3DK
#else
namespace D3D
#endif
{

//----------------------------------------------------------------------------
// Helper to create an instance of a buffer.
//
HRESULT CreateVertexIndexOrPushBuffer(
    DWORD Type,
    DWORD Size, 
    void **ppBuffer);


} // end namespace
