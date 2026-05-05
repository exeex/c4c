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
auto legacy_tag_available(int) -> decltype(static_cast<void>(&T::tag), bool()) {
  return true;
}

template <typename T>
bool legacy_tag_available(long) {
  return false;
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

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
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = param_text_id;
  ts.template_param_text_id = param_text_id;
  ts.template_param_owner_text_id = owner_text_id;
  ts.template_param_owner_namespace_context_id = owner_namespace_id;
  ts.template_param_index = param_index;
  return ts;
}

void attach_box_template(c4c::Node& primary,
                         c4c::TextId owner_text_id,
                         c4c::TextId t_text_id,
                         c4c::TextId u_text_id,
                         c4c::TypeSpec alias_type) {
  static const char* template_param_names[] = {"T", "U"};
  static bool template_param_is_nttp[] = {false, false};
  static c4c::TextId template_param_text_ids[2];
  static const char* member_typedef_names[] = {"Alias"};
  static c4c::TypeSpec member_typedef_types[1];

  template_param_text_ids[0] = t_text_id;
  template_param_text_ids[1] = u_text_id;
  member_typedef_types[0] = alias_type;

  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "Box";
  primary.unqualified_name = "Box";
  primary.unqualified_text_id = owner_text_id;
  primary.namespace_context_id = 17;
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.template_param_is_nttp = template_param_is_nttp;
  primary.n_template_params = 2;
  primary.member_typedef_names = member_typedef_names;
  primary.member_typedef_types = member_typedef_types;
  primary.n_member_typedefs = 1;
}

c4c::TypeSpec make_deferred_box_alias(c4c::TemplateArgRef* args) {
  args[0].kind = c4c::TemplateArgKind::Type;
  args[0].type = make_scalar_ts(c4c::TB_INT);
  args[1].kind = c4c::TemplateArgKind::Type;
  args[1].type = make_scalar_ts(c4c::TB_LONG);

  c4c::TypeSpec owner = make_scalar_ts(c4c::TB_STRUCT);
  owner.tpl_struct_origin = "Box";
  owner.tpl_struct_args.data = args;
  owner.tpl_struct_args.size = 2;
  owner.deferred_member_type_name = "Alias";
  return owner;
}

c4c::TypeSpec resolve_alias(c4c::Node* primary) {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.register_template_struct_primary("Box", primary);

  c4c::TemplateArgRef args[2]{};
  c4c::TypeSpec owner = make_deferred_box_alias(args);
  const bool ok = lowerer.resolve_struct_member_typedef_if_ready(&owner);
  expect_true(ok, "deferred member typedef should resolve from template origin");
  return owner;
}

void test_origin_binding_prefers_template_param_metadata_over_stale_tag() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::Node primary{};
  c4c::TypeSpec alias = make_param_ref("U", t_text, owner_text, 17, 0);
  attach_box_template(primary, owner_text, t_text, u_text, alias);

  c4c::TypeSpec resolved = resolve_alias(&primary);
  expect_true(resolved.base == c4c::TB_INT,
              "structured template parameter metadata should bind T despite stale rendered tag U");
}

void test_origin_binding_mismatch_blocks_stale_tag_fallback() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId other_owner_text = texts.intern("OtherBox");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::Node primary{};
  c4c::TypeSpec alias = make_param_ref("T", t_text, other_owner_text, 17, 0);
  attach_box_template(primary, owner_text, t_text, u_text, alias);

  c4c::TypeSpec resolved = resolve_alias(&primary);
  expect_true(resolved.base == c4c::TB_TYPEDEF,
              "structured owner mismatch must not recover through stale rendered tag T");
  expect_true(resolved.template_param_owner_text_id == other_owner_text,
              "unsubstituted structured carrier should remain visible after rejecting fallback");
}

void test_origin_binding_keeps_no_metadata_legacy_tag_fallback() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId t_text = texts.intern("T");
  const c4c::TextId u_text = texts.intern("U");

  c4c::Node primary{};
  c4c::TypeSpec alias = make_scalar_ts(c4c::TB_TYPEDEF);
  if (!legacy_tag_available<c4c::TypeSpec>(0)) return;
  set_legacy_tag_if_present(alias, "U", 0);
  attach_box_template(primary, owner_text, t_text, u_text, alias);

  c4c::TypeSpec resolved = resolve_alias(&primary);
  expect_true(resolved.base == c4c::TB_LONG,
              "tag-only no-metadata fallback should keep legacy origin binding behavior");
}

}  // namespace

int main() {
  test_origin_binding_prefers_template_param_metadata_over_stale_tag();
  test_origin_binding_mismatch_blocks_stale_tag_fallback();
  test_origin_binding_keeps_no_metadata_legacy_tag_fallback();
  std::cout << "PASS: cpp_hir_member_typedef_origin_binding_metadata_test\n";
  return 0;
}
