#ifndef __XBOXDEF_H__
#define __XBOXDEF_H__

typedef const void *LPCVOID;
typedef void VOID, *PVOID, *LPVOID;
typedef PVOID HANDLE, *PHANDLE;

#define NtCurrentThread() ( (HANDLE) -2 )

typedef unsigned char BOOLEAN, *PBOOLEAN;
typedef signed char SCHAR, *PSCHAR;
typedef char CHAR, *PCHAR, CCHAR, *LPCH, *PCH, OCHAR, *POCHAR;
typedef short SHORT, *PSHORT;
typedef int INT, *PINT, *LPINT;
typedef long LONG, *PLONG, *LPLONG;
typedef long long LONGLONG, *PLONGLONG;
typedef unsigned char BYTE;
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT, CSHORT;
typedef unsigned short WORD;
// In C++, WCHAR must be the built-in `wchar_t` (16-bit here) so idiomatic Xbox
// code like `const WCHAR s[] = L"..."` compiles -- L"" literals are wchar_t,
// which is a distinct type from unsigned short in C++. This also matches real
// winnt.h and this SDK's ntdef.h (both wchar_t in C++). The WCHAR APIs are
// extern "C" and wchar_t is 16-bit, so the ABI is identical. In C, wchar_t is
// already a 16-bit typedef and L"" matches unsigned short, so keep that (and
// avoid needing <stddef.h> included before this early header).
#ifdef __cplusplus
typedef wchar_t        WCHAR;
#else
typedef unsigned short WCHAR;
#endif
typedef WCHAR *PWSTR;
typedef WCHAR *PWCHAR;
typedef unsigned int UINT, *PUINT, *LPUINT;
typedef unsigned long DWORD, *PDWORD, *LPDWORD;
typedef unsigned long ULONG, *PULONG;
typedef unsigned long long ULONGLONG;
typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
typedef LONG HRESULT;

#define MAXDWORD 0xffffffff

typedef unsigned int SIZE_T, *PSIZE_T;
typedef int BOOL, *PBOOL;
typedef const char *PCSZ, *PCSTR, *LPCSTR;
typedef char *PSZ, *PSTR, *LPSTR;
typedef const WCHAR *LPCWSTR, *PCWSTR;
typedef WCHAR *LPWSTR;
typedef ULONGLONG QUAD;
typedef ULONG ULONG_PTR;
typedef LONG LONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef ULONG_PTR *PULONG_PTR;
typedef LONG_PTR *PLONG_PTR;

typedef struct _FLOATING_SAVE_AREA {
    WORD    ControlWord;
    WORD    StatusWord;
    WORD    TagWord;
    WORD    ErrorOpcode;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    DWORD   MXCsr;
    DWORD   Reserved2;
    BYTE    RegisterArea[128];
    BYTE    XmmRegisterArea[128];
    BYTE    Reserved4[224];
    DWORD   Cr0NpxState;
} __attribute__((packed)) FLOATING_SAVE_AREA;

#define CONTEXT_X86                0x00010000
#define CONTEXT_i386               CONTEXT_X86
#define CONTEXT_CONTROL            (CONTEXT_X86 | 0x00000001)
#define CONTEXT_INTEGER            (CONTEXT_X86 | 0x00000002)
#define CONTEXT_SEGMENTS           (CONTEXT_X86 | 0x00000004)
#define CONTEXT_FLOATING_POINT     (CONTEXT_X86 | 0x00000008)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_X86 | 0x00000020)

typedef struct _CONTEXT {
    DWORD ContextFlags;

    FLOATING_SAVE_AREA FloatSave;

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;

} __attribute__((packed)) CONTEXT, *PCONTEXT;

#define EXCEPTION_NONCONTINUABLE     0x01
#define EXCEPTION_UNWINDING          0x02
#define EXCEPTION_EXIT_UNWIND        0x04
#define EXCEPTION_STACK_INVALID      0x08
#define EXCEPTION_NESTED_CALL        0x10
#define EXCEPTION_TARGET_UNWIND      0x20
#define EXCEPTION_COLLIDED_UNWIND    0x40
#define EXCEPTION_UNWIND             (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)
#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

