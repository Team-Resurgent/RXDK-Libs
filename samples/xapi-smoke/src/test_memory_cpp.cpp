//------------------------------------------------------------------------------
// C++ counterpart to test_memory.c. Every existing memmove/memcpy regression
// test (see tools/conformance_recipes.py + samples/libc-smoke) exercises the C
// compile path only. libd3d8/libdsound/libcpp are all compiled through the real
// C++ flag set (see libs/libd3d8/build.zig cppFlags: -std=c++17, -fno-exceptions,
// -fno-rtti, no -fno-builtin) which picolibcFlags()'s -fno-builtin fix never
// touches (that flag only applies to picolibc's own sources in libs/libc). This
// file calls the same memmove/RtlMoveMemory-shaped operations from a genuine
// .cpp TU compiled that exact way, to check whether the self-recursion hang
// (RXDK-Libs commit pending, see build/xbox_target.zig) can reappear when the
// *caller* is C++ rather than C. Compiled as its own object (like opnew.cpp)
// since the sample builder compiles all of main.c's sources in one language
// mode.
//------------------------------------------------------------------------------
#include <xapi.h>

#include <string.h>

#define XAPI_OK 0

extern "C" int test_memory_cpp(void)
{
    // Basic non-overlapping copy, plain memcpy/memmove.
    {
        char src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        char dst[8] = { 0 };
        if (memcpy(dst, src, sizeof(dst)) != dst)
            return 1;
        for (unsigned i = 0; i < sizeof(dst); ++i) {
            if (dst[i] != src[i])
                return 2;
        }
        memset(dst, 0, sizeof(dst));
        if (memmove(dst, src, sizeof(dst)) != dst)
            return 3;
        for (unsigned i = 0; i < sizeof(dst); ++i) {
            if (dst[i] != src[i])
                return 4;
        }
    }

    // The exact 48-byte / 4-byte-aligned / non-overlapping shape that hung on
    // hardware inside libxapi/rtl/heap.c's RtlCreateHeap (RtlMoveMemory(&Temp,
    // Parameters, sizeof(*Parameters))). Regression guard, C++ compiled.
    {
        struct Block48 {
            unsigned long words[12];
        };
        Block48 src48, dst48;
        for (unsigned i = 0; i < 12; ++i)
            src48.words[i] = 0x11111111u * (i + 1);
        memset(&dst48, 0, sizeof(dst48));
        if (RtlMoveMemory(&dst48, &src48, sizeof(src48)) != &dst48)
            return 5;
        for (unsigned i = 0; i < 12; ++i) {
            if (dst48.words[i] != src48.words[i])
                return 6;
        }
    }

    // 64-byte struct copy, matching the sizeof(D3DMATRIX) memcpy shape used
    // throughout libd3d8 (se/d3dbase.cpp SetTransform/GetTransform et al).
    {
        struct Block64 {
            float f[16];
        };
        Block64 src64, dst64;
        for (unsigned i = 0; i < 16; ++i)
            src64.f[i] = (float)i + 0.5f;
        memset(&dst64, 0, sizeof(dst64));
        memcpy(&dst64, &src64, sizeof(src64));
        for (unsigned i = 0; i < 16; ++i) {
            if (dst64.f[i] != src64.f[i])
                return 7;
        }
    }

    // Forward/backward overlapping shifts: the two cases memmove must get
    // right that memcpy doesn't handle.
    {
        char buf[8] = { 1, 2, 3, 4, 5, 6, 0, 0 };
        memmove(buf + 2, buf, 6); // forward-overlapping: shift right by 2
        static const char expectFwd[8] = { 1, 2, 1, 2, 3, 4, 5, 6 };
        for (unsigned i = 0; i < 8; ++i) {
            if (buf[i] != expectFwd[i])
                return 8;
        }

        char buf2[8] = { 0, 0, 1, 2, 3, 4, 5, 6 };
        memmove(buf2, buf2 + 2, 6); // backward-overlapping: shift left by 2
        static const char expectBwd[8] = { 1, 2, 3, 4, 5, 6, 5, 6 };
        for (unsigned i = 0; i < 8; ++i) {
            if (buf2[i] != expectBwd[i])
                return 9;
        }
    }

    // CopyMemory/ZeroMemory macros (winbase-style wrappers over memcpy/memset)
    // called from C++, same as test_memory.c but past the SSO/tiny-buffer size.
    {
        char src[64];
        char dst[64];
        for (unsigned i = 0; i < sizeof(src); ++i)
            src[i] = (char)i;
        CopyMemory(dst, src, sizeof(dst));
        for (unsigned i = 0; i < sizeof(dst); ++i) {
            if (dst[i] != src[i])
                return 10;
        }
        ZeroMemory(dst, sizeof(dst));
        for (unsigned i = 0; i < sizeof(dst); ++i) {
            if (dst[i] != 0)
                return 11;
        }
    }

    // HeapAlloc/HeapReAlloc from C++: HeapReAlloc's growth path calls
    // RtlMoveMemory internally (libxapi/rtl/heap.c) to preserve the old block's
    // contents, same call as the original hang, but reached via HeapReAlloc's
    // own code path rather than RtlCreateHeap's one-time setup copy.
    {
        HANDLE heap = GetProcessHeap();
        if (!heap)
            return 12;

        void* block = HeapAlloc(heap, 0, 64);
        if (!block)
            return 13;
        memset(block, 0xAB, 64);

        void* grown = HeapReAlloc(heap, 0, block, 128);
        if (!grown) {
            HeapFree(heap, 0, block);
            return 14;
        }
        const unsigned char* bytes = (const unsigned char*)grown;
        for (unsigned i = 0; i < 64; ++i) {
            if (bytes[i] != 0xAB) {
                HeapFree(heap, 0, grown);
                return 15;
            }
        }
        if (!HeapFree(heap, 0, grown))
            return 16;
    }

    return XAPI_OK;
}
