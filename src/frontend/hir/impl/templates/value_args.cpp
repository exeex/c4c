#include "templates.hpp"

#include <climits>
#include <cstring>

namespace c4c::hir {

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);

namespace {

bool matches_trait_family(const std::string& name, const char* suffix) {
  if (name == suffix) return true;
  if (name.size() <= std::strlen(suffix) + 2) return false;
  return name.compare(name.size() - std::strlen(suffix), std::strlen(suffix), suffix) == 0 &&
         name.compare(name.size() - std::strlen(suffix) - 2, 2, "::") == 0;
}

bool is_generated_anonymous_record_tag(const char* name) {
  return name && std::strncmp(name, "_anon_", 6) == 0;
}

template <typename T>
auto typespec_legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
  return ts.tag;
}

const char* typespec_legacy_tag_if_present(const TypeSpec&, long) {
  return nullptr;
}

template <typename T>
auto assign_typespec_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype((void)(ts.tag = tag)) {
  ts.tag = tag;
}

void assign_typespec_legacy_tag_if_present(TypeSpec&, const char*, long) {}

bool is_signed_trait_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
  switch (ts.base) {
    case TB_FLOAT:
    case TB_DOUBLE:
    case TB_LONGDOUBLE:
    case TB_SCHAR:
    case TB_SHORT:
    case TB_INT:
    case TB_LONG:
    case TB_LONGLONG:
    case TB_INT128:
      return true;
    default:
      return false;
  }
}

bool is_unsigned_trait_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
  switch (ts.base) {
    case TB_BOOL:
    case TB_UCHAR:
    case TB_USHORT:
    case TB_UINT:
    case TB_ULONG:
    case TB_ULONGLONG:
    case TB_UINT128:
      return true;
    default:
      return false;
  }
}

bool is_const_trait_type(const TypeSpec& ts) {
  return ts.is_const && !ts.is_lvalue_ref && !ts.is_rvalue_ref;
}

bool is_reference_trait_type(const TypeSpec& ts) {
  return ts.is_lvalue_ref || ts.is_rvalue_ref;
}

bool is_enum_trait_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
  return ts.base == TB_ENUM;
}

void assign_template_arg_refs_from_hir_args(
    TypeSpec* ts,
    const std::vector<HirTemplateArg>& args) {
  if (!ts) return;
  ts->tpl_struct_args.data = nullptr;
  ts->tpl_struct_args.size = 0;
  if (args.empty()) return;

  ts->tpl_struct_args.data = new TemplateArgRef[args.size()]();
  ts->tpl_struct_args.size = static_cast<int>(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    const HirTemplateArg& arg = args[i];
    TemplateArgRef& out = ts->tpl_struct_args.data[i];
    out.kind = arg.is_value ? TemplateArgKind::Value
                            : TemplateArgKind::Type;
    out.type = arg.is_value ? TypeSpec{} : arg.type;
    out.value = arg.is_value ? arg.value : 0;
    const std::string debug_text =
        arg.is_value ? std::to_string(arg.value)
                     : encode_template_type_arg_ref_hir(arg.type);
    out.debug_text =
        debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
  }
}

void assign_template_arg_refs_from_struct_node(
    TypeSpec* ts,
    const Node* owner) {
  if (!ts) return;
  ts->tpl_struct_args.data = nullptr;
  ts->tpl_struct_args.size = 0;
  if (!owner) return;
  const int arg_count =
      owner->n_template_args > 0 ? owner->n_template_args : owner->n_template_params;
  if (arg_count <= 0) return;

  ts->tpl_struct_args.data = new TemplateArgRef[arg_count]();
  ts->tpl_struct_args.size = arg_count;
  for (int i = 0; i < owner->n_template_args; ++i) {
    TemplateArgRef& out = ts->tpl_struct_args.data[i];
    const bool is_value =
        owner->template_arg_is_value && owner->template_arg_is_value[i];
    out.kind = is_value ? TemplateArgKind::Value : TemplateArgKind::Type;
    if (is_value) {
      out.value = owner->template_arg_values ? owner->template_arg_values[i] : 0;
      std::string debug_text;
      if (owner->template_param_names &&
          owner->template_param_names[i] &&
          owner->template_param_names[i][0]) {
        debug_text = owner->template_param_names[i];
      } else {
        debug_text = std::to_string(out.value);
      }
      out.debug_text = ::strdup(debug_text.c_str());
    } else if (owner->template_arg_types) {
      out.type = owner->template_arg_types[i];
      const std::string debug_text = encode_template_type_arg_ref_hir(out.type);
      out.debug_text =
          debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
    } else if (owner->template_param_names &&
               owner->template_param_names[i] &&
               owner->template_param_names[i][0]) {
      out.debug_text = ::strdup(owner->template_param_names[i]);
    }
  }
  for (int i = owner->n_template_args; i < arg_count; ++i) {
    TemplateArgRef& out = ts->tpl_struct_args.data[i];
    const bool is_value =
        owner->template_param_is_nttp && owner->template_param_is_nttp[i];
    out.kind = is_value ? TemplateArgKind::Value : TemplateArgKind::Type;
    if (owner->template_param_names &&
        owner->template_param_names[i] &&
        owner->template_param_names[i][0]) {
      out.debug_text = ::strdup(owner->template_param_names[i]);
    }
  }
}

}  // namespace

