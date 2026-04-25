#include "lowering.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

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
    const std::string& tag, const HirStructDef& sd) const {
  TypeSpec ts{};
  ts.base = sd.is_union ? TB_UNION : TB_STRUCT;
  ts.tag = tag.c_str();
  return lookup_structured_layout(mod_, module_, ts, "field-chain");
}

bool StmtEmitter::find_field_chain(const std::string& tag, const std::string& field_name,
                                   std::vector<FieldStep>& chain, TypeSpec& out_field_ts) {
  const auto it = mod_.struct_defs.find(tag);
  if (it == mod_.struct_defs.end()) return false;
  const HirStructDef& sd = it->second;
  const StructuredLayoutLookup layout = lookup_field_chain_layout(tag, sd);
  const StructNameId structured_name_id =
      layout.structured_decl ? layout.structured_name_id : kInvalidStructName;

  for (const auto& f : sd.fields) {
    if (f.name == field_name) {
      FieldStep step;
      step.tag = tag;
      step.structured_name_id = structured_name_id;
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
    if (!f.is_anon_member) continue;
    if (!f.elem_type.tag || !f.elem_type.tag[0]) continue;
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    if (find_field_chain(f.elem_type.tag, field_name, sub_chain, sub_ts)) {
      FieldStep step;
      step.tag = tag;
      step.structured_name_id = structured_name_id;
      step.llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
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
    if (!find_field_chain(base_tag, field_name, sub_chain, sub_ts)) continue;
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = structured_name_id;
    step.llvm_idx = llvm_struct_base_slot(mod_, sd, base_index);
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
                                                       TypeSpec& out_field_ts) {
  if (member_symbol_id == kInvalidMemberSymbol) return false;
  const auto it = mod_.struct_defs.find(tag);
  if (it == mod_.struct_defs.end()) return false;
  const HirStructDef& sd = it->second;
  const StructuredLayoutLookup layout = lookup_field_chain_layout(tag, sd);
  const StructNameId structured_name_id =
      layout.structured_decl ? layout.structured_name_id : kInvalidStructName;

  for (const auto& f : sd.fields) {
    if (f.member_symbol_id != member_symbol_id) continue;
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = structured_name_id;
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
    if (!f.is_anon_member) continue;
    if (!f.elem_type.tag || !f.elem_type.tag[0]) continue;
    std::vector<FieldStep> sub_chain;
    TypeSpec sub_ts{};
    if (!find_field_chain_by_member_symbol_id(f.elem_type.tag, member_symbol_id, sub_chain,
                                              sub_ts)) {
      continue;
    }
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = structured_name_id;
    step.llvm_idx = llvm_struct_field_slot(mod_, sd, f.llvm_idx);
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
    if (!find_field_chain_by_member_symbol_id(base_tag, member_symbol_id, sub_chain, sub_ts)) {
      continue;
    }
    FieldStep step;
    step.tag = tag;
    step.structured_name_id = structured_name_id;
    step.llvm_idx = llvm_struct_base_slot(mod_, sd, base_index);
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
