// Build/coordinator lowering cluster extracted from hir_lowering_core.cpp.
//
// Included here as a staging artifact for later split integration:
// - flatten_program_items
// - collect_weak_symbol_names
// - collect_enum_def
// - collect_initial_type_definitions
// - collect_consteval_function_definitions
// - collect_template_function_definitions
// - collect_function_template_specializations
// - collect_depth0_template_instantiations
// - get_template_param_order_from_instances
// - record_template_seed
// - resolve_template_call_name
// - collect_template_instantiations
// - collect_consteval_template_instantiations
// - realize_consteval_template_seeds_fixpoint
// - finalize_template_seed_realization
// - materialize_hir_template_defs
// - collect_ref_overloaded_free_functions
// - lower_initial_program
// - instantiate_deferred_template
// - instantiate_deferred_template_type
// - build_initial_hir
// - attach_out_of_class_struct_method_defs
// - lower_non_method_functions_and_globals
// - lower_pending_struct_methods

#include "impl/hir_impl.hpp"
#include "impl/lowerer.hpp"
#include "consteval.hpp"
#include "type_utils.hpp"
#include "../parser/parser_support.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace c4c::hir {

namespace {

void collect_late_static_asserts_recursive(
    const Node* n,
    CompileTimeState* ct_state,
    const Node* template_owner = nullptr) {
  if (!n || !ct_state) return;

  if (n->kind == NK_STATIC_ASSERT) {
    if (!(template_owner && template_owner->n_template_params > 0) &&
        !(n->n_template_params > 0)) {
      ct_state->register_static_assert(n);
    }
    return;
  }

  const Node* child_template_owner = template_owner;
  if (n->n_template_params > 0) child_template_owner = n;

  for (int i = 0; i < n->n_children; ++i)
    collect_late_static_asserts_recursive(n->children[i], ct_state, child_template_owner);
  for (int i = 0; i < n->n_fields; ++i)
    collect_late_static_asserts_recursive(n->fields[i], ct_state, child_template_owner);
  for (int i = 0; i < n->n_params; ++i)
    collect_late_static_asserts_recursive(n->params[i], ct_state, child_template_owner);
  if (n->left) collect_late_static_asserts_recursive(n->left, ct_state, child_template_owner);
  if (n->right) collect_late_static_asserts_recursive(n->right, ct_state, child_template_owner);
  if (n->cond) collect_late_static_asserts_recursive(n->cond, ct_state, child_template_owner);
  if (n->then_) collect_late_static_asserts_recursive(n->then_, ct_state, child_template_owner);
  if (n->else_) collect_late_static_asserts_recursive(n->else_, ct_state, child_template_owner);
  if (n->body) collect_late_static_asserts_recursive(n->body, ct_state, child_template_owner);
  if (n->init) collect_late_static_asserts_recursive(n->init, ct_state, child_template_owner);
  if (n->update) collect_late_static_asserts_recursive(n->update, ct_state, child_template_owner);
}

std::optional<HirStructMethodLookupKey> make_out_of_class_struct_method_lookup_key(
    const Node* fn,
    Module& m) {
  if (!fn || fn->kind != NK_FUNCTION || !m.link_name_texts ||
      fn->n_qualifier_segments <= 0 || !fn->qualifier_segments ||
      !fn->unqualified_name || !fn->unqualified_name[0]) {
    return std::nullopt;
  }

  TextTable* texts = m.link_name_texts.get();
  NamespaceQualifier owner_ns = make_ns_qual(fn, texts);
  if (owner_ns.segment_text_ids.size() !=
          static_cast<size_t>(fn->n_qualifier_segments) ||
      owner_ns.segments.empty()) {
    return std::nullopt;
  }

  const TextId owner_text_id = owner_ns.segment_text_ids.back();
  owner_ns.segments.pop_back();
  owner_ns.segment_text_ids.pop_back();
  const HirRecordOwnerKey owner_key =
      make_hir_record_owner_key(owner_ns, owner_text_id);
  if (!hir_record_owner_key_has_complete_metadata(owner_key) ||
      !m.find_struct_def_tag_by_owner(owner_key)) {
    return std::nullopt;
  }

  HirStructMethodLookupKey method_key;
  method_key.owner_key = owner_key;
  method_key.method_text_id = make_unqualified_text_id(fn, texts);
  method_key.is_const_method = fn->is_const_method;
  if (!hir_struct_method_lookup_key_has_complete_metadata(method_key)) {
    return std::nullopt;
  }
  return method_key;
}

}  // namespace

