#include "stmt_emitter.hpp"
#include "canonical_symbol.hpp"
#include "../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;

namespace {

constexpr int kAmd64GpAreaBytes = 48;
constexpr int kAmd64FpAreaBytes = 176;

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

template <typename TerminatorT>
bool set_terminator_if_open(FnCtx& ctx, TerminatorT&& terminator) {
  if (!std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator)) {
    return false;
  }
  ctx.cur_block().terminator = std::forward<TerminatorT>(terminator);
  ctx.last_term = true;
  return true;
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

// ── FnPtrSig accessors ───────────────────────────────────────────────────
// These helpers extract return type and parameter types from FnPtrSig.
// canonical_sig is always populated during HIR lowering.

/// Extract the return TypeSpec from a FnPtrSig.
TypeSpec sig_return_type(const FnPtrSig& sig) {
  return sig.return_type.spec;
}

/// Extract the i-th parameter TypeSpec from a FnPtrSig via canonical_sig.
TypeSpec sig_param_type(const FnPtrSig& sig, size_t i) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig && i < fsig->params.size()) {
      return sema::typespec_from_canonical(fsig->params[i]);
    }
  }
  return i < sig.params.size() ? sig.params[i].spec : TypeSpec{};
}

/// Check if the i-th parameter is va_list by value.
bool sig_param_is_va_list_value(const FnPtrSig& sig, size_t i) {
  const TypeSpec ts = sig_param_type(sig, i);
  return ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0;
}

/// Return the number of declared parameters.
size_t sig_param_count(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->params.size();
  }
  return sig.params.size();
}

/// Check if sig is variadic.
bool sig_is_variadic(const FnPtrSig& sig) {
  if (sig.canonical_sig) {
    const auto* fsig = sema::get_function_sig(*sig.canonical_sig);
    if (fsig) return fsig->is_variadic;
  }
  return sig.variadic;
}

/// Check if sig has a void parameter list (i.e. f(void)).
bool sig_has_void_param_list(const FnPtrSig& sig) {
  if (sig_param_count(sig) != 1) return false;
  const TypeSpec ts = sig_param_type(sig, 0);
  return ts.base == TB_VOID && ts.ptr_level == 0 && ts.array_rank == 0;
}

bool sig_has_explicit_prototype(const FnPtrSig& sig) {
  if (sig.unspecified_params) return false;
  return sig_is_variadic(sig) || sig_has_void_param_list(sig) || sig_param_count(sig) > 0;
}

std::string llvm_fn_type_suffix_str(const FnPtrSig& sig) {
  if (!sig_has_explicit_prototype(sig)) return "";
  std::ostringstream out;
  out << "(";
  const bool void_param_list = sig_has_void_param_list(sig);
  for (size_t i = 0; i < sig_param_count(sig); ++i) {
    if (void_param_list) break;
    if (i) out << ", ";
    out << llvm_ty(sig_param_type(sig, i));
  }
  if (sig_is_variadic(sig)) {
    if (sig_param_count(sig) > 0 && !void_param_list) out << ", ";
    out << "...";
  }
  out << ")";
  return out.str();
}

std::string llvm_fn_type_suffix_str(const Function& fn) {
  if (fn.params.empty() && !fn.attrs.variadic) return "";
  std::ostringstream out;
  out << "(";
  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    if (i) out << ", ";
    out << llvm_ty(fn.params[i].type.spec);
  }
  if (fn.attrs.variadic) {
    if (!fn.params.empty() && !void_param_list) out << ", ";
    out << "...";
  }
  out << ")";
  return out.str();
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

int object_align_bytes(const Module& mod, const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    int align = object_align_bytes(mod, elem);
    if (ts.align_bytes > align) align = ts.align_bytes;
    return align;
  }
  int align = 1;
  if (ts.is_vector && ts.vector_bytes > 0) {
    align = static_cast<int>(ts.vector_bytes);
  } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    align = 8;
  } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    const auto it = mod.struct_defs.find(ts.tag);
    align = (it != mod.struct_defs.end()) ? std::max(1, it->second.align_bytes) : 8;
  } else if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
    align = llvm_va_list_alignment(mod.target_triple);
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

}  // namespace

StmtEmitter::StmtEmitter(const Module& m) : mod_(m){}

void StmtEmitter::set_module(lir::LirModule& module) {
    module_ = &module;
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
      } else if (cand_ts.array_rank <= 0 && best_ts.array_rank <= 0 &&
                 g.init.index() != 0 && best->init.index() == 0) {
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



void StmtEmitter::emit_lbl(FnCtx& ctx, const std::string& lbl){
    open_lbl(ctx, lbl);
  }

void StmtEmitter::emit_term_br(FnCtx& ctx, const std::string& target_label){
    (void)set_terminator_if_open(ctx, lir::LirBr{target_label});
  }

void StmtEmitter::emit_term_condbr(FnCtx& ctx, const std::string& cond,
                                    const std::string& true_label,
                                    const std::string& false_label){
    (void)set_terminator_if_open(ctx, lir::LirCondBr{cond, true_label, false_label});
  }

void StmtEmitter::emit_term_ret(FnCtx& ctx, const std::string& type_str,
                                 const std::optional<std::string>& value_str){
    (void)set_terminator_if_open(ctx, lir::LirRet{value_str, type_str});
  }

void StmtEmitter::emit_term_switch(FnCtx& ctx, const std::string& sel_name,
                                    const std::string& sel_type,
                                    const std::string& default_label,
                                    std::vector<std::pair<long long, std::string>> cases){
    (void)set_terminator_if_open(
        ctx, lir::LirSwitch{sel_name, sel_type, default_label, std::move(cases)});
  }

void StmtEmitter::emit_term_unreachable(FnCtx& ctx){
    // LirUnreachable is the default — but we need to mark last_term.
    // Only set if no real terminator has been placed.
    if (std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator)) {
      ctx.last_term = true;
    }
  }

void StmtEmitter::emit_br_and_open_lbl(FnCtx& ctx, const std::string& branch_label,
                                        const std::string& open_label){
    emit_term_br(ctx, branch_label);
    emit_lbl(ctx, open_label);
  }

void StmtEmitter::emit_fallthrough_lbl(FnCtx& ctx, const std::string& lbl){
    emit_br_and_open_lbl(ctx, lbl, lbl);
  }

std::string StmtEmitter::fresh_tmp(FnCtx& ctx){
    return "%t" + std::to_string(ctx.tmp_idx++);
  }

void StmtEmitter::record_extern_call_decl(const std::string& name, const std::string& ret_ty){
    if (name.empty() || mod_.fn_index.count(name)) return;
    module_->record_extern_decl(name, ret_ty);
  }

std::string StmtEmitter::fresh_lbl(FnCtx& ctx, const std::string& pfx){
    return pfx + std::to_string(ctx.tmp_idx++);
  }

std::string StmtEmitter::block_lbl(BlockId id){
    return "block_" + std::to_string(id.value);
  }

std::string StmtEmitter::intern_str(const std::string& raw_bytes){
    return module_->intern_str(raw_bytes);
  }

// NOTE: emit_preamble() has been replaced by lir::build_type_decls() in hir_to_lir.cpp.
// Type declaration generation is now part of module-level orchestration owned by hir_to_lir.

bool StmtEmitter::is_char_like(TypeBase b){
    return b == TB_CHAR || b == TB_SCHAR || b == TB_UCHAR;
  }

TypeSpec StmtEmitter::drop_one_array_dim(TypeSpec ts){
    if (ts.array_rank <= 0) return ts;
    for (int i = 0; i < ts.array_rank - 1; ++i) ts.array_dims[i] = ts.array_dims[i + 1];
    ts.array_dims[ts.array_rank - 1] = -1;
    ts.array_rank--;
    ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
    return ts;
  }

TypeSpec StmtEmitter::drop_one_indexed_element_type(TypeSpec ts){
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

TypeSpec StmtEmitter::resolve_indexed_gep_pointee_type(TypeSpec ts){
    if (ts.ptr_level > 0 || outer_array_rank(ts) > 0) {
      return drop_one_indexed_element_type(ts);
    }
    ts = {};
    ts.base = TB_CHAR;
    return ts;
  }

std::string StmtEmitter::bytes_from_string_literal(const StringLiteral& sl){
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' &&
               bytes.back() == '"') {
      bytes = bytes.substr(2, bytes.size() - 3);
    }
    return decode_c_escaped_bytes(bytes);
  }

std::vector<long long> StmtEmitter::decode_wide_string_values(const std::string& raw){
    std::vector<long long> out;
    // Strip L"..." wrapper
    const char* p = raw.c_str();
    while (*p && *p != '"') ++p;
    if (*p == '"') ++p;
    while (*p && *p != '"') {
      if (*p == '\\' && *(p + 1)) {
        ++p;
        switch (*p) {
          case 'n':  out.push_back('\n'); ++p; break;
          case 't':  out.push_back('\t'); ++p; break;
          case 'r':  out.push_back('\r'); ++p; break;
          case '0':  out.push_back(0); ++p; break;
          case '\\': out.push_back('\\'); ++p; break;
          case '\'': out.push_back('\''); ++p; break;
          case '"':  out.push_back('"'); ++p; break;
          case 'a':  out.push_back('\a'); ++p; break;
          case 'x': {
            ++p;
            long long v = 0;
            while (isxdigit((unsigned char)*p)) {
              v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' : tolower((unsigned char)*p) - 'a' + 10);
              ++p;
            }
            out.push_back(v);
            break;
          }
          default:
            if (*p >= '0' && *p <= '7') {
              long long v = 0;
              for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; ++k, ++p)
                v = v * 8 + (*p - '0');
              out.push_back(v);
            } else {
              out.push_back(static_cast<unsigned char>(*p)); ++p;
            }
            break;
        }
        continue;
      }
      // Decode UTF-8 to Unicode codepoint
      const unsigned char c0 = static_cast<unsigned char>(*p);
      if (c0 < 0x80) {
        out.push_back(c0); ++p;
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
        out.push_back(c0); ++p;
      }
    }
    out.push_back(0); // null terminator
    return out;
  }

std::string StmtEmitter::escape_llvm_c_bytes(const std::string& raw_bytes){
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')      { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\%02X", c);
        esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    return esc;
  }

TypeSpec StmtEmitter::field_decl_type(const HirStructField& f) const{
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

// NOTE: try_const_eval_int/float/complex_int/complex_fp and
// emit_const_scalar_expr have been moved to lir::ConstInitEmitter
// (const_init_emitter.cpp).  They are no longer used from StmtEmitter.

// emit_const_int_like is kept because it is used from emit_rval_payload.

std::string StmtEmitter::emit_const_int_like(long long value, const TypeSpec& expected_ts){
    if (llvm_ty(expected_ts) == "ptr") {
      if (value == 0) return "null";
      return "inttoptr (i64 " + std::to_string(value) + " to ptr)";
    }
    if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
      return fp_literal(expected_ts.base, static_cast<double>(value));
    return std::to_string(value);
  }


const Expr& StmtEmitter::get_expr(ExprId id) const{
    for (const auto& e : mod_.expr_pool)
      if (e.id.value == id.value) return e;
    throw std::runtime_error("StmtEmitter: expr not found id=" + std::to_string(id.value));
  }

std::string StmtEmitter::coerce(FnCtx& ctx, const std::string& val,
                     const TypeSpec& from_ts, const TypeSpec& to_ts){
    const std::string ft = llvm_ty(from_ts);
    const std::string tt = llvm_ty(to_ts);
    if (tt == "ptr" && val == "0") return "null";
    if (ft == tt) return val;
    if (ft == "ptr" && tt == "ptr") return val;

    // Reinterpret between same-sized vector and integer scalars.
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
      emit_lir_op(ctx, lir::LirInsertValueOp{with_real, tt, "undef", llvm_ty(to_elem_ts), real1, 0});
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{out, tt, with_real, llvm_ty(to_elem_ts), imag1, 1});
      return out;
    }

    if (!is_complex_base(from_ts.base) && is_complex_base(to_ts.base) &&
        from_ts.ptr_level == 0 && from_ts.array_rank == 0) {
      const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
      const std::string real = coerce(ctx, val, from_ts, to_elem_ts);
      const std::string elem_ty = llvm_ty(to_elem_ts);
      const std::string zero =
          is_float_base(to_elem_ts.base) ? fp_literal(to_elem_ts.base, 0.0) : "0";
      const std::string with_real = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{with_real, tt, "undef", elem_ty, real, 0});
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{out, tt, with_real, elem_ty, zero, 1});
      return out;
    }

    // i1 → wider int
    if (ft == "i1" && tt != "ptr" && tt != "void") {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt, "i1", val, tt});
      return tmp;
    }

    // int → int
    if (from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        to_ts.ptr_level == 0 && to_ts.array_rank == 0 &&
        is_any_int(from_ts.base) && is_any_int(to_ts.base)) {
      const int fb = int_bits(from_ts.base), tb = int_bits(to_ts.base);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      if (tb > fb) {
        const auto kind = is_signed_int(from_ts.base) ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt;
        emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
      } else {
        emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::Trunc, ft, val, tt});
      }
      return tmp;
    }

    // float → float
    if (is_float_base(from_ts.base) && is_float_base(to_ts.base)) {
      const int fb = (from_ts.base == TB_FLOAT) ? 32 : (from_ts.base == TB_LONGDOUBLE ? 128 : 64);
      const int tb = (to_ts.base == TB_FLOAT) ? 32 : (to_ts.base == TB_LONGDOUBLE ? 128 : 64);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      const auto kind = (tb > fb) ? lir::LirCastKind::FPExt : lir::LirCastKind::FPTrunc;
      emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
      return tmp;
    }

    // int → float (only for non-pointer scalar float types)
    if (is_any_int(from_ts.base) && from_ts.ptr_level == 0 &&
        is_float_base(to_ts.base) && to_ts.ptr_level == 0 && to_ts.array_rank == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const auto kind = is_signed_int(from_ts.base) ? lir::LirCastKind::SIToFP : lir::LirCastKind::UIToFP;
      emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
      return tmp;
    }

    // float → int (only for non-pointer scalar types)
    if (is_float_base(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        is_any_int(to_ts.base) && to_ts.ptr_level == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const auto kind = is_signed_int(to_ts.base) ? lir::LirCastKind::FPToSI : lir::LirCastKind::FPToUI;
      emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
      return tmp;
    }

    // Reference dereference: ptr (reference) → value type by loading
    if (ft == "ptr" && (from_ts.is_lvalue_ref || from_ts.is_rvalue_ref) &&
        to_ts.ptr_level == 0 && !to_ts.is_lvalue_ref && !to_ts.is_rvalue_ref &&
        !to_ts.is_fn_ptr) {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, tt, val});
      return tmp;
    }

    // ptr ↔ int
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

    // Same size numeric: bitcast
    return val;
  }

std::string StmtEmitter::to_bool(FnCtx& ctx, const std::string& val, const TypeSpec& ts){
    const std::string ty = llvm_ty(ts);
    if (ty == "i1") return val;
    const std::string tmp = fresh_tmp(ctx);
    // Check ptr FIRST: a TypeSpec with ptr_level>0 always uses icmp even if the
    // base is a float type (e.g. function pointer typed as float-return + ptr_level=1).
    if (ty == "ptr") {
      emit_lir_op(ctx, lir::LirCmpOp{tmp, false, "ne", "ptr", val, "null"});
    } else if (is_float_base(ts.base) && ts.ptr_level == 0 && ts.array_rank == 0) {
      emit_lir_op(ctx, lir::LirCmpOp{tmp, true, "une", ty, val, fp_literal(ts.base, 0.0)});
    } else {
      emit_lir_op(ctx, lir::LirCmpOp{tmp, false, "ne", ty, val, "0"});
    }
    return tmp;
  }

bool StmtEmitter::find_field_chain(const std::string& tag, const std::string& field_name,
                        std::vector<FieldStep>& chain, TypeSpec& out_field_ts){
    const auto it = mod_.struct_defs.find(tag);
    if (it == mod_.struct_defs.end()) return false;
    const HirStructDef& sd = it->second;

    // Direct lookup
    for (const auto& f : sd.fields) {
      if (f.name == field_name) {
        FieldStep step{tag, llvm_struct_field_slot(sd, f.llvm_idx), sd.is_union};
        if (f.bit_width >= 0) {
          step.bit_width = f.bit_width;
          step.bit_offset = f.bit_offset;
          step.storage_unit_bits = f.storage_unit_bits;
          step.bf_is_signed = f.is_bf_signed;
        }
        chain.push_back(std::move(step));
        out_field_ts = field_decl_type(f);  // includes array dims if array field
        return true;
      }
    }
    // Recursive: search anonymous embedded members
    for (const auto& f : sd.fields) {
      if (!f.is_anon_member) continue;
      if (!f.elem_type.tag || !f.elem_type.tag[0]) continue;
      std::vector<FieldStep> sub_chain;
      TypeSpec sub_ts{};
      if (find_field_chain(f.elem_type.tag, field_name, sub_chain, sub_ts)) {
        // prepend: current struct's step to reach the anon member
        chain.push_back({tag, llvm_struct_field_slot(sd, f.llvm_idx), sd.is_union});
        for (const auto& s : sub_chain) chain.push_back(s);
        out_field_ts = sub_ts;
        return true;
      }
    }
    return false;
  }

bool StmtEmitter::resolve_field_access(const std::string& tag, const std::string& field_name,
                                       std::vector<FieldStep>& chain, TypeSpec& out_field_ts,
                                       BitfieldAccess* out_bf){
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

std::string StmtEmitter::emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                                         const std::vector<FieldStep>& chain){
    std::string cur_ptr = base_ptr;
    for (const auto& step : chain) {
      const std::string sty = llvm_struct_type_str(step.tag);
      if (step.is_union) {
        // Union: GEP to field 0 (byte array); with opaque ptrs same addr
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, sty, cur_ptr, false, {"i32 0", "i32 0"}});
        cur_ptr = tmp;
      } else {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, sty, cur_ptr, false, {"i32 0", "i32 " + std::to_string(step.llvm_idx)}});
        cur_ptr = tmp;
      }
    }
    return cur_ptr;
  }

int StmtEmitter::bitfield_promoted_bits(const BitfieldAccess& bf){
    if (bf.bit_width <= 31) return 32;  // fits in int
    if (bf.bit_width == 32) return 32;  // fits in int (signed) or uint (unsigned)
    return bf.storage_unit_bits;        // >32 bits: use storage type (i64)
  }

TypeBase StmtEmitter::bitfield_promoted_base(int bit_width, bool is_signed, int storage_unit_bits){
    if (bit_width < 32) return TB_INT;
    if (bit_width == 32) return is_signed ? TB_INT : TB_UINT;
    if (storage_unit_bits > 32) return is_signed ? TB_LONGLONG : TB_ULONGLONG;
    return is_signed ? TB_INT : TB_UINT;
  }

TypeSpec StmtEmitter::bitfield_promoted_ts(const BitfieldAccess& bf){
    TypeSpec ts{};
    ts.base = bitfield_promoted_base(bf.bit_width, bf.is_signed, bf.storage_unit_bits);
    return ts;
  }

std::string StmtEmitter::emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                  const BitfieldAccess& bf){
    const std::string result = fresh_tmp(ctx);
    const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);
    const int promoted_bits = bitfield_promoted_bits(bf);
    const std::string promoted_ty = "i" + std::to_string(promoted_bits);

    // 1. Load the full storage unit
    const std::string unit = result + ".bf.unit";
    emit_lir_op(ctx, lir::LirLoadOp{unit, unit_ty, unit_ptr});

    // 2. Shift right to align bitfield to bit 0
    std::string shifted = unit;
    if (bf.bit_offset > 0) {
      shifted = result + ".bf.shr";
      emit_lir_op(ctx, lir::LirBinOp{shifted, "lshr", unit_ty, unit, std::to_string(bf.bit_offset)});
    }

    // 3. Mask to bit_width bits
    const unsigned long long mask = (bf.bit_width >= 64)
        ? ~0ULL : ((1ULL << bf.bit_width) - 1);
    const std::string masked = result + ".bf.mask";
    emit_lir_op(ctx, lir::LirBinOp{masked, "and", unit_ty, shifted, std::to_string(mask)});

    std::string cur = masked;

    // 4. Sign-extend if needed
    if (bf.is_signed && bf.bit_width < bf.storage_unit_bits) {
      const int shift_amt = bf.storage_unit_bits - bf.bit_width;
      const std::string shl_tmp = result + ".bf.shl";
      emit_lir_op(ctx, lir::LirBinOp{shl_tmp, "shl", unit_ty, masked, std::to_string(shift_amt)});
      cur = result + ".bf.sext";
      emit_lir_op(ctx, lir::LirBinOp{cur, "ashr", unit_ty, shl_tmp, std::to_string(shift_amt)});
    }

    // 5. Truncate or extend to promoted type
    if (bf.storage_unit_bits != promoted_bits) {
      if (bf.storage_unit_bits > promoted_bits) {
        emit_lir_op(ctx, lir::LirCastOp{result, lir::LirCastKind::Trunc, unit_ty, cur, promoted_ty});
      } else {
        emit_lir_op(ctx, lir::LirCastOp{result,
            bf.is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt,
            unit_ty, cur, promoted_ty});
      }
    } else {
      // No size change — emit identity add to rename
      emit_lir_op(ctx, lir::LirBinOp{result, "add", unit_ty, cur, "0"});
    }

    return result;
  }

