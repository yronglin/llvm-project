// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s

// An id-expression naming an entity captured by const reference has the
// const-qualified type of that entity.

void explicit_const_reference(int x) {
  [const &x]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
    x = 1; // expected-error {{read-only variable is not assignable}}
  }();
}

void implicit_const_reference(int x) {
  [const &]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
    x = 2; // expected-error {{read-only variable is not assignable}}
  }();
}
