#include "hir/hir_ir.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
    -> decltype((void)(ts.tag = tag)) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

c4c::Node make_primary(const char* name, c4c::TextId text_id,
                       int namespace_id) {
  c4c::Node node{};
  node.kind = c4c::NK_STRUCT_DEF;
  node.name = name;
  node.unqualified_name = name;
  node.unqualified_text_id = text_id;
  node.namespace_context_id = namespace_id;
  node.n_template_params = 1;
  return node;
}

c4c::Node make_zero_param_primary(const char* name, c4c::TextId text_id,
                                  int namespace_id) {
  c4c::Node node = make_primary(name, text_id, namespace_id);
  node.n_template_params = 0;
  return node;
}

void configure_one_type_param(c4c::Node& node) {
  static const char* param_names[] = {"T"};
  static c4c::TextId param_text_ids[] = {c4c::kInvalidText};
  static bool param_is_pack[] = {false};
  static bool param_is_nttp[] = {false};
  static bool param_has_default[] = {false};
  static c4c::TypeSpec param_default_types[] = {c4c::TypeSpec{}};
  static long long param_default_values[] = {0};
  node.template_param_names = param_names;
  node.template_param_name_text_ids = param_text_ids;
  node.template_param_is_pack = param_is_pack;
  node.template_param_is_nttp = param_is_nttp;
  node.template_param_has_default = param_has_default;
  node.template_param_default_types = param_default_types;
  node.template_param_default_values = param_default_values;
  node.n_template_params = 1;
}

c4c::TypeSpec make_origin_type(const char* rendered_origin,
                               const char* rendered_tag,
                               c4c::TextId origin_text_id,
                               c4c::TextId tag_text_id,
                               int namespace_id) {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = rendered_origin;
  ts.tpl_struct_origin_key.context_id = namespace_id;
  ts.tpl_struct_origin_key.base_text_id = origin_text_id;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = tag_text_id;
  ts.namespace_context_id = namespace_id;
  return ts;
}

void test_canonical_primary_origin_key_wins_over_stale_rendered_text() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleCanonicalPrimary");
  const c4c::TextId stale_instance_text =
      texts.intern("StaleCanonicalPrimary_T0");
  const c4c::TextId structured_text =
      texts.intern("StructuredCanonicalPrimary");

  c4c::Node stale_primary =
      make_primary("StaleCanonicalPrimary", stale_text, 0);
  c4c::Node structured_primary =
      make_primary("StructuredCanonicalPrimary", structured_text, 0);

  lowerer.register_template_struct_primary("StaleCanonicalPrimary",
                                           &stale_primary);
  lowerer.register_template_struct_primary("StructuredCanonicalPrimary",
                                           &structured_primary);

  c4c::TypeSpec ts =
      make_origin_type("StaleCanonicalPrimary_T0", "StaleCanonicalPrimary_T0",
                       structured_text, stale_instance_text, 0);

  expect_true(lowerer.canonical_template_struct_primary(ts) ==
                  &structured_primary,
              "structured template origin key must select the primary before stale rendered origin/tag fallback");
}

void test_canonical_primary_origin_key_miss_rejects_stale_rendered_text() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleCanonicalPrimaryMiss");
  const c4c::TextId stale_instance_text =
      texts.intern("StaleCanonicalPrimaryMiss_T0");
  const c4c::TextId missing_structured_text =
      texts.intern("MissingCanonicalPrimary");

  c4c::Node stale_primary =
      make_primary("StaleCanonicalPrimaryMiss", stale_text, 0);
  lowerer.register_template_struct_primary("StaleCanonicalPrimaryMiss",
                                           &stale_primary);

  c4c::TypeSpec ts =
      make_origin_type("StaleCanonicalPrimaryMiss_T0",
                       "StaleCanonicalPrimaryMiss_T0",
                       missing_structured_text, stale_instance_text, 0);

  expect_true(lowerer.canonical_template_struct_primary(ts) == nullptr,
              "complete structured template origin key miss must not recover through stale rendered origin/tag fallback");
}

