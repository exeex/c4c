// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs
//
// The bounded non-F128 float-compare seam now compiles against the shared RV64
// codegen surface while broader comparison owner work stays parked.

#include "riscv_codegen.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

namespace {

std::uint64_t next_riscv_cmp_control_flow_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

constexpr std::string_view riscv_inverted_branch_opcode(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq:
      return "bne";
    case IrCmpOp::Ne:
      return "beq";
    case IrCmpOp::Slt:
      return "bge";
    case IrCmpOp::Sge:
      return "blt";
    case IrCmpOp::Ult:
      return "bgeu";
    case IrCmpOp::Uge:
      return "bltu";
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
      return op == IrCmpOp::Sgt ? "bge" : "bgeu";
    case IrCmpOp::Sle:
    case IrCmpOp::Ule:
      return op == IrCmpOp::Sle ? "blt" : "bltu";
  }
  return "bne";
}

constexpr bool riscv_cmp_branch_swaps_operands(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
    case IrCmpOp::Sle:
    case IrCmpOp::Ule:
      return true;
    default:
      return false;
  }
}

}  // namespace

void RiscvCodegen::emit_cmp_operand_load(const Operand& lhs, const Operand& rhs, IrType ty) {
  operand_to_t0(lhs);
  state.emit("    mv t1, t0");
  operand_to_t0(rhs);
  state.emit("    mv t2, t0");

  switch (ty) {
    case IrType::U16:
      state.emit("    slli t1, t1, 48");
      state.emit("    srli t1, t1, 48");
      state.emit("    slli t2, t2, 48");
      state.emit("    srli t2, t2, 48");
      break;
    case IrType::U32:
      state.emit("    slli t1, t1, 32");
      state.emit("    srli t1, t1, 32");
      state.emit("    slli t2, t2, 32");
      state.emit("    srli t2, t2, 32");
      break;
    case IrType::I8:
      state.emit("    slli t1, t1, 56");
      state.emit("    srai t1, t1, 56");
      state.emit("    slli t2, t2, 56");
      state.emit("    srai t2, t2, 56");
      break;
    case IrType::I16:
      state.emit("    slli t1, t1, 48");
      state.emit("    srai t1, t1, 48");
      state.emit("    slli t2, t2, 48");
      state.emit("    srai t2, t2, 48");
      break;
    case IrType::I32:
      state.emit("    sext.w t1, t1");
      state.emit("    sext.w t2, t2");
      break;
    default:
      break;
  }
}

constexpr std::string_view riscv_float_suffix(IrType ty) {
  return ty == IrType::F64 ? "d" : "s";
}

constexpr std::string_view riscv_float_move_opcode(IrType ty) {
  return ty == IrType::F64 ? "fmv.d.x" : "fmv.w.x";
}

constexpr std::string_view riscv_float_predicate(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq:
      return "feq";
    case IrCmpOp::Ne:
      return "feq";
    case IrCmpOp::Slt:
    case IrCmpOp::Ult:
      return "flt";
    case IrCmpOp::Sle:
    case IrCmpOp::Ule:
      return "fle";
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
      return "flt";
    case IrCmpOp::Sge:
    case IrCmpOp::Uge:
      return "fle";
  }
  return "feq";
}

constexpr std::string_view riscv_float_operand_order(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
    case IrCmpOp::Sge:
    case IrCmpOp::Uge:
      return "swap";
    default:
      return "direct";
  }
}

