//===- SIMDTest.cpp - SIMD support unit tests -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/SIMD.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <type_traits>

using namespace llvm;

static_assert(simd::vec<std::int32_t>::size() * sizeof(std::int32_t) ==
              simd::detail::NativeVectorBytes);
static_assert(simd::vec<double>::size() * sizeof(double) ==
              simd::detail::NativeVectorBytes);
static_assert(sizeof(simd::vec<std::int32_t>) ==
              simd::detail::NativeVectorBytes);
static_assert(alignof(simd::vec<std::int32_t>) ==
              simd::detail::NativeVectorBytes);
static_assert(simd::alignment_v<simd::vec<std::int32_t>> ==
              simd::detail::NativeVectorBytes);
static_assert(std::is_trivially_copyable_v<simd::vec<std::int32_t>>);
static_assert(std::is_same_v<typename simd::vec<std::int32_t>::mask_type,
                             simd::basic_mask<sizeof(std::int32_t),
                                              simd::native<std::int32_t>>>);

TEST(SIMDTest, UncheckedLoadStoreNativeVector) {
  using Vec = simd::vec<std::int32_t>;
  alignas(64) std::int32_t InputStorage[Vec::size() + 1] = {};
  InputStorage[0] = 99;
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    InputStorage[I + 1] = static_cast<std::int32_t>(I * 3 - 7);
  const std::int32_t *Input = InputStorage + 1;
  const Vec V = simd::unchecked_load(Input, Vec::size());

  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(V[I], Input[I]);

  alignas(64) std::int32_t OutputStorage[Vec::size() + 2];
  for (std::int32_t &Value : OutputStorage)
    Value = 99;
  std::int32_t *Output = OutputStorage + 1;
  simd::unchecked_store(V, Output, Vec::size() + 1);
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(Output[I], Input[I]);
  EXPECT_EQ(OutputStorage[0], 99);
  EXPECT_EQ(OutputStorage[Vec::size() + 1], 99);
}

TEST(SIMDTest, UncheckedLoadStoreFixedSizeVector) {
  using Vec = simd::vec<std::uint32_t, 9>;
  const std::uint32_t Input[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  const Vec V = simd::unchecked_load<Vec>(Input, 9);

  std::uint32_t Output[11] = {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};
  simd::unchecked_store(V, Output, 11);
  for (std::size_t I = 0; I != 9; ++I)
    EXPECT_EQ(Output[I], Input[I]);
  EXPECT_EQ(Output[9], 99u);
  EXPECT_EQ(Output[10], 99u);
}

TEST(SIMDTest, PartialLoadZeroFillsAndPartialStorePreservesTail) {
  using Vec = simd::vec<std::int16_t>;
  const std::int16_t Input[] = {10, 20, 30};
  const Vec V = simd::partial_load<Vec>(Input, 3);

  EXPECT_EQ(V[0], 10);
  EXPECT_EQ(V[1], 20);
  EXPECT_EQ(V[2], 30);
  for (simd::simd_size_type I = 3; I != Vec::size(); ++I)
    EXPECT_EQ(V[I], 0);

  std::int16_t Output[5] = {7, 7, 7, 7, 7};
  simd::partial_store(V, Output, 3);
  EXPECT_EQ(Output[0], 10);
  EXPECT_EQ(Output[1], 20);
  EXPECT_EQ(Output[2], 30);
  EXPECT_EQ(Output[3], 7);
  EXPECT_EQ(Output[4], 7);
}

TEST(SIMDTest, PartialLoadStoreAcceptNullForZeroElements) {
  using Vec = simd::vec<float>;
  const Vec V = simd::partial_load<Vec>(static_cast<const float *>(nullptr), 0);
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(V[I], 0.0f);
  simd::partial_store(V, static_cast<float *>(nullptr), 0);
}

TEST(SIMDTest, AlignmentFlags) {
  using Vec = simd::vec<std::uint8_t>;
  alignas(64) std::uint8_t Input[Vec::size()];
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    Input[I] = static_cast<std::uint8_t>(I);
  const auto Flags = simd::flag_aligned | simd::flag_overaligned<64>;
  const Vec V = simd::unchecked_load(Input, Vec::size(), Flags);

  alignas(64) std::uint8_t Output[Vec::size()] = {};
  simd::unchecked_store(V, Output, Vec::size(), Flags);
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(Output[I], Input[I]);
}

TEST(SIMDTest, ByteComparisonsAndMaskAlgorithms) {
  using Vec = simd::vec<char>;
  char Input[Vec::size()];
  for (char &C : Input)
    C = 'a';
  Input[6] = '!';
  Input[12] = ' ';
  const Vec V = simd::unchecked_load<Vec>(Input, Vec::size());
  const auto IsIdentifier =
      (V == Vec('_')) | ((V >= Vec('A')) & (V <= Vec('Z'))) |
      ((V >= Vec('a')) & (V <= Vec('z'))) | ((V >= Vec('0')) & (V <= Vec('9')));

  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(IsIdentifier[I], I != 6 && I != 12);
  EXPECT_TRUE(simd::any_of(IsIdentifier));
  EXPECT_FALSE(simd::all_of(IsIdentifier));
  EXPECT_EQ(simd::reduce_min_index(!IsIdentifier), 6);
}

TEST(SIMDTest, UnsignedByteComparison) {
  using Vec = simd::vec<std::uint8_t>;
  std::uint8_t Input[Vec::size()];
  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    Input[I] = static_cast<std::uint8_t>(I * 37);
  const Vec V = simd::unchecked_load<Vec>(Input, Vec::size());
  const auto GreaterThan127 = V > Vec(127);

  for (simd::simd_size_type I = 0; I != Vec::size(); ++I)
    EXPECT_EQ(GreaterThan127[I], Input[I] > 127);
}