std::vector<const Node*> Lowerer::flatten_program_items(const Node* root) const {
  std::vector<const Node*> items;
  std::function<void(const Node*)> flatten = [&](const Node* n) {
    if (!n) return;
    if (n->kind == NK_BLOCK) {
      for (int j = 0; j < n->n_children; ++j) flatten(n->children[j]);
      return;
    }
    items.push_back(n);
  };
  for (int i = 0; i < root->n_children; ++i) flatten(root->children[i]);
  return items;
}

void Lowerer::collect_weak_symbol_names(const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_PRAGMA_WEAK && item->name) weak_symbols_.insert(item->name);
  }
}

void Lowerer::collect_enum_def(const Node* ed, bool register_structured_globals) {
  if (!ed || ed->kind != NK_ENUM_DEF || ed->n_enum_variants <= 0) return;
  if (!ed->enum_names || !ed->enum_vals) return;
  for (int i = 0; i < ed->n_enum_variants; ++i) {
    const char* name = ed->enum_names[i];
    if (!name || !name[0]) continue;
    enum_consts_[name] = ed->enum_vals[i];
    const auto key = register_structured_globals
                         ? make_global_enum_const_value_binding_key(ed, i)
                         : std::nullopt;
    if (key) {
      ct_state_->register_enum_const(*key, name, ed->enum_vals[i]);
    } else {
      ct_state_->register_enum_const(name, ed->enum_vals[i]);
    }
  }
}

std::optional<HirRecordOwnerKey> Lowerer::make_struct_def_node_owner_key(
    const Node* sd) const {
  if (!sd || sd->kind != NK_STRUCT_DEF || !sd->name || !sd->name[0]) {
    return std::nullopt;
  }
  TextTable* texts = module_ ? module_->link_name_texts.get() : nullptr;
  HirRecordOwnerKey key =
      make_hir_record_owner_key(make_ns_qual(sd, texts), make_unqualified_text_id(sd, texts));
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

void Lowerer::register_struct_def_node_owner(const Node* sd) {
  const auto key = make_struct_def_node_owner_key(sd);
  if (!key) return;
  struct_def_nodes_by_owner_[*key] = sd;
}

void Lowerer::collect_initial_type_definitions(const std::vector<const Node*>& items) {
  for (const Node* item : items) {
      if (item->kind == NK_STRUCT_DEF) {
      lower_struct_def(item);
      if (item->name) {
        struct_def_nodes_[item->name] = item;
        register_struct_def_node_owner(item);
      }
      if (is_primary_template_struct_def(item) && item->name) {
        register_template_struct_primary(item->name, item);
      }
      if (item->template_origin_name && item->template_origin_name[0]) {
        const Node* primary_tpl =
            find_template_struct_primary(item->template_origin_name);
        if (!primary_tpl && item->name) {
          std::string spelled_name = item->name;
          const size_t scope_sep = spelled_name.rfind("::");
          if (scope_sep != std::string::npos &&
              std::string(item->template_origin_name).find("::") ==
                  std::string::npos) {
            std::string qualified_origin =
                spelled_name.substr(0, scope_sep + 2) + item->template_origin_name;
            primary_tpl = find_template_struct_primary(qualified_origin);
          }
        }
        if (primary_tpl) register_template_struct_specialization(primary_tpl, item);
      }
    }
    if (item->kind == NK_ENUM_DEF) collect_enum_def(item, true);
  }
}

void Lowerer::collect_consteval_function_definitions(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->is_consteval && item->name) {
      ct_state_->register_consteval_def(item->name, item);
    }
  }
}

void Lowerer::collect_template_function_definitions(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->name && item->n_template_params > 0) {
      ct_state_->register_template_def(item->name, item);
    }
  }
}

void Lowerer::collect_function_template_specializations(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->name && item->is_explicit_specialization &&
        item->n_template_args > 0) {
      const Node* tpl_def = ct_state_->find_template_def(item->name);
      if (tpl_def) ct_state_->register_function_specialization(tpl_def, item);
    }
  }
}

void Lowerer::collect_depth0_template_instantiations(
    const std::vector<const Node*>& items) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION && item->body && item->n_template_params == 0) {
      collect_template_instantiations(item->body, item);
    }
  }
}

std::vector<std::string> Lowerer::get_template_param_order_from_instances(
    const std::string& fn_name) {
  const Node* tpl_def = ct_state_->find_template_def(fn_name);
  if (tpl_def) return get_template_param_order(tpl_def);
  return {};
}

std::string Lowerer::record_template_seed(const std::string& fn_name,
                                          TypeBindings bindings,
                                          NttpBindings nttp_bindings,
                                          TemplateSeedOrigin origin) {
  const Node* primary_def = ct_state_->find_template_def(fn_name);
  auto param_order =
      get_template_param_order(primary_def, &bindings, &nttp_bindings);
  return registry_.record_seed(fn_name, std::move(bindings),
                               std::move(nttp_bindings), param_order,
                               origin, primary_def);
}

std::string Lowerer::resolve_template_call_name(
    const Node* call_var,
    const TypeBindings* enclosing_bindings,
    const NttpBindings* enclosing_nttp) {
  if (!call_var || !call_var->name ||
      (call_var->n_template_args <= 0 && !call_var->has_template_args)) {
    return call_var ? (call_var->name ? call_var->name : "") : "";
  }
  const Node* fn_def = ct_state_->find_template_def(call_var->name);
  if (!fn_def) return call_var->name;
  if (fn_def->is_consteval) return call_var->name;
  TypeBindings bindings = merge_explicit_and_deduced_type_bindings(
      nullptr, call_var, fn_def, enclosing_bindings);
  NttpBindings nttp_bindings =
      build_call_nttp_bindings(call_var, fn_def, enclosing_nttp);
  return mangle_template_name(call_var->name, bindings, nttp_bindings);
}

