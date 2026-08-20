// RUN: %clang_cc1 -std=c++2c -verify %s
// RUN: %clang_cc1 -std=c++2c -Wno-c++2d-extensions -verify=disabled %s
// disabled-no-diagnostics

void extension_diagnostics(int value) {
  (void)[mutable value] {}; // expected-warning {{const and mutable lambda captures are a C++29 extension}}
  (void)[const =] { return value; }; // expected-warning {{const and mutable lambda captures are a C++29 extension}}
  (void)[]() const {}; // expected-warning {{'const' on a lambda expression is a C++29 extension}}
}
