#include "hir_to_lir.hpp"
#include "call_args_ops.hpp"
#include "hir_ir.hpp"
#include "lowering.hpp"
#include "../../llvm/calling_convention.hpp"
#include "../../shared/llvm_helpers.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;

int object_align_bytes(const c4c::hir::Module& mod, const LirModule* lir_module,
                       const TypeSpec& ts);
int object_align_bytes(const c4c::hir::Module& mod, const TypeSpec& ts);

namespace {

int align_to_bytes(int value, int align) {
  if (align <= 1) return value;
  const int rem = value % align;
  return rem == 0 ? value : value + (align - rem);
}

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

std::optional<LirTypeRef> local_array_type_ref_from_typespec(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return LirTypeRef(LirBuiltinType::Pointer);
  if (ts.array_rank > 0) {
    if (ts.array_size < 0) return std::nullopt;
    TypeSpec elem_ts = ts;
    elem_ts.array_rank--;
    if (elem_ts.array_rank > 0) {
      for (int i = 0; i < elem_ts.array_rank; ++i) {
        elem_ts.array_dims[i] = elem_ts.array_dims[i + 1];
      }
    }
    elem_ts.array_size = elem_ts.array_rank > 0 ? elem_ts.array_dims[0] : -1;
    std::optional<LirTypeRef> elem_ref = local_array_type_ref_from_typespec(elem_ts);
    if (!elem_ref) return std::nullopt;
    return LirTypeRef::array(std::move(*elem_ref),
                             static_cast<std::size_t>(ts.array_size));
  }
  return LirTypeRef(c4c::codegen::llvm_helpers::llvm_ty(ts));
}

std::optional<LirTypeRef> indexed_local_array_element_type_ref(
    const TypeSpec& array_ts) {
  if (array_ts.array_rank == 0) return std::nullopt;
  TypeSpec element = array_ts;
  --element.array_rank;
  for (int i = 0; i < element.array_rank; ++i) {
    element.array_dims[i] = element.array_dims[i + 1];
  }
  element.array_size = element.array_rank > 0 ? element.array_dims[0] : -1;
  return local_array_type_ref_from_typespec(element);
}

LirTypeRef lir_signature_type_ref(const std::string& rendered_text,
                                  LirModule* lir_module,
                                  const c4c::hir::Module& mod,
                                  const TypeSpec& type);

[[deprecated(
    "HIR-rendered AArch64 vector ABI source type text: audit this runtime-text "
    "compatibility boundary")]]
LirTypeRef hir_rendered_aarch64_vector_abi_source_type_text(
    std::string rendered_text) {
  return LirTypeRef::runtime_text(std::move(rendered_text));
}

bool is_aarch64_fixed_hfa_param(const c4c::hir::Module& mod, const TypeSpec& ts) {
  using namespace c4c::codegen::llvm_helpers;
  return llvm_target_is_aarch64(mod.target_profile) &&
         !llvm_target_is_apple(mod.target_profile) &&
         stmt_emitter_detail::classify_aarch64_hfa(mod, ts).has_value();
}

TypeSpec aarch64_hfa_lane_type(const stmt_emitter_detail::Aarch64HomogeneousFpAggregateInfo& hfa) {
  TypeSpec lane{};
  if (hfa.elem_ty == "float") {
    lane.base = TB_FLOAT;
  } else if (hfa.elem_ty == "double") {
    lane.base = TB_DOUBLE;
  } else {
    lane.base = TB_LONGDOUBLE;
  }
  return lane;
}

std::string aarch64_hfa_lane_name(std::string_view base_name, int lane_index) {
  return std::string(base_name) + ".hfa" + std::to_string(lane_index);
}

void append_aarch64_hfa_signature_params(const c4c::hir::Module& mod,
                                         const TypeSpec& param_ts,
                                         std::string_view pname,
                                         LirModule* lir_module,
                                         LirFunction& lir_fn) {
  const auto hfa = stmt_emitter_detail::classify_aarch64_hfa(mod, param_ts);
  if (!hfa.has_value()) return;
  const TypeSpec lane_ts = aarch64_hfa_lane_type(*hfa);
  for (int lane_index = 0; lane_index < hfa->elem_count; ++lane_index) {
    lir_fn.signature_params.push_back(
        {aarch64_hfa_lane_name(pname, lane_index), lane_ts, false});
    lir_fn.signature_param_type_refs.push_back(
        lir_signature_type_ref(hfa->elem_ty, lir_module, mod, lane_ts));
  }
}

std::optional<std::string> unique_template_instance_aggregate_ty(
    const c4c::hir::Module& mod, const TypeSpec& ts) {
  std::vector<TextId> primary_text_ids;
  auto add_primary_id = [&](TextId id) {
    if (id == kInvalidText) return;
    if (std::find(primary_text_ids.begin(), primary_text_ids.end(), id) ==
        primary_text_ids.end()) {
      primary_text_ids.push_back(id);
    }
  };
  add_primary_id(ts.tpl_struct_origin_key.base_text_id);
  if (ts.record_def) add_primary_id(ts.record_def->unqualified_text_id);
  add_primary_id(ts.tag_text_id);
  if (mod.link_name_texts && ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
    std::string_view origin = ts.tpl_struct_origin;
    const size_t scope_pos = origin.rfind("::");
    if (scope_pos != std::string_view::npos) origin.remove_prefix(scope_pos + 2);
    add_primary_id(mod.link_name_texts->find(origin));
  }
  if (primary_text_ids.empty()) return std::nullopt;

  const SymbolName* match = nullptr;
  const int wanted_context_id =
      ts.namespace_context_id >= 0 ? ts.namespace_context_id
                                   : ts.tpl_struct_origin_key.context_id;
  for (const auto& [owner_key, rendered_tag] : mod.struct_def_owner_index) {
    if (owner_key.kind != HirRecordOwnerKeyKind::TemplateInstantiation ||
        mod.struct_defs.count(rendered_tag) == 0) {
      continue;
    }
    if (std::find(primary_text_ids.begin(), primary_text_ids.end(),
                  owner_key.declaration_text_id) == primary_text_ids.end()) {
      continue;
    }
    if (wanted_context_id >= 0 &&
        owner_key.namespace_context_id != wanted_context_id) {
      continue;
    }
    if (match && *match != rendered_tag) return std::nullopt;
    match = &rendered_tag;
  }
  if (!match) return std::nullopt;
  return c4c::codegen::llvm_helpers::llvm_struct_type_str(*match);
}

std::optional<std::string> unique_template_specialization_decl_ty(
    const c4c::hir::Module& mod, const TypeSpec& ts) {
  if (!ts.record_def) return std::nullopt;
  const char* record_name = ts.record_def->name && ts.record_def->name[0]
                                ? ts.record_def->name
                                : ts.record_def->unqualified_name;
  if (!record_name || !record_name[0]) return std::nullopt;
  const std::string prefix = std::string(record_name) + "_";
  const std::string* match = nullptr;
  for (const auto& [tag, def] : mod.struct_defs) {
    (void)def;
    if (tag.rfind(prefix, 0) != 0) continue;
    if (match) return std::nullopt;
    match = &tag;
  }
  if (!match) return std::nullopt;
  return c4c::codegen::llvm_helpers::llvm_struct_type_str(*match);
}

std::optional<std::string> unique_decl_ty_for_tag_text(const c4c::hir::Module& mod,
                                                       const TypeSpec& ts) {
  if ((ts.base != TB_STRUCT && ts.base != TB_UNION) ||
      ts.tag_text_id == kInvalidText || !mod.link_name_texts) {
    return std::nullopt;
  }
  const std::string_view tag_text = mod.link_name_texts->lookup(ts.tag_text_id);
  if (tag_text.empty()) return std::nullopt;
  const std::string prefix = std::string(tag_text) + "_T";
  const std::string* match = nullptr;
  const auto same_namespace_context = [](int lhs, int rhs) {
    if (lhs == rhs) return true;
    return (lhs == 0 && rhs < 0) || (lhs < 0 && rhs == 0);
  };
  for (const auto& [tag, def] : mod.struct_defs) {
    if (ts.namespace_context_id >= 0 &&
        !same_namespace_context(def.ns_qual.context_id, ts.namespace_context_id)) {
      continue;
    }
    std::string_view unqualified = tag;
    const size_t scope_pos = unqualified.rfind("::");
    if (scope_pos != std::string_view::npos) {
      unqualified.remove_prefix(scope_pos + 2);
    }
    if (unqualified != tag_text && unqualified.rfind(prefix, 0) != 0) continue;
    if (match) return std::nullopt;
    match = &tag;
  }
  if (!match) return std::nullopt;
  if (ts.base == TB_UNION) return "%union." + *match;
  return c4c::codegen::llvm_helpers::llvm_struct_type_str(*match);
}

std::optional<std::string> tag_from_structured_lir_name(std::string_view name,
                                                        bool is_union) {
  (void)is_union;
  for (const std::string_view plain_prefix : {"%struct.", "%union."}) {
    if (name.rfind(plain_prefix, 0) == 0) {
      return std::string(name.substr(plain_prefix.size()));
    }
  }

  for (const std::string_view quoted_prefix : {"%\"struct.", "%\"union."}) {
    if (name.rfind(quoted_prefix, 0) == 0 &&
        name.size() >= quoted_prefix.size() + 1 && name.back() == '"') {
      return std::string(name.substr(quoted_prefix.size(),
                                     name.size() - quoted_prefix.size() - 1));
    }
  }
  return std::nullopt;
}

std::optional<std::string> direct_owned_aggregate_type_text(
    const c4c::hir::Module& mod, const TypeSpec& type,
    const LirModule* lir_module) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0 || type.tag_text_id == kInvalidText ||
      !mod.link_name_texts) {
    return std::nullopt;
  }
  const std::string_view tag = mod.link_name_texts->lookup(type.tag_text_id);
  if (tag.empty()) return std::nullopt;
  if (tag.rfind("%struct.", 0) == 0 || tag.rfind("%\"struct.", 0) == 0 ||
      tag.rfind("%union.", 0) == 0 || tag.rfind("%\"union.", 0) == 0) {
    return std::string(tag);
  }
  if (lir_module) {
    const std::string struct_name =
        c4c::codegen::llvm_helpers::llvm_struct_type_str(std::string(tag));
    const c4c::StructNameId struct_id =
        lir_module->struct_names.find(struct_name);
    if (struct_id != c4c::kInvalidStructName &&
        lir_module->find_struct_decl(struct_id)) {
      return struct_name;
    }
    const std::string union_name = "%union." + std::string(tag);
    const c4c::StructNameId union_id =
        lir_module->struct_names.find(union_name);
    if (union_id != c4c::kInvalidStructName &&
        lir_module->find_struct_decl(union_id)) {
      return union_name;
    }
  }
  if (type.base == TB_UNION) return "%union." + std::string(tag);
  return c4c::codegen::llvm_helpers::llvm_struct_type_str(std::string(tag));
}

std::optional<std::string> declared_lir_aggregate_type_text_for_tag(
    const c4c::hir::Module& mod, const TypeSpec& type,
    const LirModule* lir_module) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) ||
      type.tag_text_id == kInvalidText || !mod.link_name_texts || !lir_module) {
    return std::nullopt;
  }
  const std::string_view tag = mod.link_name_texts->lookup(type.tag_text_id);
  if (tag.empty()) return std::nullopt;
  for (const std::string& rendered :
       {c4c::codegen::llvm_helpers::llvm_struct_type_str(std::string(tag)),
        "%union." + std::string(tag)}) {
    const c4c::StructNameId name_id = lir_module->struct_names.find(rendered);
    if (name_id != c4c::kInvalidStructName &&
        lir_module->find_struct_decl(name_id)) {
      return rendered;
    }
  }
  return std::nullopt;
}

