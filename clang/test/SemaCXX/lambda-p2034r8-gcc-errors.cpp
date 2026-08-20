// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify -verify-ignore-unexpected=note %s

// Adapted from gcc/testsuite/g++.dg/cpp0x/lambda/lambda-p2034-err.C on the GCC
// lambda-p2034 development branch. The expectations for test14 through
// test21 follow P2034R8: mutable copy captures remove top-level cv, while
// mutable reference captures are ill-formed.

void test01() {
  int x = 42;
  int y = 666;
  auto z = [const x, mutable y]() {
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test02() {
  const int x = 42;
  int y = 666;
  auto z = [const x, mutable y]() {
    x = 666; // expected-error {{cannot assign to variable 'x' with const-qualified type 'const int'}}
    y = 42;
  };
}

void test03() {
  int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() {
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test04() {
  const int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() {
    x = 666; // expected-error {{cannot assign to variable 'x' with const-qualified type 'const int'}}
    y = 42;
  };
}

void test05() {
  int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() mutable {
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test06() {
  const int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() mutable {
    x = 666; // expected-error {{cannot assign to variable 'x' with const-qualified type 'const int'}}
    y = 42;
  };
}

void test07() {
  int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() const mutable { // expected-error {{lambda cannot be both const and mutable}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test08() {
  int x = 42;
  int y = 666;
  auto z = [const &x, mutable y]() mutable const { // expected-error {{lambda cannot be both const and mutable}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test09() {
  auto z = []() const static { // expected-error {{lambda cannot be both const and static}}
  };
}

void test10() {
  auto z = []() static const { // expected-error {{lambda cannot be both const and static}}
  };
}

void test11() {
  int x = 42;
  int y = 666;
  auto z = [const x, const mutable y]() { // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;  // expected-error {{read-only variable is not assignable}}
  };
}

void test12() {
  int x = 42;
  int y = 666;
  auto z = [const x, mutable const y]() { // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

template <class T> void f(T t) {
  auto z = [const mutable t]() { // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
    t = 666; // expected-error {{cannot assign}}
  };
}

template <class T> void f2(T t) {
  auto z = [mutable const t]() mutable { // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
    t = 666;
  };
}

template <class T> void f3(T t) {
  auto z = [t]() const {
    t = 666; // expected-error {{cannot assign}}
  };
}

void test13() {
  f(42);
  f2(42);
  f3(42);
}

void test14() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable y] {
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test15() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable &y] { // expected-error {{a lambda capture by reference cannot be 'mutable'}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;  // expected-error {{cannot assign to variable 'y' with const-qualified type 'const int'}}
  };
}

void test16() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable y]() mutable {
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;
  };
}

void test17() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable &y]() mutable { // expected-error {{a lambda capture by reference cannot be 'mutable'}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    y = 42;  // expected-error {{cannot assign to variable 'y' with const-qualified type 'const int'}}
  };
}

void test18() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable z = y]() mutable {
    x = 666; // expected-error {{read-only variable is not assignable}}
    z = 42;
  };
}

void test19() {
  int x = 42;
  const int y = 666;
  auto z = [const x, mutable &z = y]() mutable { // expected-error {{a lambda capture by reference cannot be 'mutable'}}
    x = 666; // expected-error {{read-only variable is not assignable}}
    z = 42;  // expected-error {{cannot assign to variable 'z' with const-qualified type 'const int &'}}
  };
}

void test20() {
  int x = 42;
  const int y = 666;
  auto z = [const a = x, mutable z = y]() mutable {
    a = 666; // expected-error {{cannot assign to variable 'a' with const-qualified type 'const int'}}
    z = 42;
  };
}

void test21() {
  int x = 42;
  const int y = 666;
  auto z = [const &a = x, mutable &z = y]() mutable { // expected-error {{a lambda capture by reference cannot be 'mutable'}}
    a = 666; // expected-error {{cannot assign to variable 'a' with const-qualified type 'const int &'}}
    z = 42;  // expected-error {{cannot assign to variable 'z' with const-qualified type 'const int &'}}
  };
}
