/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

#ifndef _RXDK_DMUSIC_OLEAUTO_H_
#define _RXDK_DMUSIC_OLEAUTO_H_
/*
 * Minimal oleauto.h for the Xbox DirectMusic port. The full OLE-Automation
 * header (VARIANT, BSTR, SAFEARRAY, IDispatch and helpers) is MIDL-heavy
 * and pulls the whole wtypes/oaidl/rpcndr chain. DirectMusic's playback
 * components pull it transitively through shared/debug.h -> ole2.h but only the
 * scripting engine (dmscript) actually manipulates automation types. This shim
 * provides just enough of the surface for the headers to parse; the exact
 * automation ABI (Sys.. / Variant.. helpers) only matters for dmscript.
 */
#include <objbase.h>

/* Base automation scalar aliases the standard wtypes.h would supply. */
#ifndef _RXDK_OLE_BASE_TYPES_
#define _RXDK_OLE_BASE_TYPES_
#ifndef _SCODE_DEFINED
#define _SCODE_DEFINED
typedef LONG SCODE;
#endif
#ifndef _LPCOLESTR_DEFINED
#define _LPCOLESTR_DEFINED
typedef WCHAR OLECHAR;
typedef OLECHAR *LPOLESTR;
typedef const OLECHAR *LPCOLESTR;
#endif
#ifndef _DISPID_DEFINED
#define _DISPID_DEFINED
typedef LONG DISPID;
typedef LONG MEMBERID;
#endif
#endif /* _RXDK_OLE_BASE_TYPES_ */

#ifndef _tagVARIANT_DEFINED
#define _tagVARIANT_DEFINED

typedef unsigned short VARTYPE;

/* VARENUM */
enum VARENUM {
    VT_EMPTY = 0, VT_NULL = 1, VT_I2 = 2, VT_I4 = 3, VT_R4 = 4, VT_R8 = 5,
    VT_CY = 6, VT_DATE = 7, VT_BSTR = 8, VT_DISPATCH = 9, VT_ERROR = 10,
    VT_BOOL = 11, VT_VARIANT = 12, VT_UNKNOWN = 13, VT_DECIMAL = 14,
    VT_I1 = 16, VT_UI1 = 17, VT_UI2 = 18, VT_UI4 = 19, VT_I8 = 20, VT_UI8 = 21,
    VT_INT = 22, VT_UINT = 23, VT_VOID = 24, VT_HRESULT = 25, VT_PTR = 26,
    VT_SAFEARRAY = 27, VT_CARRAY = 28, VT_USERDEFINED = 29, VT_LPSTR = 30,
    VT_LPWSTR = 31, VT_RECORD = 36, VT_ARRAY = 0x2000, VT_BYREF = 0x4000
};

typedef WCHAR *BSTR;
typedef BSTR *LPBSTR;
typedef short VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;
#ifndef VARIANT_TRUE
#define VARIANT_TRUE  ((VARIANT_BOOL)-1)
#define VARIANT_FALSE ((VARIANT_BOOL)0)
#endif
typedef double DATE;
typedef union tagCY { struct { unsigned long Lo; long Hi; } DUMMYSTRUCTNAME; LONGLONG int64; } CY;
typedef struct tagDEC { USHORT wReserved; BYTE scale; BYTE sign; ULONG Hi32; ULONGLONG Lo64; } DECIMAL;

struct tagSAFEARRAY;
typedef struct tagSAFEARRAY SAFEARRAY;
typedef struct tagSAFEARRAYBOUND { ULONG cElements; LONG lLbound; } SAFEARRAYBOUND;
struct tagSAFEARRAY {
    USHORT cDims; USHORT fFeatures; ULONG cbElements; ULONG cLocks;
    PVOID pvData; SAFEARRAYBOUND rgsabound[1];
};

typedef struct tagVARIANT VARIANT;
struct tagVARIANT {
    // RXDK: anonymous union/struct (MS-extension, enabled by -fms-extensions) so
    // the automation code reaches .vt/.lVal/.bstrVal etc. directly.
    union {
        struct {
            VARTYPE vt; WORD wReserved1; WORD wReserved2; WORD wReserved3;
            union {
                LONGLONG llVal; LONG lVal; BYTE bVal; SHORT iVal; FLOAT fltVal;
                DOUBLE dblVal; VARIANT_BOOL boolVal; SCODE scode; CY cyVal;
                DATE date; BSTR bstrVal; struct IUnknown *punkVal;
                struct IDispatch *pdispVal; SAFEARRAY *parray; BYTE *pbVal;
                SHORT *piVal; LONG *plVal; FLOAT *pfltVal; DOUBLE *pdblVal;
                VARIANT_BOOL *pboolVal; SCODE *pscode; CY *pcyVal; DATE *pdate;
                BSTR *pbstrVal; struct IUnknown **ppunkVal;
                struct IDispatch **ppdispVal; SAFEARRAY **pparray;
                VARIANT *pvarVal; PVOID byref; CHAR cVal; USHORT uiVal;
                ULONG ulVal; ULONGLONG ullVal; INT intVal; UINT uintVal;
                DECIMAL *pdecVal; CHAR *pcVal; USHORT *puiVal; ULONG *pulVal;
                INT *pintVal; UINT *puintVal;
            };
        };
        DECIMAL decVal;
    };
};
typedef VARIANT VARIANTARG;
typedef VARIANT *LPVARIANT;
typedef VARIANTARG *LPVARIANTARG;

#endif /* _tagVARIANT_DEFINED */

/* Automation dispatch parameter/exception structs. */
#ifndef _DISPPARAMS_DEFINED
#define _DISPPARAMS_DEFINED
typedef struct tagDISPPARAMS {
    VARIANTARG *rgvarg; DISPID *rgdispidNamedArgs; UINT cArgs; UINT cNamedArgs;
} DISPPARAMS;
typedef struct tagEXCEPINFO {
    WORD wCode; WORD wReserved; BSTR bstrSource; BSTR bstrDescription;
    BSTR bstrHelpFile; DWORD dwHelpContext; PVOID pvReserved;
    HRESULT (STDAPICALLTYPE *pfnDeferredFillIn)(struct tagEXCEPINFO *);
    SCODE scode;
} EXCEPINFO, *LPEXCEPINFO;
#endif

/* The automation interface IIDs DirectMusic's QueryInterface paths name. Declared
   here; storage is defined in the dmusic guids TU (INITGUID). */
EXTERN_C const IID IID_IDispatch;
EXTERN_C const IID IID_IDispatchEx;

#ifdef __cplusplus
extern "C" {
#endif
BSTR    __stdcall SysAllocString(const OLECHAR *);
BSTR    __stdcall SysAllocStringLen(const OLECHAR *, UINT);
BSTR    __stdcall SysAllocStringByteLen(LPCSTR, UINT);
void    __stdcall SysFreeString(BSTR);
UINT    __stdcall SysStringLen(BSTR);
UINT    __stdcall SysStringByteLen(BSTR);
INT     __stdcall SysReAllocString(BSTR *, const OLECHAR *);
INT     __stdcall SysReAllocStringLen(BSTR *, const OLECHAR *, UINT);
void    __stdcall VariantInit(VARIANTARG *);
HRESULT __stdcall VariantClear(VARIANTARG *);
HRESULT __stdcall VariantCopy(VARIANTARG *, VARIANTARG *);
HRESULT __stdcall VariantChangeType(VARIANTARG *, VARIANTARG *, USHORT, VARTYPE);
#ifdef __cplusplus
}
#endif

#endif /* _RXDK_DMUSIC_OLEAUTO_H_ */