void StmtEmitter::emit_bitfield_store(FnCtx& ctx, const std::string& unit_ptr,
                            const BitfieldAccess& bf,
                            const std::string& new_val, const TypeSpec& val_ts){
    // Coerce new_val to storage unit type
    TypeSpec unit_ts{};
    switch (bf.storage_unit_bits) {
      case 8:  unit_ts.base = TB_UCHAR; break;
      case 16: unit_ts.base = TB_USHORT; break;
      case 32: unit_ts.base = TB_UINT; break;
      case 64: unit_ts.base = TB_ULONGLONG; break;
      default: unit_ts.base = TB_UINT; break;
    }
    std::string val_coerced = coerce(ctx, new_val, val_ts, unit_ts);
    const std::string scratch = fresh_tmp(ctx);
    const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);

    // 1. Load current storage unit
    const std::string old_unit = scratch + ".bf.old";
    emit_lir_op(ctx, lir::LirLoadOp{old_unit, unit_ty, unit_ptr});

    // 2. Clear the bitfield bits
    const unsigned long long field_mask_val = (bf.bit_width >= 64)
        ? ~0ULL : ((1ULL << bf.bit_width) - 1);
    const unsigned long long clear_mask = ~(field_mask_val << bf.bit_offset);
    const std::string cleared = scratch + ".bf.clr";
    emit_lir_op(ctx, lir::LirBinOp{cleared, "and", unit_ty, old_unit,
        std::to_string(static_cast<long long>(clear_mask))});

    // 3. Mask new value to bit_width bits
    const std::string new_masked = scratch + ".bf.vm";
    emit_lir_op(ctx, lir::LirBinOp{new_masked, "and", unit_ty, val_coerced,
        std::to_string(field_mask_val)});

    // 4. Shift new value into position
    std::string new_shifted = new_masked;
    if (bf.bit_offset > 0) {
      new_shifted = scratch + ".bf.vs";
      emit_lir_op(ctx, lir::LirBinOp{new_shifted, "shl", unit_ty, new_masked,
          std::to_string(bf.bit_offset)});
    }

    // 5. Combine and store
    const std::string combined = scratch + ".bf.comb";
    emit_lir_op(ctx, lir::LirBinOp{combined, "or", unit_ty, cleared, new_shifted});
    emit_lir_op(ctx, lir::LirStoreOp{unit_ty, combined, unit_ptr});
  }

std::string StmtEmitter::emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts){
    const Expr& e = get_expr(id);
    return emit_lval_dispatch(ctx, e, pointee_ts);
  }

std::string StmtEmitter::emit_va_list_obj_ptr(FnCtx& ctx, ExprId id, TypeSpec& ts){
    const Expr& e = get_expr(id);
    ts = resolve_expr_type(ctx, id);
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const TypeSpec& pts = ctx.fn->params[*r->param_index].type.spec;
        if (pts.base == TB_VA_LIST && pts.ptr_level == 0 && pts.array_rank == 0) {
          const auto spill_it = ctx.param_slots.find(*r->param_index + 0x80000000u);
          if (spill_it != ctx.param_slots.end()) {
            const std::string tmp = fresh_tmp(ctx);
            emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), spill_it->second});
            return tmp;
          }
          const auto it = ctx.param_slots.find(*r->param_index);
          if (it != ctx.param_slots.end()) return it->second;
        }
      }
    }
    return emit_lval(ctx, id, ts);
  }

StmtEmitter::Amd64VaListPtrs StmtEmitter::load_amd64_va_list_ptrs(
    FnCtx& ctx, const std::string& ap_ptr) {
  Amd64VaListPtrs access;
  access.gp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.gp_offset_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 0"}});
  access.fp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.fp_offset_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 1"}});
  access.overflow_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.overflow_ptr_ptr,
                                 "%struct.__va_list_tag_", ap_ptr, false,
                                 {"i32 0", "i32 2"}});
  const std::string reg_save_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_save_ptr_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 3"}});
  access.reg_save_area_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{access.reg_save_area_ptr, "ptr",
                                  reg_save_ptr_ptr});
  return access;
}

std::string StmtEmitter::emit_amd64_va_arg_from_overflow(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const Amd64VaListPtrs& access, int size_bytes) {
  const std::string stack_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"),
                                  access.overflow_ptr_ptr});
  const int stride = ((size_bytes + 7) / 8) * 8;
  const std::string next_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{next_ptr, "i8", stack_ptr, false,
                                 {"i64 " + std::to_string(stride)}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), next_ptr,
                                   access.overflow_ptr_ptr});

  const std::string tmp_addr = fresh_tmp(ctx);
  const int align = object_align_bytes(mod_, res_ts);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});
  module_->need_memcpy = true;
  emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, stack_ptr,
                                    std::to_string(size_bytes), false});
  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

std::string StmtEmitter::emit_amd64_va_arg_from_registers(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const llvm_cc::Amd64VarargInfo& layout,
    const Amd64VaListPtrs& access, const std::string& gp_offset,
    const std::string& fp_offset) {
  const int size_bytes = layout.size_bytes;
  const int align = object_align_bytes(mod_, res_ts);

  std::string gp_offset_i64;
  if (layout.gp_chunks > 0) {
    gp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{gp_offset_i64, lir::LirCastKind::SExt,
                                    "i32", gp_offset, "i64"});
  }
  std::string fp_offset_i64;
  if (layout.sse_slots > 0) {
    fp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{fp_offset_i64, lir::LirCastKind::SExt,
                                    "i32", fp_offset, "i64"});
  }

  std::string gp_base_ptr;
  if (layout.gp_chunks > 0) {
    gp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{gp_base_ptr, "i8",
                                   access.reg_save_area_ptr, false,
                                   {"i64 " + gp_offset_i64}});
    const std::string new_gp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{new_gp, "add", "i32", gp_offset,
                                   std::to_string(layout.gp_chunks * 8)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_gp,
                                     access.gp_offset_ptr});
  }

  std::string fp_base_ptr;
  if (layout.sse_slots > 0) {
    fp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{fp_base_ptr, "i8",
                                   access.reg_save_area_ptr, false,
                                   {"i64 " + fp_offset_i64}});
    const std::string new_fp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{new_fp, "add", "i32", fp_offset,
                                   std::to_string(layout.sse_slots * 16)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_fp,
                                     access.fp_offset_ptr});
  }

  const std::string tmp_addr = fresh_tmp(ctx);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});

  int gp_chunk_index = 0;
  int sse_slot_index = 0;
  std::string active_sse_slot_ptr;
  for (size_t chunk = 0; chunk < layout.classes.size(); ++chunk) {
    if (static_cast<int>(chunk) * 8 >= size_bytes) break;
    const int chunk_offset = static_cast<int>(chunk) * 8;
    const int chunk_size =
        std::min(8, size_bytes - static_cast<int>(chunk) * 8);
    const auto cls = layout.classes[chunk];
    if (cls == llvm_cc::Amd64ArgClass::Memory ||
        cls == llvm_cc::Amd64ArgClass::None) {
      continue;
    }

    std::string src_ptr;
    if (cls == llvm_cc::Amd64ArgClass::Integer) {
      src_ptr = gp_base_ptr;
      if (gp_chunk_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                              src_ptr, "i8", gp_base_ptr, false,
                              {"i64 " + std::to_string(gp_chunk_index * 8)}});
      }
      ++gp_chunk_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSE) {
      src_ptr = fp_base_ptr;
      if (sse_slot_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                              src_ptr, "i8", fp_base_ptr, false,
                              {"i64 " + std::to_string(sse_slot_index * 16)}});
      }
      active_sse_slot_ptr = src_ptr;
      ++sse_slot_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSEUp) {
      if (active_sse_slot_ptr.empty()) {
        active_sse_slot_ptr = fp_base_ptr;
      }
      std::string upper_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{upper_ptr, "i8", active_sse_slot_ptr,
                                     false, {"i64 8"}});
      src_ptr = upper_ptr;
    } else {
      continue;
    }

    std::string dst_ptr = tmp_addr;
    if (chunk_offset > 0) {
      dst_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{dst_ptr, "i8", tmp_addr, false,
                                     {"i64 " + std::to_string(chunk_offset)}});
    }
    module_->need_memcpy = true;
    emit_lir_op(ctx, lir::LirMemcpyOp{dst_ptr, src_ptr,
                                      std::to_string(chunk_size), false});
  }

  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

std::string StmtEmitter::emit_amd64_va_arg(FnCtx& ctx, const TypeSpec& res_ts,
                                           const std::string& res_ty,
                                           const std::string& ap_ptr) {
  const auto layout = llvm_cc::classify_amd64_vararg(res_ts, mod_);
  if (layout.size_bytes <= 0) return "zeroinitializer";
  if (layout.needs_memory) {
    const auto access = load_amd64_va_list_ptrs(ctx, ap_ptr);
    return emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access,
                                           layout.size_bytes);
  }

  const auto access = load_amd64_va_list_ptrs(ctx, ap_ptr);
  std::string gp_offset = "0";
  if (layout.gp_chunks > 0) {
    gp_offset = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{gp_offset, std::string("i32"),
                                    access.gp_offset_ptr});
  }
  std::string fp_offset = "0";
  if (layout.sse_slots > 0) {
    fp_offset = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{fp_offset, std::string("i32"),
                                    access.fp_offset_ptr});
  }

  std::string gp_ok = "true";
  if (layout.gp_chunks > 0) {
    const int gp_limit = kAmd64GpAreaBytes - layout.gp_chunks * 8;
    gp_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{gp_ok, false, "sle", "i32", gp_offset,
                                   std::to_string(gp_limit)});
  }

  std::string fp_ok = "true";
  if (layout.sse_slots > 0) {
    const int fp_limit = kAmd64FpAreaBytes - layout.sse_slots * 16;
    fp_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{fp_ok, false, "sle", "i32", fp_offset,
                                   std::to_string(fp_limit)});
  }

  std::string regs_ok;
  if (layout.gp_chunks > 0 && layout.sse_slots > 0) {
    regs_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{regs_ok, "and", "i1", gp_ok, fp_ok});
  } else if (layout.gp_chunks > 0) {
    regs_ok = gp_ok;
  } else {
    regs_ok = fp_ok;
  }

  const std::string reg_lbl = fresh_lbl(ctx, "vaarg.amd64.reg.");
  const std::string stack_lbl = fresh_lbl(ctx, "vaarg.amd64.stack.");
  const std::string join_lbl = fresh_lbl(ctx, "vaarg.amd64.join.");

  emit_condbr_and_open_lbl(ctx, regs_ok, reg_lbl, stack_lbl, reg_lbl);
  const std::string reg_value =
      emit_amd64_va_arg_from_registers(ctx, res_ts, res_ty, layout, access,
                                       gp_offset, fp_offset);
  emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
  const std::string stack_value =
      emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access,
                                      layout.size_bytes);
  emit_br_and_open_lbl(ctx, join_lbl, join_lbl);

  const std::string phi = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{
                        phi, res_ty,
                        {{reg_value, reg_lbl}, {stack_value, stack_lbl}}});
  return phi;
}

std::string StmtEmitter::emit_lval_dispatch(FnCtx& ctx, const Expr& e, TypeSpec& pts){
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->local) {
        pts = ctx.local_types.at(r->local->value);
        return ctx.local_slots.at(r->local->value);
      }
      // Parameter used as lvalue (e.g. p++): spill to a stack slot on first use
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const auto& param = ctx.fn->params[*r->param_index];
        pts = param.type.spec;
        const std::string pname = "%p." + sanitize_llvm_ident(param.name);
        // Check if we already have a spill slot
        auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
        if (it != ctx.param_slots.end()) {
          return it->second;
        }
        // Create a new alloca for this parameter
        const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
        ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(pts), "", 0});
        ctx.alloca_insts.push_back(lir::LirStoreOp{llvm_ty(pts), pname, slot});
        ctx.param_slots[*r->param_index + 0x80000000u] = slot;
        return slot;
      }
      if (r->global) {
        size_t gv_idx = r->global->value;
        const auto& gv0 = mod_.globals[gv_idx];
        if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
        const auto& gv = mod_.globals[gv_idx];
        pts = gv.type.spec;
        return llvm_global_sym(gv.name);
      }
      // Unresolved: assume external global
      pts = e.type.spec;
      return llvm_global_sym(r->name);
    }
    if (const auto* u = std::get_if<UnaryExpr>(&e.payload)) {
      if (u->op == UnaryOp::Deref) {
        TypeSpec ptr_ts{};
        const std::string ptr = emit_rval_id(ctx, u->operand, ptr_ts);
        pts = ptr_ts;
        if (pts.ptr_level > 0) pts.ptr_level--;
        return ptr;
      }
      if (u->op == UnaryOp::RealPart || u->op == UnaryOp::ImagPart) {
        TypeSpec complex_ts{};
        const std::string complex_ptr = emit_lval(ctx, u->operand, complex_ts);
        if (!is_complex_base(complex_ts.base)) {
          throw std::runtime_error("StmtEmitter: real/imag lvalue on non-complex expr");
        }
        pts = complex_component_ts(complex_ts.base);
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(complex_ts), complex_ptr, false,
                                        {"i32 0", "i32 " + std::to_string(u->op == UnaryOp::ImagPart ? 1 : 0)}});
        return tmp;
      }
    }
    if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
      TypeSpec base_ts{};
      std::string base;
      if (const TypeSpec resolved_base_ts = resolve_expr_type(ctx, idx->base);
          is_vector_value(resolved_base_ts)) {
        TypeSpec obj_ts{};
        base = emit_lval(ctx, idx->base, obj_ts);
        base_ts = obj_ts;
      } else {
        base = emit_rval_id(ctx, idx->base, base_ts);
      }
      TypeSpec ix_ts{};
      const std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
      // Widen index to i64
      TypeSpec i64_ts{}; i64_ts.base = TB_LONGLONG;
      const std::string ix64 = coerce(ctx, ix, ix_ts, i64_ts);
      // Element type: strip one array or pointer level with proper dim shifting.
      pts = base_ts;
      if (is_vector_value(pts)) {
        pts.is_vector = false;
        pts.vector_lanes = 0;
        pts.vector_bytes = 0;
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_ty(pts), base, false, {"i64 " + ix64}});
        return tmp;
      } else {
        pts = resolve_indexed_gep_pointee_type(pts);
        return emit_indexed_gep(ctx, base, base_ts, ix64);
      }
    }
    if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
      return emit_member_lval(ctx, *m, pts);
    }
    if (const auto* call = std::get_if<CallExpr>(&e.payload)) {
      TypeSpec storage_ts = resolve_payload_type(ctx, *call);
      if (storage_ts.is_lvalue_ref || storage_ts.is_rvalue_ref) {
        pts = storage_ts;
        if (pts.ptr_level > 0) pts.ptr_level--;
        pts.is_lvalue_ref = false;
        pts.is_rvalue_ref = false;
        return emit_rval_payload(ctx, *call, e);
      }
    }
    // Reference-type cast preserves addressability (xvalue semantics).
    // e.g. static_cast<T&&>(x) → same address as x.
    if (const auto* c = std::get_if<CastExpr>(&e.payload)) {
      if (c->to_type.spec.is_rvalue_ref || c->to_type.spec.is_lvalue_ref) {
        return emit_lval(ctx, c->expr, pts);
      }
    }
    throw std::runtime_error("StmtEmitter: cannot take lval of expr");
  }

std::string StmtEmitter::emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                BitfieldAccess* out_bf){
    MemberFieldAccess access = resolve_member_field_access(ctx, m);
    const std::string base_ptr = emit_member_base_ptr(ctx, m, access.base_ts);

    if (!access.has_tag()) {
      throw std::runtime_error(
          "StmtEmitter: MemberExpr base has no struct tag (field='" + m.field + "')");
    }
    if (!access.field_found) {
      throw std::runtime_error(
          "StmtEmitter: field '" + m.field + "' not found in struct/union '" +
          std::string(access.tag) + "'");
    }
    out_pts = access.field_ts;
    if (out_bf) *out_bf = access.bf;
    return emit_member_gep(ctx, base_ptr, access.chain);
  }

AssignableLValue StmtEmitter::emit_assignable_lval(FnCtx& ctx, ExprId id) {
    AssignableLValue access;
    const Expr& e = get_expr(id);
    if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
      access.ptr = emit_member_lval(ctx, *m, access.pointee_ts, &access.bf);
      return access;
    }
    access.ptr = emit_lval(ctx, id, access.pointee_ts);
    return access;
}

LoadedAssignableValue StmtEmitter::emit_load_assignable_value(FnCtx& ctx,
                                                              const AssignableLValue& lhs) {
    LoadedAssignableValue loaded;
    if (lhs.is_bitfield()) {
      loaded.value_ts = bitfield_promoted_ts(lhs.bf);
      loaded.value = emit_bitfield_load(ctx, lhs.ptr, lhs.bf);
      return loaded;
    }

    loaded.value_ts = lhs.pointee_ts;
    loaded.value = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{loaded.value, llvm_ty(lhs.pointee_ts), lhs.ptr});
    return loaded;
}

std::string StmtEmitter::emit_store_assignable_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                     const std::string& value,
                                                     const TypeSpec& value_ts,
                                                     bool reload_after_store) {
    if (lhs.is_bitfield()) {
      emit_bitfield_store(ctx, lhs.ptr, lhs.bf, value, value_ts);
      return reload_after_store ? emit_bitfield_load(ctx, lhs.ptr, lhs.bf) : value;
    }
    emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(lhs.pointee_ts), value, lhs.ptr});
    return value;
}

std::string StmtEmitter::emit_assignable_incdec_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                      bool increment,
                                                      bool return_updated_value) {
    if (lhs.is_bitfield()) {
      const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
      const std::string pty = llvm_ty(loaded.value_ts);
      const std::string delta = increment ? "1" : "-1";
      const std::string new_val = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{new_val, "add", pty, loaded.value, delta});
      const std::string stored =
          emit_store_assignable_value(ctx, lhs, new_val, loaded.value_ts, return_updated_value);
      return return_updated_value ? stored : loaded.value;
    }

    const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
    const std::string pty = llvm_ty(loaded.value_ts);

    const std::string new_val = fresh_tmp(ctx);
    if (pty == "ptr") {
      const std::string delta = increment ? "1" : "-1";
      emit_lir_op(ctx, lir::LirGepOp{new_val, indexed_gep_elem_ty(lhs.pointee_ts), loaded.value, false,
                                     {"i64 " + delta}});
    } else if (is_float_base(loaded.value_ts.base)) {
      const std::string delta = increment ? "1.0" : "-1.0";
      emit_lir_op(ctx, lir::LirBinOp{new_val, "fadd", pty, loaded.value, delta});
    } else {
      const std::string delta = increment ? "1" : "-1";
      emit_lir_op(ctx, lir::LirBinOp{new_val, "add", pty, loaded.value, delta});
    }
    emit_store_assignable_value(ctx, lhs, new_val, lhs.pointee_ts, false);
    return return_updated_value ? new_val : loaded.value;
}

std::string StmtEmitter::emit_set_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                               const std::string& rhs,
                                               const TypeSpec& rhs_ts) {
    if (lhs.is_bitfield()) {
      // In C, (bf = val) yields the value of bf after assignment,
      // which is truncated to the bitfield width.
      return emit_store_assignable_value(ctx, lhs, rhs, rhs_ts, true);
    }

    std::string coerced_rhs = coerce(ctx, rhs, rhs_ts, lhs.pointee_ts);
    const bool is_agg = (lhs.pointee_ts.base == TB_STRUCT || lhs.pointee_ts.base == TB_UNION) &&
                        lhs.pointee_ts.ptr_level == 0 && lhs.pointee_ts.array_rank == 0;
    if (is_agg && (coerced_rhs == "0" || coerced_rhs.empty())) coerced_rhs = "zeroinitializer";
    return emit_store_assignable_value(ctx, lhs, coerced_rhs, lhs.pointee_ts, false);
}

