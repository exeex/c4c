#include "lowering.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

// Draft-only staging file for Step 2 of the stmt_emitter split refactor.
// The monolith remains the live implementation until build wiring begins.

// Rewrite GCC inline asm constraints that LLVM aarch64 backend doesn't support.
// 'g' (general operand) -> 'imr' (immediate, memory, register).
static std::string rewrite_asm_constraints(const std::string& raw) {
  std::string result;
  result.reserve(raw.size() + 8);
  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '~' && i + 1 < raw.size() && raw[i + 1] == '{') {
      auto end = raw.find('}', i);
      if (end != std::string::npos) {
        result.append(raw, i, end - i + 1);
        i = end;
        continue;
      }
    }
    if (raw[i] == 'g') {
      result += "imr";
    } else {
      result += raw[i];
    }
  }
  return result;
}

static std::vector<std::string> split_asm_constraints(const std::string& constraints) {
  std::vector<std::string> out;
  std::string cur;
  int brace_depth = 0;
  for (char ch : constraints) {
    if (ch == '{') ++brace_depth;
    if (ch == '}' && brace_depth > 0) --brace_depth;
    if (ch == ',' && brace_depth == 0) {
      out.push_back(cur);
      cur.clear();
      continue;
    }
    cur += ch;
  }
  if (!cur.empty() || !constraints.empty()) out.push_back(cur);
  return out;
}

static std::string join_asm_constraints(const std::vector<std::string>& constraints) {
  std::string out;
  for (const std::string& constraint : constraints) {
    if (!out.empty()) out += ",";
    out += constraint;
  }
  return out;
}

static bool asm_constraint_is_memory_operand(const std::string& constraint) {
  return constraint.find('m') != std::string::npos;
}

static std::string llvm_output_constraint(std::string constraint) {
  if (!constraint.empty() && constraint[0] == '+') constraint[0] = '=';
  if (!constraint.empty() && constraint[0] != '=') constraint.insert(constraint.begin(), '=');
  if (asm_constraint_is_memory_operand(constraint) &&
      constraint.find('*') == std::string::npos) {
    constraint.insert(1, "*");
  }
  return constraint;
}

static std::string llvm_memory_input_constraint(std::string constraint) {
  if (!constraint.empty() && (constraint[0] == '+' || constraint[0] == '=')) {
    constraint.erase(constraint.begin());
  }
  if (asm_constraint_is_memory_operand(constraint) &&
      constraint.find('*') == std::string::npos) {
    constraint.insert(constraint.begin(), '*');
  }
  return constraint;
}

static bool is_asm_word_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.';
}

static bool matches_word_ci(const std::string& text, size_t pos, const char* needle) {
  if (!needle) return false;
  const size_t len = std::strlen(needle);
  if (pos + len > text.size()) return false;
  for (size_t i = 0; i < len; ++i) {
    if (std::tolower(static_cast<unsigned char>(text[pos + i])) != needle[i]) return false;
  }
  return true;
}

static std::string rewrite_inline_asm_mnemonics(const std::string& raw,
                                                const std::string& target_triple) {
  if (raw.empty() || !llvm_target_is_x86_64(target_triple)) return raw;
  std::string rewritten;
  rewritten.reserve(raw.size());
  for (size_t i = 0; i < raw.size();) {
    if (matches_word_ci(raw, i, "yield")) {
      const bool start_ok = (i == 0) || !is_asm_word_char(raw[i - 1]);
      const size_t end = i + 5;
      const bool end_ok = (end >= raw.size()) || !is_asm_word_char(raw[end]);
      if (start_ok && end_ok) {
        rewritten += "pause";
        i = end;
        continue;
      }
    }
    rewritten.push_back(raw[i]);
    ++i;
  }
  return rewritten;
}

void StmtEmitter::emit_stmt(FnCtx& ctx, const Stmt& stmt) {
  std::visit([&](const auto& s) { emit_stmt_impl(ctx, s); }, stmt.payload);
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const LocalDecl& d) {
  emit_non_control_flow_stmt(ctx, d);
}

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const LocalDecl& d) {
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
    emit_lir_op(
        ctx, lir::LirAllocaOp{dyn_ptr, elem_ty, count, stack_align > 1 ? stack_align : 0});
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
    if (d.type.spec.array_rank > 0 ||
        (!d.type.spec.is_vector &&
         d.type.spec.ptr_level == 0 &&
         (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION))) {
      module_->need_memset = true;
      emit_lir_op(ctx, lir::LirMemsetOp{
                           slot, "0", std::to_string(sizeof_ts(mod_, d.type.spec)), false});
    } else {
      emit_lir_op(ctx, lir::LirStoreOp{ty, std::string("zeroinitializer"), slot});
    }
    return;
  }
  rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
  emit_lir_op(ctx, lir::LirStoreOp{ty, rhs, slot});
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ExprStmt& s) {
  emit_non_control_flow_stmt(ctx, s);
}

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const ExprStmt& s) {
  if (!s.expr) return;
  TypeSpec ts{};
  emit_rval_id(ctx, *s.expr, ts);
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const InlineAsmStmt& s) {
  emit_non_control_flow_stmt(ctx, s);
}

