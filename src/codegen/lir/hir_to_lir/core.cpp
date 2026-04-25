#include "lowering.hpp"
#include "ir.hpp"
#include "canonical_symbol.hpp"
#include "../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace stmt_emitter_detail {

static int align_to_bytes(int value, int align) {
  if (align <= 1) return value;
  const int rem = value % align;
  return rem == 0 ? value : value + (align - rem);
}

static void append_legacy_layout_field_types(const Module& mod, const HirStructDef& sd,
                                             std::vector<std::string>& out) {
  if (sd.fields.empty() && sd.base_tags.empty()) {
    if (sd.size_bytes > 0) out.push_back("[" + std::to_string(sd.size_bytes) + " x i8]");
    return;
  }

  if (sd.is_union) {
    out.push_back("[" + std::to_string(sd.size_bytes) + " x i8]");
    return;
  }

  int cur_offset = 0;
  for (const auto& base_tag : sd.base_tags) {
    const auto bit = mod.struct_defs.find(base_tag);
    if (bit == mod.struct_defs.end()) continue;
    const auto& base = bit->second;
    const int base_offset = align_to_bytes(cur_offset, std::max(1, base.align_bytes));
    if (base_offset > cur_offset) {
      out.push_back("[" + std::to_string(base_offset - cur_offset) + " x i8]");
      cur_offset = base_offset;
    }
    out.push_back(llvm_struct_type_str(base_tag));
    cur_offset = base_offset + std::max(0, base.size_bytes);
  }

  int last_idx = -1;
  for (const auto& f : sd.fields) {
    if (f.llvm_idx == last_idx) continue;
    last_idx = f.llvm_idx;
    if (f.offset_bytes > cur_offset) {
      out.push_back("[" + std::to_string(f.offset_bytes - cur_offset) + " x i8]");
      cur_offset = f.offset_bytes;
    }
    out.push_back(llvm_field_ty(f));
    cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
  }

  if (sd.size_bytes > cur_offset) {
    out.push_back("[" + std::to_string(sd.size_bytes - cur_offset) + " x i8]");
  }
}

static bool structured_fields_match_legacy_layout(const Module& mod,
                                                  const HirStructDef& legacy,
                                                  const LirStructDecl& structured) {
  std::vector<std::string> legacy_fields;
  append_legacy_layout_field_types(mod, legacy, legacy_fields);
  if (legacy_fields.size() != structured.fields.size()) return false;
  for (size_t i = 0; i < legacy_fields.size(); ++i) {
    if (legacy_fields[i] != structured.fields[i].type.str()) return false;
  }
  return true;
}

static std::vector<std::string> structured_layout_field_types(
    const LirStructDecl& structured) {
  std::vector<std::string> fields;
  fields.reserve(structured.fields.size());
  for (const auto& field : structured.fields) fields.push_back(field.type.str());
  return fields;
}

static void record_structured_layout_observation(
    const Module& mod, const LirModule& lir_module, const TypeSpec& ts, const char* site,
    const StructuredLayoutLookup& lookup) {
  LirStructuredLayoutObservation observation;
  observation.site = site ? site : "lookup_structured_layout";
  observation.type_name = ts.tag ? llvm_struct_type_str(ts.tag) : std::string();
  observation.name_id = lookup.structured_name_id;
  observation.legacy_found = lookup.legacy_decl != nullptr;
  observation.structured_found = lookup.structured_decl != nullptr;
  observation.parity_checked = lookup.structured_parity_checked;
  observation.parity_matches = lookup.structured_parity_matches;
  if (lookup.legacy_decl) {
    observation.legacy_size_bytes = lookup.legacy_decl->size_bytes;
    observation.legacy_align_bytes = lookup.legacy_decl->align_bytes;
    append_legacy_layout_field_types(mod, *lookup.legacy_decl,
                                     observation.legacy_field_types);
  }
  if (lookup.structured_decl) {
    observation.structured_field_types =
        structured_layout_field_types(*lookup.structured_decl);
  }
  lir_module.structured_layout_observations.push_back(std::move(observation));
}

