#include "expr.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

bool integer_immediate_representable_by_type(const LirOperand& operand,
                                             const LirTypeRef& type) {
  const LirIntegerImmediate* immediate = operand.integer_immediate();
  if (!immediate || type.kind() != LirTypeKind::Integer) return true;
  const std::optional<unsigned> width = type.integer_bit_width();
  if (!width || *width == 0) return false;
  if (*width >= 64) return true;
  if (immediate->value < 0) {
    const long long minimum = -(1LL << (*width - 1));
    return immediate->value >= minimum;
  }
  const unsigned long long maximum = (1ULL << *width) - 1ULL;
  return static_cast<unsigned long long>(immediate->value) <= maximum;
}

LirOperand preserve_exact_binary_operand(const LirOperand& source,
                                         const std::string& normalized,
                                         const LirTypeRef& type) {
  if (!source.has_authority() || source.str() != normalized ||
      !integer_immediate_representable_by_type(source, type)) {
    return LirOperand(normalized);
  }
  return source;
}

bool selected_floating_lhs_authority_opcode(std::string_view opcode) {
  return opcode == "fadd" || opcode == "fsub" || opcode == "fmul";
}

bool selected_floating_rhs_authority_opcode(std::string_view opcode) {
  return opcode == "fadd" || opcode == "fsub" || opcode == "fmul";
}

bool floating_lhs_authority_already_published(const FnCtx& ctx,
                                              std::string_view opcode) {
  if (opcode != "fadd" && opcode != "fsub") return false;
  for (const auto& block : ctx.lir_blocks) {
    for (const auto& inst : block.insts) {
      const auto* binary = std::get_if<LirBinOp>(&inst);
      if (binary != nullptr && binary->scalar_lhs_parameter_authority &&
          binary->opcode.str() == opcode) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

LirOperand StmtEmitter::emit_complex_binary_arith(FnCtx& ctx, BinaryOp op,
                                                  const std::string& lv,
                                                  const TypeSpec& lts,
                                                  const std::string& rv,
                                                  const TypeSpec& rts,
                                                  const TypeSpec& res_spec,
                                                  bool require_direct_aggregate_ssa) {
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
    emit_lir_op(ctx, lir::LirInsertValueOp{with_real, llvm_ty(complex_ts), "undef",
                                           llvm_ty(elem_ts), real_v, 0});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirInsertValueOp{out, llvm_ty(complex_ts), with_real,
                                           llvm_ty(elem_ts), imag_v, 1});
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
  const char* div_instr =
      is_float_base(elem_ts.base) ? "fdiv" : (is_signed_int(elem_ts.base) ? "sdiv" : "udiv");

  std::string out_real;
  std::string out_imag;
  if (op == BinaryOp::Add || op == BinaryOp::Sub) {
    out_real = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{out_real, std::string(op == BinaryOp::Add ? add_instr : sub_instr),
                                   elem_ty, lreal, rreal});
    out_imag = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{out_imag, std::string(op == BinaryOp::Add ? add_instr : sub_instr),
                                   elem_ty, limag, rimag});
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
  if (cplx_ty == llvm_ty(res_spec) && require_direct_aggregate_ssa) {
    const LirOperand out = fresh_value(ctx);
    const LirTypeRef aggregate_type = LirTypeRef::anonymous_struct(
        {LirTypeRef(elem_ty), LirTypeRef(elem_ty)});
    emit_lir_op(ctx, lir::LirInsertValueOp{out, aggregate_type, with_real,
                                           LirTypeRef(elem_ty), out_imag, 1,
                                           true, aggregate_type});
    return out;
  }
  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirInsertValueOp{out, cplx_ty, with_real, elem_ty, out_imag, 1});
  return LirOperand(cplx_ty == llvm_ty(res_spec) ? out :
                    coerce(ctx, out, complex_ts, res_spec));
}

