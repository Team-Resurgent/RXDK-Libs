#ifndef _RXDK_DMUSIC_OAIDL_H_
#define _RXDK_DMUSIC_OAIDL_H_
/*
 * Minimal oaidl.h for the Xbox DirectMusic port (dmscript). The real header is a
 * MIDL stub for the OLE-Automation type system (IDispatch/ITypeInfo/ITypeLib/
 * ITypeComp + the DISPID/DISPATCH constants). Xbox has no automation runtime, so
 * this provides just the INTERFACE DECLARATIONS (vtable shapes) + constants the
 * scripting engine's dispatch objects derive from and name -- enough to compile
 * and link. Built on the base COM surface (xobjbase) + the VARIANT/DISPPARAMS/
 * EXCEPINFO value types from oleauto.
 */
#include <objbase.h>
#include <oleauto.h>

#ifndef _LCID_DEFINED
#define _LCID_DEFINED
typedef DWORD LCID;
typedef WORD  LANGID;
#endif

/* Locale/language macros + ids the engine seeds an LCID with (enginc.h). */
#ifndef LANG_ENGLISH
#define LANG_NEUTRAL         0x00
#define LANG_ENGLISH         0x09
#define SUBLANG_NEUTRAL      0x00
#define SUBLANG_DEFAULT      0x01
#define SUBLANG_ENGLISH_US   0x01
#define SORT_DEFAULT         0x0
#endif
#ifndef MAKELANGID
#define MAKELANGID(p, s)     ((((WORD)(s)) << 10) | (WORD)(p))
#define MAKELCID(lgid, srt)  ((DWORD)((((DWORD)((WORD)(srt))) << 16) | ((DWORD)((WORD)(lgid)))))
#endif

/* OLE type-system value structs the engine's IDispatch/ITypeInfo bridge names
   (engdisp.h). Declaration-only shapes -- the real automation type library is not
   present on Xbox; the engine only passes pointers to these around. */
#ifndef _RXDK_OLE_TYPEDESC_
#define _RXDK_OLE_TYPEDESC_
typedef DWORD HREFTYPE;
typedef enum tagTYPEKIND { TKIND_ENUM=0, TKIND_RECORD, TKIND_MODULE, TKIND_INTERFACE, TKIND_DISPATCH, TKIND_COCLASS, TKIND_ALIAS, TKIND_UNION, TKIND_MAX } TYPEKIND;
typedef enum tagINVOKEKIND { INVOKE_FUNC=1, INVOKE_PROPERTYGET=2, INVOKE_PROPERTYPUT=4, INVOKE_PROPERTYPUTREF=8 } INVOKEKIND;
typedef enum tagCALLCONV { CC_FASTCALL=0, CC_CDECL=1, CC_MSCPASCAL, CC_PASCAL=CC_MSCPASCAL, CC_MACPASCAL, CC_STDCALL, CC_FPFASTCALL, CC_SYSCALL, CC_MPWCDECL, CC_MPWPASCAL, CC_MAX=CC_MPWPASCAL } CALLCONV;
typedef enum tagFUNCKIND { FUNC_VIRTUAL, FUNC_PUREVIRTUAL, FUNC_NONVIRTUAL, FUNC_STATIC, FUNC_DISPATCH } FUNCKIND;
typedef enum tagVARKIND { VAR_PERINSTANCE, VAR_STATIC, VAR_CONST, VAR_DISPATCH } VARKIND;
typedef struct tagTYPEDESC { union { struct tagTYPEDESC *lptdesc; struct tagARRAYDESC *lpadesc; HREFTYPE hreftype; }; VARTYPE vt; } TYPEDESC;
typedef struct tagPARAMDESC { struct tagPARAMDESCEX *pparamdescex; USHORT wParamFlags; } PARAMDESC;
typedef struct tagELEMDESC { TYPEDESC tdesc; PARAMDESC paramdesc; } ELEMDESC, *LPELEMDESC;
typedef struct tagTYPEATTR {
    GUID guid; LCID lcid; DWORD dwReserved; MEMBERID memidConstructor; MEMBERID memidDestructor;
    LPOLESTR lpstrSchema; ULONG cbSizeInstance; TYPEKIND typekind; WORD cFuncs; WORD cVars;
    WORD cImplTypes; WORD cbSizeVft; WORD cbAlignment; WORD wTypeFlags; WORD wMajorVerNum;
    WORD wMinorVerNum; TYPEDESC tdescAlias; struct tagIDLDESC { ULONG_PTR dwReserved; USHORT wIDLFlags; } idldescType;
} TYPEATTR, *LPTYPEATTR;
typedef struct tagFUNCDESC {
    MEMBERID memid; SCODE *lprgscode; ELEMDESC *lprgelemdescParam; FUNCKIND funckind;
    INVOKEKIND invkind; CALLCONV callconv; SHORT cParams; SHORT cParamsOpt; SHORT oVft;
    SHORT cScodes; ELEMDESC elemdescFunc; WORD wFuncFlags;
} FUNCDESC, *LPFUNCDESC;
typedef struct tagVARDESC {
    MEMBERID memid; LPOLESTR lpstrSchema; union { ULONG oInst; VARIANT *lpvarValue; };
    ELEMDESC elemdescVar; WORD wVarFlags; VARKIND varkind;
} VARDESC, *LPVARDESC;
#endif

