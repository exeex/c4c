#include "hir_lowerer_internal.hpp"

#include <cstring>
#include <functional>

namespace c4c::hir {

namespace {

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts) {
  if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
    std::string ref = "@";
    ref += ts.tpl_struct_origin;
    ref += ":";
    ref += ts.tpl_struct_arg_refs ? ts.tpl_struct_arg_refs : "";
    return ref;
  }
  if (ts.tag && ts.tag[0]) return ts.tag;

  std::string ref;
  switch (ts.base) {
    case TB_INT: ref = "int"; break;
    case TB_UINT: ref = "uint"; break;
    case TB_CHAR: ref = "char"; break;
    case TB_SCHAR: ref = "schar"; break;
    case TB_UCHAR: ref = "uchar"; break;
    case TB_SHORT: ref = "short"; break;
    case TB_USHORT: ref = "ushort"; break;
    case TB_LONG: ref = "long"; break;
    case TB_ULONG: ref = "ulong"; break;
    case TB_LONGLONG: ref = "llong"; break;
    case TB_ULONGLONG: ref = "ullong"; break;
    case TB_FLOAT: ref = "float"; break;
    case TB_DOUBLE: ref = "double"; break;
    case TB_LONGDOUBLE: ref = "ldouble"; break;
    case TB_VOID: ref = "void"; break;
    case TB_BOOL: ref = "bool"; break;
    case TB_INT128: ref = "i128"; break;
    case TB_UINT128: ref = "u128"; break;
    default: return "?";
  }
  for (int i = 0; i < ts.ptr_level; ++i) ref += "*";
  return ref;
}

}  // namespace

void Lowerer::apply_template_typedef_bindings(
    TypeSpec& ts,
    const TypeBindings& type_bindings) {
  for (const auto& [pname, pts] : type_bindings) {
    if (ts.base == TB_TYPEDEF && ts.tag && std::string(ts.tag) == pname) {
      ts.base = pts.base;
      ts.tag = pts.tag;
      ts.ptr_level += pts.ptr_level;
      ts.is_const |= pts.is_const;
      ts.is_volatile |= pts.is_volatile;
    }
  }
}

void Lowerer::materialize_template_array_extent(
    TypeSpec& ts,
    const NttpBindings& nttp_bindings) {
  if (!ts.array_size_expr) return;
  Node* ase = ts.array_size_expr;
  if (ase->kind != NK_VAR || !ase->name) return;
  for (const auto& [npname, nval] : nttp_bindings) {
    if (std::string(ase->name) == npname) {
      if (ts.array_rank > 0) {
        ts.array_dims[0] = nval;
        ts.array_size = nval;
      }
      ts.array_size_expr = nullptr;
      return;
    }
  }
}

void Lowerer::append_instantiated_template_struct_bases(
    HirStructDef& def,
    const Node* tpl_def,
    const TypeBindings& method_tpl_bindings,
    const NttpBindings& method_nttp_bindings) {
  for (int bi = 0; bi < tpl_def->n_bases; ++bi) {
    TypeSpec base_ts = tpl_def->base_types[bi];
    apply_template_typedef_bindings(base_ts, method_tpl_bindings);
    if (base_ts.tpl_struct_origin) {
      seed_template_type_dependency_if_needed(
          base_ts, method_tpl_bindings, method_nttp_bindings,
          PendingTemplateTypeKind::BaseType, "instantiation-base");
    }
    resolve_struct_member_typedef_if_ready(&base_ts);
    if (base_ts.tag && base_ts.tag[0]) def.base_tags.push_back(base_ts.tag);
  }
}

