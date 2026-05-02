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

c4c::Node make_zero_param_primary(const char* name,
                                  c4c::TextId text_id,
                                  int namespace_id) {
  c4c::Node node{};
  node.kind = c4c::NK_STRUCT_DEF;
  node.name = name;
  node.unqualified_name = name;
  node.unqualified_text_id = text_id;
  node.namespace_context_id = namespace_id;
  node.n_template_params = 0;
  return node;
}

void test_realize_template_struct_refreshes_structured_metadata() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  constexpr int structured_namespace = 7;
  constexpr int stale_namespace = 3;
  const c4c::TextId primary_text = texts.intern("StructuredRealized");
  const c4c::TextId stale_tag_text = texts.intern("StaleRenderedInput");

  c4c::Node primary =
      make_zero_param_primary("StructuredRealized", primary_text,
                              structured_namespace);
  lowerer.register_template_struct_primary("StructuredRealized", &primary);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StructuredRealized";
  ts.tpl_struct_origin_key.context_id = structured_namespace;
  ts.tpl_struct_origin_key.base_text_id = primary_text;
  ts.tag = "StaleRenderedInput";
  ts.tag_text_id = stale_tag_text;
  ts.namespace_context_id = stale_namespace;

  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  lowerer.realize_template_struct(ts, &primary, type_bindings, nttp_bindings);

  const auto realized_it = module.struct_defs.find("StructuredRealized");
  expect_true(realized_it != module.struct_defs.end(),
              "realization must create the structured template instance despite stale incoming rendered tag");
  expect_true(ts.tag_text_id == realized_it->second.tag_text_id,
              "realization must refresh tag_text_id from the realized HirStructDef");
  expect_true(ts.namespace_context_id == structured_namespace,
              "realization must refresh namespace_context_id from the realized HirStructDef namespace");
  expect_true(ts.tag != nullptr && std::string(ts.tag) == realized_it->second.tag,
              "legacy TypeSpec::tag assignment is final-spelling compatibility only");
}

}  // namespace

int main() {
  test_realize_template_struct_refreshes_structured_metadata();
  std::cout << "PASS: cpp_hir_template_realize_struct_metadata_test\n";
  return 0;
}