std::string StmtEmitter::emit_compound_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                    AssignOp op, const std::string& rhs,
                                                    const TypeSpec& rhs_ts) {
    const TypeSpec& lhs_ts = lhs.pointee_ts;
    const std::string lty = llvm_ty(lhs_ts);

    if (lhs.is_bitfield()) {
      const BitfieldAccess& bf = lhs.bf;
      const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
      const std::string promoted_ty = llvm_ty(loaded.value_ts);
      std::string lhs_op = loaded.value;
      std::string rhs_op = coerce(ctx, rhs, rhs_ts, loaded.value_ts);
      const bool ls = is_signed_int(loaded.value_ts.base);
      const char* instr = nullptr;
      static const struct { AssignOp op; const char* is; const char* iu; } tbl[] = {
        {AssignOp::Add, "add", "add"}, {AssignOp::Sub, "sub", "sub"},
        {AssignOp::Mul, "mul", "mul"}, {AssignOp::Div, "sdiv", "udiv"},
        {AssignOp::Mod, "srem", "urem"}, {AssignOp::Shl, "shl", "shl"},
        {AssignOp::Shr, "ashr", "lshr"}, {AssignOp::BitAnd, "and", "and"},
        {AssignOp::BitOr, "or", "or"}, {AssignOp::BitXor, "xor", "xor"},
      };
      for (const auto& r : tbl) {
        if (r.op == op) { instr = ls ? r.is : r.iu; break; }
      }
      if (!instr) throw std::runtime_error("StmtEmitter: bitfield compound assign: unknown op");
      const std::string result = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{result, std::string(instr), promoted_ty, lhs_op, rhs_op});
      return emit_store_assignable_value(ctx, lhs, result, loaded.value_ts, false);
    }

    const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);

    if ((op == AssignOp::Add || op == AssignOp::Sub) && lty == "ptr") {
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      std::string delta = coerce(ctx, rhs, rhs_ts, i64_ts);
      if (op == AssignOp::Sub) {
        const std::string neg = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", delta});
        delta = neg;
      }
      TypeSpec elem_ts = lhs_ts;
      if (elem_ts.ptr_level > 0) {
        elem_ts.ptr_level -= 1;
      } else {
        elem_ts.base = TB_CHAR;
      }
      const std::string gep_ety3 =
          (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
              ? "i8" : llvm_ty(elem_ts);
      const std::string result = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{result, gep_ety3, loaded.value, false, {"i64 " + delta}});
      return emit_store_assignable_value(ctx, lhs, result, lhs_ts, false);
    }

    static const struct { AssignOp op; BinaryOp bop; } compound_map[] = {
      { AssignOp::Add, BinaryOp::Add }, { AssignOp::Sub, BinaryOp::Sub },
      { AssignOp::Mul, BinaryOp::Mul }, { AssignOp::Div, BinaryOp::Div },
      { AssignOp::Mod, BinaryOp::Mod }, { AssignOp::Shl, BinaryOp::Shl },
      { AssignOp::Shr, BinaryOp::Shr }, { AssignOp::BitAnd, BinaryOp::BitAnd },
      { AssignOp::BitOr, BinaryOp::BitOr }, { AssignOp::BitXor, BinaryOp::BitXor },
    };
    const char* instr = nullptr;
    TypeSpec op_ts = lhs_ts;
    std::string coerced_rhs = rhs;
    for (const auto& row : compound_map) {
      if (row.op != op) continue;
      if ((row.bop == BinaryOp::Add || row.bop == BinaryOp::Sub ||
           row.bop == BinaryOp::Mul || row.bop == BinaryOp::Div) &&
          (is_complex_base(lhs_ts.base) || is_complex_base(rhs_ts.base))) {
        const std::string result =
            emit_complex_binary_arith(ctx, row.bop, loaded.value, lhs_ts, coerced_rhs, rhs_ts,
                                      lhs_ts);
        return emit_store_assignable_value(ctx, lhs, result, lhs_ts, false);
      }
      op_ts = resolve_compound_assign_op_type(row.bop, lhs_ts, rhs_ts);
      coerced_rhs = coerce(ctx, coerced_rhs, rhs_ts, op_ts);
      const bool lf = is_float_base(op_ts.base);
      const bool ls = is_signed_int(op_ts.base);
      static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } tbl[] = {
        { BinaryOp::Add, "add",  "add",  "fadd" }, { BinaryOp::Sub, "sub", "sub", "fsub" },
        { BinaryOp::Mul, "mul",  "mul",  "fmul" }, { BinaryOp::Div, "sdiv","udiv","fdiv"  },
        { BinaryOp::Mod, "srem", "urem", nullptr }, { BinaryOp::Shl, "shl","shl", nullptr  },
        { BinaryOp::Shr, "ashr","lshr", nullptr  }, { BinaryOp::BitAnd,"and","and",nullptr  },
        { BinaryOp::BitOr, "or","or",   nullptr  }, { BinaryOp::BitXor,"xor","xor",nullptr },
      };
      for (const auto& r : tbl) {
        if (r.op == row.bop) {
          if (lf) instr = r.f;
          else if (r.op == BinaryOp::Shr) instr = is_signed_int(lhs_ts.base) ? r.is : r.iu;
          else instr = ls ? r.is : r.iu;
          break;
        }
      }
      if (!instr) break;
      return emit_nonptr_compound_assign_value(ctx, lhs, loaded, row.bop, instr, rhs, rhs_ts);
    }
    if (!instr) throw std::runtime_error("StmtEmitter: compound assign: unknown op");
    throw std::runtime_error("StmtEmitter: compound assign: unreachable non-pointer path");
}

TypeSpec StmtEmitter::resolve_compound_assign_op_type(BinaryOp op, const TypeSpec& lhs_ts,
                                                      const TypeSpec& rhs_ts) {
    TypeSpec op_ts = lhs_ts;
    if (is_vector_value(lhs_ts)) return op_ts;
    if (lhs_ts.ptr_level != 0 || lhs_ts.array_rank != 0 ||
        rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) {
      return op_ts;
    }
    if (is_float_base(lhs_ts.base) || is_float_base(rhs_ts.base)) {
      op_ts.base = (lhs_ts.base == TB_DOUBLE || rhs_ts.base == TB_DOUBLE ||
                    lhs_ts.base == TB_LONGDOUBLE || rhs_ts.base == TB_LONGDOUBLE)
                       ? TB_DOUBLE
                       : TB_FLOAT;
      return op_ts;
    }
    if (is_any_int(lhs_ts.base) && is_any_int(rhs_ts.base)) {
      const bool is_shift = (op == BinaryOp::Shl || op == BinaryOp::Shr);
      op_ts.base = is_shift ? integer_promote(lhs_ts.base)
                            : usual_arith_conv(lhs_ts.base, rhs_ts.base);
    }
    return op_ts;
}

std::string StmtEmitter::emit_nonptr_compound_assign_value(FnCtx& ctx,
                                                           const AssignableLValue& lhs,
                                                           const LoadedAssignableValue& loaded,
                                                           BinaryOp op, const char* instr,
                                                           const std::string& rhs,
                                                           const TypeSpec& rhs_ts) {
    const TypeSpec& lhs_ts = lhs.pointee_ts;
    const TypeSpec op_ts = resolve_compound_assign_op_type(op, lhs_ts, rhs_ts);
    const std::string op_ty = llvm_ty(op_ts);
    std::string lhs_op = loaded.value;
    if (op_ty != llvm_ty(lhs_ts)) lhs_op = coerce(ctx, loaded.value, lhs_ts, op_ts);
    const std::string rhs_op = coerce(ctx, rhs, rhs_ts, op_ts);
    const std::string result = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{result, std::string(instr), op_ty, lhs_op, rhs_op});
    std::string store_v = result;
    if (op_ty != llvm_ty(lhs_ts)) store_v = coerce(ctx, result, op_ts, lhs_ts);
    return emit_store_assignable_value(ctx, lhs, store_v, lhs_ts, false);
}

TypeSpec StmtEmitter::resolve_member_base_type(FnCtx& ctx, ExprId base_id, bool is_arrow){
    const Expr& base_e = get_expr(base_id);
    TypeSpec base_ts = base_e.type.spec;
    if (base_ts.base == TB_VOID && base_ts.ptr_level == 0)
      base_ts = resolve_expr_type(ctx, base_id);
    // Resolve typedef-wrapped types so struct/union checks below see the real base.
    // resolve_expr_type returns stored type early; force payload-based resolution.
    if (base_ts.base == TB_TYPEDEF) {
      TypeSpec resolved = std::visit([&](const auto& p) -> TypeSpec {
        return resolve_payload_type(ctx, p);
      }, base_e.payload);
      if (resolved.base != TB_VOID || resolved.ptr_level > 0 || resolved.array_rank > 0)
        base_ts = resolved;
    }
    if (is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
    return base_ts;
  }

MemberFieldAccess StmtEmitter::resolve_member_field_access(FnCtx& ctx, const MemberExpr& m){
    MemberFieldAccess access;
    access.base_ts = resolve_member_base_type(ctx, m.base, m.is_arrow);
    access.tag = access.base_ts.tag;
    if (!access.has_tag()) return access;
    access.field_found = resolve_field_access(access.tag, m.field, access.chain,
                                              access.field_ts, &access.bf);
    return access;
  }

std::string StmtEmitter::emit_member_base_ptr(FnCtx& ctx, const MemberExpr& m, TypeSpec& base_ts){
    if (m.is_arrow) {
      TypeSpec ptr_ts{};
      return emit_rval_id(ctx, m.base, ptr_ts);
    }

    const Expr& base_e = get_expr(m.base);
    TypeSpec dummy{};
    try {
      return emit_lval_dispatch(ctx, base_e, dummy);
    } catch (const std::runtime_error&) {
      if (base_ts.base != TB_STRUCT && base_ts.base != TB_UNION) {
        throw;
      }
      TypeSpec rval_ts{};
      const std::string rval = emit_rval_id(ctx, m.base, rval_ts);
      if (rval_ts.base != TB_VOID || rval_ts.ptr_level > 0 || rval_ts.array_rank > 0) {
        base_ts = rval_ts;
      }
      const std::string slot = fresh_tmp(ctx) + ".agg";
      ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(base_ts), "", 0});
      emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(base_ts), rval, slot});
      return slot;
    }
  }

std::string StmtEmitter::indexed_gep_elem_ty(const TypeSpec& base_ts){
    const TypeSpec elem_ts = resolve_indexed_gep_pointee_type(base_ts);
    if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0) {
      return "i8";
    }
    if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0) {
      return llvm_alloca_ty(elem_ts);
    }
    return llvm_ty(elem_ts);
  }

std::string StmtEmitter::emit_indexed_gep(FnCtx& ctx, const std::string& base_ptr,
                                          const TypeSpec& base_ts, const std::string& idx){
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, indexed_gep_elem_ty(base_ts), base_ptr, false,
                                   {"i64 " + idx}});
    return tmp;
  }

std::string StmtEmitter::emit_rval_from_access_ptr(FnCtx& ctx, const std::string& ptr,
                                         const TypeSpec& access_ts, const TypeSpec& load_ts,
                                         bool decay_from_array_object){
    if (outer_array_rank(access_ts) > 0) {
      if (!decay_from_array_object) return ptr;
      const std::string arr_alloca_ty = llvm_alloca_ty(access_ts);
      if (arr_alloca_ty == "ptr") {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), ptr});
        return tmp;
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, arr_alloca_ty, ptr, false, {"i64 0", "i64 0"}});
      return tmp;
    }
    const std::string ty = llvm_ty(load_ts);
    if (ty == "void") return "";
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, ptr});
    return tmp;
  }

std::string StmtEmitter::emit_rval_from_access_expr(FnCtx& ctx, const Expr& e,
                                                    const std::string& ptr,
                                                    const TypeSpec& access_ts,
                                                    bool decay_from_array_object){
    TypeSpec load_ts = resolve_expr_type(ctx, e);
    if (!has_concrete_type(load_ts)) load_ts = access_ts;
    return emit_rval_from_access_ptr(ctx, ptr, access_ts, load_ts,
                                     decay_from_array_object);
  }

TypeSpec StmtEmitter::resolve_expr_type(FnCtx& ctx, ExprId id){
    const Expr& e = get_expr(id);
    // Use stored type if not void
    const TypeSpec& ts = e.type.spec;
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
        is_vector_value(ts))
      return ts;

    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

TypeSpec StmtEmitter::resolve_expr_type(FnCtx& ctx, const Expr& e){
    if (has_concrete_type(e.type.spec)) return e.type.spec;
    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

const FnPtrSig* StmtEmitter::resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e){
    if (const auto* u = std::get_if<UnaryExpr>(&callee_e.payload)) {
      if (u->op == UnaryOp::Deref) {
        const Expr& inner_e = get_expr(u->operand);
        return resolve_callee_fn_ptr_sig(ctx, inner_e);
      }
    }
    if (const auto* dr = std::get_if<DeclRef>(&callee_e.payload)) {
      if (dr->local) {
        const auto it = ctx.local_fn_ptr_sigs.find(dr->local->value);
        if (it != ctx.local_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->param_index) {
        const auto it = ctx.param_fn_ptr_sigs.find(*dr->param_index);
        if (it != ctx.param_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->global) {
        const auto it = ctx.global_fn_ptr_sigs.find(dr->global->value);
        if (it != ctx.global_fn_ptr_sigs.end()) return &it->second;
      }
    }
    // Phase C: MemberExpr — look up the struct field's fn_ptr_sig.
    if (const auto* m = std::get_if<MemberExpr>(&callee_e.payload)) {
      TypeSpec base_ts = resolve_expr_type(ctx, m->base);
      if (m->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
      if (base_ts.tag && base_ts.tag[0]) {
        const auto sit = mod_.struct_defs.find(base_ts.tag);
        if (sit != mod_.struct_defs.end()) {
          for (const auto& f : sit->second.fields) {
            if (f.name == m->field && f.fn_ptr_sig) {
              return &*f.fn_ptr_sig;
            }
          }
        }
      }
    }
    // Phase C: TernaryExpr — recurse on the then branch.
    if (const auto* t = std::get_if<TernaryExpr>(&callee_e.payload)) {
      return resolve_callee_fn_ptr_sig(ctx, get_expr(t->then_expr));
    }
    // Phase C slice 2: CastExpr — use the cast target's fn_ptr_sig.
    if (const auto* c = std::get_if<CastExpr>(&callee_e.payload)) {
      if (c->fn_ptr_sig) return &*c->fn_ptr_sig;
    }
    // Phase C slice 2: IndexExpr — recurse on base (array/ptr of fn_ptrs).
    if (const auto* idx = std::get_if<IndexExpr>(&callee_e.payload)) {
      return resolve_callee_fn_ptr_sig(ctx, get_expr(idx->base));
    }
    // Phase C slice 2: CallExpr — look up called function's ret_fn_ptr_sig.
    if (const auto* call = std::get_if<CallExpr>(&callee_e.payload)) {
      auto find_function = [&](const std::string& name) -> const Function* {
        const auto fit = mod_.fn_index.find(name);
        if (fit == mod_.fn_index.end() || fit->second.value >= mod_.functions.size()) return nullptr;
        return &mod_.functions[fit->second.value];
      };
      auto build_fn_sig = [&](const Function& fn) -> const FnPtrSig* {
        auto [it, _] = inferred_direct_fn_sigs_.try_emplace(fn.id.value);
        FnPtrSig& sig = it->second;
        if (sig.params.empty() && !sig.variadic && sig.return_type.spec.base == TB_VOID &&
            sig.return_type.spec.ptr_level == 0 && sig.return_type.spec.array_rank == 0) {
          sig.return_type = fn.return_type;
          sig.variadic = fn.attrs.variadic;
          sig.unspecified_params = false;
          for (const auto& param : fn.params) sig.params.push_back(param.type);
        }
        return &sig;
      };
      auto infer_returned_function = [&](auto&& self, const Function& fn) -> const Function* {
        for (const Block& bb : fn.blocks) {
          for (const Stmt& stmt : bb.stmts) {
            const auto* ret = std::get_if<ReturnStmt>(&stmt.payload);
            if (!ret || !ret->expr) continue;
            const Expr& expr = get_expr(*ret->expr);
            if (const auto* uret = std::get_if<UnaryExpr>(&expr.payload)) {
              if (uret->op == UnaryOp::AddrOf) {
                const Expr& inner = get_expr(uret->operand);
                if (const auto* inner_dr = std::get_if<DeclRef>(&inner.payload)) {
                  if (const Function* returned = find_function(inner_dr->name)) return returned;
                }
              }
            }
            if (const auto* dr2 = std::get_if<DeclRef>(&expr.payload)) {
              if (const Function* returned = find_function(dr2->name)) return returned;
            }
            if (const auto* ternary = std::get_if<TernaryExpr>(&expr.payload)) {
              const Expr& then_e = get_expr(ternary->then_expr);
              if (const auto* then_dr = std::get_if<DeclRef>(&then_e.payload)) {
                if (const Function* returned = find_function(then_dr->name)) return returned;
              }
              const Expr& else_e = get_expr(ternary->else_expr);
              if (const auto* else_dr = std::get_if<DeclRef>(&else_e.payload)) {
                if (const Function* returned = find_function(else_dr->name)) return returned;
              }
            }
            if (const auto* call_expr = std::get_if<CallExpr>(&expr.payload)) {
              const Expr& nested_callee = get_expr(call_expr->callee);
              if (const auto* nested_dr = std::get_if<DeclRef>(&nested_callee.payload)) {
                if (const Function* called = find_function(nested_dr->name)) {
                  if (const Function* returned = self(self, *called)) return returned;
                }
              }
            }
          }
        }
        return nullptr;
      };

      const Expr& inner_callee = get_expr(call->callee);
      if (const auto* dr = std::get_if<DeclRef>(&inner_callee.payload)) {
        if (const Function* target = find_function(dr->name)) {
          const FnPtrSig* cached_sig =
              target->ret_fn_ptr_sig ? &*target->ret_fn_ptr_sig : nullptr;
          if (cached_sig && sig_has_explicit_prototype(*cached_sig)) return cached_sig;
          if (const Function* returned = infer_returned_function(infer_returned_function, *target)) {
            return build_fn_sig(*returned);
          }
          if (cached_sig) return cached_sig;
        }
      }
      if (const auto* nested_call = std::get_if<CallExpr>(&inner_callee.payload)) {
        const Expr& nested_callee = get_expr(nested_call->callee);
        if (const auto* nested_dr = std::get_if<DeclRef>(&nested_callee.payload)) {
          if (const Function* target = find_function(nested_dr->name)) {
            if (const Function* returned = infer_returned_function(infer_returned_function, *target)) {
              if (const Function* nested_returned =
                      infer_returned_function(infer_returned_function, *returned)) {
                return build_fn_sig(*nested_returned);
              }
              return build_fn_sig(*returned);
            }
          }
        }
      }
    }
    return nullptr;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const DeclRef& r){
    if (r.local) {
      const auto it = ctx.local_types.find(r.local->value);
      if (it != ctx.local_types.end()) return it->second;
    }
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size())
      return ctx.fn->params[*r.param_index].type.spec;
    if (r.global) {
      if (const GlobalVar* gv = select_global_object(*r.global)) return gv->type.spec;
    }
    return {};
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const BinaryExpr& b){
    switch (b.op) {
      case BinaryOp::Lt:
      case BinaryOp::Le:
      case BinaryOp::Gt:
      case BinaryOp::Ge:
      case BinaryOp::Eq:
      case BinaryOp::Ne:
      case BinaryOp::LAnd:
      case BinaryOp::LOr: {
        TypeSpec ts{};
        ts.base = TB_INT;
        return ts;
      }
      case BinaryOp::Comma: {
        // Comma operator: result type/value is from RHS
        const Expr& rhs_e = get_expr(b.rhs);
        return std::visit([&](const auto& p) -> TypeSpec {
          return resolve_payload_type(ctx, p);
        }, rhs_e.payload);
      }
      default:
        break;
    }
    const TypeSpec lts = resolve_expr_type(ctx, b.lhs);
    const TypeSpec rts = resolve_expr_type(ctx, b.rhs);

    if (b.op == BinaryOp::Sub && llvm_ty(lts) == "ptr" && llvm_ty(rts) == "ptr") {
      TypeSpec ts{};
      ts.base = TB_LONGLONG;
      return ts;
    }
    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) &&
        llvm_ty(lts) == "ptr" &&
        rts.ptr_level == 0 && rts.array_rank == 0 && is_any_int(rts.base)) {
      return lts;
    }
    if (b.op == BinaryOp::Add &&
        llvm_ty(rts) == "ptr" &&
        lts.ptr_level == 0 && lts.array_rank == 0 && is_any_int(lts.base)) {
      return rts;
    }
    if (is_complex_base(lts.base) || is_complex_base(rts.base)) {
      if (!is_complex_base(lts.base)) return rts;
      if (!is_complex_base(rts.base)) return lts;
      return sizeof_ts(mod_, lts) >= sizeof_ts(mod_, rts) ? lts : rts;
    }
    if (is_vector_value(lts)) return lts;
    if (is_vector_value(rts)) return rts;

    // For arithmetic/bitwise the result type is the UAC common type.
    if (lts.base != TB_VOID && rts.base != TB_VOID &&
        lts.ptr_level == 0 && rts.ptr_level == 0 &&
        lts.array_rank == 0 && rts.array_rank == 0 &&
        is_any_int(lts.base) && is_any_int(rts.base)) {
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      TypeSpec ts{};
      ts.base = is_shift ? integer_promote(lts.base) : usual_arith_conv(lts.base, rts.base);
      return ts;
    }
    if (lts.base != TB_VOID) return lts;
    return resolve_expr_type(ctx, b.rhs);
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const UnaryExpr& u){
    TypeSpec ts = resolve_expr_type(ctx, u.operand);
    switch (u.op) {
      case UnaryOp::AddrOf:
        if (ts.array_rank > 0 && !is_vector_value(ts)) {
          // &array produces a pointer-to-array; preserve array dims so that
          // pointer arithmetic uses the correct stride and deref produces
          // array decay instead of a scalar load.
          ts.is_ptr_to_array = true;
        }
        ts.ptr_level += 1;
        return ts;
      case UnaryOp::Deref:
        if (ts.ptr_level > 0) {
          ts.ptr_level -= 1;
          // After consuming the pointer level, the result is an array value
          // (not a pointer-to-array), so clear the flag.
          if (ts.ptr_level == 0) ts.is_ptr_to_array = false;
        } else if (ts.array_rank > 0) {
          ts.array_rank -= 1;
        }
        return ts;
      case UnaryOp::RealPart:
      case UnaryOp::ImagPart:
        if (is_complex_base(ts.base)) return complex_component_ts(ts.base);
        return ts;
      case UnaryOp::Not: {
        TypeSpec out{};
        out.base = TB_INT;
        return out;
      }
      case UnaryOp::BitNot:
      case UnaryOp::Plus:
      case UnaryOp::Minus:
        if (ts.ptr_level == 0 && !is_vector_value(ts))
          ts.base = integer_promote(ts.base);
        return ts;
      default:
        return ts;
    }
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const AssignExpr& a){
    return resolve_expr_type(ctx, a.lhs);
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const CastExpr& c){
    return c.to_type.spec;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const CallExpr& c){
    const BuiltinId builtin_id = c.builtin_id;
    if (builtin_id != BuiltinId::Unknown) {
      if (builtin_id == BuiltinId::Expect && !c.args.empty())
        return resolve_expr_type(ctx, c.args[0]);
      switch (builtin_id) {
        case BuiltinId::Unreachable:
        case BuiltinId::VaStart:
        case BuiltinId::VaEnd:
        case BuiltinId::VaCopy:
        case BuiltinId::Prefetch:
          return {};
        case BuiltinId::Bswap16:
        case BuiltinId::Bswap32:
        case BuiltinId::Bswap64:
          if (!c.args.empty()) return resolve_expr_type(ctx, c.args[0]);
          break;
        default:
          break;
      }
      if (auto builtin_ts = type_spec_from_builtin_result_kind(
              builtin_result_kind(builtin_id))) {
        return *builtin_ts;
      }
    }
    const Expr& callee_e = get_expr(c.callee);
    // Direct function call: look up return type from the function definition.
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      const auto fit = mod_.fn_index.find(r->name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        TypeSpec ret = mod_.functions[fit->second.value].return_type.spec;
        // Reference return: the LLVM call actually returns ptr, so bump ptr_level
        // to keep the type annotation consistent with the LLVM value.
        if ((ret.is_lvalue_ref || ret.is_rvalue_ref) && ret.ptr_level == 0) ret.ptr_level++;
        return ret;
      }
    }
    // Indirect call through fn-ptr: use canonical fn_ptr_sig for return type.
    if (const FnPtrSig* sig = resolve_callee_fn_ptr_sig(ctx, callee_e)) {
      return sig_return_type(*sig);
    }
    // Check known libc return types before falling back to implicit int
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      if (auto kts = type_spec_from_builtin_result_kind(
              known_call_result_kind(r->name.c_str()))) {
        return *kts;
      }
    }
    // Unknown external call: C implicit declaration defaults to int return
    TypeSpec implicit_int{}; implicit_int.base = TB_INT;
    return implicit_int;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const IntLiteral&){
    TypeSpec ts{}; ts.base = TB_INT; return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const FloatLiteral&){
    TypeSpec ts{}; ts.base = TB_DOUBLE; return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const CharLiteral&){
    TypeSpec ts{}; ts.base = TB_CHAR; return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const StringLiteral& sl){
    TypeSpec ts{};
    ts.base = sl.is_wide ? TB_INT : TB_CHAR;
    ts.ptr_level = 1;
    return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const MemberExpr& m){
    const MemberFieldAccess access = resolve_member_field_access(ctx, m);
    if (!access.has_tag() || !access.field_found) return {};
    if (access.bf.is_bitfield()) return bitfield_promoted_ts(access.bf);
    return access.field_ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const IndexExpr& idx){
    TypeSpec base_ts = resolve_expr_type(ctx, idx.base);
    if (is_vector_value(base_ts)) {
      base_ts.is_vector = false;
      base_ts.vector_lanes = 0;
      base_ts.vector_bytes = 0;
      return base_ts;
    }
    return drop_one_indexed_element_type(base_ts);
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const TernaryExpr& t){
    TypeSpec ts = resolve_expr_type(ctx, t.then_expr);
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
        is_vector_value(ts))
      return ts;
    return resolve_expr_type(ctx, t.else_expr);
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const VaArgExpr&){
    return {};
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const SizeofExpr&){
    TypeSpec ts{};
    ts.base = TB_ULONGLONG;
    return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const SizeofTypeExpr&){
    TypeSpec ts{};
    ts.base = TB_ULONGLONG;
    return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const LabelAddrExpr&){
    TypeSpec ts{};
    ts.base = TB_VOID;
    ts.ptr_level = 1;
    return ts;
  }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const PendingConstevalExpr&){
    return {};
  }

