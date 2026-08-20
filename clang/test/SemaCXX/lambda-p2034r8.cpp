// RUN: %clang_cc1 -std=c++2c -Wno-c++29-extensions -verify %s
// expected-no-diagnostics

void explicit_captures() {
  const int c = 1;
  volatile int v = 2;

  [mutable c] {
    static_assert(__is_same(decltype((c)), int &));
    c = 3;
  }();
  [const c]() mutable {
    static_assert(__is_same(decltype((c)), const int &));
  }();
  [mutable v] {
    static_assert(__is_same(decltype((v)), int &));
    v = 3;
  }();
  [const v]() mutable {
    static_assert(__is_same(decltype((v)), const int &));
  }();
  [const &c] {
    static_assert(__is_same(decltype((c)), const int &));
  }();

  const auto mutable_member = [mutable n = 0]() const { return ++n; };
  (void)mutable_member();

  auto const_init = [const n = c]() mutable {
    static_assert(__is_same(decltype(n), const int));
  };
  auto mutable_init = [mutable n = c] {
    static_assert(__is_same(decltype(n), int));
    n = 4;
  };
  auto const_ref_init = [const &n = c] {
    static_assert(__is_same(decltype(n), const int &));
  };
  const_init();
  mutable_init();
  const_ref_init();
}

void capture_defaults() {
  int x = 0;
  [mutable =] { x = 1; }();
  [const =] { (void)x; }();
  [const &] { (void)x; }();

  const int const_source = 1;
  volatile int volatile_source = 2;
  [mutable =] {
    static_assert(__is_same(decltype((const_source)), int &));
    static_assert(__is_same(decltype((volatile_source)), int &));
    (void)++const_source;
    (void)++volatile_source;
  }();
  [const =]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
  }();
  [const &]() mutable {
    static_assert(__is_same(decltype((x)), const int &));
  }();

  // All non-redundant pairs of a default and an explicit capture compose.
  (void)[=, mutable x] {};
  (void)[=, const x] {};
  (void)[=, &x] {};
  (void)[=, const &x] {};

  (void)[mutable =, x] {};
  (void)[mutable =, const x] {};
  (void)[mutable =, &x] {};
  (void)[mutable =, const &x] {};

  (void)[const =, x] {};
  (void)[const =, mutable x] {};
  (void)[const =, &x] {};
  (void)[const =, const &x] {};

  (void)[&, x] {};
  (void)[&, mutable x] {};
  (void)[&, const x] {};
  (void)[&, const &x] {};

  (void)[const &, x] {};
  (void)[const &, mutable x] {};
  (void)[const &, const x] {};
  (void)[const &, &x] {};

  [const =, mutable x] { x = 2; }();
  [mutable =, const x]() mutable { (void)x; }();
  [const &, x]() mutable { x = 3; }();
  [const =, &x] { x = 4; }();

  // An init-capture is not redundant with a capture-default.
  (void)[mutable =, mutable copy = x] { ++copy; };
  (void)[const =, const copy = x]() mutable { (void)copy; };
  (void)[const &, const &reference = x] { (void)reference; };
}

struct ThisCapture {
  int member;

  void explicit_this() {
    [mutable =, this] { member = 1; }();
    [const =, this] { member = 2; }();
    [const &, this] { member = 3; }();
    [mutable =, *this]() mutable { member = 4; }();
    [const =, *this] { (void)member; }();
    [const &, *this] { (void)member; }();
  }
};

void nested_recaptures(int x) {
  [const &x] {
    [&x] {
      [x] {
        static_assert(__is_same(decltype((x)), const int &));
      }();
    }();
    [mutable x] { x = 1; }();
  }();

  [&x] {
    [x]() mutable { x = 2; }();
  }();
}

struct MoveOnly {
  MoveOnly();
  MoveOnly(MoveOnly &&);
  MoveOnly(const MoveOnly &) = delete;
};

void physical_member_qualifiers() {
  auto const_owned = [const value = MoveOnly()] {};
  auto mutable_owned = [mutable value = MoveOnly()] {};
  static_assert(
      !__is_constructible(decltype(const_owned), decltype(const_owned) &&));
  static_assert(__is_constructible(decltype(mutable_owned),
                                   decltype(mutable_owned) &&));
}

void target_function() {}

void function_reference_capture() {
  void (&f)() = target_function;
  [mutable f] { f(); }();
  [const f] { f(); }();
  [mutable =] { f(); }();
  [const =] { f(); }();
}

void array_capture() {
  const int values[] = {1, 2};
  [mutable values] {
    static_assert(__is_same(decltype((values)), int (&)[2]));
    values[0] = 3;
  }();
  [const values]() mutable {
    static_assert(__is_same(decltype((values)), const int (&)[2]));
  }();
}

template <class... T> void pack_captures(T... values) {
  [mutable values...] { ((values = 0), ...); }();
  [const values...] { ((void)values, ...); }();
  [const &values...] { ((void)values, ...); }();
  [mutable ...copies = values] { ((copies = 0), ...); }();
  [const ...copies = values] { ((void)copies, ...); }();
  [const &...refs = values] { ((void)refs, ...); }();
}

template void pack_captures<int, long>(int, long);

constexpr int constexpr_mutable_capture() {
  return [mutable n = 0]() constexpr { return ++n; }();
}

consteval int consteval_mutable_capture() {
  return [mutable n = 1]() const { return ++n; }();
}

static_assert(constexpr_mutable_capture() == 1);
static_assert(consteval_mutable_capture() == 2);

void explicit_const_specifier() {
  auto a = []() const { return 1; };
  auto b = [] const { return 2; };
  using A = decltype(a);
  using B = decltype(b);
  static_assert(__is_same(decltype(&A::operator()), int (A::*)() const));
  static_assert(__is_same(decltype(&B::operator()), int (B::*)() const));
}
