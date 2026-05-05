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

c4c::TypeSpec make_scalar_ts(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void add_struct_def(c4c::hir::Module& module,
                    const char* tag,
                    c4c::TextId tag_text_id,
                    int namespace_id) {
  c4c::hir::HirStructDef def{};
  def.tag = tag;
  def.tag_text_id = tag_text_id;
  def.ns_qual.context_id = namespace_id;
  module.index_struct_def_owner(def, true);
  module.struct_defs[def.tag] = def;
}

void test_member_lookup_owner_uses_tag_text_id_before_stale_tag() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId real_text = texts.intern("StructuredOwner");
  const c4c::TextId stale_text = texts.intern("StaleRenderedOwner");
  add_struct_def(module, "StructuredOwner", real_text, 7);
  add_struct_def(module, "StaleRenderedOwner", stale_text, 7);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec base_ts = make_scalar_ts(c4c::TB_STRUCT);
  set_legacy_tag_if_present(base_ts, "StaleRenderedOwner", 0);
  base_ts.tag_text_id = real_text;
  base_ts.namespace_context_id = 7;

  std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base_ts, false, nullptr, nullptr, nullptr, nullptr,
          "value-args-owner-metadata-test");
  expect_true(owner_tag && *owner_tag == "StructuredOwner",
              "member lookup owner should use tag_text_id before stale rendered tag");
}

void test_ast_template_type_arg_uses_template_param_text_id_binding() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId t_text = texts.intern("T");

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["StaleRenderedParam"] = make_scalar_ts(c4c::TB_LONG);
  ctx.tpl_bindings_by_text[t_text] = make_scalar_ts(c4c::TB_INT);

  c4c::TypeSpec arg_type = make_scalar_ts(c4c::TB_TYPEDEF);
  set_legacy_tag_if_present(arg_type, "StaleRenderedParam", 0);
  arg_type.template_param_text_id = t_text;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.n_template_args = 1;
  ref.template_arg_types = &arg_type;

  c4c::TypeSpec owner = make_scalar_ts(c4c::TB_STRUCT);
  lowerer.assign_template_arg_refs_from_ast_args(
      &owner, &ref, nullptr, &ctx, &ref,
      c4c::hir::PendingTemplateTypeKind::DeclarationType,
      "value-args-template-type-binding-test");

  expect_true(owner.tpl_struct_args.data && owner.tpl_struct_args.size == 1,
              "template arg refs should be populated");
  expect_true(owner.tpl_struct_args.data[0].type.base == c4c::TB_INT,
              "template_param_text_id binding should beat stale rendered tag");
}

void test_template_static_member_type_arg_uses_template_param_text_id_binding() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId t_text = texts.intern("T");

  static const char* template_param_names[] = {"T"};
  static c4c::TextId template_param_text_ids[1];
  template_param_text_ids[0] = t_text;

  c4c::Node primary{};
  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "is_same";
  primary.unqualified_name = "is_same";
  primary.unqualified_text_id = texts.intern("is_same");
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.n_template_params = 1;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.register_template_struct_primary("is_same", &primary);

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["StaleRenderedParam"] = make_scalar_ts(c4c::TB_LONG);
  ctx.tpl_bindings_by_text[t_text] = make_scalar_ts(c4c::TB_INT);

  c4c::TypeSpec args[2];
  args[0] = make_scalar_ts(c4c::TB_TYPEDEF);
  set_legacy_tag_if_present(args[0], "StaleRenderedParam", 0);
  args[0].template_param_text_id = t_text;
  args[1] = make_scalar_ts(c4c::TB_INT);

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.n_template_args = 2;
  ref.template_arg_types = args;

  std::optional<long long> value =
      lowerer.try_eval_template_static_member_const(
          &ctx, "is_same", &ref, "value");
  expect_true(value && *value == 1,
              "template static-member type args should bind by template_param_text_id");
}

void test_is_reference_trait_uses_structured_owner_before_stale_tag() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId trait_text = texts.intern("is_reference");
  const c4c::TextId owner_text = texts.intern("add_lvalue_reference_int");
  add_struct_def(module, "add_lvalue_reference_int", owner_text, 5);

  static const char* template_param_names[] = {"T"};
  static c4c::TextId template_param_text_ids[1];
  template_param_text_ids[0] = texts.intern("T");

  c4c::Node primary{};
  primary.kind = c4c::NK_STRUCT_DEF;
  primary.name = "is_reference";
  primary.unqualified_name = "is_reference";
  primary.unqualified_text_id = trait_text;
  primary.template_param_names = template_param_names;
  primary.template_param_name_text_ids = template_param_text_ids;
  primary.n_template_params = 1;

  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = "add_lvalue_reference_int";
  owner.unqualified_name = "add_lvalue_reference_int";
  owner.unqualified_text_id = owner_text;
  owner.namespace_context_id = 5;
  owner.template_origin_name = "add_lvalue_reference";

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.register_template_struct_primary("is_reference", &primary);
  lowerer.struct_def_nodes_["add_lvalue_reference_int"] = &owner;

  c4c::TypeSpec arg = make_scalar_ts(c4c::TB_STRUCT);
  set_legacy_tag_if_present(arg, "StaleRenderedReferenceOwner", 0);
  arg.tag_text_id = owner_text;
  arg.namespace_context_id = 5;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.n_template_args = 1;
  ref.template_arg_types = &arg;

  std::optional<long long> value =
      lowerer.try_eval_template_static_member_const(
          nullptr, "is_reference", &ref, "value");
  expect_true(value && *value == 1,
              "is_reference recovery should use structured owner metadata before stale rendered tag");
}

}  // namespace

int main() {
  test_member_lookup_owner_uses_tag_text_id_before_stale_tag();
  test_ast_template_type_arg_uses_template_param_text_id_binding();
  test_template_static_member_type_arg_uses_template_param_text_id_binding();
  test_is_reference_trait_uses_structured_owner_before_stale_tag();
  std::cout << "PASS: cpp_hir_value_args_residual_metadata_test\n";
  return 0;
}
