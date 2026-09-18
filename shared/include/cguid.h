/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * <cguid.h> - extern declarations of the well-known COM GUIDs that are not
 * produced by MIDL (GUID_NULL, the standard marshaller/proxy CLSIDs, ...).
 * xobjbase.h pulls these declarations in (when INITGUID is not set) so code
 * that references GUID_NULL, CLSID_StdMarshal, CLSID_StdGlobalInterfaceTable,
 * etc. compiles; the actual definitions are emitted by libxapi/uuid/cguid.c.
 *
 * The declaration list mirrors the (public-domain, mingw-w64) cguid.h that the
 * zig toolchain used to supply implicitly, so RXDK is self-contained under the
 * LLVM toolchain (which provides no such Windows header). See
 * docs/llvm-toolchain-plan.md.
 */
#ifndef __CGUID_H__
#define __CGUID_H__

#ifdef __cplusplus
extern "C" {
#endif

  extern const IID GUID_NULL;
  extern const IID CATID_MARSHALER;
  extern const IID IID_IRpcChannel;
  extern const IID IID_IRpcStub;
  extern const IID IID_IStubManager;
  extern const IID IID_IRpcProxy;
  extern const IID IID_IProxyManager;
  extern const IID IID_IPSFactory;
  extern const IID IID_IInternalMoniker;
  extern const IID IID_IDfReserved1;
  extern const IID IID_IDfReserved2;
  extern const IID IID_IDfReserved3;
  extern const CLSID CLSID_StdMarshal;
  extern const CLSID CLSID_AggStdMarshal;
  extern const CLSID CLSID_StdAsyncActManager;
  extern const IID IID_IStub;
  extern const IID IID_IProxy;
  extern const IID IID_IEnumGeneric;
  extern const IID IID_IEnumHolder;
  extern const IID IID_IEnumCallback;
  extern const IID IID_IOleManager;
  extern const IID IID_IOlePresObj;
  extern const IID IID_IDebug;
  extern const IID IID_IDebugStream;
  extern const CLSID CLSID_PSGenObject;
  extern const CLSID CLSID_PSClientSite;
  extern const CLSID CLSID_PSClassObject;
  extern const CLSID CLSID_PSInPlaceActive;
  extern const CLSID CLSID_PSInPlaceFrame;
  extern const CLSID CLSID_PSDragDrop;
  extern const CLSID CLSID_PSBindCtx;
  extern const CLSID CLSID_PSEnumerators;
  extern const CLSID CLSID_StaticMetafile;
  extern const CLSID CLSID_StaticDib;
  extern const CLSID CID_CDfsVolume;
  extern const CLSID CLSID_DCOMAccessControl;
  extern const CLSID CLSID_GlobalOptions;
  extern const CLSID CLSID_StdGlobalInterfaceTable;
  extern const CLSID CLSID_ComBinding;
  extern const CLSID CLSID_StdEvent;
  extern const CLSID CLSID_ManualResetEvent;
  extern const CLSID CLSID_SynchronizeContainer;
  extern const CLSID CLSID_AddrControl;
  extern const CLSID CLSID_IdentityUnmarshal;
  extern const CLSID CLSID_InProcFreeMarshaler;
  extern const CLSID CLSID_Picture_Metafile;
  extern const CLSID CLSID_Picture_EnhMetafile;
  extern const CLSID CLSID_Picture_Dib;
  extern const CLSID CLSID_StdFont;
  extern const CLSID CLSID_StdPicture;
  extern const GUID GUID_TRISTATE;
  extern const CLSID CLSID_ContextSwitcher;

#ifdef __cplusplus
}
#endif
#endif /* __CGUID_H__ */