void test_canonical_primary_partial_origin_metadata_rejects_qualified_family_root() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleQualifiedFamily");
  const c4c::TextId stale_instance_text =
      texts.intern("StaleQualifiedFamily_T0");
  const c4c::TextId structured_text =
      texts.intern("StructuredQualifiedFamily");

  c4c::Node stale_primary =
      make_primary("StaleQualifiedFamily", stale_text, 0);
  lowerer.register_template_struct_primary("StaleQualifiedFamily",
                                           &stale_primary);

  c4c::TypeSpec ts =
      make_origin_type("ns::StaleQualifiedFamily_T0",
                       "StaleQualifiedFamily_T0",
                       structured_text, stale_instance_text, -1);

  expect_true(
      lowerer.canonical_template_struct_primary(ts) == nullptr,
      "structured template origin metadata must block qualified _T family-root recovery after owner-key miss");
}

void test_canonical_primary_no_metadata_keeps_qualified_family_root_compatibility() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId legacy_text = texts.intern("LegacyQualifiedFamily");
  const c4c::TextId legacy_instance_text =
      texts.intern("LegacyQualifiedFamily_T0");

  c4c::Node legacy_primary =
      make_primary("LegacyQualifiedFamily", legacy_text, 0);
  lowerer.register_template_struct_primary("LegacyQualifiedFamily",
                                           &legacy_primary);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "ns::LegacyQualifiedFamily_T0";
  set_legacy_tag_if_present(ts, "LegacyQualifiedFamily_T0", 0);
  ts.tag_text_id = legacy_instance_text;

  expect_true(
      lowerer.canonical_template_struct_primary(ts) == &legacy_primary,
      "no-metadata template origin should keep legacy qualified _T family-root compatibility");
}

void test_canonical_primary_record_owner_miss_rejects_qualified_family_root() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleRecordFamily");
  const c4c::TextId stale_instance_text = texts.intern("StaleRecordFamily_T0");
  const c4c::TextId missing_text = texts.intern("MissingRecordFamily");

  c4c::Node stale_primary =
      make_primary("StaleRecordFamily", stale_text, 0);
  lowerer.register_template_struct_primary("StaleRecordFamily", &stale_primary);

  c4c::Node missing_record =
      make_primary("MissingRecordFamily", missing_text, 3);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "ns::StaleRecordFamily_T0";
  set_legacy_tag_if_present(ts, "StaleRecordFamily_T0", 0);
  ts.tag_text_id = stale_instance_text;
  ts.record_def = &missing_record;

  expect_true(
      lowerer.canonical_template_struct_primary(ts) == nullptr,
      "structured record-def owner metadata must block qualified _T family-root recovery after owner-key miss");
}

void test_collect_initial_type_definitions_rejects_stale_qualified_origin_recovery() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId rendered_primary_text = texts.intern("Wrapper");
  const c4c::TextId specialization_text = texts.intern("Wrapper_T_int");

  c4c::Node rendered_only_primary =
      make_primary("ns::Wrapper", rendered_primary_text, -1);
  rendered_only_primary.unqualified_name = "Wrapper";

  c4c::Node specialization{};
  specialization.kind = c4c::NK_STRUCT_DEF;
  specialization.name = "ns::Wrapper_T_int";
  specialization.unqualified_name = "Wrapper_T_int";
  specialization.unqualified_text_id = specialization_text;
  specialization.namespace_context_id = 7;
  specialization.template_origin_name = "Wrapper";

  const std::vector<const c4c::Node*> items = {
      &rendered_only_primary,
      &specialization,
  };
  lowerer.collect_initial_type_definitions(items);

  expect_true(
      lowerer.find_template_struct_specializations(&rendered_only_primary) ==
          nullptr,
      "structured template specialization registration must not recover a rendered-only ns::Wrapper primary by splitting ns::Wrapper_T_int");
}

