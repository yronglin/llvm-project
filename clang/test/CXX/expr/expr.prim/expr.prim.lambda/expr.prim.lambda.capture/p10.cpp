// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s
// expected-no-diagnostics

// Check the non-static data members declared for qualified copy captures.

void member_types() {
  const int cx = 1;
  volatile int vx = 2;

  [mutable cx] {
    static_assert(__is_same(decltype((cx)), int &));
    cx = 3;
  }();
  [const vx]() mutable {
    static_assert(__is_same(decltype((vx)), const int &));
  }();
}

void array_member_types() {
  const int values[] = {1, 2};
  [mutable values] {
    static_assert(__is_same(decltype((values)), int (&)[2]));
    values[0] = 3;
  }();
  [const values]() mutable {
    static_assert(__is_same(decltype((values)), const int (&)[2]));
  }();
}

void target_function() {}

void function_reference_qualifiers_are_inert() {
  void (&f)() = target_function;
  [mutable f] { f(); }();
  [const f] { f(); }();
}

template <class... T> void simple_capture_packs(T... values) {
  [mutable values...] { ((values = 0), ...); }();
  [const values...] { ((void)values, ...); }();
}

template void simple_capture_packs<int, long>(int, long);
