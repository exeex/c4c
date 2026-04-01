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

#if 0
void Lowerer::lower_initial_program(const Node* root, Module& m) {
  if (!root || root->kind != NK_PROGRAM) {
    throw std::runtime_error("build_initial_hir: root is not NK_PROGRAM");
  }

  module_ = &m;

  std::vector<const Node*> items = flatten_program_items(root);

  collect_weak_symbol_names(items);
  collect_initial_type_definitions(items);
  collect_consteval_function_definitions(items);
  collect_template_function_definitions(items);
  collect_function_template_specializations(items);
  collect_depth0_template_instantiations(items);
  registry_.realize_seeds();
  run_consteval_template_seed_fixpoint(items);
  finalize_template_seed_realization();
  populate_hir_template_defs(m);
  collect_ref_overloaded_free_functions(items);
  attach_out_of_class_struct_method_defs(items, m);
  lower_non_method_functions_and_globals(items, m);
  lower_pending_struct_methods();
}

bool Lowerer::instantiate_deferred_template(const std::string& tpl_name,
                                            const TypeBindings& bindings,
                                            const NttpBindings& nttp_bindings,
                                            const std::string& mangled) {
  // NOTE: consteval pre-check and post-instantiation metadata
  // (template_origin, spec_key) are now handled by the engine's
  // TemplateInstantiationStep. This callback is a pure lowering operation.
  const Node* fn_def = ct_state_->find_template_def(tpl_name);
  if (!fn_def) return false;

  SpecializationKey sk;
  auto selected = registry_.select_function_specialization(
      fn_def, bindings, nttp_bindings, sk);
  if (selected.selected_pattern != fn_def) {
    lower_function(selected.selected_pattern, &mangled);
  } else {
    lowering_deferred_instantiation_ = true;
    const NttpBindings* nttp_ptr = nttp_bindings.empty() ? nullptr : &nttp_bindings;
    lower_function(fn_def, &mangled, &bindings, nttp_ptr);
    lowering_deferred_instantiation_ = false;
  }
  return true;
}

DeferredTemplateTypeResult Lowerer::instantiate_deferred_template_type(
    const PendingTemplateTypeWorkItem& work_item) {
  TypeSpec ts = work_item.pending_type;
  if (ts.deferred_member_type_name) {
    return resolve_deferred_member_typedef_type(work_item);
  }
  if (ts.tpl_struct_origin &&
      work_item.kind != PendingTemplateTypeKind::OwnerStruct) {
    DeferredTemplateTypeResult result;
    if (!ensure_pending_template_owner_ready(
            ts, work_item, true, "no primary template def",
            "delegated to owner struct work", &result)) {
      return result;
    }
    return DeferredTemplateTypeResult::resolved();
  }
  if (ts.tpl_struct_origin) {
    DeferredTemplateTypeResult result;
    if (!ensure_pending_template_owner_ready(
            ts, work_item, false,
            "no primary template def for owner",
            "owner struct still pending", &result)) {
      return result;
    }
    return DeferredTemplateTypeResult::resolved();
  }
  return DeferredTemplateTypeResult::resolved();
}

InitialHirBuildResult build_initial_hir(
    const Node* program_root,
    const sema::ResolvedTypeTable* resolved_types) {
  auto lowerer = std::make_shared<Lowerer>();
  lowerer->resolved_types_ = resolved_types;

  auto module = std::make_shared<Module>();
  lowerer->lower_initial_program(program_root, *module);

  InitialHirBuildResult result{};
  result.module = module;
  result.ct_state = lowerer->ct_state();
  result.deferred_instantiate =
      [lowerer, module](const std::string& tpl_name,
                        const TypeBindings& bindings,
                        const NttpBindings& nttp_bindings,
                        const std::string& mangled) -> bool {
    (void)module;
    return lowerer->instantiate_deferred_template(
        tpl_name, bindings, nttp_bindings, mangled);
  };
  result.deferred_instantiate_type =
      [lowerer](const PendingTemplateTypeWorkItem& work_item)
          -> DeferredTemplateTypeResult {
    return lowerer->instantiate_deferred_template_type(work_item);
  };
  return result;
}
#endif

}  // namespace c4c::hir
