// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

extern "C" int mutable_copy(int value) {
  return [mutable value]() const { return ++value; }();
}

// CHECK-LABEL: define{{.*}} i32 @mutable_copy(

extern "C" int const_copy(int value) {
  return [const value]() mutable { return value; }();
}

// CHECK-LABEL: define{{.*}} i32 @const_copy(

extern "C" int const_reference(int &value) {
  return [const &value]() mutable { return value; }();
}

// CHECK-LABEL: define{{.*}} i32 @const_reference(

extern "C" int qualified_defaults(int first, int second) {
  return [mutable =, const second]() const {
    ++first;
    return first + second;
  }();
}

// CHECK-LABEL: define{{.*}} i32 @qualified_defaults(
