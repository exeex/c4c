#include "sema/consteval.hpp"
#include "sema/type_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

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

  c4c::StaticEvalIntEnumLookupInput lookup =
      c4c::StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility(
          rendered_enums);
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

  c4c::StaticEvalIntEnumLookupInput lookup =
      c4c::StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility(
          rendered_enums);
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

  c4c::StaticEvalIntEnumLookupInput lookup =
      c4c::StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility(
          rendered_enums);
  lookup.enum_consts_by_text = &text_enums;

  c4c::Node ref = var_ref("stale_enum_value", "RealEnumValue", real_text, -1);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&ref, lookup)), 31,
                "static_eval_int should use TextId enum metadata before rendered names");

  c4c::Node miss =
      var_ref("stale_enum_value", "MissingEnumValue", missing_text, -1);
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&miss, lookup)), 0,
                "TextId enum metadata miss should not recover through rendered lookup");
}

void test_static_eval_int_keeps_same_spelled_enum_domains_distinct() {
  constexpr int kNamespaceContext = 11;
  c4c::TextTable texts;
  const c4c::TextId value_text = texts.intern("Value");
  const c4c::TextId first_domain_text = texts.intern("FirstEnum");
  const c4c::TextId second_domain_text = texts.intern("SecondEnum");
  const c4c::TextId missing_domain_text = texts.intern("MissingEnum");

  c4c::hir::ConstEvalStructuredNameKey first_key;
  first_key.namespace_context_id = kNamespaceContext;
  first_key.qualifier_text_ids.push_back(first_domain_text);
  first_key.base_text_id = value_text;

  c4c::hir::ConstEvalStructuredNameKey second_key;
  second_key.namespace_context_id = kNamespaceContext;
  second_key.qualifier_text_ids.push_back(second_domain_text);
  second_key.base_text_id = value_text;

  c4c::hir::ConstStructuredMap structured_enums;
  structured_enums.emplace(first_key, 101);
  structured_enums.emplace(second_key, 202);
  c4c::hir::ConstTextMap text_enums;
  text_enums.emplace(value_text, 303);
  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("Value", 404);

  c4c::StaticEvalIntEnumLookupInput lookup =
      c4c::StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility(
          rendered_enums);
  lookup.enum_consts_by_text = &text_enums;
  lookup.enum_consts_by_key = &structured_enums;

  c4c::TextId first_qualifier[] = {first_domain_text};
  c4c::Node first_ref = var_ref("Value", "Value", value_text, kNamespaceContext);
  first_ref.n_qualifier_segments = 1;
  first_ref.qualifier_text_ids = first_qualifier;

  c4c::TextId second_qualifier[] = {second_domain_text};
  c4c::Node second_ref = var_ref("Value", "Value", value_text, kNamespaceContext);
  second_ref.n_qualifier_segments = 1;
  second_ref.qualifier_text_ids = second_qualifier;

  c4c::TextId missing_qualifier[] = {missing_domain_text};
  c4c::Node missing_ref = var_ref("Value", "Value", value_text, kNamespaceContext);
  missing_ref.n_qualifier_segments = 1;
  missing_ref.qualifier_text_ids = missing_qualifier;

  expect_eq_int(static_cast<int>(c4c::static_eval_int(&first_ref, lookup)), 101,
                "first structured enum domain should select its own value");
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&second_ref, lookup)), 202,
                "second structured enum domain should select its own value");
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&missing_ref, lookup)), 0,
                "structured enum miss must not recover through shared TextId or rendered spelling");
}

void test_static_eval_int_named_rendered_enum_compatibility_requires_opt_in() {
  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("RenderedOnlyEnumValue", 77);

  c4c::Node ref =
      var_ref("RenderedOnlyEnumValue", "RenderedOnlyEnumValue",
              c4c::kInvalidText, -1);
  c4c::StaticEvalIntEnumLookupInput ordinary_lookup;
  expect_eq_int(static_cast<int>(c4c::static_eval_int(&ref, ordinary_lookup)), 0,
                "ordinary static_eval_int should not infer rendered enum compatibility");

  expect_eq_int(
      static_cast<int>(
          c4c::static_eval_int_with_rendered_enum_compatibility(
              &ref, rendered_enums)),
      77,
      "rendered enum lookup should remain only through the named compatibility API");
}

void test_consteval_env_scoped_enum_metadata_beats_stale_rendered_mirror() {
  c4c::TextTable texts;
  const c4c::TextId same_text = texts.intern("Same");
  const c4c::TextId other_text = texts.intern("Other");

  std::unordered_map<std::string, long long> rendered_enums;
  rendered_enums.emplace("Same", 404);

  std::vector<c4c::hir::ConstTextMap> enum_scopes_by_text;
  enum_scopes_by_text.emplace_back();
  enum_scopes_by_text.back().emplace(same_text, 17);
  enum_scopes_by_text.emplace_back();
  enum_scopes_by_text.back().emplace(same_text, 29);

  c4c::hir::ConstEvalStructuredNameKey same_key;
  same_key.base_text_id = same_text;
  std::vector<c4c::hir::ConstStructuredMap> enum_scopes_by_key;
  enum_scopes_by_key.emplace_back();
  enum_scopes_by_key.back().emplace(same_key, 17);
  enum_scopes_by_key.emplace_back();
  enum_scopes_by_key.back().emplace(same_key, 29);

  c4c::hir::ConstEvalEnv env{};
  env.enum_consts = &rendered_enums;
  env.enum_scopes_by_text = &enum_scopes_by_text;
  env.enum_scopes_by_key = &enum_scopes_by_key;
  env.link_name_texts = &texts;

  c4c::Node inner_ref = var_ref("Same", "Same", same_text, -1);
  const std::optional<long long> inner_value = env.lookup(&inner_ref);
  expect_true(inner_value.has_value(),
              "scoped enum metadata lookup should find the innermost value");
  expect_eq_int(static_cast<int>(*inner_value), 29,
                "scoped enum metadata must beat stale rendered enum mirror authority");

  enum_scopes_by_text.pop_back();
  enum_scopes_by_key.pop_back();
  const std::optional<long long> outer_value = env.lookup(&inner_ref);
  expect_true(outer_value.has_value(),
              "scoped enum metadata lookup should find the restored outer value");
  expect_eq_int(static_cast<int>(*outer_value), 17,
                "scoped enum metadata should track local/block scope lifetime");

  c4c::Node missing_ref = var_ref("Same", "Same", other_text, -1);
  const std::optional<long long> missing_value = env.lookup(&missing_ref);
  expect_true(!missing_value.has_value(),
              "complete scoped enum metadata miss must not recover through stale rendered mirror");
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
  test_static_eval_int_keeps_same_spelled_enum_domains_distinct();
  test_static_eval_int_named_rendered_enum_compatibility_requires_opt_in();
  test_consteval_env_scoped_enum_metadata_beats_stale_rendered_mirror();
  std::cout << "PASS: cpp_hir_sema_consteval_type_utils_metadata_test\n";
  return 0;
}
