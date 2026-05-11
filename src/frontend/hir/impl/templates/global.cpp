#include "templates.hpp"

namespace c4c::hir {

namespace {

void apply_template_global_arg_binding(TypeSpec& target, const TypeSpec& binding) {
  const int outer_ptr_level = target.ptr_level;
  const bool outer_lref = target.is_lvalue_ref;
  const bool outer_rref = target.is_rvalue_ref;
  const bool outer_const = target.is_const;
  const bool outer_volatile = target.is_volatile;

  target = binding;
  target.ptr_level += outer_ptr_level;
  target.is_lvalue_ref = target.is_lvalue_ref || outer_lref;
  target.is_rvalue_ref =
      !target.is_lvalue_ref && (target.is_rvalue_ref || outer_rref);
  target.is_const = target.is_const || outer_const;
  target.is_volatile = target.is_volatile || outer_volatile;
}

bool resolve_template_global_record_member_typedef(const Node* record_def,
                                                   const char* member,
                                                   TypeSpec* out) {
  if (!record_def || record_def->kind != NK_STRUCT_DEF || !member || !member[0] ||
      !out || !record_def->member_typedef_names ||
      !record_def->member_typedef_types) {
    return false;
  }
  for (int i = 0; i < record_def->n_member_typedefs; ++i) {
    const char* alias_name = record_def->member_typedef_names[i];
    if (!alias_name || std::string(alias_name) != member) continue;
    *out = record_def->member_typedef_types[i];
    return true;
  }
  return false;
}

TypeSpec type_binding_lookup_carrier_with_valid_owner_text(
    TypeSpec ts,
    const Module* module) {
  if (ts.template_param_owner_text_id == kInvalidText || !module ||
      !module->link_name_texts) {
    return ts;
  }
  if (!module->link_name_texts->lookup(ts.template_param_owner_text_id).empty()) {
    return ts;
  }
  ts.template_param_owner_text_id = kInvalidText;
  ts.template_param_owner_namespace_context_id = -1;
  return ts;
}

}  // namespace

void Lowerer::collect_template_global_definitions(
    const std::vector<const Node*>& items) {
  template_global_defs_.clear();
  template_global_specializations_.clear();

  for (const Node* item : items) {
    if (!item || item->kind != NK_GLOBAL_VAR || !item->name || !item->name[0]) continue;
    if (item->n_template_params > 0 && item->n_template_args == 0) {
      template_global_defs_[item->name] = item;
      continue;
    }
    if (item->n_template_args > 0) {
      template_global_specializations_[item->name].push_back(item);
    }
  }
}

const Node* Lowerer::find_template_global_primary(const std::string& name) const {
  auto it = template_global_defs_.find(name);
  return it != template_global_defs_.end() ? it->second : nullptr;
}

const std::vector<const Node*>* Lowerer::find_template_global_specializations(
    const Node* primary_tpl) const {
  if (!primary_tpl || !primary_tpl->name) return nullptr;
  auto it = template_global_specializations_.find(primary_tpl->name);
  return it != template_global_specializations_.end() ? &it->second : nullptr;
}

std::optional<GlobalId> Lowerer::ensure_template_global_instance(
    FunctionCtx* ctx,
    const Node* ref) {
  if (!ref || ref->kind != NK_VAR || !ref->name || !ref->name[0] ||
      !(ref->n_template_args > 0 || ref->has_template_args)) {
    return std::nullopt;
  }

  const Node* primary = find_template_global_primary(ref->name);
  if (!primary) return std::nullopt;

  std::vector<HirTemplateArg> actual_args;
  actual_args.reserve(ref->n_template_args);
  auto recover_unique_realized_template_arg_carrier = [&](TypeSpec* ts) {
    if (!ts || ts->tag_text_id == kInvalidText ||
        (ts->tpl_struct_origin && ts->tpl_struct_origin[0]) ||
        (ts->record_def && ts->record_def->kind == NK_STRUCT_DEF)) {
      return;
    }
    const Node* unique_instance = nullptr;
    for (const auto& [_, candidate] : struct_def_nodes_) {
      if (!candidate || candidate->kind != NK_STRUCT_DEF ||
          !candidate->template_origin_name ||
          !candidate->template_origin_name[0] ||
          candidate->n_template_args <= 0) {
        continue;
      }
      const Node* primary_tpl =
          find_template_struct_primary(candidate->template_origin_name);
      if (!primary_tpl ||
          primary_tpl->unqualified_text_id != ts->tag_text_id) {
        continue;
      }
      if (ts->namespace_context_id >= 0 &&
          primary_tpl->namespace_context_id != ts->namespace_context_id) {
        continue;
      }
      if (unique_instance && unique_instance != candidate) {
        unique_instance = nullptr;
        break;
      }
      unique_instance = candidate;
    }
    if (!unique_instance) return;
    ts->base = TB_STRUCT;
    ts->record_def = const_cast<Node*>(unique_instance);
    ts->tag_text_id = unique_instance->unqualified_text_id;
    ts->namespace_context_id = unique_instance->namespace_context_id;
    ts->is_global_qualified = false;
  };
  auto recover_unique_derived_template_arg_carrier = [&](TypeSpec* ts) {
    if (!ts || !module_) return;
    std::string base_tag;
    if (ts->record_def && ts->record_def->name && ts->record_def->name[0]) {
      base_tag = ts->record_def->name;
    } else if (ts->tag_text_id != kInvalidText) {
      NamespaceQualifier ns_qual;
      ns_qual.context_id = ts->namespace_context_id;
      ns_qual.is_global_qualified = ts->is_global_qualified;
      if (const SymbolName* tag = module_->find_struct_def_tag_by_owner(
              make_hir_record_owner_key(ns_qual, ts->tag_text_id))) {
        base_tag = *tag;
      }
    }
    if (base_tag.empty()) return;
    const Node* unique_derived = nullptr;
    for (const auto& [derived_tag, def] : module_->struct_defs) {
      if (std::find(def.base_tags.begin(), def.base_tags.end(), base_tag) ==
          def.base_tags.end()) {
        continue;
      }
      auto node_it = struct_def_nodes_.find(derived_tag);
      if (node_it == struct_def_nodes_.end() || !node_it->second ||
          !node_it->second->template_origin_name ||
          !node_it->second->template_origin_name[0] ||
          node_it->second->n_template_args <= 0) {
        continue;
      }
      if (unique_derived && unique_derived != node_it->second) {
        unique_derived = nullptr;
        break;
      }
      unique_derived = node_it->second;
    }
    if (!unique_derived) return;
    ts->base = TB_STRUCT;
    ts->record_def = const_cast<Node*>(unique_derived);
    ts->tag_text_id = unique_derived->unqualified_text_id;
    ts->namespace_context_id = unique_derived->namespace_context_id;
    ts->is_global_qualified = false;
  };
  auto hydrate_template_origin_from_structured_key = [&](TypeSpec* ts) {
    if (!ts || (ts->tpl_struct_origin && ts->tpl_struct_origin[0]) ||
        ts->tpl_struct_origin_key.base_text_id == kInvalidText) {
      return;
    }
    const Node* primary_tpl = nullptr;
    for (const auto& [owner_key, candidate] : template_struct_defs_by_owner_) {
      if (!candidate ||
          owner_key.kind != HirRecordOwnerKeyKind::Declaration ||
          owner_key.namespace_context_id !=
              ts->tpl_struct_origin_key.context_id ||
          owner_key.is_global_qualified !=
              ts->tpl_struct_origin_key.is_global_qualified ||
          owner_key.declaration_text_id !=
              ts->tpl_struct_origin_key.base_text_id) {
        continue;
      }
      if (primary_tpl && primary_tpl != candidate) {
        primary_tpl = nullptr;
        break;
      }
      primary_tpl = candidate;
    }
    if (!primary_tpl) {
      for (const auto& [_, candidate] : template_struct_defs_) {
        if (!candidate ||
            candidate->unqualified_text_id !=
                ts->tpl_struct_origin_key.base_text_id) {
          continue;
        }
        if (ts->tpl_struct_origin_key.context_id >= 0 &&
            candidate->namespace_context_id !=
                ts->tpl_struct_origin_key.context_id) {
          continue;
        }
        if (primary_tpl && primary_tpl != candidate) {
          primary_tpl = nullptr;
          break;
        }
        primary_tpl = candidate;
      }
    }
    if (!primary_tpl) return;
    if (primary_tpl->name && primary_tpl->name[0]) {
      ts->tpl_struct_origin = primary_tpl->name;
    } else if (primary_tpl->template_origin_name &&
               primary_tpl->template_origin_name[0]) {
      ts->tpl_struct_origin = primary_tpl->template_origin_name;
    } else if (primary_tpl->unqualified_name &&
               primary_tpl->unqualified_name[0]) {
      ts->tpl_struct_origin = primary_tpl->unqualified_name;
    }
  };
  for (int i = 0; i < ref->n_template_args; ++i) {
    HirTemplateArg arg{};
    arg.is_value = ref->template_arg_is_value && ref->template_arg_is_value[i];
    const Node* value_expr =
        ref->template_arg_exprs ? ref->template_arg_exprs[i] : nullptr;
    const bool expects_type_arg =
        primary && i < primary->n_template_params &&
        !(primary->template_param_is_nttp && primary->template_param_is_nttp[i]);
    const TypeSpec* projected_value_type = nullptr;
    if (expects_type_arg && value_expr && value_expr->type.tpl_struct_origin) {
      projected_value_type = &value_expr->type;
      arg.is_value = false;
    }
    if (arg.is_value) {
      resolve_ast_template_value_arg(primary, ref, i, ctx, &arg.value);
    } else if (projected_value_type || ref->template_arg_types) {
      TypeSpec ts =
          projected_value_type ? *projected_value_type : ref->template_arg_types[i];
      hydrate_template_origin_from_structured_key(&ts);
      TypeSpec member_typedef_carrier = ts;
      const bool preserve_member_typedef_carrier =
          ts.tpl_struct_origin && ts.tpl_struct_origin[0] &&
          ts.deferred_member_type_name && ts.deferred_member_type_name[0] &&
          ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0;
      const bool had_structured_member_typedef_payload =
          ts.tpl_struct_origin_key.base_text_id != kInvalidText ||
          (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) ||
          ts.deferred_member_type_owner_key.base_text_id != kInvalidText ||
          ts.deferred_member_type_text_id != kInvalidText ||
          (ts.deferred_member_type_name && ts.deferred_member_type_name[0]);
      if (ctx && ts.base == TB_TYPEDEF) {
        const TypeSpec lookup_candidate =
            type_binding_lookup_carrier_with_valid_owner_text(ts, module_);
        if (const TypeSpec* bound = find_template_type_binding_for_call(
                &ctx->tpl_bindings, &ctx->structured_tpl_bindings,
                &ctx->tpl_bindings_by_text, module_, lookup_candidate)) {
          apply_template_global_arg_binding(ts, *bound);
        }
      }
      TypeBindings tpl_empty;
      NttpBindings nttp_empty;
      seed_and_resolve_pending_template_type_if_needed(
          ts, ctx ? ctx->tpl_bindings : tpl_empty,
          ctx ? ctx->nttp_bindings : nttp_empty, ref,
          PendingTemplateTypeKind::DeclarationType, "template-global-arg");
      while (resolve_struct_member_typedef_if_ready(&ts)) {
      }
      if (ts.deferred_member_type_name) {
        TypeSpec resolved_member{};
        bool resolved = resolve_template_global_record_member_typedef(
            ts.record_def, ts.deferred_member_type_name, &resolved_member);
        if (!resolved && ts.record_def) {
          const std::optional<HirRecordOwnerKey> owner_key =
              make_struct_def_node_owner_key(ts.record_def);
          if (owner_key) {
            if (const SymbolName* owner_tag =
                    module_->find_struct_def_tag_by_owner(*owner_key)) {
              resolved = resolve_struct_member_typedef_type(
                  *owner_tag, ts.deferred_member_type_name,
                  ts.deferred_member_type_text_id, &resolved_member);
            }
          }
        }
        if (!resolved && ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
          const Node* primary_tpl = find_template_struct_primary(ts.tpl_struct_origin);
          if (primary_tpl) {
            ResolvedTemplateArgs resolved_args = materialize_template_args(
                primary_tpl, ts, ctx ? ctx->tpl_bindings : tpl_empty,
                ctx ? ctx->nttp_bindings : nttp_empty);
            SelectedTemplateStructPattern selected =
                template_struct_has_pack_params(primary_tpl)
                    ? SelectedTemplateStructPattern{}
                    : select_template_struct_pattern_hir(
                          resolved_args.concrete_args,
                          build_template_struct_env(primary_tpl));
            if (template_struct_has_pack_params(primary_tpl)) {
              selected.primary_def = primary_tpl;
              selected.selected_pattern = primary_tpl;
              for (const auto& [name, type] : resolved_args.type_bindings) {
                selected.type_bindings[name] = type;
              }
              for (const auto& [name, value] : resolved_args.nttp_bindings) {
                selected.nttp_bindings[name] = value;
              }
            }
            const Node* selected_def =
                selected.selected_pattern ? selected.selected_pattern : primary_tpl;
            const std::string structured_owner_tag = build_template_mangled_name(
                primary_tpl, selected_def, ts.tpl_struct_origin, resolved_args);
            if (!structured_owner_tag.empty()) {
              resolved = resolve_struct_member_typedef_type(
                  structured_owner_tag, ts.deferred_member_type_name,
                  ts.deferred_member_type_text_id,
                  &resolved_member);
            }
          }
        }
        if (resolved) {
          ts = resolved_member;
        }
      }
      resolve_typedef_to_struct(ts);
      recover_unique_realized_template_arg_carrier(&ts);
      if (had_structured_member_typedef_payload) {
        recover_unique_derived_template_arg_carrier(&ts);
      }
      arg.type = preserve_member_typedef_carrier ? member_typedef_carrier : ts;
    }
    actual_args.push_back(arg);
  }

  TemplateStructEnv env;
  env.primary_def = primary;
  env.specialization_patterns = find_template_global_specializations(primary);
  SelectedTemplateStructPattern selected =
      select_template_struct_pattern_hir(actual_args, env);
  if (!selected.selected_pattern) return std::nullopt;

  const auto param_order =
      get_template_param_order(primary, &selected.type_bindings, &selected.nttp_bindings);
  const SpecializationKey spec_key =
      selected.nttp_bindings.empty()
          ? make_specialization_key(primary->name, param_order,
                                    selected.type_bindings, primary)
          : make_specialization_key(primary->name, param_order, selected.type_bindings,
                                    selected.nttp_bindings, primary);

  TemplateStructInstanceKey instance_key{primary, spec_key};
  auto existing = instantiated_template_globals_.find(instance_key);
  if (existing != instantiated_template_globals_.end()) return existing->second;

  std::string mangled = primary->name;
  for (const auto& pname : param_order) {
    mangled += "_";
    mangled += pname;
    mangled += "_";
    auto tit = selected.type_bindings.find(pname);
    if (tit != selected.type_bindings.end()) {
      mangled += type_suffix_for_mangling(tit->second);
      continue;
    }
    auto nit = selected.nttp_bindings.find(pname);
    if (nit != selected.nttp_bindings.end()) {
      mangled += std::to_string(nit->second);
    }
  }

  const GlobalId existing_global = module_->lookup_global_id(mangled);
  if (existing_global.valid()) {
    instantiated_template_globals_[instance_key] = existing_global;
    return existing_global;
  }

  const Node* chosen = selected.selected_pattern;
  TypeBindings lowered_type_bindings = selected.type_bindings;
  for (auto& [_, bound_ts] : lowered_type_bindings) {
    const bool had_structured_member_typedef_payload =
        bound_ts.tpl_struct_origin_key.base_text_id != kInvalidText ||
        (bound_ts.tpl_struct_origin && bound_ts.tpl_struct_origin[0]) ||
        bound_ts.deferred_member_type_owner_key.base_text_id != kInvalidText ||
        bound_ts.deferred_member_type_text_id != kInvalidText ||
        (bound_ts.deferred_member_type_name &&
         bound_ts.deferred_member_type_name[0]);
    if (!had_structured_member_typedef_payload) continue;
    TypeBindings tpl_empty;
    NttpBindings nttp_empty;
    if (bound_ts.tpl_struct_origin && bound_ts.tpl_struct_origin[0] &&
        bound_ts.deferred_member_type_name &&
        bound_ts.deferred_member_type_name[0]) {
      TypeSpec owner_ts = bound_ts;
      owner_ts.deferred_member_type_name = nullptr;
      owner_ts.deferred_member_type_text_id = kInvalidText;
      owner_ts.deferred_member_type_owner_key = {};
      const Node* primary_tpl = find_template_struct_primary(bound_ts.tpl_struct_origin);
      realize_template_struct_if_needed(owner_ts, tpl_empty, nttp_empty, primary_tpl);
      recover_unique_realized_template_arg_carrier(&owner_ts);
      if (owner_ts.record_def || owner_ts.tag_text_id != kInvalidText) {
        bound_ts = owner_ts;
        continue;
      }
    }
    seed_and_resolve_pending_template_type_if_needed(
        bound_ts, tpl_empty, nttp_empty, ref,
        PendingTemplateTypeKind::DeclarationType,
        "template-global-lowered-binding");
    while (resolve_struct_member_typedef_if_ready(&bound_ts)) {
    }
    resolve_typedef_to_struct(bound_ts);
    recover_unique_realized_template_arg_carrier(&bound_ts);
    recover_unique_derived_template_arg_carrier(&bound_ts);
  }
  const TypeBindings* tpl_ptr =
      lowered_type_bindings.empty() ? nullptr : &lowered_type_bindings;
  const NttpBindings* nttp_ptr =
      selected.nttp_bindings.empty() ? nullptr : &selected.nttp_bindings;
  NttpTextBindings nttp_by_text =
      build_call_nttp_text_bindings(nullptr, primary, selected.nttp_bindings);
  lower_global(chosen, &mangled, tpl_ptr, nttp_ptr,
               nttp_by_text.empty() ? nullptr : &nttp_by_text);
  const GlobalId global_id = module_->lookup_global_id(mangled);
  if (!global_id.valid()) return std::nullopt;
  instantiated_template_globals_[instance_key] = global_id;
  return global_id;
}

}  // namespace c4c::hir
