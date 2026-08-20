// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify -verify-ignore-unexpected=note %s

// A const-reference view remains const through intervening reference
// recaptures and affects the member type of a nested copy capture.

void copy_of_const_reference_view(int x) {
  [const &x] {
    [x]() mutable {
      static_assert(__is_same(decltype((x)), const int &));
      x = 1; // expected-error {{read-only variable is not assignable}}
    }();
  }();
}

void const_view_through_reference_recaptures(int x) {
  [const &x] {
    [&x] {
      [&x] {
        [x]() mutable {
          static_assert(__is_same(decltype((x)), const int &));
          x = 2; // expected-error {{read-only variable is not assignable}}
        }();
      }();
    }();
  }();
}

void const_default_through_reference_recaptures(int x) {
  [const &] {
    [&] {
      [x]() mutable {
        static_assert(__is_same(decltype((x)), const int &));
        x = 3; // expected-error {{read-only variable is not assignable}}
      }();
    }();
  }();
}

void mutable_override_of_const_view(int x) {
  [const &x] {
    [mutable x] {
      static_assert(__is_same(decltype((x)), int &));
      x = 4;
    }();
  }();
}
