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

c4c::TypeSpec make_scalar_ts(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_recover_template_identity_uses_record_def_without_rendered_tag() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId box_text = texts.intern("RecoverBox");
  const c4c::TextId t_text = texts.intern("T");

  c4c::TypeSpec int_arg = make_scalar_ts(c4c::TB_INT);
  static const char* template_param_names[] = {"T"};
  static c4c::TextId template_param_text_ids[1];
  template_param_text_ids[0] = t_text;

  c4c::Node primary{};
  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "RecoverBox";
  primary.unqualified_name = "RecoverBox";
  primary.unqualified_text_id = box_text;
  primary.namespace_context_id = 17;
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.n_template_params = 1;

  c4c::Node realized{};
  realized.kind = c4c::NK_STRUCT_DEF;
  realized.name = "RecoverBox_Tint";
  realized.unqualified_name = "RecoverBox_Tint";
  realized.unqualified_text_id = texts.intern("RecoverBox_Tint");
  realized.namespace_context_id = 17;
  realized.template_origin_name = "RecoverBox";
  realized.template_param_names = template_param_names;
  realized.template_param_name_text_ids = template_param_text_ids;
  realized.n_template_params = 1;
  realized.template_arg_types = &int_arg;
  realized.n_template_args = 1;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.register_template_struct_primary("RecoverBox", &primary);
  lowerer.struct_def_nodes_by_owner_[*lowerer.make_struct_def_node_owner_key(&realized)] =
      &realized;

  c4c::TypeSpec owner_ts = make_scalar_ts(c4c::TB_STRUCT);
  owner_ts.record_def = &realized;
  owner_ts.tag_text_id = realized.unqualified_text_id;
  owner_ts.namespace_context_id = 17;

  const bool recovered =
      lowerer.recover_template_struct_identity_from_tag(&owner_ts);
  expect_true(recovered,
              "record_def metadata should recover template identity without rendered tag");
  expect_true(owner_ts.tpl_struct_origin &&
                  std::string(owner_ts.tpl_struct_origin) == "RecoverBox",
              "recovered origin should come from structured owner metadata");
  expect_true(owner_ts.tpl_struct_args.data &&
                  owner_ts.tpl_struct_args.size == 1,
              "recovery should populate template argument refs from owner metadata");
  expect_true(owner_ts.tpl_struct_args.data[0].kind ==
                  c4c::TemplateArgKind::Type,
              "recovered template argument should preserve type kind");
  expect_true(owner_ts.tpl_struct_args.data[0].type.base == c4c::TB_INT,
              "recovered template argument should preserve structured type payload");
}

}  // namespace

int main() {
  test_recover_template_identity_uses_record_def_without_rendered_tag();
  std::cout << "PASS: cpp_hir_template_recover_identity_metadata_test\n";
  return 0;
}
