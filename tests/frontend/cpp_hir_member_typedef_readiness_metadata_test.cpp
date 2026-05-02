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

void attach_alias(c4c::Node& owner,
                  const char* alias_name,
                  c4c::TypeSpec* alias_type) {
  static const char* alias_names[1];
  alias_names[0] = alias_name;
  owner.member_typedef_names = alias_names;
  owner.member_typedef_types = alias_type;
  owner.n_member_typedefs = 1;
}

void test_readiness_prefers_tag_text_id_owner_over_stale_tag() {
  c4c::TextTable texts;
  const c4c::TextId real_owner_text = texts.intern("RealOwner");

  c4c::TypeSpec real_alias = make_scalar_ts(c4c::TB_LONG);
  c4c::Node real_owner{};
  real_owner.kind = c4c::NK_STRUCT_DEF;
  real_owner.name = "RealOwner";
  real_owner.unqualified_name = "RealOwner";
  real_owner.unqualified_text_id = real_owner_text;
  real_owner.namespace_context_id = 5;
  attach_alias(real_owner, "Alias", &real_alias);

  c4c::TypeSpec stale_alias = make_scalar_ts(c4c::TB_INT);
  c4c::Node stale_owner{};
  stale_owner.kind = c4c::NK_STRUCT_DEF;
  stale_owner.name = "StaleOwner";
  stale_owner.unqualified_name = "StaleOwner";
  stale_owner.unqualified_text_id = texts.intern("StaleOwner");
  stale_owner.namespace_context_id = 5;
  attach_alias(stale_owner, "Alias", &stale_alias);

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_[real_owner.name] = &real_owner;
  lowerer.struct_def_nodes_[stale_owner.name] = &stale_owner;

  c4c::TypeSpec owner_ts = make_scalar_ts(c4c::TB_STRUCT);
  owner_ts.tag = "StaleOwner";
  owner_ts.tag_text_id = real_owner_text;
  owner_ts.namespace_context_id = 5;
  owner_ts.deferred_member_type_name = "Alias";

  const bool ok = lowerer.resolve_struct_member_typedef_if_ready(&owner_ts);
  expect_true(ok, "structured tag_text_id owner should make deferred lookup ready");
  expect_true(owner_ts.base == c4c::TB_LONG,
              "deferred member typedef should resolve through tag_text_id owner before stale rendered tag");
}

void test_readiness_blocks_stale_tag_when_tag_text_id_misses() {
  c4c::TextTable texts;

  c4c::TypeSpec stale_alias = make_scalar_ts(c4c::TB_INT);
  c4c::Node stale_owner{};
  stale_owner.kind = c4c::NK_STRUCT_DEF;
  stale_owner.name = "StaleOwner";
  stale_owner.unqualified_name = "StaleOwner";
  stale_owner.unqualified_text_id = texts.intern("StaleOwner");
  stale_owner.namespace_context_id = 8;
  attach_alias(stale_owner, "Alias", &stale_alias);

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_[stale_owner.name] = &stale_owner;

  c4c::TypeSpec owner_ts = make_scalar_ts(c4c::TB_STRUCT);
  owner_ts.tag = "StaleOwner";
  owner_ts.tag_text_id = texts.intern("MissingOwner");
  owner_ts.namespace_context_id = 8;
  owner_ts.deferred_member_type_name = "Alias";

  const bool ok = lowerer.resolve_struct_member_typedef_if_ready(&owner_ts);
  expect_true(!ok,
              "structured tag_text_id miss should not recover through stale rendered tag");
  expect_true(owner_ts.base == c4c::TB_STRUCT,
              "unresolved owner type should remain unchanged after rejecting stale fallback");
}

}  // namespace

int main() {
  test_readiness_prefers_tag_text_id_owner_over_stale_tag();
  test_readiness_blocks_stale_tag_when_tag_text_id_misses();
  std::cout << "PASS: cpp_hir_member_typedef_readiness_metadata_test\n";
  return 0;
}