/* DISPID reserved ids + DISPATCH invoke-flags the dispatch objects name. */
#ifndef DISPID_UNKNOWN
#define DISPID_UNKNOWN     (-1)
#define DISPID_VALUE       (0)
#define DISPID_PROPERTYPUT (-3)
#define DISPID_NEWENUM     (-4)
#define DISPID_EVALUATE    (-5)
#define DISPID_CONSTRUCTOR (-6)
#define DISPID_DESTRUCTOR  (-7)
#define DISPID_COLLECT     (-8)
#endif
#ifndef DISPATCH_METHOD
#define DISPATCH_METHOD         0x1
#define DISPATCH_PROPERTYGET    0x2
#define DISPATCH_PROPERTYPUT    0x4
#define DISPATCH_PROPERTYPUTREF 0x8
#endif

/* Type-system interfaces. DirectMusic's dispatch objects only pass these around
   as pointers (GetTypeInfo returns E_NOTIMPL), so IUnknown-derived stubs with no
   extra methods are sufficient for compile+link. */
#ifndef __ITypeInfo_FWD_DEFINED__
#define __ITypeInfo_FWD_DEFINED__
typedef interface ITypeInfo ITypeInfo;
typedef interface ITypeLib  ITypeLib;
typedef interface ITypeComp ITypeComp;
#endif

