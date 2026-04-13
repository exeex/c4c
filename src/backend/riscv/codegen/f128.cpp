// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/f128.rs
// Structural mirror of the ref Rust source. This slice makes the bounded
// return-operand helper real without claiming the full RV64 soft-float owner
// or shared orchestration surface is wired yet.
//
// //! RISC-V F128 (quad-precision / long double) soft-float helpers.
// //!
// //! IEEE 754 binary128 operations via compiler-rt/libgcc soft-float libcalls.
// //! RISC-V LP64D ABI: f128 passed in GP register pairs (a0:a1, a2:a3).
// //!
// //! This file implements the `F128SoftFloat` trait for RISC-V, providing the
// //! arch-specific primitives (GP register pair representation, instruction
// //! mnemonics, S0-relative addressing). The shared orchestration logic lives
// //! in `backend/f128_softfloat.rs`.
//
// use crate::ir::reexports::{Operand, Value};
// use crate::common::types::IrType;
// use crate::backend::state::{StackSlot, SlotAddr};
// use crate::backend::traits::ArchCodegen;
// use crate::backend::f128_softfloat::F128SoftFloat;
// use super::emit::{RiscvCodegen, callee_saved_name};
//
// impl F128SoftFloat for RiscvCodegen {
//     fn state(&mut self) -> &mut crate::backend::state::CodegenState { ... }
//     fn f128_get_slot(&self, val_id: u32) -> Option<StackSlot> { ... }
//     fn f128_get_source(&self, val_id: u32) -> Option<(u32, i64, bool)> { ... }
//     fn f128_resolve_slot_addr(&self, val_id: u32) -> Option<SlotAddr> { ... }
//     fn f128_load_const_to_arg1(&mut self, lo: u64, hi: u64) { ... }
//     fn f128_load_16b_from_addr_reg_to_arg1(&mut self) { ... }
//     fn f128_load_from_frame_offset_to_arg1(&mut self, offset: i64) { ... }
//     fn f128_load_ptr_to_addr_reg(&mut self, slot: StackSlot, val_id: u32) { ... }
//     fn f128_add_offset_to_addr_reg(&mut self, offset: i64) { ... }
//     fn f128_alloca_aligned_addr(&mut self, slot: StackSlot, val_id: u32) { ... }
//     fn f128_load_operand_and_extend(&mut self, op: &Operand) { ... }
//     fn f128_move_arg1_to_arg2(&mut self) { ... }
//     fn f128_save_arg1_to_sp(&mut self) { ... }
//     fn f128_reload_arg1_from_sp(&mut self) { ... }
//     fn f128_alloc_temp_16(&mut self) { ... }
//     fn f128_free_temp_16(&mut self) { ... }
//     fn f128_call(&mut self, name: &str) { ... }
//     fn f128_truncate_result_to_acc(&mut self) { ... }
//     fn f128_store_const_halves_to_slot(&mut self, lo: u64, hi: u64, slot: StackSlot) { ... }
//     fn f128_store_arg1_to_slot(&mut self, slot: StackSlot) { ... }
//     fn f128_copy_slot_to_slot(&mut self, src_offset: i64, dest_slot: StackSlot) { ... }
//     fn f128_copy_addr_reg_to_slot(&mut self, dest_slot: StackSlot) { ... }
//     fn f128_store_const_halves_to_addr(&mut self, lo: u64, hi: u64) { ... }
//     fn f128_save_addr_reg(&mut self) { ... }
//     fn f128_copy_slot_to_saved_addr(&mut self, src_offset: i64) { ... }
//     fn f128_copy_addr_reg_to_saved_addr(&mut self) { ... }
//     fn f128_store_arg1_to_saved_addr(&mut self) { ... }
//     fn f128_flip_sign_bit(&mut self) { ... }
//     fn f128_cmp_result_to_bool(&mut self, kind: crate::backend::cast::F128CmpKind) { ... }
//     fn f128_store_acc_to_dest(&mut self, dest: &Value) { ... }
//     fn f128_track_self(&mut self, dest_id: u32) { ... }
//     fn f128_set_acc_cache(&mut self, dest_id: u32) { ... }
//     fn f128_set_dyn_alloca(&mut self, _val: bool) -> bool { ... }
//     fn f128_move_callee_reg_to_addr_reg(&mut self, val_id: u32) -> bool { ... }
//     fn f128_load_indirect_ptr_to_addr_reg(&mut self, slot: StackSlot, val_id: u32) { ... }
//     fn f128_load_from_addr_reg_to_acc(&mut self, dest: &Value) { ... }
//     fn f128_load_from_direct_slot_to_acc(&mut self, slot: StackSlot) { ... }
//     fn f128_store_result_and_truncate(&mut self, dest: &Value) { ... }
//     fn f128_move_acc_to_arg0(&mut self) { ... }
//     fn f128_move_arg0_to_acc(&mut self) { ... }
//     fn f128_load_operand_to_acc(&mut self, op: &Operand) { ... }
//     fn f128_sign_extend_acc(&mut self, from_size: usize) { ... }
//     fn f128_zero_extend_acc(&mut self, from_size: usize) { ... }
//     fn f128_narrow_acc(&mut self, to_ty: IrType) { ... }
//     fn f128_extend_float_to_f128(&mut self, from_ty: IrType) { ... }
//     fn f128_truncate_to_float_acc(&mut self, to_ty: IrType) { ... }
//     fn f128_is_alloca(&self, val_id: u32) -> bool { ... }
// }
//
// impl RiscvCodegen {
//     pub(super) fn emit_f128_operand_to_a0_a1(&mut self, op: &Operand) { ... }
//     pub(super) fn emit_f128_neg_full(&mut self, dest: &Value, src: &Operand) { ... }
// }

