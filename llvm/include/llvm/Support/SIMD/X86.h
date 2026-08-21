//===- X86.h - X86-64 SIMD implementation ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SIMD_X86_H
#define LLVM_SUPPORT_SIMD_X86_H

#include <emmintrin.h>

namespace llvm {
namespace simd {
namespace detail {

inline constexpr std::size_t NativeVectorBytes = 16;

template <typename T, std::ptrdiff_t N> struct Operations {
  static constexpr std::size_t Alignment = 16;
  static constexpr std::size_t NumBytes = sizeof(T) * N;
  static constexpr std::size_t NumRegisters = (NumBytes + 15) / 16;

  struct alignas(Alignment) Storage {
    __m128i Data[NumRegisters];
  };

  static void setZero(Storage &S) noexcept {
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = _mm_setzero_si128();
  }

  static void broadcast(Storage &S, T Value) noexcept {
    alignas(16) unsigned char Bytes[16];
    for (std::size_t I = 0; I != 16; I += sizeof(T))
      std::memcpy(Bytes + I, &Value, sizeof(T));
    const __m128i Register =
        _mm_load_si128(reinterpret_cast<const __m128i *>(Bytes));
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = Register;
  }

  [[nodiscard]] static T get(const Storage &S, std::ptrdiff_t I) noexcept {
    const std::size_t ByteOffset = static_cast<std::size_t>(I) * sizeof(T);
    const std::size_t Register = ByteOffset / 16;
    const std::size_t OffsetInRegister = ByteOffset % 16;
    alignas(16) unsigned char Bytes[16];
    _mm_store_si128(reinterpret_cast<__m128i *>(Bytes), S.Data[Register]);
    T Result;
    std::memcpy(&Result, Bytes + OffsetInRegister, sizeof(T));
    return Result;
  }

  static void load(Storage &S, const T *Ptr, std::ptrdiff_t Count,
                   bool IsAligned) noexcept {
    setZero(S);
    const auto *Bytes = reinterpret_cast<const unsigned char *>(Ptr);
    const std::size_t BytesToLoad = static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToLoad / 16;
    const std::size_t TailBytes = BytesToLoad % 16;

    for (std::size_t I = 0; I != FullRegisters; ++I) {
      const auto *Register = reinterpret_cast<const __m128i *>(Bytes + I * 16);
      S.Data[I] =
          IsAligned ? _mm_load_si128(Register) : _mm_loadu_si128(Register);
    }

    if (TailBytes != 0) {
      alignas(16) unsigned char Tail[16] = {};
      std::memcpy(Tail, Bytes + FullRegisters * 16, TailBytes);
      S.Data[FullRegisters] =
          _mm_load_si128(reinterpret_cast<const __m128i *>(Tail));
    }
  }

  static void store(const Storage &S, T *Ptr, std::ptrdiff_t Count,
                    bool IsAligned) noexcept {
    auto *Bytes = reinterpret_cast<unsigned char *>(Ptr);
    const std::size_t BytesToStore =
        static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToStore / 16;
    const std::size_t TailBytes = BytesToStore % 16;

    for (std::size_t I = 0; I != FullRegisters; ++I) {
      auto *Register = reinterpret_cast<__m128i *>(Bytes + I * 16);
      if (IsAligned)
        _mm_store_si128(Register, S.Data[I]);
      else
        _mm_storeu_si128(Register, S.Data[I]);
    }

    if (TailBytes != 0) {
      alignas(16) unsigned char Tail[16];
      _mm_store_si128(reinterpret_cast<__m128i *>(Tail), S.Data[FullRegisters]);
      std::memcpy(Bytes + FullRegisters * 16, Tail, TailBytes);
    }
  }

  [[nodiscard]] static std::uint64_t compareEqual(const Storage &LHS,
                                                  const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    std::uint64_t Bits = 0;
    for (std::size_t I = 0; I != NumRegisters; ++I) {
      const __m128i Equal = _mm_cmpeq_epi8(LHS.Data[I], RHS.Data[I]);
      Bits |= static_cast<std::uint64_t>(
                  static_cast<std::uint16_t>(_mm_movemask_epi8(Equal)))
              << (I * 16);
    }
    return Bits;
  }

  [[nodiscard]] static std::uint64_t
  compareGreater(const Storage &LHS, const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    std::uint64_t Bits = 0;
    const __m128i SignBit = _mm_set1_epi8(static_cast<char>(0x80));
    for (std::size_t I = 0; I != NumRegisters; ++I) {
      __m128i Left = LHS.Data[I];
      __m128i Right = RHS.Data[I];
      if constexpr (!std::is_signed<T>::value) {
        Left = _mm_xor_si128(Left, SignBit);
        Right = _mm_xor_si128(Right, SignBit);
      }
      const __m128i Greater = _mm_cmpgt_epi8(Left, Right);
      Bits |= static_cast<std::uint64_t>(
                  static_cast<std::uint16_t>(_mm_movemask_epi8(Greater)))
              << (I * 16);
    }
    return Bits;
  }
};

} // namespace detail
} // namespace simd
} // namespace llvm

#endif // LLVM_SUPPORT_SIMD_X86_H