LirOperand StmtEmitter::emit_binary_rval_operand(FnCtx& ctx,
                                                 const BinaryExpr& b,
                                                 const Expr& e,
                                                 bool require_direct_aggregate_ssa) {
  if (b.op == BinaryOp::Comma) {
    TypeSpec lts{};
    emit_rval_id(ctx, b.lhs, lts);
    TypeSpec rts{};
    return emit_rval_id(ctx, b.rhs, rts);
  }
  if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
    return emit_logical(ctx, b, e);
  }

  TypeSpec lts{};
  const LirOperand source_lv = emit_rval_operand(ctx, b.lhs, lts);
  std::string lv = source_lv.str();
  TypeSpec rts{};
  const LirOperand source_rv = emit_rval_operand(ctx, b.rhs, rts);
  std::string rv = source_rv.str();
  TypeSpec res_spec =
      (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0) ? e.type.spec : lts;

  if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) && llvm_ty(lts) == "ptr" &&
      llvm_ty(rts) != "ptr") {
    TypeSpec i64_ts{};
    i64_ts.base = TB_LONGLONG;
    LirOperand idx = coerce_operand(ctx, source_rv, rts, i64_ts);
    if (b.op == BinaryOp::Sub) {
      const std::string neg = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx.str()});
      idx = LirOperand::raw(neg);
    }
    return emit_indexed_gep(ctx, source_lv, lts, idx);
  }

  if ((b.op == BinaryOp::Eq || b.op == BinaryOp::Ne) &&
      (is_complex_base(lts.base) || is_complex_base(rts.base))) {
    TypeSpec cmp_lts = lts;
    TypeSpec cmp_rts = rts;
    std::string cmp_lv = lv;
    std::string cmp_rv = rv;
    const TypeSpec elem_ts =
        is_complex_base(cmp_lts.base) ? complex_component_ts(cmp_lts.base)
                                      : complex_component_ts(cmp_rts.base);
    const TypeSpec complex_ts = is_complex_base(cmp_lts.base) ? cmp_lts : cmp_rts;
    auto lift_scalar_to_complex = [&](const std::string& scalar, const TypeSpec& scalar_ts,
                                      bool scalar_is_lhs) -> std::string {
      std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string imag_v = emit_const_int_like(0, elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{with_real, llvm_ty(complex_ts), "undef",
                                             llvm_ty(elem_ts), real_v, 0});
      const std::string out = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirInsertValueOp{out, llvm_ty(complex_ts), with_real,
                                             llvm_ty(elem_ts), imag_v, 1});
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
      emit_lir_op(ctx, lir::LirCmpOp{creal, true, LirCmpPredicate::OEq,
                                     llvm_ty(elem_ts), lreal, rreal});
      emit_lir_op(ctx, lir::LirCmpOp{cimag, true, LirCmpPredicate::OEq,
                                     llvm_ty(elem_ts), limag, rimag});
    } else {
      emit_lir_op(ctx, lir::LirCmpOp{creal, false, LirCmpPredicate::Eq,
                                     llvm_ty(elem_ts), lreal, rreal});
      emit_lir_op(ctx, lir::LirCmpOp{cimag, false, LirCmpPredicate::Eq,
                                     llvm_ty(elem_ts), limag, rimag});
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

  if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub || b.op == BinaryOp::Mul ||
       b.op == BinaryOp::Div) &&
      (is_complex_base(lts.base) || is_complex_base(rts.base))) {
    return emit_complex_binary_arith(ctx, b.op, lv, lts, rv, rts, res_spec,
                                     require_direct_aggregate_ssa);
  }

  if (is_vector_value(res_spec)) {
    auto emit_splat_vec = [&](const LirOperand& scalar, const TypeSpec& scalar_ts,
                              const TypeSpec& vec_ts) -> std::string {
      TypeSpec elem_ts = vec_ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      const LirOperand coerced = coerce_operand(ctx, scalar, scalar_ts, elem_ts);
      const std::string elem_ty = llvm_ty(elem_ts);
      const std::string vec_ty_s = llvm_vector_ty(vec_ts);
      const int lanes = vec_ts.vector_lanes;
      const LirOperand ins = fresh_value(ctx);
      const LirOperand zero_index = LirOperand::integer("i64 0", 0);
      const lir::LirNativeVectorShape shape{static_cast<uint32_t>(lanes), elem_ty};
      const lir::LirVectorRef vector_ref =
          module_->register_vector({static_cast<uint32_t>(lanes), lir::LirTypeRef(elem_ty)});
      lir::LirNativeVectorAuthority insert_authority{
          ctx.lir_function->link_name_id, *ins.value_id(), std::nullopt, std::nullopt,
          coerced.value_id() ? std::optional<lir::LirValueId>(*coerced.value_id()) : std::nullopt,
          shape, shape, std::nullopt,
          lir::LirNativeVectorIndex{zero_index, lir::LirTypeRef::integer(64)}, {}};
      insert_authority.vector_ref = vector_ref;
      emit_lir_op(ctx, lir::LirInsertElementOp{ins, vec_ty_s, "poison", elem_ty, coerced,
                                               zero_index, std::move(insert_authority), true});
      const LirOperand shuf = fresh_value(ctx);
      lir::LirNativeVectorAuthority shuffle_authority{
          ctx.lir_function->link_name_id, *shuf.value_id(), *ins.value_id(), std::nullopt,
          std::nullopt, shape, shape, shape, std::nullopt,
          std::vector<lir::LirShuffleMaskLane>(
              static_cast<size_t>(lanes),
              {.kind = lir::LirShuffleMaskLane::Kind::Selected, .selected_lane = 0})};
      shuffle_authority.vector_ref = vector_ref;
      emit_lir_op(ctx, lir::LirShuffleVectorOp{
                           shuf, vec_ty_s, ins,
                           LirOperand::special_token(lir::LirSpecialToken::Poison),
                           "<" + std::to_string(lanes) + " x i32>",
                           LirOperand::special_token(lir::LirSpecialToken::ZeroInitializer),
                           std::move(shuffle_authority), true});
      return shuf.str();
    };
    if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
      rv = emit_splat_vec(source_rv, rts, lts);
      rts = lts;
    } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
      lv = emit_splat_vec(source_lv, lts, rts);
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
          emit_lir_op(ctx, lir::LirBinOp{tmp, is_signed_int(res_spec.base) ? "sdiv" : "udiv",
                                         vec_ty, lv, rv});
        return tmp;
      case BinaryOp::Mod:
        if (vec_float)
          emit_lir_op(ctx, lir::LirBinOp{tmp, "frem", vec_ty, lv, rv});
        else
          emit_lir_op(ctx, lir::LirBinOp{tmp, is_signed_int(res_spec.base) ? "srem" : "urem",
                                         vec_ty, lv, rv});
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

  const bool l_is_int =
      is_any_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 && !is_vector_value(lts);
  const bool r_is_int =
      is_any_int(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 && !is_vector_value(rts);
  const bool l_is_flt = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0 &&
                        !is_vector_value(lts);
  const bool r_is_flt = is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0 &&
                        !is_vector_value(rts);
  if (l_is_int && r_is_int) {
    const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
    if (is_shift) {
      TypeBase lp = integer_promote(lts.base);
      TypeSpec lp_ts{};
      lp_ts.base = lp;
      if (lts.base != lp) lv = coerce(ctx, lv, lts, lp_ts);
      lts = lp_ts;
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

  if (l_is_int && r_is_int && is_any_int(res_spec.base) && res_spec.ptr_level == 0 &&
      res_spec.array_rank == 0) {
    res_spec = lts;
  }

  auto emit_splat = [&](const LirOperand& scalar, const TypeSpec& scalar_ts,
                        const TypeSpec& vec_ts) -> std::string {
    TypeSpec elem_ts = vec_ts;
    elem_ts.is_vector = false;
    elem_ts.vector_lanes = 0;
    elem_ts.vector_bytes = 0;
    const LirOperand coerced = coerce_operand(ctx, scalar, scalar_ts, elem_ts);
    const std::string elem_ty = llvm_ty(elem_ts);
    const std::string vec_ty = llvm_vector_ty(vec_ts);
    const int lanes = vec_ts.vector_lanes;
    const LirOperand ins = fresh_value(ctx);
    const LirOperand zero_index = LirOperand::integer("i64 0", 0);
    const lir::LirNativeVectorShape shape{static_cast<uint32_t>(lanes), elem_ty};
    const lir::LirVectorRef vector_ref =
        module_->register_vector({static_cast<uint32_t>(lanes), lir::LirTypeRef(elem_ty)});
    lir::LirNativeVectorAuthority insert_authority{
        ctx.lir_function->link_name_id, *ins.value_id(), std::nullopt, std::nullopt,
        coerced.value_id() ? std::optional<lir::LirValueId>(*coerced.value_id()) : std::nullopt,
        shape, shape, std::nullopt,
        lir::LirNativeVectorIndex{zero_index, lir::LirTypeRef::integer(64)}, {}};
    insert_authority.vector_ref = vector_ref;
    emit_lir_op(ctx, lir::LirInsertElementOp{ins, vec_ty, "poison", elem_ty, coerced, zero_index,
        std::move(insert_authority), true});
    const LirOperand shuf = fresh_value(ctx);
    lir::LirNativeVectorAuthority shuffle_authority{
        ctx.lir_function->link_name_id, *shuf.value_id(), *ins.value_id(), std::nullopt,
        std::nullopt, shape, shape, shape, std::nullopt,
        std::vector<lir::LirShuffleMaskLane>(
            static_cast<size_t>(lanes),
            {.kind = lir::LirShuffleMaskLane::Kind::Selected, .selected_lane = 0})};
    shuffle_authority.vector_ref = vector_ref;
    emit_lir_op(ctx, lir::LirShuffleVectorOp{shuf, vec_ty, ins,
                                             LirOperand::special_token(lir::LirSpecialToken::Poison),
                                             "<" + std::to_string(lanes) + " x i32>",
                                             LirOperand::special_token(lir::LirSpecialToken::ZeroInitializer),
                                             std::move(shuffle_authority), true});
    return shuf.str();
  };
  if (is_vector_value(lts) && !is_vector_value(rts) && rts.ptr_level == 0) {
    rv = emit_splat(source_rv, rts, lts);
    rts = lts;
    res_spec = lts;
  } else if (is_vector_value(rts) && !is_vector_value(lts) && lts.ptr_level == 0) {
    lv = emit_splat(source_lv, lts, rts);
    lts = rts;
    res_spec = rts;
  }

  const std::string op_ty = llvm_ty(lts);
  const bool lf = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
  const bool ls = is_signed_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
  const bool arith_ls = ls;

  if (b.op == BinaryOp::Add || b.op == BinaryOp::Sub) {
    auto emit_ptr_gep = [&](const LirOperand& base_ptr, const TypeSpec& base_ts,
                            const LirOperand& idx_val, const TypeSpec& idx_ts,
                            bool negate_idx) -> LirOperand {
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      LirOperand idx = coerce_operand(ctx, idx_val, idx_ts, i64_ts);
      if (negate_idx) {
        const std::string neg = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx.str()});
        idx = LirOperand::raw(neg);
      }
      return emit_indexed_gep(ctx, base_ptr, base_ts, idx);
    };

    const bool rhs_is_nonptr_scalar =
        llvm_ty(rts) != "ptr" && !is_float_base(rts.base) && rts.array_rank == 0;
    const bool lhs_is_nonptr_scalar =
        llvm_ty(lts) != "ptr" && !is_float_base(lts.base) && lts.array_rank == 0;
    if (op_ty == "ptr" && rhs_is_nonptr_scalar) {
      return emit_ptr_gep(source_lv, lts, source_rv, rts, b.op == BinaryOp::Sub);
    }
    if (llvm_ty(rts) == "ptr" && lhs_is_nonptr_scalar && b.op == BinaryOp::Add) {
      return emit_ptr_gep(source_rv, rts, source_lv, lts, false);
    }
    if (b.op == BinaryOp::Sub && op_ty == "ptr" && llvm_ty(rts) == "ptr") {
      const LirOperand lhs_i = fresh_value(ctx);
      const LirOperand rhs_i = fresh_value(ctx);
      emit_lir_op(ctx, lir::LirCastOp{lhs_i, lir::LirCastKind::PtrToInt,
                                      "ptr", lv, "i64"});
      emit_lir_op(ctx, lir::LirCastOp{rhs_i, lir::LirCastKind::PtrToInt,
                                      "ptr", rv, "i64"});
      const LirOperand byte_diff = fresh_value(ctx);
      emit_lir_op(ctx, lir::LirBinOp{byte_diff, "sub", "i64", lhs_i, rhs_i});
      TypeSpec elem_ts = lts;
      if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
      const int elem_sz =
          (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0) ? 1 : std::max(1, sizeof_ts(mod_, elem_ts));
      LirOperand diff = byte_diff;
      if (elem_sz != 1) {
        const LirOperand scaled = fresh_value(ctx);
        emit_lir_op(ctx, lir::LirBinOp{scaled, "sdiv", "i64", byte_diff,
                                       std::to_string(elem_sz)});
        diff = scaled;
      }
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      if (res_spec.ptr_level > 0 || res_spec.array_rank > 0 || res_spec.base == TB_VOID) {
        return diff;
      }
      const std::string coerced = coerce(ctx, diff.str(), i64_ts, res_spec);
      return coerced == diff.str() ? diff : LirOperand::raw(coerced);
    }
  }

  rv = coerce(ctx, rv, rts, lts);

  static const struct {
    BinaryOp op;
    const char* i_s;
    const char* i_u;
    const char* f;
  } arith[] = {{BinaryOp::Add, "add", "add", "fadd"},
               {BinaryOp::Sub, "sub", "sub", "fsub"},
               {BinaryOp::Mul, "mul", "mul", "fmul"},
               {BinaryOp::Div, "sdiv", "udiv", "fdiv"},
               {BinaryOp::Mod, "srem", "urem", nullptr},
               {BinaryOp::Shl, "shl", "shl", nullptr},
               {BinaryOp::Shr, "ashr", "lshr", nullptr},
               {BinaryOp::BitAnd, "and", "and", nullptr},
               {BinaryOp::BitOr, "or", "or", nullptr},
               {BinaryOp::BitXor, "xor", "xor", nullptr}};
  for (const auto& row : arith) {
    if (row.op == b.op) {
      if ((row.op == BinaryOp::Add || row.op == BinaryOp::Sub) && op_ty == "ptr") {
        auto emit_ptr_fallback = [&](const LirOperand& base_ptr, const TypeSpec& base_ts,
                                     const LirOperand& idx_val, const TypeSpec& idx_ts,
                                     bool negate_idx) -> LirOperand {
          TypeSpec i64_ts{};
          i64_ts.base = TB_LONGLONG;
          LirOperand idx = coerce_operand(ctx, idx_val, idx_ts, i64_ts);
          if (negate_idx) {
            const std::string neg = fresh_tmp(ctx);
            emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", idx.str()});
            idx = LirOperand::raw(neg);
          }
          return emit_indexed_gep(ctx, base_ptr, base_ts, idx);
        };
        return emit_ptr_fallback(source_lv, lts, source_rv, rts,
                                 row.op == BinaryOp::Sub);
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
      const bool authoritative_scalar_integer =
          l_is_int && r_is_int && is_any_int(lts.base) &&
          lts.ptr_level == 0 && lts.array_rank == 0 &&
          !is_vector_value(lts);
      const bool authoritative_scalar_floating =
          l_is_flt && r_is_flt && is_float_base(lts.base) &&
          lts.ptr_level == 0 && lts.array_rank == 0 &&
          !is_vector_value(lts);
      if (authoritative_scalar_integer || authoritative_scalar_floating) {
        const LirOperand result = fresh_value(ctx);
        const LirTypeRef type(op_ty);
        const LirOperand lhs =
            preserve_exact_binary_operand(source_lv, lv, type);
        const LirOperand rhs =
            preserve_exact_binary_operand(source_rv, rv, type);
        const bool selected_floating_lhs =
            !authoritative_scalar_floating ||
            selected_floating_lhs_authority_opcode(instr);
        const bool selected_floating_rhs =
            !authoritative_scalar_floating ||
            selected_floating_rhs_authority_opcode(instr);
        std::optional<lir::LirScalarBinaryLhsParameterAuthority> lhs_authority;
        if (selected_floating_lhs && ctx.lir_function != nullptr &&
            lhs.value_id() != nullptr &&
            !floating_lhs_authority_already_published(ctx, instr)) {
          const auto definition = std::find_if(
              ctx.lir_function->native_body_parameter_definitions.begin(),
              ctx.lir_function->native_body_parameter_definitions.end(),
              [&](const auto& candidate) {
                return candidate.value == *lhs.value_id() &&
                       candidate.type == type &&
                       candidate.abi == lir::LirNativeBodyParameterAbi::DirectScalar;
              });
          if (definition != ctx.lir_function->native_body_parameter_definitions.end()) {
            lhs_authority = lir::LirScalarBinaryLhsParameterAuthority{
                .value = definition->value,
                .parameter_index = definition->parameter_index,
                .type = definition->type,
                .owner = definition->owner,
                .abi = definition->abi,
                .role = lir::LirScalarBinaryParameterRole::Lhs,
            };
          }
        }
        std::optional<lir::LirScalarBinaryRhsParameterAuthority> rhs_authority;
        if (selected_floating_rhs && !lhs_authority && ctx.lir_function != nullptr &&
            rhs.value_id() != nullptr) {
          const auto definition = std::find_if(
              ctx.lir_function->native_body_parameter_definitions.begin(),
              ctx.lir_function->native_body_parameter_definitions.end(),
              [&](const auto& candidate) {
                return candidate.value == *rhs.value_id() &&
                       candidate.type == type &&
                       candidate.abi == lir::LirNativeBodyParameterAbi::DirectScalar;
              });
          if (definition != ctx.lir_function->native_body_parameter_definitions.end()) {
            rhs_authority = lir::LirScalarBinaryRhsParameterAuthority{
                .value = definition->value,
                .parameter_index = definition->parameter_index,
                .type = definition->type,
                .owner = definition->owner,
                .abi = definition->abi,
                .role = lir::LirScalarBinaryParameterRole::Rhs,
            };
          }
        }
        emit_lir_op(ctx, lir::LirBinOp{result, std::string(instr), type, lhs, rhs,
                                       lhs_authority, rhs_authority});
        const std::string coerced = coerce(ctx, result.str(), lts, res_spec);
        return coerced == result.str() ? result : LirOperand::raw(coerced);
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{tmp, std::string(instr), op_ty, lv, rv});
      return coerce(ctx, tmp, lts, res_spec);
    }
  }

  static const struct {
    BinaryOp op;
    LirCmpPredicate is;
    LirCmpPredicate iu;
    LirCmpPredicate f;
  } cmp[] = {{BinaryOp::Lt, LirCmpPredicate::Slt, LirCmpPredicate::Ult,
              LirCmpPredicate::OLt},
             {BinaryOp::Le, LirCmpPredicate::Sle, LirCmpPredicate::Ule,
              LirCmpPredicate::OLe},
             {BinaryOp::Gt, LirCmpPredicate::Sgt, LirCmpPredicate::Ugt,
              LirCmpPredicate::OGt},
             {BinaryOp::Ge, LirCmpPredicate::Sge, LirCmpPredicate::Uge,
              LirCmpPredicate::OGe},
             {BinaryOp::Eq, LirCmpPredicate::Eq, LirCmpPredicate::Eq,
              LirCmpPredicate::OEq},
             {BinaryOp::Ne, LirCmpPredicate::Ne, LirCmpPredicate::Ne,
              LirCmpPredicate::UNe}};
  for (const auto& row : cmp) {
    if (row.op == b.op) {
      const bool authoritative_scalar_integer_compare =
          l_is_int && r_is_int && is_any_int(lts.base) &&
          lts.ptr_level == 0 && lts.array_rank == 0 &&
          !is_vector_value(lts);
      const bool authoritative_scalar_floating_compare =
          l_is_flt && r_is_flt && is_float_base(lts.base) &&
          lts.ptr_level == 0 && lts.array_rank == 0 &&
          !is_vector_value(lts);
      const bool authoritative_scalar_compare =
          authoritative_scalar_integer_compare ||
          authoritative_scalar_floating_compare;
      const LirOperand cmp_result = authoritative_scalar_compare
                                        ? fresh_value(ctx)
                                        : LirOperand(fresh_tmp(ctx));
      const LirOperand lhs = authoritative_scalar_compare &&
                                     source_lv.has_authority() &&
                                     source_lv.str() == lv
                                 ? source_lv
                                 : LirOperand(lv);
      const LirOperand rhs = authoritative_scalar_compare &&
                                     source_rv.has_authority() &&
                                     source_rv.str() == rv
                                 ? source_rv
                                 : LirOperand(rv);
      if (lf) {
        emit_lir_op(ctx, lir::LirCmpOp{cmp_result, true,
                                       LirCmpPredicateRef(row.f),
                                       LirTypeRef(op_ty), lhs, rhs});
      } else {
        const LirCmpPredicate pred = ls ? row.is : row.iu;
        const LirTypeRef comparison_type(op_ty);
        std::optional<lir::LirTruthinessComparisonLhsParameterAuthority>
            truthiness_lhs_authority;
        if (pred == LirCmpPredicate::Ne && comparison_type.kind() == LirTypeKind::Integer &&
            lhs.value_id() != nullptr && rhs.integer_immediate() != nullptr &&
            rhs.integer_immediate()->value == 0 && ctx.lir_function != nullptr) {
          const auto definition = std::find_if(
              ctx.lir_function->native_body_parameter_definitions.begin(),
              ctx.lir_function->native_body_parameter_definitions.end(),
              [&](const auto& candidate) {
                return candidate.value == *lhs.value_id() &&
                       candidate.type == comparison_type &&
                       candidate.owner == ctx.lir_function->link_name_id &&
                       candidate.abi == lir::LirNativeBodyParameterAbi::DirectScalar;
              });
          if (definition != ctx.lir_function->native_body_parameter_definitions.end()) {
            truthiness_lhs_authority = lir::LirTruthinessComparisonLhsParameterAuthority{
                .value = definition->value,
                .parameter_index = definition->parameter_index,
                .type = definition->type,
                .owner = definition->owner,
                .abi = definition->abi,
                .role = lir::LirTruthinessComparisonLhsParameterRole::TruthinessComparisonLhs,
            };
          }
        }
        emit_lir_op(ctx, lir::LirCmpOp{cmp_result, false,
                                       LirCmpPredicateRef(pred),
                                       comparison_type, lhs, rhs,
                                       truthiness_lhs_authority});
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt,
                                      LirTypeRef::integer(1), cmp_result,
                                      LirTypeRef::integer(32)});
      return tmp;
    }
  }

  throw std::runtime_error("StmtEmitter: unhandled binary op");
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const BinaryExpr& b,
                                           const Expr& e) {
  return emit_binary_rval_operand(ctx, b, e).str();
}

