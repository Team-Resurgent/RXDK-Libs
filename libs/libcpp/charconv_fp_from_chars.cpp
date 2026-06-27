//===----------------------------------------------------------------------===//
// RXDK-LibsZig: floating-point std::from_chars (string -> float/double).
//
// libc++ 23 delegates its from_chars float parsing to LLVM libc's __support tree
// (libc/shared/{fp_bits,str_to_float,str_to_integer}.h -> libc/src/__support/...),
// which is not vendored here and pulls in a deep dependency web. Rather than
// vendor that, we provide the one out-of-line entry point the <charconv> header
// calls -- std::__from_chars_floating_point<Fp> -- backed by the C library's
// strtof/strtod (C locale). This covers the general/scientific/fixed forms and
// hex floats; it is not the bit-exact correctly-rounded LLVM implementation for
// every edge case, but parses the conforming forms used in practice.
//
// (The matching FP to_chars lives in charconv_fp_to_chars.cpp.)
//===----------------------------------------------------------------------===//

#include <charconv>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <type_traits>

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

template <class _Fp>
__from_chars_result<_Fp>
__from_chars_floating_point(_LIBCPP_NOESCAPE const char* __first, _LIBCPP_NOESCAPE const char* __last, chars_format __fmt) {
  // std::from_chars rejects leading whitespace and a leading '+' (strtod accepts
  // both), so guard against them up front.
  if (__first == __last)
    return {_Fp{}, 0, errc::invalid_argument};
  switch (*__first) {
  case ' ': case '\t': case '\n': case '\r': case '\v': case '\f': case '+':
    return {_Fp{}, 0, errc::invalid_argument};
  default:
    break;
  }

  // strtof/strtod need a NUL-terminated string. A valid float token is short, so
  // copy a bounded prefix. For chars_format::hex, from_chars omits the "0x" that
  // strtod requires, so synthesize it.
  char __buf[512];
  size_t __off = 0;
  if (__fmt == chars_format::hex) {
    __buf[0] = '0';
    __buf[1] = 'x';
    __off = 2;
  }
  size_t __avail = sizeof(__buf) - 1 - __off;
  size_t __len = static_cast<size_t>(__last - __first);
  if (__len > __avail)
    __len = __avail;
  std::memcpy(__buf + __off, __first, __len);
  __buf[__off + __len] = '\0';

  errno = 0;
  char* __end = nullptr;
  _Fp __value;
  if constexpr (is_same_v<_Fp, float>)
    __value = std::strtof(__buf, &__end);
  else
    __value = static_cast<_Fp>(std::strtod(__buf, &__end));

  // Nothing parsed (only the synthesized "0x", or no digits).
  if (__end == __buf + __off)
    return {_Fp{}, 0, errc::invalid_argument};

  ptrdiff_t __n = (__end - __buf) - static_cast<ptrdiff_t>(__off);
  errc __ec = (errno == ERANGE) ? errc::result_out_of_range : errc{};
  return {__value, __n, __ec};
}

template __from_chars_result<float>
__from_chars_floating_point<float>(_LIBCPP_NOESCAPE const char*, _LIBCPP_NOESCAPE const char*, chars_format);
template __from_chars_result<double>
__from_chars_floating_point<double>(_LIBCPP_NOESCAPE const char*, _LIBCPP_NOESCAPE const char*, chars_format);

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD
