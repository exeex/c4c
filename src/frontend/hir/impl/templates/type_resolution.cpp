#include "templates.hpp"

#include <cstring>
#include <functional>

namespace c4c::hir {

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);

namespace {

template <typename T>
auto typespec_legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
  return ts.tag;
}

const char* typespec_legacy_tag_if_present(const TypeSpec&, long) {
  return nullptr;
}

bool template_param_owner_matches(const TypeSpec& ts, const Node* template_owner) {
  if (!template_owner || ts.template_param_owner_text_id == kInvalidText ||
      template_owner->unqualified_text_id == kInvalidText) {
    return true;
  }
  if (ts.template_param_owner_text_id != template_owner->unqualified_text_id) {
    return false;
  }
  return ts.template_param_owner_namespace_context_id < 0 ||
         ts.template_param_owner_namespace_context_id ==
             template_owner->namespace_context_id;
}

const TypeSpec* find_template_typedef_binding(
    const TypeSpec& ts,
    const TypeBindings& type_bindings,
    const Node* template_owner) {
  if (ts.base != TB_TYPEDEF || !template_param_owner_matches(ts, template_owner)) {
    return nullptr;
  }

  auto binding_for_param_index = [&](int index) -> const TypeSpec* {
    if (!template_owner || !template_owner->template_param_names ||
        index < 0 || index >= template_owner->n_template_params) {
      return nullptr;
    }
    if (template_owner->template_param_is_nttp &&
        template_owner->template_param_is_nttp[index]) {
      return nullptr;
    }
    const char* param_name = template_owner->template_param_names[index];
    if (!param_name) return nullptr;
    auto it = type_bindings.find(param_name);
    return it == type_bindings.end() ? nullptr : &it->second;
  };

  auto binding_for_param_text = [&](TextId text_id) -> const TypeSpec* {
    if (text_id == kInvalidText || !template_owner ||
        !template_owner->template_param_name_text_ids) {
      return nullptr;
    }
    for (int i = 0; i < template_owner->n_template_params; ++i) {
      if (template_owner->template_param_name_text_ids[i] != text_id) continue;
      if (const TypeSpec* bound = binding_for_param_index(i)) return bound;
    }
    return nullptr;
  };

  if (const TypeSpec* bound = binding_for_param_text(ts.template_param_text_id)) {
    return bound;
  }
  if (const TypeSpec* bound = binding_for_param_text(ts.tag_text_id)) {
    return bound;
  }
  if (const TypeSpec* bound = binding_for_param_index(ts.template_param_index)) {
    return bound;
  }

  const bool has_structured_param_carrier =
      ts.template_param_text_id != kInvalidText ||
      ts.tag_text_id != kInvalidText ||
      ts.template_param_index >= 0 ||
      ts.template_param_owner_text_id != kInvalidText ||
      ts.template_param_owner_namespace_context_id >= 0;
  if (has_structured_param_carrier) return nullptr;

  const char* legacy_tag = typespec_legacy_tag_if_present(ts, 0);
  if (!legacy_tag || !legacy_tag[0]) return nullptr;
  auto it = type_bindings.find(legacy_tag);
  return it == type_bindings.end() ? nullptr : &it->second;
}

void apply_template_typedef_binding(TypeSpec& target, const TypeSpec& concrete) {
  const int outer_ptr_level = target.ptr_level;
  const bool outer_lref = target.is_lvalue_ref;
  const bool outer_rref = target.is_rvalue_ref;
  const bool outer_const = target.is_const;
  const bool outer_volatile = target.is_volatile;
  target = concrete;
  target.ptr_level += outer_ptr_level;
  target.is_lvalue_ref = target.is_lvalue_ref || outer_lref;
  target.is_rvalue_ref = !target.is_lvalue_ref &&
                         (target.is_rvalue_ref || outer_rref);
  target.is_const = target.is_const || outer_const;
  target.is_volatile = target.is_volatile || outer_volatile;
}

std::string encode_template_arg_ref_hir(const TemplateArgRef& arg) {
  if (arg.kind == TemplateArgKind::Value && arg.value == 0 &&
      arg.debug_text && arg.debug_text[0]) {
    return arg.debug_text;
  }
  if (arg.kind == TemplateArgKind::Value) return std::to_string(arg.value);
  if (arg.kind == TemplateArgKind::Type &&
      (has_concrete_type(arg.type) || arg.type.tpl_struct_origin)) {
    return encode_template_type_arg_ref_hir(arg.type);
  }
  if (arg.debug_text && arg.debug_text[0]) return arg.debug_text;
  return {};
}

