//===- SIMDIdentifierBenchmark.cpp - ASCII identifier benchmark ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "benchmark/benchmark.h"
#include "llvm/Support/SIMD.h"

#include <cstddef>
#include <string>

static bool isASCIIIdentifierContinue(unsigned char C) {
  return C == '_' || (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') ||
         (C >= '0' && C <= '9');
}

static const char *scanASCIIIdentifierScalar(const char *CurPtr,
                                             const char * /*BufferEnd*/) {
  while (isASCIIIdentifierContinue(*CurPtr))
    ++CurPtr;
  return CurPtr;
}

static const char *scanASCIIIdentifierSIMD(const char *CurPtr,
                                           const char *BufferEnd) {
  using CharVec = llvm::simd::vec<char>;
  constexpr auto BytesPerRegister = CharVec::size();
  const CharVec Underscore('_');
  const CharVec UpperA('A');
  const CharVec UpperZ('Z');
  const CharVec LowerA('a');
  const CharVec LowerZ('z');
  const CharVec Digit0('0');
  const CharVec Digit9('9');

  while (BufferEnd - CurPtr >= BytesPerRegister) {
    const CharVec C =
        llvm::simd::unchecked_load<CharVec>(CurPtr, BytesPerRegister);
    const auto IsIdentifier =
        (C == Underscore) | ((C >= UpperA) & (C <= UpperZ)) |
        ((C >= LowerA) & (C <= LowerZ)) | ((C >= Digit0) & (C <= Digit9));
    if (!llvm::simd::all_of(IsIdentifier))
      return CurPtr + llvm::simd::reduce_min_index(!IsIdentifier);
    CurPtr += BytesPerRegister;
  }

  return scanASCIIIdentifierScalar(CurPtr, BufferEnd);
}

using Scanner = const char *(*)(const char *, const char *);

template <Scanner Scan>
static void runScanASCIIIdentifier(benchmark::State &State,
                                   std::size_t VectorBytes) {
  const std::size_t IdentifierLength = static_cast<std::size_t>(State.range(0));
  std::string Input(IdentifierLength, 'a');
  Input.push_back('!');
  const char *Begin = Input.data();
  const char *End = Begin + Input.size();

  if (Scan(Begin, End) != Begin + IdentifierLength) {
    State.SkipWithError("identifier scanner returned the wrong position");
    return;
  }

  for (auto _ : State) {
    benchmark::DoNotOptimize(Begin);
    benchmark::ClobberMemory();
    const char *Result = Scan(Begin, End);
    benchmark::DoNotOptimize(Result);
  }

  State.counters["vector_bytes"] = static_cast<double>(VectorBytes);
  State.SetBytesProcessed(State.iterations() * Input.size());
}

static void BM_ScanASCIIIdentifierScalar(benchmark::State &State) {
  runScanASCIIIdentifier<scanASCIIIdentifierScalar>(State, 1);
}

static void BM_ScanASCIIIdentifierSIMD(benchmark::State &State) {
  runScanASCIIIdentifier<scanASCIIIdentifierSIMD>(
      State, llvm::simd::vec<char>::size());
}

#define IDENTIFIER_LENGTHS                                                     \
  ->Arg(1)                                                                     \
      ->Arg(7)                                                                 \
      ->Arg(15)                                                                \
      ->Arg(16)                                                                \
      ->Arg(17)                                                                \
      ->Arg(31)                                                                \
      ->Arg(32)                                                                \
      ->Arg(63)                                                                \
      ->Arg(64)                                                                \
      ->Arg(65)                                                                \
      ->Arg(256)                                                               \
      ->Arg(1024)                                                              \
      ->Arg(4096)

BENCHMARK(BM_ScanASCIIIdentifierScalar) IDENTIFIER_LENGTHS;
BENCHMARK(BM_ScanASCIIIdentifierSIMD) IDENTIFIER_LENGTHS;

#undef IDENTIFIER_LENGTHS

BENCHMARK_MAIN();
