// RXDK: standard OLE/COM interface IIDs the DirectMusic QueryInterface paths
// reference (IID_IUnknown/IStream/IClassFactory/IPersist/IPersistStream/
// ISequentialStream/IDispatch). The leak got these from uuid.lib; RXDK has no
// uuid.lib, so define them here (compiled in the dmguids slice with -DINITGUID,
// so DEFINE_GUID allocates storage). Values are the canonical OLE GUIDs.
#include <guiddef.h>

DEFINE_GUID(IID_IUnknown,          0x00000000, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IClassFactory,     0x00000001, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IMarshal,          0x00000003, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IPersist,          0x0000010c, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IPersistStream,    0x00000109, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IStream,           0x0000000c, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_ISequentialStream, 0x0c733a30, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(IID_IDispatch,         0x00020400, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46);
DEFINE_GUID(IID_IDispatchEx,       0xa6ef9860, 0xc720, 0x11d0, 0x93, 0x37, 0x00, 0xa0, 0xc9, 0x0d, 0xca, 0xa9);
