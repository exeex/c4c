#include "lowering.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

TypeSpec field_chain_owner_type_spec(const std::string& tag, const HirStructDef& sd) {
  TypeSpec ts{};
  ts.base = sd.is_union ? TB_UNION : TB_STRUCT;
  ts.tag_text_id = sd.tag_text_id;
  ts.namespace_context_id = sd.ns_qual.context_id;
  ts.is_global_qualified = sd.ns_qual.is_global_qualified;
  ts.n_qualifier_segments = static_cast<int>(sd.ns_qual.segment_text_ids.size());
  ts.qualifier_text_ids = ts.n_qualifier_segments > 0
                              ? const_cast<TextId*>(sd.ns_qual.segment_text_ids.data())
                              : nullptr;
  set_typespec_legacy_tag_if_present(ts, tag.c_str(), 0);
  return ts;
}

std::optional<std::string> field_chain_tag_from_structured_name_id(
    const Module& mod, const LirModule* lir_module, StructNameId name_id) {
  if (!lir_module || name_id == kInvalidStructName) return std::nullopt;
  const std::string_view lir_name = lir_module->struct_names.spelling(name_id);
  if (lir_name.empty()) return std::nullopt;
  for (const auto& [tag, _] : mod.struct_defs) {
    if (llvm_struct_type_str(tag) == lir_name) return tag;
  }
  return std::nullopt;
}

StructNameId structured_child_name_id(const StructuredLayoutLookup& layout, int llvm_idx) {
  if (!layout.structured_decl || llvm_idx < 0) return kInvalidStructName;
  const auto field_index = static_cast<std::size_t>(llvm_idx);
  if (field_index >= layout.structured_decl->fields.size()) return kInvalidStructName;
  const LirTypeRef& type = layout.structured_decl->fields[field_index].type;
  return type.has_struct_name_id() ? type.struct_name_id() : kInvalidStructName;
}

struct FieldChainNestedAggregate {
  std::optional<std::string> tag;
  StructNameId structured_name_id = kInvalidStructName;
};

FieldChainNestedAggregate field_chain_nested_aggregate(
    const Module& mod, const LirModule* lir_module,
    const StructuredLayoutLookup& layout, const HirStructField& field,
    int llvm_idx, bool allow_structured_child) {
  FieldChainNestedAggregate result;
  if (!field.is_anon_member) return result;

  if (allow_structured_child) {
    result.structured_name_id = structured_child_name_id(layout, llvm_idx);
    result.tag =
        field_chain_tag_from_structured_name_id(mod, lir_module,
                                                result.structured_name_id);
    if (result.tag) return result;
  }

  // Secondary compatibility path for anonymous aggregate members whose LIR
  // field metadata is not yet rich enough to name the child layout.
  result.structured_name_id = kInvalidStructName;
  result.tag = typespec_aggregate_compatibility_tag(mod, field.elem_type);
  return result;
}

}  // namespace

// Draft-only staging file for Step 3 of the stmt_emitter split refactor.
// This file captures the type-centric lookup and promotion helpers.

TypeSpec StmtEmitter::drop_one_array_dim(TypeSpec ts) {
  if (ts.array_rank <= 0) return ts;
  for (int i = 0; i < ts.array_rank - 1; ++i) ts.array_dims[i] = ts.array_dims[i + 1];
  ts.array_dims[ts.array_rank - 1] = -1;
  ts.array_rank--;
  ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
  return ts;
}

TypeSpec StmtEmitter::drop_one_indexed_element_type(TypeSpec ts) {
  if (outer_array_rank(ts) > 0) {
    return drop_one_array_dim(ts);
  }
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) {
    ts.ptr_level--;
    if (ts.ptr_level == 0) ts.is_ptr_to_array = false;
    return ts;
  }
  if (ts.ptr_level > 0) {
    ts.ptr_level--;
  }
  return ts;
}

TypeSpec StmtEmitter::resolve_indexed_gep_pointee_type(TypeSpec ts) {
  if (ts.ptr_level > 0 || outer_array_rank(ts) > 0) {
    return drop_one_indexed_element_type(ts);
  }
  ts = {};
  ts.base = TB_CHAR;
  return ts;
}