template <typename T>
  TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const T&){ return {}; }

std::string StmtEmitter::emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts){
    const Expr& e = get_expr(id);
    out_ts = e.type.spec;
    if (const auto* b = std::get_if<BinaryExpr>(&e.payload)) {
      // Prefer semantic reconstruction for binary expressions; parser-provided
      // AST types are often too coarse for pointer arithmetic.
      out_ts = resolve_payload_type(ctx, *b);
    } else if (const auto* c = std::get_if<CallExpr>(&e.payload)) {
      // Builtin calls such as __builtin_copysignl/__builtin_fabsl can refine
      // the return type beyond what survived from the parser AST.
      out_ts = resolve_payload_type(ctx, *c);
    }
    // AST does not annotate types on all nodes; resolve recursively.
    if (out_ts.base == TB_VOID && out_ts.ptr_level == 0 && out_ts.array_rank == 0) {
      out_ts = resolve_expr_type(ctx, id);
    }
    return emit_rval_expr(ctx, e);
  }

std::string StmtEmitter::emit_rval_expr(FnCtx& ctx, const Expr& e){
    return std::visit([&](const auto& p) -> std::string {
      return emit_rval_payload(ctx, p, e);
    }, e.payload);
  }

template <typename T>
  std::string StmtEmitter::emit_rval_payload(FnCtx&, const T&, const Expr&){
    throw std::runtime_error(
        std::string("StmtEmitter: unimplemented expr: ") + typeid(T).name());
  }

std::string StmtEmitter::emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e){
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + emit_const_int_like(x.value, elem_ts) + " }";
    }
    return std::to_string(x.value);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e){
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      const std::string imag_v = fp_literal(elem_ts.base, x.value);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + imag_v + " }";
    }
    return is_float_base(e.type.spec.base) ? fp_literal(e.type.spec.base, x.value)
                                           : fp_to_hex(x.value);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&){
    return std::to_string(x.value);
  }

