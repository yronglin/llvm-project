// RUN: %clang_cc1 -ast-print -std=c++2c -Wno-c++29-extensions %s | FileCheck %s
// RUN: %clang_cc1 -ast-dump -ast-dump-filter dump_fields -std=c++2c -Wno-c++29-extensions %s | FileCheck --check-prefix=DUMP %s

void print_captures(int x, int y, int z) {
  (void)[mutable x, const y, const &z, mutable i = x, const j = y,
         const &k = z]() const {};
  // CHECK: (void)[mutable x, const y, const &z, mutable i = x, const j = y, const &k = z]() const {

  (void)[mutable =, const x] {};
  // CHECK: (void)[mutable =, const x] {

  (void)[const =, mutable y] {};
  // CHECK: (void)[const =, mutable y] {

  (void)[const &, &z] {};
  // CHECK: (void)[const&, &z] {
}

void dump_fields(const int source, volatile int volatile_source) {
  (void)[mutable source, const volatile_source] {};
}

// DUMP: FunctionDecl {{.*}} dump_fields
// DUMP: LambdaExpr
// DUMP: DefinitionData {{.*}} has_mutable_fields
// DUMP: FieldDecl {{.*}} 'int' mutable
// DUMP: FieldDecl {{.*}} 'const int'