void StmtEmitter::emit_non_control_flow_stmt(FnCtx& ctx, const InlineAsmStmt& s) {
  const std::string aliased_template =
      rewrite_inline_asm_mnemonics(s.asm_template, mod_.target_profile.triple);
  const std::string asm_text = escape_llvm_c_bytes(aliased_template);
  const std::string constraints = rewrite_asm_constraints(escape_llvm_c_bytes(s.constraints));
  const std::vector<std::string> constraint_parts = split_asm_constraints(constraints);
  std::vector<ExprId> outputs = s.outputs;
  std::vector<bool> output_readwrite = s.output_readwrite;
  if (outputs.empty() && s.output) {
    outputs.push_back(*s.output);
    output_readwrite.push_back(s.output_is_readwrite);
  }
  TypeSpec ret_ts{};
  std::string ret_ty = "void";
  const bool scalar_result_output =
      outputs.size() == 1 &&
      (constraint_parts.empty() || !asm_constraint_is_memory_operand(constraint_parts[0]));
  if (scalar_result_output) {
    ret_ts = resolve_expr_type(ctx, outputs[0]);
    ret_ty = llvm_ty(ret_ts);
  }
  std::vector<std::string> rendered_constraints;
  std::vector<std::string> asm_args;
  if (scalar_result_output) {
    const std::string output_constraint =
        constraint_parts.empty() ? "=r" : llvm_output_constraint(constraint_parts[0]);
    rendered_constraints.push_back(output_constraint);
    if (!output_readwrite.empty() && output_readwrite[0]) {
      TypeSpec out_in_ts{};
      const std::string out_in = emit_rval_id(ctx, outputs[0], out_in_ts);
      rendered_constraints.push_back("0");
      asm_args.push_back(llvm_ty(out_in_ts) + " " + out_in);
    }
  } else {
    for (size_t i = 0; i < outputs.size(); ++i) {
      const std::string raw_constraint =
          i < constraint_parts.size() ? constraint_parts[i] : std::string("=m");
      rendered_constraints.push_back(llvm_output_constraint(raw_constraint));
      TypeSpec out_ts{};
      const std::string out_ptr = emit_lval(ctx, outputs[i], out_ts);
      asm_args.push_back("ptr elementtype(" + llvm_ty(out_ts) + ") " + out_ptr);
    }
    for (size_t i = 0; i < outputs.size(); ++i) {
      if (i >= output_readwrite.size() || !output_readwrite[i]) continue;
      const std::string raw_constraint =
          i < constraint_parts.size() ? constraint_parts[i] : std::string("m");
      rendered_constraints.push_back(llvm_memory_input_constraint(raw_constraint));
      TypeSpec out_ts{};
      const std::string out_ptr = emit_lval(ctx, outputs[i], out_ts);
      asm_args.push_back("ptr elementtype(" + llvm_ty(out_ts) + ") " + out_ptr);
    }
  }
  const size_t input_constraint_start = outputs.size();
  for (size_t i = 0; i < s.inputs.size(); ++i) {
    ExprId input = s.inputs[i];
    const size_t constraint_index = input_constraint_start + i;
    if (constraint_index < constraint_parts.size()) {
      rendered_constraints.push_back(constraint_parts[constraint_index]);
    }
    TypeSpec in_ts{};
    const std::string in = emit_rval_id(ctx, input, in_ts);
    asm_args.push_back(llvm_ty(in_ts) + " " + in);
  }
  std::string asm_args_str;
  for (size_t i = 0; i < asm_args.size(); ++i) {
    if (i) asm_args_str += ", ";
    asm_args_str += asm_args[i];
  }
  const std::string rendered_constraint_text =
      rendered_constraints.empty() ? constraints : join_asm_constraints(rendered_constraints);
  if (!scalar_result_output) {
    emit_lir_op(ctx, lir::LirInlineAsmOp{
                         {}, ret_ty, asm_text, rendered_constraint_text, s.has_side_effects,
                         asm_args_str});
    return;
  }

  const std::string result = fresh_tmp(ctx);
  emit_lir_op(
      ctx,
      lir::LirInlineAsmOp{result, ret_ty, asm_text, rendered_constraint_text, s.has_side_effects,
                          asm_args_str});
  TypeSpec out_pointee_ts{};
  const std::string out_ptr = emit_lval(ctx, outputs[0], out_pointee_ts);
  const std::string coerced = coerce(ctx, result, ret_ts, out_pointee_ts);
  emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(out_pointee_ts), coerced, out_ptr});
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ReturnStmt& s) {
  emit_control_flow_stmt(ctx, s);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ReturnStmt& s) {
  if (!s.expr) {
    const auto& rts = ctx.fn->return_type.spec;
    if (rts.base == TB_VOID && rts.ptr_level == 0 && rts.array_rank == 0 &&
        !rts.is_lvalue_ref && !rts.is_rvalue_ref) {
      emit_term_ret(ctx, "void", std::nullopt);
    } else {
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
  TypeSpec coerce_target = ctx.fn->return_type.spec;
  if ((coerce_target.is_lvalue_ref || coerce_target.is_rvalue_ref) &&
      coerce_target.ptr_level == 0) {
    coerce_target.ptr_level++;
  }
  val = coerce(ctx, val, ts, coerce_target);
  emit_term_ret(ctx, llvm_ret_ty(ctx.fn->return_type.spec), val);
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const IfStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const WhileStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ForStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const GotoStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const IndirBrStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const LabelStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const BreakStmt& s) { emit_control_flow_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const ContinueStmt& s) { emit_control_flow_stmt(ctx, s); }

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const IfStmt& s) {
  TypeSpec cond_ts{};
  const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
  const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
  const std::string then_lbl = block_lbl(s.then_block);
  const std::string after_lbl = block_lbl(s.after_block);
  if (s.else_block) {
    emit_term_condbr(ctx, cond_i1, then_lbl, block_lbl(*s.else_block));
  } else {
    emit_term_condbr(ctx, cond_i1, then_lbl, after_lbl);
  }
  ctx.block_meta[s.then_block.value].break_label = std::nullopt;
  if (s.else_block) ctx.block_meta[s.else_block->value].break_label = std::nullopt;
  (void)after_lbl;
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const WhileStmt& s) {
  const std::string cond_lbl =
      s.continue_target ? block_lbl(*s.continue_target) : fresh_lbl(ctx, "while.cond.");
  const std::string body_lbl = block_lbl(s.body_block);
  const std::string end_lbl =
      s.break_target ? block_lbl(*s.break_target) : fresh_lbl(ctx, "while.end.");

  ctx.continue_redirect[s.body_block.value] = cond_lbl;

  TypeSpec cond_ts{};
  const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
  const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
  emit_term_condbr(ctx, cond_i1, body_lbl, end_lbl);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ForStmt& s) {
  if (s.init) {
    TypeSpec ts{};
    emit_rval_id(ctx, *s.init, ts);
  }
  const std::string body_lbl = block_lbl(s.body_block);
  const std::string end_lbl =
      s.break_target ? block_lbl(*s.break_target) : fresh_lbl(ctx, "for.end.");
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

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const DoWhileStmt& s) {
  const std::string body_lbl = block_lbl(s.body_block);
  const std::string end_lbl =
      s.break_target ? block_lbl(*s.break_target) : fresh_lbl(ctx, "dowhile.end.");
  const std::string cond_lbl = "dowhile.cond." + std::to_string(s.body_block.value);
  ctx.continue_redirect[s.body_block.value] = cond_lbl;
  emit_fallthrough_lbl(ctx, cond_lbl);
  TypeSpec cond_ts{};
  const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
  const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
  emit_term_condbr(ctx, cond_i1, body_lbl, end_lbl);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const SwitchStmt& s) {
  TypeSpec ts{};
  std::string val = emit_rval_id(ctx, s.cond, ts);
  if (ts.ptr_level == 0 && ts.array_rank == 0 && is_any_int(ts.base)) {
    TypeBase promoted = integer_promote(ts.base);
    if (promoted != ts.base) {
      TypeSpec promoted_ts{};
      promoted_ts.base = promoted;
      val = coerce(ctx, val, ts, promoted_ts);
      ts = promoted_ts;
    }
  }
  const std::string ty = llvm_ty(ts);
  const std::string default_lbl =
      s.default_block ? block_lbl(*s.default_block)
                      : (s.break_block ? block_lbl(*s.break_block) : fresh_lbl(ctx, "sw.end."));
  const std::string break_lbl = s.break_block ? block_lbl(*s.break_block) : default_lbl;
  ctx.block_meta[s.body_block.value].break_label = break_lbl;

  const Block* body_blk = nullptr;
  for (const auto& blk : ctx.fn->blocks) {
    if (blk.id.value == s.body_block.value) {
      body_blk = &blk;
      break;
    }
  }

  const std::string body_lbl = block_lbl(s.body_block);
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
      }
    }
  }
  emit_term_switch(ctx, val, ty, default_lbl, std::move(sw_cases));
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const GotoStmt& s) {
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

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const IndirBrStmt& s) {
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
    ctx.cur_block().insts.push_back(lir::LirIndirectBrOp{val, std::move(targets)});
    ctx.last_term = true;
  }
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const LabelStmt& s) {
  emit_fallthrough_lbl(ctx, "ulbl_" + s.name);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const BreakStmt& s) {
  if (s.target) emit_term_br(ctx, block_lbl(*s.target));
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ContinueStmt& s) {
  if (!s.target) return;
  const auto it = ctx.continue_redirect.find(s.target->value);
  if (it != ctx.continue_redirect.end()) {
    emit_term_br(ctx, it->second);
    return;
  }
  emit_term_br(ctx, block_lbl(*s.target));
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseRangeStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const DefaultStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseStmt&) {}
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseRangeStmt&) {}
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const DefaultStmt&) {}

}  // namespace c4c::codegen::lir
