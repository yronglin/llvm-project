// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify -verify-ignore-unexpected=note %s

// Check the capture grammar added by P2034R8.

void simple_captures(int x) {
  [mutable x] { x = 1; }();
  [const x]() mutable { (void)x; }();
  [const &x]() mutable { (void)x; }();
}

void capture_defaults(int x) {
  [mutable =] { x = 1; }();
  [const =] { (void)x; }();
  [const &] { (void)x; }();
}

void invalid_mutable_reference(int x) {
  (void)[mutable &x] {}; // expected-error {{a lambda capture by reference cannot be 'mutable'}}
  (void)[mutable &copy = x] {}; // expected-error {{a lambda capture by reference cannot be 'mutable'}}
}

struct qualified_this_capture {
  void f() {
    (void)[mutable this] {}; // expected-error {{capture of 'this' cannot be qualified by mutable}}
    (void)[const this] {}; // expected-error {{capture of 'this' cannot be qualified by const}}
    (void)[mutable *this] {}; // expected-error {{capture of '*this' cannot be qualified by mutable}}
    (void)[const *this] {}; // expected-error {{capture of '*this' cannot be qualified by const}}
  }
};
