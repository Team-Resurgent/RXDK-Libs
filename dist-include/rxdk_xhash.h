/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// rxdk_xhash.h -- shared guts of the <hash_map> / <hash_set> compatibility headers.
//
// The XDK shipped Visual C++ 7.x, whose standard library carried Dinkumware's
// pre-standard hash containers in namespace stdext. libc++ has no equivalent, so
// these map onto the standard unordered_* containers. That is a re-implementation
// on top of a different data structure, not a port: what is preserved is the
// interface a title compiles against, NOT bucket policy, growth behaviour, or
// iteration order. Code that only inserts and looks up is unaffected; code that
// depends on the order it gets elements back was already relying on something
// Dinkumware never promised.
//
// The interesting mismatch is hash_compare. MSVC bundles the hash AND the
// ordering into one traits object with two call operators, whereas the standard
// containers want a Hash and a KeyEqual as separate types. The two adapters below
// split it apart -- equality is recovered from the strict weak ordering, which is
// what makes the halves consistent with each other by construction.

#ifndef _RXDK_XHASH_H_
#define _RXDK_XHASH_H_

#include <cstddef>
#include <functional>
#include <memory>

namespace stdext {

// MSVC's default hash: cast the key to size_t. That is why the XDK samples give
// their test types an `operator size_t() const` and call them "hashable" -- there
// is no std::hash specialization involved, and adding one would not help, because
// hash_compare is what the container is told to use.
template <class _Kty>
inline std::size_t
hash_value(const _Kty& _Keyval)
{
    return (std::size_t)_Keyval;
}

template <class _Kty, class _Pr = std::less<_Kty>>
class hash_compare
{
  public:
    enum {
        bucket_size = 4, // accepted and ignored; see the header note
        min_buckets = 8
    };

    hash_compare() : comp() {}
    hash_compare(_Pr _Pred) : comp(_Pred) {}

    std::size_t
    operator()(const _Kty& _Keyval) const
    {
        return hash_value(_Keyval);
    }

    bool
    operator()(const _Kty& _Keyval1, const _Kty& _Keyval2) const
    {
        return comp(_Keyval1, _Keyval2);
    }

  protected:
    _Pr comp;
};

namespace _RxdkHashDetail {

// The hash half of a traits object.
template <class _Traits>
struct hasher_of {
    _Traits _Tr;

    hasher_of() : _Tr() {}
    hasher_of(const _Traits& _T) : _Tr(_T) {}

    template <class _Kty>
    std::size_t
    operator()(const _Kty& _Keyval) const
    {
        return _Tr(_Keyval);
    }
};

// The equality half. A traits object gives an ordering rather than an equality,
// so derive one: two keys are equal exactly when neither orders before the other.
// Taking it from the same object is what keeps hash and equality agreeing --
// hashing equal keys differently would silently lose elements.
template <class _Traits>
struct equal_of {
    _Traits _Tr;

    equal_of() : _Tr() {}
    equal_of(const _Traits& _T) : _Tr(_T) {}

    template <class _Kty>
    bool
    operator()(const _Kty& _Left, const _Kty& _Right) const
    {
        return !_Tr(_Left, _Right) && !_Tr(_Right, _Left);
    }
};

// hash_compare's min_buckets is a hint, but the standard containers take a real
// bucket count, so give them one rather than 0.
const std::size_t _Default_buckets = 8;

template <class _Alloc, class _Value>
struct rebound {
    typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Value> type;
};

} // namespace _RxdkHashDetail

} // namespace stdext

#endif // _RXDK_XHASH_H_
