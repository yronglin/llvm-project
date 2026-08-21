//===- llvm/Support/SIMD.h - Small std::simd subset -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file provides a small subset of the C++26 std::simd interface. It is
/// intentionally limited to the operations currently needed by LLVM:
/// same-type pointer loads and stores, byte comparisons, mask operations, and
/// mask reductions.
///
/// The public interface is target-independent. X86-64 uses baseline SSE2,
/// AVX512BW builds use 512-bit vectors, AArch64 uses NEON, and other targets
/// use a portable implementation. Fixed-size vectors may contain 1 to 64
/// elements.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SIMD_H
#define LLVM_SUPPORT_SIMD_H

#include "llvm/ADT/bit.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace llvm {
namespace simd {

using simd_size_type = std::ptrdiff_t;

template <simd_size_type N> struct fixed_size {
  static_assert(N > 0 && N <= 64,
                "llvm::simd supports between 1 and 64 elements");
};

} // namespace simd
} // namespace llvm

#if defined(__AVX512BW__) &&                                                   \
    ((defined(__x86_64__) || defined(_M_X64)) && !defined(_M_ARM64EC))
#include "llvm/Support/SIMD/X86AVX512.h"
#elif (defined(__x86_64__) || defined(_M_X64)) && !defined(_M_ARM64EC)
#include "llvm/Support/SIMD/X86.h"
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#include "llvm/Support/SIMD/AArch64.h"
#else
namespace llvm {
namespace simd {
namespace detail {

inline constexpr std::size_t NativeVectorBytes = 16;

template <typename T, simd_size_type N> struct Operations {
  static constexpr std::size_t Alignment = 16;

  struct alignas(Alignment) Storage {
    T Data[N] = {};
  };

  static void setZero(Storage &S) noexcept {
    for (simd_size_type I = 0; I != N; ++I)
      S.Data[I] = T{};
  }

  static void broadcast(Storage &S, T Value) noexcept {
    for (simd_size_type I = 0; I != N; ++I)
      S.Data[I] = Value;
  }

  [[nodiscard]] static T get(const Storage &S, simd_size_type I) noexcept {
    return S.Data[I];
  }

  static void load(Storage &S, const T *Ptr, simd_size_type Count,
                   bool /*IsAligned*/) noexcept {
    setZero(S);
    std::memcpy(S.Data, Ptr, static_cast<std::size_t>(Count) * sizeof(T));
  }

  static void store(const Storage &S, T *Ptr, simd_size_type Count,
                    bool /*IsAligned*/) noexcept {
    std::memcpy(Ptr, S.Data, static_cast<std::size_t>(Count) * sizeof(T));
  }

  [[nodiscard]] static std::uint64_t compareEqual(const Storage &LHS,
                                                  const Storage &RHS) noexcept {
    std::uint64_t Bits = 0;
    for (simd_size_type I = 0; I != N; ++I)
      Bits |= std::uint64_t{LHS.Data[I] == RHS.Data[I]} << I;
    return Bits;
  }

  [[nodiscard]] static std::uint64_t
  compareGreater(const Storage &LHS, const Storage &RHS) noexcept {
    std::uint64_t Bits = 0;
    for (simd_size_type I = 0; I != N; ++I)
      Bits |= std::uint64_t{LHS.Data[I] > RHS.Data[I]} << I;
    return Bits;
  }
};

} // namespace detail
} // namespace simd
} // namespace llvm
#endif

