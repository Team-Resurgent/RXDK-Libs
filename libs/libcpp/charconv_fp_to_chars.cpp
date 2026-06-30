//===----------------------------------------------------------------------===//
// RXDK-Libs: floating-point std::to_chars only.
//
// libc++'s charconv.cpp instantiates both to_chars and from_chars for floating
// point, but from_chars_floating_point.h pulls in src/include/shared/*.h, which
// this libc++ snapshot does not vendor. std::format / std::print only need the
// FP to_chars (output) path, so we instantiate just those overloads here. They
// are backed by the Ryu sources (vendor/llvm-project/libcxx/src/ryu/*.cpp,
// compiled by libs/libcpp/build.zig). Definitions mirror charconv.cpp exactly.
//===----------------------------------------------------------------------===//

#include <charconv>

#include "include/to_chars_floating_point.h"

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

to_chars_result to_chars(char* __first, char* __last, float __value) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(__first, __last, __value, chars_format{}, 0);
}

to_chars_result to_chars(char* __first, char* __last, double __value) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(__first, __last, __value, chars_format{}, 0);
}

to_chars_result to_chars(char* __first, char* __last, long double __value) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(
      __first, __last, static_cast<double>(__value), chars_format{}, 0);
}

to_chars_result to_chars(char* __first, char* __last, float __value, chars_format __fmt) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(__first, __last, __value, __fmt, 0);
}

to_chars_result to_chars(char* __first, char* __last, double __value, chars_format __fmt) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(__first, __last, __value, __fmt, 0);
}

to_chars_result to_chars(char* __first, char* __last, long double __value, chars_format __fmt) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(
      __first, __last, static_cast<double>(__value), __fmt, 0);
}

to_chars_result to_chars(char* __first, char* __last, float __value, chars_format __fmt, int __precision) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      __first, __last, __value, __fmt, __precision);
}

to_chars_result to_chars(char* __first, char* __last, double __value, chars_format __fmt, int __precision) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      __first, __last, __value, __fmt, __precision);
}

to_chars_result to_chars(char* __first, char* __last, long double __value, chars_format __fmt, int __precision) {
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      __first, __last, static_cast<double>(__value), __fmt, __precision);
}

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD
