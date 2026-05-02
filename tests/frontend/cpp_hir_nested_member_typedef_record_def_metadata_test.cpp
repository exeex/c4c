#include "hir/hir_ir.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#define private public
#include "hir/impl/lowerer.hpp"
#undef private

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::TypeSpec make_scalar_ts(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_nested_deferred_member_typedef_uses_record_def_owner() {
  c4c::TypeSpec nested_type = make_scalar_ts(c4c::TB_LONG);
  static const char* nested_typedef_names[] = {"type"};
  static c4c::TypeSpec nested_typedef_types[1];
  nested_typedef_types[0] = nested_type;

  c4c::Node nested_owner{};
  nested_owner.kind = c4c::NK_STRUCT_DEF;
  nested_owner.name = "Nested";
  nested_owner.unqualified_name = "Nested";
  nested_owner.member_typedef_names = nested_typedef_names;
  nested_owner.member_typedef_types = nested_typedef_types;
  nested_owner.n_member_typedefs = 1;

  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_STRUCT);
  alias.tag = "StaleRenderedOwner";
  alias.record_def = &nested_owner;
  alias.deferred_member_type_name = "type";

  static const char* owner_typedef_names[] = {"Alias"};
  static c4c::TypeSpec owner_typedef_types[1];
  owner_typedef_types[0] = alias;

  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = "Outer";
  owner.unqualified_name = "Outer";
  owner.member_typedef_names = owner_typedef_names;
  owner.member_typedef_types = owner_typedef_types;
  owner.n_member_typedefs = 1;

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_[owner.name] = &owner;
  lowerer.struct_def_nodes_[nested_owner.name] = &nested_owner;

  c4c::TypeSpec resolved{};
  const bool ok =
      lowerer.resolve_struct_member_typedef_type(owner.name, "Alias", &resolved);
  expect_true(ok, "outer member typedef should resolve");
  expect_true(resolved.base == c4c::TB_LONG,
              "nested deferred member typedef should prefer record_def owner over stale rendered tag");
}

}  // namespace

int main() {
  test_nested_deferred_member_typedef_uses_record_def_owner();
  std::cout << "PASS: cpp_hir_nested_member_typedef_record_def_metadata_test\n";
  return 0;
}