TypeSpec lir_owned_type_spec(const c4c::hir::Module& mod, const QualType& hir_type,
                             LirModule* lir_module) {
  TypeSpec type = hir_type.spec;
  if (!lir_module) return type;
  if (type.base != TB_STRUCT && type.base != TB_UNION) return type;
  // LIR owns this copy. `record_def` and qualifier arrays point into parser
  // storage, so they cannot participate in aggregate ownership after the HIR
  // function is materialized. Resolve only the retained structured key.
  type.record_def = nullptr;
  type.qualifier_segments = nullptr;
  type.qualifier_text_ids = nullptr;
  type.n_qualifier_segments = 0;
  if (hir_type.aggregate_ref) {
    if (hir_type.aggregate_owner_identity &&
        !c4c::codegen::llvm_helpers::typespec_aggregate_owner_key(hir_type, mod)) {
      throw std::runtime_error(
          "LIR-owned aggregate function type requires coherent HIR owner metadata");
    }
    const LirAggregateRef aggregate_ref =
        lir_module->find_aggregate_ref(mod, *hir_type.aggregate_ref);
    if (!aggregate_ref.valid()) {
      throw std::runtime_error(
          "LIR-owned aggregate function type requires a registered HIR aggregate ref");
    }
    const LirAggregateStoreEntry* aggregate =
        lir_module->find_aggregate(aggregate_ref);
    if (!aggregate || aggregate->is_union != (type.base == TB_UNION)) {
      throw std::runtime_error(
          "LIR-owned aggregate function type has an incoherent HIR aggregate ref");
    }
    const std::string_view name =
        lir_module->struct_names.spelling(aggregate->name_id);
    if (std::optional<std::string> tag =
            tag_from_structured_lir_name(name, type.base == TB_UNION)) {
      type.tag_text_id = lir_module->link_name_texts->intern(*tag);
      return type;
    }
    throw std::runtime_error(
        "LIR-owned aggregate function type requires a structured LIR aggregate name");
  }
  const std::optional<HirRecordOwnerKey> owner_key =
      c4c::codegen::llvm_helpers::typespec_aggregate_owner_key(hir_type, mod);
  if (!owner_key) {
    return type;
  }
  const SymbolName* owner_tag = mod.find_struct_def_tag_by_owner(*owner_key);
  std::optional<std::string> tag =
      owner_tag && !owner_tag->empty() ? std::optional<std::string>(std::string(*owner_tag))
                                       : std::nullopt;
  if (!tag) {
    tag = c4c::codegen::llvm_helpers::typespec_aggregate_compatibility_tag(
        mod, type);
    if (tag) {
      const std::string structured_name =
          type.base == TB_UNION ? "%union." + *tag
                                : c4c::codegen::llvm_helpers::llvm_struct_type_str(*tag);
      const c4c::StructNameId name_id =
          lir_module->struct_names.find(structured_name);
      if (name_id == c4c::kInvalidStructName ||
          !lir_module->find_struct_decl(name_id)) {
        tag.reset();
      }
    }
  }
  if (!tag && (type.base == TB_STRUCT || type.base == TB_UNION)) {
    std::optional<std::string> structured_type =
        unique_template_instance_aggregate_ty(mod, hir_type.spec);
    if (!structured_type) {
      structured_type = unique_template_specialization_decl_ty(mod, hir_type.spec);
    }
    if (!structured_type) {
      structured_type = unique_decl_ty_for_tag_text(mod, hir_type.spec);
    }
    if (!structured_type) {
      structured_type =
          declared_lir_aggregate_type_text_for_tag(mod, hir_type.spec, lir_module);
    }
    if (structured_type) {
      const c4c::StructNameId name_id =
          lir_module->struct_names.find(*structured_type);
      if (name_id != c4c::kInvalidStructName &&
          lir_module->find_struct_decl(name_id)) {
        tag = tag_from_structured_lir_name(*structured_type,
                                           type.base == TB_UNION);
      }
    }
  }
  if (!tag) {
    const bool root_namespace_type =
        type.namespace_context_id == 0 || type.namespace_context_id < 0;
    if ((!hir_type.aggregate_owner_identity || root_namespace_type) &&
        !declared_lir_aggregate_type_text_for_tag(mod, hir_type.spec, lir_module)) {
      return type;
    }
    throw std::runtime_error("LIR-owned aggregate function type requires a matching module owner");
  }
  const std::optional<std::string> source_tag =
      tag ? tag : c4c::codegen::llvm_helpers::typespec_aggregate_compatibility_tag(
                mod, type);
  const bool source_is_template_local =
      source_tag && source_tag->find("_tag_ctx") != std::string::npos;
  if (source_is_template_local) {
    if (const std::optional<std::string> canonical_ty =
            stmt_emitter_detail::llvm_aggregate_value_ty(mod, type)) {
      const std::string current_ty =
          tag ? c4c::codegen::llvm_helpers::llvm_struct_type_str(*tag)
              : std::string{};
      if ((current_ty.empty() || *canonical_ty != current_ty) &&
          canonical_ty->rfind("%struct.", 0) == 0) {
        tag = canonical_ty->substr(std::string_view("%struct.").size());
      }
    }
  }
  if (tag && lir_module->link_name_texts) {
    type.tag_text_id = lir_module->link_name_texts->intern(*tag);
  }
  return type;
}

LirTypeRef lir_aggregate_type_ref(const std::string& rendered_text, LirModule* lir_module,
                                  StructNameId name_id, bool is_union) {
  if (!lir_module || name_id == kInvalidStructName) {
    return LirTypeRef::hir_rendered_aggregate_field_signature_type_text(rendered_text);
  }
  const std::string_view structured_text = lir_module->struct_names.spelling(name_id);
  const std::string mirror_text =
      structured_text.empty() ? rendered_text : std::string(structured_text);
  return is_union ? LirTypeRef::union_type(mirror_text, name_id)
                  : LirTypeRef::struct_type(mirror_text, name_id);
}

// Step 1 canonical seam: callers that already carry a HIR aggregate ref can
// register it without consulting tags, text, parser pointers, or an owner-key
// reconstruction. The source HIR module validates ownership. Existing lowering
// remains on its legacy adapter path until the next migration packet supplies
// refs at every occurrence.
LirAggregateRef lir_register_aggregate_ref(const c4c::hir::Module& source_module,
                                           LirModule& lir_module,
                                           c4c::hir::HirAggregateRef hir_ref,
                                           StructNameId name_id, bool is_union) {
  return lir_module.register_aggregate(source_module, hir_ref, name_id, is_union);
}

StructNameId lir_aggregate_structured_name_id(const c4c::hir::Module& mod,
                                              LirModule* lir_module,
                                              const std::string& rendered_text,
                                              const TypeSpec& type) {
  using namespace c4c::codegen::llvm_helpers;
  if (!lir_module || (type.base != TB_STRUCT && type.base != TB_UNION) ||
      type.ptr_level > 0 || type.array_rank > 0) {
    return kInvalidStructName;
  }
  auto intern_if_rendered_match = [&](const std::optional<std::string>& tag)
      -> StructNameId {
    if (!tag) return kInvalidStructName;
    const std::string name = llvm_struct_type_str(*tag);
    return name == rendered_text ? lir_module->struct_names.intern(name) : kInvalidStructName;
  };
  auto rendered_text_is_aggregate_name = [&]() {
    return rendered_text.rfind("%struct.", 0) == 0 ||
           rendered_text.rfind("%\"struct.", 0) == 0;
  };

  if (const std::optional<HirRecordOwnerKey> owner_key =
          typespec_aggregate_owner_key(type, mod)) {
    if (!type.record_def && rendered_text_is_aggregate_name()) {
      const StructNameId declared_id = lir_module->struct_names.find(rendered_text);
      if (declared_id != kInvalidStructName &&
          lir_module->find_struct_decl(declared_id)) {
        return declared_id;
      }
    }
    const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
    if (structured_tag && !structured_tag->empty()) {
      const std::string name = llvm_struct_type_str(*structured_tag);
      if (name == rendered_text || rendered_text_is_aggregate_name()) {
        return lir_module->struct_names.intern(name);
      }
    }
    return kInvalidStructName;
  }

  if (const StructNameId declared_id = lir_module->struct_names.find(rendered_text);
      declared_id != kInvalidStructName) {
    return declared_id;
  }

  if (const StructNameId compatibility_id =
          intern_if_rendered_match(typespec_aggregate_compatibility_tag(mod, type));
      compatibility_id != kInvalidStructName) {
    return compatibility_id;
  }
  if (const StructNameId final_spelling_id =
          intern_if_rendered_match(typespec_aggregate_final_spelling(type));
      final_spelling_id != kInvalidStructName) {
    return final_spelling_id;
  }
  return kInvalidStructName;
}

LirTypeRef lir_field_type_ref(const std::string& rendered_text, LirModule* lir_module,
                              const c4c::hir::Module& mod, const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0) {
    return LirTypeRef::hir_rendered_aggregate_field_signature_type_text(rendered_text);
  }
  return lir_aggregate_type_ref(rendered_text, lir_module,
                                lir_aggregate_structured_name_id(mod, lir_module,
                                                                 rendered_text, type),
                                type.base == TB_UNION);
}

std::string lir_field_ty(const c4c::hir::Module& mod, const HirStructField& field) {
  using namespace c4c::codegen::llvm_helpers;
  if (field.array_first_dim >= 0) {
    std::string elem_ty =
        stmt_emitter_detail::llvm_alloca_ty(mod, field.elem_type);
    if (elem_ty == "void") elem_ty = "i8";
    return "[" + std::to_string(field.array_first_dim) + " x " + elem_ty + "]";
  }
  if (field.elem_type.base == TB_VA_LIST) return llvm_va_list_storage_ty();
  if (field.elem_type.base == TB_STRUCT || field.elem_type.base == TB_UNION) {
    return llvm_field_ty(field);
  }
  return stmt_emitter_detail::llvm_value_ty(mod, field.elem_type);
}

LirTypeRef lir_field_type_ref(const HirStructField& field, LirModule* lir_module,
                              const c4c::hir::Module& mod) {
  const std::string field_ty = lir_field_ty(mod, field);
  if (field.array_first_dim >= 0) {
    return LirTypeRef::hir_rendered_aggregate_field_signature_type_text(field_ty);
  }
  return lir_field_type_ref(field_ty, lir_module, mod, field.elem_type);
}

bool packed_bitfield_byte_storage_record(const HirStructDef& sd) {
  if (sd.pack_align != 1 || sd.is_union || !sd.base_tags.empty() ||
      sd.fields.empty() || sd.size_bytes <= 0) {
    return false;
  }
  for (const auto& field : sd.fields) {
    if (field.bit_width < 0 || field.packed_storage_offset_bytes < 0) {
      return false;
    }
  }
  return true;
}

std::optional<LirTypeRef> lir_global_type_ref(const std::string& rendered_text,
                                              LirModule* lir_module,
                                              const c4c::hir::Module& mod,
                                              const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0) {
    return std::nullopt;
  }
  const std::string canonical_text = stmt_emitter_detail::llvm_alloca_ty(mod, type);
  if (canonical_text != rendered_text) return std::nullopt;
  const StructNameId name_id =
      lir_aggregate_structured_name_id(mod, lir_module, canonical_text, type);
  if (name_id == kInvalidStructName) return std::nullopt;
  return lir_aggregate_type_ref(canonical_text, lir_module, name_id, type.base == TB_UNION);
}

