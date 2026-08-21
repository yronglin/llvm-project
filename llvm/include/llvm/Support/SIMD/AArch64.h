//===- AArch64.h - AArch64 SIMD implementation ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_SIMD_AARCH64_H
#define LLVM_SUPPORT_SIMD_AARCH64_H

#if defined(_MSC_VER)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif

namespace llvm {
namespace simd {
namespace detail {

inline constexpr std::size_t NativeVectorBytes = 16;

template <typename T, simd_size_type N> struct Operations {
  static constexpr std::size_t Alignment = 16;
  static constexpr std::size_t NumBytes = sizeof(T) * N;
  static constexpr std::size_t NumRegisters = (NumBytes + 15) / 16;

  struct alignas(Alignment) Storage {
    uint8x16_t Data[NumRegisters];
  };

  static void setZero(Storage &S) noexcept {
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = vdupq_n_u8(0);
  }

  static void broadcast(Storage &S, T Value) noexcept {
    alignas(16) std::uint8_t Bytes[16];
    for (std::size_t I = 0; I != 16; I += sizeof(T))
      std::memcpy(Bytes + I, &Value, sizeof(T));
    const uint8x16_t Register = vld1q_u8(Bytes);
    for (std::size_t I = 0; I != NumRegisters; ++I)
      S.Data[I] = Register;
  }

  [[nodiscard]] static T get(const Storage &S, simd_size_type I) noexcept {
    const std::size_t ByteOffset = static_cast<std::size_t>(I) * sizeof(T);
    const std::size_t Register = ByteOffset / 16;
    const std::size_t OffsetInRegister = ByteOffset % 16;
    alignas(16) std::uint8_t Bytes[16];
    vst1q_u8(Bytes, S.Data[Register]);
    T Result;
    std::memcpy(&Result, Bytes + OffsetInRegister, sizeof(T));
    return Result;
  }

  static void load(Storage &S, const T *Ptr, simd_size_type Count,
                   bool /*IsAligned*/) noexcept {
    setZero(S);
    const auto *Bytes = reinterpret_cast<const std::uint8_t *>(Ptr);
    const std::size_t BytesToLoad = static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToLoad / 16;
    const std::size_t TailBytes = BytesToLoad % 16;

    for (std::size_t I = 0; I != FullRegisters; ++I)
      S.Data[I] = vld1q_u8(Bytes + I * 16);

    if (TailBytes != 0) {
      alignas(16) std::uint8_t Tail[16] = {};
      std::memcpy(Tail, Bytes + FullRegisters * 16, TailBytes);
      S.Data[FullRegisters] = vld1q_u8(Tail);
    }
  }

  static void store(const Storage &S, T *Ptr, simd_size_type Count,
                    bool /*IsAligned*/) noexcept {
    auto *Bytes = reinterpret_cast<std::uint8_t *>(Ptr);
    const std::size_t BytesToStore =
        static_cast<std::size_t>(Count) * sizeof(T);
    const std::size_t FullRegisters = BytesToStore / 16;
    const std::size_t TailBytes = BytesToStore % 16;

    for (std::size_t I = 0; I != FullRegisters; ++I)
      vst1q_u8(Bytes + I * 16, S.Data[I]);

    if (TailBytes != 0) {
      alignas(16) std::uint8_t Tail[16];
      vst1q_u8(Tail, S.Data[FullRegisters]);
      std::memcpy(Bytes + FullRegisters * 16, Tail, TailBytes);
    }
  }

  [[nodiscard]] static std::uint16_t moveMask(uint8x16_t V) noexcept {
    alignas(8) static constexpr std::uint8_t BitWeights[8] = {1,  2,  4,  8,
                                                              16, 32, 64, 128};
    const uint8x8_t Weights = vld1_u8(BitWeights);
    const uint8x16_t Bits = vshrq_n_u8(V, 7);
    const std::uint8_t Low = vaddv_u8(vmul_u8(vget_low_u8(Bits), Weights));
    const std::uint8_t High = vaddv_u8(vmul_u8(vget_high_u8(Bits), Weights));
    return static_cast<std::uint16_t>(Low) |
           (static_cast<std::uint16_t>(High) << 8);
  }

  [[nodiscard]] static std::uint64_t compareEqual(const Storage &LHS,
                                                  const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    std::uint64_t Bits = 0;
    for (std::size_t I = 0; I != NumRegisters; ++I)
      Bits |= static_cast<std::uint64_t>(
                  moveMask(vceqq_u8(LHS.Data[I], RHS.Data[I])))
              << (I * 16);
    return Bits;
  }

  [[nodiscard]] static std::uint64_t
  compareGreater(const Storage &LHS, const Storage &RHS) noexcept {
    static_assert(sizeof(T) == 1, "only byte comparisons are implemented");
    std::uint64_t Bits = 0;
    for (std::size_t I = 0; I != NumRegisters; ++I) {
      uint8x16_t Greater;
      if constexpr (std::is_signed<T>::value)
        Greater =
            vreinterpretq_u8_s8(vcgtq_s8(vreinterpretq_s8_u8(LHS.Data[I]),
                                         vreinterpretq_s8_u8(RHS.Data[I])));
      else
        Greater = vcgtq_u8(LHS.Data[I], RHS.Data[I]);
      Bits |= static_cast<std::uint64_t>(moveMask(Greater)) << (I * 16);
    }
    return Bits;
  }
};

} // namespace detail
} // namespace simd
} // namespace llvm

#endif // LLVM_SUPPORT_SIMD_AARCH64_H