EXTERN_C const IID IID_ITypeInfo;
EXTERN_C const IID IID_ITypeLib;
EXTERN_C const IID IID_ITypeComp;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface ITypeComp : public IUnknown {};
interface ITypeLib  : public IUnknown {};
interface ITypeInfo : public IUnknown
{
public:
    STDMETHOD(GetTypeAttr)(TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr) PURE;
    STDMETHOD(GetTypeComp)(ITypeComp __RPC_FAR *__RPC_FAR *ppTComp) PURE;
    STDMETHOD(GetFuncDesc)(UINT index, FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc) PURE;
    STDMETHOD(GetVarDesc)(UINT index, VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc) PURE;
    STDMETHOD(GetNames)(MEMBERID memid, BSTR __RPC_FAR *rgBstrNames, UINT cMaxNames, UINT __RPC_FAR *pcNames) PURE;
    STDMETHOD(GetRefTypeOfImplType)(UINT index, HREFTYPE __RPC_FAR *pRefType) PURE;
    STDMETHOD(GetImplTypeFlags)(UINT index, INT __RPC_FAR *pImplTypeFlags) PURE;
    STDMETHOD(GetIDsOfNames)(LPOLESTR __RPC_FAR *rgszNames, UINT cNames, MEMBERID __RPC_FAR *pMemId) PURE;
    STDMETHOD(Invoke)(PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) PURE;
    STDMETHOD(GetDocumentation)(MEMBERID memid, BSTR __RPC_FAR *pBstrName, BSTR __RPC_FAR *pBstrDocString, DWORD __RPC_FAR *pdwHelpContext, BSTR __RPC_FAR *pBstrHelpFile) PURE;
    STDMETHOD(GetDllEntry)(MEMBERID memid, INVOKEKIND invKind, BSTR __RPC_FAR *pBstrDllName, BSTR __RPC_FAR *pBstrName, WORD __RPC_FAR *pwOrdinal) PURE;
    STDMETHOD(GetRefTypeInfo)(HREFTYPE hRefType, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) PURE;
    STDMETHOD(AddressOfMember)(MEMBERID memid, INVOKEKIND invKind, PVOID __RPC_FAR *ppv) PURE;
    STDMETHOD(CreateInstance)(IUnknown __RPC_FAR *pUnkOuter, REFIID riid, PVOID __RPC_FAR *ppvObj) PURE;
    STDMETHOD(GetMops)(MEMBERID memid, BSTR __RPC_FAR *pBstrMops) PURE;
    STDMETHOD(GetContainingTypeLib)(ITypeLib __RPC_FAR *__RPC_FAR *ppTLib, UINT __RPC_FAR *pIndex) PURE;
    STDMETHOD_(void, ReleaseTypeAttr)(TYPEATTR __RPC_FAR *pTypeAttr) PURE;
    STDMETHOD_(void, ReleaseFuncDesc)(FUNCDESC __RPC_FAR *pFuncDesc) PURE;
    STDMETHOD_(void, ReleaseVarDesc)(VARDESC __RPC_FAR *pVarDesc) PURE;
};
#else
typedef struct ITypeInfoVtbl { BEGIN_INTERFACE
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ITypeInfo*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ITypeInfo*);
    ULONG   (STDMETHODCALLTYPE *Release)(ITypeInfo*);
    END_INTERFACE } ITypeInfoVtbl;
interface ITypeInfo { CONST_VTBL struct ITypeInfoVtbl *lpVtbl; };
#endif

/* IDispatch -- the automation dispatch base the scripting dispatch objects
   derive from (they override the four methods, mostly returning E_NOTIMPL). */
#ifndef __IDispatch_FWD_DEFINED__
#define __IDispatch_FWD_DEFINED__
typedef interface IDispatch IDispatch;
#endif

EXTERN_C const IID IID_IDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    MIDL_INTERFACE("00020400-0000-0000-C000-000000000046")
    IDispatch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT __RPC_FAR *pctinfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId) = 0;
        virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) = 0;
    };
#else
typedef struct IDispatchVtbl { BEGIN_INTERFACE
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDispatch*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDispatch*);
    ULONG   (STDMETHODCALLTYPE *Release)(IDispatch*);
    HRESULT (STDMETHODCALLTYPE *GetTypeInfoCount)(IDispatch*, UINT*);
    HRESULT (STDMETHODCALLTYPE *GetTypeInfo)(IDispatch*, UINT, LCID, ITypeInfo**);
    HRESULT (STDMETHODCALLTYPE *GetIDsOfNames)(IDispatch*, REFIID, LPOLESTR*, UINT, LCID, DISPID*);
    HRESULT (STDMETHODCALLTYPE *Invoke)(IDispatch*, DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*);
    END_INTERFACE } IDispatchVtbl;
interface IDispatch { CONST_VTBL struct IDispatchVtbl *lpVtbl; };
#endif
typedef IDispatch __RPC_FAR *LPDISPATCH;

#endif /* _RXDK_DMUSIC_OAIDL_H_ */