LirTypeRef lir_signature_type_ref(const std::string& rendered_text,
                                  LirModule* lir_module,
                                  const c4c::hir::Module& mod,
                                  const TypeSpec& type) {
  using namespace c4c::codegen::llvm_helpers;
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0) {
    return LirTypeRef::hir_rendered_aggregate_field_signature_type_text(rendered_text);
  }
  const StructNameId name_id =
      lir_aggregate_structured_name_id(mod, lir_module, rendered_text, type);
  if (name_id == kInvalidStructName) {
    return LirTypeRef::hir_rendered_aggregate_field_signature_type_text(rendered_text);
  }
  const bool rendered_is_union = rendered_text.rfind("%union.", 0) == 0 ||
                                 rendered_text.rfind("%\"union.", 0) == 0;
  const bool rendered_is_struct = rendered_text.rfind("%struct.", 0) == 0 ||
                                  rendered_text.rfind("%\"struct.", 0) == 0;
  std::optional<bool> store_is_union;
  if (lir_module) {
    for (const LirAggregateStoreEntry& entry : lir_module->aggregate_store) {
      if (entry.name_id == name_id) {
        store_is_union = entry.layout_kind == LirAggregateLayoutKind::Union;
        break;
      }
    }
  }
  const bool is_union = store_is_union.value_or(
      rendered_is_union ? true : (rendered_is_struct ? false : type.base == TB_UNION));
  return lir_aggregate_type_ref(rendered_text, lir_module, name_id, is_union);
}

std::string rendered_signature_return_type(const c4c::hir::Module& mod,
                                           const TypeSpec& return_ts) {
  return stmt_emitter_detail::llvm_return_ty(mod, return_ts);
}

LirExtAttr rv64_variadic_return_ext_attr_for_abi_type(
    const c4c::hir::Module& mod, const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;
  const TypeSpec& ts = fn.return_type.spec;
  if (mod.target_profile.arch != c4c::TargetArch::Riscv64 ||
      !fn.attrs.variadic || ts.ptr_level != 0 || ts.array_rank != 0 ||
      !is_any_int(ts.base) || int_bits(ts.base) != 32) {
    return LirExtAttr::None;
  }
  return is_signed_int(ts.base) ? LirExtAttr::SignExt : LirExtAttr::ZeroExt;
}

std::string_view signature_ext_attr_prefix(LirExtAttr attr) {
  switch (attr) {
    case LirExtAttr::SignExt:
      return "signext ";
    case LirExtAttr::ZeroExt:
      return "zeroext ";
    case LirExtAttr::None:
      return "";
  }
  return "";
}

std::string rendered_signature_param_type(const c4c::hir::Module& mod,
                                          const LirModule* lir_module,
                                          const TypeSpec& param_ts) {
  using namespace c4c::codegen::llvm_helpers;
  const std::optional<std::string> aggregate_ty =
      stmt_emitter_detail::llvm_aggregate_value_ty(mod, param_ts);
  if (llvm_target_is_amd64_sysv(mod.target_profile) &&
      llvm_cc::amd64_fixed_aggregate_passed_byval(param_ts, mod)) {
    return "ptr byval(" + aggregate_ty.value_or(llvm_ty(param_ts)) + ") align " +
           std::to_string(std::max(8, object_align_bytes(mod, lir_module, param_ts)));
  }
  if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod)) return "i32";
  return stmt_emitter_detail::llvm_value_ty(mod, param_ts);
}

void populate_signature_type_refs(const c4c::hir::Module& mod,
                                  const c4c::hir::Function& fn,
                                  LirModule* lir_module,
                                  LirFunction& lir_fn) {
  using namespace c4c::codegen::llvm_helpers;
  lir_fn.signature_is_variadic = fn.attrs.variadic;
  lir_fn.signature_params.clear();
  lir_fn.signature_param_type_refs.clear();
  const TypeSpec return_ts =
      lir_owned_type_spec(mod, fn.return_type, lir_module);
  const bool templated_aggregate_return =
      (return_ts.base == TB_STRUCT || return_ts.base == TB_UNION) &&
      return_ts.ptr_level == 0 && return_ts.array_rank == 0 &&
      (fn.template_origin.empty() == false ||
       (return_ts.tpl_struct_origin && return_ts.tpl_struct_origin[0]) ||
       (return_ts.tpl_struct_args.data && return_ts.tpl_struct_args.size > 0));
  const bool direct_aggregate_return =
      !return_ts.is_lvalue_ref && !return_ts.is_rvalue_ref &&
      !templated_aggregate_return;
  const std::string return_type_text =
      direct_aggregate_return
          ? direct_owned_aggregate_type_text(mod, return_ts, lir_module)
                .value_or(rendered_signature_return_type(mod, return_ts))
          : rendered_signature_return_type(mod, return_ts);
  lir_fn.signature_return_type_ref =
      lir_signature_type_ref(return_type_text, lir_module, mod, return_ts);
  lir_fn.signature_return_ext_attr =
      rv64_variadic_return_ext_attr_for_abi_type(mod, fn);

  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;
  lir_fn.signature_has_void_param_list = void_param_list;
  if (void_param_list) return;

  for (const auto& param : fn.params) {
    const TypeSpec param_ts =
        lir_owned_type_spec(mod, param.type, lir_module);
    const std::string pname = "%p." + sanitize_llvm_ident(param.name);
    if (is_aarch64_fixed_hfa_param(mod, param.type.spec)) {
      append_aarch64_hfa_signature_params(mod, param.type.spec, pname, lir_module, lir_fn);
      continue;
    }
    if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param.type.spec, mod)) {
      lir_fn.signature_params.push_back({pname + ".abi", param_ts, false});
      lir_fn.signature_param_type_refs.push_back(LirTypeRef::integer(32));
      continue;
    }
    const bool is_byval_signature_param =
        llvm_target_is_amd64_sysv(mod.target_profile) &&
        llvm_cc::amd64_fixed_aggregate_passed_byval(param.type.spec, mod);
    const std::string rendered_param_type =
        rendered_signature_param_type(mod, lir_module, param_ts);
    const std::optional<std::string> direct_param_type =
        !is_byval_signature_param
            ? direct_owned_aggregate_type_text(mod, param_ts, lir_module)
            : std::nullopt;
    const std::string param_type_text =
        direct_param_type.has_value() && *direct_param_type == rendered_param_type
            ? *direct_param_type
            : rendered_param_type;
    lir_fn.signature_params.push_back(
        {pname, param_ts, is_byval_signature_param});
    lir_fn.signature_param_type_refs.push_back(lir_signature_type_ref(
        param_type_text, lir_module, mod, param_ts));
  }
}

void register_function_signature_ref(LirModule& module,
                                     LirFunction& lir_fn) {
  LirFunctionSignatureStoreEntry entry;
  entry.return_type_ref = lir_fn.signature_return_type_ref;
  entry.return_ext_attr = lir_fn.signature_return_ext_attr;
  entry.fixed_param_type_refs = lir_fn.signature_param_type_refs;
  entry.fixed_param_is_byval.reserve(lir_fn.signature_params.size());
  for (const LirSignatureParam& param : lir_fn.signature_params) {
    entry.fixed_param_is_byval.push_back(param.is_byval);
  }
  entry.is_variadic = lir_fn.signature_is_variadic;
  entry.has_void_param_list = lir_fn.signature_has_void_param_list;
  lir_fn.function_signature_ref =
      module.register_function_signature(std::move(entry));
}

void populate_lir_function_params(const c4c::hir::Module& mod,
                                  const c4c::hir::Function& fn,
                                  LirModule* lir_module,
                                  LirFunction& lir_fn) {
  lir_fn.params.clear();
  for (const auto& param : fn.params) {
    lir_fn.params.push_back(
        {"%p." + sanitize_llvm_ident(param.name),
         lir_owned_type_spec(mod, param.type, lir_module)});
  }
}

}  // namespace

// ── Module-level orchestration helpers ───────────────────────────────────────

int object_align_bytes(const c4c::hir::Module& mod, const LirModule* lir_module,
                       const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    int align = object_align_bytes(mod, lir_module, elem);
    if (ts.align_bytes > align) align = ts.align_bytes;
    return align;
  }
  int align = 1;
  if (ts.is_vector && ts.vector_bytes > 0) {
    align = static_cast<int>(ts.vector_bytes);
  } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    align = 8;
  } else if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    const stmt_emitter_detail::StructuredLayoutLookup layout =
        stmt_emitter_detail::lookup_structured_layout(mod, lir_module, ts,
                                                      "module-object-align");
    const std::optional<int> structured_align =
        stmt_emitter_detail::structured_layout_align_bytes(mod, lir_module, layout);
    align = structured_align ? *structured_align
                             : (layout.legacy_decl ? std::max(1, layout.legacy_decl->align_bytes)
                                                   : 8);
  } else if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
    align = llvm_va_list_alignment(mod.target_profile);
  } else {
    switch (ts.base) {
      case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: align = 1; break;
      case TB_SHORT: case TB_USHORT: align = 2; break;
      case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: align = 4; break;
      case TB_LONG: case TB_ULONG:
      case TB_LONGLONG: case TB_ULONGLONG:
      case TB_DOUBLE: align = 8; break;
      case TB_LONGDOUBLE:
      case TB_INT128: case TB_UINT128: align = 16; break;
      default: align = 8; break;
    }
  }
  if (ts.align_bytes > align) align = ts.align_bytes;
  return align;
}

int object_align_bytes(const c4c::hir::Module& mod, const TypeSpec& ts) {
  return object_align_bytes(mod, nullptr, ts);
}

