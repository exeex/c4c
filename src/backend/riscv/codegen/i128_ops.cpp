// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/i128_ops.rs
// This slice only wires the helper surface needed by the shared default-cast
// seam. Broader translated i128 owner work remains parked.

#include "riscv_codegen.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace c4c::backend::riscv::codegen {

namespace {

std::uint64_t next_i128_cmp_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

std::uint64_t next_i128_shift_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

const char* i128_to_float_helper(bool from_signed, IrType to_ty) {
  if (from_signed && to_ty == IrType::F64) {
    return "__floattidf";
  }
  if (from_signed && to_ty == IrType::F32) {
    return "__floattisf";
  }
  if (!from_signed && to_ty == IrType::F64) {
    return "__floatuntidf";
  }
  if (!from_signed && to_ty == IrType::F32) {
    return "__floatuntisf";
  }
  throw std::invalid_argument("unsupported i128-to-float conversion");
}

const char* float_to_i128_helper(bool to_signed, IrType from_ty) {
  if (to_signed && from_ty == IrType::F64) {
    return "__fixdfti";
  }
  if (to_signed && from_ty == IrType::F32) {
    return "__fixsfti";
  }
  if (!to_signed && from_ty == IrType::F64) {
    return "__fixunsdfti";
  }
  if (!to_signed && from_ty == IrType::F32) {
    return "__fixunssfti";
  }
  throw std::invalid_argument("unsupported float-to-i128 conversion");
}

}  // namespace

void RiscvCodegen::emit_load_acc_pair_impl(const Operand& src) {
  switch (src.kind) {
    case Operand::Kind::Value:
      if (const auto slot = state.get_slot(src.raw)) {
        emit_load_from_s0("t0", slot->raw, "ld");
        emit_load_from_s0("t1", slot->raw + 8, "ld");
        return;
      }
      if (const auto reg = state.assigned_reg(src.raw)) {
        state.emit("    mv t0, " + std::string(callee_saved_name(*reg)));
        state.emit("    li t1, 0");
        return;
      }
      break;
    case Operand::Kind::ImmI64:
      state.emit("    li t0, " + std::to_string(src.imm_i64));
      state.emit(std::string("    li t1, ") + (src.imm_i64 < 0 ? "-1" : "0"));
      return;
    case Operand::Kind::F32Const:
    case Operand::Kind::F64Const:
      operand_to_t0(src);
      state.emit("    li t1, 0");
      return;
  }

  state.emit("    li t0, 0");
  state.emit("    li t1, 0");
}

void RiscvCodegen::emit_store_acc_pair_impl(const Value& dest) {
  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("t0", slot->raw, "sd");
    emit_store_to_s0("t1", slot->raw + 8, "sd");
    return;
  }

  if (const auto reg = state.assigned_reg(dest.raw)) {
    state.emit("    mv " + std::string(callee_saved_name(*reg)) + ", t0");
  }
}

void RiscvCodegen::emit_store_pair_to_slot_impl(StackSlot slot) {
  emit_store_to_s0("t0", slot.raw, "sd");
  emit_store_to_s0("t1", slot.raw + 8, "sd");
}

void RiscvCodegen::emit_load_pair_from_slot_impl(StackSlot slot) {
  emit_load_from_s0("t0", slot.raw, "ld");
  emit_load_from_s0("t1", slot.raw + 8, "ld");
}

void RiscvCodegen::emit_save_acc_pair_impl() {
  state.emit("    mv t3, t0");
  state.emit("    mv t4, t1");
}

void RiscvCodegen::emit_store_pair_indirect_impl() {
  state.emit("    sd t3, 0(t5)");
  state.emit("    sd t4, 8(t5)");
}

void RiscvCodegen::emit_load_pair_indirect_impl() {
  state.emit("    ld t0, 0(t5)");
  state.emit("    ld t1, 8(t5)");
}

void RiscvCodegen::emit_i128_neg_impl() {
  state.emit("    not t0, t0");
  state.emit("    not t1, t1");
  state.emit("    addi t0, t0, 1");
  state.emit("    seqz t2, t0");
  state.emit("    add t1, t1, t2");
}

void RiscvCodegen::emit_i128_not_impl() {
  state.emit("    not t0, t0");
  state.emit("    not t1, t1");
}

void RiscvCodegen::emit_sign_extend_acc_high_impl() { state.emit("    srai t1, t0, 63"); }

void RiscvCodegen::emit_zero_acc_high_impl() { state.emit("    li t1, 0"); }

