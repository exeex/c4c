#include "sema/consteval.hpp"
#include "sema/type_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

void expect_eq_int(int actual, int expected, const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::to_string(expected) +
         "\nActual: " + std::to_string(actual));
  }
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

c4c::Node sizeof_type_node(const c4c::TypeSpec& ts) {
  c4c::Node n{};
  n.kind = c4c::NK_SIZEOF_TYPE;
  n.type = ts;
  return n;
}

c4c::Node alignof_type_node(const c4c::TypeSpec& ts) {
  c4c::Node n{};
  n.kind = c4c::NK_ALIGNOF_TYPE;
  n.type = ts;
  return n;
}

c4c::Node var_ref(const char* rendered_name,
                  const char* unqualified_name,
                  c4c::TextId text_id,
                  int namespace_context_id) {
  c4c::Node n{};
  n.kind = c4c::NK_VAR;
  n.name = rendered_name;
  n.unqualified_name = unqualified_name;
  n.unqualified_text_id = text_id;
  n.namespace_context_id = namespace_context_id;
  return n;
}

c4c::TypeSpec record_ref(c4c::TextId text_id, const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.namespace_context_id = 3;
  ts.tag_text_id = text_id;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  return ts;
}

c4c::hir::HirStructDef struct_def(std::string tag,
                                  c4c::TextId text_id,
                                  int size_bytes,
                                  int align_bytes) {
  c4c::hir::HirStructDef def{};
  def.tag = std::move(tag);
  def.tag_text_id = text_id;
  def.ns_qual.context_id = 3;
  def.size_bytes = size_bytes;
  def.align_bytes = align_bytes;
  return def;
}

void test_consteval_sizeof_prefers_owner_metadata_over_stale_rendered_tag() {
  c4c::TextTable link_texts;
  const c4c::TextId real_text = link_texts.intern("RealRecord");
  const c4c::TextId stale_text = link_texts.intern("StaleRecord");

  std::unordered_map<std::string, c4c::hir::HirStructDef> struct_defs;
  struct_defs.emplace("RealRecord", struct_def("RealRecord", real_text, 24, 8));
  struct_defs.emplace("StaleRecord", struct_def("StaleRecord", stale_text, 4, 1));

  c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(struct_defs.at("RealRecord"));
  std::unordered_map<c4c::hir::HirRecordOwnerKey, std::string,
                     c4c::hir::HirRecordOwnerKeyHash>
      owner_index;
  owner_index.emplace(real_key, "RealRecord");

  c4c::hir::ConstEvalEnv env{};
  env.struct_defs = &struct_defs;
  env.struct_def_owner_index = &owner_index;
  env.link_name_texts = &link_texts;

  c4c::Node n = sizeof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&n, env);

  expect_true(result.ok(),
              "sizeof should resolve record layout through owner metadata");
  expect_eq_int(static_cast<int>(result.as_int()), 24,
                "stale rendered tag must not select the compatibility layout");

  c4c::Node align = alignof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult align_result =
      c4c::hir::evaluate_constant_expr(&align, env);

  expect_true(align_result.ok(),
              "alignof should resolve record layout through owner metadata");
  expect_eq_int(static_cast<int>(align_result.as_int()), 8,
                "stale rendered tag must not select compatibility alignment");
}

void test_consteval_sizeof_metadata_miss_rejects_stale_rendered_tag() {
  c4c::TextTable link_texts;
  const c4c::TextId real_text = link_texts.intern("RealRecord");
  const c4c::TextId stale_text = link_texts.intern("StaleRecord");

  std::unordered_map<std::string, c4c::hir::HirStructDef> struct_defs;
  struct_defs.emplace("StaleRecord", struct_def("StaleRecord", stale_text, 8, 2));
  std::unordered_map<c4c::hir::HirRecordOwnerKey, std::string,
                     c4c::hir::HirRecordOwnerKeyHash>
      owner_index;
  owner_index.emplace(c4c::hir::make_hir_record_owner_key(
                          struct_defs.at("StaleRecord")),
                      "StaleRecord");

  c4c::hir::ConstEvalEnv env{};
  env.struct_defs = &struct_defs;
  env.struct_def_owner_index = &owner_index;
  env.link_name_texts = &link_texts;

  c4c::Node n = sizeof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&n, env);

  expect_true(!result.ok(),
              "structured owner metadata miss should not recover through stale rendered tag");

  c4c::Node align = alignof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult align_result =
      c4c::hir::evaluate_constant_expr(&align, env);
  expect_true(!align_result.ok(),
              "alignof owner metadata miss should not recover through stale rendered tag");
}

