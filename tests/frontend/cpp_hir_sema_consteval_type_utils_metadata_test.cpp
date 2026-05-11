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

c4c::TypeSpec scalar_type(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

c4c::TypeSpec typedef_ref(c4c::TextId text_id = c4c::kInvalidText) {
  c4c::TypeSpec ts = scalar_type(c4c::TB_TYPEDEF);
  ts.tag_text_id = text_id;
  return ts;
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

void test_consteval_type_binding_intrinsic_metadata_fails_closed() {
  c4c::TextTable texts;
  const c4c::TextId owner_text = texts.intern("TemplateOwner");
  const c4c::TextId param_text = texts.intern("T");
  const c4c::TextId missing_text = texts.intern("MissingT");

  c4c::hir::TypeBindingStructuredKey key;
  key.namespace_context_id = 9;
  key.template_text_id = owner_text;
  key.param_index = 0;
  key.param_text_id = param_text;

  c4c::hir::TypeBindings rendered;
  rendered.emplace("T", scalar_type(c4c::TB_INT));
  c4c::hir::TypeBindingTextMap by_text;
  by_text.emplace(param_text, scalar_type(c4c::TB_LONGLONG));
  c4c::hir::TypeBindingStructuredMap by_key;
  by_key.emplace(key, scalar_type(c4c::TB_CHAR));
  c4c::hir::TypeBindingNameTextMap text_ids_by_name;
  text_ids_by_name.emplace("T", param_text);
  c4c::hir::TypeBindingNameStructuredMap keys_by_name;
  keys_by_name.emplace("T", key);

  c4c::hir::ConstEvalEnv env{};
  env.type_bindings = &rendered;
  env.type_bindings_by_text = &by_text;
  env.type_bindings_by_key = &by_key;
  env.type_binding_text_ids_by_name = &text_ids_by_name;
  env.type_binding_keys_by_name = &keys_by_name;

  c4c::TypeSpec structured_ref = typedef_ref(param_text);
  structured_ref.template_param_owner_namespace_context_id =
      key.namespace_context_id;
  structured_ref.template_param_owner_text_id = key.template_text_id;
  structured_ref.template_param_index = key.param_index;
  structured_ref.template_param_text_id = key.param_text_id;

  c4c::Node structured_node = sizeof_type_node(structured_ref);
  const c4c::hir::ConstEvalResult structured =
      c4c::hir::evaluate_constant_expr(&structured_node, env);
  expect_true(structured.ok(),
              "consteval type binding should resolve through complete structured metadata");
  expect_eq_int(static_cast<int>(structured.as_int()), 1,
                "structured type binding metadata should beat text and rendered mirrors");

  by_key.clear();
  const c4c::hir::ConstEvalResult structured_miss =
      c4c::hir::evaluate_constant_expr(&structured_node, env);
  expect_true(!structured_miss.ok(),
              "complete structured type binding miss must not recover through text or rendered mirrors");

  env.type_bindings_by_key = nullptr;
  c4c::Node text_miss_node = sizeof_type_node(typedef_ref(missing_text));
  const c4c::hir::ConstEvalResult text_miss =
      c4c::hir::evaluate_constant_expr(&text_miss_node, env);
  expect_true(!text_miss.ok(),
              "complete TextId type binding miss must not recover through rendered mirrors");
}

void test_consteval_type_binding_name_mirrors_are_legacy_bridge_only() {
  c4c::TextTable texts;
  const c4c::TextId param_text = texts.intern("T");

  c4c::hir::TypeBindingStructuredKey key;
  key.namespace_context_id = 4;
  key.template_text_id = texts.intern("TemplateOwner");
  key.param_index = 0;
  key.param_text_id = param_text;

  c4c::hir::TypeBindings rendered;
  rendered.emplace("T", scalar_type(c4c::TB_INT));
  c4c::hir::TypeBindingTextMap by_text;
  by_text.emplace(param_text, scalar_type(c4c::TB_LONGLONG));
  c4c::hir::TypeBindingStructuredMap by_key;
  by_key.emplace(key, scalar_type(c4c::TB_CHAR));
  c4c::hir::TypeBindingNameTextMap text_ids_by_name;
  text_ids_by_name.emplace("T", param_text);
  c4c::hir::TypeBindingNameStructuredMap keys_by_name;
  keys_by_name.emplace("T", key);

  c4c::hir::ConstEvalEnv env{};
  env.type_bindings = &rendered;
  env.type_bindings_by_text = &by_text;
  env.type_bindings_by_key = &by_key;
  env.type_binding_text_ids_by_name = &text_ids_by_name;
  env.type_binding_keys_by_name = &keys_by_name;

  c4c::Node no_metadata_node = sizeof_type_node(typedef_ref());
  const c4c::hir::ConstEvalResult no_metadata =
      c4c::hir::evaluate_constant_expr(&no_metadata_node, env);
  expect_true(!no_metadata.ok(),
              "type binding name mirrors must not act as ordinary lookup authority");

  c4c::Node text_node = sizeof_type_node(typedef_ref(param_text));
  const c4c::hir::ConstEvalResult text =
      c4c::hir::evaluate_constant_expr(&text_node, env);
  expect_true(text.ok(),
              "direct TextId type binding metadata should still resolve with name mirrors present");
  expect_eq_int(static_cast<int>(text.as_int()), 8,
                "name mirrors should not override direct TextId type binding authority");
}

void test_consteval_nttp_binding_mirrors_are_metadata_authority() {
  c4c::TextTable texts;
  const c4c::TextId param_text = texts.intern("N");
  const c4c::TextId other_text = texts.intern("OtherN");

  c4c::hir::ConstMap rendered;
  rendered.emplace("RenderedN", 42);
  c4c::hir::ConstTextMap by_text;
  by_text.emplace(param_text, 5);
  c4c::hir::ConstEvalStructuredNameKey param_key;
  param_key.base_text_id = param_text;
  c4c::hir::ConstStructuredMap by_key;
  by_key.emplace(param_key, 7);

  c4c::hir::ConstEvalEnv env{};
  env.nttp_bindings = &rendered;
  env.nttp_bindings_by_text = &by_text;
  env.nttp_bindings_by_key = &by_key;

  c4c::Node ref = var_ref("RenderedN", "N", param_text, -1);
  std::optional<long long> value = env.lookup(&ref);
  expect_true(value.has_value(),
              "complete NTTP key metadata should resolve the binding");
  expect_eq_int(static_cast<int>(*value), 7,
                "complete NTTP key metadata should beat TextId and rendered mirrors");

  by_key.clear();
  value = env.lookup(&ref);
  expect_true(value.has_value(),
              "complete NTTP TextId metadata should resolve the binding");
  expect_eq_int(static_cast<int>(*value), 5,
                "complete NTTP TextId metadata should beat rendered mirrors");

  by_text.clear();
  c4c::hir::ConstEvalStructuredNameKey other_key;
  other_key.base_text_id = other_text;
  by_key.emplace(other_key, 9);
  by_text.emplace(other_text, 11);
  value = env.lookup(&ref);
  expect_true(!value.has_value(),
              "complete NTTP key/text misses must not recover through rendered mirrors");

  env.nttp_bindings_by_text = nullptr;
  env.nttp_bindings_by_key = nullptr;
  value = env.lookup(&ref);
  expect_true(value.has_value(),
              "no-metadata NTTP compatibility should keep rendered lookup available");
  expect_eq_int(static_cast<int>(*value), 42,
                "rendered NTTP map should remain a no-metadata compatibility bridge");
}

void test_consteval_forwarded_nttp_binding_fallback_is_metadata_fenced() {
  c4c::TextTable texts;
  const c4c::TextId inner_text = texts.intern("InnerN");
  const c4c::TextId outer_text = texts.intern("OuterN");
  const c4c::TextId other_text = texts.intern("OtherN");

  c4c::Node func_def{};
  func_def.kind = c4c::NK_FUNCTION;
  func_def.name = "fn";
  func_def.unqualified_name = "fn";
  func_def.unqualified_text_id = texts.intern("fn");
  func_def.n_template_params = 1;
  const char* param_names[] = {"InnerN"};
  c4c::TextId param_text_ids[] = {inner_text};
  bool param_is_nttp[] = {true};
  func_def.template_param_names = param_names;
  func_def.template_param_name_text_ids = param_text_ids;
  func_def.template_param_is_nttp = param_is_nttp;

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "fn";
  callee.has_template_args = true;
  callee.n_template_args = 1;
  bool arg_is_value[] = {true};
  long long arg_values[] = {0};
  const char* nttp_names[] = {"RenderedOuter"};
  c4c::TextId arg_text_ids[] = {outer_text};
  callee.template_arg_is_value = arg_is_value;
  callee.template_arg_values = arg_values;
  callee.template_arg_nttp_names = nttp_names;
  callee.template_arg_nttp_text_ids = arg_text_ids;

  c4c::hir::ConstMap rendered_outer;
  rendered_outer.emplace("RenderedOuter", 44);
  c4c::hir::ConstTextMap text_outer;
  text_outer.emplace(outer_text, 5);
  c4c::hir::ConstEvalStructuredNameKey outer_key;
  outer_key.base_text_id = outer_text;
  c4c::hir::ConstStructuredMap key_outer;
  key_outer.emplace(outer_key, 7);

  c4c::hir::ConstEvalEnv outer_env{};
  outer_env.nttp_bindings = &rendered_outer;
  outer_env.nttp_bindings_by_text = &text_outer;
  outer_env.nttp_bindings_by_key = &key_outer;

  c4c::hir::TypeBindings type_bindings;
  c4c::hir::ConstMap nttp_bindings;
  c4c::hir::ConstTextMap nttp_bindings_by_text;
  c4c::hir::ConstStructuredMap nttp_bindings_by_key;
  (void)c4c::hir::bind_consteval_call_env(
      &callee, &func_def, outer_env, &type_bindings, &nttp_bindings, nullptr,
      nullptr, nullptr, nullptr, &nttp_bindings_by_text,
      &nttp_bindings_by_key);

  expect_true(nttp_bindings.count("InnerN") == 1 &&
                  nttp_bindings.at("InnerN") == 5,
              "forwarded NTTP TextId metadata should beat rendered fallback");
  expect_true(nttp_bindings_by_text.count(inner_text) == 1 &&
                  nttp_bindings_by_text.at(inner_text) == 5,
              "forwarded NTTP binding should record downstream TextId mirrors");

  text_outer.clear();
  text_outer.emplace(other_text, 17);
  key_outer.clear();
  c4c::hir::ConstEvalStructuredNameKey other_key;
  other_key.base_text_id = other_text;
  key_outer.emplace(other_key, 19);
  nttp_bindings.clear();
  nttp_bindings_by_text.clear();
  nttp_bindings_by_key.clear();
  (void)c4c::hir::bind_consteval_call_env(
      &callee, &func_def, outer_env, &type_bindings, &nttp_bindings, nullptr,
      nullptr, nullptr, nullptr, &nttp_bindings_by_text,
      &nttp_bindings_by_key);
  expect_true(nttp_bindings.empty() && nttp_bindings_by_text.empty() &&
                  nttp_bindings_by_key.empty(),
              "forwarded NTTP key/text misses must not recover through rendered mirrors");

  arg_text_ids[0] = c4c::kInvalidText;
  nttp_bindings.clear();
  nttp_bindings_by_text.clear();
  nttp_bindings_by_key.clear();
  (void)c4c::hir::bind_consteval_call_env(
      &callee, &func_def, outer_env, &type_bindings, &nttp_bindings, nullptr,
      nullptr, nullptr, nullptr, &nttp_bindings_by_text,
      &nttp_bindings_by_key);
  expect_true(nttp_bindings.count("InnerN") == 1 &&
                  nttp_bindings.at("InnerN") == 44,
              "no-metadata forwarded NTTP compatibility should keep rendered lookup available");
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
  test_consteval_type_binding_intrinsic_metadata_fails_closed();
  test_consteval_type_binding_name_mirrors_are_legacy_bridge_only();
  test_consteval_nttp_binding_mirrors_are_metadata_authority();
  test_consteval_forwarded_nttp_binding_fallback_is_metadata_fenced();
  test_static_eval_int_prefers_structured_enum_metadata();
  test_static_eval_int_structured_enum_miss_rejects_rendered_bridge();
  test_static_eval_int_uses_text_id_enum_metadata_before_rendered_bridge();
  test_static_eval_int_keeps_same_spelled_enum_domains_distinct();
  test_static_eval_int_named_rendered_enum_compatibility_requires_opt_in();
  test_consteval_env_scoped_enum_metadata_beats_stale_rendered_mirror();
  std::cout << "PASS: cpp_hir_sema_consteval_type_utils_metadata_test\n";
  return 0;
}
