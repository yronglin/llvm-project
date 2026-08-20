// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s

// An explicit simple-capture cannot exactly match the capture-default.

void redundant_defaults(int x) {
  (void)[=, x] {}; // expected-error {{'&' must precede a capture when the capture default is '='}}
  (void)[mutable =, mutable x] {}; // expected-error {{explicit capture of 'x' is redundant with the capture default}}
  (void)[const =, const x] {}; // expected-error {{explicit capture of 'x' is redundant with the capture default}}
  (void)[&, &x] {}; // expected-error {{'&' cannot precede a capture when the capture default is '&'}}
  (void)[const &, const &x] {}; // expected-error {{explicit capture of 'x' is redundant with the capture default}}
}

void explicit_overrides(int x) {
  (void)[=, mutable x] {};
  (void)[mutable =, const x] {};
  (void)[const =, mutable x] {};
  (void)[&, const &x] {};
  (void)[const &, x] {};
}

void init_captures_are_not_redundant(int x) {
  (void)[mutable =, mutable copy = x] { ++copy; };
  (void)[const =, const copy = x] { (void)copy; };
  (void)[const &, const &reference = x] { (void)reference; };
}