bool matches_trait_family(const std::string& name, const char* suffix) {
  if (name == suffix) return true;
  if (name.size() <= std::strlen(suffix) + 2) return false;
  return name.compare(name.size() - std::strlen(suffix), std::strlen(suffix), suffix) == 0 &&
         name.compare(name.size() - std::strlen(suffix) - 2, 2, "::") == 0;
}

bool resolve_record_def_member_typedef_type(const Node* record_def,
                                            const std::string& member,
                                            TypeSpec* out) {
  if (!record_def || record_def->kind != NK_STRUCT_DEF || member.empty() || !out) {
    return false;
  }
  if (!record_def->member_typedef_names || !record_def->member_typedef_types) {
    return false;
  }
  for (int i = 0; i < record_def->n_member_typedefs; ++i) {
    const char* alias_name = record_def->member_typedef_names[i];
    if (!alias_name || member != alias_name) continue;
    *out = record_def->member_typedef_types[i];
    return true;
  }
  return false;
}

std::optional<TypeSpec> try_resolve_unary_type_transform_trait(
    const std::string& tpl_name,
    const std::string& member,
    const std::vector<HirTemplateArg>& actual_args) {
  if (member != "type" || actual_args.size() != 1 || actual_args[0].is_value) {
    return std::nullopt;
  }

  TypeSpec resolved = actual_args[0].type;
  if (matches_trait_family(tpl_name, "remove_const")) {
    resolved.is_const = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_volatile")) {
    resolved.is_volatile = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_cv")) {
    resolved.is_const = false;
    resolved.is_volatile = false;
    return resolved;
  }
  if (matches_trait_family(tpl_name, "remove_reference")) {
    resolved.is_lvalue_ref = false;
    resolved.is_rvalue_ref = false;
    return resolved;
  }
  return std::nullopt;
}

}  // namespace

bool Lowerer::resolve_struct_member_typedef_type(const std::string& tag,
                                                 const std::string& member,
                                                 TypeSpec* out) {
  if (tag.empty() || member.empty() || !out) return false;

  std::function<TypeSpec(TypeSpec,
                         const TypeBindings&,
                         const NttpBindings&,
                         const Node*,
                         bool*)> apply_bindings =
      [&](TypeSpec ts,
          const TypeBindings& type_bindings,
          const NttpBindings& nttp_bindings,
          const Node* binding_owner,
          bool* substituted_type = nullptr) -> TypeSpec {
    if (substituted_type) *substituted_type = false;
    if (const TypeSpec* bound =
            find_template_typedef_binding(ts, type_bindings, binding_owner)) {
      apply_template_typedef_binding(ts, *bound);
      if (substituted_type) *substituted_type = true;
    }
    if (ts.tpl_struct_origin && ts.tpl_struct_args.data &&
        ts.tpl_struct_args.size > 0) {
      TemplateArgRef* rebound_args =
          new TemplateArgRef[ts.tpl_struct_args.size]();
      bool rebound_any = false;
      for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
        const TemplateArgRef& src_arg = ts.tpl_struct_args.data[i];
        TemplateArgRef& dst_arg = rebound_args[i];
        dst_arg = src_arg;
        if (src_arg.kind == TemplateArgKind::Type) {
          bool nested_substituted = false;
          if (has_concrete_type(src_arg.type) || src_arg.type.tpl_struct_origin) {
            dst_arg.type = apply_bindings(
                src_arg.type, type_bindings, nttp_bindings, binding_owner,
                &nested_substituted);
          } else if (src_arg.debug_text && src_arg.debug_text[0]) {
            auto tit = type_bindings.find(src_arg.debug_text);
            if (tit != type_bindings.end()) {
              dst_arg.type = tit->second;
              nested_substituted = true;
            }
          }
          rebound_any = rebound_any || nested_substituted;
          const std::string debug_text = has_concrete_type(dst_arg.type) ||
                                                 dst_arg.type.tpl_struct_origin
                                             ? encode_template_type_arg_ref_hir(dst_arg.type)
                                             : encode_template_arg_ref_hir(src_arg);
          dst_arg.debug_text =
              debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
        } else {
          bool rebound_value = false;
          if (src_arg.value == 0 && src_arg.debug_text && src_arg.debug_text[0]) {
            auto nit = nttp_bindings.find(src_arg.debug_text);
            if (nit != nttp_bindings.end()) {
              dst_arg.value = nit->second;
              rebound_value = true;
            }
          }
          const std::string debug_text =
              (!rebound_value && src_arg.value == 0 && src_arg.debug_text &&
               src_arg.debug_text[0])
                  ? std::string(src_arg.debug_text)
                  : std::to_string(dst_arg.value);
          dst_arg.debug_text =
              debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
        }
      }
      ts.tpl_struct_args.data = rebound_args;
      if (rebound_any && substituted_type) *substituted_type = true;
      seed_and_resolve_pending_template_type_if_needed(
          ts, type_bindings, nttp_bindings, nullptr,
          PendingTemplateTypeKind::MemberTypedef,
          "member-typedef-binding");
      return ts;
    }
    if (ts.tpl_struct_origin) {
      std::vector<std::string> updated_refs;
      updated_refs.reserve(ts.tpl_struct_args.size);
      for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
        std::string part = encode_template_arg_ref_hir(ts.tpl_struct_args.data[i]);
        auto tit = type_bindings.find(part);
        if (tit != type_bindings.end()) {
          part = encode_template_type_arg_ref_hir(tit->second);
        } else {
          std::string base_part = part;
          TypeSpec suffix_ts{};
          suffix_ts.array_size = -1;
          suffix_ts.inner_rank = -1;
          while (base_part.size() >= 4 &&
                 base_part.compare(base_part.size() - 4, 4, "_ptr") == 0) {
            suffix_ts.ptr_level++;
            base_part.resize(base_part.size() - 4);
          }
          if (base_part.size() >= 5 &&
              base_part.compare(base_part.size() - 5, 5, "_rref") == 0) {
            suffix_ts.is_rvalue_ref = true;
            base_part.resize(base_part.size() - 5);
          } else if (base_part.size() >= 4 &&
                     base_part.compare(base_part.size() - 4, 4, "_ref") == 0) {
            suffix_ts.is_lvalue_ref = true;
            base_part.resize(base_part.size() - 4);
          }
          if (base_part.size() >= 6 &&
              base_part.compare(0, 6, "const_") == 0) {
            suffix_ts.is_const = true;
            base_part.erase(0, 6);
          }
          if (base_part.size() >= 9 &&
              base_part.compare(0, 9, "volatile_") == 0) {
            suffix_ts.is_volatile = true;
            base_part.erase(0, 9);
          }
          auto nested_tit = type_bindings.find(base_part);
          if (nested_tit != type_bindings.end()) {
            TypeSpec rebound = nested_tit->second;
            rebound.ptr_level += suffix_ts.ptr_level;
            rebound.is_lvalue_ref =
                rebound.is_lvalue_ref || suffix_ts.is_lvalue_ref;
            rebound.is_rvalue_ref =
                !rebound.is_lvalue_ref &&
                (rebound.is_rvalue_ref || suffix_ts.is_rvalue_ref);
            rebound.is_const = rebound.is_const || suffix_ts.is_const;
            rebound.is_volatile =
                rebound.is_volatile || suffix_ts.is_volatile;
            part = encode_template_type_arg_ref_hir(rebound);
          } else {
            auto nit = nttp_bindings.find(part);
            if (nit != nttp_bindings.end()) part = std::to_string(nit->second);
          }
        }
        updated_refs.push_back(part);
      }
      if (!updated_refs.empty()) {
        ts.tpl_struct_args.data = new TemplateArgRef[updated_refs.size()]();
        ts.tpl_struct_args.size = static_cast<int>(updated_refs.size());
        for (size_t i = 0; i < updated_refs.size(); ++i) {
          ts.tpl_struct_args.data[i].kind = TemplateArgKind::Type;
          ts.tpl_struct_args.data[i].debug_text =
              updated_refs[i].empty() ? nullptr : ::strdup(updated_refs[i].c_str());
        }
      }
      seed_and_resolve_pending_template_type_if_needed(
          ts, type_bindings, nttp_bindings, nullptr,
          PendingTemplateTypeKind::MemberTypedef,
          "member-typedef-binding");
    }
    return ts;
  };

  std::function<bool(const Node*, const TypeBindings&, const NttpBindings&, TypeSpec*)>
      search_node =
          [&](const Node* sdef, const TypeBindings& type_bindings,
              const NttpBindings& nttp_bindings, TypeSpec* resolved) -> bool {
            if (!sdef || !resolved) return false;
            for (int i = 0; i < sdef->n_member_typedefs; ++i) {
              const char* alias_name = sdef->member_typedef_names[i];
              if (!alias_name || member != alias_name) continue;
              bool substituted_type = false;
              *resolved = apply_bindings(sdef->member_typedef_types[i],
                                         type_bindings,
                                         nttp_bindings,
                                         sdef,
                                         &substituted_type);
              if (!substituted_type && member == "type" &&
                  type_bindings.size() == 1) {
                *resolved = type_bindings.begin()->second;
              }
              if (resolved->deferred_member_type_name) {
                TypeSpec nested = *resolved;
                if (resolve_struct_member_typedef_if_ready(&nested)) {
                  *resolved = nested;
                }
              }
              return true;
            }

            auto mod_it = module_->struct_defs.find(sdef->name ? sdef->name : "");
            if (mod_it != module_->struct_defs.end()) {
              for (const auto& base_tag : mod_it->second.base_tags) {
                auto ast_it = struct_def_nodes_.find(base_tag);
                if (ast_it == struct_def_nodes_.end()) continue;
                if (search_node(ast_it->second, type_bindings, nttp_bindings, resolved))
                  return true;
              }
            }
            return false;
          };

  auto it = struct_def_nodes_.find(tag);
  if (it == struct_def_nodes_.end()) return false;
  const Node* sdef = it->second;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  for (int i = 0; i < sdef->n_template_params && i < sdef->n_template_args; ++i) {
    const char* pname = sdef->template_param_names ? sdef->template_param_names[i] : nullptr;
    if (!pname) continue;
    if (sdef->template_param_is_nttp && sdef->template_param_is_nttp[i]) {
      nttp_bindings[pname] = sdef->template_arg_values[i];
    } else if (sdef->template_arg_types) {
      type_bindings[pname] = sdef->template_arg_types[i];
    }
  }

  auto search_selected_pattern = [&](const Node* owner) -> bool {
    if (!owner) return false;

    const Node* primary_tpl = nullptr;
    if (owner->template_origin_name && owner->template_origin_name[0]) {
      primary_tpl = find_template_struct_primary(owner->template_origin_name);
    } else if (is_primary_template_struct_def(owner)) {
      primary_tpl = owner;
    } else if (owner->name && owner->name[0]) {
      primary_tpl = find_template_struct_primary(owner->name);
    }
    if (!primary_tpl || owner->n_template_args <= 0) return false;

    std::vector<HirTemplateArg> actual_args;
    actual_args.reserve(owner->n_template_args);
    for (int i = 0; i < owner->n_template_args; ++i) {
      HirTemplateArg arg{};
      arg.is_value =
          owner->template_arg_is_value && owner->template_arg_is_value[i];
      if (arg.is_value) {
        arg.value = owner->template_arg_values[i];
      } else if (owner->template_arg_types) {
        arg.type = owner->template_arg_types[i];
      }
      actual_args.push_back(arg);
    }

    SelectedTemplateStructPattern selected =
        select_template_struct_pattern_hir(actual_args,
                                           build_template_struct_env(primary_tpl));
    if (!selected.selected_pattern) return false;
    if (auto trait_result = try_resolve_unary_type_transform_trait(
            primary_tpl->name ? primary_tpl->name : std::string{},
            member, actual_args)) {
      *out = *trait_result;
      return true;
    }
    return search_node(selected.selected_pattern, selected.type_bindings,
                       selected.nttp_bindings, out);
  };

  if (search_selected_pattern(sdef)) return true;
  if (search_node(sdef, type_bindings, nttp_bindings, out)) return true;
  if (sdef->template_origin_name) {
    auto origin_it = struct_def_nodes_.find(sdef->template_origin_name);
    if (origin_it != struct_def_nodes_.end() &&
        search_node(origin_it->second, type_bindings, nttp_bindings, out)) {
      return true;
    }
  }
  return false;
}

bool Lowerer::resolve_struct_member_typedef_if_ready(TypeSpec* ts) {
  auto owner_key_from_type = [](const TypeSpec& owner)
      -> std::optional<HirRecordOwnerKey> {
    if (owner.tag_text_id == kInvalidText) return std::nullopt;
    NamespaceQualifier ns_qual;
    ns_qual.context_id = owner.namespace_context_id;
    ns_qual.is_global_qualified = owner.is_global_qualified;
    if (owner.qualifier_text_ids && owner.n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          owner.qualifier_text_ids,
          owner.qualifier_text_ids + owner.n_qualifier_segments);
    }
    HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, owner.tag_text_id);
    if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
    return key;
  };
  auto owner_tag_from_metadata = [&]() -> std::optional<std::string> {
    if (!ts) return std::nullopt;
    if (std::optional<HirRecordOwnerKey> owner_key = owner_key_from_type(*ts)) {
      if (module_) {
        if (const SymbolName* tag =
                module_->find_struct_def_tag_by_owner(*owner_key)) {
          return *tag;
        }
      }
      for (const auto& [rendered_tag, candidate] : struct_def_nodes_) {
        if (!candidate || candidate->kind != NK_STRUCT_DEF) continue;
        if (candidate->unqualified_text_id != ts->tag_text_id) continue;
        if (ts->namespace_context_id >= 0 &&
            candidate->namespace_context_id != ts->namespace_context_id) {
          continue;
        }
        return rendered_tag;
      }
      return std::nullopt;
    }
    if (ts->tag_text_id != kInvalidText) return std::nullopt;
    const char* legacy_tag = typespec_legacy_tag_if_present(*ts, 0);
    if (legacy_tag && legacy_tag[0]) return std::string(legacy_tag);
    return std::nullopt;
  };
  auto nested_deferred_member_ready = [&](const TypeSpec& member) -> bool {
    if (!member.deferred_member_type_name || !member.deferred_member_type_name[0]) {
      return false;
    }
    if (member.record_def && member.record_def->kind == NK_STRUCT_DEF) return true;
    if (member.tpl_struct_origin && member.tpl_struct_origin[0]) return true;
    if (owner_key_from_type(member)) return true;

    const bool has_structured_owner_carrier =
        member.tag_text_id != kInvalidText ||
        member.tpl_struct_origin_key.base_text_id != kInvalidText;
    if (has_structured_owner_carrier) return false;

    const char* legacy_tag = typespec_legacy_tag_if_present(member, 0);
    return legacy_tag && legacy_tag[0];
  };
  const bool has_origin = ts && ts->tpl_struct_origin && ts->tpl_struct_origin[0];
  const bool has_record_def = ts && ts->record_def;
  const std::optional<std::string> owner_tag = owner_tag_from_metadata();
  if (!ts || !ts->deferred_member_type_name ||
      (!has_record_def && !owner_tag && !has_origin)) {
    return false;
  }
  TypeSpec structured_member{};
  if (resolve_record_def_member_typedef_type(
          ts->record_def, ts->deferred_member_type_name, &structured_member)) {
    *ts = structured_member;
    return true;
  }
  auto apply_bindings = [&](TypeSpec resolved_member,
                            const TypeBindings& type_bindings,
                            const NttpBindings& nttp_bindings,
                            const Node* binding_owner,
                            bool* substituted_type = nullptr) -> TypeSpec {
    if (substituted_type) *substituted_type = false;
    if (const TypeSpec* bound = find_template_typedef_binding(
            resolved_member, type_bindings, binding_owner)) {
      apply_template_typedef_binding(resolved_member, *bound);
      if (substituted_type) *substituted_type = true;
    }
    if (resolved_member.array_size_expr &&
        resolved_member.array_size_expr->kind == NK_VAR &&
        resolved_member.array_size_expr->name) {
      auto nit = nttp_bindings.find(resolved_member.array_size_expr->name);
      if (nit != nttp_bindings.end()) {
        if (resolved_member.array_rank > 0) {
          resolved_member.array_dims[0] = nit->second;
          resolved_member.array_size = nit->second;
        }
        resolved_member.array_size_expr = nullptr;
      }
    }
    return resolved_member;
  };
  auto try_resolve_from_origin = [&]() -> bool {
    if (!ts->tpl_struct_origin || !ts->tpl_struct_origin[0]) return false;
    const Node* primary_tpl = find_template_struct_primary(ts->tpl_struct_origin);
    if (!primary_tpl) return false;
    const std::string deferred_member_name =
        ts->deferred_member_type_name ? ts->deferred_member_type_name : "";

    TypeBindings empty_tb;
    NttpBindings empty_nb;
    ResolvedTemplateArgs resolved =
        materialize_template_args(primary_tpl, *ts, empty_tb, empty_nb);

    SelectedTemplateStructPattern selected;
    if (template_struct_has_pack_params(primary_tpl)) {
      selected.primary_def = primary_tpl;
      selected.selected_pattern = primary_tpl;
      for (const auto& [name, type] : resolved.type_bindings) {
        selected.type_bindings[name] = type;
      }
      for (const auto& [name, value] : resolved.nttp_bindings) {
        selected.nttp_bindings[name] = value;
      }
    } else {
      selected = select_template_struct_pattern_hir(
          resolved.concrete_args, build_template_struct_env(primary_tpl));
    }

    const Node* owner =
        selected.selected_pattern ? selected.selected_pattern : primary_tpl;
    if (auto trait_result = try_resolve_unary_type_transform_trait(
            primary_tpl->name ? primary_tpl->name : std::string{},
            deferred_member_name,
            resolved.concrete_args)) {
      *ts = *trait_result;
      return true;
    }

    if (owner && owner->n_member_typedefs > 0) {
      for (int i = 0; i < owner->n_member_typedefs; ++i) {
        const char* alias_name = owner->member_typedef_names[i];
        if (!alias_name || std::string(alias_name) != deferred_member_name) {
          continue;
        }
        bool substituted_type = false;
        TypeSpec resolved_member = apply_bindings(
            owner->member_typedef_types[i],
            selected.type_bindings,
            selected.nttp_bindings,
            owner,
            &substituted_type);
        if (!substituted_type &&
            std::string(alias_name) == "type" &&
            selected.type_bindings.size() == 1) {
          resolved_member = selected.type_bindings.begin()->second;
        }
        if (nested_deferred_member_ready(resolved_member)) {
          while (resolve_struct_member_typedef_if_ready(&resolved_member)) {
          }
        }
        *ts = resolved_member;
        return true;
      }
    }

    // Some aliases, especially `::type` on trait helpers, are inherited from
    // realized base structs rather than declared on the selected pattern
    // itself. Materialize the owner and then resolve through its concrete
    // base-tag chain instead of giving up while we still only have origin args.
    TypeSpec owner_ts = *ts;
    owner_ts.deferred_member_type_name = nullptr;
    realize_template_struct_if_needed(owner_ts, empty_tb, empty_nb, primary_tpl);
    const Node* structured_owner =
        selected.selected_pattern ? selected.selected_pattern : primary_tpl;
    const std::string structured_owner_tag = build_template_mangled_name(
        primary_tpl, structured_owner,
        ts->tpl_struct_origin ? ts->tpl_struct_origin
                              : (primary_tpl->name ? primary_tpl->name : ""),
        resolved);
    if (!structured_owner_tag.empty()) {
      TypeSpec resolved_member{};
      if (resolve_struct_member_typedef_type(
              structured_owner_tag, deferred_member_name, &resolved_member)) {
        *ts = resolved_member;
        return true;
      }
    }
    if (owner_ts.tag && owner_ts.tag[0]) {
      TypeSpec resolved_member{};
      if (resolve_struct_member_typedef_type(
              owner_ts.tag, deferred_member_name, &resolved_member)) {
        *ts = resolved_member;
        return true;
      }
    }
    return false;
  };

  if (try_resolve_from_origin()) return true;
  if (has_record_def) return false;
  if (!owner_tag || owner_tag->empty()) return false;
  TypeSpec resolved_member{};
  if (!resolve_struct_member_typedef_type(
          *owner_tag, ts->deferred_member_type_name, &resolved_member)) {
    return false;
  }
  *ts = resolved_member;
  return true;
}

}  // namespace c4c::hir