void test_realize_template_struct_origin_key_miss_rejects_stale_rendered_primary() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleRealizePrimary");
  const c4c::TextId stale_instance_text = texts.intern("StaleRealizePrimary");
  const c4c::TextId missing_text = texts.intern("MissingRealizePrimary");

  c4c::Node stale_primary =
      make_zero_param_primary("StaleRealizePrimary", stale_text, 0);
  lowerer.register_template_struct_primary("StaleRealizePrimary",
                                           &stale_primary);

  c4c::TypeSpec ts =
      make_origin_type("StaleRealizePrimary", "StaleRealizePrimary",
                       missing_text, stale_instance_text, 0);

  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  lowerer.realize_template_struct(ts, nullptr, type_bindings, nttp_bindings);

  expect_true(module.struct_defs.find("StaleRealizePrimary") ==
                  module.struct_defs.end(),
              "realize_template_struct must not instantiate a stale rendered primary after complete origin-key miss");
  expect_true(ts.tpl_struct_origin &&
                  std::string(ts.tpl_struct_origin) == "StaleRealizePrimary",
              "blocked realization should leave the unresolved rendered origin intact");
}

void test_realize_template_struct_record_owner_miss_rejects_stale_rendered_primary() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleRecordRealizePrimary");
  const c4c::TextId missing_text = texts.intern("MissingRecordRealizePrimary");

  c4c::Node stale_primary =
      make_zero_param_primary("StaleRecordRealizePrimary", stale_text, 0);
  lowerer.register_template_struct_primary("StaleRecordRealizePrimary",
                                           &stale_primary);

  c4c::Node missing_record =
      make_primary("MissingRecordRealizePrimary", missing_text, 11);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StaleRecordRealizePrimary";
  set_legacy_tag_if_present(ts, "StaleRecordRealizePrimary", 0);
  ts.tag_text_id = stale_text;
  ts.record_def = &missing_record;

  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  lowerer.realize_template_struct(ts, nullptr, type_bindings, nttp_bindings);

  expect_true(module.struct_defs.find("StaleRecordRealizePrimary") ==
                  module.struct_defs.end(),
              "realize_template_struct must not instantiate a stale rendered primary after record-owner miss");
  expect_true(ts.tpl_struct_origin &&
                  std::string(ts.tpl_struct_origin) ==
                      "StaleRecordRealizePrimary",
              "blocked record-owner realization should leave the unresolved rendered origin intact");
}

void test_realize_template_struct_no_metadata_keeps_exact_rendered_compatibility() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId legacy_text = texts.intern("LegacyRealizePrimary");
  c4c::Node legacy_primary =
      make_primary("LegacyRealizePrimary", legacy_text, 0);
  configure_one_type_param(legacy_primary);
  lowerer.register_template_struct_primary("LegacyRealizePrimary",
                                           &legacy_primary);

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;

  c4c::TemplateArgRef arg{};
  arg.kind = c4c::TemplateArgKind::Type;
  arg.type = int_ts;
  c4c::TemplateArgRef args[] = {arg};

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "LegacyRealizePrimary";
  ts.tpl_struct_args = c4c::TemplateArgRefList{args, 1};
  set_legacy_tag_if_present(ts, "LegacyRealizePrimary", 0);
  ts.tag_text_id = legacy_text;

  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  lowerer.realize_template_struct(ts, nullptr, type_bindings, nttp_bindings);

  expect_true(ts.tpl_struct_origin == nullptr,
              "successful legacy realization should consume template origin metadata");
}

}  // namespace

int main() {
  test_canonical_primary_origin_key_wins_over_stale_rendered_text();
  test_canonical_primary_origin_key_miss_rejects_stale_rendered_text();
  test_canonical_primary_partial_origin_metadata_rejects_qualified_family_root();
  test_canonical_primary_no_metadata_keeps_qualified_family_root_compatibility();
  test_canonical_primary_record_owner_miss_rejects_qualified_family_root();
  test_collect_initial_type_definitions_rejects_stale_qualified_origin_recovery();
  test_realize_template_struct_origin_key_miss_rejects_stale_rendered_primary();
  test_realize_template_struct_record_owner_miss_rejects_stale_rendered_primary();
  test_realize_template_struct_no_metadata_keeps_exact_rendered_compatibility();
  std::cout << "PASS: cpp_hir_template_canonical_primary_origin_metadata_test\n";
  return 0;
}
