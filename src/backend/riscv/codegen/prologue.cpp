// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/prologue.rs
// Structural mirror of the ref Rust source; the shared C++ `RiscvCodegen`
// / `CodegenState` surface does not exist yet, so this file stays as a
// source-level inventory of the prologue/epilogue and stack-frame methods.
//
// //! RiscvCodegen: prologue/epilogue and stack frame operations.
//
// use crate::ir::reexports::IrFunction;
// use crate::common::types::IrType;
// use crate::backend::generation::{calculate_stack_space_common, find_param_alloca};
// use crate::backend::call_abi::{ParamClass, classify_params};
// use super::emit::{
//     RiscvCodegen, callee_saved_name,
//     collect_inline_asm_callee_saved_riscv, RISCV_CALLEE_SAVED, CALL_TEMP_CALLEE_SAVED,
//     RISCV_ARG_REGS,
// };
//
// impl RiscvCodegen {
//     // ---- calculate_stack_space ----
//
//     pub(super) fn calculate_stack_space_impl(&mut self, func: &IrFunction) -> i64 {
//         // Variadic handling updates va_* state before stack sizing.
//         // Inline-asm clobber scanning feeds register allocation.
//         // The allocator uses the RISC-V callee-saved pool plus call temps.
//         // Stack slot sizing honors alignment and regalloc results.
//         // Callee-saved spill space is appended at the end of the frame.
//     }
//
//     // ---- aligned_frame_size ----
//
//     pub(super) fn aligned_frame_size_impl(&self, raw_space: i64) -> i64 {
//         (raw_space + 15) & !15
//     }
//
//     // ---- emit_prologue ----
//
//     pub(super) fn emit_prologue_impl(&mut self, func: &IrFunction, frame_size: i64) {
//         self.current_return_type = func.return_type;
//         self.current_frame_size = frame_size;
//         self.emit_prologue_riscv(frame_size);
//         // Save callee-saved registers at the bottom of the frame.
//     }
//
//     // ---- emit_epilogue ----
//
//     pub(super) fn emit_epilogue_impl(&mut self, frame_size: i64) {
//         // Restore callee-saved registers before the epilogue sequence.
//         self.emit_epilogue_riscv(frame_size);
//     }
//
//     // ---- emit_store_params ----
//
//     pub(super) fn emit_store_params_impl(&mut self, func: &IrFunction) {
//         // Variadic functions spill a0-a7 into the register save area.
//         // Parameter classification determines whether each arg comes in GP,
//         // FP, pair, stack, or by-ref form.
//         // F128 arguments need a temporary save area before conversion.
//     }
//
//     /// Sign/zero-extend a GP register value to 64 bits for sub-64-bit types.
//     fn emit_extend_reg(state: &mut crate::backend::state::CodegenState, src: &str, dest: &str, ty: IrType) {
//         // I8/I16 use sign-extension, U8/U16 use zero-extension, I32 uses sext.w.
//         // Wider values are copied as-is.
//     }
//
//     // ---- emit_param_ref ----
//
//     pub(super) fn emit_param_ref_impl(&mut self, dest: &crate::ir::reexports::Value, param_idx: usize, ty: IrType) {
//         // Prefer the pre-spilled alloca slot when available, otherwise read
//         // directly from the ABI location described by the parameter class.
//     }
// }