std::vector<size_t> dedup_globals(const c4c::hir::Module& mod) {
  std::unordered_map<LinkNameId, size_t> best_by_link_name;
  std::unordered_map<TextId, size_t> best_by_name_text;
  std::unordered_map<std::string, size_t> best_by_name; // fallback when no stable ids exist
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    auto update_best = [&](size_t& best_index) {
      const bool cur_has_init =
          !std::holds_alternative<std::monostate>(mod.globals[best_index].init);
      const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
      if (new_has_init || !cur_has_init) best_index = i;
    };

    if (gv.link_name_id != kInvalidLinkName) {
      auto [it, inserted] = best_by_link_name.emplace(gv.link_name_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    if (gv.name_text_id != kInvalidText) {
      auto [it, inserted] = best_by_name_text.emplace(gv.name_text_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    auto [it, inserted] = best_by_name.emplace(gv.name, i);
    if (!inserted) update_best(it->second);
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best_by_link_name.size() + best_by_name_text.size() + best_by_name.size());
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    if (gv.link_name_id != kInvalidLinkName) {
      auto it = best_by_link_name.find(gv.link_name_id);
      if (it != best_by_link_name.end() && it->second == i) result.push_back(i);
      continue;
    }
    if (gv.name_text_id != kInvalidText) {
      auto it = best_by_name_text.find(gv.name_text_id);
      if (it != best_by_name_text.end() && it->second == i) result.push_back(i);
      continue;
    }
    auto it = best_by_name.find(gv.name);
    if (it != best_by_name.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<size_t> dedup_functions(const c4c::hir::Module& mod) {
  std::unordered_map<LinkNameId, size_t> best_by_link_name;
  std::unordered_map<TextId, size_t> best_by_name_text;
  std::unordered_map<std::string, size_t> best_by_name; // fallback when no stable ids exist
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    auto update_best = [&](size_t& best_index) {
      const bool cur_is_def = !mod.functions[best_index].blocks.empty();
      const bool new_is_def = !fn.blocks.empty();
      if (new_is_def && !cur_is_def) best_index = i;
    };

    if (fn.link_name_id != kInvalidLinkName) {
      auto [it, inserted] = best_by_link_name.emplace(fn.link_name_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    if (fn.name_text_id != kInvalidText) {
      auto [it, inserted] = best_by_name_text.emplace(fn.name_text_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    auto [it, inserted] = best_by_name.emplace(fn.name, i);
    if (!inserted) update_best(it->second);
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best_by_link_name.size() + best_by_name_text.size() + best_by_name.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    if (fn.link_name_id != kInvalidLinkName) {
      auto it = best_by_link_name.find(fn.link_name_id);
      if (it != best_by_link_name.end() && it->second == i) result.push_back(i);
      continue;
    }
    if (fn.name_text_id != kInvalidText) {
      auto it = best_by_name_text.find(fn.name_text_id);
      if (it != best_by_name_text.end() && it->second == i) result.push_back(i);
      continue;
    }
    auto it = best_by_name.find(fn.name);
    if (it != best_by_name.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<LinkNameId> collect_global_init_function_link_name_ids(
    const c4c::hir::Module& mod,
    const c4c::hir::GlobalInit& init) {
  std::vector<LinkNameId> refs;
  std::unordered_set<LinkNameId> seen;

  const auto add = [&](LinkNameId id) {
    if (id == kInvalidLinkName || seen.count(id) != 0 || mod.find_function(id) == nullptr) {
      return;
    }
    seen.insert(id);
    refs.push_back(id);
  };

  std::function<void(c4c::hir::ExprId)> scan_expr = [&](c4c::hir::ExprId id) {
    if (id.value >= mod.expr_pool.size()) return;
    const c4c::hir::Expr& expr = mod.expr_pool[id.value];
    std::visit(
        [&](const auto& payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, c4c::hir::DeclRef>) {
            add(payload.link_name_id);
          } else if constexpr (std::is_same_v<T, c4c::hir::UnaryExpr>) {
            scan_expr(payload.operand);
          } else if constexpr (std::is_same_v<T, c4c::hir::BinaryExpr>) {
            scan_expr(payload.lhs);
            scan_expr(payload.rhs);
          } else if constexpr (std::is_same_v<T, c4c::hir::AssignExpr>) {
            scan_expr(payload.lhs);
            scan_expr(payload.rhs);
          } else if constexpr (std::is_same_v<T, c4c::hir::CastExpr>) {
            scan_expr(payload.expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::CallExpr>) {
            scan_expr(payload.callee);
            for (const auto arg : payload.args) scan_expr(arg);
          } else if constexpr (std::is_same_v<T, c4c::hir::VaArgExpr>) {
            scan_expr(payload.ap);
          } else if constexpr (std::is_same_v<T, c4c::hir::IndexExpr>) {
            scan_expr(payload.base);
            scan_expr(payload.index);
          } else if constexpr (std::is_same_v<T, c4c::hir::MemberExpr>) {
            scan_expr(payload.base);
          } else if constexpr (std::is_same_v<T, c4c::hir::TernaryExpr>) {
            scan_expr(payload.cond);
            scan_expr(payload.then_expr);
            scan_expr(payload.else_expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::SizeofExpr>) {
            scan_expr(payload.expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::LabelAddrExpr>) {
            add(payload.fn_link_name_id);
          }
        },
        expr.payload);
  };

  std::function<void(const c4c::hir::GlobalInit&)> scan_init =
      [&](const c4c::hir::GlobalInit& global_init) {
    std::visit(
        [&](const auto& payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, c4c::hir::InitScalar>) {
            scan_expr(payload.expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::InitList>) {
            for (const auto& item : payload.items) {
              std::visit(
                  [&](const auto& item_value) {
                    using U = std::decay_t<decltype(item_value)>;
                    if constexpr (std::is_same_v<U, c4c::hir::InitScalar>) {
                      scan_expr(item_value.expr);
                    } else {
                      scan_init(c4c::hir::GlobalInit(*item_value));
                    }
                  },
                  item.value);
            }
          }
        },
        global_init);
  };

  scan_init(init);
  return refs;
}

std::vector<LirGlobalInitializerElement> collect_global_init_elements(
    const c4c::hir::Module& mod,
    const c4c::hir::GlobalInit& init) {
  std::vector<LirGlobalInitializerElement> elements;

  const auto add_label_address = [&](const c4c::hir::LabelAddrExpr& address) {
    const c4c::hir::Function* function = mod.find_function(address.fn_link_name_id);
    if (!function) return;
    for (const auto& block : function->blocks) {
      const bool contains_target = std::any_of(
          block.stmts.begin(), block.stmts.end(), [&](const c4c::hir::Stmt& stmt) {
            const auto* label = std::get_if<c4c::hir::LabelStmt>(&stmt.payload);
            return label && label->name == address.label_name;
          });
      if (contains_target) {
        elements.emplace_back(LirGlobalInitializerLabelAddress{
            address.fn_link_name_id, LirBlockId{block.id.value}});
        return;
      }
    }
  };

  std::function<void(c4c::hir::ExprId)> scan_expr = [&](c4c::hir::ExprId id) {
    if (id.value >= mod.expr_pool.size()) return;
    const c4c::hir::Expr& expr = mod.expr_pool[id.value];
    std::visit(
        [&](const auto& payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, c4c::hir::UnaryExpr>) {
            scan_expr(payload.operand);
          } else if constexpr (std::is_same_v<T, c4c::hir::BinaryExpr> ||
                               std::is_same_v<T, c4c::hir::AssignExpr>) {
            scan_expr(payload.lhs);
            scan_expr(payload.rhs);
          } else if constexpr (std::is_same_v<T, c4c::hir::CastExpr>) {
            scan_expr(payload.expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::CallExpr>) {
            scan_expr(payload.callee);
            for (const auto arg : payload.args) scan_expr(arg);
          } else if constexpr (std::is_same_v<T, c4c::hir::VaArgExpr>) {
            scan_expr(payload.ap);
          } else if constexpr (std::is_same_v<T, c4c::hir::IndexExpr>) {
            scan_expr(payload.base);
            scan_expr(payload.index);
          } else if constexpr (std::is_same_v<T, c4c::hir::MemberExpr>) {
            scan_expr(payload.base);
          } else if constexpr (std::is_same_v<T, c4c::hir::TernaryExpr>) {
            scan_expr(payload.cond);
            scan_expr(payload.then_expr);
            scan_expr(payload.else_expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::SizeofExpr>) {
            scan_expr(payload.expr);
          } else if constexpr (std::is_same_v<T, c4c::hir::LabelAddrExpr>) {
            add_label_address(payload);
          }
        },
        expr.payload);
  };

  std::function<void(const c4c::hir::GlobalInit&)> scan_init =
      [&](const c4c::hir::GlobalInit& global_init) {
        std::visit(
            [&](const auto& payload) {
              using T = std::decay_t<decltype(payload)>;
              if constexpr (std::is_same_v<T, c4c::hir::InitScalar>) {
                scan_expr(payload.expr);
              } else if constexpr (std::is_same_v<T, c4c::hir::InitList>) {
                for (const auto& item : payload.items) {
                  std::visit(
                      [&](const auto& item_value) {
                        using U = std::decay_t<decltype(item_value)>;
                        if constexpr (std::is_same_v<U, c4c::hir::InitScalar>) {
                          scan_expr(item_value.expr);
                        } else {
                          scan_init(c4c::hir::GlobalInit(*item_value));
                        }
                      },
                      item.value);
                }
              }
            },
            global_init);
      };

  scan_init(init);
  return elements;
}

// ── Global variable lowering ─────────────────────────────────────────────────
// Semantic decisions (linkage, alignment, qualifier, type, init) are computed
// here in lowering.  The printer assembles LLVM text from the structured fields.

static void lower_global(const c4c::hir::GlobalVar& gv,
                          const c4c::hir::Module& mod,
                          ConstInitEmitter& const_init,
                          LirModule& module) {
  using namespace c4c::codegen::llvm_helpers;

  const TypeSpec& ts = gv.type.spec;
  const int align = object_align_bytes(mod, &module, ts);

  auto make_linkage_vis = [&](bool is_static, bool is_weak, bool is_extern,
                              Visibility vis) -> std::string {
    if (is_extern) {
      std::string s = is_weak ? "extern_weak " : "external ";
      s += llvm_visibility(vis);
      return s;
    }
    std::string s = is_static ? "internal " : "";
    if (is_weak && !is_static) s = "weak ";
    s += llvm_visibility(vis);
    return s;
  };

  // Check for flexible array member struct — needs a custom literal type/init.
  if (!gv.linkage.is_extern &&
      ts.ptr_level == 0 && ts.array_rank == 0 &&
      ts.base == TB_STRUCT) {
    if (const HirStructDef* sd_ptr = find_typespec_aggregate_layout(mod, ts)) {
      const auto& sd = *sd_ptr;
      if (!sd.is_union && !sd.fields.empty() && sd.fields.back().is_flexible_array) {
        std::vector<TypeSpec> field_types;
        const auto field_vals = const_init.emit_const_struct_fields(ts, sd, gv.init, &field_types);
        const TypeSpec& last_ts = field_types.back();
        if (last_ts.array_rank > 0 && last_ts.array_size > 0) {
          std::string literal_ty = "{ ";
          std::string literal_init = "{ ";
          for (size_t i = 0; i < sd.fields.size(); ++i) {
            if (i) { literal_ty += ", "; literal_init += ", "; }
            literal_ty += llvm_alloca_ty(field_types[i]);
            literal_init += llvm_alloca_ty(field_types[i]) + " " + field_vals[i];
          }
          literal_ty += " }";
          literal_init += " }";

          LirGlobal lg;
          lg.name = gv.name;
          lg.link_name_id = gv.link_name_id;
          lg.type = ts;
          lg.is_internal = gv.linkage.is_static;
          lg.is_const = gv.is_const;
          lg.linkage_vis = make_linkage_vis(gv.linkage.is_static, gv.linkage.is_weak,
                                            false, gv.linkage.visibility);
          lg.qualifier = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
          lg.llvm_type = literal_ty;
          lg.llvm_type_ref = lir_global_type_ref(lg.llvm_type, &module, mod, ts);
          lg.init_text = literal_init;
          lg.initializer_elements = collect_global_init_elements(mod, gv.init);
          lg.initializer_function_link_name_ids =
              collect_global_init_function_link_name_ids(mod, gv.init);
          lg.align_bytes = align;
          lg.is_extern_decl = false;
          module.globals.push_back(std::move(lg));
          return;
        }
      }
    }
  }

  LirGlobal lg;
  lg.name = gv.name;
  lg.link_name_id = gv.link_name_id;
  lg.type = ts;
  lg.is_internal = gv.linkage.is_static;
  lg.is_const = gv.is_const;
  lg.llvm_type = stmt_emitter_detail::llvm_alloca_ty(mod, ts);
  lg.llvm_type_ref = lir_global_type_ref(lg.llvm_type, &module, mod, ts);
  lg.align_bytes = align;

  if (gv.linkage.is_extern) {
    lg.linkage_vis = make_linkage_vis(false, gv.linkage.is_weak, true, gv.linkage.visibility);
    lg.qualifier = "global ";
    lg.is_extern_decl = true;
    // init_text left empty for extern declarations
  } else {
    lg.linkage_vis = make_linkage_vis(gv.linkage.is_static, gv.linkage.is_weak,
                                      false, gv.linkage.visibility);
    lg.qualifier = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
    lg.init_text = const_init.emit_const_init(ts, gv.init);
    lg.initializer_elements = collect_global_init_elements(mod, gv.init);
    lg.initializer_function_link_name_ids =
        collect_global_init_function_link_name_ids(mod, gv.init);
    lg.is_extern_decl = false;
  }

  module.globals.push_back(std::move(lg));
}

static void lower_globals(const std::vector<size_t>& global_indices,
                           const c4c::hir::Module& mod,
                           ConstInitEmitter& const_init,
                           LirModule& module) {
  for (size_t idx : global_indices)
    lower_global(mod.globals[idx], mod, const_init, module);
}

// ── Type declarations ────────────────────────────────────────────────────────

LirTypeRef lir_byte_storage_type_ref(std::size_t byte_count) {
  return LirTypeRef::array(LirTypeRef(LirBuiltinType::I8), byte_count);
}

std::vector<std::string> build_type_decls(const c4c::hir::Module& mod,
                                          LirModule* lir_module) {
  using namespace c4c::codegen::llvm_helpers;
  std::vector<std::string> decls;

  if (!llvm_va_list_is_pointer_object(mod.target_profile)) {
    decls.push_back(llvm_va_list_struct_decl(mod.target_profile));
    if (lir_module) {
      LirStructDecl decl;
      decl.name_id = lir_module->struct_names.intern("%struct.__va_list_tag_");
      if (llvm_target_is_amd64_sysv(mod.target_profile)) {
        decl.fields.push_back({LirTypeRef(LirBuiltinType::I32)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::I32)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::Pointer)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::Pointer)});
      } else {
        decl.fields.push_back({LirTypeRef(LirBuiltinType::Pointer)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::Pointer)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::Pointer)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::I32)});
        decl.fields.push_back({LirTypeRef(LirBuiltinType::I32)});
      }
      lir_module->record_struct_decl(std::move(decl));
    }
  }

  std::vector<std::string> ordered_tags = mod.struct_def_order;
  std::vector<std::string> missing_ordered_tags;
  for (const auto& [tag, _] : mod.struct_defs) {
    if (std::find(ordered_tags.begin(), ordered_tags.end(), tag) == ordered_tags.end()) {
      missing_ordered_tags.push_back(tag);
    }
  }
  std::sort(missing_ordered_tags.begin(), missing_ordered_tags.end());
  ordered_tags.insert(ordered_tags.end(), missing_ordered_tags.begin(),
                      missing_ordered_tags.end());

  for (const auto& tag : ordered_tags) {
    const auto it = mod.struct_defs.find(tag);
    if (it == mod.struct_defs.end()) continue;
    const auto& sd = it->second;
    const std::string sty = llvm_struct_type_str(tag);
    LirStructDecl structured_decl;
    std::optional<LirAggregateRef> aggregate_ref;
    LirAggregateLayoutKind aggregate_layout_kind = LirAggregateLayoutKind::Direct;
    structured_decl.name_id =
        lir_module ? lir_module->struct_names.intern(sty) : kInvalidStructName;
    if (lir_module) {
      const std::optional<c4c::hir::HirAggregateRef> hir_ref =
          sd.aggregate_ref ? sd.aggregate_ref : mod.aggregate_ref_for_definition(sd);
      if (!hir_ref || !hir_ref->complete()) {
        throw std::runtime_error(
            "aggregate definition lowering requires a registered complete HIR aggregate ref");
      }
      aggregate_ref = lir_register_aggregate_ref(mod, *lir_module, *hir_ref,
                                                  structured_decl.name_id, sd.is_union);
    }
    structured_decl.is_packed = sd.pack_align > 0;
    auto record_structured_decl = [&]() {
      if (!lir_module) return;
      if (aggregate_ref) {
        lir_module->record_aggregate_decl_facts(*aggregate_ref, structured_decl,
                                                aggregate_layout_kind);
      }
      lir_module->record_struct_decl(std::move(structured_decl));
    };

    if (packed_bitfield_byte_storage_record(sd)) {
      aggregate_layout_kind = LirAggregateLayoutKind::ByteStorage;
      decls.push_back(sty + " = type <{ [" + std::to_string(sd.size_bytes) +
                      " x i8] }>");
      structured_decl.fields.push_back(
          {lir_byte_storage_type_ref(static_cast<std::size_t>(sd.size_bytes))});
      record_structured_decl();
      continue;
    }

    if (sd.fields.empty() && sd.base_tags.empty()) {
      if (sd.size_bytes == 0) {
        decls.push_back(sty + " = type " +
                         std::string(structured_decl.is_packed ? "<{}>" : "{}"));
      } else {
        aggregate_layout_kind = LirAggregateLayoutKind::ByteStorage;
        decls.push_back(sty + " = type " +
                         std::string(structured_decl.is_packed ? "<{ " : "{ ") +
                         "[" + std::to_string(sd.size_bytes) + " x i8]" +
                         std::string(structured_decl.is_packed ? " }>" : " }"));
        structured_decl.fields.push_back(
            {lir_byte_storage_type_ref(static_cast<std::size_t>(sd.size_bytes))});
      }
      record_structured_decl();
      continue;
    }
    if (sd.is_union) {
      aggregate_layout_kind = LirAggregateLayoutKind::Union;
      decls.push_back(sty + " = type " +
                       std::string(structured_decl.is_packed ? "<{ " : "{ ") +
                       "[" + std::to_string(sd.size_bytes) + " x i8]" +
                       std::string(structured_decl.is_packed ? " }>" : " }"));
      structured_decl.fields.push_back(
          {lir_byte_storage_type_ref(static_cast<std::size_t>(sd.size_bytes))});
      record_structured_decl();
    } else {
      std::ostringstream line;
      line << sty << " = type " << (structured_decl.is_packed ? "<{ " : "{ ");
      bool first = true;
      int cur_offset = 0;
      for (const auto& base_tag : sd.base_tags) {
        const auto bit = mod.struct_defs.find(base_tag);
        if (bit == mod.struct_defs.end()) continue;
        const auto& base = bit->second;
        const int base_align = std::max(1, base.align_bytes);
        const int base_offset = align_to_bytes(cur_offset, base_align);
        if (base_offset > cur_offset) {
          if (!first) line << ", ";
          first = false;
          const LirTypeRef pad_type =
              lir_byte_storage_type_ref(static_cast<std::size_t>(base_offset - cur_offset));
          line << pad_type.render_llvm();
          structured_decl.fields.push_back({pad_type});
          cur_offset = base_offset;
        }
        if (!first) line << ", ";
        first = false;
        const std::string base_ty = llvm_struct_type_str(base_tag);
        line << base_ty;
        const StructNameId base_name_id =
            lir_module ? lir_module->struct_names.intern(base_ty) : kInvalidStructName;
        structured_decl.fields.push_back(
            {lir_aggregate_type_ref(base_ty, lir_module, base_name_id, base.is_union)});
        cur_offset = base_offset + std::max(0, base.size_bytes);
      }
      int last_idx = -1;
      for (const auto& f : sd.fields) {
        if (f.llvm_idx == last_idx) continue;
        last_idx = f.llvm_idx;
        if (f.offset_bytes > cur_offset) {
          if (!first) line << ", ";
          first = false;
          const LirTypeRef pad_type =
              lir_byte_storage_type_ref(static_cast<std::size_t>(f.offset_bytes - cur_offset));
          line << pad_type.render_llvm();
          structured_decl.fields.push_back({pad_type});
          cur_offset = f.offset_bytes;
        }
        if (!first) line << ", ";
        first = false;
        const std::string field_ty = lir_field_ty(mod, f);
        line << field_ty;
        structured_decl.fields.push_back({lir_field_type_ref(f, lir_module, mod)});
        cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
      }
      if (sd.size_bytes > cur_offset) {
        if (!first) line << ", ";
        const LirTypeRef pad_type =
            lir_byte_storage_type_ref(static_cast<std::size_t>(sd.size_bytes - cur_offset));
        line << pad_type.render_llvm();
        structured_decl.fields.push_back({pad_type});
      }
      line << (structured_decl.is_packed ? " }>" : " }");
      decls.push_back(line.str());
      record_structured_decl();
    }
  }
  return decls;
}

// ── Function signature building ───────────────────────────────────────────────
// Builds the LLVM IR signature text for a HIR function.  Ownership of this
// logic belongs to hir_to_lir; StmtEmitter consumes the pre-built text.

std::string build_fn_signature(const c4c::hir::Module& mod,
                               const c4c::hir::Function& fn,
                               const LirModule* lir_module) {
  using namespace c4c::codegen::llvm_helpers;

  std::ostringstream sig_out;
  const std::string ret_ty = rendered_signature_return_type(mod, fn.return_type.spec);
  const std::string emitted_name = emitted_link_name(mod, fn.link_name_id, fn.name);
  const LirExtAttr return_ext_attr = rv64_variadic_return_ext_attr_for_abi_type(mod, fn);

  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;

  // Declaration (extern with no body)
  if (fn.linkage.is_extern && fn.blocks.empty()) {
    const std::string decl_kw = fn.linkage.is_weak ? "declare extern_weak " : "declare ";
    sig_out << decl_kw << llvm_visibility(fn.linkage.visibility)
            << signature_ext_attr_prefix(return_ext_attr) << ret_ty << " "
            << llvm_global_sym(emitted_name) << "(";
    bool first_param = true;
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      const TypeSpec& param_ts = fn.params[i].type.spec;
      if (const auto hfa = is_aarch64_fixed_hfa_param(mod, param_ts)
                               ? stmt_emitter_detail::classify_aarch64_hfa(mod, param_ts)
                               : std::nullopt;
          hfa.has_value()) {
        for (int lane_index = 0; lane_index < hfa->elem_count; ++lane_index) {
          if (!first_param) sig_out << ", ";
          first_param = false;
          sig_out << hfa->elem_ty;
        }
      } else if (llvm_target_is_amd64_sysv(mod.target_profile) &&
          llvm_cc::amd64_fixed_aggregate_passed_byval(param_ts, mod)) {
        if (!first_param) sig_out << ", ";
        first_param = false;
        sig_out << rendered_signature_param_type(mod, lir_module, param_ts);
      } else if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod)) {
        if (!first_param) sig_out << ", ";
        first_param = false;
        sig_out << "i32";
      } else {
        if (!first_param) sig_out << ", ";
        first_param = false;
        sig_out << rendered_signature_param_type(mod, lir_module, param_ts);
      }
    }
    if (fn.attrs.variadic) {
      if (!fn.params.empty() && !void_param_list) sig_out << ", ";
      sig_out << "...";
    }
    sig_out << ")\n\n";
    return sig_out.str();
  }

  // Definition — template comments
  if (!fn.template_origin.empty()) {
    sig_out << "; template-origin: " << fn.template_origin << "\n";
    if (!fn.spec_key.empty()) {
      sig_out << "; spec-key: " << fn.spec_key.canonical << "\n";
    }
  }

  // Definition — linkage + visibility + return type + name
  std::string fn_lk = fn.linkage.is_static ? "internal " : "";
  if (fn.linkage.is_weak && !fn.linkage.is_static) fn_lk = "weak ";
  sig_out << "define " << fn_lk << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
          << llvm_global_sym(emitted_name) << "(";

  // Parameters
  bool first_param = true;
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    const TypeSpec& param_ts = fn.params[i].type.spec;
    const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
    if (const auto hfa = is_aarch64_fixed_hfa_param(mod, param_ts)
                             ? stmt_emitter_detail::classify_aarch64_hfa(mod, param_ts)
                             : std::nullopt;
        hfa.has_value()) {
      for (int lane_index = 0; lane_index < hfa->elem_count; ++lane_index) {
        if (!first_param) sig_out << ", ";
        first_param = false;
        sig_out << hfa->elem_ty << " " << aarch64_hfa_lane_name(pname, lane_index);
      }
    } else if (llvm_target_is_amd64_sysv(mod.target_profile) &&
        llvm_cc::amd64_fixed_aggregate_passed_byval(param_ts, mod)) {
      if (!first_param) sig_out << ", ";
      first_param = false;
      sig_out << rendered_signature_param_type(mod, lir_module, param_ts)
              << " " << pname;
    } else if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod)) {
      if (!first_param) sig_out << ", ";
      first_param = false;
      sig_out << "i32 " << pname << ".abi";
    } else {
      if (!first_param) sig_out << ", ";
      first_param = false;
      sig_out << rendered_signature_param_type(mod, lir_module, param_ts)
              << " " << pname;
    }
  }
  if (fn.attrs.variadic) {
    if (!fn.params.empty()) sig_out << ", ";
    sig_out << "...";
  }
  sig_out << ")";

  // Function attributes
  if (fn.attrs.no_inline) sig_out << " noinline";
  if (fn.attrs.always_inline) sig_out << " alwaysinline";
  sig_out << "\n";

  return sig_out.str();
}

// ── Module-level finalization ─────────────────────────────────────────────────
// After per-item lowering, the emitter has written intrinsic flags and extern
// call declarations directly into the LirModule.  This function converts the
// dedup map into the extern_decls vector for the printer.

static void finalize_module(LirModule& module,
                            const c4c::hir::Module& hir_mod) {
  std::unordered_set<LinkNameId> local_fn_link_names;
  local_fn_link_names.reserve(hir_mod.functions.size());
  for (const auto& fn : hir_mod.functions) {
    if (fn.link_name_id != kInvalidLinkName) {
      local_fn_link_names.insert(fn.link_name_id);
    }
  }

  auto push_extern_decl = [&](const LirModule::ExternDeclInfo& decl_info) {
    const std::string fallback_name = decl_info.name.empty()
        ? emitted_link_name(hir_mod, decl_info.link_name_id, "")
        : decl_info.name;
    if (decl_info.link_name_id != kInvalidLinkName &&
        local_fn_link_names.count(decl_info.link_name_id)) {
      return;
    }
    if (decl_info.link_name_id == kInvalidLinkName &&
        hir_mod.fn_index.count(fallback_name)) {
      return;
    }

    LirExternDecl ed;
    ed.name = fallback_name;
    ed.return_type_str = decl_info.return_type_str;
    ed.return_type = decl_info.return_type.empty()
        ? module.extern_return_type_ref(decl_info.return_type_str)
        : decl_info.return_type;
    ed.return_ext_attr = decl_info.return_ext_attr;
    if (!ed.return_type.has_struct_name_id()) {
      const LirTypeRef finalized_type =
          module.extern_return_type_ref(decl_info.return_type_str);
      if (finalized_type.has_struct_name_id()) ed.return_type = finalized_type;
    }
    ed.link_name_id = decl_info.link_name_id;
    ed.function_signature_ref = decl_info.function_signature_ref;
    module.extern_decls.push_back(std::move(ed));
  };

  // Convert extern declaration dedup state into the printer vector. LinkNameId
  // entries are the authoritative path; the raw-name map is retained only for
  // legacy import/output compatibility when complete link metadata was absent.
  for (const auto& [_, decl_info] : module.extern_decl_link_name_map) {
    push_extern_decl(decl_info);
  }
  for (const auto& [name, decl_info] : module.extern_decl_name_map) {
    (void)name;
    push_extern_decl(decl_info);
  }
}

// ── Block ordering ───────────────────────────────────────────────────────────
// Computes the HIR block iteration order: entry block first, then remaining
// blocks in their original order.  Ownership of this decision belongs to
// hir_to_lir, not StmtEmitter.

std::vector<const c4c::hir::Block*>
build_block_order(const c4c::hir::Function& fn) {
  std::vector<const c4c::hir::Block*> ordered;
  ordered.reserve(fn.blocks.size());
  const c4c::hir::Block* entry_blk = nullptr;
  for (const auto& blk : fn.blocks) {
    if (blk.id.value == fn.entry.value) { entry_blk = &blk; }
  }
  if (entry_blk) ordered.push_back(entry_blk);
  for (const auto& blk : fn.blocks) {
    if (blk.id.value != fn.entry.value) ordered.push_back(&blk);
  }
  return ordered;
}

// ── Fallthrough return injection ─────────────────────────────────────────────
// After per-statement lowering, any LirBlock whose terminator is still
// LirUnreachable did not receive an explicit terminator from the HIR.
// This post-pass injects a default "ret" matching the function's return type.

static void inject_fallthrough_returns(LirFunction& lir_fn,
                                       const c4c::hir::Module& mod,
                                       const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;
  const auto& rts = fn.return_type.spec;
  const std::string ret_ty = stmt_emitter_detail::llvm_return_ty(mod, rts);

  // Pre-compute the default zero value for non-void returns.
  std::optional<std::string> zero_val;
  if (rts.base == TB_VOID && rts.ptr_level == 0 && rts.array_rank == 0 &&
      !rts.is_lvalue_ref && !rts.is_rvalue_ref) {
    // void → no value
  } else if (ret_ty == "ptr") {
    zero_val = "null";
  } else if (is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0) {
    zero_val = fp_literal(rts.base, 0.0);
  } else if (is_complex_base(rts.base) ||
             ((rts.base == TB_STRUCT || rts.base == TB_UNION) &&
              rts.ptr_level == 0 && rts.array_rank == 0)) {
    zero_val = "zeroinitializer";
  } else {
    zero_val = "0";
  }

  for (auto& blk : lir_fn.blocks) {
    if (!std::holds_alternative<LirUnreachable>(blk.terminator)) continue;
    blk.terminator = LirRet{zero_val, ret_ty};
  }
}

// ── Per-function skeleton helpers ─────────────────────────────────────────

static const c4c::hir::Expr& lookup_expr(const c4c::hir::Module& mod, c4c::hir::ExprId id) {
  for (const auto& e : mod.expr_pool)
    if (e.id.value == id.value) return e;
  throw std::runtime_error("hir_to_lir: expr not found id=" + std::to_string(id.value));
}

std::unordered_set<uint32_t> find_modified_params(
    const c4c::hir::Module& mod, const c4c::hir::Function& fn) {
  using namespace c4c::hir;
  std::unordered_set<uint32_t> result;
  std::function<void(ExprId)> scan = [&](ExprId id) {
    const Expr& e = lookup_expr(mod, id);
    std::visit([&](const auto& p) {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, UnaryExpr>) {
        if (p.op == UnaryOp::PreInc || p.op == UnaryOp::PostInc ||
            p.op == UnaryOp::PreDec || p.op == UnaryOp::PostDec ||
            p.op == UnaryOp::AddrOf) {
          const Expr& op_e = lookup_expr(mod, p.operand);
          if (const auto* r = std::get_if<DeclRef>(&op_e.payload)) {
            if (r->param_index) result.insert(*r->param_index);
          }
        }
        scan(p.operand);
      } else if constexpr (std::is_same_v<T, AssignExpr>) {
        const Expr& lhs_e = lookup_expr(mod, p.lhs);
        if (const auto* r = std::get_if<DeclRef>(&lhs_e.payload)) {
          if (r->param_index) result.insert(*r->param_index);
        }
        scan(p.lhs); scan(p.rhs);
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        scan(p.lhs); scan(p.rhs);
      } else if constexpr (std::is_same_v<T, CallExpr>) {
        scan(p.callee);
        for (const auto& a : p.args) scan(a);
      } else if constexpr (std::is_same_v<T, TernaryExpr>) {
        scan(p.cond); scan(p.then_expr); scan(p.else_expr);
      } else if constexpr (std::is_same_v<T, IndexExpr>) {
        scan(p.base); scan(p.index);
      } else if constexpr (std::is_same_v<T, MemberExpr>) {
        scan(p.base);
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        scan(p.expr);
      } else if constexpr (std::is_same_v<T, SizeofExpr>) {
        scan(p.expr);
      }
    }, e.payload);
  };
  for (const auto& blk : fn.blocks) {
    for (const auto& stmt : blk.stmts) {
      std::visit([&](const auto& s) {
        using S = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<S, LocalDecl>) {
          if (s.init) scan(*s.init);
        } else if constexpr (std::is_same_v<S, ExprStmt>) {
          if (s.expr) scan(*s.expr);
        } else if constexpr (std::is_same_v<S, ReturnStmt>) {
          if (s.expr) scan(*s.expr);
        } else if constexpr (std::is_same_v<S, IfStmt>) {
          scan(s.cond);
        } else if constexpr (std::is_same_v<S, WhileStmt>) {
          scan(s.cond);
        } else if constexpr (std::is_same_v<S, ForStmt>) {
          if (s.init) scan(*s.init);
          if (s.cond) scan(*s.cond);
          if (s.update) scan(*s.update);
        } else if constexpr (std::is_same_v<S, SwitchStmt>) {
          scan(s.cond);
        }
      }, stmt.payload);
    }
  }
  return result;
}