std::string StmtEmitter::emit_complex_binary_arith(FnCtx& ctx,
                                        BinaryOp op,
                                        const std::string& lv,
                                        const TypeSpec& lts,
                                        const std::string& rv,
                                        const TypeSpec& rts,
                                        const TypeSpec& res_spec){
    TypeSpec complex_ts = lts;
    if (!is_complex_base(complex_ts.base) ||
        (is_complex_base(rts.base) && sizeof_ts(mod_, rts) > sizeof_ts(mod_, complex_ts))) {
      complex_ts = rts;
    }
    const TypeSpec elem_ts = complex_component_ts(complex_ts.base);
    auto lift_scalar_to_complex = [&](const std::string& scalar,
                                      const TypeSpec& scalar_ts) -> std::string {
      const std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string imag_v = emit_const_int_like(0, elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{with_real, llvm_ty(complex_ts), "undef", llvm_ty(elem_ts), real_v, 0});
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{out, llvm_ty(complex_ts), with_real, llvm_ty(elem_ts), imag_v, 1});
      return out;
    };

    std::string complex_lv = lv;
    std::string complex_rv = rv;
    TypeSpec complex_lts = lts;
    TypeSpec complex_rts = rts;
    if (!is_complex_base(complex_lts.base)) {
      complex_lv = lift_scalar_to_complex(complex_lv, complex_lts);
      complex_lts = complex_ts;
    } else if (llvm_ty(complex_lts) != llvm_ty(complex_ts)) {
      complex_lv = coerce(ctx, complex_lv, complex_lts, complex_ts);
      complex_lts = complex_ts;
    }
    if (!is_complex_base(complex_rts.base)) {
      complex_rv = lift_scalar_to_complex(complex_rv, complex_rts);
      complex_rts = complex_ts;
    } else if (llvm_ty(complex_rts) != llvm_ty(complex_ts)) {
      complex_rv = coerce(ctx, complex_rv, complex_rts, complex_ts);
      complex_rts = complex_ts;
    }

    const std::string cplx_ty = llvm_ty(complex_ts);
    const std::string lreal = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{lreal, cplx_ty, complex_lv, 0});
    const std::string limag = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{limag, cplx_ty, complex_lv, 1});
    const std::string rreal = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{rreal, cplx_ty, complex_rv, 0});
    const std::string rimag = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{rimag, cplx_ty, complex_rv, 1});

    const std::string elem_ty = llvm_ty(elem_ts);
    const char* add_instr = is_float_base(elem_ts.base) ? "fadd" : "add";
    const char* sub_instr = is_float_base(elem_ts.base) ? "fsub" : "sub";
    const char* mul_instr = is_float_base(elem_ts.base) ? "fmul" : "mul";
    const char* div_instr = is_float_base(elem_ts.base)
                                ? "fdiv"
                                : (is_signed_int(elem_ts.base) ? "sdiv" : "udiv");

    std::string out_real;
    std::string out_imag;
    if (op == BinaryOp::Add || op == BinaryOp::Sub) {
      out_real = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{out_real, std::string(op == BinaryOp::Add ? add_instr : sub_instr), elem_ty, lreal, rreal});
      out_imag = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{out_imag, std::string(op == BinaryOp::Add ? add_instr : sub_instr), elem_ty, limag, rimag});
    } else if (op == BinaryOp::Mul) {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{ac, std::string(mul_instr), elem_ty, lreal, rreal});
      emit_lir_op(ctx, lir::LirBinOp{bd, std::string(mul_instr), elem_ty, limag, rimag});
      emit_lir_op(ctx, lir::LirBinOp{ad, std::string(mul_instr), elem_ty, lreal, rimag});
      emit_lir_op(ctx, lir::LirBinOp{bc, std::string(mul_instr), elem_ty, limag, rreal});
      out_real = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{out_real, std::string(sub_instr), elem_ty, ac, bd});
      out_imag = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{out_imag, std::string(add_instr), elem_ty, ad, bc});
    } else {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string cc = fresh_tmp(ctx);
      const std::string dd = fresh_tmp(ctx);
      const std::string denom = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{ac, std::string(mul_instr), elem_ty, lreal, rreal});
      emit_lir_op(ctx, lir::LirBinOp{bd, std::string(mul_instr), elem_ty, limag, rimag});
      emit_lir_op(ctx, lir::LirBinOp{bc, std::string(mul_instr), elem_ty, limag, rreal});
      emit_lir_op(ctx, lir::LirBinOp{ad, std::string(mul_instr), elem_ty, lreal, rimag});
      emit_lir_op(ctx, lir::LirBinOp{cc, std::string(mul_instr), elem_ty, rreal, rreal});
      emit_lir_op(ctx, lir::LirBinOp{dd, std::string(mul_instr), elem_ty, rimag, rimag});
      emit_lir_op(ctx, lir::LirBinOp{denom, std::string(add_instr), elem_ty, cc, dd});
      const std::string real_num = fresh_tmp(ctx);
      const std::string imag_num = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{real_num, std::string(add_instr), elem_ty, ac, bd});
      emit_lir_op(ctx, lir::LirBinOp{imag_num, std::string(sub_instr), elem_ty, bc, ad});
      out_real = fresh_tmp(ctx);
      out_imag = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{out_real, std::string(div_instr), elem_ty, real_num, denom});
      emit_lir_op(ctx, lir::LirBinOp{out_imag, std::string(div_instr), elem_ty, imag_num, denom});
    }

    const std::string with_real = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{with_real, cplx_ty, "undef", elem_ty, out_real, 0});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{out, cplx_ty, with_real, elem_ty, out_imag, 1});
    return cplx_ty == llvm_ty(res_spec) ? out : coerce(ctx, out, complex_ts, res_spec);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& /*e*/){
    if (sl.is_wide) {
      // Wide string: emit as global array of i32 (wchar_t) values
      const auto vals = decode_wide_string_values(sl.raw);
      const std::string gname = "@.str" + std::to_string(module_->str_pool_idx++);
      std::string init = "[";
      for (size_t i = 0; i < vals.size(); ++i) {
        if (i) init += ", ";
        init += "i32 " + std::to_string(vals[i]);
      }
      init += "]";
      {
        lir::LirStringConst sc;
        sc.pool_name = gname;
        sc.raw_bytes = "[" + std::to_string(vals.size()) + " x i32] " + init;
        sc.byte_length = -1;  // sentinel: raw_bytes is pre-formatted type+init
        module_->string_pool.push_back(std::move(sc));
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(vals.size()) + " x i32]", gname, false, {"i64 0", "i64 0"}});
      return tmp;
    }
    // Strip enclosing quotes from raw, use as bytes
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    }
    bytes = decode_c_escaped_bytes(bytes);
    const std::string gname = intern_str(bytes);
    const size_t len = bytes.size() + 1;
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(len) + " x i8]", gname, false, {"i64 0", "i64 0"}});
    return tmp;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e){
    // Param: SSA value (check before function refs — params shadow function names)
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size()) {
      // If we already have a spill slot for this param (due to lval use like p++),
      // load from it to get the current (possibly modified) value.
      const auto spill_it = ctx.param_slots.find(*r.param_index + 0x80000000u);
      if (spill_it != ctx.param_slots.end()) {
        const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
        const std::string ty = llvm_ty(pts);
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, spill_it->second});
        return tmp;
      }
      // Otherwise use the SSA param value directly.
      const auto it = ctx.param_slots.find(*r.param_index);
      if (it != ctx.param_slots.end()) return it->second;
    }

    // Local: load
    if (r.local) {
      const auto it = ctx.local_slots.find(r.local->value);
      if (it == ctx.local_slots.end())
        throw std::runtime_error("StmtEmitter: local slot not found: " + r.name);
      const TypeSpec ts = ctx.local_types.at(r.local->value);
      const auto vit = ctx.local_is_vla.find(r.local->value);
      if (vit != ctx.local_is_vla.end() && vit->second) {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), it->second});
        return tmp;
      }
      if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
        // Treat va_list as array-like in expressions: it decays to a pointer to
        // the va_list object, so callers like vprintf(fmt, ap) receive ptr.
        return it->second;
      }
      if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(ts), it->second, false, {"i64 0", "i64 0"}});
        return tmp;
      }
      const std::string ty = llvm_ty(ts);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, it->second});
      return tmp;
    }

    // Global: load
    if (r.global) {
      size_t gv_idx = r.global->value;
      const auto& gv0 = mod_.globals[gv_idx];
      if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
      const auto& gv = mod_.globals[gv_idx];
      if (gv.type.spec.array_rank > 0 && !gv.type.spec.is_ptr_to_array) {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(gv.type.spec), llvm_global_sym(gv.name), false, {"i64 0", "i64 0"}});
        return tmp;
      }
      const std::string ty = llvm_ty(gv.type.spec);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, llvm_global_sym(gv.name)});
      return tmp;
    }

    // Function reference: return as ptr value (after checking locals/params)
    if (mod_.fn_index.count(r.name)) return llvm_global_sym(r.name);

    if ((r.name == "__func__" || r.name == "__FUNCTION__" || r.name == "__PRETTY_FUNCTION__") &&
        ctx.fn) {
      const std::string gname = intern_str(ctx.fn->name);
      const size_t len = ctx.fn->name.size() + 1;
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(len) + " x i8]", gname, false, {"i64 0", "i64 0"}});
      return tmp;
    }

    // Unresolved: load from external global
    const TypeSpec ets = resolve_expr_type(ctx, e);
    if (!has_concrete_type(ets)) return "0";
    const std::string ty = llvm_ty(ets);
    if (ty == "void") return "0";
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, "@" + r.name});
    return tmp;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e){
    // Short-circuit ops that use emit_lval (not emit_rval_id) to avoid
    // double-evaluating the operand and duplicating side effects (e.g. cnt++).
    if (u.op == UnaryOp::AddrOf) {
      TypeSpec pts{};
      try {
        return emit_lval(ctx, u.operand, pts);
      } catch (const std::runtime_error&) {
        TypeSpec obj_ts{};
        const std::string rval = emit_rval_id(ctx, u.operand, obj_ts);
        if (obj_ts.base != TB_STRUCT && obj_ts.base != TB_UNION) throw;
        const std::string slot = fresh_tmp(ctx) + ".addrtmp";
        ctx.alloca_insts.push_back(
            lir::LirAllocaOp{slot, llvm_alloca_ty(obj_ts), "", 0});
        emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(obj_ts), rval, slot});
        return slot;
      }
    }
    if (u.op == UnaryOp::PreInc || u.op == UnaryOp::PreDec ||
        u.op == UnaryOp::PostInc || u.op == UnaryOp::PostDec) {
      // Fall through to the switch below but skip the rval evaluation
      // that would duplicate side effects.
    } else {
      // Only evaluate operand as rval for ops that actually need it.
    }

    TypeSpec et = resolve_expr_type(ctx, e);
    TypeSpec op_ts{};
    // For pre/post inc/dec, the operand is an lvalue — avoid evaluating as rval
    // which would duplicate any side effects in the operand expression.
    const bool skip_rval = (u.op == UnaryOp::PreInc || u.op == UnaryOp::PreDec ||
                            u.op == UnaryOp::PostInc || u.op == UnaryOp::PostDec);
    const std::string val = skip_rval ? "" : emit_rval_id(ctx, u.operand, op_ts);
    if (skip_rval) op_ts = resolve_expr_type(ctx, u.operand);
    if (!has_concrete_type(et)) et = op_ts;
    const std::string ty = llvm_ty(et);
    const std::string op_ty = llvm_ty(op_ts);

    switch (u.op) {
      case UnaryOp::Plus: {
        // Integer promotion for small types
        if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
          const std::string ext = fresh_tmp(ctx);
          const bool is_signed = is_signed_int(op_ts.base);
          emit_lir_op(ctx, lir::LirCastOp{ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt, op_ty, val, ty});
          return ext;
        }
        return val;
      }

      case UnaryOp::Minus: {
        // Integer promotion for small types
        std::string promoted_val = val;
        std::string promoted_ty = op_ty;
        if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
          const std::string ext = fresh_tmp(ctx);
          const bool is_signed = is_signed_int(op_ts.base);
          emit_lir_op(ctx, lir::LirCastOp{ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt, op_ty, val, ty});
          promoted_val = ext;
          promoted_ty = ty;
        }
        const std::string tmp = fresh_tmp(ctx);
        if (is_vector_value(op_ts)) {
          emit_lir_op(ctx, lir::LirBinOp{tmp, "sub", ty, "zeroinitializer", val});
        } else if (is_float_base(op_ts.base)) {
          emit_lir_op(ctx, lir::LirBinOp{tmp, "fneg", ty, val, ""});
        } else {
          emit_lir_op(ctx, lir::LirBinOp{tmp, "sub", promoted_ty, "0", promoted_val});
        }
        return tmp;
      }

      case UnaryOp::Not: {
        const std::string cmp = to_bool(ctx, val, op_ts);
        // Invert i1
        const std::string inv = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{inv, "xor", "i1", cmp, "true"});
        if (ty == "i1") return inv;
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt, "i1", inv, ty});
        return tmp;
      }

      case UnaryOp::BitNot: {
        if (is_complex_base(op_ts.base)) {
          const TypeSpec elem_ts = complex_component_ts(op_ts.base);
          const std::string real_v = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirExtractValueOp{real_v, op_ty, val, 0});
          const std::string imag_v0 = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirExtractValueOp{imag_v0, op_ty, val, 1});
          const std::string imag_v = fresh_tmp(ctx);
          if (is_float_base(elem_ts.base)) {
            emit_lir_op(ctx, lir::LirBinOp{imag_v, "fneg", llvm_ty(elem_ts), imag_v0, ""});
          } else {
            emit_lir_op(ctx, lir::LirBinOp{imag_v, "sub", llvm_ty(elem_ts), "0", imag_v0});
          }
          const std::string elem_ty_str = llvm_ty(elem_ts);
          const std::string with_real = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirInsertValueOp{with_real, op_ty, "undef", elem_ty_str, real_v, 0});
          const std::string out = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirInsertValueOp{out, op_ty, with_real, elem_ty_str, imag_v, 1});
          return out;
        }
        const std::string tmp = fresh_tmp(ctx);
        if (is_vector_value(op_ts)) {
          TypeSpec elem_ts = op_ts;
          elem_ts.is_vector = false;
          elem_ts.vector_lanes = 0;
          elem_ts.vector_bytes = 0;
          std::string mask = "<";
          for (long long i = 0; i < op_ts.vector_lanes; ++i) {
            if (i) mask += ", ";
            mask += llvm_ty(elem_ts) + " -1";
          }
          mask += ">";
          emit_lir_op(ctx, lir::LirBinOp{tmp, "xor", ty, val, mask});
        } else {
          // Integer promotion: extend small types (i8, i16) to i32 before ~
          std::string promoted_val = val;
          std::string promoted_ty = op_ty;
          if (op_ty != ty && !is_vector_value(op_ts)) {
            const std::string ext = fresh_tmp(ctx);
            const bool is_signed = is_signed_int(op_ts.base);
            emit_lir_op(ctx, lir::LirCastOp{ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt, op_ty, val, ty});
            promoted_val = ext;
            promoted_ty = ty;
          }
          emit_lir_op(ctx, lir::LirBinOp{tmp, "xor", promoted_ty, promoted_val, "-1"});
        }
        return tmp;
      }

      case UnaryOp::AddrOf: {
        // Handled above before emit_rval_id to avoid side-effect duplication.
        __builtin_unreachable();
      }

      case UnaryOp::Deref: {
        TypeSpec load_ts = op_ts;
        if (load_ts.ptr_level > 0) {
          load_ts.ptr_level -= 1;
        } else if (load_ts.array_rank > 0) {
          // Decayed array: the value is already a pointer to the first element.
          // Dereference it by loading the element type.
          load_ts.array_rank--;
          load_ts.array_size = -1;
        }
        // Dereferencing a pointer-to-array yields an array lvalue. In rvalue
        // position it decays back to the same base address; loading a ptr here
        // would incorrectly treat the first elements as an address.
        if (load_ts.array_rank > 0) return val;
        // In C, dereferencing a function pointer (*fp) yields the function,
        // which immediately decays back to a function pointer — identity op.
        if (load_ts.is_fn_ptr && load_ts.ptr_level == 0) return val;
        const std::string load_ty = llvm_ty(load_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, load_ty, val});
        return tmp;
      }

      case UnaryOp::PreInc:
      case UnaryOp::PreDec: {
        const AssignableLValue lhs = emit_assignable_lval(ctx, u.operand);
        return emit_assignable_incdec_value(ctx, lhs, u.op == UnaryOp::PreInc, true);
      }

      case UnaryOp::PostInc:
      case UnaryOp::PostDec: {
        const AssignableLValue lhs = emit_assignable_lval(ctx, u.operand);
        return emit_assignable_incdec_value(ctx, lhs, u.op == UnaryOp::PostInc, false);
      }

      case UnaryOp::RealPart:
      case UnaryOp::ImagPart: {
        try {
          TypeSpec complex_ts{};
          const std::string complex_ptr = emit_lval(ctx, u.operand, complex_ts);
          if (!is_complex_base(complex_ts.base)) throw std::runtime_error("non-complex");
          const TypeSpec part_ts = complex_component_ts(complex_ts.base);
          const std::string part_ptr = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirGepOp{part_ptr, llvm_alloca_ty(complex_ts), complex_ptr, false,
                                          {"i32 0", "i32 " + std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0)}});
          const std::string tmp = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirLoadOp{tmp, llvm_ty(part_ts), part_ptr});
          return tmp;
        } catch (const std::runtime_error&) {
        }
        if (!is_complex_base(op_ts.base)) return "0";
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractValueOp{tmp, llvm_ty(op_ts), val,
                                              u.op == UnaryOp::ImagPart ? 1 : 0});
        return tmp;
      }
    }
    return "0";
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const BinaryExpr& b, const Expr& e){
    // Comma: eval both, return rhs
    if (b.op == BinaryOp::Comma) {
      TypeSpec lts{};
      emit_rval_id(ctx, b.lhs, lts);
      TypeSpec rts{};
      return emit_rval_id(ctx, b.rhs, rts);
    }
    // Short-circuit logical
    if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
      return emit_logical(ctx, b, e);
    }

    TypeSpec lts{};
    std::string lv = emit_rval_id(ctx, b.lhs, lts);
    TypeSpec rts{};
    std::string rv = emit_rval_id(ctx, b.rhs, rts);
    TypeSpec res_spec = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                            ? e.type.spec
                            : lts;

    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) &&
        llvm_ty(lts) == "ptr" && llvm_ty(rts) != "ptr") {
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      std::string idx = coerce(ctx, rv, rts, i64_ts);
      if (b.op == BinaryOp::Sub) {
        const std::string neg = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx});
        idx = neg;
      }
      return emit_indexed_gep(ctx, lv, lts, idx);
    }

    if ((b.op == BinaryOp::Eq || b.op == BinaryOp::Ne) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      TypeSpec cmp_lts = lts;
      TypeSpec cmp_rts = rts;
      std::string cmp_lv = lv;
      std::string cmp_rv = rv;
      const TypeSpec elem_ts = is_complex_base(cmp_lts.base)
                                   ? complex_component_ts(cmp_lts.base)
                                   : complex_component_ts(cmp_rts.base);
      const TypeSpec complex_ts = is_complex_base(cmp_lts.base) ? cmp_lts : cmp_rts;
      auto lift_scalar_to_complex = [&](const std::string& scalar,
                                        const TypeSpec& scalar_ts,
                                        bool scalar_is_lhs) -> std::string {
        std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
        const std::string imag_v = emit_const_int_like(0, elem_ts);
        const std::string with_real = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirInsertValueOp{with_real, llvm_ty(complex_ts), "undef", llvm_ty(elem_ts), real_v, 0});
        const std::string out = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirInsertValueOp{out, llvm_ty(complex_ts), with_real, llvm_ty(elem_ts), imag_v, 1});
        (void)scalar_is_lhs;
        return out;
      };
      if (!is_complex_base(cmp_lts.base)) {
        cmp_lv = lift_scalar_to_complex(cmp_lv, cmp_lts, true);
        cmp_lts = complex_ts;
      } else if (llvm_ty(cmp_lts) != llvm_ty(complex_ts)) {
        cmp_lv = coerce(ctx, cmp_lv, cmp_lts, complex_ts);
        cmp_lts = complex_ts;
      }
      if (!is_complex_base(cmp_rts.base)) {
        cmp_rv = lift_scalar_to_complex(cmp_rv, cmp_rts, false);
        cmp_rts = complex_ts;
      } else if (llvm_ty(cmp_rts) != llvm_ty(complex_ts)) {
        cmp_rv = coerce(ctx, cmp_rv, cmp_rts, complex_ts);
        cmp_rts = complex_ts;
      }
      const std::string lreal = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractValueOp{lreal, llvm_ty(cmp_lts), cmp_lv, 0});
      const std::string limag = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractValueOp{limag, llvm_ty(cmp_lts), cmp_lv, 1});
      const std::string rreal0 = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractValueOp{rreal0, llvm_ty(cmp_rts), cmp_rv, 0});
      const std::string rimag0 = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractValueOp{rimag0, llvm_ty(cmp_rts), cmp_rv, 1});
      const std::string rreal = coerce(ctx, rreal0, elem_ts, elem_ts);
      const std::string rimag = coerce(ctx, rimag0, elem_ts, elem_ts);
      const std::string creal = fresh_tmp(ctx);
      const std::string cimag = fresh_tmp(ctx);
      if (is_float_base(elem_ts.base)) {
        emit_lir_op(ctx, lir::LirCmpOp{creal, true, "oeq", llvm_ty(elem_ts), lreal, rreal});
        emit_lir_op(ctx, lir::LirCmpOp{cimag, true, "oeq", llvm_ty(elem_ts), limag, rimag});
      } else {
        emit_lir_op(ctx, lir::LirCmpOp{creal, false, "eq", llvm_ty(elem_ts), lreal, rreal});
        emit_lir_op(ctx, lir::LirCmpOp{cimag, false, "eq", llvm_ty(elem_ts), limag, rimag});
      }
      const std::string both = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{both, "and", "i1", creal, cimag});
      const std::string out = fresh_tmp(ctx);
      if (b.op == BinaryOp::Eq) {
        emit_lir_op(ctx, lir::LirCastOp{out, lir::LirCastKind::ZExt, "i1", both, "i32"});
      } else {
        const std::string inv = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{inv, "xor", "i1", both, "true"});
        emit_lir_op(ctx, lir::LirCastOp{out, lir::LirCastKind::ZExt, "i1", inv, "i32"});
      }
      return out;
    }

    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub ||
         b.op == BinaryOp::Mul || b.op == BinaryOp::Div) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      return emit_complex_binary_arith(ctx, b.op, lv, lts, rv, rts, res_spec);
    }

    if (is_vector_value(res_spec)) {
      // Scalar-to-vector splatting: when one operand is scalar and the other
      // is a vector, splat the scalar to match the vector type before the op.
      auto emit_splat_vec = [&](const std::string& scalar, const TypeSpec& scalar_ts,
                            const TypeSpec& vec_ts) -> std::string {
        TypeSpec elem_ts = vec_ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        std::string coerced = coerce(ctx, scalar, scalar_ts, elem_ts);
        const std::string elem_ty = llvm_ty(elem_ts);
        const std::string vec_ty_s = llvm_vector_ty(vec_ts);
        const int lanes = vec_ts.vector_lanes;
        const std::string ins = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirInsertElementOp{ins, vec_ty_s, "poison", elem_ty, coerced, "i64 0"});
        const std::string shuf = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirShuffleVectorOp{shuf, vec_ty_s, ins, "poison",
                         "<" + std::to_string(lanes) + " x i32>", "zeroinitializer"});
        return shuf;
      };
      if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
        rv = emit_splat_vec(rv, rts, lts);
        rts = lts;
      } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
        lv = emit_splat_vec(lv, lts, rts);
        lts = rts;
      }
      const std::string tmp = fresh_tmp(ctx);
      const std::string vec_ty = llvm_ty(res_spec);
      const bool vec_float = is_float_base(res_spec.base);
      switch (b.op) {
        case BinaryOp::Add:
          emit_lir_op(ctx, lir::LirBinOp{tmp, vec_float ? "fadd" : "add", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::Sub:
          emit_lir_op(ctx, lir::LirBinOp{tmp, vec_float ? "fsub" : "sub", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::Mul:
          emit_lir_op(ctx, lir::LirBinOp{tmp, vec_float ? "fmul" : "mul", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::Div:
          if (vec_float)
            emit_lir_op(ctx, lir::LirBinOp{tmp, "fdiv", vec_ty, lv, rv});
          else
            emit_lir_op(ctx, lir::LirBinOp{tmp, is_signed_int(res_spec.base) ? "sdiv" : "udiv", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::Mod:
          if (vec_float)
            emit_lir_op(ctx, lir::LirBinOp{tmp, "frem", vec_ty, lv, rv});
          else
            emit_lir_op(ctx, lir::LirBinOp{tmp, is_signed_int(res_spec.base) ? "srem" : "urem", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::BitAnd:
          emit_lir_op(ctx, lir::LirBinOp{tmp, "and", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::BitOr:
          emit_lir_op(ctx, lir::LirBinOp{tmp, "or", vec_ty, lv, rv});
          return tmp;
        case BinaryOp::BitXor:
          emit_lir_op(ctx, lir::LirBinOp{tmp, "xor", vec_ty, lv, rv});
          return tmp;
        default:
          break;
      }
    }

    // Apply C usual arithmetic conversions (UAC) for integer operands.
    // This promotes both operands to at least int, then to the common type.
    const bool l_is_int = is_any_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 &&
                          !is_vector_value(lts);
    const bool r_is_int = is_any_int(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 &&
                          !is_vector_value(rts);
    const bool l_is_flt = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 &&
                          !is_vector_value(lts);
    const bool r_is_flt = is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 &&
                          !is_vector_value(rts);
    if (l_is_int && r_is_int) {
      // For shift operations, the result type is the promoted LHS only;
      // the RHS (shift amount) doesn't participate in usual arithmetic conversions.
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      if (is_shift) {
        TypeBase lp = integer_promote(lts.base);
        TypeSpec lp_ts{}; lp_ts.base = lp;
        if (lts.base != lp) lv = coerce(ctx, lv, lts, lp_ts);
        lts = lp_ts;
        // Coerce RHS to match LHS width for LLVM (shift needs same-width operands)
        if (llvm_ty(rts) != llvm_ty(lts)) rv = coerce(ctx, rv, rts, lts);
        rts = lts;
      } else {
        TypeBase common = usual_arith_conv(lts.base, rts.base);
        TypeSpec common_ts{};
        common_ts.base = common;
        if (lts.base != common) lv = coerce(ctx, lv, lts, common_ts);
        lts = common_ts;
        if (rts.base != common) rv = coerce(ctx, rv, rts, common_ts);
        rts = common_ts;
      }
    } else if (l_is_int && r_is_flt) {
      lv = coerce(ctx, lv, lts, rts);
      lts = rts;
    } else if (l_is_flt && r_is_int) {
      rv = coerce(ctx, rv, rts, lts);
      rts = lts;
    }

    // After UAC, the result type should be at least as wide as the promoted
    // operand type.  The stored res_spec may reflect the pre-promotion type
    // from sema, so override it with the promoted lts for int arithmetic.
    if (l_is_int && r_is_int && is_any_int(res_spec.base) &&
        res_spec.ptr_level == 0 && res_spec.array_rank == 0) {
      res_spec = lts;
    }

    // Scalar-to-vector splatting: when one operand is scalar and the other
    // is a vector, splat the scalar to match the vector type.
    auto emit_splat = [&](const std::string& scalar, const TypeSpec& scalar_ts,
                          const TypeSpec& vec_ts) -> std::string {
      // First coerce the scalar to the vector element type
      TypeSpec elem_ts = vec_ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      std::string coerced = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string elem_ty = llvm_ty(elem_ts);
      const std::string vec_ty = llvm_vector_ty(vec_ts);
      const int lanes = vec_ts.vector_lanes;
      // insertelement + shufflevector to broadcast
      const std::string ins = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertElementOp{ins, vec_ty, "poison", elem_ty, coerced, "i64 0"});
      const std::string shuf = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirShuffleVectorOp{shuf, vec_ty, ins, "poison",
                       "<" + std::to_string(lanes) + " x i32>", "zeroinitializer"});
      return shuf;
    };
    if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
      rv = emit_splat(rv, rts, lts);
      rts = lts;
      res_spec = lts;  // update result type to vector
    } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
      lv = emit_splat(lv, lts, rts);
      lts = rts;
      res_spec = rts;  // update result type to vector
    }

    // If result type is unannotated (void), use the operand type
    const std::string res_ty = llvm_ty(res_spec);
    const std::string op_ty  = llvm_ty(lts);
    // lf/ls must be false when the LLVM type is ptr (even if base is double/float).
    const bool lf  = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool ls  = is_signed_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool arith_ls = ls;

    // Pointer arithmetic: ptr +/- int and int + ptr.
    if (b.op == BinaryOp::Add || b.op == BinaryOp::Sub) {
      auto emit_ptr_gep = [&](const std::string& base_ptr, const TypeSpec& base_ts,
                              const std::string& idx_val, const TypeSpec& idx_ts,
                              bool negate_idx) -> std::string {
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        std::string idx = coerce(ctx, idx_val, idx_ts, i64_ts);
        if (negate_idx) {
          const std::string neg = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx});
          idx = neg;
        }
        return emit_indexed_gep(ctx, base_ptr, base_ts, idx);
      };

      const bool rhs_is_nonptr_scalar =
          llvm_ty(rts) != "ptr" && !is_float_base(rts.base) && rts.array_rank == 0;
      const bool lhs_is_nonptr_scalar =
          llvm_ty(lts) != "ptr" && !is_float_base(lts.base) && lts.array_rank == 0;
      if (op_ty == "ptr" && rhs_is_nonptr_scalar) {
        return emit_ptr_gep(lv, lts, rv, rts, b.op == BinaryOp::Sub);
      }
      if (llvm_ty(rts) == "ptr" && lhs_is_nonptr_scalar && b.op == BinaryOp::Add) {
        return emit_ptr_gep(rv, rts, lv, lts, false);
      }
      if (b.op == BinaryOp::Sub && op_ty == "ptr" && llvm_ty(rts) == "ptr") {
        const std::string lhs_i = fresh_tmp(ctx);
        const std::string rhs_i = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCastOp{lhs_i, lir::LirCastKind::PtrToInt, "ptr", lv, "i64"});
        emit_lir_op(ctx, lir::LirCastOp{rhs_i, lir::LirCastKind::PtrToInt, "ptr", rv, "i64"});
        const std::string byte_diff = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{byte_diff, "sub", "i64", lhs_i, rhs_i});
        TypeSpec elem_ts = lts;
        if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
        // GCC extension: void* arithmetic is byte-granular (sizeof(void) == 1)
        const int elem_sz = (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0)
                                ? 1 : std::max(1, sizeof_ts(mod_, elem_ts));
        std::string diff = byte_diff;
        if (elem_sz != 1) {
          const std::string scaled = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirBinOp{scaled, "sdiv", "i64", byte_diff, std::to_string(elem_sz)});
          diff = scaled;
        }
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        if (res_spec.ptr_level > 0 || res_spec.array_rank > 0 || res_spec.base == TB_VOID) {
          return diff;
        }
        return coerce(ctx, diff, i64_ts, res_spec);
      }
    }

    rv = coerce(ctx, rv, rts, lts);

    // Arithmetic
    static const struct { BinaryOp op; const char* i_s; const char* i_u; const char* f; } arith[] = {
      { BinaryOp::Add, "add",  "add",  "fadd" },
      { BinaryOp::Sub, "sub",  "sub",  "fsub" },
      { BinaryOp::Mul, "mul",  "mul",  "fmul" },
      { BinaryOp::Div, "sdiv", "udiv", "fdiv" },
      { BinaryOp::Mod, "srem", "urem", nullptr },
      { BinaryOp::Shl, "shl",  "shl",  nullptr },
      { BinaryOp::Shr, "ashr", "lshr", nullptr },
      { BinaryOp::BitAnd, "and", "and", nullptr },
      { BinaryOp::BitOr,  "or",  "or",  nullptr },
      { BinaryOp::BitXor, "xor", "xor", nullptr },
    };
    for (const auto& row : arith) {
      if (row.op == b.op) {
        if ((row.op == BinaryOp::Add || row.op == BinaryOp::Sub) && op_ty == "ptr") {
          auto emit_ptr_fallback = [&](const std::string& base_ptr, const TypeSpec& base_ts,
                                       const std::string& idx_val, const TypeSpec& idx_ts,
                                       bool negate_idx) -> std::string {
            TypeSpec i64_ts{};
            i64_ts.base = TB_LONGLONG;
            std::string idx = coerce(ctx, idx_val, idx_ts, i64_ts);
            if (negate_idx) {
              const std::string neg = fresh_tmp(ctx);
              emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx});
              idx = neg;
            }
            return emit_indexed_gep(ctx, base_ptr, base_ts, idx);
          };
          return emit_ptr_fallback(lv, lts, rv, rts, row.op == BinaryOp::Sub);
        }
        const char* instr = nullptr;
        if (lf) {
          instr = row.f;
        } else if (row.op == BinaryOp::Shr) {
          instr = ls ? row.i_s : row.i_u;
        } else {
          instr = arith_ls ? row.i_s : row.i_u;
        }
        if (!instr) break;
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{tmp, std::string(instr), op_ty, lv, rv});
        return coerce(ctx, tmp, lts, res_spec);
      }
    }

    // Comparison
    static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } cmp[] = {
      { BinaryOp::Lt, "slt", "ult", "olt" },
      { BinaryOp::Le, "sle", "ule", "ole" },
      { BinaryOp::Gt, "sgt", "ugt", "ogt" },
      { BinaryOp::Ge, "sge", "uge", "oge" },
      { BinaryOp::Eq, "eq",  "eq",  "oeq" },
      { BinaryOp::Ne, "ne",  "ne",  "une" },
    };
    for (const auto& row : cmp) {
      if (row.op == b.op) {
        // C comparisons always yield int. Always zext i1 → i32 so the returned
        // value matches the TB_INT TypeSpec reported by resolve_payload_type.
        const std::string cmp_tmp = fresh_tmp(ctx);
        if (lf) {
          emit_lir_op(ctx, lir::LirCmpOp{cmp_tmp, true, std::string(row.f), op_ty, lv, rv});
        } else {
          // After UAC, both operands are at the common type.
          // Use the common type's signedness for the comparison predicate.
          const char* pred = ls ? row.is : row.iu;
          emit_lir_op(ctx, lir::LirCmpOp{cmp_tmp, false, std::string(pred), op_ty, lv, rv});
        }
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt, "i1", cmp_tmp, "i32"});
        return tmp;
      }
    }

    throw std::runtime_error("StmtEmitter: unhandled binary op");
  }

std::string StmtEmitter::emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e){
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);
    TypeSpec lts{};
    const std::string lv = emit_rval_id(ctx, b.lhs, lts);
    const std::string lc = to_bool(ctx, lv, lts);

    const std::string rhs_lbl  = fresh_lbl(ctx, "logic.rhs.");
    const std::string skip_lbl = fresh_lbl(ctx, "logic.skip.");
    const std::string rhs_end_lbl = fresh_lbl(ctx, "logic.rhs.end.");
    const std::string end_lbl  = fresh_lbl(ctx, "logic.end.");

    if (b.op == BinaryOp::LAnd) {
      emit_condbr_and_open_lbl(ctx, lc, rhs_lbl, skip_lbl, rhs_lbl);
    } else {
      emit_condbr_and_open_lbl(ctx, lc, skip_lbl, rhs_lbl, rhs_lbl);
    }
    TypeSpec rts{};
    const std::string rv = emit_rval_id(ctx, b.rhs, rts);
    const std::string rc = to_bool(ctx, rv, rts);
    std::string rhs_val;
    if (res_ty == "i1") {
      rhs_val = rc;
    } else if (is_float_base(res_spec.base)) {
      const std::string as_i32 = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{as_i32, lir::LirCastKind::ZExt, "i1", rc, "i32"});
      rhs_val = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{rhs_val, lir::LirCastKind::SIToFP, "i32", as_i32, res_ty});
    } else {
      rhs_val = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{rhs_val, lir::LirCastKind::ZExt, "i1", rc, res_ty});
    }
    emit_fallthrough_lbl(ctx, rhs_end_lbl);
    emit_br_and_open_lbl(ctx, end_lbl, skip_lbl);
    std::string skip_val;
    if (res_ty == "i1") {
      skip_val = (b.op == BinaryOp::LAnd) ? "false" : "true";
    } else if (is_float_base(res_spec.base)) {
      skip_val = (b.op == BinaryOp::LAnd) ? "0.0" : "1.0";
    } else {
      skip_val = (b.op == BinaryOp::LAnd) ? "0" : "1";
    }
    emit_fallthrough_lbl(ctx, end_lbl);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirPhiOp{tmp, res_ty, {{rhs_val, rhs_end_lbl}, {skip_val, skip_lbl}}});
    return tmp;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& /*e*/){
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, a.rhs, rhs_ts);

    const AssignableLValue lhs = emit_assignable_lval(ctx, a.lhs);
    if (a.op == AssignOp::Set) return emit_set_assign_value(ctx, lhs, rhs, rhs_ts);
    return emit_compound_assign_value(ctx, lhs, a.op, rhs, rhs_ts);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr& /*e*/){
    TypeSpec from_ts{};
    const std::string val = emit_rval_id(ctx, c.expr, from_ts);
    return coerce(ctx, val, from_ts, c.to_type.spec);
  }

