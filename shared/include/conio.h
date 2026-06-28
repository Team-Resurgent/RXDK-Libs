#pragma once
/*
 * RXDK stub for the vendored D3D8 sources (mp.hpp / mpcore.cpp #include <conio.h>).
 * Shadows zig's MinGW conio.h, which pulls _mingw.h/vadefs.h and fails to compile
 * under our flags. The Xbox has no console I/O; the only thing the D3D8 driver
 * actually uses from here is the MSVC x86 port-I/O intrinsics (_outp/_inp...),
 * which we provide as inline asm.
 */

#ifdef __cplusplus
extern "C" {
#endif

static __inline int _inp(unsigned short port) {
    unsigned char v;
    __asm__ __volatile__("inb %w1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static __inline int _outp(unsigned short port, int databyte) {
    __asm__ __volatile__("outb %0, %w1" : : "a"((unsigned char)databyte), "Nd"(port));
    return databyte;
}

static __inline unsigned short _inpw(unsigned short port) {
    unsigned short v;
    __asm__ __volatile__("inw %w1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static __inline unsigned short _outpw(unsigned short port, unsigned short dataword) {
    __asm__ __volatile__("outw %0, %w1" : : "a"(dataword), "Nd"(port));
    return dataword;
}

static __inline unsigned long _inpd(unsigned short port) {
    unsigned long v;
    __asm__ __volatile__("inl %w1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static __inline unsigned long _outpd(unsigned short port, unsigned long dataword) {
    __asm__ __volatile__("outl %0, %w1" : : "a"(dataword), "Nd"(port));
    return dataword;
}

#ifdef __cplusplus
}
#endif
