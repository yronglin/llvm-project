// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify -verify-ignore-unexpected=note %s

void redundant_defaults(int x) {
  (void)[=, x] {};                 // expected-error {{'&' must precede a capture when the capture default is '='}}
  (void)[mutable =, mutable x] {}; // expected-error {{explicit capture of 'x' is redundant with the capture default}}
  (void)[const =, const x] {};     // expected-error {{explicit capture of 'x' is redundant with the capture default}}
  (void)[&, &x] {};                // expected-error {{'&' cannot precede a capture when the capture default is '&'}}
  (void)[const &, const &x] {};    // expected-error {{explicit capture of 'x' is redundant with the capture default}}
}

void const_mutation(int x) {
  [const x]() mutable { x = 1; }(); // expected-error {{read-only variable is not assignable}}
  [const &x]() mutable { x = 1; }(); // expected-error {{read-only variable is not assignable}}
  [const =]() mutable { x = 1; }(); // expected-error {{read-only variable is not assignable}}
  [const &]() mutable { x = 1; }(); // expected-error {{read-only variable is not assignable}}
}

void nested_const_reference(int x) {
  [const &x] {
    [x]() mutable {
      x = 1; // expected-error {{read-only variable is not assignable}}
    }();
    [&x] {
      [x]() mutable {
        x = 2; // expected-error {{read-only variable is not assignable}}
      }();
    }();
  }();
}

void nested_const_reference_default(int x) {
  [const &] {
    [&] {
      [x]() mutable {
        x = 3; // expected-error {{read-only variable is not assignable}}
      }();
    }();
  }();
}

void invalid_qualifiers(int x) {
  (void)[mutable &x] {};     // expected-error {{a lambda capture by reference cannot be 'mutable'}}
  (void)[mutable &y = x] {}; // expected-error {{a lambda capture by reference cannot be 'mutable'}}
  (void)[const mutable x] {}; // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
  (void)[mutable const x] {}; // expected-error {{lambda capture cannot be qualified by both 'const' and 'mutable'}}
}

struct QualifiedThis {
  int member;

  void captures() {
    (void)[const this] {};    // expected-error {{capture of 'this' cannot be qualified by const}}
    (void)[mutable this] {};  // expected-error {{capture of 'this' cannot be qualified by mutable}}
    (void)[const *this] {};   // expected-error {{capture of '*this' cannot be qualified by const}}
    (void)[mutable *this] {}; // expected-error {{capture of '*this' cannot be qualified by mutable}}

    (void)[const =] { return member; };   // expected-error {{'this' cannot be implicitly captured in this context}}
    (void)[mutable =] { return member; }; // expected-error {{'this' cannot be implicitly captured in this context}}
    (void)[const &] { return member; };   // expected-error {{'this' cannot be implicitly captured in this context}}
  }
};

void invalid_lambda_specifiers() {
  (void)[]() const mutable {}; // expected-error {{lambda cannot be both const and mutable}}
  (void)[]() mutable const {}; // expected-error {{lambda cannot be both const and mutable}}
  (void)[]() const static {};  // expected-error {{lambda cannot be both const and static}}
  (void)[]() static const {};  // expected-error {{lambda cannot be both const and static}}
  (void)[]() const const {};   // expected-error {{'const' cannot appear multiple times in a lambda declarator}}
  (void)[](this auto) const {}; // expected-error {{a lambda with an explicit object parameter cannot be const}}
}