CallTargetInfo StmtEmitter::resolve_call_target_info(FnCtx& ctx, const CallExpr& call,
                                                     const Expr& e) {
    CallTargetInfo info;
    info.builtin_id = call.builtin_id;
    info.builtin = builtin_by_id(info.builtin_id);

    const Expr& callee_e = get_expr(call.callee);
    bool unresolved_external_callee = false;
    std::string unresolved_external_name;
    if (info.builtin && info.builtin->category == BuiltinCategory::AliasCall &&
        !info.builtin->canonical_name.empty()) {
      info.callee_val = llvm_global_sym(std::string(info.builtin->canonical_name));
      unresolved_external_callee = true;
      unresolved_external_name = std::string(info.builtin->canonical_name);
    } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload);
               r && !r->local && !r->param_index && !r->global) {
      // Treat unresolved decl refs in call position as external functions.
      info.callee_val = llvm_global_sym(r->name);
      info.callee_ts = resolve_payload_type(ctx, *r);
      unresolved_external_callee = true;
      unresolved_external_name = r->name;
    } else {
      info.callee_val = emit_rval_id(ctx, call.callee, info.callee_ts);
    }

    info.ret_spec = resolve_payload_type(ctx, call);
    if (info.ret_spec.base == TB_VOID && info.ret_spec.ptr_level == 0 &&
        info.ret_spec.array_rank == 0) {
      info.ret_spec = e.type.spec;
    }
    info.ret_ty = llvm_ret_ty(info.ret_spec);
    info.builtin_special =
        info.builtin && info.builtin->lowering != BuiltinLoweringKind::AliasCall;
    if (unresolved_external_callee && !info.builtin_special) {
      record_extern_call_decl(unresolved_external_name, info.ret_ty);
    }

    if (info.builtin_id != BuiltinId::Unknown) {
      info.fn_name = std::string(builtin_name_from_id(info.builtin_id));
    } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      info.fn_name = r->name;
    }

    if (!info.fn_name.empty()) {
      const auto fit = mod_.fn_index.find(info.fn_name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        info.target_fn = &mod_.functions[fit->second.value];
      }
    }
    info.callee_fn_ptr_sig =
        info.target_fn ? nullptr : resolve_callee_fn_ptr_sig(ctx, callee_e);
    if (info.target_fn) {
      info.callee_type_suffix = llvm_fn_type_suffix_str(*info.target_fn);
    } else if (info.callee_fn_ptr_sig) {
      info.callee_type_suffix = llvm_fn_type_suffix_str(*info.callee_fn_ptr_sig);
    }

    return info;
  }

bool StmtEmitter::callee_needs_va_list_by_value_copy(const CallTargetInfo& call_target,
                                                     size_t arg_index) const {
    const Function* target_fn = call_target.target_fn;
    const FnPtrSig* callee_fn_ptr_sig = call_target.callee_fn_ptr_sig;
    if (target_fn) {
      if (arg_index < target_fn->params.size()) {
        const TypeSpec& param_ts = target_fn->params[arg_index].type.spec;
        return param_ts.base == TB_VA_LIST &&
               param_ts.ptr_level == 0 &&
               param_ts.array_rank == 0;
      }
      return false;
    }
    if (callee_fn_ptr_sig) {
      if (arg_index < sig_param_count(*callee_fn_ptr_sig)) {
        return sig_param_is_va_list_value(*callee_fn_ptr_sig, arg_index);
      }
      return false;
    }
    return call_target.fn_name == "vprintf"   || call_target.fn_name == "vfprintf" ||
           call_target.fn_name == "vsprintf"  || call_target.fn_name == "vsnprintf" ||
           call_target.fn_name == "vscanf"    || call_target.fn_name == "vfscanf" ||
           call_target.fn_name == "vsscanf"   || call_target.fn_name == "vasprintf" ||
           call_target.fn_name == "vdprintf";
  }

void StmtEmitter::apply_default_arg_promotion(FnCtx& ctx, std::string& arg,
                                              TypeSpec& out_ts,
                                              const TypeSpec& in_ts) {
    TypeSpec promoted = in_ts;
    if (promoted.array_rank > 0 && !promoted.is_ptr_to_array) {
      promoted.array_rank = 0;
      promoted.array_size = -1;
      promoted.ptr_level += 1;
    }
    if (promoted.ptr_level == 0 && promoted.array_rank == 0) {
      if (promoted.base == TB_FLOAT) {
        promoted.base = TB_DOUBLE;
      } else if (promoted.base == TB_BOOL || promoted.base == TB_CHAR ||
                 promoted.base == TB_SCHAR || promoted.base == TB_UCHAR ||
                 promoted.base == TB_SHORT || promoted.base == TB_USHORT) {
        promoted.base = TB_INT;
      }
    }
    arg = coerce(ctx, arg, in_ts, promoted);
    out_ts = promoted;
  }

PreparedCallArg StmtEmitter::prepare_call_arg(FnCtx& ctx, const CallExpr& call,
                                              const CallTargetInfo& call_target,
                                              size_t arg_index,
                                              Amd64CallArgState* amd64_state) {
    const Function* target_fn = call_target.target_fn;
    const FnPtrSig* callee_fn_ptr_sig = call_target.callee_fn_ptr_sig;
    const TypeSpec* fixed_param_ts = nullptr;
    TypeSpec fn_ptr_param_ts{};
    if (target_fn) {
      const bool has_void_param_list =
          target_fn->params.size() == 1 &&
          target_fn->params[0].type.spec.base == TB_VOID &&
          target_fn->params[0].type.spec.ptr_level == 0 &&
          target_fn->params[0].type.spec.array_rank == 0;
      if (!has_void_param_list && arg_index < target_fn->params.size()) {
        fixed_param_ts = &target_fn->params[arg_index].type.spec;
      }
    } else if (callee_fn_ptr_sig) {
      const bool has_void_pl = sig_has_void_param_list(*callee_fn_ptr_sig);
      if (!has_void_pl && arg_index < sig_param_count(*callee_fn_ptr_sig)) {
        fn_ptr_param_ts = sig_param_type(*callee_fn_ptr_sig, arg_index);
        fixed_param_ts = &fn_ptr_param_ts;
      }
    }

    TypeSpec arg_ts{};
    std::string arg;
    if (fixed_param_ts &&
        (fixed_param_ts->is_lvalue_ref || fixed_param_ts->is_rvalue_ref)) {
      try {
        TypeSpec pointee_ts{};
        arg = emit_lval(ctx, call.args[arg_index], pointee_ts);
        arg_ts = pointee_ts;
        arg_ts.ptr_level += 1;
        arg_ts.is_lvalue_ref = fixed_param_ts->is_lvalue_ref;
        arg_ts.is_rvalue_ref = fixed_param_ts->is_rvalue_ref;
      } catch (const std::runtime_error&) {
        arg = emit_rval_id(ctx, call.args[arg_index], arg_ts);
      }
    } else {
      arg = emit_rval_id(ctx, call.args[arg_index], arg_ts);
    }

    TypeSpec out_arg_ts = arg_ts;
    const bool is_va_list_value =
        arg_ts.base == TB_VA_LIST &&
        arg_ts.ptr_level == 0 &&
        arg_ts.array_rank == 0;
    const bool is_variadic_aggregate =
        target_fn && target_fn->attrs.variadic && arg_index >= target_fn->params.size() &&
        (arg_ts.base == TB_STRUCT || arg_ts.base == TB_UNION) &&
        arg_ts.ptr_level == 0 && arg_ts.array_rank == 0 &&
        arg_ts.tag && arg_ts.tag[0];
    if (target_fn) {
      const bool has_void_param_list =
          target_fn->params.size() == 1 &&
          target_fn->params[0].type.spec.base == TB_VOID &&
          target_fn->params[0].type.spec.ptr_level == 0 &&
          target_fn->params[0].type.spec.array_rank == 0;
      if (!has_void_param_list && arg_index < target_fn->params.size()) {
        out_arg_ts = target_fn->params[arg_index].type.spec;
        arg = coerce(ctx, arg, arg_ts, out_arg_ts);
      } else if (target_fn->attrs.variadic && !is_variadic_aggregate) {
        apply_default_arg_promotion(ctx, arg, out_arg_ts, arg_ts);
      }
    } else if (callee_fn_ptr_sig) {
      const bool has_void_pl = sig_has_void_param_list(*callee_fn_ptr_sig);
      if (!has_void_pl && arg_index < sig_param_count(*callee_fn_ptr_sig)) {
        out_arg_ts = sig_param_type(*callee_fn_ptr_sig, arg_index);
        arg = coerce(ctx, arg, arg_ts, out_arg_ts);
      } else if (sig_is_variadic(*callee_fn_ptr_sig) && !is_variadic_aggregate) {
        apply_default_arg_promotion(ctx, arg, out_arg_ts, arg_ts);
      }
    }
    if (is_va_list_value &&
        callee_needs_va_list_by_value_copy(call_target, arg_index)) {
      TypeSpec ap_ts{};
      const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[arg_index], ap_ts);
      const std::string tmp_addr = fresh_tmp(ctx);
      const int va_align = llvm_va_list_alignment(mod_.target_triple);
      ctx.alloca_insts.push_back(
          lir::LirAllocaOp{tmp_addr, llvm_va_list_storage_ty(), {}, va_align});
      module_->need_memcpy = true;
      emit_lir_op(ctx, lir::LirMemcpyOp{
          tmp_addr, src_ptr, std::to_string(llvm_va_list_storage_size()), false});
      arg = tmp_addr;
      out_arg_ts = arg_ts;
    }
    if (is_variadic_aggregate) {
      const auto sit = mod_.struct_defs.find(arg_ts.tag);
      const int payload_sz = sit == mod_.struct_defs.end() ? 0 : sit->second.size_bytes;
      if (payload_sz == 0) return {{}, true};

      std::string obj_ptr;
      if (get_expr(call.args[arg_index]).type.category == ValueCategory::LValue) {
        TypeSpec obj_ts{};
        obj_ptr = emit_lval(ctx, call.args[arg_index], obj_ts);
      } else {
        const std::string tmp_addr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, llvm_ty(arg_ts), {}, 0});
        emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(arg_ts), arg, tmp_addr});
        obj_ptr = tmp_addr;
      }

      if (llvm_target_is_amd64_sysv(mod_.target_triple)) {
        return prepare_amd64_variadic_aggregate_arg(ctx, arg_ts, obj_ptr,
                                                    payload_sz, amd64_state);
      }

      if (payload_sz > 16) {
        return {{std::string("ptr ") + obj_ptr}, false};
      }
      if (payload_sz <= 8) {
        const std::string tmp_addr = fresh_tmp(ctx);
        module_->need_memcpy = true;
        emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, "i64", {}, 0});
        emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
        const std::string packed = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{packed, std::string("i64"), tmp_addr});
        return {{std::string("i64 ") + packed}, false};
      }

      const std::string tmp_addr = fresh_tmp(ctx);
      module_->need_memcpy = true;
      emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, "[2 x i64]", {}, 0});
      emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
      const std::string packed = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{packed, std::string("[2 x i64]"), tmp_addr});
      return {{std::string("[2 x i64] ") + packed}, false};
    }

    PreparedCallArg out_arg{{llvm_ty(out_arg_ts) + " " + arg}, false};
    if (amd64_state && !out_arg.skip) {
      amd64_account_type_if_needed(mod_, out_arg_ts, amd64_state);
    }
    return out_arg;
  }

PreparedCallArg StmtEmitter::prepare_amd64_variadic_aggregate_arg(
    FnCtx& ctx, const TypeSpec& arg_ts, const std::string& obj_ptr,
    int payload_sz, Amd64CallArgState* amd64_state) {
    PreparedCallArg out;
    const auto layout = llvm_cc::classify_amd64_vararg(arg_ts, mod_);
    if (layout.size_bytes <= 0) {
      out.skip = true;
      return out;
    }
    bool force_memory = layout.needs_memory || layout.size_bytes > 16;
    if (!force_memory && amd64_state &&
        !amd64_registers_available(layout, *amd64_state)) {
      force_memory = true;
    }
    if (force_memory) {
      const int align = std::max(1, object_align_bytes(mod_, arg_ts));
      std::string arg = "ptr ";
      arg += "byval(" + llvm_ty(arg_ts) + ") align " + std::to_string(align) + " " + obj_ptr;
      out.texts.push_back(std::move(arg));
      return out;
    }

    const auto make_byte_ptr = [&](const std::string& base, int offset) {
      if (offset == 0) {
        const std::string zero_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{zero_ptr, "i8", base, false, {"i64 0"}});
        return zero_ptr;
      }
      const std::string ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{
          ptr, "i8", base, false, {"i64 " + std::to_string(offset)}});
      return ptr;
    };

    const auto copy_chunk = [&](const std::string& llvm_ty,
                                const std::string& zero_value,
                                const std::string& src_ptr,
                                int copy_bytes) {
      const std::string tmp_addr = fresh_tmp(ctx);
      ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, llvm_ty, "", 0});
      emit_lir_op(ctx, lir::LirStoreOp{llvm_ty, zero_value, tmp_addr});
      module_->need_memcpy = true;
      emit_lir_op(ctx, lir::LirMemcpyOp{
          tmp_addr, src_ptr, std::to_string(copy_bytes), false});
      const std::string loaded = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{loaded, llvm_ty, tmp_addr});
      return loaded;
    };

    const std::string base_ptr = make_byte_ptr(obj_ptr, 0);
    for (size_t chunk = 0; chunk < layout.classes.size(); ++chunk) {
      const int chunk_offset = static_cast<int>(chunk) * 8;
      if (chunk_offset >= payload_sz) break;
      const int chunk_size = std::min(8, payload_sz - chunk_offset);
      const auto cls = layout.classes[chunk];
      if (cls == llvm_cc::Amd64ArgClass::None) continue;
      std::string chunk_ptr = base_ptr;
      if (chunk_offset > 0) {
        chunk_ptr = make_byte_ptr(base_ptr, chunk_offset);
      }
      if (cls == llvm_cc::Amd64ArgClass::Integer) {
        const std::string loaded = copy_chunk("i64", "0", chunk_ptr, chunk_size);
        out.texts.push_back(std::string("i64 ") + loaded);
        continue;
      }
      if (cls == llvm_cc::Amd64ArgClass::SSE) {
        const bool combine =
            (chunk + 1 < layout.classes.size() &&
             layout.classes[chunk + 1] == llvm_cc::Amd64ArgClass::SSEUp &&
             chunk_offset + 8 < payload_sz);
        if (combine) {
          const int combined_size = std::min(16, payload_sz - chunk_offset);
          const std::string loaded =
              copy_chunk("<2 x i64>", "zeroinitializer", chunk_ptr, combined_size);
          out.texts.push_back(std::string("<2 x i64> ") + loaded);
          ++chunk;
          continue;
        }
        const std::string loaded =
            copy_chunk("double", "0.0", chunk_ptr, chunk_size);
        out.texts.push_back(std::string("double ") + loaded);
        continue;
      }
      if (cls == llvm_cc::Amd64ArgClass::SSEUp) {
        const std::string loaded =
            copy_chunk("double", "0.0", chunk_ptr, chunk_size);
        out.texts.push_back(std::string("double ") + loaded);
        continue;
      }
      out.texts.clear();
      out.texts.push_back(std::string("ptr ") + obj_ptr);
      return out;
    }
    if (amd64_state) {
      amd64_track_usage(layout, *amd64_state);
    }
    return out;
  }

std::string StmtEmitter::prepare_call_args(FnCtx& ctx, const CallExpr& call,
                                           const CallTargetInfo& call_target) {
    std::string args_str;
    Amd64CallArgState amd64_state;
    amd64_state.sse_bytes = kAmd64GpAreaBytes;
    Amd64CallArgState* amd64_state_ptr = nullptr;
    if (llvm_target_is_amd64_sysv(mod_.target_triple)) {
      amd64_state_ptr = &amd64_state;
    }
    for (size_t i = 0; i < call.args.size(); ++i) {
      PreparedCallArg prepared =
          prepare_call_arg(ctx, call, call_target, i, amd64_state_ptr);
      if (prepared.skip) continue;
      for (const std::string& part : prepared.texts) {
        if (part.empty()) continue;
        if (!args_str.empty()) args_str += ", ";
        args_str += part;
      }
    }
    return args_str;
  }

void StmtEmitter::emit_void_call(FnCtx& ctx, const CallTargetInfo& call_target,
                                 const std::string& args_str) {
    emit_lir_op(
        ctx, lir::LirCallOp{"", "void", call_target.callee_val,
                            call_target.callee_type_suffix, args_str});
  }

std::string StmtEmitter::emit_call_with_result(FnCtx& ctx, const CallTargetInfo& call_target,
                                               const std::string& args_str) {
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, call_target.ret_ty, call_target.callee_val,
                                    call_target.callee_type_suffix, args_str});
    return tmp;
  }

PreparedBuiltinIntArg StmtEmitter::prepare_builtin_int_arg(FnCtx& ctx, ExprId arg_id,
                                                           BuiltinId builtin_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    const bool is_i64 = builtin_uses_i64_width(builtin_id);
    TypeSpec target_ts{};
    target_ts.base = is_i64 ? TB_LONGLONG : TB_INT;
    arg = coerce(ctx, arg, arg_ts, target_ts);
    return {arg, is_i64 ? "i64" : "i32", is_i64};
  }

std::string StmtEmitter::narrow_builtin_int_result(FnCtx& ctx,
                                                   const PreparedBuiltinIntArg& arg,
                                                   const std::string& value) {
    if (!arg.is_i64) return value;
    const std::string trunc = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{trunc, lir::LirCastKind::Trunc, arg.llvm_ty, value, "i32"});
    return trunc;
  }

std::string StmtEmitter::emit_builtin_ffs_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const std::string cttz = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{cttz, arg.llvm_ty, "@llvm.cttz." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + arg.value + ", i1 false"});
    const std::string plus1 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{plus1, "add", arg.llvm_ty, cttz, "1"});
    const std::string is_zero = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{is_zero, false, "eq", arg.llvm_ty, arg.value, "0"});
    const std::string sel = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirSelectOp{sel, arg.llvm_ty, is_zero, "0", plus1});
    return narrow_builtin_int_result(ctx, arg, sel);
  }

std::string StmtEmitter::emit_builtin_ctz_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, arg.llvm_ty, "@llvm.cttz." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + arg.value + ", i1 true"});
    return narrow_builtin_int_result(ctx, arg, tmp);
  }

std::string StmtEmitter::emit_builtin_clz_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, arg.llvm_ty, "@llvm.ctlz." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + arg.value + ", i1 true"});
    return narrow_builtin_int_result(ctx, arg, tmp);
  }

std::string StmtEmitter::emit_builtin_popcount_call(FnCtx& ctx, ExprId arg_id,
                                                    BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, arg.llvm_ty, "@llvm.ctpop." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + arg.value});
    return narrow_builtin_int_result(ctx, arg, tmp);
  }

std::string StmtEmitter::emit_builtin_parity_call(FnCtx& ctx, ExprId arg_id,
                                                  BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const std::string pop = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{pop, arg.llvm_ty, "@llvm.ctpop." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + arg.value});
    const std::string parity = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{parity, "and", arg.llvm_ty, pop, "1"});
    return narrow_builtin_int_result(ctx, arg, parity);
  }

std::string StmtEmitter::emit_builtin_clrsb_call(FnCtx& ctx, ExprId arg_id,
                                                 BuiltinId builtin_id) {
    const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
    const int bitwidth = arg.is_i64 ? 64 : 32;
    const std::string shift = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{shift, "ashr", arg.llvm_ty, arg.value,
                                   std::to_string(bitwidth - 1)});
    const std::string xored = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{xored, "xor", arg.llvm_ty, arg.value, shift});
    const std::string clz = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{clz, arg.llvm_ty, "@llvm.ctlz." + arg.llvm_ty, "",
                                    arg.llvm_ty + " " + xored + ", i1 false"});
    const std::string sub1 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{sub1, "sub", arg.llvm_ty, clz, "1"});
    return narrow_builtin_int_result(ctx, arg, sub1);
  }

void StmtEmitter::promote_builtin_fp_predicate_arg(FnCtx& ctx, std::string& value,
                                                   TypeSpec& value_ts) {
    if (value_ts.base != TB_FLOAT || value_ts.ptr_level != 0 || value_ts.array_rank != 0) {
      return;
    }
    const std::string promoted = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{promoted, lir::LirCastKind::FPExt, "float", value, "double"});
    value = promoted;
    value_ts.base = TB_DOUBLE;
  }

std::string StmtEmitter::emit_builtin_fp_predicate_result(FnCtx& ctx, const char* predicate,
                                                          const std::string& fp_ty,
                                                          const std::string& lhs,
                                                          const std::string& rhs) {
    const std::string cmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{cmp, true, std::string(predicate), fp_ty, lhs, rhs});
    const std::string widened = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{widened, lir::LirCastKind::ZExt, "i1", cmp, "i32"});
    return widened;
  }

std::string StmtEmitter::emit_builtin_fp_compare_call(FnCtx& ctx, const CallExpr& call,
                                                      BuiltinId builtin_id) {
    TypeSpec lhs_ts{}, rhs_ts{};
    std::string lhs = emit_rval_id(ctx, call.args[0], lhs_ts);
    std::string rhs = emit_rval_id(ctx, call.args[1], rhs_ts);
    promote_builtin_fp_predicate_arg(ctx, lhs, lhs_ts);
    promote_builtin_fp_predicate_arg(ctx, rhs, rhs_ts);
    return emit_builtin_fp_predicate_result(ctx, builtin_fp_compare_predicate(builtin_id),
                                            llvm_ty(lhs_ts), lhs, rhs);
  }

std::string StmtEmitter::emit_builtin_isunordered_call(FnCtx& ctx, const CallExpr& call) {
    TypeSpec lhs_ts{}, rhs_ts{};
    std::string lhs = emit_rval_id(ctx, call.args[0], lhs_ts);
    std::string rhs = emit_rval_id(ctx, call.args[1], rhs_ts);
    promote_builtin_fp_predicate_arg(ctx, lhs, lhs_ts);
    promote_builtin_fp_predicate_arg(ctx, rhs, rhs_ts);
    return emit_builtin_fp_predicate_result(ctx, "uno", llvm_ty(lhs_ts), lhs, rhs);
  }

std::string StmtEmitter::emit_builtin_isnan_call(FnCtx& ctx, ExprId arg_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    promote_builtin_fp_predicate_arg(ctx, arg, arg_ts);
    return emit_builtin_fp_predicate_result(ctx, "uno", llvm_ty(arg_ts), arg, arg);
  }

