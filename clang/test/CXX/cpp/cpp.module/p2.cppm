// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t

// RUN: %clang_cc1 -std=c++20 %t/A.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/B.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/C.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/D.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/E.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/F.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/G.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/H.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/I.cppm -triple x86_64-linux-gnu -verify
// RUN: %clang_cc1 -std=c++20 %t/J.cppm -triple x86_64-linux-gnu -verify

//--- version.h
#ifndef VERSION_H
#define VERSION_H

#define VERSION libv5
#define A a
#define B b
#define C c
#define FUNC_LIKE(X) function_like_##X
#define ATTRS [[]]
#define SEMICOLON ;

#endif // VERSION_H

//--- A.cppm
module;
#include "version.h"
export module VERSION;  // expected-error {{the module name in a module declaration cannot contain an object-like macro 'VERSION', it's consists of one or more identifiers separated by '.'}}

//--- B.cppm
module;
#include "version.h"
export module A.B;      // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                        // expected-error {{the module name in a module declaration cannot contain an object-like macro 'B', it's consists of one or more identifiers separated by '.'}}

//--- C.cppm
module;
#include "version.h"
export module A.FUNC_LIKE(foo):C;   // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                                    // expected-error {{unexpected '(' after the module name in a module declaration}} \
                                    // expected-error {{the module name in a module partition declaration cannot contain an object-like macro 'C', it's consists of one or more identifiers separated by '.'}}

//--- D.cppm
module;
#include "version.h"
export module B.A.FUNC_LIKE(bar):C;   // expected-error {{the module name in a module declaration cannot contain an object-like macro 'B', it's consists of one or more identifiers separated by '.'}} \
                                      // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                                      // expected-error {{unexpected '(' after the module name in a module declaration}} \
                                      // expected-error {{the module name in a module partition declaration cannot contain an object-like macro 'C', it's consists of one or more identifiers separated by '.'}}

//--- E.cppm
module;
#include "version.h"
export module a.FUNC_LIKE:c; // OK, FUNC_LIKE would not be treated as a macro name.
// expected-no-diagnostics

//--- F.cppm
module;
#include "version.h"
export module a.FUNC_LIKE:c ATTRS; // OK, FUNC_LIKE would not be treated as a macro name.
// expected-no-diagnostics

//--- G.cppm
module;
#include "version.h"
export module A.FUNC_LIKE(B c:C ATTRS // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                                      // expected-error {{unexpected '(' after the module name in a module declaration}} \
                                      // expected-error {{the module name in a module partition declaration cannot contain an object-like macro 'C', it's consists of one or more identifiers separated by '.'}} \
                                      // expected-error {{expected ';' after module name}}

//--- H.cppm
module;
#include "version.h"
export module A.FUNC_LIKE(B,). c:C ATTRS  // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                                          // expected-error {{unexpected '(' after the module name in a module declaration}} \
                                          // expected-error {{the module name in a module partition declaration cannot contain an object-like macro 'C', it's consists of one or more identifiers separated by '.'}} \
                                          // expected-error {{expected ';' after module name}}

//--- I.cppm
module;
#include "version.h"
export module A.FUNC_LIKE(B,) c:C ATTRS   // expected-error {{the module name in a module declaration cannot contain an object-like macro 'A', it's consists of one or more identifiers separated by '.'}} \
                                          // expected-error {{unexpected '(' after the module name in a module declaration}} \
                                          // expected-error {{expected ';' after module name}} \
                                          // expected-error {{unknown type name 'c'}} \
                                          // expected-error {{expected unqualified-id}}

//--- J.cppm
module;
#include "version.h"
export module unexpanded : unexpanded ATTRS SEMICOLON // OK, ATTRS and SEMICOLON can be expanded.
// expected-no-diagnostics