#ifndef RXDK_EXCEPTION_POINTERS_DEFINED
#define RXDK_EXCEPTION_POINTERS_DEFINED
typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
#endif

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING, *PANSI_STRING;

typedef union _LARGE_INTEGER {
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    };
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };
    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;

} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _RTL_CRITICAL_SECTION {
    union {
        struct {
            UCHAR Type;
            UCHAR Absolute;
            UCHAR Size;
            UCHAR Inserted;
            LONG SignalState;
            LIST_ENTRY WaitListHead;
        } Event;
        ULONG RawEvent[4];
    } Synchronization;

    LONG LockCount;
    LONG RecursionCount;
    PVOID OwningThread;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE   0x00000020
#define FILE_ATTRIBUTE_DEVICE    0x00000040
#define FILE_ATTRIBUTE_NORMAL    0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define INVALID_FILE_ATTRIBUTES  0xFFFFFFFF

//
// Generic access rights (dwDesiredAccess for CreateFile and other securable
// objects). Guarded so the libxapi-internal win32 bridge headers can coexist.
//
#ifndef GENERIC_READ
#define GENERIC_READ     0x80000000L
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE    0x40000000L
#endif
#ifndef GENERIC_EXECUTE
#define GENERIC_EXECUTE  0x20000000L
#endif
#ifndef GENERIC_ALL
#define GENERIC_ALL      0x10000000L
#endif

//
// Standard access rights (the type-independent upper bits of an access mask).
//
#ifndef DELETE
#define DELETE                   0x00010000L
#define READ_CONTROL             0x00020000L
#define WRITE_DAC                0x00040000L
#define WRITE_OWNER              0x00080000L
#define SYNCHRONIZE              0x00100000L
#define STANDARD_RIGHTS_REQUIRED 0x000F0000L
#define STANDARD_RIGHTS_READ     READ_CONTROL
#define STANDARD_RIGHTS_WRITE    READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE  READ_CONTROL
#define STANDARD_RIGHTS_ALL      0x001F0000L
#endif

//
// Memory-protection constants (flProtect).
//
#ifndef PAGE_NOACCESS
#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD             0x100
#define PAGE_NOCACHE           0x200
#define PAGE_WRITECOMBINE      0x400
#endif

//
// File share modes (dwShareMode for CreateFile).
//
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ   0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE  0x00000002
#endif
#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 0x00000004
#endif

//
// File-specific access rights (the low 9 bits of a file access mask).
//
#ifndef FILE_READ_DATA
#define FILE_READ_DATA            0x0001  // file & pipe
#define FILE_LIST_DIRECTORY       0x0001  // directory
#define FILE_WRITE_DATA           0x0002  // file & pipe
#define FILE_ADD_FILE             0x0002  // directory
#define FILE_APPEND_DATA          0x0004  // file
#define FILE_ADD_SUBDIRECTORY     0x0004  // directory
#define FILE_CREATE_PIPE_INSTANCE 0x0004  // named pipe
#define FILE_READ_EA              0x0008  // file & directory
#define FILE_WRITE_EA             0x0010  // file & directory
#define FILE_EXECUTE              0x0020  // file
#define FILE_TRAVERSE             0x0020  // directory
#define FILE_DELETE_CHILD         0x0040  // directory
#define FILE_READ_ATTRIBUTES      0x0080  // all
#define FILE_WRITE_ATTRIBUTES     0x0100  // all
#define FILE_ALL_ACCESS      (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)
#define FILE_GENERIC_READ    (STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE)
#define FILE_GENERIC_WRITE   (STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE)
#define FILE_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE | FILE_READ_ATTRIBUTES | FILE_EXECUTE | SYNCHRONIZE)
#endif

typedef struct _XBE_SECTION_HEADER {
    DWORD Flags;
    DWORD VirtualAddress;
    DWORD VirtualSize;
    DWORD FileAddress;
    DWORD FileSize;
    PCSZ SectionName;
    LONG SectionReferenceCount;
    WORD *HeadReferenceCount;
    WORD *TailReferenceCount;
    BYTE CheckSum[20];
} XBE_SECTION_HEADER, *PXBE_SECTION_HEADER;

#endif
