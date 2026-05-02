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

std::string text_id_spelling(TextId text_id, const TextTable* texts) {
  if (text_id == kInvalidText || !texts) return {};
  return std::string(texts->lookup(text_id));
}

std::string template_param_binding_key_from_text(const TypeSpec& ts,
                                                 const TextTable* texts) {
  return text_id_spelling(ts.template_param_text_id, texts);
}

std::string legacy_template_binding_name_without_usable_text_spelling(
    const TypeSpec& ts,
    const TextTable* texts) {
  if (!text_id_spelling(ts.template_param_text_id, texts).empty()) return {};
  // No-usable-text-metadata compatibility bridge for legacy template binding
  // maps have no structured carrier once rendered tag spelling is unavailable.
  return {};
}

std::string legacy_type_name_without_complete_text_metadata(const TypeSpec& ts) {
  if (ts.tag_text_id != kInvalidText) return {};
  return {};
}

std::optional<std::string> unique_non_pack_template_binding_name(
    const TypeBindings& bindings) {
  std::optional<std::string> out;
  for (const auto& [key, _] : bindings) {
    if (key.find('#') != std::string::npos) continue;
    if (out && *out != key) return std::nullopt;
    out = key;
  }
  return out;
}

std::optional<std::string> unique_pack_template_binding_base(
    const TypeBindings& bindings) {
  std::optional<std::string> out;
  for (const auto& [key, _] : bindings) {
    const size_t split = key.rfind('#');
    if (split == std::string::npos || split + 1 == key.size()) continue;
    bool numeric_suffix = true;
    for (size_t i = split + 1; i < key.size(); ++i) {
      if (key[i] < '0' || key[i] > '9') {
        numeric_suffix = false;
        break;
      }
    }
    if (!numeric_suffix) continue;
    const std::string base = key.substr(0, split);
    if (out && *out != base) return std::nullopt;
    out = base;
  }
  return out;
}

bool has_member_typedef_owner_metadata_or_legacy_name(const TypeSpec& ts) {
  return ts.record_def || (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) ||
         ts.tag_text_id != kInvalidText;
}

void apply_signature_template_concrete(TypeSpec& target,
                                       const TypeSpec& concrete) {
  const int outer_ptr_level = target.ptr_level;
  const bool outer_lref = target.is_lvalue_ref;
  const bool outer_rref = target.is_rvalue_ref;
  const bool outer_const = target.is_const;
  const bool outer_volatile = target.is_volatile;

  target = concrete;
  target.ptr_level += outer_ptr_level;
  target.is_lvalue_ref = concrete.is_lvalue_ref || outer_lref;
  target.is_rvalue_ref =
      !target.is_lvalue_ref && (concrete.is_rvalue_ref || outer_rref);
  target.is_const = concrete.is_const || outer_const;
  target.is_volatile = concrete.is_volatile || outer_volatile;
}

bool apply_signature_template_binding_by_text(
    TypeSpec& ts,
    const std::unordered_map<TextId, TypeSpec>* tpl_bindings_by_text) {
  if (!tpl_bindings_by_text ||
      ts.template_param_text_id == kInvalidText) {
    return false;
  }
  auto text_it = tpl_bindings_by_text->find(ts.template_param_text_id);
  if (text_it == tpl_bindings_by_text->end()) return false;
  apply_signature_template_concrete(ts, text_it->second);
  return true;
}

bool apply_signature_template_binding_by_text_spelling(
    TypeSpec& ts,
    const TypeBindings* tpl_bindings,
    const TextTable* texts) {
  if (!tpl_bindings || ts.template_param_text_id == kInvalidText) return false;
  const std::string key = template_param_binding_key_from_text(ts, texts);
  if (key.empty()) return false;
  auto it = tpl_bindings->find(key);
  if (it == tpl_bindings->end()) return false;
  apply_signature_template_concrete(ts, it->second);
  return true;
}

bool apply_legacy_template_binding_without_usable_text_spelling(
    TypeSpec& ts,
    const TypeBindings* tpl_bindings,
    const TextTable* texts) {
  if (!tpl_bindings) return false;
  const std::string key =
      legacy_template_binding_name_without_usable_text_spelling(ts, texts);
  if (key.empty()) {
    if (ts.template_param_text_id == kInvalidText) return false;
    const auto unique_key = unique_non_pack_template_binding_name(*tpl_bindings);
    if (!unique_key) return false;
    auto unique_it = tpl_bindings->find(*unique_key);
    if (unique_it == tpl_bindings->end()) return false;
    apply_signature_template_concrete(ts, unique_it->second);
    return true;
  }
  auto it = tpl_bindings->find(key);
  if (it == tpl_bindings->end()) return false;
  apply_signature_template_concrete(ts, it->second);
  return true;
}

