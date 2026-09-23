/* C <endian.h> byte-order primitives on the little-endian original Xbox (x86).
   The consolidated picolibc branch derives native order from the compiler's
   __BYTE_ORDER__, and only C++ <bit> and C23 <stdbit.h> asserted little-endian
   here -- the C endian.h surface (BYTE_ORDER, the htobe/htole and be/le-toh
   families, __builtin_bswap) had no test. On a little-endian host the
   host-to-little and little-to-host conversions are identity and the big-endian
   ones are byte swaps. (The 360 copy of this test asserts the mirror image.) */
#include "rxdk_test.h"
#include <sys/types.h>   /* __uint16_t/__uint32_t/__uint64_t the endian.h macros cast to */
#include <endian.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    /* the platform is little-endian: both the macro and the actual memory layout */
    CHECK(BYTE_ORDER == LITTLE_ENDIAN, "BYTE_ORDER == LITTLE_ENDIAN");
    CHECK(BIG_ENDIAN != LITTLE_ENDIAN, "BIG_ENDIAN != LITTLE_ENDIAN");

    uint32_t v = 0x01020304u;
    unsigned char b[4];
    memcpy(b, &v, 4);
    CHECK(b[0] == 0x04 && b[1] == 0x03 && b[2] == 0x02 && b[3] == 0x01,
          "uint32 is stored LSB-first in memory (little-endian)");

    /* __builtin_bswap correctness (endian-agnostic: always swaps) */
    CHECK((int)__builtin_bswap16(0x1234u) == 0x3412, "bswap16");
    CHECK_EQI((long)__builtin_bswap32(0x12345678u), (long)0x78563412u, "bswap32");
    CHECK(__builtin_bswap64(0x0123456789ABCDEFull) == 0xEFCDAB8967452301ull, "bswap64");

    /* host<->little is identity on a little-endian host */
    CHECK((int)htole16(0x1234u) == 0x1234, "htole16 identity (LE host)");
    CHECK_EQI((long)htole32(0x12345678u), (long)0x12345678u, "htole32 identity (LE host)");
    CHECK(htole64(0x0123456789ABCDEFull) == 0x0123456789ABCDEFull, "htole64 identity (LE host)");
    CHECK((int)le16toh(0x1234u) == 0x1234, "le16toh identity (LE host)");
    CHECK_EQI((long)le32toh(0x12345678u), (long)0x12345678u, "le32toh identity (LE host)");

    /* host<->big is a byte swap on a little-endian host */
    CHECK((int)htobe16(0x1234u) == 0x3412, "htobe16 swaps (LE host)");
    CHECK_EQI((long)htobe32(0x12345678u), (long)0x78563412u, "htobe32 swaps (LE host)");
    CHECK(htobe64(0x0123456789ABCDEFull) == 0xEFCDAB8967452301ull, "htobe64 swaps (LE host)");
    CHECK((int)be16toh(0x1234u) == 0x3412, "be16toh swaps (LE host)");
    CHECK_EQI((long)be32toh(0x12345678u), (long)0x78563412u, "be32toh swaps (LE host)");

    CHECK_DONE("endian");
    return 0;
}