StructuredLayoutLookup lookup_structured_layout(const Module& mod,
                                                const LirModule* lir_module,
                                                const TypeSpec& ts,
                                                const char* site) {
  StructuredLayoutLookup result;
  if ((ts.base != TB_STRUCT && ts.base != TB_UNION) || ts.ptr_level != 0 ||
      ts.array_rank != 0 || !ts.tag || !ts.tag[0]) {
    return result;
  }

  const auto legacy_it = mod.struct_defs.find(ts.tag);
  if (legacy_it != mod.struct_defs.end()) result.legacy_decl = &legacy_it->second;

  if (!lir_module) return result;
  const std::string rendered_name = llvm_struct_type_str(ts.tag);
  result.structured_name_id = lir_module->struct_names.find(rendered_name);
  if (result.structured_name_id == kInvalidStructName) return result;

  result.structured_lookup_attempted = true;
  result.structured_decl = lir_module->find_struct_decl(result.structured_name_id);
  if (!result.structured_decl || !result.legacy_decl) {
    record_structured_layout_observation(mod, *lir_module, ts, site, result);
    return result;
  }

  result.structured_parity_checked = true;
  result.structured_parity_matches =
      structured_fields_match_legacy_layout(mod, *result.legacy_decl, *result.structured_decl);
  record_structured_layout_observation(mod, *lir_module, ts, site, result);
  return result;
}

bool amd64_registers_available(const llvm_cc::Amd64VarargInfo& layout,
                               const Amd64CallArgState& state) {
  if (layout.gp_chunks > 0) {
    const int remaining_gp = kAmd64GpAreaBytes - state.gp_bytes;
    if (remaining_gp < layout.gp_chunks * 8) return false;
  }
  if (layout.sse_slots > 0) {
    const int remaining_sse = kAmd64FpAreaBytes - state.sse_bytes;
    if (remaining_sse < layout.sse_slots * 16) return false;
  }
  return true;
}

void amd64_track_usage(const llvm_cc::Amd64VarargInfo& layout,
                       Amd64CallArgState& state) {
  state.gp_bytes += layout.gp_chunks * 8;
  state.sse_bytes += layout.sse_slots * 16;
}

void amd64_account_type_if_needed(const hir::Module& mod, const TypeSpec& ts,
                                  Amd64CallArgState* state) {
  if (!state) return;
  const auto layout = llvm_cc::classify_amd64_vararg(ts, mod);
  if (layout.size_bytes <= 0 || layout.needs_memory) return;
  amd64_track_usage(layout, *state);
}

bool amd64_fixed_aggregate_byval(const hir::Module& mod, const TypeSpec& ts) {
  return llvm_target_is_amd64_sysv(mod.target_profile) &&
         llvm_cc::amd64_fixed_aggregate_passed_byval(ts, mod);
}

void open_lbl(FnCtx& ctx, const std::string& lbl) {
  lir::LirBlock blk;
  blk.id = lir::LirBlockId{static_cast<uint32_t>(ctx.lir_blocks.size())};
  blk.label = lbl;
  ctx.lir_blocks.push_back(std::move(blk));
  ctx.current_block_idx = ctx.lir_blocks.size() - 1;
  ctx.last_term = false;
}

void emit_condbr_and_open_lbl(FnCtx& ctx, const std::string& cond,
                              const std::string& true_label,
                              const std::string& false_label,
                              const std::string& open_label) {
  (void)set_terminator_if_open(ctx, lir::LirCondBr{cond, true_label, false_label});
  open_lbl(ctx, open_label);
}

void emit_condbr_and_open_sibling_lbl(FnCtx& ctx, const std::string& cond,
                                      const std::string& true_label,
                                      const std::string& false_label,
                                      const std::string& sibling_label) {
  emit_condbr_and_open_lbl(ctx, cond, true_label, false_label, sibling_label);
}

void emit_condbr_and_fallthrough_lbl(FnCtx& ctx, const std::string& cond,
                                     const std::string& true_label,
                                     const std::string& false_label) {
  emit_condbr_and_open_lbl(ctx, cond, true_label, false_label, false_label);
}

bool set_terminator_if_open(FnCtx& ctx, lir::LirTerminator terminator) {
  if (!std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator)) {
    return false;
  }
  ctx.cur_block().terminator = std::move(terminator);
  ctx.last_term = true;
  return true;
}