void Lowerer::register_instantiated_template_struct_methods(
    const std::string& mangled,
    const Node* tpl_def,
    const TypeBindings& method_tpl_bindings,
    const NttpBindings& method_nttp_bindings) {
  for (int mi = 0; mi < tpl_def->n_children; ++mi) {
    const Node* method = tpl_def->children[mi];
    if (!method || method->kind != NK_FUNCTION || !method->name) continue;
    const char* csuf = method->is_const_method ? "_const" : "";
    std::string mmangled = mangled + "__" + method->name + csuf;
    std::string mkey = mangled + "::" + method->name + csuf;
    if (struct_methods_.count(mkey)) continue;
    struct_methods_[mkey] = mmangled;
    struct_method_ret_types_[mkey] = method->type;
    lower_struct_method(mmangled, mangled, method,
                        &method_tpl_bindings, &method_nttp_bindings);
  }
}

void Lowerer::record_instantiated_template_struct_field_metadata(
    const std::string& mangled,
    const Node* orig_f,
    const NttpBindings& selected_nttp_bindings_map) {
  if (!orig_f || !orig_f->name) return;
  struct_static_member_decls_[mangled][orig_f->name] = orig_f;
  if (orig_f->is_static && orig_f->is_constexpr && orig_f->init) {
    struct_static_member_const_values_[mangled][orig_f->name] =
        eval_const_int_with_nttp_bindings(orig_f->init, selected_nttp_bindings_map);
  }
}

std::optional<HirStructField> Lowerer::instantiate_template_struct_field(
    const Node* orig_f,
    const TypeBindings& selected_type_bindings,
    const NttpBindings& selected_nttp_bindings_map,
    const Node* tpl_def,
    int llvm_idx) {
  if (!orig_f || !orig_f->name || orig_f->is_static) return std::nullopt;

  TypeSpec ft = orig_f->type;
  apply_template_typedef_bindings(ft, selected_type_bindings);
  materialize_template_array_extent(ft, selected_nttp_bindings_map);

  HirStructField hf;
  hf.name = orig_f->name;
  if (ft.array_rank > 0) {
    hf.is_flexible_array = (ft.array_size < 0);
    hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
    for (int i = 0; i + 1 < ft.array_rank; ++i) ft.array_dims[i] = ft.array_dims[i + 1];
    if (ft.array_rank > 0) ft.array_dims[ft.array_rank - 1] = -1;
    --ft.array_rank;
    ft.array_size = (ft.array_rank > 0) ? ft.array_dims[0] : -1;
  }
  hf.elem_type = ft;
  hf.is_anon_member = orig_f->is_anon_field;
  hf.llvm_idx = tpl_def->is_union ? 0 : llvm_idx;
  return hf;
}

void Lowerer::append_instantiated_template_struct_fields(
    HirStructDef& def,
    const std::string& mangled,
    const Node* tpl_def,
    const TypeBindings& selected_type_bindings,
    const NttpBindings& selected_nttp_bindings_map) {
  const int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
  int llvm_idx = 0;
  for (int fi = 0; fi < num_fields; ++fi) {
    const Node* orig_f = tpl_def->fields[fi];
    record_instantiated_template_struct_field_metadata(
        mangled, orig_f, selected_nttp_bindings_map);
    std::optional<HirStructField> hf = instantiate_template_struct_field(
        orig_f, selected_type_bindings, selected_nttp_bindings_map, tpl_def, llvm_idx);
    if (!hf) continue;
    def.fields.push_back(std::move(*hf));
    if (!tpl_def->is_union) ++llvm_idx;
  }
}