bool fn_has_vla_locals(const c4c::hir::Function& fn) {
  for (const auto& blk : fn.blocks) {
    for (const auto& stmt : blk.stmts) {
      if (const auto* d = std::get_if<c4c::hir::LocalDecl>(&stmt.payload)) {
        if (d->vla_size.has_value()) return true;
      }
    }
  }
  return false;
}

void hoist_allocas(c4c::codegen::FnCtx& ctx, const c4c::hir::Module& mod,
                   const c4c::hir::Function& fn,
                   const LirModule* lir_module) {
  using namespace c4c::codegen::llvm_helpers;

  const std::unordered_set<uint32_t> modified_params = find_modified_params(mod, fn);
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (!modified_params.count(static_cast<uint32_t>(i))) continue;
    const auto& param = fn.params[i];
    if (llvm_target_is_amd64_sysv(mod.target_profile) &&
        llvm_cc::amd64_fixed_aggregate_passed_byval(param.type.spec, mod)) {
      continue;
    }
    if (is_aarch64_fixed_hfa_param(mod, param.type.spec)) {
      continue;
    }
    const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
    const std::string pname = "%p." + sanitize_llvm_ident(param.name);
    ctx.param_slots[static_cast<uint32_t>(i) + 0x80000000u] = slot;
    const int param_align = object_align_bytes(mod, lir_module, param.type.spec);
    ctx.alloca_insts.push_back(
        LirAllocaOp{slot, stmt_emitter_detail::llvm_alloca_ty(mod, param.type.spec), "",
                    param_align});
    ctx.alloca_insts.push_back(
        LirStoreOp{stmt_emitter_detail::llvm_value_ty(mod, param.type.spec), pname, slot});
  }

  std::unordered_map<std::string, int> name_count;
  for (const auto& blk : fn.blocks) {
    for (const auto& stmt : blk.stmts) {
      const auto* d = std::get_if<c4c::hir::LocalDecl>(&stmt.payload);
      if (!d) continue;
      if (ctx.local_slots.count(d->id.value)) continue;
      const int cnt = name_count[d->name]++;
      const std::string base = sanitize_llvm_ident(d->name);
      const std::string slot = cnt == 0
          ? "%lv." + base
          : "%lv." + base + "." + std::to_string(cnt);
      ctx.local_slots[d->id.value] = slot;
      ctx.local_types[d->id.value] = d->type.spec;
      ctx.local_is_vla[d->id.value] = d->vla_size.has_value();
      const LirTypeRef pointee_type = d->vla_size
          ? LirTypeRef(LirBuiltinType::Pointer)
          : LirTypeRef(stmt_emitter_detail::llvm_alloca_ty(mod, d->type.spec));
      const LirCurrentFunctionLocalObjectPointer authority{
          .pointer_definition = const_cast<LirModule*>(lir_module)->alloc_value(),
          .object = ctx.lir_function->alloc_object(),
          .owner = ctx.lir_function->link_name_id,
          .pointer_type = LirTypeRef(LirBuiltinType::Pointer),
          .pointee_type = pointee_type,
          .live = true,
	          .indexed_element_type =
	              d->vla_size || d->type.spec.array_rank == 0
	                  ? std::nullopt
	                  : indexed_local_array_element_type_ref(d->type.spec),
	      };
      ctx.local_object_authorities.emplace(d->id.value, authority);
      const LirOperand slot_operand =
          LirOperand::ssa(slot, authority.pointer_definition);
      if (d->vla_size) {
        // VLA: alloca a pointer slot (the actual dynamic alloca happens later)
        TypeSpec ptr_ts{};
        ptr_ts.base = TB_VOID;
        ptr_ts.ptr_level = 1;
        ctx.alloca_insts.push_back(
            LirAllocaOp{slot_operand, stmt_emitter_detail::llvm_alloca_ty(mod, ptr_ts), "", 0,
                        authority});
      } else {
        const int stack_align = object_align_bytes(mod, lir_module, d->type.spec);
        ctx.alloca_insts.push_back(
            LirAllocaOp{slot_operand, stmt_emitter_detail::llvm_alloca_ty(mod, d->type.spec), "",
                        stack_align, authority});
      }
    }
  }
}