void Lowerer::collect_template_instantiations(const Node* n,
                                              const Node* enclosing_fn) {
  if (!n) return;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args)) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && !fn_def->is_consteval && fn_def->n_template_params > 0) {
      if (enclosing_fn && enclosing_fn->name) {
        auto* enc_list = registry_.find_instances(enclosing_fn->name);
        if (enc_list) {
          for (const auto& enc_inst : *enc_list) {
            TypeBindings inner = merge_explicit_and_deduced_type_bindings(
                n, n->left, fn_def, &enc_inst.bindings, enclosing_fn);
            NttpBindings call_nttp = build_call_nttp_bindings(
                n->left, fn_def, &enc_inst.nttp_bindings);
            record_template_seed(
                n->left->name, std::move(inner), call_nttp,
                TemplateSeedOrigin::EnclosingTemplateExpansion);
          }
          goto recurse;
        }
      }
      NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
      if (has_forwarded_nttp(n->left)) goto recurse;
      TypeBindings bindings = merge_explicit_and_deduced_type_bindings(
          n, n->left, fn_def, nullptr, enclosing_fn);
      std::string mangled = record_template_seed(
          n->left->name, std::move(bindings), call_nttp,
          TemplateSeedOrigin::DirectCall);
      if (fn_def->n_template_params > n->left->n_template_args) {
        TypeBindings full_bindings = merge_explicit_and_deduced_type_bindings(
            n, n->left, fn_def, nullptr, enclosing_fn);
        deduced_template_calls_[n] = {
            mangled, std::move(full_bindings), std::move(call_nttp)};
      }
    }
  }
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name && n->left->n_template_args == 0 &&
      !n->left->has_template_args) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && !fn_def->is_consteval && fn_def->n_template_params > 0) {
      TypeBindings deduced =
          try_deduce_template_type_args(n, fn_def, enclosing_fn);
      if (deduction_covers_all_type_params(deduced, fn_def)) {
        fill_deduced_defaults(deduced, fn_def);
        NttpBindings nttp{};
        if (fn_def->template_param_has_default) {
          for (int i = 0; i < fn_def->n_template_params; ++i) {
            if (!fn_def->template_param_names[i]) continue;
            if (!fn_def->template_param_is_nttp ||
                !fn_def->template_param_is_nttp[i]) {
              continue;
            }
            if (fn_def->template_param_has_default[i]) {
              nttp[fn_def->template_param_names[i]] =
                  fn_def->template_param_default_values[i];
            }
          }
        }
        std::string mangled = record_template_seed(
            n->left->name, TypeBindings(deduced), nttp,
            TemplateSeedOrigin::DeducedCall);
        deduced_template_calls_[n] = {
            mangled, std::move(deduced), std::move(nttp)};
      }
    }
  }

recurse:
  if (n->left) collect_template_instantiations(n->left, enclosing_fn);
  if (n->right) collect_template_instantiations(n->right, enclosing_fn);
  if (n->cond) collect_template_instantiations(n->cond, enclosing_fn);
  if (n->then_) collect_template_instantiations(n->then_, enclosing_fn);
  if (n->else_) collect_template_instantiations(n->else_, enclosing_fn);
  if (n->body) collect_template_instantiations(n->body, enclosing_fn);
  if (n->init) collect_template_instantiations(n->init, enclosing_fn);
  if (n->update) collect_template_instantiations(n->update, enclosing_fn);
  for (int i = 0; i < n->n_children; ++i) {
    if (n->children[i]) collect_template_instantiations(n->children[i],
                                                        enclosing_fn);
  }
}

void Lowerer::collect_consteval_template_instantiations(
    const Node* n, const Node* enclosing_fn) {
  if (!n) return;
  if (n->kind == NK_CALL && n->left && n->left->kind == NK_VAR &&
      n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args)) {
    const Node* fn_def = ct_state_->find_template_def(n->left->name);
    if (fn_def && fn_def->is_consteval && fn_def->n_template_params > 0) {
      if (enclosing_fn && enclosing_fn->name) {
        auto* enc_list = registry_.find_instances(enclosing_fn->name);
        if (enc_list) {
          for (const auto& enc_inst : *enc_list) {
            TypeBindings inner =
                build_call_bindings(n->left, fn_def, &enc_inst.bindings);
            NttpBindings call_nttp = build_call_nttp_bindings(
                n->left, fn_def, &enc_inst.nttp_bindings);
            record_template_seed(
                n->left->name, std::move(inner), call_nttp,
                TemplateSeedOrigin::ConstevalEnclosingExpansion);
          }
          goto recurse_ce;
        }
        if (enclosing_fn->n_template_params > 0) goto recurse_ce;
      }
      NttpBindings call_nttp = build_call_nttp_bindings(n->left, fn_def);
      if (has_forwarded_nttp(n->left)) goto recurse_ce;
      TypeBindings bindings = build_call_bindings(n->left, fn_def, nullptr);
      record_template_seed(n->left->name, std::move(bindings), call_nttp,
                           TemplateSeedOrigin::ConstevalSeed);
    }
  }