stmt_emitter_detail::StructuredLayoutLookup StmtEmitter::lookup_field_chain_layout(
    const std::string& tag, const HirStructDef& sd, StructNameId structured_name_id) const {
  const TypeSpec ts = field_chain_owner_type_spec(tag, sd);
  return lookup_structured_layout(mod_, module_, ts, "field-chain", structured_name_id);
}

bool StmtEmitter::find_field_chain(const std::string& tag, const std::string& field_name,
                                   std::vector<FieldStep>& chain, TypeSpec& out_field_ts,
                                   StructNameId structured_name_id) {
  const auto legacy_compat_it = mod_.struct_defs.find(tag);
  if (legacy_compat_it == mod_.struct_defs.end()) return false;
  const HirStructDef& sd = legacy_compat_it->second;
  const StructuredLayoutLookup layout = lookup_field_chain_layout(tag, sd, structured_name_id);
  const StructNameId step_structured_name_id =
      layout.structured_decl ? layout.structured_name_id : kInvalidStructName;

  for (const auto& f : sd.fields) {
    if (f.name == field_name) {
      FieldStep step;
      step.tag = tag;
      step.structured_name_id = step_structured_name_id;
      step.llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
      step.is_union = sd.is_union;
      if (f.bit_width >= 0) {
        step.bit_width = f.bit_width;
        step.bit_offset = f.bit_offset;
        step.storage_unit_bits = f.storage_unit_bits;
        step.bf_is_signed = f.is_bf_signed;
      }
      chain.push_back(std::move(step));
      out_field_ts = field_decl_type(f);
      return true;
    }
  }

  for (const auto& f : sd.fields) {
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    const int llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
    const FieldChainNestedAggregate nested = field_chain_nested_aggregate(
        mod_, module_, layout, f, llvm_idx, !sd.is_union);
    if (!nested.tag || nested.tag->empty()) continue;
    if (find_field_chain(*nested.tag, field_name, sub_chain, sub_ts,
                         nested.structured_name_id)) {
      FieldStep step;
      step.tag = tag;
      step.structured_name_id = step_structured_name_id;
      step.llvm_idx = llvm_idx;
      step.is_union = sd.is_union;
      chain.push_back(std::move(step));
      for (const auto& s : sub_chain) chain.push_back(s);
      out_field_ts = sub_ts;
      return true;
    }
  }

  for (size_t base_index = 0; base_index < sd.base_tags.size(); ++base_index) {
    const auto& base_tag = sd.base_tags[base_index];
    if (base_tag.empty()) continue;
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    const int llvm_idx = llvm_struct_base_slot(mod_, sd, base_index);
    const StructNameId child_structured_name_id =
        sd.is_union ? kInvalidStructName : structured_child_name_id(layout, llvm_idx);
    if (!find_field_chain(base_tag, field_name, sub_chain, sub_ts, child_structured_name_id)) {
      continue;
    }
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = step_structured_name_id;
    step.llvm_idx = llvm_idx;
    step.is_union = sd.is_union;
    chain.push_back(std::move(step));
    for (const auto& s : sub_chain) chain.push_back(s);
    out_field_ts = sub_ts;
    return true;
  }
  return false;
}