TypeSpec sig_return_type(const FnPtrSig& sig) { return sig.return_type.spec; }

TypeSpec sig_param_type(const FnPtrSig& sig, size_t i) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig && i < fsig->params.size()) return sema::typespec_from_canonical(fsig->params[i]);
  }
  return i < sig.params.size() ? sig.params[i].spec : TypeSpec{};
}

bool sig_param_is_va_list_value(const FnPtrSig& sig, size_t i) {
  const TypeSpec ts = sig_param_type(sig, i);
  return ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0;
}

size_t sig_param_count(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->params.size();
  }
  return sig.params.size();
}

bool sig_is_variadic(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->is_variadic;
  }
  return sig.variadic;
}

bool sig_has_void_param_list(const FnPtrSig& sig) {
  if (sig_param_count(sig) != 1) return false;
  const TypeSpec ts = sig_param_type(sig, 0);
  return ts.base == TB_VOID && ts.ptr_level == 0 && ts.array_rank == 0;
}

static bool sig_has_explicit_prototype(const FnPtrSig& sig) {
  if (sig.unspecified_params) return false;
  return sig_is_variadic(sig) || sig_has_void_param_list(sig) || sig_param_count(sig) > 0;
}

std::string llvm_fn_type_suffix_str(const hir::Module& mod, const FnPtrSig& sig) {
  if (!sig_has_explicit_prototype(sig)) return "";
  std::ostringstream out;
  out << "(";
  const bool void_param_list = sig_has_void_param_list(sig);
  for (size_t i = 0; i < sig_param_count(sig); ++i) {
    if (void_param_list) break;
    if (i) out << ", ";
    const TypeSpec param_ts = sig_param_type(sig, i);
    out << (amd64_fixed_aggregate_byval(mod, param_ts) ? "ptr" : llvm_ty(param_ts));
  }
  if (sig_is_variadic(sig)) {
    if (sig_param_count(sig) > 0 && !void_param_list) out << ", ";
    out << "...";
  }
  out << ")";
  return out.str();
}

bool sig_has_meaningful_prototype(const FnPtrSig& sig) {
  if (sig_has_explicit_prototype(sig)) return true;
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->is_variadic || !fsig->params.empty();
  }
  return false;
}

std::string llvm_fn_type_suffix_str(const hir::Module& mod, const Function& fn) {
  if (fn.params.empty() && !fn.attrs.variadic) return "";
  std::ostringstream out;
  out << "(";
  const bool void_param_list = fn.params.size() == 1 &&
                               fn.params[0].type.spec.base == TB_VOID &&
                               fn.params[0].type.spec.ptr_level == 0 &&
                               fn.params[0].type.spec.array_rank == 0;
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    if (i) out << ", ";
    const TypeSpec& param_ts = fn.params[i].type.spec;
    out << (amd64_fixed_aggregate_byval(mod, param_ts) ? "ptr" : llvm_ty(param_ts));
  }
  if (fn.attrs.variadic) {
    if (!fn.params.empty() && !void_param_list) out << ", ";
    out << "...";
  }
  out << ")";
  return out.str();
}

int llvm_struct_base_slot(const hir::Module& mod, const HirStructDef& sd,
                          size_t base_index) {
  if (sd.is_union) return 0;
  int cur_offset = 0;
  int slot = 0;
  for (size_t i = 0; i < sd.base_tags.size(); ++i) {
    const auto it = mod.struct_defs.find(sd.base_tags[i]);
    if (it == mod.struct_defs.end()) continue;
    const auto& base = it->second;
    if (base.align_bytes > 0 && cur_offset % base.align_bytes != 0) ++slot;
    cur_offset = align_to_bytes(cur_offset, std::max(1, base.align_bytes));
    if (i == base_index) return slot;
    cur_offset += std::max(0, base.size_bytes);
    ++slot;
  }
  return 0;
}