recurse_ce:
  if (n->left) collect_consteval_template_instantiations(n->left, enclosing_fn);
  if (n->right) {
    collect_consteval_template_instantiations(n->right, enclosing_fn);
  }
  if (n->cond) collect_consteval_template_instantiations(n->cond, enclosing_fn);
  if (n->then_) collect_consteval_template_instantiations(n->then_, enclosing_fn);
  if (n->else_) collect_consteval_template_instantiations(n->else_, enclosing_fn);
  if (n->body) collect_consteval_template_instantiations(n->body, enclosing_fn);
  if (n->init) collect_consteval_template_instantiations(n->init, enclosing_fn);
  if (n->update) {
    collect_consteval_template_instantiations(n->update, enclosing_fn);
  }
  for (int i = 0; i < n->n_children; ++i) {
    if (n->children[i]) {
      collect_consteval_template_instantiations(n->children[i], enclosing_fn);
    }
  }
}

void Lowerer::realize_consteval_template_seeds_fixpoint(
    const std::vector<const Node*>& items) {
  for (int pass = 0; pass < 8; ++pass) {
    size_t prev_size = registry_.total_instance_count();
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION && item->body && item->n_template_params > 0) {
        collect_consteval_template_instantiations(item->body, item);
      }
    }
    registry_.realize_seeds();
    if (registry_.total_instance_count() == prev_size) break;
  }
}

void Lowerer::finalize_template_seed_realization() {
  registry_.realize_seeds();
  if (!registry_.verify_parity()) {
    registry_.dump_parity(stderr);
    throw std::runtime_error(
        "InstantiationRegistry: seed/instance parity violation after "
        "realize_seeds()");
  }
}

void Lowerer::materialize_hir_template_defs(Module& m) {
  ct_state_->for_each_template_def([&](const std::string& name, const Node* fn_def) {
    HirTemplateDef tdef;
    tdef.name = name;
    tdef.name_text_id = make_text_id(tdef.name, m.link_name_texts.get());
    tdef.is_consteval = fn_def->is_consteval;
    tdef.span = make_span(fn_def);
    for (int i = 0; i < fn_def->n_template_params; ++i) {
      if (fn_def->template_param_names[i]) {
        tdef.template_params.emplace_back(fn_def->template_param_names[i]);
        tdef.template_param_text_ids.push_back(
            make_text_id(tdef.template_params.back(), m.link_name_texts.get()));
      }
      tdef.param_is_nttp.push_back(
          fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]);
    }
    tdef.instances = ct_state_->instances_to_hir_metadata(
        name, tdef.template_params);
    for (auto& instance : tdef.instances) {
      instance.mangled_link_name_id = m.link_names.intern(instance.mangled_name);
    }
    m.template_defs[name] = std::move(tdef);
  });
}

