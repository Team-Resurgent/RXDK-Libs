// RXDK: interface IIDs the DirectMusic QueryInterface paths reference that
// nothing else in the tree defines -- the Active Scripting / OLE-Automation
// extension set. Compiled in the dmguids slice with -DINITGUID, so DEFINE_GUID
// allocates storage.
//
// The standard OLE/COM IIDs (IUnknown, IStream, IClassFactory, IPersist,
// IPersistStream, ISequentialStream, IDispatch, ITypeInfo/Lib/Comp) are NOT
// here: libxapi's uuid component owns those, and defining them again made every
// link that pulled both libraries fail on duplicate symbols. An earlier comment
// here claimed RXDK had no uuid.lib equivalent; it does.
#include <guiddef.h>

DEFINE_GUID(IID_IDispatchEx,       0xa6ef9860, 0xc720, 0x11d0, 0x93, 0x37, 0x00, 0xa0, 0xc9, 0x0d, 0xca, 0xa9);
// OLE-Automation type-system + Active Scripting IIDs (dmscript QueryInterface).
DEFINE_GUID(IID_IActiveScript,      0xbb1a2ae1, 0xa4f9, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
DEFINE_GUID(IID_IActiveScriptParse, 0xbb1a2ae2, 0xa4f9, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
DEFINE_GUID(IID_IActiveScriptSite,  0xdb01a1e3, 0xa42b, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
DEFINE_GUID(IID_IActiveScriptError, 0xeae1ba61, 0xa4ed, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