namespace {

std::string unqualified_name(const std::string& name) {
  const size_t split = name.rfind("::");
  return (split == std::string::npos) ? name : name.substr(split + 2);
}

std::string qualified_owner_name(const std::string& name) {
  const size_t split = name.rfind("::");
  return (split == std::string::npos) ? std::string{} : name.substr(0, split);
}

std::string family_root(const std::string& tag) {
  const size_t scope_sep = tag.rfind("::");
  const size_t search_from = (scope_sep == std::string::npos) ? 0 : (scope_sep + 2);
  const size_t tpl_sep = tag.find("_T", search_from);
  return (tpl_sep == std::string::npos) ? tag : tag.substr(0, tpl_sep);
}

bool typespec_has_structured_owner_metadata(const TypeSpec& ts);

bool same_unqualified_or_qualified_name(const std::string& lhs,
                                        const std::string& rhs) {
  return lhs == rhs || unqualified_name(lhs) == unqualified_name(rhs);
}

std::optional<std::string> structured_injected_class_family_from_type(
    const TypeSpec& ts,
    const Module* module) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (ts.record_def->name && ts.record_def->name[0]) {
      return family_root(ts.record_def->name);
    }
    if (ts.record_def->unqualified_text_id != kInvalidText && module &&
        module->link_name_texts) {
      const std::string_view text =
          module->link_name_texts->lookup(ts.record_def->unqualified_text_id);
      if (!text.empty()) return family_root(std::string(text));
    }
  }
  if (ts.tag_text_id != kInvalidText && module && module->link_name_texts) {
    const std::string_view text = module->link_name_texts->lookup(ts.tag_text_id);
    if (!text.empty()) return family_root(std::string(text));
  }
  if (!typespec_has_structured_owner_metadata(ts)) {
    const char* legacy_tag = typespec_legacy_tag_if_present(ts, 0);
    if (legacy_tag && legacy_tag[0]) return family_root(legacy_tag);
  }
  return std::nullopt;
}

std::optional<HirRecordOwnerKey> template_origin_owner_key_from_type(
    const TypeSpec& ts) {
  const QualifiedNameKey& origin_key = ts.tpl_struct_origin_key;
  if (origin_key.base_text_id == kInvalidText) return std::nullopt;
  NamespaceQualifier ns_qual;
  ns_qual.context_id = origin_key.context_id;
  ns_qual.is_global_qualified = origin_key.is_global_qualified;
  if (origin_key.qualifier_path_id != kInvalidNamePath &&
      (!ts.qualifier_text_ids || ts.n_qualifier_segments <= 0)) {
    return std::nullopt;
  }
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  HirRecordOwnerKey key =
      make_hir_record_owner_key(ns_qual, origin_key.base_text_id);
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

std::optional<HirRecordOwnerKey> record_owner_key_from_type_tag_metadata(
    const TypeSpec& ts) {
  if (ts.tag_text_id == kInvalidText) return std::nullopt;
  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(
        ts.qualifier_text_ids,
        ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, ts.tag_text_id);
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

bool typespec_has_structured_owner_metadata(const TypeSpec& ts) {
  return (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) ||
         ts.tag_text_id != kInvalidText ||
         ts.namespace_context_id >= 0 ||
         (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) ||
         (ts.tpl_struct_origin_key.base_text_id != kInvalidText) ||
         (ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0) ||
         (ts.tpl_struct_origin && ts.tpl_struct_origin[0]);
}

bool typespec_has_structured_template_param_metadata(const TypeSpec& ts) {
  return ts.template_param_text_id != kInvalidText ||
         ts.tag_text_id != kInvalidText ||
         ts.template_param_index >= 0 ||
         ts.template_param_owner_text_id != kInvalidText ||
         ts.template_param_owner_namespace_context_id >= 0;
}

std::optional<std::string> structured_owner_tag_from_type(
    const TypeSpec& ts,
    const Module* module) {
  if (!module) return std::nullopt;
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (ts.record_def->unqualified_text_id != kInvalidText) {
      NamespaceQualifier ns_qual;
      ns_qual.context_id = ts.record_def->namespace_context_id;
      HirRecordOwnerKey owner_key =
          make_hir_record_owner_key(ns_qual, ts.record_def->unqualified_text_id);
      if (const SymbolName* owner_tag =
              module->find_struct_def_tag_by_owner(owner_key)) {
        if (module->struct_defs.count(*owner_tag)) return std::string(*owner_tag);
      }
    }
    if (ts.record_def->name && ts.record_def->name[0] &&
        module->struct_defs.count(ts.record_def->name)) {
      return std::string(ts.record_def->name);
    }
  }
  if (auto owner_key = record_owner_key_from_type_tag_metadata(ts)) {
    if (const SymbolName* owner_tag =
            module->find_struct_def_tag_by_owner(*owner_key)) {
      if (module->struct_defs.count(*owner_tag)) return std::string(*owner_tag);
    }
  }
  if (ts.tag_text_id != kInvalidText && module->link_name_texts) {
    const std::string_view tag_text = module->link_name_texts->lookup(ts.tag_text_id);
    if (!tag_text.empty() && module->struct_defs.count(std::string(tag_text))) {
      return std::string(tag_text);
    }
  }
  return std::nullopt;
}

std::optional<std::string> legacy_owner_tag_from_type_if_no_metadata(
    const TypeSpec& ts,
    const Module* module) {
  if (typespec_has_structured_owner_metadata(ts)) return std::nullopt;
  const char* legacy_tag = typespec_legacy_tag_if_present(ts, 0);
  if (!legacy_tag || !legacy_tag[0]) return std::nullopt;
  if (module && !module->struct_defs.count(legacy_tag)) return std::nullopt;
  return std::string(legacy_tag);
}

}  // namespace

