// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/atomics.rs
// Structural mirror of the ref Rust source; a dedicated RISC-V C++ codegen
// header is not present yet, so this file keeps the translation local and
// method-shaped without inventing a fake public interface.
//
// //! RISC-V sub-word atomic operations, atomic load/store/RMW/CAS, fence,
// //! and software implementations of CLZ/CTZ/BSWAP/POPCOUNT builtins.
// //
// // use crate::ir::reexports::{AtomicOrdering, AtomicRmwOp, Operand, Value};
// // use crate::common::types::IrType;
// // use crate::backend::state::CodegenState;
// // use super::emit::RiscvCodegen;
// //
// // impl RiscvCodegen {
// //     pub(super) fn amo_ordering(ordering: AtomicOrdering) -> &'static str { ... }
// //     pub(super) fn amo_width_suffix(ty: IrType) -> &'static str { ... }
// //     pub(super) fn sign_extend_riscv(state: &mut CodegenState, ty: IrType) { ... }
// //     pub(super) fn is_subword_type(ty: IrType) -> bool { ... }
// //     pub(super) fn subword_bits(ty: IrType) -> u32 { ... }
// //     pub(super) fn emit_subword_atomic_rmw(&mut self, op: AtomicRmwOp, ty: IrType, aq_rl: &str) { ... }
// //     pub(super) fn emit_subword_atomic_cmpxchg(&mut self, ty: IrType, aq_rl: &str, returns_bool: bool) { ... }
// //     pub(super) fn emit_clz(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_ctz(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_bswap(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_popcount(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_atomic_rmw_impl(&mut self, dest: &Value, op: AtomicRmwOp, ptr: &Operand, val: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_atomic_cmpxchg_impl(&mut self, dest: &Value, ptr: &Operand, expected: &Operand, desired: &Operand, ty: IrType, ordering: AtomicOrdering, _failure_ordering: AtomicOrdering, returns_bool: bool) { ... }
// //     pub(super) fn emit_atomic_load_impl(&mut self, dest: &Value, ptr: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_atomic_store_impl(&mut self, ptr: &Operand, val: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_fence_impl(&mut self, ordering: AtomicOrdering) { ... }
// // }

namespace c4c::backend::riscv::codegen {

}  // namespace c4c::backend::riscv::codegen
