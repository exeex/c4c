#include "templates.hpp"

namespace c4c::hir {

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
      if (ctx && ts.base == TB_TYPEDEF && ts.tag) {
        const int outer_ptr_level = ts.ptr_level;
        const bool outer_lref = ts.is_lvalue_ref;
        const bool outer_rref = ts.is_rvalue_ref;
        const bool outer_const = ts.is_const;
        const bool outer_volatile = ts.is_volatile;
        auto it = ctx->tpl_bindings.find(ts.tag);
        if (it != ctx->tpl_bindings.end()) {
          ts = it->second;
          ts.ptr_level += outer_ptr_level;
          ts.is_lvalue_ref = ts.is_lvalue_ref || outer_lref;
          ts.is_rvalue_ref = !ts.is_lvalue_ref && (ts.is_rvalue_ref || outer_rref);
          ts.is_const = ts.is_const || outer_const;
          ts.is_volatile = ts.is_volatile || outer_volatile;
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
      if (ts.deferred_member_type_name && ts.tag && ts.tag[0]) {
        TypeSpec resolved_member{};
        if (resolve_struct_member_typedef_type(ts.tag, ts.deferred_member_type_name,
                                               &resolved_member)) {
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
