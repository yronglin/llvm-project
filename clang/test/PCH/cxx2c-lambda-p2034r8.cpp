// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -emit-pch %s -o %t
// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -include-pch %t -DTEST -ast-print %s | FileCheck %s

#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED

inline auto explicit_captures(int x) {
  return [const x, mutable y = x]() const {
    ++y;
    return x + y;
  };
}

template <class T> auto mutable_default(T value) {
  return [mutable =]() const {
    ++value;
    return value;
  };
}

inline auto const_ref_capture(int &value) {
  return [const &value] { return value; };
}

#else

// CHECK: return [const x, mutable y = x]() const {
// CHECK: return [mutable =]() const {
// CHECK: return [const &value] {

#ifdef TEST
void use_imported_lambdas(int &value) {
  const auto a = explicit_captures(value);
  (void)a();
  const auto b = mutable_default(value);
  (void)b();
  (void)const_ref_capture(value)();
}
#endif

#endif
