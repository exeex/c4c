#include "hir/hir_ir.hpp"

#include <cstdlib>
#include <iostream>
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

}  // namespace

int main() {
  test_same_spelling_distinct_by_owner_and_index();
  test_complete_metadata_rejects_text_id_only_key();
  test_unordered_map_lookup_uses_structured_hash();
  std::cout << "PASS: cpp_hir_template_parameter_binding_key_test\n";
  return 0;
}