void test_type_binding_equivalence_rejects_rendered_name_when_metadata_exists() {
  c4c::TypeSpec lhs = record_ref(51, "RenderedSame");
  c4c::TypeSpec rhs = record_ref(52, "RenderedSame");

  expect_true(!c4c::type_binding_values_equivalent(lhs, rhs),
              "text-name metadata should block rendered-name compatibility");
}

void test_type_binding_equivalence_keeps_no_metadata_rendered_compatibility() {
  c4c::TypeSpec lhs{};
  lhs.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(lhs, "RenderedOnly", 0);

  c4c::TypeSpec rhs{};
  rhs.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(rhs, "RenderedOnly", 0);

  expect_true(c4c::type_binding_values_equivalent(lhs, rhs),
              "rendered tags remain a no-metadata compatibility fallback");
}

void test_static_eval_int_prefers_structured_enum_metadata() {
  constexpr int kNamespaceContext = 7;
  c4c::TextTable texts;
  const c4c::TextId real_text = texts.intern("RealEnumValue");

  c4c::hir::ConstEvalStructuredNameKey key;
  key.namespace_context_id = kNamespaceContext;
  key.base_text_id = real_text;

  c4c::hir::ConstStructuredMap structured_enums;
  structured_enums.emplace(key, 42);
  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("stale_enum_value", 99);

  c4c::StaticEvalIntEnumLookupInput lookup;
  lookup.rendered_enum_consts = &rendered_enums;
  lookup.enum_consts_by_key = &structured_enums;

  c4c::Node ref =
      var_ref("stale_enum_value", "RealEnumValue", real_text, kNamespaceContext);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&ref, lookup)), 42,
                "static_eval_int should prefer structured enum metadata over rendered names");
}

void test_static_eval_int_structured_enum_miss_rejects_rendered_bridge() {
  constexpr int kNamespaceContext = 7;
  c4c::TextTable texts;
  const c4c::TextId real_text = texts.intern("RealEnumValue");
  const c4c::TextId other_text = texts.intern("OtherEnumValue");

  c4c::hir::ConstEvalStructuredNameKey key;
  key.namespace_context_id = kNamespaceContext;
  key.base_text_id = other_text;

  c4c::hir::ConstStructuredMap structured_enums;
  structured_enums.emplace(key, 13);
  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("stale_enum_value", 99);

  c4c::StaticEvalIntEnumLookupInput lookup;
  lookup.rendered_enum_consts = &rendered_enums;
  lookup.enum_consts_by_key = &structured_enums;

  c4c::Node ref =
      var_ref("stale_enum_value", "RealEnumValue", real_text, kNamespaceContext);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&ref, lookup)), 0,
                "structured enum metadata miss should not recover through rendered lookup");
}

void test_static_eval_int_uses_text_id_enum_metadata_before_rendered_bridge() {
  c4c::TextTable texts;
  const c4c::TextId real_text = texts.intern("RealEnumValue");
  const c4c::TextId missing_text = texts.intern("MissingEnumValue");

  c4c::hir::ConstTextMap text_enums;
  text_enums.emplace(real_text, 31);
  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("stale_enum_value", 99);

  c4c::StaticEvalIntEnumLookupInput lookup;
  lookup.rendered_enum_consts = &rendered_enums;
  lookup.enum_consts_by_text = &text_enums;

  c4c::Node ref = var_ref("stale_enum_value", "RealEnumValue", real_text, -1);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&ref, lookup)), 31,
                "static_eval_int should use TextId enum metadata before rendered names");

  c4c::Node miss =
      var_ref("stale_enum_value", "MissingEnumValue", missing_text, -1);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&miss, lookup)), 0,
                "TextId enum metadata miss should not recover through rendered lookup");
}

}  // namespace

int main() {
  test_consteval_sizeof_prefers_owner_metadata_over_stale_rendered_tag();
  test_consteval_sizeof_metadata_miss_rejects_stale_rendered_tag();
  test_type_binding_equivalence_rejects_rendered_name_when_metadata_exists();
  test_type_binding_equivalence_keeps_no_metadata_rendered_compatibility();
  test_static_eval_int_prefers_structured_enum_metadata();
  test_static_eval_int_structured_enum_miss_rejects_rendered_bridge();
  test_static_eval_int_uses_text_id_enum_metadata_before_rendered_bridge();
  std::cout << "PASS: cpp_hir_sema_consteval_type_utils_metadata_test\n";
  return 0;
}
