//
// RXDK: instantiate the D3DX8 interface GUIDs.
//
// The headers declare them with DEFINE_GUID, which only emits the data when INITGUID is set
// before the declaration is seen. core/init.c does that for the .X-file GUIDs (rmxfguid.h) but
// cannot include the D3DX8 headers -- they do not compile as C (see the note there) -- so
// IID_ID3DXSprite, IID_ID3DXEffect, IID_ID3DXBaseMesh and friends were declared and never
// defined, and any title that named one failed at link. This C++ TU covers them.
//

#define INITGUID

#include <xtl.h>
#include <d3dx8.h>