void Lowerer::instantiate_template_struct_body(
    const std::string& mangled,
    const Node* primary_tpl,
    const Node* tpl_def,
    const SelectedTemplateStructPattern& selected_pattern,
    const std::vector<HirTemplateArg>& concrete_args,
    const TemplateStructInstanceKey& instance_key) {
  if (module_->struct_defs.count(mangled) ||
      instantiated_tpl_struct_keys_.count(instance_key)) {
    return;
  }
  instantiated_tpl_struct_keys_.insert(instance_key);

  const TypeBindings& selected_type_bindings = selected_pattern.type_bindings;
  const NttpBindings& selected_nttp_bindings_map = selected_pattern.nttp_bindings;

  HirStructDef def;
  def.tag = mangled;
  def.ns_qual = make_ns_qual(tpl_def);
  def.is_union = tpl_def->is_union;
  def.pack_align = tpl_def->pack_align;
  def.struct_align = tpl_def->struct_align;
  TypeBindings method_tpl_bindings = selected_type_bindings;
  NttpBindings method_nttp_bindings = selected_nttp_bindings_map;
  append_instantiated_template_struct_bases(
      def, tpl_def, method_tpl_bindings, method_nttp_bindings);
  append_instantiated_template_struct_fields(
      def, mangled, tpl_def, selected_type_bindings, selected_nttp_bindings_map);

  compute_struct_layout(module_, def);
  module_->struct_def_order.push_back(mangled);
  module_->struct_defs[mangled] = std::move(def);
  register_instantiated_template_struct_methods(
      mangled, tpl_def, method_tpl_bindings, method_nttp_bindings);
}

const Node* Lowerer::find_template_struct_primary(const std::string& name) const {
  auto it = template_struct_defs_.find(name);
  return it != template_struct_defs_.end() ? it->second : nullptr;
}

const std::vector<const Node*>* Lowerer::find_template_struct_specializations(
    const Node* primary_tpl) const {
  if (!primary_tpl || !primary_tpl->name) return nullptr;
  auto it = template_struct_specializations_.find(primary_tpl->name);
  return it != template_struct_specializations_.end() ? &it->second : nullptr;
}

TemplateStructEnv Lowerer::build_template_struct_env(const Node* primary_tpl) const {
  TemplateStructEnv env;
  env.primary_def = primary_tpl;
  env.specialization_patterns = find_template_struct_specializations(primary_tpl);
  return env;
}

void Lowerer::register_template_struct_primary(
    const std::string& name,
    const Node* node) {
  if (!is_primary_template_struct_def(node)) return;
  template_struct_defs_[name] = node;
  ct_state_->register_template_struct_def(name, node);
}

void Lowerer::register_template_struct_specialization(
    const Node* primary_tpl,
    const Node* node) {
  if (!primary_tpl || !primary_tpl->name || !node) return;
  template_struct_specializations_[primary_tpl->name].push_back(node);
  ct_state_->register_template_struct_specialization(primary_tpl, node);
}

void Lowerer::seed_pending_template_type(const TypeSpec& ts,
                                         const TypeBindings& tpl_bindings,
                                         const NttpBindings& nttp_bindings,
                                         const Node* span_node,
                                         PendingTemplateTypeKind kind,
                                         const std::string& context_name) {
  if (!ts.tpl_struct_origin && !ts.deferred_member_type_name) return;
  const Node* owner_primary_def =
      ts.tpl_struct_origin ? find_template_struct_primary(ts.tpl_struct_origin) : nullptr;
  TypeSpec canonical_ts = ts;
  if (owner_primary_def && owner_primary_def->name && canonical_ts.tpl_struct_origin) {
    canonical_ts.tpl_struct_origin = owner_primary_def->name;
  }
  ct_state_->record_pending_template_type(
      kind, canonical_ts, owner_primary_def, tpl_bindings, nttp_bindings,
      make_span(span_node), context_name);
}

const Node* Lowerer::canonical_template_struct_primary(
    const TypeSpec& ts,
    const Node* primary_tpl) const {
  if (primary_tpl) return primary_tpl;
  if (!ts.tpl_struct_origin) return nullptr;
  return find_template_struct_primary(ts.tpl_struct_origin);
}

void Lowerer::resolve_pending_tpl_struct_if_needed(
    TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    const Node* primary_tpl) {
  if (!ts.tpl_struct_origin) return;
  resolve_pending_tpl_struct(
      ts, canonical_template_struct_primary(ts, primary_tpl),
      tpl_bindings, nttp_bindings);
}