bool Lowerer::recover_template_struct_identity_from_tag(
    TypeSpec* ts,
    const std::string* current_struct_tag) const {
  if (!ts || (ts->tpl_struct_origin && ts->tpl_struct_origin[0])) {
    return false;
  }
  const bool has_explicit_template_args =
      ts->tpl_struct_args.data && ts->tpl_struct_args.size > 0;
  if (!has_explicit_template_args && current_struct_tag &&
      !current_struct_tag->empty() && module_) {
    const std::string current_family = family_root(*current_struct_tag);
    if (auto source_family =
            structured_injected_class_family_from_type(*ts, module_);
        source_family &&
        same_unqualified_or_qualified_name(*source_family, current_family)) {
      auto current_def = module_->struct_defs.find(*current_struct_tag);
      if (current_def != module_->struct_defs.end()) {
        assign_typespec_legacy_tag_if_present(
            *ts, current_def->second.tag.c_str(), 0);
        ts->tag_text_id = current_def->second.tag_text_id;
        ts->namespace_context_id = current_def->second.ns_qual.context_id;
        ts->is_global_qualified =
            current_def->second.ns_qual.is_global_qualified;
        if (!current_def->second.ns_qual.segment_text_ids.empty()) {
          ts->qualifier_text_ids = const_cast<TextId*>(
              current_def->second.ns_qual.segment_text_ids.data());
          ts->n_qualifier_segments = static_cast<int>(
              current_def->second.ns_qual.segment_text_ids.size());
        }
        auto node_it = struct_def_nodes_.find(*current_struct_tag);
        if (node_it != struct_def_nodes_.end()) {
          ts->record_def = const_cast<Node*>(node_it->second);
        }
        return false;
      }
    }
  }
  const Node* owner = nullptr;
  bool has_structured_owner_metadata = false;

  if (auto origin_key = template_origin_owner_key_from_type(*ts)) {
    has_structured_owner_metadata = true;
    auto origin_it = template_struct_defs_by_owner_.find(*origin_key);
    if (origin_it != template_struct_defs_by_owner_.end()) {
      owner = origin_it->second;
    } else {
      return false;
    }
  }

  if (!owner && ts->record_def && ts->record_def->kind == NK_STRUCT_DEF) {
    has_structured_owner_metadata = true;
    owner = ts->record_def;
    if (auto record_key = make_struct_def_node_owner_key(ts->record_def)) {
      auto node_it = struct_def_nodes_by_owner_.find(*record_key);
      if (node_it != struct_def_nodes_by_owner_.end() && node_it->second) {
        owner = node_it->second;
      }
    }
  }

  if (!owner) {
    if (auto tag_key = record_owner_key_from_type_tag_metadata(*ts)) {
      has_structured_owner_metadata = true;
      auto node_it = struct_def_nodes_by_owner_.find(*tag_key);
      if (node_it != struct_def_nodes_by_owner_.end() && node_it->second) {
        owner = node_it->second;
      } else if (module_) {
        if (const SymbolName* rendered_tag =
                module_->find_struct_def_tag_by_owner(*tag_key)) {
          auto rendered_it = struct_def_nodes_.find(*rendered_tag);
          if (rendered_it != struct_def_nodes_.end()) {
            owner = rendered_it->second;
          }
        }
      }
    }
  }

  if (!owner && ts->tag_text_id != kInvalidText && module_ &&
      module_->link_name_texts) {
    has_structured_owner_metadata = true;
    const std::string_view tag_text = module_->link_name_texts->lookup(ts->tag_text_id);
    if (!tag_text.empty()) {
      auto text_it = struct_def_nodes_.find(std::string(tag_text));
      if (text_it != struct_def_nodes_.end()) {
        owner = text_it->second;
      }
    }
  }

  if (!owner && has_structured_owner_metadata) return false;

  if (!owner) {
    const char* legacy_tag = typespec_legacy_tag_if_present(*ts, 0);
    if (!legacy_tag || !legacy_tag[0]) return false;
    auto it = struct_def_nodes_.find(legacy_tag);
    if ((it == struct_def_nodes_.end() || !it->second) && current_struct_tag &&
        !current_struct_tag->empty()) {
      auto current_it = struct_def_nodes_.find(*current_struct_tag);
      if (current_it != struct_def_nodes_.end() && current_it->second) {
        const Node* current_owner = current_it->second;
        const std::string raw_tag = legacy_tag;
        const std::string raw_unqualified = unqualified_name(raw_tag);
        const std::string owner_name =
            current_owner->name ? current_owner->name : "";
        const std::string owner_unqualified = unqualified_name(owner_name);
        const std::string owner_origin =
            current_owner->template_origin_name ? current_owner->template_origin_name : "";
        const std::string owner_origin_unqualified = unqualified_name(owner_origin);
        if ((!owner_name.empty() &&
             (raw_tag == owner_name || raw_unqualified == owner_unqualified)) ||
            (!owner_origin.empty() &&
             (raw_tag == owner_origin || raw_unqualified == owner_origin_unqualified))) {
          assign_typespec_legacy_tag_if_present(*ts, current_struct_tag->c_str(), 0);
          it = current_it;
        }
      }
    }
    owner = (it != struct_def_nodes_.end()) ? it->second : nullptr;
  }

  if (!owner) {
    owner = canonical_template_struct_primary(*ts, owner);
  }
  if (!owner) return false;
  const char* origin = nullptr;
  if (owner->template_origin_name && owner->template_origin_name[0]) {
    origin = owner->template_origin_name;
  } else if (is_primary_template_struct_def(owner) && owner->name && owner->name[0]) {
    origin = owner->name;
  }
  if (!origin) return false;
  ts->tpl_struct_origin = origin;
  assign_template_arg_refs_from_struct_node(ts, owner);
  return true;
}

