// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/alu.rs
// Structural mirror of the ref Rust source. This slice makes the first broader
// integer-owner packet real without widening into comparison or call work.

#include "riscv_codegen.hpp"

namespace c4c::backend::riscv::codegen {

namespace {

constexpr const char* riscv_int_binop_mnemonic(IrBinOp op, bool use_32bit) {
  switch (op) {
    case IrBinOp::Add: return use_32bit ? "addw" : "add";
    case IrBinOp::Sub: return use_32bit ? "subw" : "sub";
    case IrBinOp::Mul: return use_32bit ? "mulw" : "mul";
    case IrBinOp::SDiv: return use_32bit ? "divw" : "div";
    case IrBinOp::UDiv: return use_32bit ? "divuw" : "divu";
    case IrBinOp::SRem: return use_32bit ? "remw" : "rem";
    case IrBinOp::URem: return use_32bit ? "remuw" : "remu";
    case IrBinOp::And: return "and";
    case IrBinOp::Or: return "or";
    case IrBinOp::Xor: return "xor";
    case IrBinOp::Shl: return use_32bit ? "sllw" : "sll";
    case IrBinOp::AShr: return use_32bit ? "sraw" : "sra";
    case IrBinOp::LShr: return use_32bit ? "srlw" : "srl";
  }
  return "add";
}

}  // namespace

void RiscvCodegen::emit_float_neg_impl(IrType ty) {
  if (ty == IrType::F64) {
    state.emit("    fmv.d.x ft0, t0");
    state.emit("    fneg.d ft0, ft0");
    state.emit("    fmv.x.d t0, ft0");
    return;
  }

  state.emit("    fmv.w.x ft0, t0");
  state.emit("    fneg.s ft0, ft0");
  state.emit("    fmv.x.w t0, ft0");
}

void RiscvCodegen::emit_int_neg_impl(IrType /*ty*/) { state.emit("    neg t0, t0"); }

void RiscvCodegen::emit_int_not_impl(IrType /*ty*/) { state.emit("    not t0, t0"); }

void RiscvCodegen::emit_int_binop_impl(const Value& dest,
                                       IrBinOp op,
                                       const Operand& lhs,
                                       const Operand& rhs,
                                       IrType ty) {
  operand_to_t0(lhs);
  state.emit("    mv t1, t0");
  operand_to_t0(rhs);
  state.emit("    mv t2, t0");

  const bool use_32bit = ty == IrType::I32 || ty == IrType::U32;
  state.emit(std::string("    ") + riscv_int_binop_mnemonic(op, use_32bit) + " t0, t1, t2");
  store_t0_to(dest);
}

void RiscvCodegen::emit_copy_i128_impl(const Value& dest, const Operand& src) {
  emit_load_acc_pair_impl(src);
  emit_store_acc_pair_impl(dest);
}

}  // namespace c4c::backend::riscv::codegen
