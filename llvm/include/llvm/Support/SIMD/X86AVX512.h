//===- X86AVX512.h - X86 AVX-512 SIMD implementation ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SIMD_X86AVX512_H
#define LLVM_SUPPORT_SIMD_X86AVX512_H

#include <immintrin.h>

namespace llvm {
namespace simd {
namespace detail {

inline constexpr std::size_t NativeVectorBytes = 64;

template <typename T, simd_size_type N> struct Operations {
  static constexpr std::size_t Alignment = 64;
  static constexpr std::size_t NumBytes = sizeof(T) * N;
  static constexpr std::size_t NumRegisters = (NumBytes + 63) / 64;

  struct alignas(Alignment) Storage {
    __m512i Data[NumRegisters];
  };

  static void setZero(Storage &S) noexcept {
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = _mm512_setzero_si512();
  }

  static void broadcast(Storage &S, T Value) noexcept {
    alignas(Alignment) unsigned char Bytes[64];
    for (std::size_t I = 0; I != 64; I += sizeof(T))
      std::memcpy(Bytes + I, &Value, sizeof(T));
    const __m512i Register = _mm512_load_si512(Bytes);
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = Register;
  }

  [[nodiscard]] static T get(const Storage &S, simd_size_type I) noexcept {
    const std::size_t ByteOffset = static_cast<std::size_t>(I) * sizeof(T);
    const std::size_t Register = ByteOffset / 64;
    const std::size_t OffsetInRegister = ByteOffset % 64;
    alignas(Alignment) unsigned char Bytes[64];
    _mm512_store_si512(Bytes, S.Data[Register]);
    T Result;
    std::memcpy(&Result, Bytes + OffsetInRegister, sizeof(T));
    return Result;
  }

  static void load(Storage &S, const T *Ptr, simd_size_type Count,
                   bool IsAligned) noexcept {
    setZero(S);
    const auto *Bytes = reinterpret_cast<const unsigned char *>(Ptr);
    const std::size_t BytesToLoad = static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToLoad / 64;
    const std::size_t TailBytes = BytesToLoad % 64;

    for (std::size_t I = 0; I != FullRegisters; ++I) {
      const void *Register = Bytes + I * 64;
      S.Data[I] = IsAligned ? _mm512_load_si512(Register)
                            : _mm512_loadu_si512(Register);
    }

    if (TailBytes != 0) {
      alignas(Alignment) unsigned char Tail[64] = {};
      std::memcpy(Tail, Bytes + FullRegisters * 64, TailBytes);
      S.Data[FullRegisters] = _mm512_load_si512(Tail);
    }
  }

  static void store(const Storage &S, T *Ptr, simd_size_type Count,
                    bool IsAligned) noexcept {
    auto *Bytes = reinterpret_cast<unsigned char *>(Ptr);
    const std::size_t BytesToStore =
        static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToStore / 64;
    const std::size_t TailBytes = BytesToStore % 64;

    for (std::size_t I = 0; I != FullRegisters; ++I) {
      void *Register = Bytes + I * 64;
      if (IsAligned)
        _mm512_store_si512(Register, S.Data[I]);
      else
        _mm512_storeu_si512(Register, S.Data[I]);
    }

    if (TailBytes != 0) {
      alignas(Alignment) unsigned char Tail[64];
      _mm512_store_si512(Tail, S.Data[FullRegisters]);
      std::memcpy(Bytes + FullRegisters * 64, Tail, TailBytes);
    }
  }

  [[nodiscard]] static std::uint64_t compareEqual(const Storage &LHS,
                                                  const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    static_assert(NumRegisters == 1,
                  "byte vectors contain at most 64 elements");
    return static_cast<std::uint64_t>(
        _mm512_cmpeq_epi8_mask(LHS.Data[0], RHS.Data[0]));
  }

  [[nodiscard]] static std::uint64_t
  compareGreater(const Storage &LHS, const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    static_assert(NumRegisters == 1,
                  "byte vectors contain at most 64 elements");
    if constexpr (std::is_signed<T>::value)
      return static_cast<std::uint64_t>(
          _mm512_cmpgt_epi8_mask(LHS.Data[0], RHS.Data[0]));
    return static_cast<std::uint64_t>(
        _mm512_cmp_epu8_mask(LHS.Data[0], RHS.Data[0], _MM_CMPINT_GT));
  }
};

} // namespace detail
} // namespace simd
} // namespace llvm

#endif // LLVM_SUPPORT_SIMD_X86AVX512_H