#include "riscv_codegen.hpp"
#include "../../f128_softfloat.hpp"

namespace c4c::backend::riscv::codegen {

bool RiscvCodegen::f128_try_get_frame_offset(int value_id, std::int64_t& offset) const {
  if (const auto slot = state.get_slot(value_id)) {
    offset = slot->raw;
    return true;
  }
  return false;
}

void RiscvCodegen::f128_load_from_frame_offset_to_arg1(std::int64_t offset) {
  emit_load_from_s0("a0", offset, "ld");
  emit_load_from_s0("a1", offset + 8, "ld");
}

void RiscvCodegen::f128_load_from_addr_reg_to_arg1() {
  state.emit("    ld a0, 0(t5)");
  state.emit("    ld a1, 8(t5)");
}

void RiscvCodegen::f128_load_operand_and_extend(const Operand& src) {
  operand_to_t0(src);
  state.emit("    fmv.d.x fa0, t0");
  state.emit("    call __extenddftf2");
}

void RiscvCodegen::f128_flip_sign_bit() {
  state.emit("    li t0, 1");
  state.emit("    slli t0, t0, 63");
  state.emit("    xor a1, a1, t0");
}

void RiscvCodegen::f128_store_arg1_to_frame_offset(std::int64_t offset) {
  emit_store_to_s0("a0", offset, "sd");
  emit_store_to_s0("a1", offset + 8, "sd");
}

void RiscvCodegen::f128_store_arg1_to_addr() {
  state.emit("    sd a0, 0(t5)");
  state.emit("    sd a1, 8(t5)");
}

void RiscvCodegen::f128_track_self(int dest_id) { static_cast<void>(dest_id); }

void RiscvCodegen::f128_truncate_result_to_acc() {
  state.emit("    call __trunctfdf2");
  state.emit("    fmv.x.d t0, fa0");
}

void RiscvCodegen::f128_set_acc_cache(int dest_id) {
  if (const auto reg = state.assigned_reg(dest_id)) {
    state.emit("    mv " + std::string(callee_saved_name(*reg)) + ", t0");
  }
}

void RiscvCodegen::f128_alloc_temp_16() { state.emit("    addi sp, sp, -16"); }

void RiscvCodegen::f128_save_arg1_to_sp() {
  state.emit("    sd a0, 0(sp)");
  state.emit("    sd a1, 8(sp)");
}

void RiscvCodegen::f128_move_arg1_to_arg2() {
  state.emit("    mv a2, a0");
  state.emit("    mv a3, a1");
}

void RiscvCodegen::f128_reload_arg1_from_sp() {
  state.emit("    ld a0, 0(sp)");
  state.emit("    ld a1, 8(sp)");
}

void RiscvCodegen::f128_free_temp_16() { state.emit("    addi sp, sp, 16"); }

void RiscvCodegen::f128_call(const char* name) { state.emit(std::string("    call ") + name); }

void RiscvCodegen::f128_cmp_result_to_bool(c4c::backend::F128CmpKind kind) {
  switch (kind) {
    case c4c::backend::F128CmpKind::Eq:
      state.emit("    seqz t0, a0");
      return;
    case c4c::backend::F128CmpKind::Ne:
      state.emit("    snez t0, a0");
      return;
    case c4c::backend::F128CmpKind::Lt:
      state.emit("    slt t0, a0, x0");
      return;
    case c4c::backend::F128CmpKind::Le:
      state.emit("    slt t0, x0, a0");
      state.emit("    xori t0, t0, 1");
      return;
    case c4c::backend::F128CmpKind::Gt:
      state.emit("    slt t0, x0, a0");
      return;
    case c4c::backend::F128CmpKind::Ge:
      state.emit("    slt t0, a0, x0");
      state.emit("    xori t0, t0, 1");
      return;
  }
}

void RiscvCodegen::f128_store_bool_result(const Value& dest) { store_t0_to(dest); }

void RiscvCodegen::f128_load_operand_to_acc(const Operand& operand) { operand_to_t0(operand); }

void RiscvCodegen::f128_sign_extend_acc(std::size_t from_size) {
  switch (from_size) {
    case 1:
      state.emit("    slli t0, t0, 56");
      state.emit("    srai t0, t0, 56");
      return;
    case 2:
      state.emit("    slli t0, t0, 48");
      state.emit("    srai t0, t0, 48");
      return;
    case 4:
      state.emit("    sext.w t0, t0");
      return;
    default: return;
  }
}

void RiscvCodegen::f128_zero_extend_acc(std::size_t from_size) {
  switch (from_size) {
    case 1:
      state.emit("    andi t0, t0, 0xff");
      return;
    case 2:
      state.emit("    slli t0, t0, 48");
      state.emit("    srli t0, t0, 48");
      return;
    case 4:
      state.emit("    slli t0, t0, 32");
      state.emit("    srli t0, t0, 32");
      return;
    default: return;
  }
}

void RiscvCodegen::f128_move_acc_to_arg0() { state.emit("    mv a0, t0"); }

void RiscvCodegen::f128_store_result_and_truncate(const Value& dest) {
  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("a0", slot->raw, "sd");
    emit_store_to_s0("a1", slot->raw + 8, "sd");
  }