std::string StmtEmitter::emit_builtin_isinf_call(FnCtx& ctx, ExprId arg_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    promote_builtin_fp_predicate_arg(ctx, arg, arg_ts);
    const std::string fp_ty = llvm_ty(arg_ts);
    const std::string inf_val = fp_literal(arg_ts.base, std::numeric_limits<double>::infinity());
    const std::string abs_value = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{abs_value, fp_ty, "@llvm.fabs." + fp_ty, "",
                                    fp_ty + " " + arg});
    return emit_builtin_fp_predicate_result(ctx, "oeq", fp_ty, abs_value, inf_val);
  }

std::string StmtEmitter::emit_builtin_isfinite_call(FnCtx& ctx, ExprId arg_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    promote_builtin_fp_predicate_arg(ctx, arg, arg_ts);
    const std::string fp_ty = llvm_ty(arg_ts);
    const std::string inf_val = fp_literal(arg_ts.base, std::numeric_limits<double>::infinity());
    const std::string abs_value = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{abs_value, fp_ty, "@llvm.fabs." + fp_ty, "",
                                    fp_ty + " " + arg});
    return emit_builtin_fp_predicate_result(ctx, "olt", fp_ty, abs_value, inf_val);
  }

void StmtEmitter::promote_builtin_fp_math_arg(FnCtx& ctx, std::string& value,
                                              TypeSpec& value_ts, BuiltinId builtin_id) {
    if (value_ts.ptr_level != 0 || value_ts.array_rank != 0) {
      return;
    }
    const auto cast_fp_arg = [&](const std::string& src_ty, const std::string& dst_ty,
                                 lir::LirCastKind kind, TypeBase dst_base) {
      const std::string promoted = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{promoted, kind, src_ty, value, dst_ty});
      value = promoted;
      value_ts.base = dst_base;
    };

    const std::string long_double_ty =
        llvm_helpers::llvm_long_double_ty(mod_.target_triple);

    switch (builtin_id) {
      case BuiltinId::Copysign:
      case BuiltinId::Fabs:
        if (value_ts.base == TB_FLOAT) {
          cast_fp_arg("float", "double", lir::LirCastKind::FPExt, TB_DOUBLE);
        }
        break;
      case BuiltinId::CopysignF:
      case BuiltinId::FabsF:
        if (value_ts.base == TB_DOUBLE) {
          cast_fp_arg("double", "float", lir::LirCastKind::FPTrunc, TB_FLOAT);
        }
        break;
      case BuiltinId::CopysignL:
      case BuiltinId::FabsL:
        if (value_ts.base == TB_FLOAT) {
          cast_fp_arg("float", long_double_ty, lir::LirCastKind::FPExt,
                      TB_LONGDOUBLE);
        } else if (value_ts.base == TB_DOUBLE) {
          cast_fp_arg("double", long_double_ty, lir::LirCastKind::FPExt,
                      TB_LONGDOUBLE);
        }
        break;
      default:
        break;
    }
  }

std::string StmtEmitter::emit_builtin_copysign_call(FnCtx& ctx, const CallExpr& call,
                                                    BuiltinId builtin_id) {
    TypeSpec lhs_ts{}, rhs_ts{};
    std::string lhs = emit_rval_id(ctx, call.args[0], lhs_ts);
    std::string rhs = emit_rval_id(ctx, call.args[1], rhs_ts);
    promote_builtin_fp_math_arg(ctx, lhs, lhs_ts, builtin_id);
    promote_builtin_fp_math_arg(ctx, rhs, rhs_ts, builtin_id);

    const std::string fp_ty = llvm_ty(lhs_ts);
    std::string intrinsic = "@llvm.copysign.f64";
    if (builtin_id == BuiltinId::CopysignF) {
      intrinsic = "@llvm.copysign.f32";
    } else if (builtin_id == BuiltinId::CopysignL) {
      intrinsic = "@llvm.copysign.f128";
    }

    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, fp_ty, intrinsic, "",
                                    fp_ty + " " + lhs + ", " + fp_ty + " " + rhs});
    return tmp;
  }

std::string StmtEmitter::emit_builtin_fabs_call(FnCtx& ctx, ExprId arg_id,
                                                BuiltinId builtin_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    promote_builtin_fp_math_arg(ctx, arg, arg_ts, builtin_id);

    const std::string fp_ty = llvm_ty(arg_ts);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCallOp{tmp, fp_ty, "@llvm.fabs." + fp_ty, "",
                                    fp_ty + " " + arg});
    return tmp;
  }

std::string StmtEmitter::emit_builtin_conj_call(FnCtx& ctx, ExprId arg_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    if (!is_complex_base(arg_ts.base)) return arg;

    const TypeSpec elem_ts = complex_component_ts(arg_ts.base);
    const std::string complex_ty = llvm_ty(arg_ts);
    const std::string elem_ty = llvm_ty(elem_ts);
    const std::string real_v = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{real_v, complex_ty, arg, 0});
    const std::string imag_v0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirExtractValueOp{imag_v0, complex_ty, arg, 1});
    const std::string imag_v = fresh_tmp(ctx);
    if (is_float_base(elem_ts.base)) {
      emit_lir_op(ctx, lir::LirBinOp{imag_v, "fneg", elem_ty, imag_v0, ""});
    } else {
      emit_lir_op(ctx, lir::LirBinOp{imag_v, "sub", elem_ty, "0", imag_v0});
    }
    const std::string with_real = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{with_real, complex_ty, "undef", elem_ty, real_v, 0});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{out, complex_ty, with_real, elem_ty, imag_v, 1});
    return out;
  }

void StmtEmitter::promote_builtin_signbit_arg(FnCtx& ctx, std::string& value,
                                              TypeSpec& value_ts, BuiltinId builtin_id) {
    if (builtin_id != BuiltinId::SignBit) return;
    promote_builtin_fp_predicate_arg(ctx, value, value_ts);
  }

std::string StmtEmitter::emit_builtin_signbit_call(FnCtx& ctx, ExprId arg_id,
                                                   BuiltinId builtin_id) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
    promote_builtin_signbit_arg(ctx, arg, arg_ts, builtin_id);

    const std::string fp_ty = llvm_ty(arg_ts);
    std::string int_ty = "i64";
    int shift_bits = 63;
    if (arg_ts.base == TB_FLOAT) {
      int_ty = "i32";
      shift_bits = 31;
    } else if (arg_ts.base == TB_LONGDOUBLE) {
      int_ty = "i128";
      shift_bits = 127;
    }

    const std::string bc = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{bc, lir::LirCastKind::Bitcast, fp_ty, arg, int_ty});
    const std::string shifted = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{shifted, "lshr", int_ty, bc, std::to_string(shift_bits)});
    const std::string trunc = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{trunc, lir::LirCastKind::Trunc, int_ty, shifted, "i32"});
    return trunc;
  }

std::string StmtEmitter::emit_post_builtin_call(FnCtx& ctx, const CallExpr& call,
                                                const CallTargetInfo& call_target) {
    const BuiltinId builtin_id = call_target.builtin_id;

    // Implicit builtins: standard library functions recognized by GCC/Clang
    // even without __builtin_ prefix. abs/labs/llabs -> llvm.abs intrinsic.
    if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 &&
        (call_target.fn_name == "abs" || call_target.fn_name == "labs" ||
         call_target.fn_name == "llabs")) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
      const bool is_ll =
          (call_target.fn_name == "llabs" || call_target.fn_name == "labs");
      const std::string ity = is_ll ? "i64" : "i32";
      TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
      arg = coerce(ctx, arg, arg_ts, target_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAbsOp{tmp, arg, ity});
      module_->need_abs = true;
      return tmp;
    }

    // Implicit builtin: alloca() -> LLVM alloca instruction (like __builtin_alloca).
    if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 &&
        call_target.fn_name == "alloca") {
      TypeSpec size_ts{};
      std::string size = emit_rval_id(ctx, call.args[0], size_ts);
      TypeSpec i64_ts{}; i64_ts.base = TB_ULONGLONG;
      size = coerce(ctx, size, size_ts, i64_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAllocaOp{tmp, "i8", size, 16});
      return tmp;
    }

    const std::string args_str = prepare_call_args(ctx, call, call_target);
    if (call_target.ret_ty == "void") {
      emit_void_call(ctx, call_target, args_str);
      return "";
    }
    return emit_call_with_result(ctx, call_target, args_str);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e){
    const CallTargetInfo call_target = resolve_call_target_info(ctx, call, e);
    const BuiltinId builtin_id = call_target.builtin_id;

    if (builtin_id == BuiltinId::Unknown && has_builtin_prefix(call_target.fn_name)) {
      throw std::runtime_error("StmtEmitter: unsupported builtin call: " + call_target.fn_name);
    }

    // Handle GCC/Clang builtins
    if (call_target.builtin_special) {
      if (builtin_id == BuiltinId::Memcpy && call.args.size() >= 3) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        TypeSpec size_ts{};
        const std::string dst = emit_rval_id(ctx, call.args[0], dst_ts);
        const std::string src = emit_rval_id(ctx, call.args[1], src_ts);
        std::string size = emit_rval_id(ctx, call.args[2], size_ts);
        TypeSpec i64_ts{};
        i64_ts.base = TB_ULONGLONG;
        size = coerce(ctx, size, size_ts, i64_ts);
        module_->need_memcpy = true;
        emit_lir_op(ctx, lir::LirMemcpyOp{dst, src, size, false});
        return dst;
      }
      if (builtin_id == BuiltinId::VaStart && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        module_->need_va_start = true;
        emit_lir_op(ctx, lir::LirVaStartOp{ap_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::VaEnd && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        module_->need_va_end = true;
        emit_lir_op(ctx, lir::LirVaEndOp{ap_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::VaCopy && call.args.size() >= 2) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        const std::string dst_ptr = emit_va_list_obj_ptr(ctx, call.args[0], dst_ts);
        const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[1], src_ts);
        module_->need_va_copy = true;
        emit_lir_op(ctx, lir::LirVaCopyOp{dst_ptr, src_ptr});
        return "";
      }
      if (builtin_id == BuiltinId::Alloca && call.args.size() == 1) {
        TypeSpec size_ts{};
        std::string size = emit_rval_id(ctx, call.args[0], size_ts);
        TypeSpec i64_ts{}; i64_ts.base = TB_ULONGLONG;
        size = coerce(ctx, size, size_ts, i64_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirAllocaOp{tmp, "i8", size, 16});
        return tmp;
      }
      if (builtin_id == BuiltinId::ConstantP) {
        // At -O0, __builtin_constant_p always returns 0
        // Still need to emit args for side effects
        for (auto& a : call.args) {
          TypeSpec dummy{};
          emit_rval_id(ctx, a, dummy);
        }
        return "0";
      }
      if (builtin_id == BuiltinId::ClassifyType && call.args.size() == 1) {
        // GCC __builtin_classify_type: return integer type class
        // We need the type of the argument, not its value
        TypeSpec arg_ts{};
        emit_rval_id(ctx, call.args[0], arg_ts);
        // GCC type classes:
        // 0=void, 1=integer, 2=char, 3=enum(=integer), 5=pointer,
        // 8=real, 9=complex, 12=record, 13=union, 14=array, 15=string(=pointer)
        int type_class = 5; // default: pointer (for unknown)
        if (arg_ts.ptr_level > 0 || arg_ts.is_fn_ptr) {
          type_class = 5; // pointer
        } else if (is_complex_base(arg_ts.base)) {
          type_class = 9; // complex
        } else if (is_float_base(arg_ts.base)) {
          type_class = 8; // real
        } else if (arg_ts.base == TB_VOID) {
          type_class = 0; // void
        } else if (arg_ts.base == TB_BOOL || arg_ts.base == TB_CHAR ||
                   arg_ts.base == TB_SCHAR || arg_ts.base == TB_UCHAR) {
          type_class = 1; // integer (char is integer class in GCC)
        } else if (is_any_int(arg_ts.base)) {
          type_class = 1; // integer
        } else if (arg_ts.base == TB_STRUCT) {
          type_class = 12; // record
        } else if (arg_ts.base == TB_UNION) {
          type_class = 13; // union
        }
        return std::to_string(type_class);
      }
      if ((builtin_id == BuiltinId::AddOverflow ||
           builtin_id == BuiltinId::SubOverflow ||
           builtin_id == BuiltinId::MulOverflow) && call.args.size() == 3) {
        TypeSpec a_ts{}, b_ts{}, p_ts{};
        std::string a = emit_rval_id(ctx, call.args[0], a_ts);
        std::string b = emit_rval_id(ctx, call.args[1], b_ts);
        const std::string result_ptr = emit_rval_id(ctx, call.args[2], p_ts);
        // Determine the result type from the pointer target type
        TypeSpec res_ts = p_ts;
        res_ts.ptr_level--;
        const std::string res_ty = llvm_ty(res_ts);
        // Coerce operands to result type
        a = coerce(ctx, a, a_ts, res_ts);
        b = coerce(ctx, b, b_ts, res_ts);
        // Choose signed or unsigned intrinsic based on result type
        const bool is_signed = is_signed_int(res_ts.base);
        std::string op;
        if (builtin_id == BuiltinId::AddOverflow) op = is_signed ? "sadd" : "uadd";
        else if (builtin_id == BuiltinId::SubOverflow) op = is_signed ? "ssub" : "usub";
        else op = is_signed ? "smul" : "umul";
        const std::string intrinsic = "@llvm." + op + ".with.overflow." + res_ty;
        const std::string pair = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCallOp{pair, "{ " + res_ty + ", i1 }", intrinsic, "",
                        res_ty + " " + a + ", " + res_ty + " " + b});
        const std::string ovf_agg_ty = "{ " + res_ty + ", i1 }";
        const std::string val = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractValueOp{val, ovf_agg_ty, pair, 0});
        const std::string ovf = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractValueOp{ovf, ovf_agg_ty, pair, 1});
        emit_lir_op(ctx, lir::LirStoreOp{res_ty, val, result_ptr});
        const std::string ret = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCastOp{ret, lir::LirCastKind::ZExt, "i1", ovf, "i32"});
        return ret;
      }
      if (builtin_id == BuiltinId::Bswap16 || builtin_id == BuiltinId::Bswap32 ||
          builtin_id == BuiltinId::Bswap64) {
        if (call.args.size() == 1) {
          TypeSpec arg_ts{};
          const std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
          const std::string aty = llvm_ty(arg_ts);
          const std::string intrinsic = "@llvm.bswap." + aty;
          const std::string tmp = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirCallOp{tmp, aty, intrinsic, "", aty + " " + arg});
          return tmp;
        }
      }
      if (builtin_id == BuiltinId::Expect && call.args.size() >= 1) {
        TypeSpec arg_ts{};
        std::string result = emit_rval_id(ctx, call.args[0], arg_ts);
        // Evaluate remaining args for side effects (e.g., z++)
        for (size_t i = 1; i < call.args.size(); ++i) {
          TypeSpec dummy{};
          emit_rval_id(ctx, call.args[i], dummy);
        }
        return result;
      }
      if (builtin_id == BuiltinId::Unreachable && call.args.empty()) {
        emit_term_unreachable(ctx);
        return "";
      }
      if (builtin_id == BuiltinId::Prefetch) {
        // __builtin_prefetch is a no-op hint; evaluate args for side effects only
        TypeSpec dummy{};
        for (size_t i = 0; i < call.args.size(); ++i) {
          emit_rval_id(ctx, call.args[i], dummy);
        }
        return "";
      }
      switch (builtin_constant_fp_kind(builtin_id)) {
        case BuiltinConstantFpKind::Infinity:
          if (builtin_id == BuiltinId::HugeValF || builtin_id == BuiltinId::InfF) {
            return fp_to_float_literal(std::numeric_limits<float>::infinity());
          }
          if (auto base = fp_base_from_builtin_result_kind(
                  builtin_result_kind(builtin_id))) {
            return fp_literal(*base, std::numeric_limits<double>::infinity());
          }
          break;
        case BuiltinConstantFpKind::QuietNaN:
          if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
            return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
          }
          if (auto base = fp_base_from_builtin_result_kind(
                  builtin_result_kind(builtin_id))) {
            return fp_literal(*base, std::numeric_limits<double>::quiet_NaN());
          }
          break;
        case BuiltinConstantFpKind::None:
          break;
      }
      if ((builtin_id == BuiltinId::SignBit ||
           builtin_id == BuiltinId::SignBitF ||
           builtin_id == BuiltinId::SignBitL) && call.args.size() == 1) {
        return emit_builtin_signbit_call(ctx, call.args[0], builtin_id);
      }
      if (const char* pred = builtin_fp_compare_predicate(builtin_id);
          pred && call.args.size() == 2) {
        return emit_builtin_fp_compare_call(ctx, call, builtin_id);
      }
      if (builtin_id == BuiltinId::IsUnordered && call.args.size() == 2) {
        return emit_builtin_isunordered_call(ctx, call);
      }
      if (builtin_is_isnan(builtin_id) && call.args.size() == 1) {
        return emit_builtin_isnan_call(ctx, call.args[0]);
      }
      if (builtin_is_isinf(builtin_id) && call.args.size() == 1) {
        return emit_builtin_isinf_call(ctx, call.args[0]);
      }
      if (builtin_is_isfinite(builtin_id) && call.args.size() == 1) {
        return emit_builtin_isfinite_call(ctx, call.args[0]);
      }
      if ((builtin_id == BuiltinId::Copysign || builtin_id == BuiltinId::CopysignL ||
           builtin_id == BuiltinId::CopysignF) && call.args.size() == 2) {
        return emit_builtin_copysign_call(ctx, call, builtin_id);
      }
      if ((builtin_id == BuiltinId::Fabs || builtin_id == BuiltinId::FabsL ||
           builtin_id == BuiltinId::FabsF) && call.args.size() == 1) {
        return emit_builtin_fabs_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_ffs(builtin_id) && call.args.size() == 1) {
        return emit_builtin_ffs_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_ctz(builtin_id) && call.args.size() == 1) {
        return emit_builtin_ctz_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_clz(builtin_id) && call.args.size() == 1) {
        return emit_builtin_clz_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_popcount(builtin_id) && call.args.size() == 1) {
        return emit_builtin_popcount_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_parity(builtin_id) && call.args.size() == 1) {
        return emit_builtin_parity_call(ctx, call.args[0], builtin_id);
      }
      if (builtin_is_clrsb(builtin_id) && call.args.size() == 1) {
        return emit_builtin_clrsb_call(ctx, call.args[0], builtin_id);
      }
      if ((builtin_id == BuiltinId::Conj || builtin_id == BuiltinId::ConjF ||
           builtin_id == BuiltinId::ConjL) && call.args.size() == 1) {
        return emit_builtin_conj_call(ctx, call.args[0]);
      }
    }
    return emit_post_builtin_call(ctx, call, call_target);
  }

std::string StmtEmitter::emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr, int slot_bytes){
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{offs_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 3"}});
    const std::string offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, "sge", "i32", offs, "0"});
    emit_condbr_and_open_lbl(ctx, is_stack0, stack_lbl, reg_try_lbl, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(slot_bytes)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
    const std::string use_reg = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, "sle", "i32", next_offs, "0"});
    emit_condbr_and_open_lbl(ctx, use_reg, reg_lbl, stack_lbl, reg_lbl);
    const std::string gr_top_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{gr_top_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 1"}});
    const std::string gr_top = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{gr_top, std::string("ptr"), gr_top_ptr});
    const std::string reg_addr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", gr_top, false, {"i32 " + offs}});
    emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 0"}});
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
    const std::string stack_next = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", stack_ptr, false, {"i64 " + std::to_string(slot_bytes)}});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
    emit_fallthrough_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirPhiOp{src_ptr, "ptr", {{reg_addr, reg_lbl}, {stack_ptr, stack_lbl}}});
    return src_ptr;
  }