void RiscvCodegen::emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs) {
  emit_load_acc_pair_impl(lhs);
  state.emit("    mv t3, t0");
  state.emit("    mv t4, t1");
  switch (rhs.kind) {
    case Operand::Kind::Value:
      if (const auto slot = state.get_slot(rhs.raw)) {
        emit_load_from_s0("t5", slot->raw, "ld");
        emit_load_from_s0("t6", slot->raw + 8, "ld");
        return;
      }
      if (const auto reg = state.assigned_reg(rhs.raw)) {
        state.emit("    mv t5, " + std::string(callee_saved_name(*reg)));
        state.emit("    li t6, 0");
        return;
      }
      break;
    case Operand::Kind::ImmI64:
      state.emit("    li t5, " + std::to_string(rhs.imm_i64));
      state.emit(std::string("    li t6, ") + (rhs.imm_i64 < 0 ? "-1" : "0"));
      return;
    case Operand::Kind::F32Const:
    case Operand::Kind::F64Const:
      operand_to_t0(rhs);
      state.emit("    mv t5, t0");
      state.emit("    li t6, 0");
      return;
  }

  state.emit("    li t5, 0");
  state.emit("    li t6, 0");
}

void RiscvCodegen::emit_i128_add_impl() {
  state.emit("    add t0, t3, t5");
  state.emit("    sltu t2, t0, t3");
  state.emit("    add t1, t4, t6");
  state.emit("    add t1, t1, t2");
}

void RiscvCodegen::emit_i128_sub_impl() {
  state.emit("    sltu t2, t3, t5");
  state.emit("    sub t0, t3, t5");
  state.emit("    sub t1, t4, t6");
  state.emit("    sub t1, t1, t2");
}

void RiscvCodegen::emit_i128_mul_impl() {
  state.emit("    mul t0, t3, t5");
  state.emit("    mulhu t1, t3, t5");
  state.emit("    mul t2, t4, t5");
  state.emit("    add t1, t1, t2");
  state.emit("    mul t2, t3, t6");
  state.emit("    add t1, t1, t2");
}

void RiscvCodegen::emit_i128_and_impl() {
  state.emit("    and t0, t3, t5");
  state.emit("    and t1, t4, t6");
}

void RiscvCodegen::emit_i128_or_impl() {
  state.emit("    or t0, t3, t5");
  state.emit("    or t1, t4, t6");
}

void RiscvCodegen::emit_i128_xor_impl() {
  state.emit("    xor t0, t3, t5");
  state.emit("    xor t1, t4, t6");
}

void RiscvCodegen::emit_i128_shl_impl() {
  const auto label_id = next_i128_shift_label_id();
  const auto shift_label = ".Li128shl_" + std::to_string(label_id);
  const auto done_label = ".Li128shl_done_" + std::to_string(label_id);
  const auto noop_label = ".Li128shl_noop_" + std::to_string(label_id);

  state.emit("    andi t5, t5, 127");
  state.emit("    beqz t5, " + noop_label);
  state.emit("    li t2, 64");
  state.emit("    bge t5, t2, " + shift_label);
  state.emit("    sll t1, t4, t5");
  state.emit("    sub t2, t2, t5");
  state.emit("    srl t6, t3, t2");
  state.emit("    or t1, t1, t6");
  state.emit("    sll t0, t3, t5");
  state.emit("    j " + done_label);
  state.emit(shift_label + ":");
  state.emit("    li t2, 64");
  state.emit("    sub t5, t5, t2");
  state.emit("    sll t1, t3, t5");
  state.emit("    li t0, 0");
  state.emit("    j " + done_label);
  state.emit(noop_label + ":");
  state.emit("    mv t0, t3");
  state.emit("    mv t1, t4");
  state.emit(done_label + ":");
}

void RiscvCodegen::emit_i128_lshr_impl() {
  const auto label_id = next_i128_shift_label_id();
  const auto shift_label = ".Li128lshr_" + std::to_string(label_id);
  const auto done_label = ".Li128lshr_done_" + std::to_string(label_id);
  const auto noop_label = ".Li128lshr_noop_" + std::to_string(label_id);

  state.emit("    andi t5, t5, 127");
  state.emit("    beqz t5, " + noop_label);
  state.emit("    li t2, 64");
  state.emit("    bge t5, t2, " + shift_label);
  state.emit("    srl t0, t3, t5");
  state.emit("    sub t2, t2, t5");
  state.emit("    sll t6, t4, t2");
  state.emit("    or t0, t0, t6");
  state.emit("    srl t1, t4, t5");
  state.emit("    j " + done_label);
  state.emit(shift_label + ":");
  state.emit("    li t2, 64");
  state.emit("    sub t5, t5, t2");
  state.emit("    srl t0, t4, t5");
  state.emit("    li t1, 0");
  state.emit("    j " + done_label);
  state.emit(noop_label + ":");
  state.emit("    mv t0, t3");
  state.emit("    mv t1, t4");
  state.emit(done_label + ":");
}

