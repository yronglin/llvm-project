// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify -verify-ignore-unexpected=note %s

// A qualified capture-default does not implicitly capture *this.

struct qualified_default_this {
  int value;

  void explicit_capture() {
    [mutable =, this] { value = 1; }();
    [const =, this] { value = 2; }();
    [const &, this] { value = 3; }();
    [mutable =, *this]() mutable { value = 4; }();
    [const =, *this] { (void)value; }();
    [const &, *this] { (void)value; }();
  }

  void no_implicit_capture() {
    (void)[mutable =] { return value; }; // expected-error {{'this' cannot be implicitly captured in this context}}
    (void)[const =] { return value; }; // expected-error {{'this' cannot be implicitly captured in this context}}
    (void)[const &] { return value; }; // expected-error {{'this' cannot be implicitly captured in this context}}
  }
};