namespace llvm {
namespace simd {

template <typename T>
using native = fixed_size<detail::NativeVectorBytes / sizeof(T)>;

namespace detail {

struct AlignedFlag {};

template <std::size_t N> struct OveralignedFlag {
  static_assert(N != 0 && (N & (N - 1)) == 0,
                "an overalignment must be a power of two");
};

template <typename T> struct IsFlag : std::false_type {};
template <> struct IsFlag<AlignedFlag> : std::true_type {};
template <std::size_t N> struct IsFlag<OveralignedFlag<N>> : std::true_type {};

template <typename T>
struct Overalignment : std::integral_constant<std::size_t, 0> {};
template <std::size_t N>
struct Overalignment<OveralignedFlag<N>>
    : std::integral_constant<std::size_t, N> {};

template <typename T>
struct IsVectorizable
    : std::integral_constant<bool, (std::is_integral<T>::value ||
                                    std::is_floating_point<T>::value) &&
                                       !std::is_same<T, bool>::value &&
                                       !std::is_same<T, long double>::value &&
                                       !std::is_const<T>::value &&
                                       !std::is_volatile<T>::value> {};

template <typename T>
inline constexpr bool IsVectorizableV = IsVectorizable<T>::value;

template <std::size_t Bytes> struct IntegerFrom;
template <> struct IntegerFrom<1> {
  using type = std::uint8_t;
};
template <> struct IntegerFrom<2> {
  using type = std::uint16_t;
};
template <> struct IntegerFrom<4> {
  using type = std::uint32_t;
};
template <> struct IntegerFrom<8> {
  using type = std::uint64_t;
};

template <typename Vec> struct VecAccess;
template <typename Mask> struct MaskAccess;

} // namespace detail

template <typename... Flags> struct flags {
  static_assert((detail::IsFlag<Flags>::value && ...),
                "unsupported llvm::simd load/store flag");
  static_assert((0 + ... +
                 (detail::Overalignment<Flags>::value != 0 ? 1 : 0)) <= 1,
                "at most one overalignment may be specified");
};

template <typename... LHS, typename... RHS>
constexpr flags<LHS..., RHS...> operator|(flags<LHS...>,
                                          flags<RHS...>) noexcept {
  return {};
}

inline constexpr flags<> flag_default{};
inline constexpr flags<detail::AlignedFlag> flag_aligned{};
template <std::size_t N>
inline constexpr flags<detail::OveralignedFlag<N>> flag_overaligned{};

template <std::size_t Bytes,
          typename Abi = native<typename detail::IntegerFrom<Bytes>::type>,
          typename = void>
class basic_mask {
public:
  using value_type = bool;
  using abi_type = Abi;

  basic_mask() = delete;
  ~basic_mask() = delete;
  basic_mask(const basic_mask &) = delete;
  basic_mask &operator=(const basic_mask &) = delete;
};

template <std::size_t Bytes, simd_size_type N>
class basic_mask<
    Bytes, fixed_size<N>,
    std::enable_if_t<Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes == 8>> {
  static constexpr std::uint64_t AllBits =
      N == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << N) - 1;

  std::uint64_t Bits = 0;

  template <typename Mask> friend struct detail::MaskAccess;

public:
  using value_type = bool;
  using abi_type = fixed_size<N>;

  inline static constexpr std::integral_constant<simd_size_type, N> size{};

  basic_mask() noexcept = default;
  explicit basic_mask(bool Value) noexcept : Bits(Value ? AllBits : 0) {}

  [[nodiscard]] bool operator[](simd_size_type I) const noexcept {
    assert(I >= 0 && I < N && "llvm::simd mask index out of bounds");
    return (Bits >> I) & 1;
  }
};

namespace detail {

template <std::size_t Bytes, simd_size_type N>
struct MaskAccess<basic_mask<Bytes, fixed_size<N>>> {
  using Mask = basic_mask<Bytes, fixed_size<N>>;

  static constexpr std::uint64_t allBits() { return Mask::AllBits; }
  static std::uint64_t get(const Mask &M) { return M.Bits; }
  static Mask make(std::uint64_t Bits) {
    Mask Result;
    Result.Bits = Bits & allBits();
    return Result;
  }
};

} // namespace detail

template <typename T, typename Abi = native<T>, typename = void>
class basic_vec {
public:
  using value_type = T;
  using abi_type = Abi;
  using mask_type = basic_mask<sizeof(T), Abi>;

