// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s

// Check the explicit const lambda-specifier and its constraints.

void explicit_const_specifier() {
  auto with_parameters = []() const { return 1; };
  auto without_parameters = [] const { return 2; };

  using A = decltype(with_parameters);
  using B = decltype(without_parameters);
  static_assert(__is_same(decltype(&A::operator()), int (A::*)() const));
  static_assert(__is_same(decltype(&B::operator()), int (B::*)() const));
}

void mutually_exclusive_specifiers() {
  (void)[]() const mutable {}; // expected-error {{lambda cannot be both const and mutable}}
  (void)[]() mutable const {}; // expected-error {{lambda cannot be both const and mutable}}
  (void)[]() const static {}; // expected-error {{lambda cannot be both const and static}}
  (void)[]() static const {}; // expected-error {{lambda cannot be both const and static}}
  (void)[]() const const {}; // expected-error {{'const' cannot appear multiple times in a lambda declarator}}
}

void explicit_object_parameter() {
  (void)[](this auto) const {}; // expected-error {{a lambda with an explicit object parameter cannot be const}}
}

constexpr int constexpr_mutable_capture() {
  return [mutable value = 0]() constexpr { return ++value; }();
}

consteval int consteval_mutable_capture() {
  return [mutable value = 1]() const { return ++value; }();
}

static_assert(constexpr_mutable_capture() == 1);
static_assert(consteval_mutable_capture() == 2);
