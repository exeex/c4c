#include "hir/hir_ir.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
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

void attach_alias(c4c::Node& owner,
                  const char* alias_name,
                  c4c::TypeSpec* alias_type) {
  static const char* alias_names[1];
  alias_names[0] = alias_name;
  owner.member_typedef_names = alias_names;
  owner.member_typedef_types = alias_type;
  owner.n_member_typedefs = 1;
}

void test_template_owner_fallback_uses_realized_owner_metadata_for_bases() {
  c4c::hir::Module module;
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId base_text = texts.intern("BaseWithAlias");
  const c4c::TextId box_text = texts.intern("BoxWithInheritedAlias");
  const c4c::TextId t_text = texts.intern("T");

  c4c::TypeSpec inherited_alias = make_scalar_ts(c4c::TB_LONG);
  c4c::Node base{};
  base.kind = c4c::NK_STRUCT_DEF;
  base.name = "BaseWithAlias";
  base.unqualified_name = "BaseWithAlias";
  base.unqualified_text_id = base_text;
  base.namespace_context_id = 41;
  attach_alias(base, "Alias", &inherited_alias);

  c4c::hir::HirStructDef base_def;
  base_def.tag = "BaseWithAlias";
  base_def.tag_text_id = base_text;
  base_def.ns_qual.context_id = 41;
  module.struct_defs[base_def.tag] = base_def;
  module.index_struct_def_owner(module.struct_defs[base_def.tag], true);

  c4c::TypeSpec base_type = make_scalar_ts(c4c::TB_STRUCT);
  base_type.record_def = &base;
  base_type.tag_text_id = base_text;
  base_type.namespace_context_id = 41;

  static const char* template_param_names[] = {"T"};
  static bool template_param_is_nttp[] = {false};
  static c4c::TextId template_param_text_ids[1];
  template_param_text_ids[0] = t_text;

  c4c::Node primary{};
  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "BoxWithInheritedAlias";
  primary.unqualified_name = "BoxWithInheritedAlias";
  primary.unqualified_text_id = box_text;
  primary.namespace_context_id = 41;
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.template_param_is_nttp = template_param_is_nttp;
  primary.n_template_params = 1;
  primary.base_types = &base_type;
  primary.n_bases = 1;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["BaseWithAlias"] = &base;
  lowerer.register_template_struct_primary("BoxWithInheritedAlias", &primary);

  c4c::TemplateArgRef args[1]{};
  args[0].kind = c4c::TemplateArgKind::Type;
  args[0].type = make_scalar_ts(c4c::TB_INT);

  c4c::TypeSpec owner = make_scalar_ts(c4c::TB_STRUCT);
  owner.tpl_struct_origin = "BoxWithInheritedAlias";
  owner.tpl_struct_args.data = args;
  owner.tpl_struct_args.size = 1;
  owner.deferred_member_type_name = "Alias";

  const bool ok = lowerer.resolve_struct_member_typedef_if_ready(&owner);
  expect_true(ok, "realized template owner should resolve inherited member typedef");
  expect_true(owner.base == c4c::TB_LONG,
              "inherited member typedef should come from the realized owner's base chain");
  expect_true(!module.struct_def_owner_index.empty(),
              "fixture should prove a structured owner index was available");
}

}  // namespace

int main() {
  test_template_owner_fallback_uses_realized_owner_metadata_for_bases();
  std::cout << "PASS: cpp_hir_member_typedef_realized_owner_metadata_test\n";
  return 0;
}
