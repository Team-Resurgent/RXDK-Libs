#pragma once

// common.h must come first: it pulls the xapi/xtl/NT environment (defining
// NT_INCLUDED + the xboxkrnl base types) so the later windows/d3d8 headers use
// our types and skip zig's MinGW <winnt.h>. Mirrors samples/d3d8-triangle.
#include <xapi.h>
#include <math.h>
#include <stdio.h>
#include <guiddef.h>  // GUID/REFGUID -- d3d8.h's resource interfaces reference them
#include <d3d8.h>

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE   __stdcall
#define STDMETHODVCALLTYPE  __cdecl
#define STDAPICALLTYPE      __stdcall
#define STDAPIVCALLTYPE     __cdecl
#endif
#ifndef EXTERN_C
#define EXTERN_C extern
#endif
#ifndef STDAPI
#define STDAPI       EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(t)   EXTERN_C t STDAPICALLTYPE
#endif
#ifndef _HMODULE_DEFINED_
#define _HMODULE_DEFINED_
DECLARE_HANDLE(HMODULE);
#endif
#include <d3dx8math.h>

#define XMATH_OK   0
#define XMATH_SKIP 255

// Thin extern-"C" wrapper (added in libs/libd3d8/se/math.cpp) around the
// internal D3D::MatrixProduct4x4 -- the SSE-based function that actually
// combines WORLD/VIEW/PROJECTION for the GPU in the real render path
// (lazy.cpp's LazySetTransform), as opposed to the public D3DX8 API which
// nothing in the render path itself calls. Must be declared extern "C" here
// too (this sample is C++, matching the real definition's linkage) or the
// caller mangles the name and the linker can't find it.
extern "C" VOID WINAPI RxdkTestMatrixProduct4x4(
    D3DMATRIX *res, CONST D3DMATRIX *a, CONST D3DMATRIX *b);

// Same idea, wrapping D3D::Inverse4x4 (also libs/libd3d8/se/math.cpp) -- the
// function LazySetTransform calls for the inverse modelview matrix whenever
// lighting or texgen needs it.
extern "C" int WINAPI RxdkTestInverse4x4(
    D3DMATRIX *inverse, CONST D3DMATRIX *src, BOOL bNormalize);

// Approximate float equality, used throughout since these are FPU/SSE
// results, not bit-exact values.
int xmath_approx_eq(float a, float b, float eps);
int xmath_approx_eq3(float ax, float ay, float az, float bx, float by, float bz, float eps);

void xmath_set_matrix(D3DXMATRIX* m,
    float m11, float m12, float m13, float m14,
    float m21, float m22, float m23, float m24,
    float m31, float m32, float m33, float m34,
    float m41, float m42, float m43, float m44);

// A = [[1,2,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]], B = [[1,0,0,0],[3,1,0,0],[0,0,1,0],[0,0,0,1]]
// Hand-computed A*B = [[7,2,0,0],[3,1,0,0],[0,0,1,0],[0,0,0,1]] (see
// test_matrix_multiply.c). Shared so both the D3DX8 and libd3d8 (SSE)
// multiply paths get cross-checked against the exact same known inputs.
void xmath_build_known_pair(D3DXMATRIX* a, D3DXMATRIX* b);
int xmath_check_known_product(const D3DXMATRIX* r, const char* label);

// One DbgPrint call per logical line, "d3dmath-smoke: " prefixed to match the
// style of the other *-smoke samples.
void xmath_trace(const char* msg);
void xmath_trace_fail_f3(const char* test, float ex, float ey, float ez,
                                            float ax, float ay, float az);
void xmath_trace_matrix(const char* label, const D3DXMATRIX* m);