c4c::codegen::FnCtx init_fn_ctx(const c4c::hir::Module& mod,
                                const c4c::hir::Function& fn,
                                LirFunction& lir_function,
                                const LirModule* lir_module) {
  using namespace c4c::codegen::llvm_helpers;

  c4c::codegen::FnCtx ctx;
  ctx.fn = &fn;
  ctx.lir_function = &lir_function;
  uint32_t next_dynamic_block_id = 0;
  for (const auto& block : fn.blocks) {
    next_dynamic_block_id = std::max(next_dynamic_block_id, block.id.value + 1);
  }
  lir_function.next_block_id = next_dynamic_block_id;

  // Set up fn_ptr_sig metadata for parameters and globals.
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (fn.params[i].fn_ptr_sig) {
      ctx.param_fn_ptr_sigs[static_cast<uint32_t>(i)] = *fn.params[i].fn_ptr_sig;
    }
  }
  for (const auto& g : mod.globals) {
    if (g.fn_ptr_sig) ctx.global_fn_ptr_sigs[g.id.value] = *g.fn_ptr_sig;
  }

  // Populate param_slots for body lowering.
  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
    ctx.param_slots[static_cast<uint32_t>(i)] = pname;
    const TypeSpec& param_ts = fn.params[i].type.spec;
    const bool native_body_pointer_parameter =
        param_ts.ptr_level > 0 && param_ts.array_rank == 0 &&
        !stmt_emitter_detail::amd64_fixed_aggregate_byval(mod, param_ts) &&
        !is_aarch64_fixed_hfa_param(mod, param_ts) &&
        !llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod);
    const bool native_body_direct_scalar_parameter =
        (param_ts.base == TB_INT || param_ts.base == TB_UINT ||
         param_ts.base == TB_LONG || param_ts.base == TB_ULONG ||
         param_ts.base == TB_LONGLONG || param_ts.base == TB_ULONGLONG ||
         param_ts.base == TB_FLOAT || param_ts.base == TB_DOUBLE) &&
        param_ts.ptr_level == 0 && param_ts.array_rank == 0 &&
        !param_ts.is_vector && !param_ts.is_lvalue_ref && !param_ts.is_rvalue_ref &&
        !param_ts.is_fn_ptr;
    if (native_body_pointer_parameter) {
      const LirValueId value = const_cast<LirModule*>(lir_module)->alloc_value();
      ctx.param_value_authorities.emplace(static_cast<uint32_t>(i), value);
      lir_function.native_body_parameter_definitions.push_back(
          LirCurrentFunctionBodyParameterDefinition{
              .value = value,
              .parameter_index = static_cast<uint32_t>(i),
              .type = LirTypeRef(LirBuiltinType::Pointer),
              .owner = lir_function.link_name_id,
              .abi = LirNativeBodyParameterAbi::DirectPointer,
          });
    }
    if (native_body_direct_scalar_parameter) {
      const LirValueId value = const_cast<LirModule*>(lir_module)->alloc_value();
      ctx.param_value_authorities.emplace(static_cast<uint32_t>(i), value);
      lir_function.native_body_parameter_definitions.push_back(
          LirCurrentFunctionBodyParameterDefinition{
              .value = value,
              .parameter_index = static_cast<uint32_t>(i),
              // The signature mirror is the typed ABI authority for this
              // current-function parameter.  Reuse it rather than rebuilding
              // a parallel scalar type from rendered lowering text.
              .type = lir_function.signature_param_type_refs.at(i),
              .owner = lir_function.link_name_id,
              .abi = LirNativeBodyParameterAbi::DirectScalar,
          });
    }
    if (const auto hfa = is_aarch64_fixed_hfa_param(mod, param_ts)
                             ? stmt_emitter_detail::classify_aarch64_hfa(mod, param_ts)
                             : std::nullopt;
        hfa.has_value()) {
      const std::string slot = "%lv.param." + sanitize_llvm_ident(fn.params[i].name);
      ctx.param_slots[static_cast<uint32_t>(i) + 0x80000000u] = slot;
      ctx.alloca_insts.push_back(
          LirAllocaOp{slot, stmt_emitter_detail::llvm_alloca_ty(mod, param_ts), "",
                      object_align_bytes(mod, lir_module, param_ts)});
      for (int lane_index = 0; lane_index < hfa->elem_count; ++lane_index) {
        const std::string lane_ptr =
            slot + ".hfa.ptr." + std::to_string(lane_index);
        const std::string lane_name = aarch64_hfa_lane_name(pname, lane_index);
        ctx.alloca_insts.push_back(
            LirGepOp{lane_ptr, "i8", slot, false,
                     {"i64 " + std::to_string(lane_index * hfa->elem_size)}});
        ctx.alloca_insts.push_back(LirStoreOp{hfa->elem_ty, lane_name, lane_ptr});
      }
    } else if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod)) {
      ctx.alloca_insts.push_back(
          LirCastOp{pname, LirCastKind::Bitcast, LirTypeRef(LirBuiltinType::I32),
                    pname + ".abi", hir_rendered_aarch64_vector_abi_source_type_text(
                                            stmt_emitter_detail::llvm_value_ty(mod, param_ts))});
    }
  }

  // Create entry block.
  LirBlock entry_blk;
  entry_blk.id = LirBlockId{fn.entry.value};
  entry_blk.label = "entry";
  ctx.lir_blocks.push_back(std::move(entry_blk));
  ctx.current_block_idx = 0;

  // Hoist allocas.
  hoist_allocas(ctx, mod, fn, lir_module);

  // VLA stack save — must happen after alloca hoisting but before statements.
  if (fn_has_vla_locals(fn)) {
    const std::string saved_sp = "%t" + std::to_string(ctx.tmp_idx++);
    const LirCurrentFunctionLocalObjectPointer authority{
        .pointer_definition = const_cast<LirModule*>(lir_module)->alloc_value(),
        .object = lir_function.alloc_object(),
        .owner = lir_function.link_name_id,
        .pointer_type = LirTypeRef(LirBuiltinType::Pointer),
        .pointee_type = LirTypeRef(LirBuiltinType::Pointer),
        .live = true,
    };
    ctx.cur_block().insts.push_back(
        LirStackSaveOp{LirOperand::ssa(saved_sp, authority.pointer_definition), authority,
                       true});
    ctx.vla_stack_save_ptr = saved_sp;
    ctx.vla_stack_lifetime_authority = authority;
  }

  return ctx;
}