std::optional<std::string> Lowerer::resolve_member_lookup_owner_tag(
    TypeSpec base_ts,
    bool is_arrow,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const std::string* current_struct_tag,
    const Node* span_node,
    const std::string& context_name) {
  if (is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
  if (!is_arrow && (base_ts.is_lvalue_ref || base_ts.is_rvalue_ref)) {
    base_ts.is_lvalue_ref = false;
    base_ts.is_rvalue_ref = false;
    if (base_ts.ptr_level > 0) base_ts.ptr_level--;
  }
  while (resolve_struct_member_typedef_if_ready(&base_ts)) {
  }
  resolve_typedef_to_struct(base_ts);
  const bool has_explicit_template_args =
      base_ts.tpl_struct_args.data && base_ts.tpl_struct_args.size > 0;

  auto try_current_injected_class_owner_tag =
      [&]() -> std::optional<std::string> {
    if (has_explicit_template_args || !current_struct_tag ||
        current_struct_tag->empty() || !module_ ||
        !module_->struct_defs.count(*current_struct_tag)) {
      return std::nullopt;
    }
    const std::string current_family = family_root(*current_struct_tag);
    if (auto source_family =
            structured_injected_class_family_from_type(base_ts, module_);
        source_family &&
        same_unqualified_or_qualified_name(*source_family, current_family)) {
      return *current_struct_tag;
    }
    if (base_ts.tpl_struct_origin && base_ts.tpl_struct_origin[0] &&
        same_unqualified_or_qualified_name(
            family_root(base_ts.tpl_struct_origin), current_family)) {
      return *current_struct_tag;
    }
    const char* legacy_tag = typespec_legacy_tag_if_present(base_ts, 0);
    if (legacy_tag && legacy_tag[0] &&
        same_unqualified_or_qualified_name(family_root(legacy_tag),
                                           current_family)) {
      return *current_struct_tag;
    }
    return std::nullopt;
  };

  if (auto owner_tag = try_current_injected_class_owner_tag()) return owner_tag;

  if ((base_ts.base != TB_STRUCT && base_ts.base != TB_UNION) ||
      base_ts.ptr_level != 0 || base_ts.array_rank != 0) {
    return std::nullopt;
  }
  const std::optional<HirRecordOwnerKey> record_owner_key =
      (base_ts.record_def && base_ts.record_def->kind == NK_STRUCT_DEF)
          ? make_struct_def_node_owner_key(base_ts.record_def)
          : std::nullopt;
  const bool record_def_has_structured_owner_identity =
      record_owner_key.has_value() &&
      !is_generated_anonymous_record_tag(base_ts.record_def->name);
  const bool has_structured_owner_identity =
      record_def_has_structured_owner_identity ||
      typespec_has_structured_owner_metadata(base_ts) ||
      (base_ts.tpl_struct_origin && base_ts.tpl_struct_origin[0]) ||
      (base_ts.tpl_struct_args.data && base_ts.tpl_struct_args.size > 0);

  auto try_structured_record_owner_tag = [&]() -> std::optional<std::string> {
    if (!base_ts.record_def || base_ts.record_def->kind != NK_STRUCT_DEF) {
      return std::nullopt;
    }
    if (record_owner_key) {
      if (const SymbolName* owner_tag =
              module_->find_struct_def_tag_by_owner(*record_owner_key)) {
        if (module_->struct_defs.count(*owner_tag)) {
          return std::string(*owner_tag);
        }
      }
    }
    if (base_ts.record_def->name && base_ts.record_def->name[0] &&
        module_->struct_defs.count(base_ts.record_def->name)) {
      return std::string(base_ts.record_def->name);
    }
    return std::nullopt;
  };

  auto try_structured_template_owner_tag = [&]() -> std::optional<std::string> {
    if (!base_ts.tpl_struct_origin || !base_ts.tpl_struct_origin[0]) {
      return std::nullopt;
    }
    const std::optional<std::string> incoming_tag =
        structured_owner_tag_from_type(base_ts, module_);
    const char* incoming_legacy_tag =
        typespec_legacy_tag_if_present(base_ts, 0);
    NttpBindings empty_nttp;
    if (tpl_bindings) {
      seed_and_resolve_pending_template_type_if_needed(
          base_ts,
          *tpl_bindings,
          nttp_bindings ? *nttp_bindings : empty_nttp,
          span_node,
          PendingTemplateTypeKind::OwnerStruct,
          context_name);
    } else {
      TypeBindings empty_tb;
      realize_template_struct_if_needed(
          base_ts, empty_tb, nttp_bindings ? *nttp_bindings : empty_nttp);
    }
    if (auto owner_tag = structured_owner_tag_from_type(base_ts, module_)) {
      if (!incoming_tag || *incoming_tag != *owner_tag) return owner_tag;
    }
    const char* realized_legacy_tag =
        typespec_legacy_tag_if_present(base_ts, 0);
    if (!typespec_has_structured_owner_metadata(base_ts) &&
        realized_legacy_tag && realized_legacy_tag[0] &&
        (!incoming_legacy_tag || std::strcmp(incoming_legacy_tag,
                                            realized_legacy_tag) != 0) &&
        module_->struct_defs.count(realized_legacy_tag)) {
      return std::string(realized_legacy_tag);
    }
    return std::nullopt;
  };

  if (auto owner_tag = try_structured_record_owner_tag()) return owner_tag;
  if (auto owner_tag = structured_owner_tag_from_type(base_ts, module_)) {
    return owner_tag;
  }
  if (auto owner_tag = try_structured_template_owner_tag()) return owner_tag;

  if (!has_structured_owner_identity &&
      (!base_ts.tpl_struct_origin || !base_ts.tpl_struct_origin[0]) &&
      current_struct_tag && !current_struct_tag->empty()) {
    const char* legacy_tag = typespec_legacy_tag_if_present(base_ts, 0);
    if (legacy_tag && legacy_tag[0]) {
      const std::string raw_tag = legacy_tag;
      const std::string current_family = family_root(*current_struct_tag);
      const std::string current_family_unqualified = unqualified_name(current_family);
      if (raw_tag == current_family || raw_tag == current_family_unqualified) {
        return *current_struct_tag;
      }
    }
  }

  const bool recovered_identity =
      recover_template_struct_identity_from_tag(&base_ts, current_struct_tag);
  if (base_ts.tpl_struct_origin &&
      (tpl_bindings || recovered_identity)) {
    if (auto owner_tag = try_structured_template_owner_tag()) return owner_tag;
  } else if (recovered_identity) {
    TypeBindings empty_tb;
    NttpBindings empty_nb;
    realize_template_struct_if_needed(base_ts, empty_tb, empty_nb);
  }

  if (auto owner_tag =
          legacy_owner_tag_from_type_if_no_metadata(base_ts, module_)) return owner_tag;
  return std::nullopt;
}

void Lowerer::assign_template_arg_refs_from_ast_args(
    TypeSpec* ts,
    const Node* ref,
    const Node* owner_tpl,
    const FunctionCtx* ctx,
    const Node* span_node,
    PendingTemplateTypeKind kind,
    const std::string& context_name) {
  if (!ts) return;
  ts->tpl_struct_args.data = nullptr;
  ts->tpl_struct_args.size = 0;
  if (!ref || ref->n_template_args <= 0) return;

  std::vector<HirTemplateArg> actual_args;
  actual_args.reserve(ref->n_template_args);
  for (int i = 0; i < ref->n_template_args; ++i) {
    HirTemplateArg arg{};
    arg.is_value = ref->template_arg_is_value && ref->template_arg_is_value[i];
    if (arg.is_value) {
      if (!resolve_ast_template_value_arg(owner_tpl, ref, i, ctx, &arg.value)) {
        arg.value = ref->template_arg_values ? ref->template_arg_values[i] : 0;
      }
    } else if (ref->template_arg_types) {
      TypeSpec arg_ts = ref->template_arg_types[i];
      if (ctx && arg_ts.base == TB_TYPEDEF) {
        const int outer_ptr_level = arg_ts.ptr_level;
        const bool outer_lref = arg_ts.is_lvalue_ref;
        const bool outer_rref = arg_ts.is_rvalue_ref;
        const bool outer_const = arg_ts.is_const;
        const bool outer_volatile = arg_ts.is_volatile;
        const TypeSpec* bound = nullptr;
        const TextId param_text_id =
            arg_ts.template_param_text_id != kInvalidText
                ? arg_ts.template_param_text_id
                : arg_ts.tag_text_id;
        if (param_text_id != kInvalidText) {
          auto text_it = ctx->tpl_bindings_by_text.find(param_text_id);
          if (text_it != ctx->tpl_bindings_by_text.end()) bound = &text_it->second;
        }
        if (!bound && !typespec_has_structured_template_param_metadata(arg_ts)) {
          const char* legacy_tag = typespec_legacy_tag_if_present(arg_ts, 0);
          if (legacy_tag && legacy_tag[0]) {
            auto it = ctx->tpl_bindings.find(legacy_tag);
            if (it != ctx->tpl_bindings.end()) bound = &it->second;
          }
        }
        if (bound) {
          arg_ts = *bound;
          arg_ts.ptr_level += outer_ptr_level;
          arg_ts.is_lvalue_ref = arg_ts.is_lvalue_ref || outer_lref;
          arg_ts.is_rvalue_ref =
              !arg_ts.is_lvalue_ref && (arg_ts.is_rvalue_ref || outer_rref);
          arg_ts.is_const = arg_ts.is_const || outer_const;
          arg_ts.is_volatile = arg_ts.is_volatile || outer_volatile;
        }
      }
      TypeBindings tpl_empty;
      NttpBindings nttp_empty;
      seed_and_resolve_pending_template_type_if_needed(
          arg_ts, ctx ? ctx->tpl_bindings : tpl_empty,
          ctx ? ctx->nttp_bindings : nttp_empty, span_node, kind, context_name);
      while (resolve_struct_member_typedef_if_ready(&arg_ts)) {
      }
      if (arg_ts.deferred_member_type_name) {
        TypeSpec resolved_member{};
        std::optional<std::string> owner_tag =
            structured_owner_tag_from_type(arg_ts, module_);
        if (!owner_tag) {
          owner_tag = legacy_owner_tag_from_type_if_no_metadata(arg_ts, module_);
        }
        if (owner_tag &&
            resolve_struct_member_typedef_type(
                *owner_tag, arg_ts.deferred_member_type_name, &resolved_member)) {
          arg_ts = resolved_member;
        }
      }
      resolve_typedef_to_struct(arg_ts);
      arg.type = arg_ts;
    }
    actual_args.push_back(arg);
  }

  assign_template_arg_refs_from_hir_args(ts, actual_args);
}

bool Lowerer::resolve_ast_template_value_arg(
    const Node* owner_tpl,
    const Node* ref,
    int index,
    const FunctionCtx* ctx,
    long long* out_value,
    const char** out_debug_ref) {
  if (!ref || index < 0 || index >= ref->n_template_args || !out_value) return false;
  const char* nttp_name =
      (ref->template_arg_nttp_names && ref->template_arg_nttp_names[index])
          ? ref->template_arg_nttp_names[index]
          : nullptr;
  if (out_debug_ref) *out_debug_ref = nttp_name;
  if (ref->template_arg_exprs && ref->template_arg_exprs[index]) {
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv env = make_lowerer_consteval_env(
        structured_maps, ctx ? &ctx->local_const_bindings : nullptr);
    if (ctx) {
      env.nttp_bindings = &ctx->nttp_bindings;
      if (!ctx->nttp_bindings_by_text.empty()) {
        env.nttp_bindings_by_text = &ctx->nttp_bindings_by_text;
      }
    }
    auto expr_value = evaluate_constant_expr(ref->template_arg_exprs[index], env);
    if (expr_value.ok()) {
      *out_value = expr_value.as_int();
      return true;
    }
    if (try_eval_template_value_arg_expr(ref->template_arg_exprs[index], ctx,
                                         out_value)) {
      return true;
    }
  }
  if (ctx && nttp_name && nttp_name[0]) {
    const TextId forwarded_text_id =
        ref->template_arg_nttp_text_ids ? ref->template_arg_nttp_text_ids[index]
                                        : kInvalidText;
    if (forwarded_text_id != kInvalidText &&
        !ctx->nttp_bindings_by_text.empty()) {
      auto text_it = ctx->nttp_bindings_by_text.find(forwarded_text_id);
      if (text_it != ctx->nttp_bindings_by_text.end()) {
        *out_value = text_it->second;
        return true;
      }
    }
    if (auto nttp_value = lookup_nttp_binding(ctx, nullptr, nttp_name)) {
      *out_value = *nttp_value;
      return true;
    }
    if (owner_tpl && owner_tpl->n_template_params > 0 &&
        is_deferred_nttp_expr_ref(nttp_name)) {
      std::vector<std::pair<std::string, TypeSpec>> type_env;
      std::vector<std::pair<std::string, long long>> nttp_env;
      if (ctx) {
        type_env.reserve(ctx->tpl_bindings.size());
        for (const auto& [name, ts] : ctx->tpl_bindings) type_env.push_back({name, ts});
        nttp_env.reserve(ctx->nttp_bindings.size());
        for (const auto& [name, val] : ctx->nttp_bindings) nttp_env.push_back({name, val});
      }
      std::string expr = deferred_nttp_expr_text(nttp_name);
      if (eval_deferred_nttp_expr_hir(
              owner_tpl, 0, type_env, nttp_env, &expr, out_value)) {
        return true;
      }
    }
  }
  *out_value = ref->template_arg_values ? ref->template_arg_values[index] : 0;
  return ref->template_arg_values != nullptr;
}

bool Lowerer::try_eval_template_value_arg_expr(
    const Node* expr,
    const FunctionCtx* ctx,
    long long* out_value) {
  if (!expr || !out_value) return false;
  LowererConstEvalStructuredMaps structured_maps;
  ConstEvalEnv env = make_lowerer_consteval_env(
      structured_maps, ctx ? &ctx->local_const_bindings : nullptr);
  if (ctx) {
    env.nttp_bindings = &ctx->nttp_bindings;
    if (!ctx->nttp_bindings_by_text.empty()) {
      env.nttp_bindings_by_text = &ctx->nttp_bindings_by_text;
    }
  }
  if (auto value = evaluate_constant_expr(expr, env); value.ok()) {
    *out_value = value.as_int();
    return true;
  }

  switch (expr->kind) {
    case NK_INT_LIT:
    case NK_CHAR_LIT:
      *out_value = expr->ival;
      return true;
    case NK_CAST:
      return try_eval_template_value_arg_expr(expr->left, ctx, out_value);
    case NK_UNARY: {
      long long value = 0;
      if (!expr->op || !try_eval_template_value_arg_expr(expr->left, ctx, &value)) {
        return false;
      }
      if (std::strcmp(expr->op, "+") == 0) {
        *out_value = value;
        return true;
      }
      if (std::strcmp(expr->op, "-") == 0) {
        *out_value = -value;
        return true;
      }
      if (std::strcmp(expr->op, "~") == 0) {
        *out_value = ~value;
        return true;
      }
      if (std::strcmp(expr->op, "!") == 0) {
        *out_value = !value;
        return true;
      }
      return false;
    }
    case NK_BINOP: {
      if (!expr->op) return false;
      long long lhs = 0;
      if (!try_eval_template_value_arg_expr(expr->left, ctx, &lhs)) return false;
      if (std::strcmp(expr->op, "&&") == 0 && !lhs) {
        *out_value = 0;
        return true;
      }
      if (std::strcmp(expr->op, "||") == 0 && lhs) {
        *out_value = 1;
        return true;
      }
      long long rhs = 0;
      if (!try_eval_template_value_arg_expr(expr->right, ctx, &rhs)) return false;
      if (std::strcmp(expr->op, "+") == 0) *out_value = lhs + rhs;
      else if (std::strcmp(expr->op, "-") == 0) *out_value = lhs - rhs;
      else if (std::strcmp(expr->op, "*") == 0) *out_value = lhs * rhs;
      else if (std::strcmp(expr->op, "/") == 0) {
        if (rhs == 0) return false;
        *out_value = lhs / rhs;
      } else if (std::strcmp(expr->op, "%") == 0) {
        if (rhs == 0) return false;
        *out_value = lhs % rhs;
      } else if (std::strcmp(expr->op, "<<") == 0) *out_value = lhs << rhs;
      else if (std::strcmp(expr->op, ">>") == 0) *out_value = lhs >> rhs;
      else if (std::strcmp(expr->op, "&") == 0) *out_value = lhs & rhs;
      else if (std::strcmp(expr->op, "|") == 0) *out_value = lhs | rhs;
      else if (std::strcmp(expr->op, "^") == 0) *out_value = lhs ^ rhs;
      else if (std::strcmp(expr->op, "&&") == 0) *out_value = lhs && rhs;
      else if (std::strcmp(expr->op, "||") == 0) *out_value = lhs || rhs;
      else if (std::strcmp(expr->op, "==") == 0) *out_value = lhs == rhs;
      else if (std::strcmp(expr->op, "!=") == 0) *out_value = lhs != rhs;
      else if (std::strcmp(expr->op, "<") == 0) *out_value = lhs < rhs;
      else if (std::strcmp(expr->op, "<=") == 0) *out_value = lhs <= rhs;
      else if (std::strcmp(expr->op, ">") == 0) *out_value = lhs > rhs;
      else if (std::strcmp(expr->op, ">=") == 0) *out_value = lhs >= rhs;
      else return false;
      return true;
    }
    case NK_TERNARY: {
      long long cond = 0;
      if (!try_eval_template_value_arg_expr(expr->cond, ctx, &cond)) return false;
      return try_eval_template_value_arg_expr(cond ? expr->then_ : expr->else_,
                                             ctx, out_value);
    }
    case NK_VAR: {
      if (!expr->name || !expr->name[0]) return false;
      if (auto nttp_value = lookup_nttp_binding(ctx, expr, expr->name)) {
        *out_value = *nttp_value;
        return true;
      }
      const std::string qname = expr->name;
      const std::string member = unqualified_name(qname);
      const std::string owner = qualified_owner_name(qname);
      if (owner.empty() || member.empty()) return false;
      if (auto value = try_eval_instantiated_struct_static_member_const(
              owner, member)) {
        *out_value = *value;
        return true;
      }
      if (!(expr->has_template_args || expr->n_template_args > 0)) return false;
      if (auto value = try_eval_template_static_member_const(
              const_cast<FunctionCtx*>(ctx), owner, expr, member)) {
        *out_value = *value;
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

std::optional<long long> Lowerer::try_eval_template_static_member_const(
    FunctionCtx* ctx,
    const std::string& tpl_name,
    const Node* ref,
    const std::string& member) {
  const Node* primary = nullptr;
  if (ref && ref->qualifier_text_ids && ref->n_qualifier_segments > 0) {
    const TextId owner_text_id =
        ref->qualifier_text_ids[ref->n_qualifier_segments - 1];
    if (owner_text_id != kInvalidText) {
      NamespaceQualifier owner_ns;
      owner_ns.context_id = ref->namespace_context_id;
      owner_ns.is_global_qualified = ref->is_global_qualified;
      owner_ns.segment_text_ids.reserve(ref->n_qualifier_segments - 1);
      for (int i = 0; i + 1 < ref->n_qualifier_segments; ++i) {
        if (ref->qualifier_text_ids[i] == kInvalidText) {
          owner_ns.segment_text_ids.clear();
          break;
        }
        owner_ns.segment_text_ids.push_back(ref->qualifier_text_ids[i]);
      }
      HirRecordOwnerKey owner_key =
          make_hir_record_owner_key(owner_ns, owner_text_id);
      if (hir_record_owner_key_has_complete_metadata(owner_key)) {
        auto it = template_struct_defs_by_owner_.find(owner_key);
        if (it != template_struct_defs_by_owner_.end()) primary = it->second;
      }
    }
  }
  if (!primary) primary = find_template_struct_primary(tpl_name);
  if (!primary && tpl_name.find("::") == std::string::npos) {
    const Node* unique_match = nullptr;
    const std::string suffix = "::" + tpl_name;
    for (const auto& [registered_name, registered_node] : template_struct_defs_) {
      if (registered_name == tpl_name ||
          (registered_name.size() > suffix.size() &&
           registered_name.compare(
               registered_name.size() - suffix.size(),
               suffix.size(), suffix) == 0)) {
        if (unique_match && unique_match != registered_node) {
          unique_match = nullptr;
          break;
        }
        unique_match = registered_node;
      }
    }
    primary = unique_match;
  }
  if (!primary && ref && ref->n_qualifier_segments > 0 &&
      ref->qualifier_segments &&
      ref->qualifier_segments[ref->n_qualifier_segments - 1]) {
    primary = find_template_struct_primary(
        ref->qualifier_segments[ref->n_qualifier_segments - 1]);
  }
  if (!primary || !ref) return std::nullopt;

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
        const int outer_ptr_level = ts.ptr_level;
        const bool outer_lref = ts.is_lvalue_ref;
        const bool outer_rref = ts.is_rvalue_ref;
        const bool outer_const = ts.is_const;
        const bool outer_volatile = ts.is_volatile;
        const TypeSpec* bound = nullptr;
        const TextId param_text_id =
            ts.template_param_text_id != kInvalidText
                ? ts.template_param_text_id
                : ts.tag_text_id;
        if (param_text_id != kInvalidText) {
          auto text_it = ctx->tpl_bindings_by_text.find(param_text_id);
          if (text_it != ctx->tpl_bindings_by_text.end()) bound = &text_it->second;
        }
        if (!bound && !typespec_has_structured_template_param_metadata(ts)) {
          const char* legacy_tag = typespec_legacy_tag_if_present(ts, 0);
          if (legacy_tag && legacy_tag[0]) {
            auto it = ctx->tpl_bindings.find(legacy_tag);
            if (it != ctx->tpl_bindings.end()) bound = &it->second;
          }
        }
        if (bound) {
          ts = *bound;
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
          PendingTemplateTypeKind::DeclarationType, "template-static-member");
      while (resolve_struct_member_typedef_if_ready(&ts)) {
      }
      if (ts.deferred_member_type_name) {
        TypeSpec resolved_member{};
        std::optional<std::string> owner_tag =
            structured_owner_tag_from_type(ts, module_);
        if (!owner_tag) {
          owner_tag = legacy_owner_tag_from_type_if_no_metadata(ts, module_);
        }
        if (owner_tag &&
            resolve_struct_member_typedef_type(*owner_tag, ts.deferred_member_type_name,
                                               &resolved_member)) {
          ts = resolved_member;
        }
      }
      resolve_typedef_to_struct(ts);
      arg.type = ts;
    }
    actual_args.push_back(arg);
  }
  if (primary->template_param_has_default &&
      static_cast<int>(actual_args.size()) < primary->n_template_params) {
    std::vector<std::pair<std::string, TypeSpec>> type_env;
    std::vector<std::pair<std::string, long long>> nttp_env;
    for (int i = 0; i < static_cast<int>(actual_args.size()) &&
                    i < primary->n_template_params; ++i) {
      if (!primary->template_param_names || !primary->template_param_names[i]) {
        continue;
      }
      if (actual_args[i].is_value) {
        nttp_env.push_back({primary->template_param_names[i], actual_args[i].value});
      } else {
        type_env.push_back({primary->template_param_names[i], actual_args[i].type});
      }
    }
    for (int i = static_cast<int>(actual_args.size());
         i < primary->n_template_params; ++i) {
      if (!primary->template_param_has_default[i]) break;
      HirTemplateArg arg{};
      arg.is_value =
          primary->template_param_is_nttp && primary->template_param_is_nttp[i];
      if (arg.is_value) {
        long long value = primary->template_param_default_values
                              ? primary->template_param_default_values[i]
                              : 0;
        if (value == LLONG_MIN) {
          if (!eval_deferred_nttp_expr_hir(
                  primary, i, type_env, nttp_env, nullptr, &value)) {
            break;
          }
        }
        arg.value = value;
        if (primary->template_param_names && primary->template_param_names[i]) {
          nttp_env.push_back({primary->template_param_names[i], value});
        }
      } else if (primary->template_param_default_types) {
        TypeSpec ts = primary->template_param_default_types[i];
        TypeBindings type_bindings;
        NttpBindings nttp_bindings;
        for (const auto& [name, type] : type_env) type_bindings[name] = type;
        for (const auto& [name, value] : nttp_env) nttp_bindings[name] = value;
        seed_and_resolve_pending_template_type_if_needed(
            ts, type_bindings, nttp_bindings, ref,
            PendingTemplateTypeKind::DeclarationType,
            "template-static-member-default");
        arg.type = ts;
        if (primary->template_param_names && primary->template_param_names[i]) {
          type_env.push_back({primary->template_param_names[i], ts});
        }
      }
      actual_args.push_back(arg);
    }
  }

  if (member == "value" && actual_args.size() == 1 && !actual_args[0].is_value) {
    if (matches_trait_family(tpl_name, "is_signed")) {
      return is_signed_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_unsigned")) {
      return is_unsigned_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_const")) {
      return is_const_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_enum")) {
      return is_enum_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_reference")) {
      if (is_reference_trait_type(actual_args[0].type)) {
        return 1LL;
      }
      std::optional<std::string> structured_owner_tag =
          structured_owner_tag_from_type(actual_args[0].type, module_);
      std::optional<std::string> legacy_owner_tag;
      if (!structured_owner_tag) {
        legacy_owner_tag =
            legacy_owner_tag_from_type_if_no_metadata(actual_args[0].type, module_);
      }
      const std::optional<std::string> owner_tag =
          structured_owner_tag ? structured_owner_tag : legacy_owner_tag;
      if (owner_tag) {
        TypeSpec resolved_member{};
        if (resolve_struct_member_typedef_type(
                *owner_tag, "type", &resolved_member)) {
          return is_reference_trait_type(resolved_member) ? 1LL : 0LL;
        }
        auto owner_it = struct_def_nodes_.find(*owner_tag);
        if (owner_it != struct_def_nodes_.end() && owner_it->second) {
          const Node* owner = owner_it->second;
          const char* origin =
              (owner->template_origin_name && owner->template_origin_name[0])
                  ? owner->template_origin_name
                  : owner->name;
          if (origin &&
              (matches_trait_family(origin, "add_lvalue_reference") ||
               matches_trait_family(origin, "add_rvalue_reference"))) {
            return 1LL;
          }
        }
        if (legacy_owner_tag &&
            (legacy_owner_tag->find("::add_lvalue_reference_") != std::string::npos ||
             legacy_owner_tag->find("::add_rvalue_reference_") != std::string::npos)) {
          return 1LL;
        }
      }
      return 0LL;
    }
  }
  if (member == "value" && actual_args.size() == 2 &&
      !actual_args[0].is_value && !actual_args[1].is_value) {
    if (matches_trait_family(tpl_name, "is_same")) {
      return canonical_type_str(actual_args[0].type) ==
                     canonical_type_str(actual_args[1].type)
                 ? 1LL
                 : 0LL;
    }
  }
  TemplateStructEnv env = build_template_struct_env(primary);
  SelectedTemplateStructPattern selected =
      select_template_struct_pattern_hir(actual_args, env);
  if (selected.selected_pattern) {
    long long value = 0;
    if (eval_struct_static_member_value_hir(
            selected.selected_pattern, struct_def_nodes_, member,
            &selected.nttp_bindings, &value)) {
      return value;
    }
  }
  return std::nullopt;
}

std::optional<long long> Lowerer::try_eval_instantiated_struct_static_member_const(
    const std::string& tag,
    const std::string& member) const {
  auto it = struct_def_nodes_.find(tag);
  if (it == struct_def_nodes_.end() || !it->second) return std::nullopt;
  const Node* owner = it->second;
  const char* origin =
      (owner->template_origin_name && owner->template_origin_name[0])
          ? owner->template_origin_name
          : nullptr;
  if (!origin || owner->n_template_args <= 0) return std::nullopt;

  std::vector<HirTemplateArg> actual_args;
  actual_args.reserve(owner->n_template_args);
  for (int i = 0; i < owner->n_template_args; ++i) {
    HirTemplateArg arg{};
    arg.is_value =
        owner->template_arg_is_value && owner->template_arg_is_value[i];
    if (arg.is_value) {
      arg.value = owner->template_arg_values ? owner->template_arg_values[i] : 0;
    } else if (owner->template_arg_types) {
      arg.type = owner->template_arg_types[i];
    }
    actual_args.push_back(arg);
  }

  const std::string tpl_name = origin;
  if (member == "value" && actual_args.size() == 1 && !actual_args[0].is_value) {
    if (matches_trait_family(tpl_name, "is_signed")) {
      return is_signed_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_unsigned")) {
      return is_unsigned_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_const")) {
      return is_const_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_enum")) {
      return is_enum_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
    if (matches_trait_family(tpl_name, "is_reference")) {
      return is_reference_trait_type(actual_args[0].type) ? 1LL : 0LL;
    }
  }
  if (member == "value" && actual_args.size() == 2 &&
      !actual_args[0].is_value && !actual_args[1].is_value &&
      matches_trait_family(tpl_name, "is_same")) {
    return canonical_type_str(actual_args[0].type) ==
                   canonical_type_str(actual_args[1].type)
               ? 1LL
               : 0LL;
  }
  NttpBindings selected_nttp_bindings;
  if (owner->template_param_names) {
    for (int i = 0; i < owner->n_template_args && i < owner->n_template_params; ++i) {
      if (!owner->template_param_is_nttp || !owner->template_param_is_nttp[i]) {
        continue;
      }
      const char* param_name = owner->template_param_names[i];
      if (!param_name || !param_name[0]) continue;
      selected_nttp_bindings[param_name] =
          owner->template_arg_values ? owner->template_arg_values[i] : 0;
    }
  }
  long long value = 0;
  if (eval_struct_static_member_value_hir(
          owner, struct_def_nodes_, member, &selected_nttp_bindings, &value)) {
    return value;
  }
  return std::nullopt;
}

}  // namespace c4c::hir