  basic_vec() = delete;
  ~basic_vec() = delete;
  basic_vec(const basic_vec &) = delete;
  basic_vec &operator=(const basic_vec &) = delete;
};

template <typename T, simd_size_type N>
class basic_vec<T, fixed_size<N>,
                std::enable_if_t<detail::IsVectorizableV<T>>> {
  using Operations = detail::Operations<T, N>;
  typename Operations::Storage Data;

  template <typename Vec> friend struct detail::VecAccess;

public:
  using value_type = T;
  using abi_type = fixed_size<N>;
  using mask_type = basic_mask<sizeof(value_type), abi_type>;

  inline static constexpr std::integral_constant<simd_size_type, N> size{};

  basic_vec() noexcept { Operations::setZero(Data); }

  explicit basic_vec(value_type Value) noexcept {
    Operations::broadcast(Data, Value);
  }

  [[nodiscard]] value_type operator[](simd_size_type I) const noexcept {
    assert(I >= 0 && I < N && "llvm::simd vector index out of bounds");
    return Operations::get(Data, I);
  }
};

namespace detail {

template <typename T, simd_size_type N, typename Enable>
struct VecAccess<basic_vec<T, fixed_size<N>, Enable>> {
  using Vec = basic_vec<T, fixed_size<N>, Enable>;
  using Impl = Operations<T, N>;
  using Storage = typename Impl::Storage;

