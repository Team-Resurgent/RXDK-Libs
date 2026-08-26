/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Lightweight exception type for the D3DX shape code. CD3DXException carries an
 * HRESULT, a message and the throw-site line number; the D3DX_TRY / D3DX_CATCH /
 * D3DX_THROW macros wrap the standard try/catch/throw so a caught exception logs
 * a debug string and is turned back into its HRESULT return value.
 */

#ifndef __D3DXEXCEPT_H__
#define __D3DXEXCEPT_H__

#include <string.h>
#include "calloc.h"

#define D3DX_THROW( hResult, string )  throw CD3DXException( hResult, string, __LINE__);
#define D3DX_CATCH   catch( CD3DXException e ) { e.DebugString(); return e.error; }
#define D3DX_TRY     try

class CD3DXException : public CD3duAlloc
{
public:
    CD3DXException(HRESULT res, char *msg, int LineNumber = __LINE__) 
    {
        error = res; 
        strcpy(message, msg); 
        line = LineNumber;
    }
    char message[128];
    HRESULT error;
    int line;
    
    void DebugString() {};
    void Popup() {};
};

#endif // __D3DXEXCEPT_H__