void RiscvCodegen::emit_int_cmp_impl(const Value& dest,
                                     IrCmpOp op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty) {
  emit_cmp_operand_load(lhs, rhs, ty);

  switch (op) {
    case IrCmpOp::Eq:
      state.emit("    sub t0, t1, t2");
      state.emit("    seqz t0, t0");
      break;
    case IrCmpOp::Ne:
      state.emit("    sub t0, t1, t2");
      state.emit("    snez t0, t0");
      break;
    case IrCmpOp::Slt:
      state.emit("    slt t0, t1, t2");
      break;
    case IrCmpOp::Ult:
      state.emit("    sltu t0, t1, t2");
      break;
    case IrCmpOp::Sge:
      state.emit("    slt t0, t1, t2");
      state.emit("    xori t0, t0, 1");
      break;
    case IrCmpOp::Uge:
      state.emit("    sltu t0, t1, t2");
      state.emit("    xori t0, t0, 1");
      break;
    case IrCmpOp::Sgt:
      state.emit("    slt t0, t2, t1");
      break;
    case IrCmpOp::Ugt:
      state.emit("    sltu t0, t2, t1");
      break;
    case IrCmpOp::Sle:
      state.emit("    slt t0, t2, t1");
      state.emit("    xori t0, t0, 1");
      break;
    case IrCmpOp::Ule:
      state.emit("    sltu t0, t2, t1");
      state.emit("    xori t0, t0, 1");
      break;
  }

  store_t0_to(dest);
}

void RiscvCodegen::emit_fused_cmp_branch_impl(IrCmpOp op,
                                              const Operand& lhs,
                                              const Operand& rhs,
                                              IrType ty,
                                              const std::string& true_label,
                                              const std::string& false_label) {
  emit_cmp_operand_load(lhs, rhs, ty);

  const auto skip_label =
      ".Lcmp_skip_" + std::to_string(next_riscv_cmp_control_flow_label_id());
  const auto lhs_reg = riscv_cmp_branch_swaps_operands(op) ? "t2" : "t1";
  const auto rhs_reg = riscv_cmp_branch_swaps_operands(op) ? "t1" : "t2";
  state.emit("    " + std::string(riscv_inverted_branch_opcode(op)) + " " + lhs_reg + ", " +
             rhs_reg + ", " + skip_label);
  state.emit("    j " + true_label);
  state.emit(skip_label + ":");
  state.emit("    j " + false_label);
}

void RiscvCodegen::emit_select_impl(const Value& dest,
                                    const Operand& cond,
                                    const Operand& true_val,
                                    const Operand& false_val,
                                    IrType /*ty*/) {
  const auto skip_label =
      ".Lsel_skip_" + std::to_string(next_riscv_cmp_control_flow_label_id());

  operand_to_t0(false_val);
  state.emit("    mv t2, t0");

  operand_to_t0(cond);
  state.emit("    beqz t0, " + skip_label);

  operand_to_t0(true_val);
  state.emit("    mv t2, t0");

  state.emit(skip_label + ":");
  state.emit("    mv t0, t2");
  store_t0_to(dest);
}

void RiscvCodegen::emit_float_cmp_impl(const Value& dest,
                                       IrCmpOp op,
                                       const Operand& lhs,
                                       const Operand& rhs,
                                       IrType ty) {
  if (ty == IrType::F128) {
    emit_f128_cmp_impl(dest, op, lhs, rhs);
    return;
  }

  const auto suffix = riscv_float_suffix(ty);
  const auto move_opcode = riscv_float_move_opcode(ty);
  const auto predicate = riscv_float_predicate(op);
  const auto swap_operands = riscv_float_operand_order(op) == "swap";

  operand_to_t0(lhs);
  state.emit("    mv t1, t0");
  operand_to_t0(rhs);
  state.emit("    mv t2, t0");
  state.emit("    " + std::string(move_opcode) + " ft0, t1");
  state.emit("    " + std::string(move_opcode) + " ft1, t2");

  const auto lhs_reg = swap_operands ? "ft1" : "ft0";
  const auto rhs_reg = swap_operands ? "ft0" : "ft1";
  state.emit("    " + std::string(predicate) + "." + std::string(suffix) + " t0, " + lhs_reg + ", " +
             rhs_reg);

  if (op == IrCmpOp::Ne) {
    state.emit("    xori t0, t0, 1");
  }

  store_t0_to(dest);
}

}  // namespace c4c::backend::riscv::codegen
