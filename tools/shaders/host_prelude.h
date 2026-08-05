typedef unsigned long DWORD; typedef unsigned char BYTE; typedef unsigned short WORD;
typedef int BOOL; typedef float FLOAT; typedef long LONG; typedef unsigned int UINT;
typedef void *HANDLE; typedef char CHAR; typedef unsigned long ULONG; typedef short SHORT;
typedef unsigned long ULONG_PTR; typedef long LONG_PTR;
typedef struct _RECT { LONG left,top,right,bottom; } RECT;
typedef struct _POINT { LONG x,y; } POINT;
typedef struct _GUID { DWORD a; WORD b,c; BYTE d[8]; } GUID;
#define DEFINE_GUID(n,a,b,c,d,e,f,g,h,i,j,k)
#define ZeroMemory(p,n) do{unsigned char*_q=(unsigned char*)(p);unsigned _n=(n);while(_n--)*_q++=0;}while(0)
typedef void *HWND; typedef int INT; typedef struct D3DSurface D3DSurface;
typedef struct D3DVertexBuffer D3DVertexBuffer;
typedef union _LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; long long QuadPart; } LARGE_INTEGER;
#define CONST const
