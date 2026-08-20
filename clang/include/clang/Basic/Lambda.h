//===--- Lambda.h - Types for C++ Lambdas -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Defines several types used to describe C++ lambda expressions
/// that are shared between the parser and AST.
///
//===----------------------------------------------------------------------===//


#ifndef LLVM_CLANG_BASIC_LAMBDA_H
#define LLVM_CLANG_BASIC_LAMBDA_H

namespace clang {

/// The default, if any, capture method for a lambda expression.
enum LambdaCaptureDefault {
  LCD_None,
  LCD_ByCopy,
  LCD_ByRef,
  LCD_ByConstCopy,
  LCD_ByMutableCopy,
  LCD_ByConstRef
};

/// An explicit or default qualifier applied to a lambda capture.
enum LambdaCaptureQualifier { LCQ_None, LCQ_Const, LCQ_Mutable };

inline bool isLambdaCaptureDefaultByCopy(LambdaCaptureDefault Default) {
  return Default == LCD_ByCopy || Default == LCD_ByConstCopy ||
         Default == LCD_ByMutableCopy;
}

inline bool isLambdaCaptureDefaultByRef(LambdaCaptureDefault Default) {
  return Default == LCD_ByRef || Default == LCD_ByConstRef;
}

inline LambdaCaptureQualifier
getLambdaCaptureDefaultQualifier(LambdaCaptureDefault Default) {
  switch (Default) {
  case LCD_None:
  case LCD_ByCopy:
  case LCD_ByRef:
    return LCQ_None;
  case LCD_ByConstCopy:
  case LCD_ByConstRef:
    return LCQ_Const;
  case LCD_ByMutableCopy:
    return LCQ_Mutable;
  }
  return LCQ_None;
}

/// The different capture forms in a lambda introducer
///
/// C++11 allows capture of \c this, or of local variables by copy or
/// by reference.  C++1y also allows "init-capture", where the initializer
/// is an expression.
enum LambdaCaptureKind {
  LCK_This,   ///< Capturing the \c *this object by reference
  LCK_StarThis, ///< Capturing the \c *this object by copy
  LCK_ByCopy, ///< Capturing by copy (a.k.a., by value)
  LCK_ByRef,  ///< Capturing by reference
  LCK_VLAType ///< Capturing variable-length array type
};

} // end namespace clang

#endif // LLVM_CLANG_BASIC_LAMBDA_H
