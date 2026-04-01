// Unintegrated draft extracted from ast_to_hir.cpp.
// This file is not yet wired into the build; it stages callable/program-level
// lowering helpers so they can be split further before ast_to_hir.cpp is cleaned up.
//
// Intentionally omitted for now because they are tightly coupled to other
// clusters that will likely move separately:
// - lower_expr, lower_stmt_node, lower_call_expr, lower_stmt_expr_block
// - lower_function, lower_struct_method, lower_global, lower_local_decl_stmt
// - initializer normalization helpers
// - template collection / template-struct realization helpers

#include "ast_to_hir.hpp"
#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"
#include "type_utils.hpp"
#include "../parser/parser_internal.hpp"

#include <algorithm>
#include <stdexcept>

namespace c4c::hir {

void Lowerer::register_bodyless_callable(Function&& fn) {
  module_->fn_index[fn.name] = fn.id;
  module_->functions.push_back(std::move(fn));
}

bool Lowerer::maybe_register_bodyless_callable(Function* fn,
                                               bool has_lowerable_body) {
  if (has_lowerable_body) return false;
  register_bodyless_callable(std::move(*fn));
  return true;
}

BlockId Lowerer::begin_callable_body_lowering(Function& fn, FunctionCtx& ctx) {
  module_->fn_index[fn.name] = fn.id;
  if (fn.id.value == module_->functions.size()) {
    // Push a skeleton; callers replace it after body lowering completes.
    module_->functions.push_back(Function{fn.id, fn.name, fn.ns_qual, fn.return_type});
  }

  const BlockId entry = create_block(ctx);
  fn.entry = entry;
  ctx.current_block = entry;
  return entry;
}

void Lowerer::finish_lowered_callable(Function* fn, BlockId entry) {
  if (fn->blocks.empty()) {
    fn->blocks.push_back(Block{entry, {}, false});
  }
  module_->functions[fn->id.value] = std::move(*fn);
}

TypeSpec Lowerer::substitute_signature_template_type(
    TypeSpec ts, const TypeBindings* tpl_bindings) const {
  if (!tpl_bindings || ts.base != TB_TYPEDEF || !ts.tag) return ts;
  auto it = tpl_bindings->find(ts.tag);
  if (it == tpl_bindings->end()) return ts;
  const TypeSpec& concrete = it->second;
  const int outer_ptr_level = ts.ptr_level;
  const bool outer_lref = ts.is_lvalue_ref;
  const bool outer_rref = ts.is_rvalue_ref;
  const bool outer_const = ts.is_const;
  const bool outer_volatile = ts.is_volatile;
  ts = concrete;
  ts.ptr_level += outer_ptr_level;
  ts.is_lvalue_ref = concrete.is_lvalue_ref || outer_lref;
  ts.is_rvalue_ref = !ts.is_lvalue_ref && (concrete.is_rvalue_ref || outer_rref);
  ts.is_const = concrete.is_const || outer_const;
  ts.is_volatile = concrete.is_volatile || outer_volatile;
  return ts;
}

void Lowerer::resolve_signature_template_type_if_needed(
    TypeSpec& ts,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const Node* span_node,
    const std::string& context_name) {
  if (!tpl_bindings || !ts.tpl_struct_origin) return;
  NttpBindings nttp_empty;
  seed_and_resolve_pending_template_type_if_needed(
      ts, *tpl_bindings, nttp_bindings ? *nttp_bindings : nttp_empty,
      span_node, PendingTemplateTypeKind::DeclarationType, context_name);
}

TypeSpec Lowerer::prepare_callable_return_type(
    TypeSpec ret_ts,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const Node* span_node,
    const std::string& context_name,
    bool resolve_typedef_struct) {
  ret_ts = substitute_signature_template_type(ret_ts, tpl_bindings);
  resolve_signature_template_type_if_needed(
      ret_ts, tpl_bindings, nttp_bindings, span_node, context_name);
  if (resolve_typedef_struct) resolve_typedef_to_struct(ret_ts);
  return ret_ts;
}

void Lowerer::append_explicit_callable_param(
    Function& fn,
    FunctionCtx& ctx,
    const Node* param_node,
    const std::string& emitted_name,
    TypeSpec param_ts,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const std::string& context_name,
    bool resolve_typedef_struct) {
  param_ts = substitute_signature_template_type(param_ts, tpl_bindings);
  resolve_signature_template_type_if_needed(
      param_ts, tpl_bindings, nttp_bindings, param_node, context_name);
  if (resolve_typedef_struct) resolve_typedef_to_struct(param_ts);

  Param param{};
  param.name = emitted_name;
  param.type = qtype_from(reference_storage_ts(param_ts), ValueCategory::LValue);
  param.fn_ptr_sig = fn_ptr_sig_from_decl_node(param_node);
  param.span = make_span(param_node);
  ctx.params[param.name] = static_cast<uint32_t>(fn.params.size());
  if (param.fn_ptr_sig) ctx.param_fn_ptr_sigs[param.name] = *param.fn_ptr_sig;
  fn.params.push_back(std::move(param));
}

void Lowerer::append_callable_params(
    Function& fn,
    FunctionCtx& ctx,
    const Node* callable_node,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const std::string& context_prefix,
    bool resolve_typedef_struct,
    bool expand_parameter_packs) {
  for (int i = 0; i < callable_node->n_params; ++i) {
    const Node* p = callable_node->params[i];
    if (!p) continue;
    const std::string param_name = p->name ? p->name : "<anon_param>";

    if (expand_parameter_packs && p->is_parameter_pack && tpl_bindings &&
        p->type.base == TB_TYPEDEF && p->type.tag) {
      std::vector<std::pair<int, TypeSpec>> pack_types;
      for (const auto& [key, ts] : *tpl_bindings) {
        int pack_index = 0;
        if (parse_pack_binding_name(key, p->type.tag, &pack_index)) {
          pack_types.push_back({pack_index, ts});
        }
      }
      std::sort(pack_types.begin(), pack_types.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const auto& [pack_index, concrete] : pack_types) {
        TypeSpec param_ts = concrete;
        const bool outer_lref = p->type.is_lvalue_ref;
        const bool outer_rref = p->type.is_rvalue_ref;
        param_ts.is_lvalue_ref = concrete.is_lvalue_ref || outer_lref;
        param_ts.is_rvalue_ref =
            !param_ts.is_lvalue_ref && (concrete.is_rvalue_ref || outer_rref);
        const std::string emitted_name =
            param_name + "__pack" + std::to_string(pack_index);
        append_explicit_callable_param(
            fn, ctx, p, emitted_name, param_ts, tpl_bindings, nttp_bindings,
            context_prefix + emitted_name, resolve_typedef_struct);
        ctx.pack_params[param_name].push_back(
            FunctionCtx::PackParamElem{
                emitted_name, param_ts, static_cast<uint32_t>(fn.params.size() - 1)});
      }
      continue;
    }

    append_explicit_callable_param(
        fn, ctx, p, param_name, p->type, tpl_bindings, nttp_bindings,
        context_prefix + param_name, resolve_typedef_struct);
  }
}

}  // namespace c4c::hir