std::string Lowerer::format_deferred_template_type_diagnostic(
    const PendingTemplateTypeWorkItem& work_item,
    const char* prefix,
    const char* detail) const {
  std::string message = prefix;
  if (!work_item.context_name.empty()) {
    message += ": ";
    message += work_item.context_name;
    if (detail && detail[0]) {
      message += " (";
      message += detail;
      message += ")";
    }
    return message;
  }
  if (detail && detail[0]) {
    message += ": ";
    message += detail;
  }
  return message;
}

DeferredTemplateTypeResult Lowerer::blocked_deferred_template_type(
    const PendingTemplateTypeWorkItem& work_item,
    const char* detail,
    bool spawned_new_work) const {
  return DeferredTemplateTypeResult::blocked(
      spawned_new_work,
      format_deferred_template_type_diagnostic(
          work_item, "blocked template type", detail));
}

DeferredTemplateTypeResult Lowerer::terminal_deferred_template_type(
    const PendingTemplateTypeWorkItem& work_item,
    const char* detail) const {
  return DeferredTemplateTypeResult::terminal(
      format_deferred_template_type_diagnostic(
          work_item, "unresolved template type", detail));
}

const Node* Lowerer::require_pending_template_type_primary(
    const TypeSpec& ts,
    const PendingTemplateTypeWorkItem& work_item,
    const char* missing_detail,
    DeferredTemplateTypeResult* out_result) const {
  const Node* primary_tpl =
      canonical_template_struct_primary(ts, work_item.owner_primary_def);
  if (primary_tpl) return primary_tpl;
  if (out_result) {
    *out_result = terminal_deferred_template_type(work_item, missing_detail);
  }
  return nullptr;
}

bool Lowerer::resolve_pending_template_owner_if_ready(
    TypeSpec& owner_ts,
    const PendingTemplateTypeWorkItem& work_item,
    const Node* primary_tpl,
    bool spawned_owner_work,
    const char* pending_detail,
    DeferredTemplateTypeResult* out_result) {
  resolve_pending_tpl_struct_if_needed(
      owner_ts, work_item.type_bindings, work_item.nttp_bindings,
      primary_tpl);
  if (!owner_ts.tpl_struct_origin) return true;
  if (out_result) {
    *out_result = blocked_deferred_template_type(
        work_item, pending_detail, spawned_owner_work);
  }
  return false;
}

bool Lowerer::spawn_pending_template_owner_work(
    const PendingTemplateTypeWorkItem& work_item,
    const TypeSpec& owner_ts) {
  const Node* owner_primary_def = work_item.owner_primary_def;
  if (!owner_primary_def && owner_ts.tpl_struct_origin) {
    owner_primary_def = canonical_template_struct_primary(owner_ts);
  }
  return ct_state_->record_pending_template_type(
      PendingTemplateTypeKind::OwnerStruct,
      owner_ts,
      owner_primary_def,
      work_item.type_bindings,
      work_item.nttp_bindings,
      work_item.span,
      work_item.context_name.empty()
          ? "owner-struct"
          : work_item.context_name + ":owner");
}

bool Lowerer::ensure_pending_template_owner_ready(
    TypeSpec& owner_ts,
    const PendingTemplateTypeWorkItem& work_item,
    bool spawn_owner_work,
    const char* missing_detail,
    const char* pending_detail,
    DeferredTemplateTypeResult* out_result) {
  if (!owner_ts.tpl_struct_origin) return true;
  const Node* primary_tpl = require_pending_template_type_primary(
      owner_ts, work_item, missing_detail, out_result);
  if (!primary_tpl) return false;
  bool spawned_owner_work = false;
  if (spawn_owner_work) {
    spawned_owner_work = spawn_pending_template_owner_work(work_item, owner_ts);
  }
  return resolve_pending_template_owner_if_ready(
      owner_ts, work_item, primary_tpl, spawned_owner_work,
      pending_detail, out_result);
}

