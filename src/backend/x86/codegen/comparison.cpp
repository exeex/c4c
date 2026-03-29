#include "emit.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_float_cmp_impl(const Value& dest,
                                     IrCmpOp op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty) {
  if (ty == IrType::F128) {
    this->emit_f128_load_to_x87(&lhs);
    this->emit_f128_load_to_x87(&rhs);
    this->state.emit("    <x87-compare>");
    this->store_rax_to(dest);
    return;
  }

  // The scalar float path compares through xmm registers and materializes a
  // boolean result in the accumulator.
  this->operand_to_rax(lhs);
  this->operand_to_rcx(rhs);
  this->state.emit("    <float-compare>");
  this->store_rax_to(dest);
}

void X86Codegen::emit_f128_cmp_impl(const Value& dest,
                                    IrCmpOp op,
                                    const Operand& lhs,
                                    const Operand& rhs) {
  this->emit_f128_load_to_x87(&lhs);
  this->emit_f128_load_to_x87(&rhs);
  this->state.emit("    <long-double-compare>");
  this->store_rax_to(dest);
}

void X86Codegen::emit_int_cmp_impl(const Value& dest,
                                   IrCmpOp op,
                                   const Operand& lhs,
                                   const Operand& rhs,
                                   IrType ty) {
  this->operand_to_rax(lhs);
  this->operand_to_rcx(rhs);
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    <cmp32>"
                                                           : "    <cmp64>");
  this->store_rax_to(dest);
}

void X86Codegen::emit_fused_cmp_branch_impl(IrCmpOp op,
                                            const Operand& lhs,
                                            const Operand& rhs,
                                            BlockId true_block,
                                            BlockId false_block,
                                            IrType ty) {
  // The ref backend fuses compare+branch when it can; the translation keeps the
  // same decision point but does not expand every exact flag pattern here.
  this->state.emit("    <fused-compare-and-branch>");
  this->emit_cond_branch_blocks_impl(&lhs, true_block, false_block);
}

void X86Codegen::emit_fused_cmp_branch_blocks_impl(IrCmpOp op,
                                                   const Operand& lhs,
                                                   const Operand& rhs,
                                                   BlockId true_block,
                                                   BlockId false_block,
                                                   IrType ty) {
  this->emit_fused_cmp_branch_impl(op, lhs, rhs, true_block, false_block, ty);
}

void X86Codegen::emit_cond_branch_blocks_impl(const Operand& cond,
                                              BlockId true_block,
                                              BlockId false_block) {
  this->state.emit("    <conditional-branch>");
}

void X86Codegen::emit_select_impl(const Value& dest,
                                  const Operand& cond,
                                  const Operand& true_val,
                                  const Operand& false_val,
                                  IrType /*ty*/) {
  this->state.emit("    <select>");
  this->store_rax_to(dest);
}

}  // namespace c4c::backend::x86
