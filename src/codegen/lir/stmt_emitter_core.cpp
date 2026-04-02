#include "stmt_emitter.hpp"

namespace c4c::codegen::lir {

// Draft-only staging file for Step 3 of the stmt_emitter split refactor.
// The monolith remains the live implementation until Step 4 build wiring.

StmtEmitter::StmtEmitter(const Module& m) : mod_(m) {}

void StmtEmitter::set_module(lir::LirModule& module) { module_ = &module; }

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

void StmtEmitter::record_extern_call_decl(const std::string& name, const std::string& ret_ty) {
  if (name.empty() || mod_.fn_index.count(name)) return;
  module_->record_extern_decl(name, ret_ty);
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
    const int fb = int_bits(from_ts.base);
    const int tb = int_bits(to_ts.base);
    if (fb == tb) return val;
    const std::string tmp = fresh_tmp(ctx);
    if (tb > fb) {
      const auto kind =
          is_signed_int(from_ts.base) ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt;
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
        is_signed_int(from_ts.base) ? lir::LirCastKind::SIToFP : lir::LirCastKind::UIToFP;
    emit_lir_op(ctx, lir::LirCastOp{tmp, kind, ft, val, tt});
    return tmp;
  }

  if (is_float_base(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
      is_any_int(to_ts.base) && to_ts.ptr_level == 0) {
    const std::string tmp = fresh_tmp(ctx);
    const auto kind =
        is_signed_int(to_ts.base) ? lir::LirCastKind::FPToSI : lir::LirCastKind::FPToUI;
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