bool StmtEmitter::find_field_chain_by_member_symbol_id(const std::string& tag,
                                                       MemberSymbolId member_symbol_id,
                                                       std::vector<FieldStep>& chain,
                                                       TypeSpec& out_field_ts,
                                                       StructNameId structured_name_id) {
  if (member_symbol_id == kInvalidMemberSymbol) return false;
  const auto legacy_compat_it = mod_.struct_defs.find(tag);
  if (legacy_compat_it == mod_.struct_defs.end()) return false;
  const HirStructDef& sd = legacy_compat_it->second;
  const StructuredLayoutLookup layout = lookup_field_chain_layout(tag, sd, structured_name_id);
  const StructNameId step_structured_name_id =
      layout.structured_decl ? layout.structured_name_id : kInvalidStructName;

  for (const auto& f : sd.fields) {
    if (f.member_symbol_id != member_symbol_id) continue;
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = step_structured_name_id;
    step.llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
    step.is_union = sd.is_union;
    if (f.bit_width >= 0) {
      step.bit_width = f.bit_width;
      step.bit_offset = f.bit_offset;
      step.storage_unit_bits = f.storage_unit_bits;
      step.bf_is_signed = f.is_bf_signed;
    }
    chain.push_back(std::move(step));
    out_field_ts = field_decl_type(f);
    return true;
  }

  for (const auto& f : sd.fields) {
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    const int llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
    const FieldChainNestedAggregate nested = field_chain_nested_aggregate(
        mod_, module_, layout, f, llvm_idx, !sd.is_union);
    if (!nested.tag || nested.tag->empty()) continue;
    if (!find_field_chain_by_member_symbol_id(*nested.tag, member_symbol_id, sub_chain,
                                              sub_ts, nested.structured_name_id)) {
      continue;
    }
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = step_structured_name_id;
    step.llvm_idx = llvm_idx;
    step.is_union = sd.is_union;
    chain.push_back(std::move(step));
    for (const auto& s : sub_chain) chain.push_back(s);
    out_field_ts = sub_ts;
    return true;
  }

  for (size_t base_index = 0; base_index < sd.base_tags.size(); ++base_index) {
    const auto& base_tag = sd.base_tags[base_index];
    if (base_tag.empty()) continue;
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    const int llvm_idx = llvm_struct_base_slot(mod_, sd, base_index);
    const StructNameId child_structured_name_id =
        sd.is_union ? kInvalidStructName : structured_child_name_id(layout, llvm_idx);
    if (!find_field_chain_by_member_symbol_id(base_tag, member_symbol_id, sub_chain, sub_ts,
                                              child_structured_name_id)) {
      continue;
    }
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = step_structured_name_id;
    step.llvm_idx = llvm_idx;
    step.is_union = sd.is_union;
    chain.push_back(std::move(step));
    for (const auto& s : sub_chain) chain.push_back(s);
    out_field_ts = sub_ts;
    return true;
  }
  return false;
}

bool StmtEmitter::resolve_field_access(const std::string& tag, const std::string& field_name,
                                       std::vector<FieldStep>& chain, TypeSpec& out_field_ts,
                                       BitfieldAccess* out_bf) {
  if (!find_field_chain(tag, field_name, chain, out_field_ts)) {
    return false;
  }
  if (out_bf && !chain.empty()) {
    const auto& last = chain.back();
    out_bf->bit_width = last.bit_width;
    out_bf->bit_offset = last.bit_offset;
    out_bf->storage_unit_bits = last.storage_unit_bits;
    out_bf->is_signed = last.bf_is_signed;
  }
  return true;
}

bool StmtEmitter::resolve_field_access_by_member_symbol_id(
    const std::string& tag, MemberSymbolId member_symbol_id, std::vector<FieldStep>& chain,
    TypeSpec& out_field_ts, BitfieldAccess* out_bf) {
  if (!find_field_chain_by_member_symbol_id(tag, member_symbol_id, chain, out_field_ts)) {
    return false;
  }
  if (out_bf && !chain.empty()) {
    const auto& last = chain.back();
    out_bf->bit_width = last.bit_width;
    out_bf->bit_offset = last.bit_offset;
    out_bf->storage_unit_bits = last.storage_unit_bits;
    out_bf->is_signed = last.bf_is_signed;
  }
  return true;
}

int StmtEmitter::bitfield_promoted_bits(const BitfieldAccess& bf) {
  if (bf.bit_width <= 31) return 32;
  if (bf.bit_width == 32) return 32;
  return bf.storage_unit_bits;
}

TypeBase StmtEmitter::bitfield_promoted_base(int bit_width, bool is_signed,
                                             int storage_unit_bits) {
  if (bit_width < 32) return TB_INT;
  if (bit_width == 32) return is_signed ? TB_INT : TB_UINT;
  if (storage_unit_bits > 32) return is_signed ? TB_LONGLONG : TB_ULONGLONG;
  return is_signed ? TB_INT : TB_UINT;
}

TypeSpec StmtEmitter::bitfield_promoted_ts(const BitfieldAccess& bf) {
  TypeSpec ts{};
  ts.base = bitfield_promoted_base(bf.bit_width, bf.is_signed, bf.storage_unit_bits);
  return ts;
}

}  // namespace c4c::codegen::lir