int llvm_struct_field_slot(const hir::Module& mod, const HirStructDef& sd,
                           int target_llvm_idx) {
  if (sd.is_union) return 0;
  int last_idx = -1;
  int cur_offset = 0;
  int slot = 0;
  for (const auto& base_tag : sd.base_tags) {
    const auto it = mod.struct_defs.find(base_tag);
    if (it == mod.struct_defs.end()) continue;
    const auto& base = it->second;
    if (base.align_bytes > 0 && cur_offset % base.align_bytes != 0) ++slot;
    cur_offset = align_to_bytes(cur_offset, std::max(1, base.align_bytes));
    cur_offset += std::max(0, base.size_bytes);
    ++slot;
  }
  for (const auto& f : sd.fields) {
    if (f.llvm_idx == last_idx) continue;
    last_idx = f.llvm_idx;
    if (f.offset_bytes > cur_offset) ++slot;
    if (f.llvm_idx == target_llvm_idx) return slot;
    cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
    ++slot;
  }
  return 0;
}

int llvm_struct_field_slot(const HirStructDef& sd, int target_llvm_idx) {
  if (sd.is_union) return 0;
  int last_idx = -1;
  int cur_offset = 0;
  int slot = 0;
  for (const auto& f : sd.fields) {
    if (f.llvm_idx == last_idx) continue;
    last_idx = f.llvm_idx;
    if (f.offset_bytes > cur_offset) ++slot;
    if (f.llvm_idx == target_llvm_idx) return slot;
    cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
    ++slot;
  }
  return 0;
}

int llvm_struct_field_slot_by_name(const HirStructDef& sd, const std::string& field_name) {
  for (const auto& f : sd.fields) {
    if (f.name == field_name) return llvm_struct_field_slot(sd, f.llvm_idx);
  }
  return 0;
}

static TypeSpec field_decl_ts(const HirStructField& f) {
  TypeSpec ts = f.elem_type;
  if (f.array_first_dim >= 0) {
    for (int i = std::min(ts.array_rank, 7); i > 0; --i) ts.array_dims[i] = ts.array_dims[i - 1];
    ts.array_rank = std::min(ts.array_rank + 1, 8);
    ts.array_size = f.array_first_dim;
    ts.array_dims[0] = f.array_first_dim;
  }
  return ts;
}

static bool collect_aarch64_hfa_elements(const Module& mod, const TypeSpec& ts,
                                         TypeBase* elem_base, int* elem_count) {
  if (ts.ptr_level != 0) return false;
  if (ts.array_rank > 0) {
    if (ts.array_size <= 0) return false;
    TypeSpec elem = ts;
    elem.array_rank--;
    for (int i = 0; i < 8; ++i) elem.array_dims[i] = (i + 1 < 8) ? ts.array_dims[i + 1] : -1;
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    for (int i = 0; i < ts.array_size; ++i) {
      if (!collect_aarch64_hfa_elements(mod, elem, elem_base, elem_count)) return false;
    }
    return true;
  }
  if (ts.base == TB_FLOAT || ts.base == TB_DOUBLE) {
    if (*elem_count == 0) {
      *elem_base = ts.base;
    } else if (*elem_base != ts.base) {
      return false;
    }
    ++*elem_count;
    return *elem_count <= 4;
  }
  if (ts.base != TB_STRUCT || !ts.tag || !ts.tag[0]) return false;
  const auto it = mod.struct_defs.find(ts.tag);
  if (it == mod.struct_defs.end()) return false;
  const auto& sd = it->second;
  if (sd.is_union || sd.fields.empty()) return false;
  for (const auto& field : sd.fields) {
    if (field.bit_width >= 0 || field.is_flexible_array) return false;
    if (!collect_aarch64_hfa_elements(mod, field_decl_ts(field), elem_base, elem_count)) {
      return false;
    }
  }
  return true;
}

std::optional<Aarch64HomogeneousFpAggregateInfo> classify_aarch64_hfa(
    const Module& mod, const TypeSpec& ts) {
  if (ts.base != TB_STRUCT || ts.ptr_level != 0 || ts.array_rank != 0 || !ts.tag ||
      !ts.tag[0]) {
    return std::nullopt;
  }
  const auto it = mod.struct_defs.find(ts.tag);
  if (it == mod.struct_defs.end()) return std::nullopt;

  TypeBase elem_base = TB_VOID;
  int elem_count = 0;
  if (!collect_aarch64_hfa_elements(mod, ts, &elem_base, &elem_count) || elem_count < 1) {
    return std::nullopt;
  }

  Aarch64HomogeneousFpAggregateInfo info;
  TypeSpec elem_ts{};
  elem_ts.base = elem_base;
  info.elem_ty = llvm_ty(elem_ts);
  info.elem_count = elem_count;
  info.elem_size = (elem_base == TB_FLOAT) ? 4 : 8;
  info.aggregate_size = std::max(1, it->second.size_bytes);
  info.aggregate_align = std::max(1, object_align_bytes(mod, ts));
  return info;
}

