// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s

// Check explicit and implicit capture by const reference.

void const_reference_captures(int x) {
  [const &x]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
    x = 1; // expected-error {{read-only variable is not assignable}}
  }();

  [const &]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
    x = 2; // expected-error {{read-only variable is not assignable}}
  }();
}

template <class... T> void const_reference_pack(T... values) {
  [const &values...] { ((void)values, ...); }();
}

template void const_reference_pack<int, long>(int, long);