std::string StmtEmitter::emit_aarch64_vaarg_fp_src_ptr(
      FnCtx& ctx, const std::string& ap_ptr, int reg_slot_bytes, int stack_slot_bytes,
      int stack_align_bytes){
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{offs_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 4"}});
    const std::string offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.fp.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.fp.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.fp.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.fp.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, "sge", "i32", offs, "0"});
    emit_condbr_and_open_lbl(ctx, is_stack0, stack_lbl, reg_try_lbl, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(reg_slot_bytes)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
    const std::string use_reg = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, "sle", "i32", next_offs, "0"});
    emit_condbr_and_open_lbl(ctx, use_reg, reg_lbl, stack_lbl, reg_lbl);
    const std::string vr_top_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{vr_top_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 2"}});
    const std::string vr_top = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{vr_top, std::string("ptr"), vr_top_ptr});
    const std::string reg_addr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", vr_top, false, {"i32 " + offs}});
    emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, "%struct.__va_list_tag_", ap_ptr, false, {"i32 0", "i32 0"}});
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
    std::string aligned_stack_ptr = stack_ptr;
    if (stack_align_bytes > 1) {
      const std::string stack_i = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{stack_i, lir::LirCastKind::PtrToInt, "ptr", stack_ptr, "i64"});
      const std::string plus_mask = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{plus_mask, "add", "i64", stack_i, std::to_string(stack_align_bytes - 1)});
      const std::string masked = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{masked, "and", "i64", plus_mask, std::to_string(-stack_align_bytes)});
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{aligned_stack_ptr, lir::LirCastKind::IntToPtr, "i64", masked, "ptr"});
    }
    const std::string stack_next = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", aligned_stack_ptr, false, {"i64 " + std::to_string(stack_slot_bytes)}});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
    emit_fallthrough_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirPhiOp{src_ptr, "ptr", {{reg_addr, reg_lbl}, {aligned_stack_ptr, stack_lbl}}});
    return src_ptr;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e){
    TypeSpec ap_ts{};
    const std::string ap_ptr = emit_va_list_obj_ptr(ctx, v.ap, ap_ts);
    TypeSpec res_ts = e.type.spec;
    if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
    const std::string res_ty = llvm_ty(res_ts);
    if (res_ty == "void") return "";
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const HirStructDef& sd = it->second;
        int payload_sz = 0;
        if (sd.is_union) {
          for (const auto& f : sd.fields) {
            payload_sz = std::max(payload_sz, f.size_bytes);
          }
        } else {
          payload_sz = sd.size_bytes;
        }
        // GCC zero-size aggregate extension: va_arg yields a zero-initialized value
        // and does not consume any argument slot.
        if (payload_sz == 0) return "zeroinitializer";
      }
    }
    if (llvm_va_list_is_pointer_object(mod_.target_triple)) {
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
      return out;
    }
    if (llvm_target_is_amd64_sysv(mod_.target_triple)) {
      return emit_amd64_va_arg(ctx, res_ts, res_ty, ap_ptr);
    }
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const int payload_sz = it->second.size_bytes;
        if (payload_sz == 0) return "zeroinitializer";
        if (payload_sz > 0) {
          if (payload_sz > 16) {
            const std::string slot_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
            const std::string src_ptr = fresh_tmp(ctx);
            emit_lir_op(ctx, lir::LirLoadOp{src_ptr, std::string("ptr"), slot_ptr});
            const std::string tmp_addr = fresh_tmp(ctx);
            emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, res_ty, {}, 0});
            module_->need_memcpy = true;
            emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
            const std::string out = fresh_tmp(ctx);
            emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
            return out;
          }

          const int slot_bytes = payload_sz > 8 ? 16 : 8;
          const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, slot_bytes);
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, res_ty, {}, 0});
          module_->need_memcpy = true;
          emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
          const std::string out = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
          return out;
        }
      }
    }

    const bool is_gp_scalar =
        (res_ty == "ptr") ||
        (res_ts.ptr_level == 0 && res_ts.array_rank == 0 && is_any_int(res_ts.base));
    const bool is_fp_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        (res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE);
    const bool is_fp128_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 && res_ts.base == TB_LONGDOUBLE;

    // AArch64 va_list matches clang's __va_list_tag layout:
    // { stack, gr_top, vr_top, gr_offs, vr_offs }.
    // For integer/pointer args we first try GR save area, then fall back to stack.
    if (is_gp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
      return out;
    }
    if (is_fp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 8, 8);
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
      return out;
    }
    if (is_fp128_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 16, 16);
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
      return out;
    }

    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
    return out;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e){
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, t.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);

    const std::string then_lbl     = fresh_lbl(ctx, "tern.then.");
    const std::string then_end_lbl = fresh_lbl(ctx, "tern.then.end.");
    const std::string else_lbl     = fresh_lbl(ctx, "tern.else.");
    const std::string else_end_lbl = fresh_lbl(ctx, "tern.else.end.");
    const std::string end_lbl      = fresh_lbl(ctx, "tern.end.");
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);

    emit_condbr_and_open_lbl(ctx, cond_i1, then_lbl, else_lbl, then_lbl);
    TypeSpec then_ts{};
    std::string then_v = emit_rval_id(ctx, t.then_expr, then_ts);
    then_v = coerce(ctx, then_v, then_ts, res_spec);
    emit_fallthrough_lbl(ctx, then_end_lbl);
    emit_br_and_open_lbl(ctx, end_lbl, else_lbl);
    TypeSpec else_ts{};
    std::string else_v = emit_rval_id(ctx, t.else_expr, else_ts);
    else_v = coerce(ctx, else_v, else_ts, res_spec);
    emit_fallthrough_lbl(ctx, else_end_lbl);
    emit_fallthrough_lbl(ctx, end_lbl);
    if (res_ty == "void") return "";
    // If either arm produced a void/empty value (e.g. void function call in one arm),
    // substitute a zero value to avoid an invalid empty phi operand.
    auto void_to_zero = [&](const std::string& v) -> std::string {
      if (!v.empty()) return v;
      if (res_ty == "ptr") return "null";
      if (res_ty == "float" || res_ty == "double") return "0.0";
      return "0";
    };
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirPhiOp{tmp, res_ty, {{void_to_zero(then_v), then_end_lbl}, {void_to_zero(else_v), else_end_lbl}}});
    return tmp;
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&){
    const Expr& op = get_expr(s.expr);
    TypeSpec op_ts = resolve_expr_type(ctx, op);
    if (!has_concrete_type(op_ts)) {
      op_ts = std::visit([&](const auto& p) -> TypeSpec {
        return resolve_payload_type(ctx, p);
      }, op.payload);
    }
    // String literals: sizeof("abc") = 4 (array size, not pointer size)
    if (const auto* sl = std::get_if<StringLiteral>(&op.payload)) {
      if (sl->is_wide) {
        const auto vals = decode_wide_string_values(sl->raw);
        return std::to_string(vals.size() * 4);  // vals includes null terminator
      }
      const auto bytes = bytes_from_string_literal(*sl);
      return std::to_string(bytes.size() + 1);  // +1 for null terminator
    }
    // DeclRef: get type from global/local declaration (NK_VAR nodes have no type set).
    if (const auto* r = std::get_if<DeclRef>(&op.payload)) {
      auto resolve_named_global_object_type = [&](const std::string& name) -> std::optional<TypeSpec> {
        if (const GlobalVar* best = select_global_object(name)) return best->type.spec;
        return std::nullopt;
      };
      if (r->global) {
        if (const GlobalVar* gv = select_global_object(*r->global)) op_ts = gv->type.spec;
      } else if (r->local) {
        const auto it = ctx.local_types.find(r->local->value);
        if (it != ctx.local_types.end()) op_ts = it->second;
      } else if (r->param_index && ctx.fn &&
                 *r->param_index < ctx.fn->params.size()) {
        op_ts = ctx.fn->params[*r->param_index].type.spec;
        // Array params decay to pointer in C — sizeof(param) = sizeof(void*)
        if (op_ts.array_rank > 0) {
          op_ts.array_rank = 0; op_ts.array_size = -1; op_ts.ptr_level = 1;
        }
      } else if (!r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
      if (op_ts.array_rank == 0 && !r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
    }
    if (!has_concrete_type(op_ts)) return "8";
    return std::to_string(sizeof_ts(mod_, op_ts));
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr&){
    return std::to_string(sizeof_ts(mod_, s.type.spec));
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr&){
    return "blockaddress(@" + ctx.fn->name + ", %ulbl_" + la.label_name + ")";
  }

std::string StmtEmitter::emit_rval_payload(FnCtx&, const PendingConstevalExpr& p, const Expr&){
    throw std::runtime_error(
        "StmtEmitter: unevaluated PendingConstevalExpr reached codegen for '" + p.fn_name + "'");
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e){
    if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
      const TypeSpec base_ts = resolve_expr_type(ctx, idx->base);
      if (is_vector_value(base_ts)) {
        TypeSpec vec_ts{};
        const std::string vec = emit_rval_id(ctx, idx->base, vec_ts);
        TypeSpec ix_ts{};
        std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
        TypeSpec i32_ts{};
        i32_ts.base = TB_INT;
        ix = coerce(ctx, ix, ix_ts, i32_ts);
        TypeSpec elem_ts = base_ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractElementOp{tmp, llvm_ty(base_ts), vec, "i32", ix});
        return tmp;
      }
    }
    TypeSpec pts{};
    const std::string ptr = emit_lval_dispatch(ctx, e, pts);
    return emit_rval_from_access_expr(ctx, e, ptr, pts, false);
  }

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e){
    TypeSpec field_ts{};
    BitfieldAccess bf;
    const std::string gep = emit_member_lval(ctx, m, field_ts, &bf);
    // Bitfield: use shift/mask load
    if (bf.is_bitfield()) {
      return emit_bitfield_load(ctx, gep, bf);
    }
    return emit_rval_from_access_expr(ctx, e, gep, field_ts, true);
  }

void StmtEmitter::emit_stmt(FnCtx& ctx, const Stmt& stmt){
    std::visit([&](const auto& s) { emit_stmt_impl(ctx, s); }, stmt.payload);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const LocalDecl& d){
    emit_non_control_flow_stmt(ctx, d);
  }

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const LocalDecl& d){
    if (d.fn_ptr_sig) {
      ctx.local_fn_ptr_sigs[d.id.value] = *d.fn_ptr_sig;
    }
    if (d.vla_size) {
      const std::string slot = ctx.local_slots.at(d.id.value);
      TypeSpec sz_ts{};
      std::string count = emit_rval_id(ctx, *d.vla_size, sz_ts);
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      count = coerce(ctx, count, sz_ts, i64_ts);

      long long static_mult = 1;
      for (int i = 1; i < d.type.spec.array_rank; ++i) {
        const long long dim = d.type.spec.array_dims[i];
        if (dim > 0) static_mult *= dim;
      }
      if (static_mult > 1) {
        const std::string scaled = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{scaled, "mul", "i64", count, std::to_string(static_mult)});
        count = scaled;
      }

      TypeSpec elem_ts = d.type.spec;
      elem_ts.array_rank = 0;
      elem_ts.array_size = -1;
      for (int i = 0; i < 8; ++i) elem_ts.array_dims[i] = -1;
      std::string elem_ty = llvm_ty(elem_ts);
      if (elem_ty == "void") elem_ty = "i8";
      const std::string dyn_ptr = fresh_tmp(ctx);
      const int stack_align = object_align_bytes(mod_, d.type.spec);
      emit_lir_op(ctx, lir::LirAllocaOp{dyn_ptr, elem_ty, count, stack_align > 1 ? stack_align : 0});
      emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), dyn_ptr, slot});
    }

    if (!d.init) return;
    const std::string slot = ctx.local_slots.at(d.id.value);
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, *d.init, rhs_ts);
    const std::string ty =
        (d.type.spec.array_rank > 0) ? llvm_alloca_ty(d.type.spec) : llvm_ty(d.type.spec);
    const bool is_agg_or_array =
        d.type.spec.array_rank > 0 ||
        (d.type.spec.is_vector && d.type.spec.vector_lanes > 0) ||
        (d.type.spec.ptr_level == 0 &&
         (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
         d.type.spec.array_rank == 0);
    if (is_agg_or_array && (rhs == "0" || rhs.empty())) {
      emit_lir_op(ctx, lir::LirStoreOp{ty, std::string("zeroinitializer"), slot});
      return;
    }
    rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
    emit_lir_op(ctx, lir::LirStoreOp{ty, rhs, slot});
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ExprStmt& s){
    emit_non_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const ExprStmt& s){
    if (!s.expr) return;
    TypeSpec ts{};
    emit_rval_id(ctx, *s.expr, ts);
  }

// Rewrite GCC inline asm constraints that LLVM aarch64 backend doesn't support.
// 'g' (general operand) → 'imr' (immediate, memory, register).
static std::string rewrite_asm_constraints(const std::string& raw) {
    std::string result;
    result.reserve(raw.size() + 8);
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '~' && i + 1 < raw.size() && raw[i + 1] == '{') {
            // skip clobber spec ~{...} verbatim
            auto end = raw.find('}', i);
            if (end != std::string::npos) {
                result.append(raw, i, end - i + 1);
                i = end;
                continue;
            }
        }
        if (raw[i] == 'g') {
            // Ensure 'g' is a standalone constraint letter, not part of a clobber name.
            // In the constraint string, each comma-separated segment is a constraint spec.
            // 'g' should be replaced with 'imr'.
            result += "imr";
        } else {
            result += raw[i];
        }
    }
    return result;
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const InlineAsmStmt& s){
    emit_non_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const InlineAsmStmt& s){
    const std::string asm_text = escape_llvm_c_bytes(s.asm_template);
    const std::string constraints = rewrite_asm_constraints(escape_llvm_c_bytes(s.constraints));
    TypeSpec ret_ts{};
    std::string ret_ty = "void";
    if (s.output) {
      ret_ts = resolve_expr_type(ctx, *s.output);
      ret_ty = llvm_ty(ret_ts);
    }
    std::vector<std::string> arg_vals;
    std::vector<TypeSpec> arg_tys;
    for (ExprId input : s.inputs) {
      TypeSpec in_ts{};
      arg_vals.push_back(emit_rval_id(ctx, input, in_ts));
      arg_tys.push_back(in_ts);
    }
    std::string asm_args_str;
    for (size_t i = 0; i < arg_vals.size(); ++i) {
      if (i) asm_args_str += ", ";
      asm_args_str += llvm_ty(arg_tys[i]) + " " + arg_vals[i];
    }
    if (!s.output) {
      emit_lir_op(ctx, lir::LirInlineAsmOp{{}, ret_ty, asm_text, constraints, s.has_side_effects, asm_args_str});
      return;
    }

    const std::string result = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInlineAsmOp{result, ret_ty, asm_text, constraints, s.has_side_effects, asm_args_str});
    TypeSpec out_pointee_ts{};
    const std::string out_ptr = emit_lval(ctx, *s.output, out_pointee_ts);
    const std::string coerced = coerce(ctx, result, ret_ts, out_pointee_ts);
    emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(out_pointee_ts), coerced, out_ptr});
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ReturnStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ReturnStmt& s){
    if (!s.expr) {
      const auto& rts = ctx.fn->return_type.spec;
      if (rts.base == TB_VOID && rts.ptr_level == 0 && rts.array_rank == 0 &&
          !rts.is_lvalue_ref && !rts.is_rvalue_ref) {
        emit_term_ret(ctx, "void", std::nullopt);
      } else {
        // C89: bare 'return;' in non-void function → return 0/null/0.0
        const std::string ret_ty = llvm_ret_ty(rts);
        if (ret_ty == "ptr") {
          emit_term_ret(ctx, "ptr", "null");
        } else if (is_float_base(rts.base) && rts.ptr_level == 0) {
          emit_term_ret(ctx, ret_ty, "0.0");
        } else {
          emit_term_ret(ctx, ret_ty, "0");
        }
      }
      return;
    }
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, *s.expr, ts);
    // For reference returns, the LLVM return type is ptr, so coerce to ptr.
    TypeSpec coerce_target = ctx.fn->return_type.spec;
    if ((coerce_target.is_lvalue_ref || coerce_target.is_rvalue_ref) && coerce_target.ptr_level == 0) {
      coerce_target.ptr_level++;
    }
    val = coerce(ctx, val, ts, coerce_target);
    emit_term_ret(ctx, llvm_ret_ty(ctx.fn->return_type.spec), val);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const IfStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const IfStmt& s){
    TypeSpec cond_ts{};
    const std::string cond_v  = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    const std::string then_lbl = block_lbl(s.then_block);
    const std::string after_lbl = block_lbl(s.after_block);
    if (s.else_block) {
      emit_term_condbr(ctx, cond_i1, then_lbl, block_lbl(*s.else_block));
    } else {
      emit_term_condbr(ctx, cond_i1, then_lbl, after_lbl);
    }
    // Store after_block in meta for then/else blocks (so they fall through)
    ctx.block_meta[s.then_block.value].break_label = std::nullopt;  // not a loop
    if (s.else_block)
      ctx.block_meta[s.else_block->value].break_label = std::nullopt;
    // after_block label is needed as fallthrough for then/else
    // after_block label is the fallthrough target for then/else
    (void)after_lbl;
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const WhileStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const WhileStmt& s){
    const std::string cond_lbl = s.continue_target
        ? block_lbl(*s.continue_target)
        : fresh_lbl(ctx, "while.cond.");
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "while.end.");

    ctx.continue_redirect[s.body_block.value] = cond_lbl;

    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term_condbr(ctx, cond_i1, body_lbl, end_lbl);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ForStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ForStmt& s){
    if (s.init) {
      TypeSpec ts{};
      emit_rval_id(ctx, *s.init, ts);
    }
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "for.end.");
    const std::string cond_lbl = "for.cond." + std::to_string(s.body_block.value);
    const std::string latch_lbl = "for.latch." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = latch_lbl;

    emit_fallthrough_lbl(ctx, cond_lbl);
    if (s.cond) {
      TypeSpec cts{};
      std::string cv = emit_rval_id(ctx, *s.cond, cts);
      cv = to_bool(ctx, cv, cts);
      emit_condbr_and_open_sibling_lbl(ctx, cv, body_lbl, end_lbl, latch_lbl);
    } else {
      emit_br_and_open_lbl(ctx, body_lbl, latch_lbl);
    }
    if (s.update) {
      TypeSpec uts{};
      (void)emit_rval_id(ctx, *s.update, uts);
    }
    emit_term_br(ctx, cond_lbl);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const DoWhileStmt& s){
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "dowhile.end.");
    const std::string cond_lbl = "dowhile.cond." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = cond_lbl;
    emit_fallthrough_lbl(ctx, cond_lbl);
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term_condbr(ctx, cond_i1, body_lbl, end_lbl);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const SwitchStmt& s){
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, s.cond, ts);
    // C requires integer promotion on the controlling expression.
    if (ts.ptr_level == 0 && ts.array_rank == 0 && is_any_int(ts.base)) {
      TypeBase promoted = integer_promote(ts.base);
      if (promoted != ts.base) {
        TypeSpec promoted_ts{}; promoted_ts.base = promoted;
        val = coerce(ctx, val, ts, promoted_ts);
        ts = promoted_ts;
      }
    }
    const std::string ty  = llvm_ty(ts);

    // Default label: use explicit default block if present, else the break (after-switch) block
    const std::string default_lbl = s.default_block
        ? block_lbl(*s.default_block)
        : (s.break_block ? block_lbl(*s.break_block) : fresh_lbl(ctx, "sw.end."));
    const std::string break_lbl = s.break_block
        ? block_lbl(*s.break_block)
        : default_lbl;

    ctx.block_meta[s.body_block.value].break_label = break_lbl;

    // Collect case values by scanning body block statements
    // All cases jump to body_block (they share the body, case markers are sequential)
    const Block* body_blk = nullptr;
    for (const auto& blk : ctx.fn->blocks) {
      if (blk.id.value == s.body_block.value) { body_blk = &blk; break; }
    }

    const std::string body_lbl = block_lbl(s.body_block);

    // Case ranges need comparison chains before the switch.
    // For each range [lo, hi], emit: if (val >= lo && val <= hi) goto case_block
    // This is done before the switch instruction so the switch only handles discrete cases.
    if (!s.case_range_blocks.empty()) {
      for (const auto& [lo, hi, bid] : s.case_range_blocks) {
        const char* pred_ge = is_signed_int(ts.base) ? "sge" : "uge";
        const char* pred_le = is_signed_int(ts.base) ? "sle" : "ule";
        const std::string t_ge = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCmpOp{t_ge, false, pred_ge, ty, val, std::to_string(lo)});
        const std::string t_le = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirCmpOp{t_le, false, pred_le, ty, val, std::to_string(hi)});
        const std::string t_and = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{t_and, "and", "i1", t_ge, t_le});
        const std::string next_lbl = fresh_lbl(ctx, "sw.range.next.");
        emit_condbr_and_fallthrough_lbl(ctx, t_and, block_lbl(bid), next_lbl);
      }
    }

    std::vector<std::pair<long long, std::string>> sw_cases;
    if (!s.case_blocks.empty()) {
      for (const auto& [case_val, case_bid] : s.case_blocks) {
        sw_cases.emplace_back(case_val, block_lbl(case_bid));
      }
    } else if (body_blk) {
      for (const auto& stmt : body_blk->stmts) {
        if (const auto* cs = std::get_if<CaseStmt>(&stmt.payload)) {
          sw_cases.emplace_back(cs->value, body_lbl);
        } else if (std::get_if<CaseRangeStmt>(&stmt.payload)) {
          // Handled by range chains above
        }
      }
    }
    emit_term_switch(ctx, val, ty, default_lbl, std::move(sw_cases));
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const GotoStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const GotoStmt& s){
    if (ctx.vla_stack_save_ptr && s.target.resolved_block.valid() &&
        s.target.resolved_block.value <= ctx.current_block_id) {
      module_->need_stackrestore = true;
      emit_lir_op(ctx, lir::LirStackRestoreOp{*ctx.vla_stack_save_ptr});
    }
    if (s.target.resolved_block.valid()) {
      emit_term_br(ctx, block_lbl(s.target.resolved_block));
    } else {
      emit_term_br(ctx, "ulbl_" + s.target.user_name);
    }
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const IndirBrStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const IndirBrStmt& s){
    // Collect all user labels in this function as possible targets.
    std::vector<std::string> targets;
    for (const auto& bb : ctx.fn->blocks) {
      for (const auto& stmt : bb.stmts) {
        if (const auto* ls = std::get_if<LabelStmt>(&stmt.payload)) {
          targets.push_back("%ulbl_" + ls->name);
        }
      }
    }
    TypeSpec dummy_ts{};
    const std::string val = emit_rval_id(ctx, s.target, dummy_ts);
    if (!ctx.last_term) {
      ctx.cur_block().insts.push_back(
          lir::LirIndirectBrOp{val, std::move(targets)});
      ctx.last_term = true;
    }
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const LabelStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const LabelStmt& s){
    emit_fallthrough_lbl(ctx, "ulbl_" + s.name);
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const BreakStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const BreakStmt& s){
    if (s.target) emit_term_br(ctx, block_lbl(*s.target));
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ContinueStmt& s){
    emit_control_flow_stmt(ctx, s);
  }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ContinueStmt& s){
    if (!s.target) return;
    const auto it = ctx.continue_redirect.find(s.target->value);
    if (it != ctx.continue_redirect.end()) {
      emit_term_br(ctx, it->second);
      return;
    }
    emit_term_br(ctx, block_lbl(*s.target));
  }

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseStmt& s){
    emit_switch_label_stmt(ctx, s);
  }

void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseStmt&){}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseRangeStmt& s){
    emit_switch_label_stmt(ctx, s);
  }

void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseRangeStmt&){}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const DefaultStmt& s){
    emit_switch_label_stmt(ctx, s);
  }

void StmtEmitter::emit_switch_label_stmt(FnCtx&, const DefaultStmt&){}


}  // namespace c4c::codegen::lir