int round_up_to(int value, int align) {
  if (align <= 1) return value;
  return ((value + align - 1) / align) * align;
}

int object_align_bytes(const Module& mod, const LirModule* lir_module,
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
  } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    const StructuredLayoutLookup layout =
        lookup_structured_layout(mod, lir_module, ts, "stmt-object-align");
    align = layout.legacy_decl ? std::max(1, layout.legacy_decl->align_bytes) : 8;
  } else if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
    align = llvm_va_list_alignment(mod.target_profile);
  } else {
    switch (ts.base) {
      case TB_BOOL:
      case TB_CHAR:
      case TB_SCHAR:
      case TB_UCHAR: align = 1; break;
      case TB_SHORT:
      case TB_USHORT: align = 2; break;
      case TB_INT:
      case TB_UINT:
      case TB_FLOAT:
      case TB_ENUM: align = 4; break;
      case TB_LONG:
      case TB_ULONG:
      case TB_LONGLONG:
      case TB_ULONGLONG:
      case TB_DOUBLE: align = 8; break;
      case TB_LONGDOUBLE:
      case TB_INT128:
      case TB_UINT128: align = 16; break;
      default: align = 8; break;
    }
  }
  if (ts.align_bytes > align) align = ts.align_bytes;
  return align;
}

int object_align_bytes(const Module& mod, const TypeSpec& ts) {
  return object_align_bytes(mod, nullptr, ts);
}

}  // namespace stmt_emitter_detail

// Draft-only staging file for Step 3 of the stmt_emitter split refactor.
// The monolith remains the live implementation until Step 4 build wiring.

StmtEmitter::StmtEmitter(const Module& m) : mod_(m) {}

void StmtEmitter::set_module(lir::LirModule& module) { module_ = &module; }

void StmtEmitter::emit_lir_op(FnCtx& ctx, lir::LirInst op) {
  // Skip dead code after a terminator has been placed in this block.
  if (!std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator)) return;
  ctx.cur_block().insts.push_back(std::move(op));
  ctx.last_term = false;
}

const GlobalVar* StmtEmitter::select_global_object(const std::string& name) const {
  const GlobalVar* best = nullptr;
  for (const auto& g : mod_.globals) {
    if (g.name != name) continue;
    if (!best) {
      best = &g;
      continue;
    }
    const TypeSpec& best_ts = best->type.spec;
    const TypeSpec& cand_ts = g.type.spec;
    if (cand_ts.array_rank > 0 && best_ts.array_rank <= 0) {
      best = &g;
    } else if (cand_ts.array_rank > 0 && best_ts.array_rank > 0 &&
               cand_ts.array_size > best_ts.array_size) {
      best = &g;
    } else if (cand_ts.array_rank <= 0 && best_ts.array_rank <= 0 && g.init.index() != 0 &&
               best->init.index() == 0) {
      best = &g;
    }
  }
  return best;
}

const GlobalVar* StmtEmitter::select_global_object(GlobalId id) const {
  const GlobalVar* gv = mod_.find_global(id);
  if (!gv) return nullptr;
  if (const GlobalVar* best = select_global_object(gv->name)) return best;
  return gv;
}

const GlobalVar* StmtEmitter::select_global_object(const DeclRef& ref) const {
  if (ref.global) return select_global_object(*ref.global);
  if (const GlobalVar* gv = mod_.find_global(ref.link_name_id)) return gv;
  return select_global_object(ref.name);
}

void StmtEmitter::emit_lbl(FnCtx& ctx, const std::string& lbl) { open_lbl(ctx, lbl); }

void StmtEmitter::emit_term_br(FnCtx& ctx, const std::string& target_label) {
  (void)set_terminator_if_open(ctx, lir::LirBr{target_label});
}