DeferredTemplateTypeResult Lowerer::resolve_deferred_member_typedef_type(
    const PendingTemplateTypeWorkItem& work_item) {
  TypeSpec owner_ts = work_item.pending_type;
  owner_ts.deferred_member_type_name = nullptr;
  DeferredTemplateTypeResult result;
  if (!ensure_pending_template_owner_ready(
          owner_ts, work_item, true,
          "no primary template def for member typedef owner",
          "owner struct still pending", &result)) {
    return result;
  }
  if (!owner_ts.tag || !owner_ts.tag[0]) {
    return blocked_deferred_template_type(
        work_item, "owner tag unavailable");
  }
  TypeSpec resolved_member{};
  if (resolve_struct_member_typedef_hir(
          owner_ts.tag, work_item.pending_type.deferred_member_type_name,
          &resolved_member)) {
    return DeferredTemplateTypeResult::resolved();
  }
  return terminal_deferred_template_type(
      work_item, "member typedef lookup failed");
}

void Lowerer::seed_template_type_dependency_if_needed(
    const TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    PendingTemplateTypeKind kind,
    const std::string& context_name,
    const Node* span_node) {
  if (!ts.tpl_struct_origin) return;
  seed_pending_template_type(
      ts, tpl_bindings, nttp_bindings, span_node, kind, context_name);
}

void Lowerer::seed_and_resolve_pending_template_type_if_needed(
    TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    const Node* span_node,
    PendingTemplateTypeKind kind,
    const std::string& context_name,
    const Node* primary_tpl) {
  if (!ts.tpl_struct_origin) return;
  seed_pending_template_type(
      ts, tpl_bindings, nttp_bindings, span_node, kind, context_name);
  resolve_pending_tpl_struct_if_needed(
      ts, tpl_bindings, nttp_bindings, primary_tpl);
}

bool Lowerer::resolve_struct_member_typedef_hir(const std::string& tag,
                                                const std::string& member,
                                                TypeSpec* out) {
  if (tag.empty() || member.empty() || !out) return false;

  auto apply_bindings = [&](TypeSpec ts,
                            const TypeBindings& type_bindings,
                            const NttpBindings& nttp_bindings) -> TypeSpec {
    if (ts.base == TB_TYPEDEF && ts.tag) {
      auto it = type_bindings.find(ts.tag);
      if (it != type_bindings.end()) ts = it->second;
    }
    if (ts.tpl_struct_origin) {
      std::vector<std::string> refs;
      if (ts.tpl_struct_arg_refs) {
        refs = split_deferred_template_arg_refs(ts.tpl_struct_arg_refs);
      }
      std::string updated_refs;
      for (size_t i = 0; i < refs.size(); ++i) {
        std::string part = refs[i];
        auto tit = type_bindings.find(part);
        if (tit != type_bindings.end()) {
          part = encode_template_type_arg_ref_hir(tit->second);
        } else {
          auto nit = nttp_bindings.find(part);
          if (nit != nttp_bindings.end()) part = std::to_string(nit->second);
        }
        if (i) updated_refs += ",";
        updated_refs += part;
      }
      if (!updated_refs.empty()) ts.tpl_struct_arg_refs = ::strdup(updated_refs.c_str());
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
              *resolved = apply_bindings(sdef->member_typedef_types[i],
                                         type_bindings, nttp_bindings);
              if (resolved->deferred_member_type_name &&
                  resolved->tag && resolved->tag[0]) {
                TypeSpec nested{};
                if (resolve_struct_member_typedef_hir(
                        resolved->tag, resolved->deferred_member_type_name, &nested)) {
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
  if (!ts || !ts->deferred_member_type_name || !ts->tag || !ts->tag[0]) {
    return false;
  }
  TypeSpec resolved_member{};
  if (!resolve_struct_member_typedef_hir(
          ts->tag, ts->deferred_member_type_name, &resolved_member)) {
    return false;
  }
  *ts = resolved_member;
  return true;
}

}  // namespace c4c::hir
