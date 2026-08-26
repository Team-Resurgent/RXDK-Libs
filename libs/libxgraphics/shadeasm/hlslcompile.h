/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * HLSL vertex-shader front end for XGCompileShader. Targets the vs_1_1 shader
 * model only (vs.1.1 / xvs.1.1 / xvss.1.1); pixel-shader HLSL is not supported
 * because a pixel "shader" on this hardware is a register-combiner
 * configuration, not a program.
 *
 * It lowers HLSL to vs.1.1 assembly *text*, which the assembler and optimizer
 * then turn into NV2A microcode. This file owns only the front end: lex, parse,
 * semantic binding and naive code generation. It performs no optimization --
 * register pressure and instruction pairing are the assembler optimizer's job.
 */

#ifndef __HLSLCOMPILE_H__
#define __HLSLCOMPILE_H__

namespace XGRAPHICS {

// Compile HLSL vertex-shader source to vs.1.1/xvs.1.1/xvss.1.1 assembly text.
//
//   pSource / sourceLen  already-preprocessed HLSL (XGPreprocess ran first)
//   pEntryName           entry function to compile (NULL -> "main")
//   pTargetName          "vs.1.1" | "xvs.1.1" | "xvss.1.1"
//   pSourceFileName      used only for diagnostics
//   pAsmOut              receives the generated assembly text on success
//   pErrorLog            diagnostics, MSVC "file(line) : error Vnnnn: msg" form
//
// Returns S_OK on success (pAsmOut filled), E_FAIL on any HLSL error (logged),
// or E_INVALIDARG / E_OUTOFMEMORY.
HRESULT CompileHlslVertexShader(
    const char*    pSource,
    DWORD          sourceLen,
    const char*    pEntryName,
    const char*    pTargetName,
    const char*    pSourceFileName,
    Buffer*        pAsmOut,
    XD3DXErrorLog* pErrorLog);

} // namespace XGRAPHICS

#endif // __HLSLCOMPILE_H__