void StmtEmitter::emit_term_condbr(FnCtx& ctx, const std::string& cond,
                                   const std::string& true_label,
                                   const std::string& false_label) {
  (void)set_terminator_if_open(ctx, lir::LirCondBr{cond, true_label, false_label});
}

void StmtEmitter::emit_term_ret(FnCtx& ctx, const std::string& type_str,
                                const std::optional<std::string>& value_str) {
  (void)set_terminator_if_open(ctx, lir::LirRet{value_str, type_str});
}

void StmtEmitter::emit_term_switch(
    FnCtx& ctx, const std::string& sel_name, const std::string& sel_type,
    const std::string& default_label, std::vector<std::pair<long long, std::string>> cases) {
  (void)set_terminator_if_open(
      ctx, lir::LirSwitch{sel_name, sel_type, default_label, std::move(cases)});
}

void StmtEmitter::emit_term_unreachable(FnCtx& ctx) {
  if (std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator)) {
    ctx.last_term = true;
  }
}

void StmtEmitter::emit_br_and_open_lbl(FnCtx& ctx, const std::string& branch_label,
                                       const std::string& open_label) {
  emit_term_br(ctx, branch_label);
  emit_lbl(ctx, open_label);
}

void StmtEmitter::emit_fallthrough_lbl(FnCtx& ctx, const std::string& lbl) {
  emit_br_and_open_lbl(ctx, lbl, lbl);
}

std::string StmtEmitter::fresh_tmp(FnCtx& ctx) { return "%t" + std::to_string(ctx.tmp_idx++); }

void StmtEmitter::record_extern_call_decl(const std::string& name, const std::string& ret_ty,
                                          LinkNameId link_name_id) {
  if (name.empty() || mod_.fn_index.count(name)) return;
  module_->record_extern_decl(name, ret_ty, link_name_id);
}

std::string StmtEmitter::fresh_lbl(FnCtx& ctx, const std::string& pfx) {
  return pfx + std::to_string(ctx.tmp_idx++);
}

std::string StmtEmitter::block_lbl(BlockId id) { return "block_" + std::to_string(id.value); }

std::string StmtEmitter::intern_str(const std::string& raw_bytes) {
  return module_->intern_str(raw_bytes);
}

bool StmtEmitter::is_char_like(TypeBase b) {
  return b == TB_CHAR || b == TB_SCHAR || b == TB_UCHAR;
}

std::string StmtEmitter::bytes_from_string_literal(const StringLiteral& sl) {
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' && bytes.back() == '"') {
    bytes = bytes.substr(2, bytes.size() - 3);
  }
  return decode_c_escaped_bytes(bytes);
}

std::vector<long long> StmtEmitter::decode_wide_string_values(const std::string& raw) {
  std::vector<long long> out;
  const char* p = raw.c_str();
  while (*p && *p != '"') ++p;
  if (*p == '"') ++p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1)) {
      ++p;
      switch (*p) {
        case 'n':
          out.push_back('\n');
          ++p;
          break;
        case 't':
          out.push_back('\t');
          ++p;
          break;
        case 'r':
          out.push_back('\r');
          ++p;
          break;
        case '0':
          out.push_back(0);
          ++p;
          break;
        case '\\':
          out.push_back('\\');
          ++p;
          break;
        case '\'':
          out.push_back('\'');
          ++p;
          break;
        case '"':
          out.push_back('"');
          ++p;
          break;
        case 'a':
          out.push_back('\a');
          ++p;
          break;
        case 'x': {
          ++p;
          long long v = 0;
          while (isxdigit((unsigned char)*p)) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0'
                                                     : tolower((unsigned char)*p) - 'a' + 10);
            ++p;
          }
          out.push_back(v);
          break;
        }
        default:
          if (*p >= '0' && *p <= '7') {
            long long v = 0;
            for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; ++k, ++p) v = v * 8 + (*p - '0');
            out.push_back(v);
          } else {
            out.push_back(static_cast<unsigned char>(*p));
            ++p;
          }
          break;
      }
      continue;
    }
    const unsigned char c0 = static_cast<unsigned char>(*p);
    if (c0 < 0x80) {
      out.push_back(c0);
      ++p;
    } else if ((c0 & 0xE0) == 0xC0 && p[1]) {
      out.push_back(((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F));
      p += 2;
    } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
      out.push_back(((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(p[2]) & 0x3F));
      p += 3;
    } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      out.push_back(((c0 & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                    ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(p[3]) & 0x3F));
      p += 4;
    } else {
      out.push_back(c0);
      ++p;
    }
  }
  out.push_back(0);
  return out;
}

