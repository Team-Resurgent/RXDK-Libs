//------------------------------------------------------------------------------
// Minimal global operator new/delete for the d3d8-textures title.
//
// libd3dx8's C++ image codecs (CD3DXImage / CD3DXCodec) allocate scratch buffers
// with `new`/`delete`. A real Xbox title gets these from its C++ runtime/CRT;
// this C sample has none, so we supply a tiny malloc/free-backed set. Compiled as
// its own C++ object and linked into the sample (the sample builder compiles all
// its sources in one language mode, so this can't live in the C main.c).
//------------------------------------------------------------------------------
#include <stdlib.h>

void *operator new(unsigned int n)        { return malloc(n ? n : 1); }
void *operator new[](unsigned int n)      { return malloc(n ? n : 1); }
void  operator delete(void *p)            { free(p); }
void  operator delete[](void *p)          { free(p); }
void  operator delete(void *p, unsigned int)   { free(p); }
void  operator delete[](void *p, unsigned int) { free(p); }
