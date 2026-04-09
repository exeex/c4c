// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs
//
// The shared RISC-V C++ codegen surface does not exist yet, so this file keeps
// the comparison logic local and self-contained. The Rust impl methods are
// represented here as concrete helper functions that preserve the opcode and
// branch-selection rules.

#include <string_view>

namespace c4c::backend::riscv::codegen {

enum class IrCmpOp {
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};

enum class IrType {
  I32,
  U32,
  I64,
  U64,
  F32,
  F64,
  F128,
};

struct FloatComparePlan {
  std::string_view suffix;
  std::string_view move_from_int;
  std::string_view predicate;
  bool swap_operands = false;
  bool invert = false;
};

struct IntComparePlan {
  std::string_view primary;
  std::string_view secondary;
  bool swap_operands = false;
};

struct FusedCmpBranchPlan {
  std::string_view inverted_branch;
  std::string_view lhs_reg;
  std::string_view rhs_reg;
};

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

constexpr IntComparePlan riscv_int_cmp_plan(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq:
      return {"sub", "seqz", false};
    case IrCmpOp::Ne:
      return {"sub", "snez", false};
    case IrCmpOp::Slt:
      return {"slt", "", false};
    case IrCmpOp::Ult:
      return {"sltu", "", false};
    case IrCmpOp::Sge:
      return {"slt", "xori", false};
    case IrCmpOp::Uge:
      return {"sltu", "xori", false};
    case IrCmpOp::Sgt:
      return {"slt", "", true};
    case IrCmpOp::Ugt:
      return {"sltu", "", true};
    case IrCmpOp::Sle:
      return {"slt", "xori", true};
    case IrCmpOp::Ule:
      return {"sltu", "xori", true};
  }
  return {"slt", "", false};
}

constexpr FusedCmpBranchPlan riscv_fused_cmp_branch_plan(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq:
      return {"bne", "t1", "t2"};
    case IrCmpOp::Ne:
      return {"beq", "t1", "t2"};
    case IrCmpOp::Slt:
      return {"bge", "t1", "t2"};
    case IrCmpOp::Sge:
      return {"blt", "t1", "t2"};
    case IrCmpOp::Ult:
      return {"bgeu", "t1", "t2"};
    case IrCmpOp::Uge:
      return {"bltu", "t1", "t2"};
    case IrCmpOp::Sgt:
      return {"bge", "t2", "t1"};
    case IrCmpOp::Sle:
      return {"blt", "t2", "t1"};
    case IrCmpOp::Ugt:
      return {"bgeu", "t2", "t1"};
    case IrCmpOp::Ule:
      return {"bltu", "t2", "t1"};
  }
  return {"bne", "t1", "t2"};
}

constexpr std::string_view riscv_select_skip_label_prefix() {
  return ".Lsel_skip_";
}

// ---- Rust method mirrors ---------------------------------------------------
//
// pub(super) fn emit_float_cmp_impl(&mut self, dest: &Value, op: IrCmpOp,
//                                   lhs: &Operand, rhs: &Operand, ty: IrType)
// pub(super) fn emit_int_cmp_impl(&mut self, dest: &Value, op: IrCmpOp,
//                                 lhs: &Operand, rhs: &Operand, ty: IrType)
// pub(super) fn emit_fused_cmp_branch_impl(&mut self, op: IrCmpOp,
//                                          lhs: &Operand, rhs: &Operand,
//                                          ty: IrType, true_label: &str,
//                                          false_label: &str)
// pub(super) fn emit_select_impl(&mut self, dest: &Value, cond: &Operand,
//                                true_val: &Operand, false_val: &Operand,
//                                _ty: IrType)
// pub(super) fn emit_f128_cmp_impl(&mut self, dest: &Value, op: IrCmpOp,
//                                  lhs: &Operand, rhs: &Operand)
//
// The actual method bodies require the shared RiscvCodegen / Operand / Value
// surface, which is still being translated file-by-file.

}  // namespace c4c::backend::riscv::codegen

