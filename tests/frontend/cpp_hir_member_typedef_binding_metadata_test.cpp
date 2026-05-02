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

c4c::TypeSpec make_param_ref(const char* rendered_tag,
                             c4c::TextId param_text_id,
                             c4c::TextId owner_text_id,
                             int owner_namespace_id,
                             int param_index) {
  c4c::TypeSpec ts = make_scalar_ts(c4c::TB_TYPEDEF);
  ts.tag = rendered_tag;
  ts.template_param_text_id = param_text_id;
  ts.template_param_owner_text_id = owner_text_id;
  ts.template_param_owner_namespace_context_id = owner_namespace_id;
  ts.template_param_index = param_index;
  return ts;
}

c4c::Node make_instantiated_owner(c4c::TextId owner_text_id,
                                  c4c::TextId t_text_id,
                                  c4c::TextId u_text_id,
                                  c4c::TypeSpec alias_type) {
  static const char* template_param_names[] = {"T", "U"};
  static bool template_param_is_nttp[] = {false, false};
  static c4c::TypeSpec template_arg_types[2];
  static const char* member_typedef_names[] = {"Alias"};
  static c4c::TypeSpec member_typedef_types[1];
  static c4c::TextId template_param_text_ids[2];

  template_arg_types[0] = make_scalar_ts(c4c::TB_INT);
  template_arg_types[1] = make_scalar_ts(c4c::TB_LONG);
  member_typedef_types[0] = alias_type;
  template_param_text_ids[0] = t_text_id;
  template_param_text_ids[1] = u_text_id;

  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = "Box<int,long>";
  owner.unqualified_name = "Box";
  owner.unqualified_text_id = owner_text_id;
  owner.namespace_context_id = 11;
  owner.template_param_names = template_param_names;
  owner.template_param_name_text_ids = template_param_text_ids;
  owner.template_param_is_nttp = template_param_is_nttp;
  owner.n_template_params = 2;
  owner.template_arg_types = template_arg_types;
  owner.n_template_args = 2;
  owner.member_typedef_names = member_typedef_names;
  owner.member_typedef_types = member_typedef_types;
  owner.n_member_typedefs = 1;
  return owner;
}

c4c::TypeSpec resolve_alias(c4c::Node* owner) {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_[owner->name] = owner;

  c4c::TypeSpec resolved{};
  const bool ok =
      lowerer.resolve_struct_member_typedef_type(owner->name, "Alias", &resolved);
  expect_true(ok, "member typedef should be found on the synthetic owner");
  return resolved;
}

void test_structured_param_text_id_beats_stale_tag() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::TypeSpec alias =
      make_param_ref("U", t_text, owner_text, 11, 0);
  c4c::Node owner = make_instantiated_owner(owner_text, t_text, u_text, alias);
  c4c::TypeSpec resolved = resolve_alias(&owner);

  expect_true(resolved.base == c4c::TB_INT,
              "structured template_param_text_id/index must bind T despite stale rendered tag U");
}

void test_structured_mismatch_blocks_stale_tag_fallback() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId other_owner_text = texts.intern("OtherBox");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::TypeSpec alias =
      make_param_ref("T", t_text, other_owner_text, 11, 0);
  c4c::Node owner = make_instantiated_owner(owner_text, t_text, u_text, alias);
  c4c::TypeSpec resolved = resolve_alias(&owner);

  expect_true(resolved.base == c4c::TB_TYPEDEF,
              "mismatched structured owner must not recover through stale tag T");
  expect_true(resolved.template_param_owner_text_id == other_owner_text,
              "unsubstituted structured carrier should remain visible after rejecting fallback");
}

void test_no_metadata_tag_fallback_still_binds() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_TYPEDEF);
  alias.tag = "U";
  c4c::Node owner = make_instantiated_owner(owner_text, t_text, u_text, alias);
  c4c::TypeSpec resolved = resolve_alias(&owner);

  expect_true(resolved.base == c4c::TB_LONG,
              "tag-only no-metadata fallback should keep legacy binding behavior");
}

}  // namespace

int main() {
  test_structured_param_text_id_beats_stale_tag();
  test_structured_mismatch_blocks_stale_tag_fallback();
  test_no_metadata_tag_fallback_still_binds();
  std::cout << "PASS: cpp_hir_member_typedef_binding_metadata_test\n";
  return 0;
}
