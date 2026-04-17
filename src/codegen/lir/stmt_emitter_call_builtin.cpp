#include "stmt_emitter.hpp"
#include "call_args_ops.hpp"

namespace c4c::codegen::lir {

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
  emit_lir_op(ctx, make_lir_call_op(cttz, arg.llvm_ty, "@llvm.cttz." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, arg.value}, {"i1", "false"}}));
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
  emit_lir_op(ctx, make_lir_call_op(tmp, arg.llvm_ty, "@llvm.cttz." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, arg.value}, {"i1", "true"}}));
  return narrow_builtin_int_result(ctx, arg, tmp);
}

std::string StmtEmitter::emit_builtin_clz_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id) {
  const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(tmp, arg.llvm_ty, "@llvm.ctlz." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, arg.value}, {"i1", "true"}}));
  return narrow_builtin_int_result(ctx, arg, tmp);
}

std::string StmtEmitter::emit_builtin_popcount_call(FnCtx& ctx, ExprId arg_id,
                                                    BuiltinId builtin_id) {
  const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(tmp, arg.llvm_ty, "@llvm.ctpop." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, arg.value}}));
  return narrow_builtin_int_result(ctx, arg, tmp);
}

std::string StmtEmitter::emit_builtin_parity_call(FnCtx& ctx, ExprId arg_id,
                                                  BuiltinId builtin_id) {
  const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
  const std::string pop = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(pop, arg.llvm_ty, "@llvm.ctpop." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, arg.value}}));
  const std::string parity = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{parity, "and", arg.llvm_ty, pop, "1"});
  return narrow_builtin_int_result(ctx, arg, parity);
}

std::string StmtEmitter::emit_builtin_clrsb_call(FnCtx& ctx, ExprId arg_id,
                                                 BuiltinId builtin_id) {
  const PreparedBuiltinIntArg arg = prepare_builtin_int_arg(ctx, arg_id, builtin_id);
  const int bitwidth = arg.is_i64 ? 64 : 32;
  const std::string shift = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{shift, "ashr", arg.llvm_ty, arg.value, std::to_string(bitwidth - 1)});
  const std::string xored = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{xored, "xor", arg.llvm_ty, arg.value, shift});
  const std::string clz = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(clz, arg.llvm_ty, "@llvm.ctlz." + arg.llvm_ty, "",
                                    {{arg.llvm_ty, xored}, {"i1", "false"}}));
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
  emit_lir_op(ctx, make_lir_call_op(abs_value, fp_ty, "@llvm.fabs." + fp_ty, "", {{fp_ty, arg}}));
  return emit_builtin_fp_predicate_result(ctx, "oeq", fp_ty, abs_value, inf_val);
}

std::string StmtEmitter::emit_builtin_isfinite_call(FnCtx& ctx, ExprId arg_id) {
  TypeSpec arg_ts{};
  std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
  promote_builtin_fp_predicate_arg(ctx, arg, arg_ts);
  const std::string fp_ty = llvm_ty(arg_ts);
  const std::string inf_val = fp_literal(arg_ts.base, std::numeric_limits<double>::infinity());
  const std::string abs_value = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(abs_value, fp_ty, "@llvm.fabs." + fp_ty, "", {{fp_ty, arg}}));
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

  const std::string long_double_ty = llvm_helpers::llvm_long_double_ty(mod_.target_profile);

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
        cast_fp_arg("float", long_double_ty, lir::LirCastKind::FPExt, TB_LONGDOUBLE);
      } else if (value_ts.base == TB_DOUBLE) {
        cast_fp_arg("double", long_double_ty, lir::LirCastKind::FPExt, TB_LONGDOUBLE);
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
  emit_lir_op(ctx, make_lir_call_op(tmp, fp_ty, intrinsic, "", {{fp_ty, lhs}, {fp_ty, rhs}}));
  return tmp;
}

