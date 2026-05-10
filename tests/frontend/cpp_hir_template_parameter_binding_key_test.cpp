#include "hir/hir_ir.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::hir::HirTemplateParameterBindingKey make_type_key(
    int owner_context, c4c::TextId owner_text_id, int parameter_index,
    c4c::TextId parameter_text_id) {
  c4c::hir::HirTemplateParameterBindingKey key{};
  key.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::Type;
  key.owner_namespace_context_id = owner_context;
  key.owner_template_text_id = owner_text_id;
  key.parameter_index = parameter_index;
  key.parameter_text_id = parameter_text_id;
  return key;
}

void test_same_spelling_distinct_by_owner_and_index() {
  constexpr c4c::TextId kParamT = 11;
  auto owner_a_t0 = make_type_key(3, 101, 0, kParamT);
  auto owner_b_t0 = make_type_key(4, 202, 0, kParamT);
  auto owner_a_t1 = make_type_key(3, 101, 1, kParamT);

  expect_true(owner_a_t0 != owner_b_t0,
              "same-spelled parameters from different owners must be distinct");
  expect_true(owner_a_t0 != owner_a_t1,
              "same-spelled parameters at different indices must be distinct");

  std::set<c4c::hir::HirTemplateParameterBindingKey> ordered_keys;
  ordered_keys.insert(owner_a_t0);
  ordered_keys.insert(owner_b_t0);
  ordered_keys.insert(owner_a_t1);
  ordered_keys.insert(owner_a_t0);
  expect_true(ordered_keys.size() == 3,
              "ordering must preserve owner/index-distinct keys");
}

void test_complete_metadata_rejects_text_id_only_key() {
  c4c::hir::HirTemplateParameterBindingKey text_id_only{};
  text_id_only.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::Type;
  text_id_only.parameter_text_id = 11;

  expect_true(
      !c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
          text_id_only),
      "raw parameter TextId alone must not count as complete binding metadata");

  auto complete = make_type_key(3, 101, 0, 11);
  expect_true(c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
                  complete),
              "owner text, parameter index, and parameter TextId should be complete");

  complete.owner_qualifier_segment_text_ids.push_back(c4c::kInvalidText);
  expect_true(
      !c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
          complete),
      "invalid owner qualifier TextIds must reject complete metadata");
}

void test_unordered_map_lookup_uses_structured_hash() {
  constexpr c4c::TextId kParamN = 17;
  c4c::hir::HirTemplateParameterBindingKey owner_a_n0 =
      make_type_key(8, 301, 0, kParamN);
  owner_a_n0.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::NonType;
  c4c::hir::HirTemplateParameterBindingKey owner_b_n0 = owner_a_n0;
  owner_b_n0.owner_template_text_id = 302;
  c4c::hir::HirTemplateParameterBindingKey owner_a_n1 = owner_a_n0;
  owner_a_n1.parameter_index = 1;

  c4c::hir::HirTemplateNttpBindings bindings;
  bindings.emplace(owner_a_n0, 10);
  bindings.emplace(owner_b_n0, 20);
  bindings.emplace(owner_a_n1, 30);

  expect_true(bindings.size() == 3,
              "hash map must store owner/index-distinct NTTP bindings");
  expect_true(bindings.at(owner_a_n0) == 10,
              "hash lookup should find first owner binding");
  expect_true(bindings.at(owner_b_n0) == 20,
              "hash lookup should find second owner binding");
  expect_true(bindings.at(owner_a_n1) == 30,
              "hash lookup should find second-index binding");
}

void test_constructs_distinct_structured_keys_from_template_nodes() {
  c4c::Node owner_a{};
  owner_a.namespace_context_id = 8;
  owner_a.unqualified_text_id = 301;
  owner_a.n_template_params = 2;
  const char* owner_a_names[] = {"T", "T"};
  c4c::TextId owner_a_text_ids[] = {17, 17};
  owner_a.template_param_names = owner_a_names;
  owner_a.template_param_name_text_ids = owner_a_text_ids;

  c4c::Node owner_b = owner_a;
  owner_b.namespace_context_id = 9;
  owner_b.unqualified_text_id = 302;

  auto owner_a_t0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_a, 0, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto owner_a_t1 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_a, 1, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto owner_b_t0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_b, 0, c4c::hir::HirTemplateParameterBindingKind::Type);

  expect_true(owner_a_t0.has_value(), "owner A index 0 should produce a key");
  expect_true(owner_a_t1.has_value(), "owner A index 1 should produce a key");
  expect_true(owner_b_t0.has_value(), "owner B index 0 should produce a key");
  expect_true(*owner_a_t0 != *owner_a_t1,
              "same-spelled template params on distinct indices must differ");
  expect_true(*owner_a_t0 != *owner_b_t0,
              "same-spelled template params on distinct owners must differ");

  c4c::hir::HirTemplateTypeBindings bindings;
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec char_ts{};
  char_ts.base = c4c::TB_CHAR;
  c4c::TypeSpec long_ts{};
  long_ts.base = c4c::TB_LONG;
  bindings.emplace(*owner_a_t0, int_ts);
  bindings.emplace(*owner_a_t1, char_ts);
  bindings.emplace(*owner_b_t0, long_ts);
  expect_true(bindings.size() == 3,
              "structured type bindings must keep owner/index keys distinct");
  expect_true(bindings.at(*owner_a_t0).base == c4c::TB_INT,
              "owner A index 0 binding should be preserved");
  expect_true(bindings.at(*owner_a_t1).base == c4c::TB_CHAR,
              "owner A index 1 binding should be preserved");
  expect_true(bindings.at(*owner_b_t0).base == c4c::TB_LONG,
              "owner B index 0 binding should be preserved");
}

}  // namespace

int main() {
  test_same_spelling_distinct_by_owner_and_index();
  test_complete_metadata_rejects_text_id_only_key();
  test_unordered_map_lookup_uses_structured_hash();
  test_constructs_distinct_structured_keys_from_template_nodes();
  std::cout << "PASS: cpp_hir_template_parameter_binding_key_test\n";
  return 0;
}
