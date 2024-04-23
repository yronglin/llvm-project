//===--- PragmaKinds.h - #pragma comment() kinds  ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_PRAGMAKINDS_H
#define LLVM_CLANG_BASIC_PRAGMAKINDS_H

namespace clang {

enum PragmaMSCommentKind {
  PCK_Unknown,
  PCK_Linker,   // #pragma comment(linker, ...)
  PCK_Lib,      // #pragma comment(lib, ...)
  PCK_Compiler, // #pragma comment(compiler, ...)
  PCK_ExeStr,   // #pragma comment(exestr, ...)
  PCK_User      // #pragma comment(user, ...)
};

enum PragmaMSStructKind {
  PMSST_OFF, // #pragms ms_struct off
  PMSST_ON   // #pragms ms_struct on
};

enum PragmaFloatControlKind {
  PFC_Unknown,
  PFC_Precise,   // #pragma float_control(precise, [,on])
  PFC_NoPrecise, // #pragma float_control(precise, off)
  PFC_Except,    // #pragma float_control(except [,on])
  PFC_NoExcept,  // #pragma float_control(except, off)
  PFC_Push,      // #pragma float_control(push)
  PFC_Pop        // #pragma float_control(pop)
};

enum PragmaFPKind {
  PFK_Contract,    // #pragma clang fp contract
  PFK_Reassociate, // #pragma clang fp reassociate
  PFK_Reciprocal,  // #pragma clang fp reciprocal
  PFK_Exceptions,  // #pragma clang fp exceptions
  PFK_EvalMethod   // #pragma clang fp eval_method
};

enum PragmaLoopHintKind {
  PLHK_clang_loop_vectorize,                       // #pragma clang loop vectorize
  PLHK_clang_loop_vectorize_width,                 // #pragma clang loop vectorize_width
  PLHK_clang_loop_interleave,                      // #pragma clang loop interleave
  PLHK_clang_loop_vectorize_predicate,             // #pragma clang loop vectorize_predicate
  PLHK_clang_loop_interleave_count,                // #pragma clang loop interleave_count
  PLHK_clang_loop_unroll,                          // #pragma clang loop unroll
  PLHK_clang_loop_unroll_count,                    // #pragma clang loop unroll_count
  PLHK_clang_loop_pipeline,                        // #pragma clang loop pipeline
  PLHK_clang_loop_pipeline_initiation_interval,    // #pragma clang loop pipeline_initiation_interval
  PLHK_clang_loop_distribute,                      // #pragma clang loop distribute
  PLHK_unroll,                                     // #pragma unroll
  PLHK_nounroll,                                   // #pragma nounroll
  PLHK_unroll_and_jam,                             // #pragma unroll_and_jam
  PLHK_nounroll_and_jam                             // #pragma nounroll_and_jam
};

enum PragmaLoopHintStateKind {
  PLHSK_enable,
  PLHSK_disable,
  PLHSK_numeric,
  PLHSK_fixed_width,
  PLHSK_scalable_width,
  PLHSK_assume_safety,
  PLHSK_full
};
} // namespace clang

#endif