  state.emit("    call __trunctfdf2");
  state.emit("    fmv.x.d t0, fa0");

  if (const auto reg = state.assigned_reg(dest.raw)) {
    state.emit("    mv " + std::string(callee_saved_name(*reg)) + ", t0");
  }
}

void RiscvCodegen::f128_move_arg0_to_acc() { state.emit("    mv t0, a0"); }

void RiscvCodegen::f128_narrow_acc(IrType to_ty) {
  switch (to_ty) {
    case IrType::I8:
      state.emit("    slli t0, t0, 56");
      state.emit("    srai t0, t0, 56");
      return;
    case IrType::U16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srli t0, t0, 48");
      return;
    case IrType::I16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srai t0, t0, 48");
      return;
    case IrType::U32:
      state.emit("    slli t0, t0, 32");
      state.emit("    srli t0, t0, 32");
      return;
    case IrType::I32:
      state.emit("    sext.w t0, t0");
      return;
    default: return;
  }
}

void RiscvCodegen::f128_store_acc_to_dest(const Value& dest) { store_t0_to(dest); }

void RiscvCodegen::f128_extend_float_to_f128(IrType from_ty) {
  if (from_ty == IrType::F32) {
    state.emit("    fmv.w.x fa0, t0");
    state.emit("    call __extendsftf2");
    return;
  }

  state.emit("    fmv.d.x fa0, t0");
  state.emit("    call __extenddftf2");
}

void RiscvCodegen::f128_truncate_to_float_acc(IrType to_ty) {
  if (to_ty == IrType::F32) {
    state.emit("    call __trunctfsf2");
    state.emit("    fmv.x.w t0, fa0");
    return;
  }

  state.emit("    call __trunctfdf2");
  state.emit("    fmv.x.d t0, fa0");
}

void RiscvCodegen::emit_f128_operand_to_a0_a1(const Operand& src) {
  c4c::backend::emit_f128_operand_to_arg1(*this, src);
}

void RiscvCodegen::emit_f128_neg_full(const Value& dest, const Operand& src) {
  c4c::backend::emit_f128_neg(*this, dest, src);
}

void RiscvCodegen::emit_f128_cmp_impl(const Value& dest,
                                      IrCmpOp op,
                                      const Operand& lhs,
                                      const Operand& rhs) {
  c4c::backend::emit_riscv_f128_cmp(*this, dest, op, lhs, rhs);
}

}  // namespace c4c::backend::riscv::codegen