void Lowerer::collect_ref_overloaded_free_functions(
    const std::vector<const Node*>& items) {
  std::unordered_map<std::string, const Node*> first_fn_decl;
  for (const Node* item : items) {
    if (item->kind != NK_FUNCTION || !item->name || item->is_consteval ||
        item->n_template_params > 0 || item->is_explicit_specialization) {
      continue;
    }
    std::string fn_name = item->name;
    auto prev_it = first_fn_decl.find(fn_name);
    if (prev_it == first_fn_decl.end()) {
      first_fn_decl[fn_name] = item;
      continue;
    }
    const Node* prev = prev_it->second;
    if (prev->n_params != item->n_params) continue;
    bool has_ref_diff = false;
    bool base_match = true;
    for (int pi = 0; pi < item->n_params; ++pi) {
      const TypeSpec& a = prev->params[pi]->type;
      const TypeSpec& b = item->params[pi]->type;
      if (a.is_lvalue_ref != b.is_lvalue_ref || a.is_rvalue_ref != b.is_rvalue_ref) {
        has_ref_diff = true;
      }
      if (a.base != b.base || a.ptr_level != b.ptr_level) {
        base_match = false;
        break;
      }
      if ((a.base == TB_STRUCT || a.base == TB_UNION) && a.tag && b.tag) {
        if (std::string(a.tag) != std::string(b.tag)) {
          base_match = false;
          break;
        }
      }
    }
    if (!has_ref_diff || !base_match) continue;
    auto& ovset = ref_overload_set_[fn_name];
    if (ovset.empty()) {
      Lowerer::RefOverloadEntry e0;
      e0.mangled_name = fn_name;
      for (int pi = 0; pi < prev->n_params; ++pi) {
        e0.param_is_rvalue_ref.push_back(prev->params[pi]->type.is_rvalue_ref);
        e0.param_is_lvalue_ref.push_back(prev->params[pi]->type.is_lvalue_ref);
      }
      ovset.push_back(std::move(e0));
      ref_overload_mangled_[prev] = fn_name;
    }
    Lowerer::RefOverloadEntry e1;
    e1.mangled_name = fn_name + "__rref_overload";
    for (int pi = 0; pi < item->n_params; ++pi) {
      e1.param_is_rvalue_ref.push_back(item->params[pi]->type.is_rvalue_ref);
      e1.param_is_lvalue_ref.push_back(item->params[pi]->type.is_lvalue_ref);
    }
    ovset.push_back(std::move(e1));
    ref_overload_mangled_[item] = fn_name + "__rref_overload";
  }
}

void Lowerer::attach_out_of_class_struct_method_defs(
    const std::vector<const Node*>& items,
    Module& m) {
  for (const Node* item : items) {
    if (item->kind != NK_FUNCTION || !item->body) continue;
    std::optional<std::string> mangled;
    const auto structured_key =
        make_out_of_class_struct_method_lookup_key(item, m);
    if (structured_key) {
      auto owner_it = struct_methods_by_owner_.find(*structured_key);
      if (owner_it != struct_methods_by_owner_.end()) {
        mangled = owner_it->second;
      }
    }
    if (!mangled) {
      auto method_ref = try_parse_qualified_struct_method_name(item);
      if (!method_ref.has_value()) continue;
      if (!m.struct_defs.count(method_ref->struct_tag)) continue;
      auto mit = struct_methods_.find(method_ref->key);
      if (mit == struct_methods_.end()) continue;
      mangled = mit->second;
    }
    for (auto& pm : pending_methods_) {
      if (pm.mangled == *mangled) {
        pm.method_node = item;
        break;
      }
    }
  }
}