std::string StmtEmitter::emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e) {
  TypeSpec res_spec = resolve_expr_type(ctx, e);
  if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
  const std::string res_ty = llvm_ty(res_spec);
  TypeSpec lts{};
  const LirOperand lv = emit_rval_operand(ctx, b.lhs, lts);
  const LirOperand lc = to_bool_operand(ctx, lv, lts);

  const auto rhs_target = fresh_direct_target(ctx, fresh_lbl(ctx, "logic.rhs."));
  const auto skip_target = fresh_direct_target(ctx, fresh_lbl(ctx, "logic.skip."));
  const auto rhs_end_target = fresh_direct_target(ctx, fresh_lbl(ctx, "logic.rhs.end."));
  const auto end_target = fresh_direct_target(ctx, fresh_lbl(ctx, "logic.end."));

  if (b.op == BinaryOp::LAnd) {
    emit_condbr_and_open_lbl(ctx, lc, rhs_target, skip_target, rhs_target);
  } else {
    emit_condbr_and_open_lbl(ctx, lc, skip_target, rhs_target, rhs_target);
  }
  TypeSpec rts{};
  const LirOperand rv = emit_rval_operand(ctx, b.rhs, rts);
  const LirOperand rc = to_bool_operand(ctx, rv, rts);
  LirOperand rhs_val;
  if (res_ty == "i1") {
    rhs_val = rc;
  } else if (is_float_base(res_spec.base)) {
    const std::string as_i32 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{as_i32, lir::LirCastKind::ZExt, "i1", rc, "i32"});
    rhs_val = fresh_value(ctx);
    emit_lir_op(ctx,
                lir::LirCastOp{rhs_val, lir::LirCastKind::SIToFP, "i32", as_i32, res_ty});
  } else {
    const LirOperand rhs_result = fresh_value(ctx);
    emit_lir_op(ctx,
                lir::LirCastOp{rhs_result, lir::LirCastKind::ZExt, "i1", rc, res_ty, true});
    rhs_val = rhs_result;
  }
  emit_fallthrough_lbl(ctx, rhs_end_target);
  emit_br_and_open_lbl(ctx, end_target, skip_target);
  LirOperand skip_val;
  if (res_ty == "i1") {
    skip_val = LirOperand::special_token(
        b.op == BinaryOp::LAnd ? LirSpecialToken::False : LirSpecialToken::True);
  } else if (is_float_base(res_spec.base)) {
    skip_val = LirOperand((b.op == BinaryOp::LAnd) ? "0.0" : "1.0");
  } else {
    skip_val = LirOperand::integer((b.op == BinaryOp::LAnd) ? "0" : "1",
                                   b.op == BinaryOp::LAnd ? 0 : 1);
  }
  emit_fallthrough_lbl(ctx, end_target);
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(
      ctx, lir::LirPhiOp{tmp, res_ty,
                          {{rhs_val, rhs_end_target.label, rhs_end_target.id,
                            lir::LirSuccessorOccurrenceId::direct_branch()},
                           {skip_val, skip_target.label, skip_target.id,
                            lir::LirSuccessorOccurrenceId::direct_branch()}}});
  return tmp;
}

}  // namespace c4c::codegen::lir