std::string qualified_type_spelling_from_text_ids(const TypeSpec& ts,
                                                  const TextTable* texts) {
  if (!texts || ts.tag_text_id == kInvalidText) return {};
  std::string out;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    for (int i = 0; i < ts.n_qualifier_segments; ++i) {
      const std::string segment =
          text_id_spelling(ts.qualifier_text_ids[i], texts);
      if (segment.empty()) return {};
      if (!out.empty()) out += "::";
      out += segment;
    }
  }
  const std::string name = text_id_spelling(ts.tag_text_id, texts);
  if (name.empty()) return {};
  if (!out.empty()) out += "::";
  out += name;
  return out;
}

std::string member_typedef_compatibility_name(const TypeSpec& ts,
                                              const TextTable* texts) {
  std::string name = qualified_type_spelling_from_text_ids(ts, texts);
  if (!name.empty()) return name;
  // No-complete-text-metadata compatibility bridge for legacy TypeSpec
  // producers. Structured TextId-qualified spelling remains authoritative.
  return legacy_type_name_without_complete_text_metadata(ts);
}

std::optional<HirRecordOwnerKey> record_owner_key_from_type_metadata(
    const TypeSpec& ts,
    const TextTable* texts) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    const TextId record_text_id = make_unqualified_text_id(ts.record_def,
                                                           const_cast<TextTable*>(texts));
    if (record_text_id != kInvalidText) {
      return make_hir_record_owner_key(
          make_ns_qual(ts.record_def, const_cast<TextTable*>(texts)),
          record_text_id);
    }
  }
  if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
    return std::nullopt;
  }

  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  return make_hir_record_owner_key(ns_qual, ts.tag_text_id);
}