std::string StmtEmitter::emit_builtin_fabs_call(FnCtx& ctx, ExprId arg_id,
                                                BuiltinId builtin_id) {
  TypeSpec arg_ts{};
  std::string arg = emit_rval_id(ctx, arg_id, arg_ts);
  promote_builtin_fp_math_arg(ctx, arg, arg_ts, builtin_id);

  const std::string fp_ty = llvm_ty(arg_ts);
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, make_lir_call_op(tmp, fp_ty, "@llvm.fabs." + fp_ty, "", {{fp_ty, arg}}));
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
    const std::string long_double_ty = llvm_helpers::llvm_long_double_ty(mod_.target_profile);
    if (long_double_ty == "x86_fp80") {
      int_ty = "i80";
      shift_bits = 79;
    } else {
      int_ty = "i128";
      shift_bits = 127;
    }
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

  if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 &&
      (call_target.fn_name == "abs" || call_target.fn_name == "labs" ||
       call_target.fn_name == "llabs")) {
    TypeSpec arg_ts{};
    std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
    const bool is_ll = (call_target.fn_name == "llabs" || call_target.fn_name == "labs");
    const std::string ity = is_ll ? "i64" : "i32";
    TypeSpec target_ts{};
    target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
    arg = coerce(ctx, arg, arg_ts, target_ts);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirAbsOp{tmp, arg, ity});
    module_->need_abs = true;
    return tmp;
  }

  if (builtin_id == BuiltinId::Unknown && call.args.size() == 1 && call_target.fn_name == "alloca") {
    TypeSpec size_ts{};
    std::string size = emit_rval_id(ctx, call.args[0], size_ts);
    TypeSpec i64_ts{};
    i64_ts.base = TB_ULONGLONG;
    size = coerce(ctx, size, size_ts, i64_ts);
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirAllocaOp{tmp, "i8", size, 16});
    return tmp;
  }

  const auto args = prepare_call_args(ctx, call, call_target);
  if (call_target.ret_ty == "void") {
    emit_void_call(ctx, call_target, args);
    return "";
  }
  return emit_call_with_result(ctx, call_target, args);
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e) {
  const CallTargetInfo call_target = resolve_call_target_info(ctx, call, e);
  const BuiltinId builtin_id = call_target.builtin_id;

  if (builtin_id == BuiltinId::Unknown && has_builtin_prefix(call_target.fn_name)) {
    throw std::runtime_error("StmtEmitter: unsupported builtin call: " + call_target.fn_name);
  }

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
      TypeSpec i64_ts{};
      i64_ts.base = TB_ULONGLONG;
      size = coerce(ctx, size, size_ts, i64_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAllocaOp{tmp, "i8", size, 16});
      return tmp;
    }
    if (builtin_id == BuiltinId::ConstantP) {
      for (auto& a : call.args) {
        TypeSpec dummy{};
        emit_rval_id(ctx, a, dummy);
      }
      return "0";
    }
    if (builtin_id == BuiltinId::ClassifyType && call.args.size() == 1) {
      TypeSpec arg_ts{};
      emit_rval_id(ctx, call.args[0], arg_ts);
      int type_class = 5;
      if (arg_ts.ptr_level > 0 || arg_ts.is_fn_ptr) {
        type_class = 5;
      } else if (is_complex_base(arg_ts.base)) {
        type_class = 9;
      } else if (is_float_base(arg_ts.base)) {
        type_class = 8;
      } else if (arg_ts.base == TB_VOID) {
        type_class = 0;
      } else if (arg_ts.base == TB_BOOL || arg_ts.base == TB_CHAR || arg_ts.base == TB_SCHAR ||
                 arg_ts.base == TB_UCHAR) {
        type_class = 1;
      } else if (is_any_int(arg_ts.base)) {
        type_class = 1;
      } else if (arg_ts.base == TB_STRUCT) {
        type_class = 12;
      } else if (arg_ts.base == TB_UNION) {
        type_class = 13;
      }
      return std::to_string(type_class);
    }
    if ((builtin_id == BuiltinId::AddOverflow || builtin_id == BuiltinId::SubOverflow ||
         builtin_id == BuiltinId::MulOverflow) &&
        call.args.size() == 3) {
      TypeSpec a_ts{}, b_ts{}, p_ts{};
      std::string a = emit_rval_id(ctx, call.args[0], a_ts);
      std::string b = emit_rval_id(ctx, call.args[1], b_ts);
      const std::string result_ptr = emit_rval_id(ctx, call.args[2], p_ts);
      TypeSpec res_ts = p_ts;
      res_ts.ptr_level--;
      const std::string res_ty = llvm_ty(res_ts);
      a = coerce(ctx, a, a_ts, res_ts);
      b = coerce(ctx, b, b_ts, res_ts);
      const bool is_signed = is_signed_int(res_ts.base);
      std::string op;
      if (builtin_id == BuiltinId::AddOverflow) op = is_signed ? "sadd" : "uadd";
      else if (builtin_id == BuiltinId::SubOverflow) op = is_signed ? "ssub" : "usub";
      else op = is_signed ? "smul" : "umul";
      const std::string intrinsic = "@llvm." + op + ".with.overflow." + res_ty;
      const std::string pair = fresh_tmp(ctx);
      emit_lir_op(ctx, make_lir_call_op(pair, "{ " + res_ty + ", i1 }", intrinsic, "",
                                        {{res_ty, a}, {res_ty, b}}));
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
        emit_lir_op(ctx, make_lir_call_op(tmp, aty, intrinsic, "", {{aty, arg}}));
        return tmp;
      }
    }
    if (builtin_id == BuiltinId::Expect && call.args.size() >= 1) {
      TypeSpec arg_ts{};
      std::string result = emit_rval_id(ctx, call.args[0], arg_ts);
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
        if (auto base =
                fp_base_from_builtin_result_kind(builtin_result_kind(builtin_id))) {
          return fp_literal(*base, std::numeric_limits<double>::infinity());
        }
        break;
      case BuiltinConstantFpKind::QuietNaN:
        if (builtin_id == BuiltinId::NanF || builtin_id == BuiltinId::NansF) {
          return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
        }
        if (auto base =
                fp_base_from_builtin_result_kind(builtin_result_kind(builtin_id))) {
          return fp_literal(*base, std::numeric_limits<double>::quiet_NaN());
        }
        break;
      case BuiltinConstantFpKind::None:
        break;
    }
    if ((builtin_id == BuiltinId::SignBit || builtin_id == BuiltinId::SignBitF ||
         builtin_id == BuiltinId::SignBitL) &&
        call.args.size() == 1) {
      return emit_builtin_signbit_call(ctx, call.args[0], builtin_id);
    }
    if (const char* pred = builtin_fp_compare_predicate(builtin_id); pred && call.args.size() == 2) {
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
         builtin_id == BuiltinId::CopysignF) &&
        call.args.size() == 2) {
      return emit_builtin_copysign_call(ctx, call, builtin_id);
    }
    if ((builtin_id == BuiltinId::Fabs || builtin_id == BuiltinId::FabsL ||
         builtin_id == BuiltinId::FabsF) &&
        call.args.size() == 1) {
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
         builtin_id == BuiltinId::ConjL) &&
        call.args.size() == 1) {
      return emit_builtin_conj_call(ctx, call.args[0]);
    }
  }
  return emit_post_builtin_call(ctx, call, call_target);
}

}  // namespace c4c::codegen::lir
