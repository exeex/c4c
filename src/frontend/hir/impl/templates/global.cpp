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
  for (int i = 0; i < ref->n_template_args; ++i) {
    HirTemplateArg arg{};
    arg.is_value = ref->template_arg_is_value && ref->template_arg_is_value[i];
    if (arg.is_value) {
      resolve_ast_template_value_arg(primary, ref, i, ctx, &arg.value);
    } else if (ref->template_arg_types) {
      TypeSpec ts = ref->template_arg_types[i];
      if (ctx && ts.base == TB_TYPEDEF) {
        if (ts.template_param_text_id != kInvalidText) {
          auto it = ctx->tpl_bindings_by_text.find(ts.template_param_text_id);
          if (it != ctx->tpl_bindings_by_text.end()) {
            apply_template_global_arg_binding(ts, it->second);
          }
        }
        if (ts.base == TB_TYPEDEF && ts.tag_text_id != kInvalidText) {
          auto it = ctx->tpl_bindings_by_text.find(ts.tag_text_id);
          if (it != ctx->tpl_bindings_by_text.end()) {
            apply_template_global_arg_binding(ts, it->second);
          }
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
                  *owner_tag, ts.deferred_member_type_name, &resolved_member);
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
                  &resolved_member);
            }
          }
        }
        if (resolved) {
          ts = resolved_member;
        }
      }
      resolve_typedef_to_struct(ts);
      arg.type = ts;
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
          ? make_specialization_key(primary->name, param_order, selected.type_bindings)
          : make_specialization_key(primary->name, param_order, selected.type_bindings,
                                    selected.nttp_bindings);

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
  const TypeBindings* tpl_ptr = selected.type_bindings.empty() ? nullptr : &selected.type_bindings;
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
