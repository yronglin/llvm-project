// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s
// expected-no-diagnostics

// A qualified init-capture deduces its type as an auto declaration with the
// capture qualifier applied.

void init_captures(const int source) {
  [mutable value = source] {
    static_assert(__is_same(decltype(value), int));
    ++value;
  }();
  [const value = source]() mutable {
    static_assert(__is_same(decltype(value), const int));
  }();
  [const &value = source]() mutable {
    static_assert(__is_same(decltype(value), const int &));
  }();
}

template <class... T> void init_capture_packs(T... values) {
  [mutable ...copies = values] { ((copies = 0), ...); }();
  [const ...copies = values] { ((void)copies, ...); }();
  [const &...references = values] { ((void)references, ...); }();
}

template void init_capture_packs<int, long>(int, long);
