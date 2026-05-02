#include "hir/impl/templates/templates.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::Node make_primary_template() {
  static const char* primary_param_names[] = {"PrimaryT"};
  static bool primary_param_is_nttp[] = {false};
  static bool primary_param_has_default[] = {false};

  c4c::Node primary{};
  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "Selector";
  primary.unqualified_name = "Selector";
  primary.template_param_names = primary_param_names;
  primary.template_param_is_nttp = primary_param_is_nttp;
  primary.template_param_has_default = primary_param_has_default;
  primary.n_template_params = 1;
  return primary;
}

c4c::hir::HirTemplateArg make_actual_type(c4c::TypeBase base) {
  c4c::hir::HirTemplateArg arg{};
  arg.type.base = base;
  return arg;
}

c4c::Node make_single_type_arg_specialization(
    c4c::TypeSpec pattern,
    const char** param_names,
    c4c::TextId* param_text_ids,
    bool* param_is_nttp,
    bool* param_has_default) {
  c4c::Node spec{};
  spec.kind = c4c::NK_STRUCT_DEF;
  spec.name = "SelectorSpecialization";
  spec.unqualified_name = "Selector";
  spec.template_param_names = param_names;
  spec.template_param_name_text_ids = param_text_ids;
  spec.template_param_is_nttp = param_is_nttp;
  spec.template_param_has_default = param_has_default;
  spec.n_template_params = 2;
  spec.template_arg_types = new c4c::TypeSpec[1]{pattern};
  spec.n_template_args = 1;
  return spec;
}

void destroy_specialization(c4c::Node* spec) {
  delete[] spec->template_arg_types;
  spec->template_arg_types = nullptr;
}

void test_pattern_match_prefers_template_param_text_id_over_stale_tag() {
  const char* param_names[] = {"T", "U"};
  c4c::TextId param_text_ids[] = {101, 202};
  bool param_is_nttp[] = {false, false};
  bool param_has_default[] = {false, true};

  c4c::TypeSpec pattern{};
  pattern.base = c4c::TB_TYPEDEF;
  pattern.tag = "U";
  pattern.template_param_text_id = 101;

  c4c::Node primary = make_primary_template();
  c4c::Node spec = make_single_type_arg_specialization(
      pattern, param_names, param_text_ids, param_is_nttp, param_has_default);
  std::vector<const c4c::Node*> specializations = {&spec};
  c4c::hir::TemplateStructEnv env{&primary, &specializations};
  std::vector<c4c::hir::HirTemplateArg> actual_args = {
      make_actual_type(c4c::TB_INT)};

  c4c::hir::SelectedTemplateStructPattern selected =
      c4c::hir::select_template_struct_pattern_hir(actual_args, env);

  expect_true(selected.selected_pattern == &spec,
              "structured template parameter TextId should select the specialization despite stale tag text");
  auto t_binding = selected.type_bindings.find("T");
  expect_true(t_binding != selected.type_bindings.end() &&
                  t_binding->second.base == c4c::TB_INT,
              "pattern matching should bind the structured T parameter");
  expect_true(selected.type_bindings.count("U") == 0,
              "stale rendered tag text must not bind the wrong parameter");

  destroy_specialization(&spec);
}

void test_pattern_match_rejects_tag_fallback_when_metadata_mismatches() {
  const char* param_names[] = {"T", "U"};
  c4c::TextId param_text_ids[] = {101, 202};
  bool param_is_nttp[] = {false, false};
  bool param_has_default[] = {false, true};

  c4c::TypeSpec pattern{};
  pattern.base = c4c::TB_TYPEDEF;
  pattern.tag = "T";
  pattern.template_param_text_id = 202;

  c4c::Node primary = make_primary_template();
  c4c::Node spec = make_single_type_arg_specialization(
      pattern, param_names, param_text_ids, param_is_nttp, param_has_default);
  std::vector<const c4c::Node*> specializations = {&spec};
  c4c::hir::TemplateStructEnv env{&primary, &specializations};
  std::vector<c4c::hir::HirTemplateArg> actual_args = {
      make_actual_type(c4c::TB_LONG)};

  c4c::hir::SelectedTemplateStructPattern selected =
      c4c::hir::select_template_struct_pattern_hir(actual_args, env);

  expect_true(selected.selected_pattern != &spec,
              "mismatched structured metadata must not recover through stale tag text");

  destroy_specialization(&spec);
}

void test_pattern_match_keeps_no_metadata_tag_fallback() {
  const char* param_names[] = {"T", "U"};
  c4c::TextId param_text_ids[] = {101, 202};
  bool param_is_nttp[] = {false, false};
  bool param_has_default[] = {false, true};

  c4c::TypeSpec pattern{};
  pattern.base = c4c::TB_TYPEDEF;
  pattern.tag = "T";

  c4c::Node primary = make_primary_template();
  c4c::Node spec = make_single_type_arg_specialization(
      pattern, param_names, param_text_ids, param_is_nttp, param_has_default);
  std::vector<const c4c::Node*> specializations = {&spec};
  c4c::hir::TemplateStructEnv env{&primary, &specializations};
  std::vector<c4c::hir::HirTemplateArg> actual_args = {
      make_actual_type(c4c::TB_UINT)};

  c4c::hir::SelectedTemplateStructPattern selected =
      c4c::hir::select_template_struct_pattern_hir(actual_args, env);

  expect_true(selected.selected_pattern == &spec,
              "tag-only compatibility fallback should bind when no structured carrier exists");
  auto t_binding = selected.type_bindings.find("T");
  expect_true(t_binding != selected.type_bindings.end() &&
                  t_binding->second.base == c4c::TB_UINT,
              "tag-only fallback should preserve legacy type parameter binding");

  destroy_specialization(&spec);
}

}  // namespace

int main() {
  test_pattern_match_prefers_template_param_text_id_over_stale_tag();
  test_pattern_match_rejects_tag_fallback_when_metadata_mismatches();
  test_pattern_match_keeps_no_metadata_tag_fallback();
  std::cout << "PASS: cpp_hir_template_pattern_match_metadata_test\n";
  return 0;
}