const HirStructDef* find_struct_def_for_callable_type(Module* module,
                                                       const TypeSpec& ts) {
  if (!module) return nullptr;
  if (auto owner_key = record_owner_key_from_type_metadata(
          ts, module->link_name_texts.get())) {
    if (hir_record_owner_key_has_complete_metadata(*owner_key)) {
      if (const HirStructDef* structured =
              module->find_struct_def_by_owner_structured(*owner_key)) {
        return structured;
      }
      return nullptr;
    }
  }
  // No-complete-owner-metadata compatibility bridge for legacy TypeSpec
  // producers used by the staged callable zero-sized return normalization.
  const std::string tag = legacy_type_name_without_complete_text_metadata(ts);
  if (tag.empty()) return nullptr;
  auto sit = module->struct_defs.find(tag);
  return sit == module->struct_defs.end() ? nullptr : &sit->second;
}

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
  if (ret_ts.base != TB_STRUCT || ret_ts.ptr_level != 0 || ret_ts.array_rank != 0 ||
      ret_ts.is_lvalue_ref || ret_ts.is_rvalue_ref) {
    return false;
  }

  const HirStructDef* ret_def = find_struct_def_for_callable_type(module, ret_ts);
  if (!ret_def || ret_def->size_bytes != 0) {
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

void Lowerer::populate_template_type_text_bindings(
    FunctionCtx& ctx, const Node* template_owner,
    const TypeBindings* bindings) const {
  ctx.tpl_bindings_by_text.clear();
  if (!template_owner || !bindings || bindings->empty() ||
      template_owner->n_template_params <= 0 ||
      !template_owner->template_param_names) {
    return;
  }
  for (int i = 0; i < template_owner->n_template_params; ++i) {
    if (!template_owner->template_param_names[i]) continue;
    if (template_owner->template_param_is_nttp &&
        template_owner->template_param_is_nttp[i]) {
      continue;
    }
    const auto it = bindings->find(template_owner->template_param_names[i]);
    if (it == bindings->end()) continue;
    TextId param_text_id = kInvalidText;
    if (template_owner->template_param_name_text_ids) {
      param_text_id = template_owner->template_param_name_text_ids[i];
    }
    if (param_text_id == kInvalidText && module_ && module_->link_name_texts) {
      param_text_id =
          module_->link_name_texts->intern(template_owner->template_param_names[i]);
    }
    if (param_text_id == kInvalidText) continue;
    ctx.tpl_bindings_by_text[param_text_id] = it->second;
  }
}

void Lowerer::lower_function(const Node* fn_node,
                             const std::string* name_override,
                             const TypeBindings* tpl_override,
                             const NttpBindings* nttp_override,
                             const NttpTextBindings* nttp_text_override) {
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
  populate_template_type_text_bindings(ctx, fn_node, tpl_override);
  if (nttp_override) {
    ctx.nttp_bindings = *nttp_override;
  }
  if (nttp_text_override) {
    ctx.nttp_bindings_by_text = *nttp_text_override;
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
  if (ts.base != TB_TYPEDEF) return ts;

  auto substitute_template_owner_params = [&](TypeSpec& target,
                                              const Node* owner_tpl,
                                              const auto& self) -> void {
    if (!owner_tpl || !tpl_bindings) return;
    if (target.base == TB_TYPEDEF) {
      TextId carrier_text_id = target.template_param_text_id;
      if (carrier_text_id == kInvalidText) carrier_text_id = target.tag_text_id;
      int param_index = -1;
      if (carrier_text_id != kInvalidText &&
          owner_tpl->template_param_name_text_ids) {
        for (int i = 0; i < owner_tpl->n_template_params; ++i) {
          if (owner_tpl->template_param_is_nttp &&
              owner_tpl->template_param_is_nttp[i]) {
            continue;
          }
          if (owner_tpl->template_param_name_text_ids[i] == carrier_text_id) {
            param_index = i;
            break;
          }
        }
      }
      if (param_index >= 0 &&
          owner_tpl->template_param_names &&
          owner_tpl->template_param_names[param_index]) {
        auto binding_it =
            tpl_bindings->find(owner_tpl->template_param_names[param_index]);
        if (binding_it != tpl_bindings->end()) {
          apply_signature_template_concrete(target, binding_it->second);
        }
      }
    }
    if (target.tpl_struct_args.data && target.tpl_struct_args.size > 0) {
      for (int i = 0; i < target.tpl_struct_args.size; ++i) {
        if (target.tpl_struct_args.data[i].kind == TemplateArgKind::Type) {
          self(target.tpl_struct_args.data[i].type, owner_tpl, self);
        }
      }
    }
  };

  auto resolve_qualified_member_typedef_by_text_id = [&]() -> std::optional<TypeSpec> {
    if (!tpl_bindings || !ts.qualifier_text_ids ||
        ts.n_qualifier_segments != 1 ||
        ts.qualifier_text_ids[0] == kInvalidText ||
        ts.tag_text_id == kInvalidText) {
      return std::nullopt;
    }
    const Node* owner_tpl = nullptr;
    for (const auto& [_, candidate] : template_struct_defs_) {
      if (!candidate || candidate->unqualified_text_id != ts.qualifier_text_ids[0]) {
        continue;
      }
      if (owner_tpl && owner_tpl != candidate) return std::nullopt;
      owner_tpl = candidate;
    }
    if (!owner_tpl || owner_tpl->n_member_typedefs <= 0 ||
        !owner_tpl->member_typedef_text_ids || !owner_tpl->member_typedef_types) {
      return std::nullopt;
    }
    int member_index = -1;
    for (int i = 0; i < owner_tpl->n_member_typedefs; ++i) {
      if (owner_tpl->member_typedef_text_ids[i] == ts.tag_text_id) {
        member_index = i;
        break;
      }
    }
    if (member_index < 0) return std::nullopt;

    TypeSpec resolved = owner_tpl->member_typedef_types[member_index];
    substitute_template_owner_params(resolved, owner_tpl,
                                     substitute_template_owner_params);
    if (resolved.base == TB_TYPEDEF) {
      resolved = substitute_signature_template_type(resolved, tpl_bindings);
    }
    while (resolve_struct_member_typedef_if_ready(&resolved)) {
      substitute_template_owner_params(resolved, owner_tpl,
                                       substitute_template_owner_params);
      if (resolved.base == TB_TYPEDEF) {
        resolved = substitute_signature_template_type(resolved, tpl_bindings);
      }
    }
    return resolved;
  };

  if (ts.template_param_text_id != kInvalidText) {
    if (apply_signature_template_binding_by_text_spelling(
            ts, tpl_bindings, module_ ? module_->link_name_texts.get() : nullptr)) {
      return ts;
    }
  }

  if (apply_legacy_template_binding_without_usable_text_spelling(
          ts, tpl_bindings, module_ ? module_->link_name_texts.get() : nullptr)) {
    return ts;
  }

  if (auto resolved_member = resolve_qualified_member_typedef_by_text_id()) {
    return *resolved_member;
  }

  const std::string qualified_name = member_typedef_compatibility_name(
      ts, module_ ? module_->link_name_texts.get() : nullptr);
  const size_t split = qualified_name.rfind("::");
  if (split == std::string::npos) return ts;

  TypeSpec resolved{};
  if (!resolve_struct_member_typedef_type(
          qualified_name.substr(0, split), qualified_name.substr(split + 2),
          &resolved)) {
    return ts;
  }

  apply_signature_template_concrete(ts, resolved);
  if (tpl_bindings && ts.base == TB_TYPEDEF)
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
      has_member_typedef_owner_metadata_or_legacy_name(ret_ts)) {
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
      !ret_ts.is_rvalue_ref && ret_ts.base == TB_STRUCT) {
    TypeSpec resolved_member{};
    TypeSpec owner_ts = ret_ts;
    owner_ts.deferred_member_type_name = "type";
    owner_ts.deferred_member_type_text_id = make_text_id(
        "type", module_ ? module_->link_name_texts.get() : nullptr);
    bool resolved_type_member = false;
    if (owner_ts.record_def ||
        (owner_ts.tpl_struct_origin && owner_ts.tpl_struct_origin[0])) {
      resolved_type_member =
          resolve_struct_member_typedef_if_ready(&owner_ts);
      if (resolved_type_member) resolved_member = owner_ts;
    } else {
      const std::string owner_name = member_typedef_compatibility_name(
          owner_ts, module_ ? module_->link_name_texts.get() : nullptr);
      if (!owner_name.empty()) {
        resolved_type_member =
            resolve_struct_member_typedef_type(owner_name, "type",
                                               &resolved_member);
      }
    }
    if (resolved_type_member) {
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
  if (!(param_ts.base == TB_TYPEDEF &&
        param_ts.template_param_text_id != kInvalidText &&
        apply_signature_template_binding_by_text(
            param_ts, &ctx.tpl_bindings_by_text))) {
    param_ts = substitute_signature_template_type(param_ts, tpl_bindings);
  }
  const std::string* current_struct_tag =
      ctx.method_struct_tag.empty() ? nullptr : &ctx.method_struct_tag;
  resolve_signature_template_type_if_needed(
      param_ts, tpl_bindings, nttp_bindings, current_struct_tag, param_node, context_name);
  while (resolve_struct_member_typedef_if_ready(&param_ts)) {
  }
  if (param_ts.deferred_member_type_name &&
      has_member_typedef_owner_metadata_or_legacy_name(param_ts)) {
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
      !param_ts.is_rvalue_ref && param_ts.base == TB_STRUCT) {
    TypeSpec resolved_member{};
    TypeSpec owner_ts = param_ts;
    owner_ts.deferred_member_type_name = "type";
    owner_ts.deferred_member_type_text_id = make_text_id(
        "type", module_ ? module_->link_name_texts.get() : nullptr);
    bool resolved_type_member = false;
    if (owner_ts.record_def ||
        (owner_ts.tpl_struct_origin && owner_ts.tpl_struct_origin[0])) {
      resolved_type_member =
          resolve_struct_member_typedef_if_ready(&owner_ts);
      if (resolved_type_member) resolved_member = owner_ts;
    } else {
      const std::string owner_name = member_typedef_compatibility_name(
          owner_ts, module_ ? module_->link_name_texts.get() : nullptr);
      if (!owner_name.empty()) {
        resolved_type_member =
            resolve_struct_member_typedef_type(owner_name, "type",
                                               &resolved_member);
      }
    }
    if (resolved_type_member) {
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

    std::string pack_binding_base;
    if (p->type.template_param_text_id != kInvalidText) {
      pack_binding_base = template_param_binding_key_from_text(
          p->type, module_ ? module_->link_name_texts.get() : nullptr);
    }
    if (pack_binding_base.empty()) {
      pack_binding_base = legacy_template_binding_name_without_usable_text_spelling(
          p->type, module_ ? module_->link_name_texts.get() : nullptr);
    }
    if (pack_binding_base.empty() && tpl_bindings &&
        p->type.template_param_text_id != kInvalidText) {
      if (auto unique_pack_base = unique_pack_template_binding_base(*tpl_bindings)) {
        pack_binding_base = *unique_pack_base;
      }
    }
    if (expand_parameter_packs && p->is_parameter_pack && tpl_bindings &&
        p->type.base == TB_TYPEDEF && !pack_binding_base.empty()) {
      std::vector<std::pair<int, TypeSpec>> pack_types;
      for (const auto& [key, ts] : *tpl_bindings) {
        int pack_index = 0;
        if (parse_pack_binding_name(key, pack_binding_base, &pack_index)) {
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