// ── Block label helpers ──────────────────────────────────────────────────────
// Ownership of block naming and creation belongs to hir_to_lir, not StmtEmitter.

std::string block_lbl(c4c::hir::BlockId id) {
  return "block_" + std::to_string(id.value);
}

void emit_lbl(c4c::codegen::FnCtx& ctx,
              const c4c::codegen::LirDirectBranchTarget& target) {
  LirBlock blk;
  blk.id = target.id;
  blk.label = target.label;
  ctx.lir_blocks.push_back(std::move(blk));
  ctx.current_block_idx = ctx.lir_blocks.size() - 1;
  ctx.last_term = false;
}

// ── Dead internal function elimination ────────────────────────────────────────
// Removes unreferenced internal (static) functions from the module.
// This is a semantic decision (what to emit) and belongs in lowering, not the
// printer.  The algorithm is a worklist reachability analysis: external functions
// and global initializers seed the worklist; internal functions are kept only if
// transitively reachable.

// Compatibility scanner for final LLVM spelling payloads that still may contain
// @name / @"quoted name" global references. Callers should prefer structured
// LinkNameId carriers when they exist and use this only for classified legacy
// payloads or unresolved producer-boundary text.
static void scan_refs(const std::string& s,
                      std::unordered_set<std::string>& refs) {
  size_t pos = 0;
  while ((pos = s.find('@', pos)) != std::string::npos) {
    ++pos;
    if (pos < s.size() && s[pos] == '"') {
      ++pos;  // skip opening "
      size_t start = pos;
      while (pos < s.size() && s[pos] != '"') ++pos;
      if (pos > start)
        refs.insert("\"" + s.substr(start, pos - start) + "\"");
      if (pos < s.size()) ++pos;  // skip closing "
    } else {
      size_t start = pos;
      while (pos < s.size() &&
             (std::isalnum(static_cast<unsigned char>(s[pos])) ||
              s[pos] == '_' || s[pos] == '.'))
        ++pos;
      if (pos > start) refs.insert(s.substr(start, pos - start));
    }
  }
}

struct LirGlobalRefs {
  std::unordered_set<LinkNameId> link_name_ids;
  // Compatibility fallback names collected from final LLVM spelling payloads
  // when no structured LinkNameId carrier exists for that reference.
  std::unordered_set<std::string> names;
};

static void collect_operand_ref(const LirOperand& operand, LirGlobalRefs& refs) {
  if (const auto id = operand.link_name_id();
      id != nullptr && *id != kInvalidLinkName) {
    refs.link_name_ids.insert(*id);
    return;
  }
  collect_lir_global_symbol_refs_from_text(
      operand.str(), [&](std::string_view ref) {
        refs.names.insert(std::string(ref));
      });
}

// Collect all global symbol references from a single LIR instruction.
// Prefer structured direct-call identities and scan legacy final-spelling or
// compatibility string operands only where LIR still has no semantic symbol
// carrier.
static void collect_inst_refs(const LirInst& inst, LirGlobalRefs& refs) {
  auto S = [&](const std::string& s) { scan_refs(s, refs.names); };
  // Visit each typed LIR op and scan its string-valued operand fields.
  // Types with no string operands (legacy typed ops with no producers) are
  // handled by the default case.
  auto visitor = [&](const auto& op) {
    using T = std::decay_t<decltype(op)>;
    if constexpr (std::is_same_v<T, LirCallOp>) {
      if (op.direct_callee_link_name_id != kInvalidLinkName) {
        refs.link_name_ids.insert(op.direct_callee_link_name_id);
        if (lir_call_has_complete_structured_arg_authority(op)) {
          for (const LirCallArg& arg : op.structured_args) {
            collect_operand_ref(arg.operand, refs);
          }
        } else {
          collect_lir_global_symbol_refs_from_call_args(
              op.args_str,
              [&](std::string_view ref) {
                refs.names.insert(std::string(ref));
              });
        }
      } else {
        collect_lir_global_symbol_refs_from_call(
            op,
            [&](std::string_view ref) {
              refs.names.insert(std::string(ref));
            });
      }
    } else if constexpr (std::is_same_v<T, LirStoreOp>) {
      collect_operand_ref(op.val, refs);
      collect_operand_ref(op.ptr, refs);
    } else if constexpr (std::is_same_v<T, LirLoadOp>) {
      collect_operand_ref(op.ptr, refs);
    } else if constexpr (std::is_same_v<T, LirGepOp>) {
      collect_operand_ref(op.ptr, refs);
      for (const auto& idx : op.indices) {
        if (!idx.is_authoritative()) S(idx.presentation());
      }
    } else if constexpr (std::is_same_v<T, LirCastOp>) {
      collect_operand_ref(op.operand, refs);
    } else if constexpr (std::is_same_v<T, LirBinOp>) {
      collect_operand_ref(op.lhs, refs); collect_operand_ref(op.rhs, refs);
    } else if constexpr (std::is_same_v<T, LirCmpOp>) {
      collect_operand_ref(op.lhs, refs); collect_operand_ref(op.rhs, refs);
    } else if constexpr (std::is_same_v<T, LirPhiOp>) {
      for (const auto& incoming : op.incoming) {
        collect_operand_ref(incoming.value, refs);
      }
    } else if constexpr (std::is_same_v<T, LirSelectOp>) {
      collect_operand_ref(op.cond, refs); collect_operand_ref(op.true_val, refs); collect_operand_ref(op.false_val, refs);
    } else if constexpr (std::is_same_v<T, LirExtractValueOp>) {
      S(op.agg);
    } else if constexpr (std::is_same_v<T, LirInsertValueOp>) {
      S(op.agg); S(op.elem);
    } else if constexpr (std::is_same_v<T, LirAllocaOp>) {
      S(op.count);
    } else if constexpr (std::is_same_v<T, LirInlineAsmOp>) {
      S(op.args_str);
    } else if constexpr (std::is_same_v<T, LirAbsOp>) {
      S(op.arg);
    } else if constexpr (std::is_same_v<T, LirInsertElementOp>) {
      S(op.vec); S(op.elem);
    } else if constexpr (std::is_same_v<T, LirExtractElementOp>) {
      S(op.vec);
    } else if constexpr (std::is_same_v<T, LirShuffleVectorOp>) {
      S(op.vec1); S(op.vec2);
    } else if constexpr (std::is_same_v<T, LirVaArgOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirMemcpyOp>) {
      S(op.dst); S(op.src);
    } else if constexpr (std::is_same_v<T, LirVaStartOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirVaEndOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirVaCopyOp>) {
      S(op.dst_ptr); S(op.src_ptr);
    } else if constexpr (std::is_same_v<T, LirStackRestoreOp>) {
      S(op.saved_ptr);
    } else if constexpr (std::is_same_v<T, LirIndirectBrOp>) {
      S(op.addr);
    }
    // LirStackSaveOp and legacy typed ops: no string operands to scan.
  };
  std::visit(visitor, inst);
}