void RiscvCodegen::emit_i128_ashr_impl() {
  const auto label_id = next_i128_shift_label_id();
  const auto shift_label = ".Li128ashr_" + std::to_string(label_id);
  const auto done_label = ".Li128ashr_done_" + std::to_string(label_id);
  const auto noop_label = ".Li128ashr_noop_" + std::to_string(label_id);

  state.emit("    andi t5, t5, 127");
  state.emit("    beqz t5, " + noop_label);
  state.emit("    li t2, 64");
  state.emit("    bge t5, t2, " + shift_label);
  state.emit("    srl t0, t3, t5");
  state.emit("    sub t2, t2, t5");
  state.emit("    sll t6, t4, t2");
  state.emit("    or t0, t0, t6");
  state.emit("    sra t1, t4, t5");
  state.emit("    j " + done_label);
  state.emit(shift_label + ":");
  state.emit("    li t2, 64");
  state.emit("    sub t5, t5, t2");
  state.emit("    sra t0, t4, t5");
  state.emit("    srai t1, t4, 63");
  state.emit("    j " + done_label);
  state.emit(noop_label + ":");
  state.emit("    mv t0, t3");
  state.emit("    mv t1, t4");
  state.emit(done_label + ":");
}

void RiscvCodegen::emit_i128_prep_shift_lhs_impl(const Operand& lhs) {
  emit_load_acc_pair_impl(lhs);
  state.emit("    mv t3, t0");
  state.emit("    mv t4, t1");
}

void RiscvCodegen::emit_i128_shl_const_impl(std::uint32_t amount) {
  amount &= 127;
  if (amount == 0) {
    state.emit("    mv t0, t3");
    state.emit("    mv t1, t4");
  } else if (amount == 64) {
    state.emit("    mv t1, t3");
    state.emit("    li t0, 0");
  } else if (amount > 64) {
    state.emit("    slli t1, t3, " + std::to_string(amount - 64));
    state.emit("    li t0, 0");
  } else {
    state.emit("    slli t1, t4, " + std::to_string(amount));
    state.emit("    srli t2, t3, " + std::to_string(64 - amount));
    state.emit("    or t1, t1, t2");
    state.emit("    slli t0, t3, " + std::to_string(amount));
  }
}

void RiscvCodegen::emit_i128_lshr_const_impl(std::uint32_t amount) {
  amount &= 127;
  if (amount == 0) {
    state.emit("    mv t0, t3");
    state.emit("    mv t1, t4");
  } else if (amount == 64) {
    state.emit("    mv t0, t4");
    state.emit("    li t1, 0");
  } else if (amount > 64) {
    state.emit("    srli t0, t4, " + std::to_string(amount - 64));
    state.emit("    li t1, 0");
  } else {
    state.emit("    srli t0, t3, " + std::to_string(amount));
    state.emit("    slli t2, t4, " + std::to_string(64 - amount));
    state.emit("    or t0, t0, t2");
    state.emit("    srli t1, t4, " + std::to_string(amount));
  }
}

void RiscvCodegen::emit_i128_ashr_const_impl(std::uint32_t amount) {
  amount &= 127;
  if (amount == 0) {
    state.emit("    mv t0, t3");
    state.emit("    mv t1, t4");
  } else if (amount == 64) {
    state.emit("    mv t0, t4");
    state.emit("    srai t1, t4, 63");
  } else if (amount > 64) {
    state.emit("    srai t0, t4, " + std::to_string(amount - 64));
    state.emit("    srai t1, t4, 63");
  } else {
    state.emit("    srli t0, t3, " + std::to_string(amount));
    state.emit("    slli t2, t4, " + std::to_string(64 - amount));
    state.emit("    or t0, t0, t2");
    state.emit("    srai t1, t4, " + std::to_string(amount));
  }
}

