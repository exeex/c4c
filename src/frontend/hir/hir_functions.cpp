// Unintegrated draft extracted from hir_lowering_core.cpp.
// This file is not yet wired into the build; it stages callable/program-level
// lowering helpers so they can be split further before hir_lowering_core.cpp is cleaned up.
//
// Intentionally omitted for now because they are tightly coupled to other
// clusters that will likely move separately:
// - lower_expr, lower_stmt_node, lower_call_expr, lower_stmt_expr_block
// - lower_struct_method, lower_global, lower_local_decl_stmt
// - initializer normalization helpers
// - template collection / template-struct realization helpers

#include "impl/hir_impl.hpp"
#include "impl/lowerer.hpp"
#include "consteval.hpp"
#include "type_utils.hpp"
#include "../parser/parser_support.hpp"

#include <algorithm>
#include <stdexcept>

namespace c4c::hir {

namespace {

void ensure_function_slot(Module* module, FunctionId id) {
  if (!module) return;
  if (module->functions.size() <= id.value) {
    const size_t old_size = module->functions.size();
    module->functions.resize(static_cast<size_t>(id.value) + 1);
    for (size_t i = old_size; i < module->functions.size(); ++i) {
      module->functions[i].id = FunctionId::invalid();
    }
  }
}

bool normalize_zero_sized_struct_return_from_body(Module* module, Function* fn) {
  if (!module || !fn) return false;
  const TypeSpec ret_ts = fn->return_type.spec;
  if (ret_ts.base != TB_STRUCT || !ret_ts.tag || !ret_ts.tag[0] ||
      ret_ts.ptr_level != 0 || ret_ts.array_rank != 0 ||
      ret_ts.is_lvalue_ref || ret_ts.is_rvalue_ref) {
    return false;
  }

  auto sit = module->struct_defs.find(ret_ts.tag);
  if (sit == module->struct_defs.end() || sit->second.size_bytes != 0) {
    return false;
  }

  std::optional<TypeSpec> inferred;
  for (const auto& block : fn->blocks) {
    for (const auto& stmt : block.stmts) {
      const auto* ret = std::get_if<ReturnStmt>(&stmt.payload);
      if (!ret || !ret->expr) continue;
      ExprId expr_id = *ret->expr;
      auto eit = std::find_if(
          module->expr_pool.begin(),
          module->expr_pool.end(),
          [&](const Expr& expr) { return expr.id.value == expr_id.value; });
      if (eit == module->expr_pool.end()) continue;

      const TypeSpec expr_ts = eit->type.spec;
      if (expr_ts.base == TB_VOID || expr_ts.ptr_level > 0 ||
          expr_ts.array_rank > 0 || expr_ts.is_lvalue_ref ||
          expr_ts.is_rvalue_ref || expr_ts.base == TB_STRUCT ||
          expr_ts.base == TB_UNION) {
        return false;
      }

      if (!inferred) {
        inferred = expr_ts;
        continue;
      }

      std::unordered_map<std::string, TypeSpec> empty_typedefs;
      if (!types_compatible_p(*inferred, expr_ts, empty_typedefs)) {
        return false;
      }
    }
  }

  if (!inferred) return false;
  fn->return_type.spec = *inferred;
  fn->return_type.category = ValueCategory::RValue;
  return true;
}

}  // namespace

void Lowerer::register_bodyless_callable(Function&& fn) {
  module_->index_function_decl(fn);
  ensure_function_slot(module_, fn.id);
  module_->functions[fn.id.value] = std::move(fn);
}

bool Lowerer::maybe_register_bodyless_callable(Function* fn,
                                               bool has_lowerable_body) {
  if (has_lowerable_body) return false;
  register_bodyless_callable(std::move(*fn));
  return true;
}

BlockId Lowerer::begin_callable_body_lowering(Function& fn, FunctionCtx& ctx) {
  module_->index_function_decl(fn);
  ensure_function_slot(module_, fn.id);
  // Reserve a skeleton; callers replace it after body lowering completes.
  Function skeleton{};
  skeleton.id = fn.id;
  skeleton.name = fn.name;
  skeleton.name_text_id = fn.name_text_id;
  skeleton.link_name_id = fn.link_name_id;
  skeleton.execution_domain = fn.execution_domain;
  skeleton.ns_qual = fn.ns_qual;
  skeleton.return_type = fn.return_type;
  module_->functions[fn.id.value] = std::move(skeleton);

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

void Lowerer::lower_function(const Node* fn_node,
                             const std::string* name_override,
                             const TypeBindings* tpl_override,
                             const NttpBindings* nttp_override) {
  Function fn{};
  fn.id = next_fn_id();
  fn.name = name_override ? *name_override
                          : (fn_node->name ? fn_node->name : "<anon_fn>");
  fn.name_text_id = make_unqualified_text_id(
      fn_node, module_ ? module_->link_name_texts.get() : nullptr);
  fn.link_name_id = module_->link_names.intern(fn.name);
  fn.ns_qual = make_ns_qual(fn_node, module_ ? module_->link_name_texts.get() : nullptr);
  {
    TypeSpec ret_ts = fn_node->type;
    if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
        !ret_ts.is_fn_ptr) {
      ret_ts.is_fn_ptr = true;
      ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
    }
    ret_ts = prepare_callable_return_type(
        ret_ts, tpl_override, nttp_override, fn_node,
        std::string("function-return:") + fn.name, false);
    fn.return_type = qtype_from(ret_ts);
  }
  if (fn_node->type.is_fn_ptr ||
      fn_node->n_ret_fn_ptr_params > 0 ||
      fn_node->ret_fn_ptr_variadic) {
    auto fn_ct = sema::canonicalize_declarator_type(fn_node);
    const auto* fn_fsig = sema::get_function_sig(fn_ct);
    if (fn_fsig && fn_fsig->return_type &&
        sema::is_callable_type(*fn_fsig->return_type)) {
      const auto* ret_fsig = sema::get_function_sig(*fn_fsig->return_type);
      if (ret_fsig) {
        FnPtrSig ret_sig{};
        ret_sig.canonical_sig = fn_fsig->return_type;
        if (ret_fsig->return_type) {
          TypeSpec ret_ts = sema::typespec_from_canonical(*ret_fsig->return_type);
          if ((fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic) &&
              !ret_ts.is_fn_ptr) {
            ret_ts.is_fn_ptr = true;
            ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
          }
          ret_sig.return_type = qtype_from(ret_ts);
        }
        ret_sig.variadic = ret_fsig->is_variadic;
        ret_sig.unspecified_params = ret_fsig->unspecified_params;
        for (const auto& param : ret_fsig->params) {
          ret_sig.params.push_back(
              qtype_from(sema::typespec_from_canonical(param),
                         ValueCategory::LValue));
        }
        if (ret_sig.params.empty() &&
            (fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic)) {
          ret_sig.variadic = fn_node->ret_fn_ptr_variadic;
          ret_sig.unspecified_params = false;
          for (int i = 0; i < fn_node->n_ret_fn_ptr_params; ++i) {
            const Node* param = fn_node->ret_fn_ptr_params[i];
            ret_sig.params.push_back(
                qtype_from(param->type, ValueCategory::LValue));
          }
        }
        fn.ret_fn_ptr_sig = std::move(ret_sig);
      }
    }
    if (!fn.ret_fn_ptr_sig &&
        (fn_node->n_ret_fn_ptr_params > 0 || fn_node->ret_fn_ptr_variadic)) {
      FnPtrSig ret_sig{};
      ret_sig.return_type = qtype_from(fn.return_type.spec);
      ret_sig.variadic = fn_node->ret_fn_ptr_variadic;
      ret_sig.unspecified_params = false;
      for (int i = 0; i < fn_node->n_ret_fn_ptr_params; ++i) {
        const Node* param = fn_node->ret_fn_ptr_params[i];
        ret_sig.params.push_back(qtype_from(param->type, ValueCategory::LValue));
      }
      fn.ret_fn_ptr_sig = std::move(ret_sig);
    }
  }
  fn.linkage = {fn_node->is_static,
                fn_node->is_extern || fn_node->body == nullptr,
                fn_node->is_inline,
                weak_symbols_.count(fn.name) > 0,
                static_cast<Visibility>(fn_node->visibility)};
  fn.execution_domain = fn_node->execution_domain;
  fn.attrs.variadic = fn_node->variadic;
  fn.attrs.no_inline = fn_node->type.is_noinline;
  fn.attrs.always_inline = fn_node->type.is_always_inline;
  if (fn_node->type.align_bytes > 0) {
    fn.attrs.align_bytes = fn_node->type.align_bytes;
  }
  if (fn.attrs.align_bytes == 0) {
    DeclRef prev_ref{};
    prev_ref.name = fn.name;
    prev_ref.name_text_id = fn.name_text_id;
    prev_ref.link_name_id = fn.link_name_id;
    prev_ref.ns_qual = fn.ns_qual;
    if (const Function* prev_fn = module_->resolve_function_decl(prev_ref)) {
      int prev_align = prev_fn->attrs.align_bytes;
      if (prev_align > 0) {
        fn.attrs.align_bytes = prev_align;
      }
    }
  }
  fn.span = make_span(fn_node);

  FunctionCtx ctx{};
  ctx.fn = &fn;
  if (tpl_override) {
    ctx.tpl_bindings = *tpl_override;
  }
  if (nttp_override) {
    ctx.nttp_bindings = *nttp_override;
  }
  append_callable_params(
      fn, ctx, fn_node, tpl_override, nttp_override, "function-param:", false,
      true);

  if (maybe_register_bodyless_callable(&fn, fn_node->body != nullptr)) {
    return;
  }

  const BlockId entry = begin_callable_body_lowering(fn, ctx);

  for (int i = 0; i < fn_node->n_params; ++i) {
    const Node* p = fn_node->params[i];
    if (!p) continue;
    if (p->type.array_rank <= 0 || !p->type.array_size_expr) continue;
    if (p->type.array_size > 0 && p->type.array_dims[0] > 0) continue;
    ExprStmt side_effect{};
    side_effect.expr = lower_expr(&ctx, p->type.array_size_expr);
    append_stmt(ctx, Stmt{StmtPayload{side_effect}, make_span(p)});
  }

  lower_stmt_node(ctx, fn_node->body);
  normalize_zero_sized_struct_return_from_body(module_, &fn);
  finish_lowered_callable(&fn, entry);
}

TypeSpec Lowerer::substitute_signature_template_type(
    TypeSpec ts, const TypeBindings* tpl_bindings) {
  if (ts.base != TB_TYPEDEF || !ts.tag) return ts;
  const int outer_ptr_level = ts.ptr_level;
  const bool outer_lref = ts.is_lvalue_ref;
  const bool outer_rref = ts.is_rvalue_ref;
  const bool outer_const = ts.is_const;
  const bool outer_volatile = ts.is_volatile;

  auto apply_concrete = [&](const TypeSpec& concrete) {
    ts = concrete;
    ts.ptr_level += outer_ptr_level;
    ts.is_lvalue_ref = concrete.is_lvalue_ref || outer_lref;
    ts.is_rvalue_ref =
        !ts.is_lvalue_ref && (concrete.is_rvalue_ref || outer_rref);
    ts.is_const = concrete.is_const || outer_const;
    ts.is_volatile = concrete.is_volatile || outer_volatile;
  };

  if (tpl_bindings) {
    auto it = tpl_bindings->find(ts.tag);
    if (it != tpl_bindings->end()) {
      apply_concrete(it->second);
      return ts;
    }
  }

  const std::string qualified_name = ts.tag;
  const size_t split = qualified_name.rfind("::");
  if (split == std::string::npos) return ts;

  TypeSpec resolved{};
  if (!resolve_struct_member_typedef_type(
          qualified_name.substr(0, split), qualified_name.substr(split + 2),
          &resolved)) {
    return ts;
  }

  apply_concrete(resolved);
  if (tpl_bindings && ts.base == TB_TYPEDEF && ts.tag)
    return substitute_signature_template_type(ts, tpl_bindings);
  return ts;
}

void Lowerer::resolve_signature_template_type_if_needed(
    TypeSpec& ts,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const std::string* current_struct_tag,
    const Node* span_node,
    const std::string& context_name) {
  if (!tpl_bindings) return;
  if (!ts.tpl_struct_origin) {
    recover_template_struct_identity_from_tag(&ts, current_struct_tag);
  }
  if (!ts.tpl_struct_origin) return;
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
      ret_ts, tpl_bindings, nttp_bindings, nullptr, span_node, context_name);
  while (resolve_struct_member_typedef_if_ready(&ret_ts)) {
  }
  if (ret_ts.deferred_member_type_name &&
      ((ret_ts.tpl_struct_origin && ret_ts.tpl_struct_origin[0]) ||
       (ret_ts.tag && ret_ts.tag[0]))) {
    seed_pending_template_type(
        ret_ts,
        tpl_bindings ? *tpl_bindings : TypeBindings{},
        nttp_bindings ? *nttp_bindings : NttpBindings{},
        span_node,
        PendingTemplateTypeKind::MemberTypedef,
        context_name);
    while (resolve_struct_member_typedef_if_ready(&ret_ts)) {
    }
  }
  if (!ret_ts.deferred_member_type_name && ret_ts.ptr_level == 0 &&
      ret_ts.array_rank == 0 && !ret_ts.is_lvalue_ref &&
      !ret_ts.is_rvalue_ref && ret_ts.base == TB_STRUCT && ret_ts.tag &&
      ret_ts.tag[0]) {
    TypeSpec resolved_member{};
    if (resolve_struct_member_typedef_type(ret_ts.tag, "type",
                                           &resolved_member)) {
      while (resolve_struct_member_typedef_if_ready(&resolved_member)) {
      }
      resolve_typedef_to_struct(resolved_member);
      ret_ts = resolved_member;
    }
  }
  if (resolve_typedef_struct) resolve_typedef_to_struct(ret_ts);
  resolve_signature_template_type_if_needed(
      ret_ts, tpl_bindings, nttp_bindings, nullptr, span_node, context_name);
  while (resolve_struct_member_typedef_if_ready(&ret_ts)) {
  }
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
  const std::string* current_struct_tag =
      ctx.method_struct_tag.empty() ? nullptr : &ctx.method_struct_tag;
  resolve_signature_template_type_if_needed(
      param_ts, tpl_bindings, nttp_bindings, current_struct_tag, param_node, context_name);
  while (resolve_struct_member_typedef_if_ready(&param_ts)) {
  }
  if (param_ts.deferred_member_type_name &&
      ((param_ts.tpl_struct_origin && param_ts.tpl_struct_origin[0]) ||
       (param_ts.tag && param_ts.tag[0]))) {
    seed_pending_template_type(
        param_ts,
        tpl_bindings ? *tpl_bindings : TypeBindings{},
        nttp_bindings ? *nttp_bindings : NttpBindings{},
        param_node,
        PendingTemplateTypeKind::MemberTypedef,
        context_name);
    while (resolve_struct_member_typedef_if_ready(&param_ts)) {
    }
  }
  if (!param_ts.deferred_member_type_name && param_ts.ptr_level == 0 &&
      param_ts.array_rank == 0 && !param_ts.is_lvalue_ref &&
      !param_ts.is_rvalue_ref && param_ts.base == TB_STRUCT && param_ts.tag &&
      param_ts.tag[0]) {
    TypeSpec resolved_member{};
    if (resolve_struct_member_typedef_type(param_ts.tag, "type",
                                           &resolved_member)) {
      while (resolve_struct_member_typedef_if_ready(&resolved_member)) {
      }
      resolve_typedef_to_struct(resolved_member);
      param_ts = resolved_member;
    }
  }
  if (resolve_typedef_struct) resolve_typedef_to_struct(param_ts);
  resolve_signature_template_type_if_needed(
      param_ts, tpl_bindings, nttp_bindings, current_struct_tag, param_node, context_name);
  while (resolve_struct_member_typedef_if_ready(&param_ts)) {
  }
  if (resolve_typedef_struct) resolve_typedef_to_struct(param_ts);

  Param param{};
  param.name = emitted_name;
  param.name_text_id = make_text_id(
      param.name, module_ ? module_->link_name_texts.get() : nullptr);
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