static void collect_function_signature_refs(const LirModule& mod,
                                            const LirFunction& fn,
                                            LirGlobalRefs& refs) {
  const LirFunctionSignatureStoreEntry* signature =
      mod.find_function_signature(fn.function_signature_ref);
  if (!signature) return;

  // Function-signature family refs currently carry type-family facts, not
  // callable symbol references. Walking them here marks the collector's
  // authority boundary explicitly and keeps retained signature_text out of
  // reachability once the nominal store owns the signature.
  auto visit_type_ref = [](const LirTypeRef&) {};
  if (signature->return_type_ref.has_value()) {
    visit_type_ref(*signature->return_type_ref);
  }
  for (const LirTypeRef& param_type : signature->fixed_param_type_refs) {
    visit_type_ref(param_type);
  }
  (void)refs;
}

// Collect all global references from a function's body.
static LirGlobalRefs collect_fn_refs(const LirModule& mod, const LirFunction& fn) {
  LirGlobalRefs refs;
  if (fn.function_signature_ref.valid()) {
    collect_function_signature_refs(mod, fn, refs);
  } else {
    // Unresolved producer-carrier boundary: signature_text is final LLVM header
    // spelling plus template-comment compatibility payload. This remains only
    // for legacy functions that have no nominal function-signature ref.
    scan_refs(fn.signature_text, refs.names);
  }
  // Scan hoisted allocas.
  for (const auto& inst : fn.alloca_insts) collect_inst_refs(inst, refs);
  // Scan block instructions.
  for (const auto& blk : fn.blocks) {
    for (const auto& inst : blk.insts) collect_inst_refs(inst, refs);
  }
  return refs;
}

void eliminate_dead_internals(LirModule& mod) {
  std::unordered_map<LinkNameId, size_t> discardable_by_link_name;
  // Compatibility fallback for legacy reference payloads that have not been
  // paired with a LinkNameId. Structured references seed through
  // discardable_by_link_name first.
  std::unordered_map<std::string, size_t> discardable_by_name;
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& f = mod.functions[i];
    if (!f.can_elide_if_unreferenced) continue;
    if (f.link_name_id != kInvalidLinkName) {
      discardable_by_link_name.emplace(f.link_name_id, i);
    }
    discardable_by_name.emplace(f.name, i);
  }
  if (discardable_by_link_name.empty() && discardable_by_name.empty()) return;

  // Build per-function reference sets.
  std::vector<LirGlobalRefs> fn_refs(mod.functions.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    fn_refs[i] = collect_fn_refs(mod, mod.functions[i]);
  }

  std::vector<bool> reachable(mod.functions.size(), false);
  std::vector<size_t> worklist;

  auto seed_idx = [&](size_t idx) {
    if (!reachable[idx]) {
      reachable[idx] = true;
      worklist.push_back(idx);
    }
  };

  auto seed = [&](const LirGlobalRefs& refs) {
    for (const LinkNameId id : refs.link_name_ids) {
      auto it = discardable_by_link_name.find(id);
      if (it != discardable_by_link_name.end()) {
        seed_idx(it->second);
      }
    }
    for (const auto& name : refs.names) {
      auto it = discardable_by_name.find(name);
      if (it != discardable_by_name.end()) {
        seed_idx(it->second);
      }
    }
  };

  // Seed from non-discardable function bodies.
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    if (!mod.functions[i].can_elide_if_unreferenced) seed(fn_refs[i]);
  }

  // Seed from global initializers.
  for (const auto& g : mod.globals) {
    LirGlobalRefs grefs;
    bool has_structured_initializer_function_ids = false;
    for (const LinkNameId id : g.initializer_function_link_name_ids) {
      if (id != kInvalidLinkName) {
        grefs.link_name_ids.insert(id);
        has_structured_initializer_function_ids = true;
      }
    }
    if (!has_structured_initializer_function_ids) {
      scan_refs(g.init_text, grefs.names);
    }
    seed(grefs);
  }

  // Propagate: internal functions referenced by reachable internal functions.
  while (!worklist.empty()) {
    const size_t cur = worklist.back();
    worklist.pop_back();
    seed(fn_refs[cur]);
  }

  // Remove unreachable discardable functions.
  size_t idx = 0;
  mod.functions.erase(
      std::remove_if(mod.functions.begin(), mod.functions.end(),
                     [&](const LirFunction& f) {
                       const bool remove = f.can_elide_if_unreferenced &&
                                           !reachable[idx];
                       ++idx;
                       return remove;
                     }),
      mod.functions.end());
}

// ── Main lowering entry point ────────────────────────────────────────────────

LirModule lower(const c4c::hir::Module& hir_mod, const LowerOptions& options) {
  using namespace c4c::codegen::llvm_helpers;
  const c4c::TargetProfile& target_profile = hir_mod.target_profile;
  // Module-level orchestration: owned by hir_to_lir, not StmtEmitter.
  set_active_target_profile(target_profile);

  LirModule module;
  module.target_profile = target_profile;
  module.data_layout = !hir_mod.data_layout.empty()
      ? hir_mod.data_layout
      : llvm_default_datalayout(target_profile);
  module.link_name_texts = hir_mod.link_name_texts;
  module.link_names = hir_mod.link_names;
  module.struct_names.attach_text_table(module.link_name_texts.get());
  // Register every HIR aggregate definition before lowering its declarations
  // or any later aggregate occurrences. This is the producer-side handoff;
  // build_type_decls consumes these refs without consulting legacy adapters.
  for (const auto& [_, definition] : hir_mod.struct_defs) {
    if (!definition.aggregate_ref) {
      (void)hir_mod.register_aggregate_definition(definition);
    }
  }
  module.type_decls = build_type_decls(hir_mod, &module);
  module.prefer_semantic_va_ops = options.preserve_semantic_va_ops;

  auto global_indices = dedup_globals(hir_mod);
  auto fn_indices = dedup_functions(hir_mod);

  // Per-item lowering: hir_to_lir owns iteration and the module;
  // Const-init lowering (globals) — standalone, no StmtEmitter dependency.
  ConstInitEmitter const_init(hir_mod, module);

  // StmtEmitter owns statement/expression lowering and writes into module.
  StmtEmitter emitter(hir_mod);
  emitter.set_module(module);

  bool any_vla = false;
  std::vector<LirSpecEntry> spec_entries;

  lower_globals(global_indices, hir_mod, const_init, module);
  for (size_t idx : fn_indices) {
    const auto& fn = hir_mod.functions[idx];
    std::string sig = build_fn_signature(hir_mod, fn, &module);

    if (fn.linkage.is_extern && fn.blocks.empty()) {
      // Declaration — no body to lower; hir_to_lir owns this directly.
      LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.link_name_id = fn.link_name_id;
      lir_fn.is_internal = false;
      lir_fn.can_elide_if_unreferenced = false;
      lir_fn.is_declaration = true;
      lir_fn.return_type =
          lir_owned_type_spec(hir_mod, fn.return_type, &module);
      populate_lir_function_params(hir_mod, fn, &module, lir_fn);
      lir_fn.signature_text = sig;
      populate_signature_type_refs(hir_mod, fn, &module, lir_fn);
      register_function_signature_ref(module, lir_fn);
      module.functions.push_back(std::move(lir_fn));
    } else {
      // Definition — hir_to_lir owns the local LirFunction shell, FnCtx setup,
      // alloca hoisting, VLA stack save, and spec entry collection.
      // StmtEmitter owns statement / expression emission only.
      // Keep the shell local through emission so FnCtx can safely allocate
      // value IDs from its exact owner without module-vector invalidation.
      LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.link_name_id = fn.link_name_id;
      lir_fn.is_internal = fn.linkage.is_static;
      const bool is_std_impl_helper = fn.name.rfind("std::__", 0) == 0;
      lir_fn.can_elide_if_unreferenced =
          fn.linkage.is_static || fn.linkage.is_inline || is_std_impl_helper;
      lir_fn.is_declaration = false;
      lir_fn.return_type =
          lir_owned_type_spec(hir_mod, fn.return_type, &module);
      populate_lir_function_params(hir_mod, fn, &module, lir_fn);
      lir_fn.signature_text = sig;
      populate_signature_type_refs(hir_mod, fn, &module, lir_fn);
      register_function_signature_ref(module, lir_fn);

      auto ctx = init_fn_ctx(hir_mod, fn, lir_fn, &module);
      if (ctx.vla_stack_save_ptr) any_vla = true;
      auto block_order = build_block_order(fn);

      // Spec entries — owned by hir_to_lir.
      if (!fn.template_origin.empty() && !fn.spec_key.empty()) {
        spec_entries.push_back(
            {fn.spec_key.canonical, fn.template_origin, std::string(fn.name), fn.link_name_id});
      }

      // Block iteration — owned by hir_to_lir.
      for (size_t bi = 0; bi < block_order.size(); ++bi) {
        const auto* blk = block_order[bi];
        ctx.current_block_id = blk->id.value;
        if (bi > 0) {
          emit_lbl(ctx, {block_lbl(blk->id), LirBlockId{blk->id.value}});
        }
        for (const auto& stmt : blk->stmts) {
          emitter.emit_stmt(ctx, stmt);
        }
      }

      // Finish the locally owned LirFunction from accumulated context.
      lir_fn.alloca_insts = std::move(ctx.alloca_insts);
      lir_fn.blocks = std::move(ctx.lir_blocks);

      inject_fallthrough_returns(lir_fn, hir_mod, fn);
      module.functions.push_back(std::move(lir_fn));
    }
  }

  if (any_vla) module.need_stacksave = true;

  // Add spec entries collected by hir_to_lir.
  for (auto& e : spec_entries) {
    module.spec_entries.push_back(std::move(e));
  }

  // Module-level finalization: owned by hir_to_lir.
  finalize_module(module, hir_mod);

  // Dead internal function elimination: owned by hir_to_lir.
  eliminate_dead_internals(module);

  return module;
}

}  // namespace c4c::codegen::lir
