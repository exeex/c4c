#include "hir/hir_ir.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
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

}  // namespace

int main() {
  test_canonical_primary_origin_key_wins_over_stale_rendered_text();
  test_canonical_primary_origin_key_miss_rejects_stale_rendered_text();
  std::cout << "PASS: cpp_hir_template_canonical_primary_origin_metadata_test\n";
  return 0;
}