std::string StmtEmitter::escape_llvm_c_bytes(const std::string& raw_bytes) {
  std::string esc;
  for (unsigned char c : raw_bytes) {
    if (c == '"') {
      esc += "\\22";
    } else if (c == '\\') {
      esc += "\\5C";
    } else if (c == '\n') {
      esc += "\\0A";
    } else if (c == '\r') {
      esc += "\\0D";
    } else if (c == '\t') {
      esc += "\\09";
    } else if (c < 32 || c >= 127) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\%02X", c);
      esc += buf;
    } else {
      esc += static_cast<char>(c);
    }
  }
  return esc;
}

TypeSpec StmtEmitter::field_decl_type(const HirStructField& f) const {
  TypeSpec ts = f.elem_type;
  if (f.array_first_dim >= 0) {
    const int trailing_rank = std::max(0, std::min(ts.array_rank, 7));
    for (int i = trailing_rank; i > 0; --i) ts.array_dims[i] = ts.array_dims[i - 1];
    for (int i = trailing_rank + 1; i < 8; ++i) ts.array_dims[i] = -1;
    ts.array_rank = trailing_rank + 1;
    ts.array_size = f.array_first_dim;
    ts.array_dims[0] = f.array_first_dim;
    ts.is_ptr_to_array = false;
    ts.inner_rank = -1;
  }
  return ts;
}

std::string StmtEmitter::emit_const_int_like(long long value, const TypeSpec& expected_ts) {
  if (llvm_ty(expected_ts) == "ptr") {
    if (value == 0) return "null";
    return "inttoptr (i64 " + std::to_string(value) + " to ptr)";
  }
  if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 &&
      expected_ts.array_rank == 0) {
    return fp_literal(expected_ts.base, static_cast<double>(value));
  }
  return std::to_string(value);
}

const Expr& StmtEmitter::get_expr(ExprId id) const {
  for (const auto& e : mod_.expr_pool)
    if (e.id.value == id.value) return e;
  throw std::runtime_error("StmtEmitter: expr not found id=" + std::to_string(id.value));
}