void Lowerer::lower_non_method_functions_and_globals(
    const std::vector<const Node*>& items,
    Module& m) {
  for (const Node* item : items) {
    if (item->kind == NK_FUNCTION) {
      bool is_out_of_class_method = false;
      const auto structured_key =
          make_out_of_class_struct_method_lookup_key(item, m);
      if (structured_key &&
          struct_methods_by_owner_.count(*structured_key) > 0) {
        is_out_of_class_method = true;
      } else {
        auto method_ref = try_parse_qualified_struct_method_name(item);
        is_out_of_class_method =
            method_ref.has_value() &&
            m.struct_defs.count(method_ref->struct_tag) > 0 &&
            struct_methods_.count(method_ref->key) > 0;
      }
      if (is_out_of_class_method) {
        continue;
      }
      if (item->is_consteval && item->n_template_params == 0) {
        Function ce_fn{};
        ce_fn.id = next_fn_id();
        ce_fn.name = item->name ? item->name : "<anon_consteval>";
        ce_fn.name_text_id = make_unqualified_text_id(item, m.link_name_texts.get());
        ce_fn.link_name_id = m.link_names.intern(ce_fn.name);
        ce_fn.execution_domain = item->execution_domain;
        ce_fn.ns_qual = make_ns_qual(item, m.link_name_texts.get());
        ce_fn.return_type = qtype_from(item->type);
        ce_fn.consteval_only = true;
        ce_fn.span = make_span(item);
        for (int i = 0; i < item->n_params; ++i) {
          const Node* p = item->params[i];
          if (!p) continue;
          Param param{};
          param.name = p->name ? p->name : "<anon_param>";
          param.name_text_id = make_text_id(param.name, m.link_name_texts.get());
          param.type = qtype_from(p->type, ValueCategory::LValue);
          param.span = make_span(p);
          ce_fn.params.push_back(std::move(param));
        }
        m.index_function_decl(ce_fn);
        m.functions.push_back(std::move(ce_fn));
        continue;
      }
      if (item->is_consteval) continue;
      if (item->n_template_params > 0 && item->name) {
        auto* inst_list = registry_.find_instances(item->name);
        if (inst_list && !inst_list->empty()) {
          for (const auto& inst : *inst_list) {
            auto selected = registry_.select_function_specialization(
                item, inst.bindings, inst.nttp_bindings, inst.spec_key);
            if (selected.selected_pattern != item) {
              lower_function(selected.selected_pattern, &inst.mangled_name);
            } else {
              lower_function(item, &inst.mangled_name, &inst.bindings,
                             inst.nttp_bindings.empty() ? nullptr : &inst.nttp_bindings);
            }
            if (!m.functions.empty()) {
              m.functions.back().template_origin = item->name ? item->name : "";
              m.functions.back().spec_key = inst.spec_key;
            }
          }
        } else {
          if (!is_referenced_without_template_args(item->name, items)) continue;
          lower_function(item);
        }
      } else if (!item->is_explicit_specialization) {
        auto ovit = ref_overload_mangled_.find(item);
        if (ovit != ref_overload_mangled_.end()) {
          lower_function(item, &ovit->second);
        } else {
          lower_function(item);
        }
      }
    } else if (item->kind == NK_GLOBAL_VAR) {
      if (item->name && item->name[0] &&
          (template_global_defs_.count(item->name) > 0 ||
           template_global_specializations_.count(item->name) > 0) &&
          (item->n_template_params > 0 || item->n_template_args > 0)) {
        continue;
      }
      lower_global(item);
    }
  }
}

void Lowerer::lower_pending_struct_methods() {
  for (const auto& pm : pending_methods_) {
    lower_struct_method(pm.mangled, pm.struct_tag, pm.method_node,
                        pm.tpl_bindings.empty() ? nullptr : &pm.tpl_bindings,
                        pm.nttp_bindings.empty() ? nullptr : &pm.nttp_bindings);
  }
}

void Lowerer::lower_initial_program(const Node* root, Module& m) {
  if (!root || root->kind != NK_PROGRAM) {
    throw std::runtime_error("build_initial_hir: root is not NK_PROGRAM");
  }

  module_ = &m;

  std::vector<const Node*> items = flatten_program_items(root);
  function_decl_nodes_.clear();
  for (const Node* item : items) {
    if (item && item->kind == NK_FUNCTION && item->name && item->name[0]) {
      function_decl_nodes_.emplace(item->name, item);
    }
  }

  collect_weak_symbol_names(items);
  collect_initial_type_definitions(items);
  collect_consteval_function_definitions(items);
  collect_template_function_definitions(items);
  collect_function_template_specializations(items);
  collect_template_global_definitions(items);
  collect_depth0_template_instantiations(items);
  registry_.realize_seeds();
  realize_consteval_template_seeds_fixpoint(items);
  finalize_template_seed_realization();
  materialize_hir_template_defs(m);
  collect_ref_overloaded_free_functions(items);
  attach_out_of_class_struct_method_defs(items, m);
  lower_non_method_functions_and_globals(items, m);
  lower_pending_struct_methods();
}

bool Lowerer::instantiate_deferred_template(const std::string& tpl_name,
                                            const TypeBindings& bindings,
                                            const NttpBindings& nttp_bindings,
                                            const std::string& mangled) {
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
  collect_late_static_asserts_recursive(program_root, lowerer->ct_state().get());

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

}  // namespace c4c::hir
