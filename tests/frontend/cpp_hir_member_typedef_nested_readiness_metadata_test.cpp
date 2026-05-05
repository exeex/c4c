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

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, bool()) {
  ts.tag = tag;
  return true;
}

bool set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {
  return false;
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

void attach_box_template(c4c::Node& primary,
                         c4c::TextId owner_text_id,
                         c4c::TextId param_text_id,
                         c4c::TypeSpec alias_type) {
  static const char* template_param_names[] = {"T"};
  static bool template_param_is_nttp[] = {false};
  static c4c::TextId template_param_text_ids[1];
  static const char* member_typedef_names[] = {"Alias"};
  static c4c::TypeSpec member_typedef_types[1];

  template_param_text_ids[0] = param_text_id;
  member_typedef_types[0] = alias_type;

  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "Box";
  primary.unqualified_name = "Box";
  primary.unqualified_text_id = owner_text_id;
  primary.namespace_context_id = 11;
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.template_param_is_nttp = template_param_is_nttp;
  primary.n_template_params = 1;
  primary.member_typedef_names = member_typedef_names;
  primary.member_typedef_types = member_typedef_types;
  primary.n_member_typedefs = 1;
}

c4c::TypeSpec resolve_box_alias(c4c::hir::Lowerer& lowerer, c4c::Node* primary) {
  lowerer.register_template_struct_primary("Box", primary);

  c4c::TemplateArgRef args[1]{};
  args[0].kind = c4c::TemplateArgKind::Type;
  args[0].type = make_scalar_ts(c4c::TB_INT);

  c4c::TypeSpec owner = make_scalar_ts(c4c::TB_STRUCT);
  owner.tpl_struct_origin = "Box";
  owner.tpl_struct_args.data = args;
  owner.tpl_struct_args.size = 1;
  owner.deferred_member_type_name = "Alias";

  const bool ok = lowerer.resolve_struct_member_typedef_if_ready(&owner);
  expect_true(ok, "deferred member typedef should resolve from template origin");
  return owner;
}

void test_nested_readiness_uses_record_def_without_rendered_tag() {
  c4c::TextTable texts;
  const c4c::TextId box_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId nested_text = texts.intern("NestedOwner");
  const c4c::TextId inner_text = texts.intern("Inner");

  c4c::TypeSpec nested_alias = make_scalar_ts(c4c::TB_LONG);
  c4c::Node nested_owner{};
  nested_owner.kind = c4c::NK_STRUCT_DEF;
  nested_owner.name = "NestedOwner";
  nested_owner.unqualified_name = "NestedOwner";
  nested_owner.unqualified_text_id = nested_text;
  nested_owner.namespace_context_id = 23;
  attach_alias(nested_owner, "Inner", &nested_alias);

  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_STRUCT);
  alias.record_def = &nested_owner;
  alias.tag_text_id = nested_text;
  alias.namespace_context_id = 23;
  alias.deferred_member_type_name = "Inner";
  alias.deferred_member_type_text_id = inner_text;

  c4c::Node primary{};
  attach_box_template(primary, box_text, t_text, alias);

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec resolved = resolve_box_alias(lowerer, &primary);
  expect_true(resolved.base == c4c::TB_LONG,
              "nested deferred member should resolve from record_def without rendered tag");
}

void test_nested_readiness_keeps_no_metadata_legacy_tag_fallback() {
  c4c::TextTable texts;
  const c4c::TextId box_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");

  c4c::TypeSpec nested_alias = make_scalar_ts(c4c::TB_INT);
  c4c::Node nested_owner{};
  nested_owner.kind = c4c::NK_STRUCT_DEF;
  nested_owner.name = "LegacyNested";
  nested_owner.unqualified_name = "LegacyNested";
  attach_alias(nested_owner, "Inner", &nested_alias);

  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_STRUCT);
  if (!set_legacy_tag_if_present(alias, "LegacyNested", 0)) return;
  alias.deferred_member_type_name = "Inner";

  c4c::Node primary{};
  attach_box_template(primary, box_text, t_text, alias);

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["LegacyNested"] = &nested_owner;

  c4c::TypeSpec resolved = resolve_box_alias(lowerer, &primary);
  expect_true(resolved.base == c4c::TB_INT,
              "tag-only nested deferred member should keep legacy compatibility");
}

void test_nested_readiness_blocks_stale_tag_when_structured_owner_misses() {
  c4c::TextTable texts;
  const c4c::TextId box_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId missing_owner_text = texts.intern("MissingNested");

  c4c::TypeSpec stale_alias = make_scalar_ts(c4c::TB_LONG);
  c4c::Node stale_owner{};
  stale_owner.kind = c4c::NK_STRUCT_DEF;
  stale_owner.name = "StaleNested";
  stale_owner.unqualified_name = "StaleNested";
  stale_owner.unqualified_text_id = texts.intern("StaleNested");
  stale_owner.namespace_context_id = 31;
  attach_alias(stale_owner, "Inner", &stale_alias);

  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_STRUCT);
  set_legacy_tag_if_present(alias, "StaleNested", 0);
  alias.tag_text_id = missing_owner_text;
  alias.namespace_context_id = 31;
  alias.deferred_member_type_name = "Inner";

  c4c::Node primary{};
  attach_box_template(primary, box_text, t_text, alias);

  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["StaleNested"] = &stale_owner;

  c4c::TypeSpec resolved = resolve_box_alias(lowerer, &primary);
  expect_true(resolved.base == c4c::TB_STRUCT,
              "structured owner miss must not recover through stale rendered tag");
  expect_true(resolved.deferred_member_type_name != nullptr,
              "unresolved nested deferred member should remain visible");
}

}  // namespace

int main() {
  test_nested_readiness_uses_record_def_without_rendered_tag();
  test_nested_readiness_keeps_no_metadata_legacy_tag_fallback();
  test_nested_readiness_blocks_stale_tag_when_structured_owner_misses();
  std::cout << "PASS: cpp_hir_member_typedef_nested_readiness_metadata_test\n";
  return 0;
}