std::string StmtEmitter::coerce(FnCtx& ctx, const std::string& val, const TypeSpec& from_ts,
                                const TypeSpec& to_ts) {
  const std::string ft = llvm_ty(from_ts);
  const std::string tt = llvm_ty(to_ts);
  const TypeBase from_scalar_base = llvm_storage_base(from_ts);
  const TypeBase to_scalar_base = llvm_storage_base(to_ts);
  if (tt == "ptr" && val == "0") return "null";
  if (ft == tt) return val;
  if (ft == "ptr" && tt == "ptr") return val;

  if (((is_vector_value(from_ts) && is_any_int(to_ts.base) && to_ts.ptr_level == 0 &&
        to_ts.array_rank == 0) ||
       (is_any_int(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        is_vector_value(to_ts))) &&
      sizeof_ts(mod_, from_ts) == sizeof_ts(mod_, to_ts)) {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::Bitcast, ft, val, tt});
    return tmp;
  }

  if (is_complex_base(from_ts.base) && is_complex_base(to_ts.base)) {
    const TypeSpec from_elem_ts = complex_component_ts(from_ts.base);
    const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
    const std::string real0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{real0, ft, val, 0});
    const std::string imag0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{imag0, ft, val, 1});
    const std::string real1 = coerce(ctx, real0, from_elem_ts, to_elem_ts);
    const std::string imag1 = coerce(ctx, imag0, from_elem_ts, to_elem_ts);
    const std::string with_real = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirInsertValueOp{with_real, tt, "undef", llvm_ty(to_elem_ts), real1, 0});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{out, tt, with_real, llvm_ty(to_elem_ts), imag1, 1});
    return out;
  }

  if (!is_complex_base(from_ts.base) && is_complex_base(to_ts.base) && from_ts.ptr_level == 0 &&
      from_ts.array_rank == 0) {
    const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
    const std::string real = coerce(ctx, val, from_ts, to_elem_ts);
    const std::string elem_ty = llvm_ty(to_elem_ts);
    const std::string zero = is_float_base(to_elem_ts.base) ? fp_literal(to_elem_ts.base, 0.0)
                                                            : "0";
    const std::string with_real = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{with_real, tt, "undef", elem_ty, real, 0});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{out, tt, with_real, elem_ty, zero, 1});
    return out;
  }

  if (ft == "i1" && tt != "ptr" && tt != "void") {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt, "i1", val, tt});
    return tmp;
  }

  if (from_ts.ptr_level == 0 && from_ts.array_rank == 0 && to_ts.ptr_level == 0 &&
      to_ts.array_rank == 0 && is_any_int(from_ts.base) && is_any_int(to_ts.base)) {
    const int fb = int_bits(from_scalar_base);
    const int tb = int_bits(to_scalar_base);
    if (fb == tb) return val;
    const std::string tmp = fresh_tmp(ctx);
    if (tb > fb) {
      const auto kind =
          is_signed_int(from_scalar_base) ? lir::LirCastKind::SExt
                                          : lir::LirCastKind::ZExt;
      emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
    } else {
      emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::Trunc, ft, val, tt});
    }
    return tmp;
  }

  if (is_float_base(from_ts.base) && is_float_base(to_ts.base)) {
    const int fb = (from_ts.base == TB_FLOAT) ? 32 : (from_ts.base == TB_LONGDOUBLE ? 128 : 64);
    const int tb = (to_ts.base == TB_FLOAT) ? 32 : (to_ts.base == TB_LONGDOUBLE ? 128 : 64);
    if (fb == tb) return val;
    const std::string tmp = fresh_tmp(ctx);
    const auto kind = (tb > fb) ? lir::LirCastKind::FPExt : lir::LirCastKind::FPTrunc;
    emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
    return tmp;
  }

  if (is_any_int(from_ts.base) && from_ts.ptr_level == 0 && is_float_base(to_ts.base) &&
      to_ts.ptr_level == 0 && to_ts.array_rank == 0) {
    const std::string tmp = fresh_tmp(ctx);
    const auto kind =
        is_signed_int(from_scalar_base) ? lir::LirCastKind::SIToFP
                                        : lir::LirCastKind::UIToFP;
    emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
    return tmp;
  }

  if (is_float_base(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
      is_any_int(to_ts.base) && to_ts.ptr_level == 0) {
    const std::string tmp = fresh_tmp(ctx);
    const auto kind =
        is_signed_int(to_scalar_base) ? lir::LirCastKind::FPToSI
                                      : lir::LirCastKind::FPToUI;
    emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
    return tmp;
  }

  if (ft == "ptr" && (from_ts.is_lvalue_ref || from_ts.is_rvalue_ref) && to_ts.ptr_level == 0 &&
      !to_ts.is_lvalue_ref && !to_ts.is_rvalue_ref && !to_ts.is_fn_ptr) {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{tmp, tt, val});
    return tmp;
  }

  if (ft == "ptr" && is_any_int(to_ts.base)) {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::PtrToInt, "ptr", val, tt});
    return tmp;
  }
  if (is_any_int(from_ts.base) && tt == "ptr") {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::IntToPtr, ft, val, "ptr"});
    return tmp;
  }

  return val;
}

std::string StmtEmitter::to_bool(FnCtx& ctx, const std::string& val, const TypeSpec& ts) {
  const std::string ty = llvm_ty(ts);
  if (ty == "i1") return val;
  const std::string tmp = fresh_tmp(ctx);
  if (ty == "ptr") {
    emit_lir_op(ctx, lir::LirCmpOp{tmp, false, "ne", "ptr", val, "null"});
  } else if (is_float_base(ts.base) && ts.ptr_level == 0 && ts.array_rank == 0) {
    emit_lir_op(ctx, lir::LirCmpOp{tmp, true, "une", ty, val, fp_literal(ts.base, 0.0)});
  } else {
    emit_lir_op(ctx, lir::LirCmpOp{tmp, false, "ne", ty, val, "0"});
  }
  return tmp;
}

}  // namespace c4c::codegen::lir
