// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/f128.rs
// Structural mirror of the ref Rust source; a shared RISC-V C++ codegen
// header is not present yet, so this file keeps the translation local and
// method-shaped without inventing a fake public interface.
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

namespace c4c::backend::riscv::codegen {

}  // namespace c4c::backend::riscv::codegen
