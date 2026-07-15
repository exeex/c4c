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
    const auto local_authority = ctx.local_object_authorities.find(d.id.value);
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
    if (local_authority == ctx.local_object_authorities.end() || !ctx.lir_function) {
      throw std::logic_error("VLA local object authority was not hoisted");
    }
    lir::LirCurrentFunctionLocalObjectPointer dynamic_authority = local_authority->second;
    dynamic_authority.pointer_definition = module_->alloc_value();
    dynamic_authority.pointee_type = lir::LirTypeRef(elem_ty);
    dynamic_authority.indexed_element_type.reset();
    ctx.local_object_authorities[d.id.value] = dynamic_authority;
    const lir::LirOperand dynamic_pointer =
        lir::LirOperand::ssa(dyn_ptr, dynamic_authority.pointer_definition);
    emit_lir_op(
        ctx, lir::LirAllocaOp{dynamic_pointer, elem_ty, count,
                              stack_align > 1 ? stack_align : 0, dynamic_authority});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), dynamic_pointer,
                                     lir::LirOperand::ssa(
                                         slot, local_authority->second.pointer_definition),
                                     local_authority->second});
  }

  if (!d.init) return;
  const std::string slot = ctx.local_slots.at(d.id.value);
  const auto authority = ctx.local_object_authorities.find(d.id.value);
  const auto store_with_local_authority = [&](lir::LirOperand value, const std::string& type,
                                              bool requires_native_store_authority = false) {
    emit_lir_op(ctx, lir::LirStoreOp{type, std::move(value),
                                     lir::LirOperand::ssa(
                                         slot, authority->second.pointer_definition),
                                     authority->second,
                                     requires_native_store_authority});
  };
  if (authority == ctx.local_object_authorities.end()) {
    throw std::logic_error("local object authority was not hoisted");
  }
  TypeSpec rhs_ts{};
  if (std::holds_alternative<LabelAddrExpr>(get_expr(*d.init).payload)) {
    const lir::LirOperand direct = emit_rval_operand(ctx, *d.init, rhs_ts);
    const std::string ty =
        (d.type.spec.array_rank > 0) ? llvm_alloca_ty(mod_, d.type.spec)
                                     : llvm_value_ty(mod_, d.type.spec);
    store_with_local_authority(coerce_operand(ctx, direct, rhs_ts, d.type.spec), ty);
    return;
  }
  lir::LirOperand rhs_operand = emit_rval_operand(ctx, *d.init, rhs_ts);
  std::string rhs = rhs_operand.str();
  const std::string ty =
      (d.type.spec.array_rank > 0) ? llvm_alloca_ty(mod_, d.type.spec)
                                   : llvm_value_ty(mod_, d.type.spec);
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
      const long long size = sizeof_ts(mod_, d.type.spec);
      if (size > 0) {
        emit_lir_op(ctx, lir::LirMemsetOp{
                             .dst = lir::LirOperand::ssa(
                                 slot, authority->second.pointer_definition),
                             .byte_val = lir::LirOperand::integer("0", 0),
                             .size = lir::LirOperand::integer(std::to_string(size), size),
                             .is_volatile = false,
                             .requires_native_memory_va_authority = true,
                             .dst_authority = lir::LirMemoryVaPointerAuthority{
                                 authority->second},
                             .byte_authority = lir::LirMemoryVaIntegerAuthority{
                                 lir::LirTypeRef::integer(8), lir::LirIntegerImmediate{0}},
                             .size_authority = lir::LirMemoryVaIntegerAuthority{
                                 lir::LirTypeRef::integer(64), lir::LirIntegerImmediate{size}},
                         });
      } else {
        emit_lir_op(ctx, lir::LirMemsetOp{slot, "0", std::to_string(size), false});
      }
    } else {
      store_with_local_authority(lir::LirOperand::raw("zeroinitializer"), ty);
    }
    return;
  }
  const bool candidate_native_local_scalar_store =
      d.type.spec.array_rank == 0 && d.type.spec.ptr_level == 0 &&
      is_any_int(d.type.spec.base) && rhs_operand.integer_immediate() != nullptr;
  if (candidate_native_local_scalar_store) {
    const lir::LirOperand coerced = coerce_operand(ctx, rhs_operand, rhs_ts,
                                                    d.type.spec);
    if (coerced.integer_immediate()) {
      store_with_local_authority(coerced, ty, true);
      return;
    }
  }
  rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
  store_with_local_authority(lir::LirOperand::raw(rhs), ty);
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
  std::vector<lir::LirInlineAsmValueBinding> ordinary_inputs;
  std::vector<lir::LirInlineAsmValueBinding> ordinary_results;
  if (scalar_result_output) {
    const std::string output_constraint =
        constraint_parts.empty() ? "=r" : llvm_output_constraint(constraint_parts[0]);
    rendered_constraints.push_back(output_constraint);
    if (!output_readwrite.empty() && output_readwrite[0]) {
      TypeSpec out_in_ts{};
      const std::string out_in = emit_rval_id(ctx, outputs[0], out_in_ts);
      rendered_constraints.push_back("0");
      asm_args.push_back(llvm_ty(out_in_ts) + " " + out_in);
      ordinary_inputs.push_back(lir::LirInlineAsmValueBinding{
          lir::LirOperand(out_in),
          lir::LirTypeRef::hir_inline_asm_type_text(llvm_ty(out_in_ts)),
          lir::LirInlineAsmValueRole::ReadWrite, 0});
    }
  } else {
    std::vector<std::string> output_ptrs;
    std::vector<TypeSpec> output_types;
    output_ptrs.reserve(outputs.size());
    output_types.reserve(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
      const std::string raw_constraint =
          i < constraint_parts.size() ? constraint_parts[i] : std::string("=m");
      rendered_constraints.push_back(llvm_output_constraint(raw_constraint));
      TypeSpec out_ts{};
      const std::string out_ptr = emit_lval(ctx, outputs[i], out_ts);
      output_ptrs.push_back(out_ptr);
      output_types.push_back(out_ts);
      asm_args.push_back("ptr elementtype(" + llvm_ty(out_ts) + ") " + out_ptr);
    }
    for (size_t i = 0; i < outputs.size(); ++i) {
      if (i >= output_readwrite.size() || !output_readwrite[i]) continue;
      const std::string raw_constraint =
          i < constraint_parts.size() ? constraint_parts[i] : std::string("m");
      rendered_constraints.push_back(llvm_memory_input_constraint(raw_constraint));
      asm_args.push_back("ptr elementtype(" + llvm_ty(output_types[i]) + ") " +
                         output_ptrs[i]);
    }
    for (size_t i = 0; i < outputs.size(); ++i) {
      const TypeSpec& output_ts = output_types[i];
      const bool is_read_write =
          i < output_readwrite.size() && output_readwrite[i];
      if (is_read_write) {
        const std::string old_value = emit_rval_from_access_ptr(
            ctx, output_ptrs[i], output_ts, output_ts, false);
        ordinary_inputs.push_back(lir::LirInlineAsmValueBinding{
            lir::LirOperand(old_value),
            lir::LirTypeRef::hir_inline_asm_type_text(llvm_ty(output_ts)),
            lir::LirInlineAsmValueRole::ReadWrite, i});
      }
      ordinary_results.push_back(lir::LirInlineAsmValueBinding{
          lir::LirOperand(fresh_tmp(ctx)),
          lir::LirTypeRef::hir_inline_asm_type_text(llvm_ty(output_ts)),
          is_read_write ? lir::LirInlineAsmValueRole::ReadWrite
                        : lir::LirInlineAsmValueRole::Output,
          i});
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
    ordinary_inputs.push_back(lir::LirInlineAsmValueBinding{
        lir::LirOperand(in),
        lir::LirTypeRef::hir_inline_asm_type_text(llvm_ty(in_ts)),
        lir::LirInlineAsmValueRole::Input, constraint_index});
  }
  for (const auto& clobber : s.clobbers) {
    if (!clobber.empty()) {
      rendered_constraints.push_back("~{" + clobber + "}");
    }
  }
  std::string asm_args_str;
  for (size_t i = 0; i < asm_args.size(); ++i) {
    if (i) asm_args_str += ", ";
    asm_args_str += asm_args[i];
  }
  const std::string rendered_constraint_text =
      rendered_constraints.empty() ? constraints : join_asm_constraints(rendered_constraints);
  lir::LirInlineAsmOp inline_asm;
  inline_asm.ret_type = lir::LirTypeRef::hir_inline_asm_type_text(ret_ty);
  inline_asm.asm_text = asm_text;
  inline_asm.constraints = rendered_constraint_text;
  inline_asm.side_effects = s.has_side_effects;
  inline_asm.args_str = asm_args_str;
  inline_asm.clobbers = s.clobbers;
  inline_asm.insn_r =
      s.insn_r ? std::optional<lir::LirInlineAsmInsnRMetadata>{
                     lir::LirInlineAsmInsnRMetadata{
                         .opcode = s.insn_r->opcode,
                         .funct3 = s.insn_r->funct3,
                         .funct7 = s.insn_r->funct7,
                         .operand_indices = s.insn_r->operand_indices,
                     }}
               : std::nullopt;
  inline_asm.original_asm_text = s.asm_template;
  inline_asm.original_constraint_text = s.constraints;
  inline_asm.ordinary_inputs = std::move(ordinary_inputs);
  inline_asm.ordinary_results = std::move(ordinary_results);
  if (!scalar_result_output) {
    emit_lir_op(ctx, std::move(inline_asm));
    return;
  }

  const bool is_read_write =
      !output_readwrite.empty() && output_readwrite[0];
  const lir::LirOperand semantic_result =
      is_read_write ? lir::LirOperand(fresh_tmp(ctx)) : fresh_value(ctx);
  inline_asm.result = lir::LirOperand(semantic_result.str());
  inline_asm.ordinary_results.push_back(lir::LirInlineAsmValueBinding{
      semantic_result, lir::LirTypeRef::hir_inline_asm_type_text(ret_ty),
      is_read_write ? lir::LirInlineAsmValueRole::ReadWrite
                    : lir::LirInlineAsmValueRole::Output,
      0});
  emit_lir_op(ctx, std::move(inline_asm));
  TypeSpec out_pointee_ts{};
  const std::string out_ptr = emit_lval(ctx, outputs[0], out_pointee_ts);
  const lir::LirOperand coerced =
      coerce_operand(ctx, semantic_result, ret_ts, out_pointee_ts);
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
      emit_term_ret(ctx, lir::LirTypeRef(lir::LirBuiltinType::Void), std::nullopt);
    } else {
      const std::string ret_ty = llvm_return_ty(mod_, rts);
      if (ret_ty == "ptr") {
        emit_term_ret(ctx, lir::LirTypeRef(lir::LirBuiltinType::Pointer),
                      lir::LirOperand("null"));
      } else if (is_float_base(rts.base) && rts.ptr_level == 0) {
        emit_term_ret(ctx, lir::LirTypeRef(ret_ty), lir::LirOperand("0.0"));
      } else {
        emit_term_ret(ctx, lir::LirTypeRef(ret_ty),
                      lir::LirOperand::integer("0", 0));
      }
    }
    return;
  }
  TypeSpec ts{};
  const lir::LirOperand source = emit_rval_operand(ctx, *s.expr, ts);
  const auto& function_return = ctx.fn->return_type.spec;
  if (function_return.base == TB_VOID && function_return.ptr_level == 0 &&
      function_return.array_rank == 0 && !function_return.is_lvalue_ref &&
      !function_return.is_rvalue_ref) {
    emit_term_ret(ctx, lir::LirTypeRef(lir::LirBuiltinType::Void), std::nullopt);
    return;
  }
  TypeSpec coerce_target = ctx.fn->return_type.spec;
  if ((coerce_target.is_lvalue_ref || coerce_target.is_rvalue_ref) &&
      coerce_target.ptr_level == 0) {
    coerce_target.ptr_level++;
  }
  const bool same_representation =
      llvm_value_ty(mod_, ts) == llvm_value_ty(mod_, coerce_target);
  // coerce() returns immediately, before emitting an instruction, when these
  // LLVM representations match. Any representation-changing path therefore
  // remains raw compatibility instead of inheriting the source authority.
  std::string presentation = coerce(ctx, source.str(), ts, coerce_target);
  lir::LirTypeRef return_type(
      llvm_return_ty(mod_, ctx.fn->return_type.spec));
  lir::LirOperand value = lir::LirOperand::raw(presentation);
  if (same_representation && return_type.kind() == lir::LirTypeKind::Integer) {
    if (const auto* immediate = source.integer_immediate()) {
      value = lir::LirOperand::integer(std::move(presentation), immediate->value);
    } else if (const auto* id = source.value_id()) {
      value = lir::LirOperand::ssa(std::move(presentation), *id);
    }
  }
  emit_term_ret(ctx, std::move(return_type), std::move(value));
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
  const lir::LirOperand cond_v = emit_rval_operand(ctx, s.cond, cond_ts);
  const lir::LirOperand cond_i1 = to_bool_operand(ctx, cond_v, cond_ts);
  const auto then_target = scheduled_target(s.then_block);
  const auto after_target = scheduled_target(s.after_block);
  if (s.else_block) {
    emit_term_condbr(ctx, cond_i1, then_target, scheduled_target(*s.else_block));
  } else {
    emit_term_condbr(ctx, cond_i1, then_target, after_target);
  }
  ctx.block_meta[s.then_block.value].break_label = std::nullopt;
  if (s.else_block) ctx.block_meta[s.else_block->value].break_label = std::nullopt;
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const WhileStmt& s) {
  const auto cond_target = s.continue_target
      ? scheduled_target(*s.continue_target)
      : fresh_direct_target(ctx, fresh_lbl(ctx, "while.cond."));
  const auto body_target = scheduled_target(s.body_block);
  const auto end_target = s.break_target
      ? scheduled_target(*s.break_target)
      : fresh_direct_target(ctx, fresh_lbl(ctx, "while.end."));

  ctx.continue_redirect[s.body_block.value] = cond_target;

  TypeSpec cond_ts{};
  const lir::LirOperand cond_v = emit_rval_operand(ctx, s.cond, cond_ts);
  const lir::LirOperand cond_i1 = to_bool_operand(ctx, cond_v, cond_ts);
  emit_term_condbr(ctx, cond_i1, body_target, end_target);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ForStmt& s) {
  if (s.init) {
    TypeSpec ts{};
    emit_rval_id(ctx, *s.init, ts);
  }
  const auto body_target = scheduled_target(s.body_block);
  const auto end_target = s.break_target
      ? scheduled_target(*s.break_target)
      : fresh_direct_target(ctx, fresh_lbl(ctx, "for.end."));
  const auto cond_target = fresh_direct_target(
      ctx, "for.cond." + std::to_string(s.body_block.value));
  const auto latch_target = fresh_direct_target(
      ctx, "for.latch." + std::to_string(s.body_block.value));
  ctx.continue_redirect[s.body_block.value] = latch_target;

  emit_fallthrough_lbl(ctx, cond_target);
  if (s.cond) {
    TypeSpec cts{};
    lir::LirOperand cv = emit_rval_operand(ctx, *s.cond, cts);
    cv = to_bool_operand(ctx, cv, cts);
    emit_condbr_and_open_sibling_lbl(ctx, cv, body_target, end_target, latch_target);
  } else {
    emit_br_and_open_lbl(ctx, body_target, latch_target);
  }
  if (s.update) {
    TypeSpec uts{};
    (void)emit_rval_id(ctx, *s.update, uts);
  }
  emit_term_br(ctx, cond_target);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const DoWhileStmt& s) {
  const auto body_target = scheduled_target(s.body_block);
  const auto end_target = s.break_target
      ? scheduled_target(*s.break_target)
      : fresh_direct_target(ctx, fresh_lbl(ctx, "dowhile.end."));
  const auto cond_target = fresh_direct_target(
      ctx, "dowhile.cond." + std::to_string(s.body_block.value));
  ctx.continue_redirect[s.body_block.value] = cond_target;
  emit_fallthrough_lbl(ctx, cond_target);
  TypeSpec cond_ts{};
  const lir::LirOperand cond_v = emit_rval_operand(ctx, s.cond, cond_ts);
  const lir::LirOperand cond_i1 = to_bool_operand(ctx, cond_v, cond_ts);
  emit_term_condbr(ctx, cond_i1, body_target, end_target);
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const SwitchStmt& s) {
  TypeSpec ts{};
  lir::LirOperand selector = emit_rval_operand(ctx, s.cond, ts);
  std::string val = selector.str();
  if (ts.ptr_level == 0 && ts.array_rank == 0 && is_any_int(ts.base)) {
    TypeBase promoted = integer_promote(ts.base);
    if (promoted != ts.base) {
      TypeSpec promoted_ts{};
      promoted_ts.base = promoted;
      if (selector.value_id()) {
        const lir::LirOperand promoted_selector = fresh_value(ctx);
        const lir::LirCastKind kind = is_signed_int(ts.base)
                                          ? lir::LirCastKind::SExt
                                          : lir::LirCastKind::ZExt;
        emit_lir_op(ctx, lir::LirCastOp{promoted_selector, kind,
                                        lir::LirTypeRef(llvm_ty(ts)), selector,
                                        lir::LirTypeRef(llvm_ty(promoted_ts))});
        selector = promoted_selector;
        val = selector.str();
      } else {
        val = coerce(ctx, val, ts, promoted_ts);
        selector = lir::LirOperand::raw(val);
      }
      ts = promoted_ts;
    }
  }
  const std::string ty = llvm_ty(ts);
  const auto default_target = s.default_block
      ? scheduled_target(*s.default_block)
      : (s.break_block ? scheduled_target(*s.break_block)
                       : fresh_direct_target(ctx, fresh_lbl(ctx, "sw.end.")));
  ctx.block_meta[s.body_block.value].break_label = default_target.label;

  const Block* body_blk = nullptr;
  for (const auto& blk : ctx.fn->blocks) {
    if (blk.id.value == s.body_block.value) {
      body_blk = &blk;
      break;
    }
  }

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
      const auto next_target = fresh_direct_target(ctx, fresh_lbl(ctx, "sw.range.next."));
      TypeSpec bool_ts{};
      bool_ts.base = TB_BOOL;
      emit_condbr_and_fallthrough_lbl(
          ctx, to_bool_operand(ctx, lir::LirOperand::raw(t_and), bool_ts),
          scheduled_target(bid), next_target);
    }
  }

  std::vector<std::pair<long long, c4c::codegen::LirDirectBranchTarget>> sw_cases;
  if (!s.case_blocks.empty()) {
    for (const auto& [case_val, case_bid] : s.case_blocks) {
      sw_cases.emplace_back(case_val, scheduled_target(case_bid));
    }
  } else if (body_blk) {
    for (const auto& stmt : body_blk->stmts) {
      if (const auto* cs = std::get_if<CaseStmt>(&stmt.payload)) {
        sw_cases.emplace_back(cs->value, scheduled_target(s.body_block));
      } else if (std::get_if<CaseRangeStmt>(&stmt.payload)) {
      }
    }
  }
  if (!selector.value_id()) {
    // A legacy/raw expression has no reusable value carrier. Materialize one
    // here rather than letting switch authority depend on its display text.
    const lir::LirOperand materialized_selector = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirBinOp{materialized_selector, "add",
                                   lir::LirTypeRef(ty),
                                   lir::LirOperand::raw(val),
                                   lir::LirOperand::integer("0", 0)});
    selector = materialized_selector;
  }
  emit_term_switch(ctx, selector, ty, default_target, std::move(sw_cases));
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const GotoStmt& s) {
  if (ctx.vla_stack_save_ptr && s.target.resolved_block.valid() &&
      s.target.resolved_block.value <= ctx.current_block_id) {
    module_->need_stackrestore = true;
    if (!ctx.vla_stack_lifetime_authority) {
      throw std::logic_error("VLA stack restore without typed stack lifetime authority");
    }
    emit_lir_op(ctx, lir::LirStackRestoreOp{
                         lir::LirOperand::ssa(*ctx.vla_stack_save_ptr,
                                              ctx.vla_stack_lifetime_authority->pointer_definition),
                         *ctx.vla_stack_lifetime_authority,
                         true,
                         lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition{
                             lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition::Kind::
                                 RestoreSavedVlaStackCheckpoint,
                             ctx.vla_stack_lifetime_authority->pointer_definition}});
  }
  if (s.target.resolved_block.valid()) {
    emit_term_br(ctx, scheduled_target(s.target.resolved_block));
  } else {
    emit_term_br(ctx, user_label_target(ctx, s.target.user_name));
  }
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const IndirBrStmt& s) {
  std::vector<std::string> targets;
  std::vector<lir::LirBlockId> successors;
  for (const auto& bb : ctx.fn->blocks) {
    for (const auto& stmt : bb.stmts) {
      if (const auto* ls = std::get_if<LabelStmt>(&stmt.payload)) {
        const auto target = user_label_target(ctx, ls->name);
        targets.push_back(target.label);
        successors.push_back(target.id);
      }
    }
  }
  TypeSpec dummy_ts{};
  const lir::LirOperand addr = emit_rval_operand(ctx, s.target, dummy_ts);
  const std::optional<lir::LirValueId> addr_value =
      addr.value_id() ? std::optional<lir::LirValueId>(*addr.value_id()) : std::nullopt;
  if (!ctx.last_term) {
    ctx.cur_block().insts.push_back(
        lir::LirIndirectBrOp{addr, addr_value, std::move(targets),
                             std::move(successors)});
    ctx.last_term = true;
  }
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const LabelStmt& s) {
  emit_fallthrough_lbl(ctx, user_label_target(ctx, s.name));
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const BreakStmt& s) {
  if (s.target) emit_term_br(ctx, scheduled_target(*s.target));
}

void StmtEmitter::emit_control_flow_stmt(FnCtx& ctx, const ContinueStmt& s) {
  if (!s.target) return;
  const auto it = ctx.continue_redirect.find(s.target->value);
  if (it != ctx.continue_redirect.end()) {
    emit_term_br(ctx, it->second);
    return;
  }
  emit_term_br(ctx, scheduled_target(*s.target));
}

void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const CaseRangeStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_stmt_impl(FnCtx& ctx, const DefaultStmt& s) { emit_switch_label_stmt(ctx, s); }
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseStmt&) {}
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const CaseRangeStmt&) {}
void StmtEmitter::emit_switch_label_stmt(FnCtx&, const DefaultStmt&) {}

}  // namespace c4c::codegen::lir