void RiscvCodegen::emit_i128_divrem_call_impl(const std::string& func_name,
                                              const Operand& lhs,
                                              const Operand& rhs) {
  emit_load_acc_pair_impl(lhs);
  state.emit("    mv a0, t0");
  state.emit("    mv a1, t1");
  emit_load_acc_pair_impl(rhs);
  state.emit("    mv a2, t0");
  state.emit("    mv a3, t1");
  state.emit("    call " + func_name);
  state.emit("    mv t0, a0");
  state.emit("    mv t1, a1");
}

void RiscvCodegen::emit_i128_store_result_impl(const Value& dest) { emit_store_acc_pair_impl(dest); }

void RiscvCodegen::emit_i128_to_float_call_impl(const Operand& src,
                                                bool from_signed,
                                                IrType to_ty) {
  emit_load_acc_pair_impl(src);
  state.emit("    mv a0, t0");
  state.emit("    mv a1, t1");
  state.emit(std::string("    call ") + i128_to_float_helper(from_signed, to_ty));
  state.emit(std::string("    fmv.x.") + (to_ty == IrType::F32 ? "w" : "d") + " t0, fa0");
}

void RiscvCodegen::emit_float_to_i128_call_impl(const Operand& src,
                                                bool to_signed,
                                                IrType from_ty) {
  operand_to_t0(src);
  state.emit(std::string("    fmv.") + (from_ty == IrType::F32 ? "w" : "d") + ".x fa0, t0");
  state.emit(std::string("    call ") + float_to_i128_helper(to_signed, from_ty));
  state.emit("    mv t0, a0");
  state.emit("    mv t1, a1");
}

void RiscvCodegen::emit_i128_cmp_eq_impl(bool is_ne) {
  state.emit("    xor t0, t3, t5");
  state.emit("    xor t1, t4, t6");
  state.emit("    or t0, t0, t1");
  state.emit(std::string("    ") + (is_ne ? "snez" : "seqz") + " t0, t0");
}

void RiscvCodegen::emit_i128_cmp_ordered_impl(IrCmpOp op) {
  if (op == IrCmpOp::Eq || op == IrCmpOp::Ne) {
    throw std::invalid_argument("i128 ordered cmp got equality op");
  }

  const auto label_id = next_i128_cmp_label_id();
  const auto true_label = ".Li128cmp_true_" + std::to_string(label_id);
  const auto false_label = ".Li128cmp_false_" + std::to_string(label_id);
  const auto done_label = ".Li128cmp_done_" + std::to_string(label_id);

  switch (op) {
    case IrCmpOp::Slt:
      state.emit("    blt t1, t6, " + true_label);
      state.emit("    blt t6, t1, " + false_label);
      state.emit("    bltu t0, t5, " + true_label);
      break;
    case IrCmpOp::Sle:
      state.emit("    blt t1, t6, " + true_label);
      state.emit("    blt t6, t1, " + false_label);
      state.emit("    bgeu t5, t0, " + true_label);
      break;
    case IrCmpOp::Sgt:
      state.emit("    blt t6, t1, " + true_label);
      state.emit("    blt t1, t6, " + false_label);
      state.emit("    bltu t5, t0, " + true_label);
      break;
    case IrCmpOp::Sge:
      state.emit("    blt t1, t6, " + false_label);
      state.emit("    blt t6, t1, " + true_label);
      state.emit("    bgeu t0, t5, " + true_label);
      break;
    case IrCmpOp::Ult:
      state.emit("    bltu t1, t6, " + true_label);
      state.emit("    bltu t6, t1, " + false_label);
      state.emit("    bltu t0, t5, " + true_label);
      break;
    case IrCmpOp::Ule:
      state.emit("    bltu t1, t6, " + true_label);
      state.emit("    bltu t6, t1, " + false_label);
      state.emit("    bgeu t5, t0, " + true_label);
      break;
    case IrCmpOp::Ugt:
      state.emit("    bltu t6, t1, " + true_label);
      state.emit("    bltu t1, t6, " + false_label);
      state.emit("    bltu t5, t0, " + true_label);
      break;
    case IrCmpOp::Uge:
      state.emit("    bltu t1, t6, " + false_label);
      state.emit("    bltu t6, t1, " + true_label);
      state.emit("    bgeu t0, t5, " + true_label);
      break;
    default:
      break;
  }

  state.emit("    j " + false_label);
  state.emit(true_label + ":");
  state.emit("    li t0, 1");
  state.emit("    j " + done_label);
  state.emit(false_label + ":");
  state.emit("    li t0, 0");
  state.emit(done_label + ":");
}

void RiscvCodegen::emit_i128_cmp_store_result_impl(const Value& dest) { store_t0_to(dest); }

}  // namespace c4c::backend::riscv::codegen