  static Storage &get(Vec &V) { return V.Data; }
  static const Storage &get(const Vec &V) { return V.Data; }
};

} // namespace detail

template <typename Vec, typename U = typename Vec::value_type> struct alignment;

template <typename T, simd_size_type N, typename Enable, typename U>
struct alignment<basic_vec<T, fixed_size<N>, Enable>, U>
    : std::integral_constant<std::size_t, detail::Operations<T, N>::Alignment> {
  static_assert(std::is_same<T, U>::value,
                "llvm::simd only supports same-type loads and stores");
};

template <typename Vec, typename U = typename Vec::value_type>
inline constexpr std::size_t alignment_v = alignment<Vec, U>::value;

namespace detail {

template <typename... Flags> struct FlagTraits {
  inline static constexpr bool HasAligned =
      (std::is_same<Flags, AlignedFlag>::value || ...);
  inline static constexpr std::size_t Overaligned =
      (std::size_t{0} | ... | Overalignment<Flags>::value);
};

template <typename Vec, typename... Flags>
constexpr std::size_t requiredAlignment(flags<Flags...>) {
  using Traits = FlagTraits<Flags...>;
  constexpr std::size_t Base =
      Traits::HasAligned ? alignment_v<Vec> : alignof(typename Vec::value_type);
  return Base < Traits::Overaligned ? Traits::Overaligned : Base;
}

template <typename Vec, typename... Flags>
void checkPointer([[maybe_unused]] const typename Vec::value_type *Ptr,
                  simd_size_type Count, flags<Flags...> F) {
  assert(Count >= 0 && "llvm::simd load/store count is negative");
  if (Count == 0)
    return;
  assert(Ptr && "llvm::simd load/store pointer is null");
  constexpr std::size_t Alignment = requiredAlignment<Vec>(F);
  static_assert(Alignment >= alignof(typename Vec::value_type),
                "overalignment is weaker than the element alignment");
  if constexpr (Alignment > alignof(typename Vec::value_type))
    assert(reinterpret_cast<std::uintptr_t>(Ptr) % Alignment == 0 &&
           "llvm::simd pointer does not satisfy its alignment flag");
}

template <typename Vec, typename... Flags>
[[nodiscard]] Vec load(const typename Vec::value_type *Ptr,
                       simd_size_type Count, flags<Flags...> F) noexcept {
  checkPointer<Vec>(Ptr, Count, F);
  Vec Result;
  const simd_size_type Elements = std::min(Count, Vec::size());
  if (Elements != 0) {
    using Impl = Operations<typename Vec::value_type, Vec::size()>;
    constexpr bool IsAligned = requiredAlignment<Vec>(F) >= Impl::Alignment;
    Impl::load(VecAccess<Vec>::get(Result), Ptr, Elements, IsAligned);
  }
  return Result;
}

template <typename Vec, typename... Flags>
void store(const Vec &V, typename Vec::value_type *Ptr, simd_size_type Count,
           flags<Flags...> F) noexcept {
  checkPointer<Vec>(Ptr, Count, F);
  const simd_size_type Elements = std::min(Count, Vec::size());
  if (Elements != 0) {
    using Impl = Operations<typename Vec::value_type, Vec::size()>;
    constexpr bool IsAligned = requiredAlignment<Vec>(F) >= Impl::Alignment;
    Impl::store(VecAccess<Vec>::get(V), Ptr, Elements, IsAligned);
  }
}

template <typename V, typename T>
using LoadVec = std::conditional_t<std::is_void<V>::value, basic_vec<T>, V>;

} // namespace detail

template <typename V = void, typename T, typename... Flags>
[[nodiscard]] detail::LoadVec<V, T>
unchecked_load(const T *Ptr, [[maybe_unused]] simd_size_type Count,
               flags<Flags...> F = {}) noexcept {
  using Result = detail::LoadVec<V, T>;
  static_assert(std::is_same<T, typename Result::value_type>::value,
                "llvm::simd only supports same-type loads");
  assert(Count >= Result::size() &&
         "llvm::simd::unchecked_load requires count >= V::size()");
  return detail::load<Result>(Ptr, Result::size(), F);
}

template <typename V = void, typename T, typename... Flags>
[[nodiscard]] detail::LoadVec<V, T>
partial_load(const T *Ptr, simd_size_type Count,
             flags<Flags...> F = {}) noexcept {
  using Result = detail::LoadVec<V, T>;
  static_assert(std::is_same<T, typename Result::value_type>::value,
                "llvm::simd only supports same-type loads");
  return detail::load<Result>(Ptr, Count, F);
}

template <typename T, typename Abi, typename Enable, typename... Flags>
void unchecked_store(const basic_vec<T, Abi, Enable> &V, T *Ptr,
                     [[maybe_unused]] simd_size_type Count,
                     flags<Flags...> F = {}) noexcept {
  assert(Count >= V.size() &&
         "llvm::simd::unchecked_store requires count >= V::size()");
  detail::store(V, Ptr, V.size(), F);
}

template <typename T, typename Abi, typename Enable, typename... Flags>
void partial_store(const basic_vec<T, Abi, Enable> &V, T *Ptr,
                   simd_size_type Count, flags<Flags...> F = {}) noexcept {
  detail::store(V, Ptr, Count, F);
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] basic_mask<Bytes, fixed_size<N>>
operator&(const basic_mask<Bytes, fixed_size<N>> &LHS,
          const basic_mask<Bytes, fixed_size<N>> &RHS) noexcept {
  using Mask = basic_mask<Bytes, fixed_size<N>>;
  return detail::MaskAccess<Mask>::make(detail::MaskAccess<Mask>::get(LHS) &
                                        detail::MaskAccess<Mask>::get(RHS));
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] basic_mask<Bytes, fixed_size<N>>
operator|(const basic_mask<Bytes, fixed_size<N>> &LHS,
          const basic_mask<Bytes, fixed_size<N>> &RHS) noexcept {
  using Mask = basic_mask<Bytes, fixed_size<N>>;
  return detail::MaskAccess<Mask>::make(detail::MaskAccess<Mask>::get(LHS) |
                                        detail::MaskAccess<Mask>::get(RHS));
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] basic_mask<Bytes, fixed_size<N>>
operator!(const basic_mask<Bytes, fixed_size<N>> &M) noexcept {
  using Mask = basic_mask<Bytes, fixed_size<N>>;
  return detail::MaskAccess<Mask>::make(~detail::MaskAccess<Mask>::get(M));
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] bool all_of(const basic_mask<Bytes, fixed_size<N>> &M) noexcept {
  using Mask = basic_mask<Bytes, fixed_size<N>>;
  return detail::MaskAccess<Mask>::get(M) ==
         detail::MaskAccess<Mask>::allBits();
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] bool any_of(const basic_mask<Bytes, fixed_size<N>> &M) noexcept {
  return detail::MaskAccess<basic_mask<Bytes, fixed_size<N>>>::get(M) != 0;
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] bool none_of(const basic_mask<Bytes, fixed_size<N>> &M) noexcept {
  return !any_of(M);
}

template <std::size_t Bytes, simd_size_type N>
[[nodiscard]] simd_size_type
reduce_min_index(const basic_mask<Bytes, fixed_size<N>> &M) noexcept {
  const std::uint64_t Bits =
      detail::MaskAccess<basic_mask<Bytes, fixed_size<N>>>::get(M);
  assert(Bits != 0 && "llvm::simd::reduce_min_index requires a set element");
  return llvm::countr_zero(Bits);
}

namespace detail {

template <typename T>
inline constexpr bool IsComparableByteV =
    std::is_integral<T>::value && sizeof(T) == 1;

template <typename T, simd_size_type N, typename Enable>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
compareEqual(const basic_vec<T, fixed_size<N>, Enable> &LHS,
             const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  using Vec = basic_vec<T, fixed_size<N>, Enable>;
  using Mask = typename Vec::mask_type;
  using Impl = Operations<T, N>;
  return MaskAccess<Mask>::make(
      Impl::compareEqual(VecAccess<Vec>::get(LHS), VecAccess<Vec>::get(RHS)));
}

template <typename T, simd_size_type N, typename Enable>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
compareGreater(const basic_vec<T, fixed_size<N>, Enable> &LHS,
               const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  using Vec = basic_vec<T, fixed_size<N>, Enable>;
  using Mask = typename Vec::mask_type;
  using Impl = Operations<T, N>;
  return MaskAccess<Mask>::make(
      Impl::compareGreater(VecAccess<Vec>::get(LHS), VecAccess<Vec>::get(RHS)));
}

} // namespace detail

template <typename T, simd_size_type N, typename Enable,
          std::enable_if_t<detail::IsComparableByteV<T>, int> = 0>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
operator==(const basic_vec<T, fixed_size<N>, Enable> &LHS,
           const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  return detail::compareEqual(LHS, RHS);
}

template <typename T, simd_size_type N, typename Enable,
          std::enable_if_t<detail::IsComparableByteV<T>, int> = 0>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
operator>(const basic_vec<T, fixed_size<N>, Enable> &LHS,
          const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  return detail::compareGreater(LHS, RHS);
}

template <typename T, simd_size_type N, typename Enable,
          std::enable_if_t<detail::IsComparableByteV<T>, int> = 0>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
operator<(const basic_vec<T, fixed_size<N>, Enable> &LHS,
          const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  return RHS > LHS;
}

template <typename T, simd_size_type N, typename Enable,
          std::enable_if_t<detail::IsComparableByteV<T>, int> = 0>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
operator>=(const basic_vec<T, fixed_size<N>, Enable> &LHS,
           const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  return !(LHS < RHS);
}

template <typename T, simd_size_type N, typename Enable,
          std::enable_if_t<detail::IsComparableByteV<T>, int> = 0>
[[nodiscard]] typename basic_vec<T, fixed_size<N>, Enable>::mask_type
operator<=(const basic_vec<T, fixed_size<N>, Enable> &LHS,
           const basic_vec<T, fixed_size<N>, Enable> &RHS) noexcept {
  return !(LHS > RHS);
}

template <typename T, simd_size_type N = basic_vec<T>::size()>
using vec = basic_vec<T, fixed_size<N>>;

template <typename T, simd_size_type N = basic_vec<T>::size()>
using mask = basic_mask<sizeof(T), fixed_size<N>>;

} // namespace simd
} // namespace llvm

#endif // LLVM_SUPPORT_SIMD_H
